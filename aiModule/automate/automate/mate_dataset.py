"""PyTorch datasets and batching for AutoMate mate prediction."""

from __future__ import annotations

import collections
import hashlib
import json
import math
import random
from dataclasses import dataclass
from pathlib import Path

import torch
from torch.utils.data import DataLoader, Dataset

from .brep import flatbatch


@dataclass
class MateSample:
    sample_id: str
    graph_a: object
    graph_b: object
    candidate_pairs: torch.Tensor
    pair_labels: torch.Tensor
    mate_type: int
    axial_offsets: torch.Tensor
    axis_flipped: torch.Tensor


@dataclass
class MateBatch:
    graph_a: object
    graph_b: object
    candidate_pairs: torch.Tensor
    pair_labels: torch.Tensor
    pair_to_sample: torch.Tensor
    positive_pair_indices: torch.Tensor
    mate_types: torch.Tensor
    axial_offsets: torch.Tensor
    axis_flipped: torch.Tensor
    sample_ids: list[str]

    def to(self, device):
        self.graph_a = self.graph_a.to(device)
        self.graph_b = self.graph_b.to(device)
        for name in (
            "candidate_pairs",
            "pair_labels",
            "pair_to_sample",
            "positive_pair_indices",
            "mate_types",
            "axial_offsets",
            "axis_flipped",
        ):
            setattr(self, name, getattr(self, name).to(device))
        return self


class GraphCache:
    """Small per-process LRU cache for deserialized graph objects."""

    def __init__(self, root, max_size=32):
        self.root = Path(root)
        self.max_size = max_size
        self.values = collections.OrderedDict()

    def load(self, relative_path):
        key = str(relative_path)
        if key in self.values:
            self.values.move_to_end(key)
            return self.values[key]
        payload = torch.load(self.root / relative_path, map_location="cpu", weights_only=False)
        graph = payload["graph"]
        self.values[key] = graph
        if len(self.values) > self.max_size:
            self.values.popitem(last=False)
        return graph


def _part_scale(graph):
    # part_feat layout: volume, surface area, center (3), AABB (2x3), inertia (9)
    if hasattr(graph, "part_feat") and graph.part_feat.shape[1] >= 11:
        minimum = graph.part_feat[0, 5:8]
        maximum = graph.part_feat[0, 8:11]
        diagonal = float(torch.linalg.vector_norm(maximum - minimum))
        if math.isfinite(diagonal) and diagonal > 1.0e-9:
            return diagonal
    return 1.0


def _rank_negative_candidates(graph, positive_index, rng):
    mcfs = graph.mcfs
    count = mcfs.shape[0]
    if count <= 1:
        return []

    positive = mcfs[positive_index]
    axes = torch.nn.functional.normalize(mcfs[:, :3], dim=1)
    positive_axis = torch.nn.functional.normalize(positive[:3], dim=0)
    absolute_dot = torch.abs(axes @ positive_axis).clamp(0.0, 1.0)
    angle = torch.acos(absolute_dot)
    delta = mcfs[:, 3:] - positive[3:]
    axial = (delta * axes).sum(dim=1, keepdim=True)
    line_distance = torch.linalg.vector_norm(delta - axial * axes, dim=1)

    equivalent = (angle <= math.radians(1.0)) & (line_distance <= 1.0e-4)
    equivalent[positive_index] = True
    scale = _part_scale(graph)
    score = angle + line_distance / scale
    valid_indices = torch.nonzero(~equivalent, as_tuple=False).flatten().tolist()
    # Stable random jitter prevents topology-order bias between equal candidates.
    ranked = sorted(valid_indices, key=lambda index: (float(score[index]), rng.random()))
    return ranked


def _negative_pairs(graph_a, graph_b, positive_a, positive_b, count, rng):
    ranked_a = _rank_negative_candidates(graph_a, positive_a, rng)
    ranked_b = _rank_negative_candidates(graph_b, positive_b, rng)
    pairs = []
    seen = {(positive_a, positive_b)}

    # Single-side substitutions are useful hard negatives.
    cursor = 0
    while len(pairs) < count and (cursor < len(ranked_a) or cursor < len(ranked_b)):
        if cursor < len(ranked_a):
            pair = (ranked_a[cursor], positive_b)
            if pair not in seen:
                seen.add(pair)
                pairs.append(pair)
        if len(pairs) >= count:
            break
        if cursor < len(ranked_b):
            pair = (positive_a, ranked_b[cursor])
            if pair not in seen:
                seen.add(pair)
                pairs.append(pair)
        cursor += 1

    # Fill remaining slots with two-sided combinations from the hard pools.
    pool_a = ranked_a[: max(count, 16)]
    pool_b = ranked_b[: max(count, 16)]
    attempts = 0
    while len(pairs) < count and pool_a and pool_b and attempts < count * 20:
        pair = (rng.choice(pool_a), rng.choice(pool_b))
        if pair not in seen:
            seen.add(pair)
            pairs.append(pair)
        attempts += 1
    return pairs


class MateDataset(Dataset):
    def __init__(
        self,
        index_dir,
        split="train",
        negative_count=15,
        seed=20260725,
        graph_cache_size=32,
    ):
        self.index_dir = Path(index_dir)
        self.split = split
        self.negative_count = negative_count
        self.seed = seed
        self.epoch = 0
        self.summary = json.loads((self.index_dir / "summary.json").read_text(encoding="utf-8"))
        self.cache_root = Path(self.summary["cache_root"])
        self.rows = [
            json.loads(line)
            for line in (self.index_dir / f"{split}.jsonl").open(encoding="utf-8")
        ]
        self.graph_cache = GraphCache(self.cache_root, graph_cache_size)
        self.class_weights = torch.tensor(
            [
                self.summary["class_weights"][name]
                for name, _ in sorted(self.summary["mate_type_to_id"].items(), key=lambda item: item[1])
            ],
            dtype=torch.float32,
        )

    def __len__(self):
        return len(self.rows)

    def set_epoch(self, epoch):
        self.epoch = int(epoch)

    def _rng(self, sample_id):
        digest = hashlib.sha1(f"{self.seed}:{self.epoch}:{sample_id}".encode()).digest()
        return random.Random(int.from_bytes(digest[:8], "big"))

    def __getitem__(self, index):
        row = self.rows[index]
        side_a, side_b = row["sides"]
        graph_a = self.graph_cache.load(side_a["cache"])
        graph_b = self.graph_cache.load(side_b["cache"])
        positive = (side_a["candidate_index"], side_b["candidate_index"])
        negatives = _negative_pairs(
            graph_a,
            graph_b,
            positive[0],
            positive[1],
            self.negative_count,
            self._rng(row["sample_id"]),
        )
        candidate_pairs = torch.tensor([positive, *negatives], dtype=torch.long)
        pair_labels = torch.zeros(candidate_pairs.shape[0], dtype=torch.float32)
        pair_labels[0] = 1.0
        return MateSample(
            sample_id=row["sample_id"],
            graph_a=graph_a,
            graph_b=graph_b,
            candidate_pairs=candidate_pairs,
            pair_labels=pair_labels,
            mate_type=int(row["mate_type_id"]),
            axial_offsets=torch.tensor(
                [side_a["axial_offset_m"], side_b["axial_offset_m"]], dtype=torch.float32
            ),
            axis_flipped=torch.tensor(
                [side_a["axis_flipped"], side_b["axis_flipped"]], dtype=torch.bool
            ),
        )


def collate_mates(samples):
    graph_a = flatbatch([sample.graph_a for sample in samples])
    graph_b = flatbatch([sample.graph_b for sample in samples])
    counts_a = [sample.graph_a.mcfs.shape[0] for sample in samples]
    counts_b = [sample.graph_b.mcfs.shape[0] for sample in samples]
    offsets_a = [0]
    offsets_b = [0]
    for count in counts_a[:-1]:
        offsets_a.append(offsets_a[-1] + count)
    for count in counts_b[:-1]:
        offsets_b.append(offsets_b[-1] + count)

    pairs = []
    labels = []
    pair_to_sample = []
    positive_pair_indices = []
    cursor = 0
    for sample_index, sample in enumerate(samples):
        adjusted = sample.candidate_pairs + torch.tensor(
            [offsets_a[sample_index], offsets_b[sample_index]], dtype=torch.long
        )
        positive_pair_indices.append(cursor)
        pairs.append(adjusted)
        labels.append(sample.pair_labels)
        pair_to_sample.append(torch.full((adjusted.shape[0],), sample_index, dtype=torch.long))
        cursor += adjusted.shape[0]

    return MateBatch(
        graph_a=graph_a,
        graph_b=graph_b,
        candidate_pairs=torch.cat(pairs, dim=0),
        pair_labels=torch.cat(labels, dim=0),
        pair_to_sample=torch.cat(pair_to_sample, dim=0),
        positive_pair_indices=torch.tensor(positive_pair_indices, dtype=torch.long),
        mate_types=torch.tensor([sample.mate_type for sample in samples], dtype=torch.long),
        axial_offsets=torch.stack([sample.axial_offsets for sample in samples]),
        axis_flipped=torch.stack([sample.axis_flipped for sample in samples]),
        sample_ids=[sample.sample_id for sample in samples],
    )


def make_mate_dataloader(
    index_dir,
    split="train",
    batch_size=4,
    shuffle=None,
    num_workers=0,
    negative_count=15,
    seed=20260725,
    **kwargs,
):
    dataset = MateDataset(
        index_dir=index_dir,
        split=split,
        negative_count=negative_count,
        seed=seed,
    )
    if shuffle is None:
        shuffle = split == "train"
    generator = torch.Generator().manual_seed(seed)
    loader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=shuffle,
        num_workers=num_workers,
        collate_fn=collate_mates,
        generator=generator,
        **kwargs,
    )
    return dataset, loader
