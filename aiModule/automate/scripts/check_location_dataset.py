"""Smoke-check the independent face-conditioned Location dataset."""

from pathlib import Path
import sys

import torch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import make_location_dataloader


def main():
    index = ROOT / "dataset" / "training" / "location_paper_full"
    dataset, loader = make_location_dataloader(
        index, split="validation", max_candidate_pairs=12000,
        max_samples=16, shuffle=False, num_workers=0,
    )
    batch = next(iter(loader))
    assert batch.candidate_pairs.shape[1] == 2
    assert batch.selected_faces.shape == (len(batch.sample_ids), 2)
    assert batch.sample_pair_offsets.shape[0] == len(batch.sample_ids) + 1
    assert int(batch.sample_pair_offsets[-1]) == batch.candidate_pairs.shape[0]
    assert int(batch.candidate_pairs[:, 0].max()) < batch.graph_a.mcfs.shape[0]
    assert int(batch.candidate_pairs[:, 1].max()) < batch.graph_b.mcfs.shape[0]
    assert int(batch.selected_faces[:, 0].max()) < batch.graph_a.faces.shape[0]
    assert int(batch.selected_faces[:, 1].max()) < batch.graph_b.faces.shape[0]
    for sample_index in range(len(batch.sample_ids)):
        mask = batch.pair_to_sample == sample_index
        assert bool(batch.pair_labels[mask].any())
    print(f"dataset_size={len(dataset)}")
    print(f"batch_samples={len(batch.sample_ids)}")
    print(f"candidate_pairs={batch.candidate_pairs.shape[0]}")
    print(f"positive_pairs={int(batch.pair_labels.sum())}")
    print(f"local_mcfs_a={batch.local_mcfs_a.numel()}")
    print(f"local_mcfs_b={batch.local_mcfs_b.numel()}")
    print("location_dataset_check=OK")


if __name__ == "__main__":
    main()
