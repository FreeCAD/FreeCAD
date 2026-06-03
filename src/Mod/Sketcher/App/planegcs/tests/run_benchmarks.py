# SPDX-FileCopyrightText: © 2026 Dustin Hartlyn <https://github.com/dustinhartlyn>
# SPDX-License-Identifier: LGPL-2.1-or-later

#!/usr/bin/env python3
"""
PlaneGCS Solver Benchmarking and Documentation Sync Script
===========================================================

This script programmatically runs the performance profile comparing
the original legacy C++ solver (planegcs_ext) or Python legacy baseline
against the new high-performance vectorized solver, and then writes the
exact, real-world benchmark metrics directly into the active markdown docs.
"""

import os
import sys
import time
import json
import re
import numpy as np

# Absolute workspace and path setup
WORKSPACE_ROOT = r"C:\Users\dusti\.roo\worktrees\feature\planegcs-cpp-solver"
DOC_DIR = os.path.join(WORKSPACE_ROOT, "src", "Mod", "Sketcher", "App", "planegcs", "doc")
TESTS_DIR = os.path.join(WORKSPACE_ROOT, "src", "Mod", "Sketcher", "App", "planegcs", "tests")
FREE_CAD_LIB_PATH = r"C:\Users\dusti\Documents\Projects\FreeCad"

# Inject FreeCAD path for legacy planegcs_ext and tests dir for local imports
sys.path.append(FREE_CAD_LIB_PATH)
sys.path.append(TESTS_DIR)

from planegcs_derivation import HighPerformanceSparseJacobian


def main():
    print("=" * 80)
    print("PlaneGCS Unified Performance Benchmark & Documentation Patcher")
    print("=" * 80)

    # 1. Initialize HighPerformanceSparseJacobian
    edges_path = os.path.join(TESTS_DIR, "hex_grid_edges.json")
    if not os.path.exists(edges_path):
        print(f"Error: Missing test file at {edges_path}")
        sys.exit(1)

    print("\n[1/4] Initializing and preparing Vectorized Solver...")
    vec_solver = HighPerformanceSparseJacobian(grid_size=(5, 5))
    vec_solver.initialize_hexagonal_grid(spacing=1.0)
    vec_solver.load_edges_from_json(edges_path)
    print("  [OK] Vectorized solver initialized.")

    # 2. Import and Initialize Legacy Solver
    use_ext = False
    planegcs_ext = None
    try:
        import planegcs_ext

        print("  [OK] Successfully imported legacy planegcs_ext extension.")
        # Prepare edge data formatted for the legacy extension
        # Since initialize_solver expects a list of dictionaries with target_distance
        edge_data_ext = []
        for edge in vec_solver.edges:
            node_A_str = f"{edge.node_A[0]}_{edge.node_A[1]}"
            node_B_str = f"{edge.node_B[0]}_{edge.node_B[1]}"
            edge_data_ext.append(
                {
                    "edge_id": edge.edge_id,
                    "node_A": node_A_str,
                    "node_B": node_B_str,
                    "target_distance": float(edge.target_distance),
                }
            )
        planegcs_ext.initialize_solver(edge_data_ext)
        use_ext = True
        print("  [OK] Legacy C++ solver initialized successfully.")
    except Exception as e:
        print(f"  [WARN] Failed to load/initialize legacy C++ extension: {e}")
        print("         Falling back to Python legacy loop simulation baseline.")
        use_ext = False

    # 3. Standardized Performance Profiling (500,000 Iterations)
    num_iterations = 500000
    constraints_count = 57
    total_evals = num_iterations * constraints_count

    print(f"\n[2/4] Standardized Profiling: Running {num_iterations:,} iterations...")

    # A. Profile Vectorized Solver
    print("  Evaluating Vectorized Solver...")
    config = vec_solver.get_configuration_vector()
    # Warmup
    for _ in range(1000):
        _ = vec_solver.evaluate_constraints_vectorized(config)

    start_vec = time.perf_counter()
    for _ in range(num_iterations):
        _ = vec_solver.evaluate_constraints_vectorized(config)
    elapsed_vec = time.perf_counter() - start_vec
    throughput_vec = total_evals / elapsed_vec / 1e6
    print(f"    Vectorized Time: {elapsed_vec:.6f} s")
    print(f"    Vectorized Throughput: {throughput_vec:.6f} M constraints/sec")

    # B. Profile Legacy Solver
    print("  Evaluating Legacy Solver...")
    if use_ext:
        coords = vec_solver.get_configuration_vector()
        # Warmup
        for _ in range(1000):
            _ = planegcs_ext.evaluate_constraints(coords)

        start_leg = time.perf_counter()
        for _ in range(num_iterations):
            _ = planegcs_ext.evaluate_constraints(coords)
        elapsed_leg = time.perf_counter() - start_leg
    else:
        from planegcs_legacy_loop import LegacyLoopPlaneGCS

        legacy_solver = LegacyLoopPlaneGCS(grid_size=(5, 5))
        legacy_solver.initialize_hexagonal_grid(spacing=1.0)
        legacy_solver.load_edges_from_json(edges_path)
        # Warmup
        for _ in range(1000):
            _ = legacy_solver.evaluate_constraints_legacy_loop()

        start_leg = time.perf_counter()
        for _ in range(num_iterations):
            _ = legacy_solver.evaluate_constraints_legacy_loop()
        elapsed_leg = time.perf_counter() - start_leg

    throughput_leg = total_evals / elapsed_leg / 1e6
    speedup = elapsed_leg / elapsed_vec
    improvement = (speedup - 1.0) * 100
    print(f"    Legacy Time: {elapsed_leg:.6f} s")
    print(f"    Legacy Throughput: {throughput_leg:.6f} M constraints/sec")
    print(f"    Calculated Speedup: {speedup:.6f}x ({improvement:.2f}% improvement)")

    # 4. Programmatic Documentation Patching
    print("\n[3/4] Programmatic Documentation Patching...")

    # Document paths
    readme_path = os.path.join(
        WORKSPACE_ROOT, "README.md"
    )  # Root README, check if exists, but we target doc folder primarily
    readme_doc_path = os.path.join(DOC_DIR, "README.md")
    audit_path = os.path.join(DOC_DIR, "numerical_audit.md")

    # Format values for documentation
    elapsed_leg_str = f"{elapsed_leg:.4f}"
    elapsed_vec_str = f"{elapsed_vec:.4f}"
    throughput_leg_str = f"{throughput_leg:.2f}"
    throughput_vec_str = f"{throughput_vec:.2f}"
    speedup_str = f"{speedup:.2f}"
    improvement_str = f"{improvement:.2f}"

    def patch_readme(path):
        if not os.path.exists(path):
            return False
        print(f"  Patching {path}...")
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()

        # Replacement 1: Executive Summary Bullet Point
        old_bullet = "- **High Throughput Execution**: Achieves up to **5.07M constraints/sec** evaluation throughput (on reference hardware), representing a 249.66% improvement over loop-based baselines."
        new_bullet = f"- **High Throughput Execution**: Achieves up to **{throughput_vec_str}M constraints/sec** evaluation throughput (on reference hardware), representing a {improvement_str}% improvement over loop-based baselines."

        if old_bullet in content:
            content = content.replace(old_bullet, new_bullet)
        else:
            # Fallback to regex in case of slight variance
            content = re.sub(
                r"-\s+\*\*High\s+Throughput\s+Execution\*\*:\s+Achieves\s+up\s+to\s+\*\*\d+\.\d+M\s+constraints/sec\*\*\s+evaluation\s+throughput\s+\(on\s+reference\s+hardware\),\s+representing\s+a\s+\d+\.\d+%\s+improvement\s+over\s+loop-based\s+baselines\.",
                new_bullet,
                content,
            )

        # Replacement 2: Technical Performance Dashboard row
        old_row = "| **Throughput Performance** | 5.07M constraints/sec | Verified Optimal |"
        new_row = f"| **Throughput Performance** | {throughput_vec_str}M constraints/sec | Verified Optimal |"

        if old_row in content:
            content = content.replace(old_row, new_row)
        else:
            content = re.sub(
                r"\|\s+\*\*Throughput\s+Performance\*\*\s+\|\s+\d+\.\d+M\s+constraints/sec\s+\|\s+Verified\s+Optimal\s+\|",
                new_row,
                content,
            )

        with open(path, "w", encoding="utf-8") as f:
            f.write(content)
        return True

    # Patch README in doc folder
    if patch_readme(readme_doc_path):
        print("  [OK] doc/README.md patched successfully.")

    # Patch Root README if it exists and has similar tables
    if patch_readme(readme_path):
        print("  [OK] Root README.md patched successfully.")

    # Patch numerical_audit.md
    if os.path.exists(audit_path):
        print(f"  Patching {audit_path}...")
        with open(audit_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Replacement 1: Legacy Row in Table
        old_row_leg = (
            "| **Legacy Loop-Based Reference** | 19.7017 s | 1.45 M/s | *Baseline (1.00x)* |"
        )
        new_row_leg = f"| **Legacy Loop-Based Reference** | {elapsed_leg_str} s | {throughput_leg_str} M/s | *Baseline (1.00x)* |"

        if old_row_leg in content:
            content = content.replace(old_row_leg, new_row_leg)
        else:
            content = re.sub(
                r"\|\s+\*\*Legacy\s+Loop-Based\s+Reference\*\*\s+\|\s+\d+\.\d+\s+s\s+\|\s+\d+\.\d+\s+M/s\s+\|\s+\*Baseline\s+\(1\.00x\)\*\s+\|",
                new_row_leg,
                content,
            )

        # Replacement 2: Optimized Row in Table
        old_row_vec = "| **Optimized Vectorized Core** | 5.6244 s | 5.07 M/s | **3.50x** (249.66% improvement) |"
        new_row_vec = f"| **Optimized Vectorized Core** | {elapsed_vec_str} s | {throughput_vec_str} M/s | **{speedup_str}x** ({improvement_str}% improvement) |"

        if old_row_vec in content:
            content = content.replace(old_row_vec, new_row_vec)
        else:
            content = re.sub(
                r"\|\s+\*\*Optimized\s+Vectorized\s+Core\*\*\s+\|\s+\d+\.\d+\s+s\s+\|\s+\d+\.\d+\s+M/s\s+\|\s+\*\*\d+\.\d+x\*\*\s+\(\d+\.\d+%\s+improvement\)\s+\|",
                new_row_vec,
                content,
            )

        # Replacement 3: Section 4.2 Legacy paragraph
        old_para_leg = "- **Legacy Loop-Based Solver**: Required **19.7017 seconds** to perform 28,500,000 constraint evaluations, bottlenecked by python-level loop overhead and lack of vectorization."
        new_para_leg = f"- **Legacy Loop-Based Solver**: Required **{elapsed_leg_str} seconds** to perform 28,500,000 constraint evaluations, bottlenecked by python-level loop overhead and lack of vectorization."

        if old_para_leg in content:
            content = content.replace(old_para_leg, new_para_leg)
        else:
            content = re.sub(
                r"-\s+\*\*Legacy\s+Loop-Based\s+Solver\*\*:\s+Required\s+\*\*\d+\.\d+\s+seconds\*\*\s+to\s+perform\s+28,500,000\s+constraint\s+evaluations,\s+bottlenecked\s+by\s+python-level\s+loop\s+overhead\s+and\s+lack\s+of\s+vectorization\.",
                new_para_leg,
                content,
            )

        # Replacement 4: Section 4.2 Vectorized paragraph
        old_para_vec = "- **Optimized Vectorized Solver**: Completed the identical 28,500,000 constraint evaluations in **5.6244 seconds**, demonstrating a true throughput of **5.07 M constraints/sec** due to loop-free array operations and high-precision cache alignment."
        new_para_vec = f"- **Optimized Vectorized Solver**: Completed the identical 28,500,000 constraint evaluations in **{elapsed_vec_str} seconds**, demonstrating a true throughput of **{throughput_vec_str} M constraints/sec** due to loop-free array operations and high-precision cache alignment."

        if old_para_vec in content:
            content = content.replace(old_para_vec, new_para_vec)
        else:
            content = re.sub(
                r"-\s+\*\*Optimized\s+Vectorized\s+Solver\*\*:\s+Completed\s+the\s+identical\s+28,500,000\s+constraint\s+evaluations\s+in\s+\*\*\d+\.\d+\s+seconds\*\*,\s+demonstrating\s+a\s+true\s+throughput\s+of\s+\*\*\d+\.\d+\s+M\s+constraints/sec\*\*\s+due\s+to\s+loop-free\s+array\s+operations\s+and\s+high-precision\s+cache\s+alignment\.",
                new_para_vec,
                content,
            )

        with open(audit_path, "w", encoding="utf-8") as f:
            f.write(content)
        print("  [OK] numerical_audit.md patched successfully.")
    else:
        print(f"  [ERROR] numerical_audit.md not found at {audit_path}")

    print("\n[4/4] Verifying documentation updates on disk...")
    # Read back to ensure updates were recorded
    with open(readme_doc_path, "r", encoding="utf-8") as f:
        r_text = f.read()
    with open(audit_path, "r", encoding="utf-8") as f:
        a_text = f.read()

    readme_verified = f"{throughput_vec_str}M constraints/sec" in r_text
    audit_verified = f"{elapsed_vec_str} s" in a_text

    print(
        f"  Verification - README.md contains '{throughput_vec_str}M constraints/sec': {readme_verified}"
    )
    print(f"  Verification - numerical_audit.md contains '{elapsed_vec_str} s': {audit_verified}")

    if readme_verified and audit_verified:
        print("\n  [SUCCESS] All documentation files successfully updated with actual metrics!")
    else:
        print("\n  [WARNING] Verification partially failed or formatting differed. Review changes.")

    print("\n" + "=" * 80)
    print("FINAL BENCHMARK RACE METRICS")
    print("=" * 80)
    print(f"Legacy Solver Time      : {elapsed_leg_str} s")
    print(f"Legacy Throughput      : {throughput_leg_str} M constraints/sec")
    print(f"Vectorized Solver Time  : {elapsed_vec_str} s")
    print(f"Vectorized Throughput  : {throughput_vec_str} M constraints/sec")
    print(f"Relative Speedup       : {speedup_str}x ({improvement_str}% improvement)")
    print("=" * 80)


if __name__ == "__main__":
    main()
