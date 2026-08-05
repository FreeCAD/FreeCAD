"""Independent dataset and candidate-budget batching for MCF Location."""

from __future__ import annotations

import json
import os
import random
from dataclasses import dataclass
from pathlib import Path

import torch
from torch.utils.data import BatchSampler, DataLoader, Dataset

from .brep import flatbatch
from .mate_dataset import GraphCache


ROOT = Path(__file__).resolve().parent.parent


class JsonlRowStore:
    """Random access to JSONL without retaining the decoded rows in memory."""

    def __init__(self, path):
        self.path = Path(path)
        self.offsets = []
        self.candidate_counts = []
        offset = 0
        with self.path.open("rb") as stream:
            for line in stream:
                if line.strip():
                    row = json.loads(line)
                    self.offsets.append(offset)
                    self.candidate_counts.append(int(row["candidate_pair_count"]))
                offset += len(line)
        self._stream = None
        self._pid = None

    def __len__(self):
        return len(self.offsets)

    def __getitem__(self, index):
        pid = os.getpid()
        if self._stream is None or self._pid != pid:
            if self._stream is not None:
                self._stream.close()
            self._stream = self.path.open("rb")
            self._pid = pid
        self._stream.seek(self.offsets[index])
        return json.loads(self._stream.readline())

    def __getstate__(self):
        state = self.__dict__.copy()
        state["_stream"] = None
        state["_pid"] = None
        return state

    def __del__(self):
        stream = getattr(self, "_stream", None)
        if stream is not None:
            stream.close()


@dataclass
class LocationSample:
    sample_id: str
    mate_type: str
    graph_a: object
    graph_b: object
    selected_faces: torch.Tensor
    local_mcfs_a: torch.Tensor
    local_mcfs_b: torch.Tensor
    candidate_pairs: torch.Tensor
    pair_labels: torch.Tensor


@dataclass
class LocationBatch:
    graph_a: object
    graph_b: object
    selected_faces: torch.Tensor
    local_mcfs_a: torch.Tensor
    local_mcfs_b: torch.Tensor
    local_mcf_to_sample_a: torch.Tensor
    local_mcf_to_sample_b: torch.Tensor
    candidate_pairs: torch.Tensor
    candidate_local_pairs: torch.Tensor
    pair_labels: torch.Tensor
    pair_to_sample: torch.Tensor
    sample_pair_offsets: torch.Tensor
    sample_ids: list[str]
    mate_types: list[str]

    def to(self, device):
        self.graph_a = self.graph_a.to(device)
        self.graph_b = self.graph_b.to(device)
        for name in (
            "selected_faces", "local_mcfs_a", "local_mcfs_b",
            "local_mcf_to_sample_a", "local_mcf_to_sample_b",
            "candidate_pairs", "candidate_local_pairs", "pair_labels", "pair_to_sample",
            "sample_pair_offsets",
        ):
            setattr(self, name, getattr(self, name).to(device))
        return self


class LocationDataset(Dataset):
    def __init__(self, index_dir, split="train", graph_cache_size=32):
        self.index_dir = Path(index_dir)
        self.split = split
        self.summary = json.loads((self.index_dir / "summary.json").read_text(encoding="utf-8"))
        cache_root = Path(self.summary["cache_root"])
        self.cache_root = cache_root if cache_root.is_absolute() else ROOT / cache_root
        self.rows = JsonlRowStore(self.index_dir / f"{split}.jsonl")
        if not self.rows:
            raise ValueError(f"Location split is empty: {split}")
        self.candidate_counts = self.rows.candidate_counts
        self.graph_cache = GraphCache(self.cache_root, graph_cache_size)

    def __len__(self):
        return len(self.rows)

    @staticmethod
    def _validate_indices(values, upper_bound, name, sample_id):
        result = torch.tensor(values, dtype=torch.long)
        if result.numel() == 0:
            raise ValueError(f"Empty {name} for {sample_id}")
        if int(result.min()) < 0 or int(result.max()) >= int(upper_bound):
            raise IndexError(f"{name} out of range for {sample_id}")
        if result.unique().numel() != result.numel():
            raise ValueError(f"Duplicate {name} for {sample_id}")
        return result

    def __getitem__(self, index):
        row = self.rows[index]
        sample_id = str(row["sample_id"])
        side_a, side_b = row["sides"]
        graph_a = self.graph_cache.load(side_a["cache"])
        graph_b = self.graph_cache.load(side_b["cache"])
        face_a, face_b = int(side_a["selected_face"]), int(side_b["selected_face"])
        if not 0 <= face_a < int(graph_a.n_faces) or not 0 <= face_b < int(graph_b.n_faces):
            raise IndexError(f"Selected face out of range for {sample_id}")
        local_a = self._validate_indices(side_a["local_mcfs"], graph_a.mcfs.shape[0], "local_mcfs_a", sample_id)
        local_b = self._validate_indices(side_b["local_mcfs"], graph_b.mcfs.shape[0], "local_mcfs_b", sample_id)
        candidate_pairs = torch.stack((
            local_a.repeat_interleave(local_b.numel()),
            local_b.repeat(local_a.numel()),
        ), dim=1)
        if candidate_pairs.shape[0] != int(row["candidate_pair_count"]):
            raise ValueError(f"Candidate pair count mismatch for {sample_id}")
        positions_a = {int(value): position for position, value in enumerate(local_a.tolist())}
        positions_b = {int(value): position for position, value in enumerate(local_b.tolist())}
        pair_labels = torch.zeros(candidate_pairs.shape[0], dtype=torch.bool)
        for positive_a, positive_b in row["positive_pairs"]:
            if int(positive_a) not in positions_a or int(positive_b) not in positions_b:
                raise ValueError(f"Positive pair outside local candidates for {sample_id}")
            flat_index = positions_a[int(positive_a)] * local_b.numel() + positions_b[int(positive_b)]
            pair_labels[flat_index] = True
        if not bool(pair_labels.any()):
            raise ValueError(f"No positive pair for {sample_id}")
        return LocationSample(
            sample_id=sample_id, mate_type=str(row["mate_type"]),
            graph_a=graph_a, graph_b=graph_b,
            selected_faces=torch.tensor([face_a, face_b], dtype=torch.long),
            local_mcfs_a=local_a, local_mcfs_b=local_b,
            candidate_pairs=candidate_pairs, pair_labels=pair_labels,
        )


class CandidateBudgetBatchSampler(BatchSampler):
    """Greedily batch samples without exceeding a candidate-pair budget."""

    def __init__(self, candidate_counts, max_candidate_pairs=12000, max_samples=32,
                 shuffle=True, seed=20260803, drop_last=False):
        self.candidate_counts = [int(value) for value in candidate_counts]
        self.max_candidate_pairs = int(max_candidate_pairs)
        self.max_samples = int(max_samples)
        self.shuffle = bool(shuffle)
        self.seed = int(seed)
        self.drop_last = bool(drop_last)
        self.epoch = 0
        if self.max_candidate_pairs <= 0 or self.max_samples <= 0:
            raise ValueError("Batch budgets must be positive")
        if any(value <= 0 for value in self.candidate_counts):
            raise ValueError("Candidate counts must be positive")

    def set_epoch(self, epoch):
        self.epoch = int(epoch)

    def _batches(self):
        indices = list(range(len(self.candidate_counts)))
        if self.shuffle:
            random.Random(self.seed + self.epoch).shuffle(indices)
        batch, cost = [], 0
        for index in indices:
            item_cost = self.candidate_counts[index]
            if batch and (cost + item_cost > self.max_candidate_pairs or len(batch) >= self.max_samples):
                yield batch
                batch, cost = [], 0
            batch.append(index)
            cost += item_cost
        if batch and not self.drop_last:
            yield batch

    def __iter__(self):
        return iter(self._batches())

    def __len__(self):
        return sum(1 for _ in self._batches())


def collate_locations(samples):
    if not samples:
        raise ValueError("Cannot collate an empty Location batch")
    graph_a = flatbatch([sample.graph_a for sample in samples])
    graph_b = flatbatch([sample.graph_b for sample in samples])
    mcf_offsets_a, mcf_offsets_b, face_offsets_a, face_offsets_b = [], [], [], []
    mcf_a = mcf_b = face_a = face_b = 0
    for sample in samples:
        mcf_offsets_a.append(mcf_a); mcf_offsets_b.append(mcf_b)
        face_offsets_a.append(face_a); face_offsets_b.append(face_b)
        mcf_a += int(sample.graph_a.mcfs.shape[0]); mcf_b += int(sample.graph_b.mcfs.shape[0])
        face_a += int(sample.graph_a.n_faces); face_b += int(sample.graph_b.n_faces)
    pairs, local_pairs, labels, pair_to_sample = [], [], [], []
    local_a, local_b, local_to_sample_a, local_to_sample_b = [], [], [], []
    pair_offsets = [0]
    selected_faces = []
    for index, sample in enumerate(samples):
        pair_offset = torch.tensor([mcf_offsets_a[index], mcf_offsets_b[index]])
        pairs.append(sample.candidate_pairs + pair_offset)
        local_pairs.append(torch.stack((
            torch.arange(sample.local_mcfs_a.numel()).repeat_interleave(sample.local_mcfs_b.numel())
                + sum(item.numel() for item in local_a),
            torch.arange(sample.local_mcfs_b.numel()).repeat(sample.local_mcfs_a.numel())
                + sum(item.numel() for item in local_b),
        ), dim=1))
        labels.append(sample.pair_labels)
        pair_to_sample.append(torch.full((len(sample.pair_labels),), index, dtype=torch.long))
        pair_offsets.append(pair_offsets[-1] + len(sample.pair_labels))
        local_a.append(sample.local_mcfs_a + mcf_offsets_a[index])
        local_b.append(sample.local_mcfs_b + mcf_offsets_b[index])
        local_to_sample_a.append(torch.full((sample.local_mcfs_a.numel(),), index, dtype=torch.long))
        local_to_sample_b.append(torch.full((sample.local_mcfs_b.numel(),), index, dtype=torch.long))
        selected_faces.append(sample.selected_faces + torch.tensor([face_offsets_a[index], face_offsets_b[index]]))
    return LocationBatch(
        graph_a=graph_a, graph_b=graph_b, selected_faces=torch.stack(selected_faces),
        local_mcfs_a=torch.cat(local_a), local_mcfs_b=torch.cat(local_b),
        local_mcf_to_sample_a=torch.cat(local_to_sample_a), local_mcf_to_sample_b=torch.cat(local_to_sample_b),
        candidate_pairs=torch.cat(pairs), candidate_local_pairs=torch.cat(local_pairs), pair_labels=torch.cat(labels),
        pair_to_sample=torch.cat(pair_to_sample), sample_pair_offsets=torch.tensor(pair_offsets),
        sample_ids=[sample.sample_id for sample in samples],
        mate_types=[sample.mate_type for sample in samples],
    )


def make_location_dataloader(index_dir, split="train", max_candidate_pairs=12000,
                             max_samples=32, shuffle=None, num_workers=0,
                             seed=20260803, graph_cache_size=32, **kwargs):
    dataset = LocationDataset(index_dir, split=split, graph_cache_size=graph_cache_size)
    if shuffle is None:
        shuffle = split == "train"
    sampler = CandidateBudgetBatchSampler(
        dataset.candidate_counts, max_candidate_pairs=max_candidate_pairs,
        max_samples=max_samples, shuffle=shuffle, seed=seed,
    )
    loader = DataLoader(dataset, batch_sampler=sampler, num_workers=num_workers,
                        collate_fn=collate_locations, **kwargs)
    return dataset, loader
