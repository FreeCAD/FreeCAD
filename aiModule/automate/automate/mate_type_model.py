"""Independent SB-GCN classifier for mate type at a known MCF pair."""

from __future__ import annotations

from dataclasses import asdict, dataclass

import torch
from torch import nn
from torch.nn import functional as F

from .sbgcn import LinearBlock, SBGCN


@dataclass
class MateTypeModelConfig:
    face_width: int
    loop_width: int
    edge_width: int
    vertex_width: int
    graph_width: int = 64
    mcf_width: int = 64
    # In the paper, k=6 is the number of inner Face-to-Face SB-GCN layers.
    message_passing_steps: int = 6
    inference_types: int = 32
    inference_embedding_width: int = 8
    dropout: float = 0.1
    num_mate_types: int = 8
    normalize_graph_inputs: bool = True

    def to_dict(self):
        return asdict(self)


class MateTypeModel(nn.Module):
    """Classify one known MCF pair with a shared dual-side SB-GCN encoder."""

    def __init__(self, config: MateTypeModelConfig):
        super().__init__()
        if config.num_mate_types != 8:
            raise ValueError("The paper Mate Type task requires exactly eight classes")
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
            scale = torch.linalg.vector_norm(
                maximum - minimum, dim=1, keepdim=True
            ).clamp_min(1.0e-9)
            origins = (origins - 0.5 * (minimum + maximum)) / scale
        return torch.cat((axes, origins), dim=1)

    def encode_graph(self, graph):
        topology, parts = self.encoder(graph)[:2]
        axis_topology = topology[graph.mcf_refs[0]]
        origin_topology = topology[graph.mcf_refs[1]]
        graph_index = graph.mcf_to_graph_idx.flatten()
        part_context = parts[graph_index]
        inference_type = graph.mcf_refs[2].clamp(0, self.config.inference_types - 1)
        inference = self.inference_embedding(inference_type)
        geometry = self._normalized_mcf_geometry(graph)
        axis_topology = self._stable_layer_norm(axis_topology)
        origin_topology = self._stable_layer_norm(origin_topology)
        part_context = self._stable_layer_norm(part_context)
        encoded = self.mcf_encoder(
            torch.cat(
                (axis_topology, origin_topology, part_context, geometry, inference), dim=1
            )
        )
        encoded_scale = encoded.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
        return self.mcf_normalization(encoded / encoded_scale)

    @staticmethod
    def _pair_features(embeddings_a, embeddings_b, mcf_pairs):
        pair_a = embeddings_a[mcf_pairs[:, 0]]
        pair_b = embeddings_b[mcf_pairs[:, 1]]
        return torch.cat(
            (pair_a, pair_b, torch.abs(pair_a - pair_b), pair_a * pair_b), dim=1
        )

    def forward(self, batch):
        embeddings_a = self.encode_graph(batch.graph_a)
        embeddings_b = self.encode_graph(batch.graph_b)
        return self.type_head(
            self._pair_features(embeddings_a, embeddings_b, batch.mcf_pairs)
        )
