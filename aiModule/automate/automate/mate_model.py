"""Neural model for scoring pairs of AutoMate mating-coordinate frames."""

from __future__ import annotations

from dataclasses import asdict, dataclass

import torch
from torch import nn
from torch.nn import functional as F

from .sbgcn import LinearBlock, SBGCN


@dataclass
class MateModelConfig:
    face_width: int
    loop_width: int
    edge_width: int
    vertex_width: int
    graph_width: int = 64
    mcf_width: int = 64
    message_passing_steps: int = 2
    inference_types: int = 32
    inference_embedding_width: int = 8
    dropout: float = 0.1
    # Zero preserves compatibility with the frozen v1 location-only checkpoint.
    # Multitask v2 explicitly sets this to the eight AutoMate mate classes.
    num_mate_types: int = 0
    normalize_graph_inputs: bool = False

    def to_dict(self):
        return asdict(self)


@dataclass
class MateModelOutput:
    pair_logits: torch.Tensor
    type_logits: torch.Tensor | None


class MatePairModel(nn.Module):
    """Shared dual-side SB-GCN encoder followed by an MCF pair scorer."""

    def __init__(self, config: MateModelConfig):
        super().__init__()
        self.config = config
        self.encoder = SBGCN(
            f_in_width=config.face_width,
            l_in_width=config.loop_width,
            e_in_width=config.edge_width,
            v_in_width=config.vertex_width,
            out_width=config.graph_width,
            k=config.message_passing_steps,
            use_uvnet_features=False,
            normalize_inputs=config.normalize_graph_inputs,
        )
        self.inference_embedding = nn.Embedding(
            config.inference_types, config.inference_embedding_width
        )
        mcf_input_width = 3 * config.graph_width + 6 + config.inference_embedding_width
        self.mcf_encoder = LinearBlock(
            mcf_input_width,
            config.mcf_width,
            config.mcf_width,
            dropout=config.dropout,
        )
        self.mcf_normalization = nn.LayerNorm(config.mcf_width)
        self.pair_head = LinearBlock(
            4 * config.mcf_width,
            2 * config.mcf_width,
            config.mcf_width,
            1,
            dropout=config.dropout,
            last_linear=True,
        )
        self.type_head = None
        if config.num_mate_types > 0:
            self.type_head = LinearBlock(
                4 * config.mcf_width,
                2 * config.mcf_width,
                config.mcf_width,
                config.num_mate_types,
                dropout=config.dropout,
                last_linear=True,
            )

    @staticmethod
    def _stable_layer_norm(values):
        """Apply per-row layer normalization without overflowing its backward pass."""
        # Some valid B-Rep features lead to very large but still finite SB-GCN
        # activations. Native LayerNorm can then overflow while accumulating its
        # backward variance. Scaling is detached because it is purely numerical;
        # LayerNorm is otherwise invariant to a positive per-row scale.
        scale = values.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
        return F.layer_norm(values / scale, (values.shape[1],))

    @staticmethod
    def _normalized_mcf_geometry(graph):
        graph_index = graph.mcf_to_graph_idx.flatten()
        axes = F.normalize(graph.mcfs[:, :3], dim=1)
        origins = graph.mcfs[:, 3:]
        part_features = graph.part_feat[graph_index]
        if part_features.shape[1] >= 11:
            minimum = part_features[:, 5:8]
            maximum = part_features[:, 8:11]
            scale = torch.linalg.vector_norm(maximum - minimum, dim=1, keepdim=True).clamp_min(1e-9)
            origins = (origins - 0.5 * (minimum + maximum)) / scale
        return torch.cat((axes, origins), dim=1)

    def encode_graph(self, graph):
        """Encode every MCF in one already-batched side graph."""
        topology, parts = self.encoder(graph)[:2]
        axis_topology = topology[graph.mcf_refs[0]]
        origin_topology = topology[graph.mcf_refs[1]]
        graph_index = graph.mcf_to_graph_idx.flatten()
        part_context = parts[graph_index]
        inference_type = graph.mcf_refs[2].clamp(0, self.config.inference_types - 1)
        inference = self.inference_embedding(inference_type)
        geometry = self._normalized_mcf_geometry(graph)
        # Legacy SB-GCN features contain physical quantities with very different
        # scales. Per-entity normalization prevents large parts from producing
        # extreme pair logits while retaining the learned feature direction.
        if self.config.normalize_graph_inputs:
            axis_topology = self._stable_layer_norm(axis_topology)
            origin_topology = self._stable_layer_norm(origin_topology)
            part_context = self._stable_layer_norm(part_context)
        else:
            # Preserve the frozen v1 computation path exactly.
            axis_topology = F.layer_norm(axis_topology, (axis_topology.shape[1],))
            origin_topology = F.layer_norm(origin_topology, (origin_topology.shape[1],))
            part_context = F.layer_norm(part_context, (part_context.shape[1],))
        encoded = self.mcf_encoder(
            torch.cat(
                (axis_topology, origin_topology, part_context, geometry, inference), dim=1
            )
        )
        if self.config.normalize_graph_inputs:
            encoded_scale = encoded.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
            encoded = encoded / encoded_scale
        return self.mcf_normalization(encoded)

    @staticmethod
    def _pair_features(embeddings_a, embeddings_b, candidate_pairs):
        pair_a = embeddings_a[candidate_pairs[:, 0]]
        pair_b = embeddings_b[candidate_pairs[:, 1]]
        return torch.cat(
            (pair_a, pair_b, torch.abs(pair_a - pair_b), pair_a * pair_b), dim=1
        )

    def score_encoded_pairs(self, embeddings_a, embeddings_b, candidate_pairs):
        """Score candidate index pairs using precomputed side embeddings."""
        pair_features = self._pair_features(
            embeddings_a, embeddings_b, candidate_pairs
        )
        return self.pair_head(pair_features).squeeze(1)

    def predict_encoded_pairs(self, embeddings_a, embeddings_b, candidate_pairs):
        """Return location and optional mate-type logits for candidate pairs."""
        pair_features = self._pair_features(
            embeddings_a, embeddings_b, candidate_pairs
        )
        return MateModelOutput(
            pair_logits=self.pair_head(pair_features).squeeze(1),
            type_logits=(
                self.type_head(pair_features) if self.type_head is not None else None
            ),
        )

    def forward_multitask(self, batch):
        embeddings_a = self.encode_graph(batch.graph_a)
        embeddings_b = self.encode_graph(batch.graph_b)
        return self.predict_encoded_pairs(
            embeddings_a, embeddings_b, batch.candidate_pairs
        )

    def forward(self, batch):
        return self.forward_multitask(batch).pair_logits
