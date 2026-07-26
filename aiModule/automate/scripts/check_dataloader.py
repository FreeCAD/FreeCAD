"""Smoke-test MateDataset, batching, and an SB-GCN forward pass."""

from pathlib import Path
import sys

import torch

ROOT = Path(__file__).resolve().parent.parent
INDEX = ROOT / "dataset" / "training" / "index_v1"
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import SBGCN, make_mate_dataloader


def main():
    dataset, loader = make_mate_dataloader(
        INDEX,
        split="train",
        batch_size=4,
        shuffle=False,
        num_workers=0,
        negative_count=15,
    )
    batch = next(iter(loader))
    assert len(batch.sample_ids) == 4
    assert batch.candidate_pairs.shape == (64, 2)
    assert batch.pair_labels.sum().item() == 4
    assert torch.equal(batch.pair_labels[batch.positive_pair_indices], torch.ones(4))
    assert batch.axial_offsets.shape == (4, 2)
    assert batch.axis_flipped.shape == (4, 2)
    assert int(batch.candidate_pairs[:, 0].max()) < batch.graph_a.mcfs.shape[0]
    assert int(batch.candidate_pairs[:, 1].max()) < batch.graph_b.mcfs.shape[0]

    model = SBGCN(
        f_in_width=batch.graph_a.faces.shape[1],
        l_in_width=batch.graph_a.loops.shape[1],
        e_in_width=batch.graph_a.edges.shape[1],
        v_in_width=batch.graph_a.vertices.shape[1],
        out_width=32,
        k=2,
        use_uvnet_features=False,
    )
    model.eval()
    with torch.no_grad():
        encoded = model(batch.graph_a)
    topology_features, part_features = encoded[:2]
    assert topology_features.shape[1] == 32
    assert part_features.shape == (4, 32)

    print(f"dataset_size={len(dataset)}")
    print(f"batch_size={len(batch.sample_ids)}")
    print(f"graph_a_mcfs={batch.graph_a.mcfs.shape[0]}")
    print(f"graph_b_mcfs={batch.graph_b.mcfs.shape[0]}")
    print(f"candidate_pairs={batch.candidate_pairs.shape[0]}")
    print(f"positive_pairs={int(batch.pair_labels.sum())}")
    print(f"topology_features={tuple(topology_features.shape)}")
    print(f"part_features={tuple(part_features.shape)}")
    print("dataloader_check=OK")


if __name__ == "__main__":
    main()
