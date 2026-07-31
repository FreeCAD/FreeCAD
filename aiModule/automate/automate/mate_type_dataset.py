"""Dataset and batching for mate-type prediction at a known MCF pair."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

import torch
from torch.utils.data import DataLoader, Dataset

from .brep import flatbatch
from .mate_dataset import GraphCache


@dataclass
class MateTypeSample:
    sample_id: str
    graph_a: object
    graph_b: object
    mcf_pair: torch.Tensor
    mate_type: int


@dataclass
class MateTypeBatch:
    graph_a: object
    graph_b: object
    mcf_pairs: torch.Tensor
    mate_types: torch.Tensor
    sample_ids: list[str]

    def to(self, device):
        self.graph_a = self.graph_a.to(device)
        self.graph_b = self.graph_b.to(device)
        self.mcf_pairs = self.mcf_pairs.to(device)
        self.mate_types = self.mate_types.to(device)
        return self


class MateTypeDataset(Dataset):
    """Load only the ground-truth MCF pair and its eight-class mate label."""

    def __init__(self, index_dir, split="train", graph_cache_size=32):
        self.index_dir = Path(index_dir)
        self.split = split
        self.summary = json.loads(
            (self.index_dir / "summary.json").read_text(encoding="utf-8")
        )
        self.cache_root = Path(self.summary["cache_root"])
        self.rows = [
            json.loads(line)
            for line in (self.index_dir / f"{split}.jsonl").open(encoding="utf-8")
            if line.strip()
        ]
        if not self.rows:
            raise ValueError(f"Mate Type split is empty: {split}")

        ordered_types = sorted(
            self.summary["mate_type_to_id"].items(), key=lambda item: item[1]
        )
        expected_ids = list(range(len(ordered_types)))
        actual_ids = [int(type_id) for _, type_id in ordered_types]
        if actual_ids != expected_ids:
            raise ValueError(
                f"mate_type_to_id must use contiguous IDs starting at zero: {actual_ids}"
            )
        self.mate_type_names = [name for name, _ in ordered_types]
        self.graph_cache = GraphCache(self.cache_root, graph_cache_size)

    def __len__(self):
        return len(self.rows)

    @staticmethod
    def _validate_mcf_index(graph, index, sample_id, side):
        index = int(index)
        if not 0 <= index < int(graph.mcfs.shape[0]):
            raise IndexError(
                f"MCF index {index} is out of range for side {side} of {sample_id} "
                f"(count={graph.mcfs.shape[0]})"
            )
        return index

    def __getitem__(self, index):
        row = self.rows[index]
        side_a, side_b = row["sides"]
        graph_a = self.graph_cache.load(side_a["cache"])
        graph_b = self.graph_cache.load(side_b["cache"])
        sample_id = str(row["sample_id"])
        mcf_a = self._validate_mcf_index(
            graph_a, side_a["candidate_index"], sample_id, "A"
        )
        mcf_b = self._validate_mcf_index(
            graph_b, side_b["candidate_index"], sample_id, "B"
        )
        mate_type = int(row["mate_type_id"])
        if not 0 <= mate_type < len(self.mate_type_names):
            raise ValueError(f"Invalid Mate Type ID {mate_type} for {sample_id}")
        return MateTypeSample(
            sample_id=sample_id,
            graph_a=graph_a,
            graph_b=graph_b,
            mcf_pair=torch.tensor([mcf_a, mcf_b], dtype=torch.long),
            mate_type=mate_type,
        )


def collate_mate_types(samples):
    if not samples:
        raise ValueError("Cannot collate an empty Mate Type batch")
    graph_a = flatbatch([sample.graph_a for sample in samples])
    graph_b = flatbatch([sample.graph_b for sample in samples])

    offsets_a = []
    offsets_b = []
    offset_a = 0
    offset_b = 0
    for sample in samples:
        offsets_a.append(offset_a)
        offsets_b.append(offset_b)
        offset_a += int(sample.graph_a.mcfs.shape[0])
        offset_b += int(sample.graph_b.mcfs.shape[0])

    pairs = torch.stack([sample.mcf_pair for sample in samples])
    pairs = pairs + torch.tensor(list(zip(offsets_a, offsets_b)), dtype=torch.long)
    return MateTypeBatch(
        graph_a=graph_a,
        graph_b=graph_b,
        mcf_pairs=pairs,
        mate_types=torch.tensor([sample.mate_type for sample in samples], dtype=torch.long),
        sample_ids=[sample.sample_id for sample in samples],
    )


def make_mate_type_dataloader(
    index_dir,
    split="train",
    batch_size=4,
    shuffle=None,
    num_workers=0,
    seed=20260730,
    **kwargs,
):
    dataset = MateTypeDataset(index_dir=index_dir, split=split)
    if shuffle is None:
        shuffle = split == "train"
    generator = torch.Generator().manual_seed(seed)
    loader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=shuffle,
        num_workers=num_workers,
        collate_fn=collate_mate_types,
        generator=generator,
        **kwargs,
    )
    return dataset, loader
