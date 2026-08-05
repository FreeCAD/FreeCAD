"""Paper-style face-conditioned SB-GCN model for MCF Location ranking."""

from __future__ import annotations

from dataclasses import asdict, dataclass

import torch
from torch import nn
from torch.nn import functional as F

from .sbgcn import LinearBlock, SBGCN


@dataclass
class LocationModelConfig:
    face_width: int
    loop_width: int
    edge_width: int
    vertex_width: int
    graph_width: int = 64
    mcf_width: int = 64
    # The paper uses six inner Face-to-Face SB-GCN layers.
    message_passing_steps: int = 6
    inference_types: int = 32
    inference_embedding_width: int = 8
    dropout: float = 0.1
    normalize_graph_inputs: bool = True
    normalize_intermediate: bool = True

    def to_dict(self):
        return asdict(self)


class LocationModel(nn.Module):
    """Rank local MCF pairs conditioned on one user-selected face per part."""

    def __init__(self, config: LocationModelConfig):
        super().__init__()
        if config.message_passing_steps != 6:
            raise ValueError("The paper Location configuration requires six SB-GCN Face-to-Face layers")
        self.config = config
        self.encoder = SBGCN(
            f_in_width=config.face_width, l_in_width=config.loop_width,
            e_in_width=config.edge_width, v_in_width=config.vertex_width,
            out_width=config.graph_width, k=config.message_passing_steps,
            use_uvnet_features=False, normalize_inputs=config.normalize_graph_inputs,
            normalize_intermediate=config.normalize_intermediate,
        )
        self.inference_embedding = nn.Embedding(config.inference_types, config.inference_embedding_width)
        # axis topology, origin topology, part, selected face, MCF geometry, inference type
        mcf_input_width = 4 * config.graph_width + 6 + config.inference_embedding_width
        self.mcf_encoder = LinearBlock(
            mcf_input_width, config.mcf_width, config.mcf_width,
            dropout=config.dropout,
        )
        self.mcf_normalization = nn.LayerNorm(config.mcf_width)
        self.pair_head = LinearBlock(
            4 * config.mcf_width, 2 * config.mcf_width,
            config.mcf_width, 1, dropout=config.dropout, last_linear=True,
        )

    @staticmethod
    def _stable_layer_norm(values):
        scale = values.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
        return F.layer_norm(values / scale, (values.shape[1],))

    @staticmethod
    def _normalized_geometry(graph, mcf_indices):
        graph_indices = graph.mcf_to_graph_idx.flatten()[mcf_indices]
        values = graph.mcfs[mcf_indices]
        axes = F.normalize(values[:, :3], dim=1)
        origins = values[:, 3:]
        part_features = graph.part_feat[graph_indices]
        if part_features.shape[1] >= 11:
            minimum, maximum = part_features[:, 5:8], part_features[:, 8:11]
            part_scale = torch.linalg.vector_norm(
                maximum - minimum, dim=1, keepdim=True
            ).clamp_min(1.0e-9)
            centered = origins - 0.5 * (minimum + maximum)
            # A few otherwise valid inferred MCFs contain finite float32 origins
            # near 1e38. Dividing those by a small part bbox overflows. Preserve
            # normal bbox scaling, but enlarge the denominator only as needed to
            # bound pathological normalized coordinates.
            overflow_scale = centered.abs().amax(dim=1, keepdim=True) / 1.0e4
            origins = centered / torch.maximum(part_scale, overflow_scale)
        return torch.cat((axes, origins), dim=1)

    def _encode_local_mcfs(self, graph, topology, parts, faces, selected_faces,
                           local_mcfs, local_to_sample):
        refs = graph.mcf_refs[:, local_mcfs]
        graph_indices = graph.mcf_to_graph_idx.flatten()[local_mcfs]
        axis_context = self._stable_layer_norm(topology[refs[0]])
        origin_context = self._stable_layer_norm(topology[refs[1]])
        part_context = self._stable_layer_norm(parts[graph_indices])
        face_context = self._stable_layer_norm(faces[selected_faces[local_to_sample]])
        inference = self.inference_embedding(refs[2].clamp(0, self.config.inference_types - 1))
        geometry = self._normalized_geometry(graph, local_mcfs)
        encoded = self.mcf_encoder(torch.cat((
            axis_context, origin_context, part_context, face_context, geometry, inference,
        ), dim=1))
        scale = encoded.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
        return self.mcf_normalization(encoded / scale)

    @staticmethod
    def _pair_features(encoded_a, encoded_b, local_pairs):
        side_a = encoded_a[local_pairs[:, 0]]
        side_b = encoded_b[local_pairs[:, 1]]
        return torch.cat((side_a, side_b, torch.abs(side_a - side_b), side_a * side_b), dim=1)

    def forward(self, batch):
        encoded_graph_a = self.encoder(batch.graph_a)
        encoded_graph_b = self.encoder(batch.graph_b)
        local_a = self._encode_local_mcfs(
            batch.graph_a, encoded_graph_a[0], encoded_graph_a[1], encoded_graph_a[2],
            batch.selected_faces[:, 0], batch.local_mcfs_a, batch.local_mcf_to_sample_a,
        )
        local_b = self._encode_local_mcfs(
            batch.graph_b, encoded_graph_b[0], encoded_graph_b[1], encoded_graph_b[2],
            batch.selected_faces[:, 1], batch.local_mcfs_b, batch.local_mcf_to_sample_b,
        )
        return self.pair_head(
            self._pair_features(local_a, local_b, batch.candidate_local_pairs)
        ).squeeze(1)


def multi_positive_location_loss(logits, positive_mask, sample_pair_offsets, reduction="mean"):
    """Negative log probability mass assigned to all equivalent positives."""
    if logits.ndim != 1 or positive_mask.shape != logits.shape:
        raise ValueError("logits and positive_mask must be equal-length vectors")
    if sample_pair_offsets.ndim != 1 or sample_pair_offsets.numel() < 2:
        raise ValueError("sample_pair_offsets must contain at least [0, end]")
    if int(sample_pair_offsets[0]) != 0 or int(sample_pair_offsets[-1]) != logits.numel():
        raise ValueError("sample_pair_offsets do not cover all logits")
    losses = []
    for start_tensor, end_tensor in zip(sample_pair_offsets[:-1], sample_pair_offsets[1:]):
        start, end = int(start_tensor), int(end_tensor)
        labels = positive_mask[start:end].bool()
        if end <= start or not bool(labels.any()):
            raise ValueError("Every Location sample must contain at least one positive candidate")
        sample_logits = logits[start:end]
        losses.append(torch.logsumexp(sample_logits, dim=0) - torch.logsumexp(sample_logits[labels], dim=0))
    losses = torch.stack(losses)
    if reduction == "none": return losses
    if reduction == "sum": return losses.sum()
    if reduction == "mean": return losses.mean()
    raise ValueError(f"Unsupported reduction: {reduction}")


def location_ranking_statistics(logits, positive_mask, sample_pair_offsets):
    """Return additive Top-k/MRR statistics using the best equivalent positive."""
    totals = {"samples": 0, "top1": 0, "top5": 0, "reciprocal_rank": 0.0,
              "rank_sum": 0.0, "candidate_pairs": 0, "positive_pairs": 0}
    for start_tensor, end_tensor in zip(sample_pair_offsets[:-1], sample_pair_offsets[1:]):
        start, end = int(start_tensor), int(end_tensor)
        labels = positive_mask[start:end].bool()
        if end <= start or not bool(labels.any()):
            raise ValueError("Every Location sample must contain at least one positive candidate")
        order = torch.argsort(logits[start:end], descending=True)
        best_rank = int(torch.nonzero(labels[order], as_tuple=False)[0]) + 1
        totals["samples"] += 1
        totals["top1"] += int(best_rank <= 1)
        totals["top5"] += int(best_rank <= 5)
        totals["reciprocal_rank"] += 1.0 / best_rank
        totals["rank_sum"] += best_rank
        totals["candidate_pairs"] += end - start
        totals["positive_pairs"] += int(labels.sum())
    return totals
