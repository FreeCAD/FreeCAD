"""Check paper Location model forward/backward and multi-positive loss."""

import math
from pathlib import Path
import sys

import torch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import (
    LocationModel, LocationModelConfig, location_ranking_statistics, make_location_dataloader,
    multi_positive_location_loss,
)


def main():
    # Exact analytical check: uniform logits assign mass positives/candidates.
    logits = torch.zeros(5, requires_grad=True)
    labels = torch.tensor([1, 0, 1, 0, 0], dtype=torch.bool)
    loss = multi_positive_location_loss(logits, labels, torch.tensor([0, 5]))
    assert math.isclose(float(loss), math.log(5 / 2), rel_tol=1e-6)
    loss.backward()
    assert bool(torch.isfinite(logits.grad).all())
    ranking = location_ranking_statistics(
        torch.tensor([0.1, 0.9, 0.8, 0.7, 0.6]), labels, torch.tensor([0, 5])
    )
    assert ranking["top1"] == 0 and ranking["top5"] == 1
    assert math.isclose(ranking["reciprocal_rank"], 0.5)

    _, loader = make_location_dataloader(
        ROOT / "dataset/training/location_paper_full", split="validation",
        max_candidate_pairs=2000, max_samples=2, shuffle=False, num_workers=0,
    )
    batch = next(iter(loader))
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    config = LocationModelConfig(
        face_width=batch.graph_a.faces.shape[1], loop_width=batch.graph_a.loops.shape[1],
        edge_width=batch.graph_a.edges.shape[1], vertex_width=batch.graph_a.vertices.shape[1],
    )
    model = LocationModel(config).to(device)
    batch = batch.to(device)
    model.train()
    pair_logits = model(batch)
    assert pair_logits.shape == batch.pair_labels.shape
    train_loss = multi_positive_location_loss(
        pair_logits, batch.pair_labels, batch.sample_pair_offsets
    )
    assert bool(torch.isfinite(train_loss))
    train_loss.backward()
    gradients = [parameter.grad for parameter in model.parameters() if parameter.grad is not None]
    assert gradients and all(bool(torch.isfinite(gradient).all()) for gradient in gradients)
    print(f"device={device}")
    print(f"message_passing_steps={config.message_passing_steps}")
    print(f"samples={len(batch.sample_ids)}")
    print(f"candidate_pairs={pair_logits.numel()}")
    print(f"positive_pairs={int(batch.pair_labels.sum())}")
    print(f"loss={float(train_loss.detach()):.6f}")
    print("finite_gradients=OK")
    print("location_model_check=OK")


if __name__ == "__main__":
    main()
