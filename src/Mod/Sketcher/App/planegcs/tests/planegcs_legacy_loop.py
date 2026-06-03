# SPDX-FileCopyrightText: © 2026 Dustin Hartlyn <https://github.com/dustinhartlyn>
# SPDX-License-Identifier: LGPL-2.1-or-later

#!/usr/bin/env python3
"""
Legacy Loop-Based Geometric Constraint Solver Simulation (PlaneGCS)
===================================================================

This script provides a standardized, non-vectorized reference baseline
simulating FreeCAD's legacy, loop-based constraint evaluation mechanism.
"""

import numpy as np
import time
import json
import os
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass


@dataclass
class ConstraintEdge:
    edge_id: int
    node_A: Tuple[int, int]
    node_B: Tuple[int, int]
    target_distance: float


@dataclass
class SystemNode:
    i: int
    j: int
    x: float
    y: float


class LegacyLoopPlaneGCS:
    def __init__(self, grid_size: Tuple[int, int] = (5, 5)):
        self.rows, self.cols = grid_size
        self.num_nodes = self.rows * self.cols
        self.nodes: Dict[Tuple[int, int], SystemNode] = {}
        self.edges: List[ConstraintEdge] = []
        self.num_constraints = 0

    def initialize_hexagonal_grid(self, spacing: float = 1.0) -> None:
        self.nodes.clear()
        sqrt3_half = np.sqrt(3) / 2
        half_spacing = 0.5 * spacing
        for i in range(self.rows):
            x_offset = half_spacing if i % 2 == 1 else 0.0
            y = i * sqrt3_half * spacing
            for j in range(self.cols):
                self.nodes[(i, j)] = SystemNode(i, j, j * spacing + x_offset, y)

    def load_edges_from_json(self, filepath: str) -> None:
        with open(filepath, "r") as f:
            edge_data = json.load(f)
        self.edges.clear()
        for edge_dict in edge_data:
            node_A_str = edge_dict["node_A"]
            node_B_str = edge_dict["node_B"]
            i_A, j_A = map(int, node_A_str.split("_"))
            i_B, j_B = map(int, node_B_str.split("_"))
            node_A_key = (i_A, j_A)
            node_B_key = (i_B, j_B)
            if node_A_key in self.nodes and node_B_key in self.nodes:
                node_A = self.nodes[node_A_key]
                node_B = self.nodes[node_B_key]
                dx = node_A.x - node_B.x
                dy = node_A.y - node_B.y
                distance = np.sqrt(dx * dx + dy * dy)
                edge = ConstraintEdge(
                    edge_id=edge_dict["edge_id"],
                    node_A=node_A_key,
                    node_B=node_B_key,
                    target_distance=distance,
                )
                self.edges.append(edge)
        self.num_constraints = len(self.edges)

    def evaluate_constraints_legacy_loop(self) -> np.ndarray:
        """
        Evaluate all constraints one-by-one in a standard loop.
        Completely avoids any vectorized NumPy operations across edges.
        """
        F = np.empty(self.num_constraints, dtype=np.float64)
        for k, edge in enumerate(self.edges):
            node_A = self.nodes[edge.node_A]
            node_B = self.nodes[edge.node_B]
            dx = node_A.x - node_B.x
            dy = node_A.y - node_B.y
            dist_sq = dx * dx + dy * dy
            target_dist_sq = edge.target_distance**2
            F[k] = dist_sq - target_dist_sq
        return F


if __name__ == "__main__":
    print("=" * 70)
    print("Legacy Loop-Based Constraint Solver Simulation (PlaneGCS)")
    print("=" * 70)

    solver = LegacyLoopPlaneGCS(grid_size=(5, 5))

    print("\n[1/5] Initializing hexagonal grid...")
    solver.initialize_hexagonal_grid(spacing=1.0)
    print(f"  [OK] Initialized {solver.num_nodes} nodes")

    print("\n[2/5] Loading edge constraints...")
    edges_file = "hex_grid_edges.json"
    if not os.path.exists(edges_file) and os.path.exists("../" + edges_file):
        edges_file = "../" + edges_file
    elif not os.path.exists(edges_file) and os.path.exists(
        "src/Mod/Sketcher/App/planegcs/tests/" + edges_file
    ):
        edges_file = "src/Mod/Sketcher/App/planegcs/tests/" + edges_file

    solver.load_edges_from_json(edges_file)
    print(f"  [OK] Loaded {solver.num_constraints} edge constraints")

    print("\n[3/5] Evaluating constraints...")
    F = solver.evaluate_constraints_legacy_loop()
    print(f"  [OK] Residual norm: {np.linalg.norm(F):.6e}")

    print("\n[4/5] Running LEGACY LOOP performance benchmark...")

    # Warm-up run to ensure cache warming
    for _ in range(1000):
        F = solver.evaluate_constraints_legacy_loop()

    # Benchmark standard python loop evaluation
    num_iterations = 500000
    start = time.perf_counter()
    for _ in range(num_iterations):
        F = solver.evaluate_constraints_legacy_loop()
    elapsed = time.perf_counter() - start

    total_evals = num_iterations * solver.num_constraints
    throughput = total_evals / elapsed / 1e6

    print(f"  [OK] Iterations: {num_iterations:,}")
    print(f"  [OK] Total constraint evaluations: {total_evals:,}")
    print(f"  [OK] Time elapsed: {elapsed:.4f} seconds")
    print(f"  [OK] Throughput: {throughput:.2f} M constraints/sec")

    print("\n" + "=" * 70)
    print("[COMPLETE] Legacy Loop-Based Constraint Solver Simulation (PlaneGCS)")
    print("=" * 70)
