"""Smoke-test the v2 eight-class mate-type head and v1 compatibility."""

from __future__ import annotations

import sys
from pathlib import Path

import torch
from torch.nn import functional as F


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateModelConfig, MatePairModel, make_mate_dataloader


def config_from_batch(batch, num_mate_types):
    return MateModelConfig(
        face_width=batch.graph_a.faces.shape[1],
        loop_width=batch.graph_a.loops.shape[1],
        edge_width=batch.graph_a.edges.shape[1],
        vertex_width=batch.graph_a.vertices.shape[1],
        graph_width=32,
        mcf_width=32,
        message_passing_steps=1,
        num_mate_types=num_mate_types,
        normalize_graph_inputs=num_mate_types > 0,
    )


def main():
    dataset, loader = make_mate_dataloader(
        ROOT / "dataset/training/index_v1",
        split="train",
        batch_size=2,
        shuffle=False,
        num_workers=0,
        negative_count=15,
    )
    batch = next(iter(loader))
    model = MatePairModel(config_from_batch(batch, num_mate_types=8))
    output = model.forward_multitask(batch)
    assert output.pair_logits.shape == (32,)
    assert output.type_logits.shape == (32, 8)
    # Only the known-positive pair from each sample has a mate-type label.
    positive_type_logits = output.type_logits[batch.positive_pair_indices]
    assert positive_type_logits.shape == (2, 8)
    location_loss = F.binary_cross_entropy_with_logits(
        output.pair_logits, batch.pair_labels, pos_weight=torch.tensor(15.0)
    )
    type_loss = F.cross_entropy(
        positive_type_logits,
        batch.mate_types,
        weight=dataset.class_weights,
    )
    total_loss = location_loss + type_loss
    total_loss.backward()
    assert model.type_head is not None
    assert all(parameter.grad is not None for parameter in model.type_head.parameters())

    # A config loaded from the frozen v1 checkpoint does not construct a new
    # head, so its original state_dict remains strictly loadable.
    checkpoint = torch.load(
        ROOT / "runs/mate_pair_v1/best.pt", map_location="cpu", weights_only=False
    )
    v1_model = MatePairModel(MateModelConfig(**checkpoint["model_config"]))
    v1_model.load_state_dict(checkpoint["model_state"], strict=True)
    v1_model.eval()
    with torch.no_grad():
        v1_output = v1_model.forward_multitask(batch)
    assert v1_model.type_head is None
    assert v1_output.type_logits is None
    assert v1_output.pair_logits.shape == (32,)

    print(f"candidate_pairs={output.pair_logits.shape[0]}")
    print(f"positive_type_labels={positive_type_logits.shape[0]}")
    print(f"mate_classes={positive_type_logits.shape[1]}")
    print(f"location_loss={float(location_loss.detach()):.6f}")
    print(f"type_loss={float(type_loss.detach()):.6f}")
    print("v1_strict_checkpoint_load=OK")
    print("multitask_model_check=OK")


if __name__ == "__main__":
    main()
