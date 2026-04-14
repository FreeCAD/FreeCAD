#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""
Developer test runner for the CAM workbench.

Wraps `FreeCADCmd -t TestCAMApp[...]` with presets, change-aware selection,
and benchmarking. Run from anywhere:

    python3 src/Mod/CAM/run_tests.py --help

Examples:
    run_tests.py                          # run the full suite (one invocation)
    run_tests.py --post                   # run only post-processor tests
    run_tests.py --ops --benchmark        # time each op test class
    run_tests.py --changed                # run tests related to git-changed files
    run_tests.py -k TestPathProfile       # run a specific class (or method)
    run_tests.py --list                   # list registered test classes
"""

import argparse
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
CAM_ROOT = Path(__file__).resolve().parent
TESTS_DIR = CAM_ROOT / "CAMTests"
TEST_APP = CAM_ROOT / "TestCAMApp.py"
DEFAULT_BIN = REPO_ROOT / "build" / "bin" / "FreeCADCmd"

# Preset groups matched against test class names by substring / regex.
PRESETS = {
    "post": [
        r"Post$",
        r"^TestPost",
        r"^TestGeneric",
        r"^TestLinuxCNC",
        r"^TestFanuc",
        r"^TestDxf",
        r"^TestSnapmaker",
        r"^TestSVGPost",
        r"^TestCentroidLegacy",
        r"^TestMach3Mach4",
        r"^TestTestPost",
        r"^TestDressupPost",
        r"^TestGcodeProcessing",
        r"^TestToolLengthOffset",
        r"^TestToolProcessing",
        r"^TestFileNameGenerator",
        r"^TestExport2",
        r"^TestHeaderBuilder",
        r"^TestConfigurationBundle",
        r"^TestResolvingPostProcessorName",
        r"^TestPostProcessorFactory",
        r"^TestBuildPostList",
        r"^TestJobPropertyOverrides",
        r"^TestPathPostUtils",
    ],
    "ops": [
        r"^TestPathProfile",
        r"^TestPathPocket",
        r"^TestPathAdaptive",
        r"^TestPathHelix$",
        r"^TestPathDrillable",
        r"^TestPathVcarve",
        r"^TestPathThreadMilling$",
        r"^TestPathOpDeburr",
        r"^TestPathCore",
        r"^TestDressup",
        r"^TestHoldingTags",
    ],
    "generators": [r"Generator", r"^TestGetLinkingMoves", r"^TestDrillCycleExpander"],
    "tool": [
        r"^TestPathTool",
        r"^TestToolBit",
        r"^Test.*ToolBit",
        r"^Test.*Serializer",
        r"^Test.*Library",
    ],
    "machine": [r"^TestMachine", r"^TestToolhead"],
    "dressup": [r"^TestDressup", r"^TestHoldingTags", r"^TestGeneratorDogbone"],
    "sanity": [r"^TestCAMSanity"],
}


def parse_registered_classes():
    """Extract class names imported in TestCAMApp.py."""
    text = TEST_APP.read_text()
    # Strip comments
    text = re.sub(r"#.*", "", text)
    # Join parenthesized multi-line imports onto one logical line.
    text = re.sub(r"\(([^)]*)\)", lambda m: m.group(1).replace("\n", " "), text)
    classes = []
    for line in text.splitlines():
        m = re.match(r"\s*from\s+CAMTests\.[\w.]+\s+import\s+(.+?)\s*$", line)
        if not m:
            continue
        for name in m.group(1).split(","):
            name = name.strip()
            if name:
                classes.append(name)
    # De-dup preserving order
    seen = set()
    out = []
    for c in classes:
        if c not in seen:
            seen.add(c)
            out.append(c)
    return out


def select(classes, patterns):
    rx = [re.compile(p) for p in patterns]
    return [c for c in classes if any(r.search(c) for r in rx)]


def changed_test_classes(classes, base="master"):
    """Pick test classes whose name stem matches a changed source file."""
    try:
        diff = subprocess.check_output(
            ["git", "diff", "--name-only", f"{base}...HEAD"], cwd=REPO_ROOT, text=True
        )
        staged = subprocess.check_output(["git", "diff", "--name-only"], cwd=REPO_ROOT, text=True)
    except subprocess.CalledProcessError as e:
        print(f"git diff failed: {e}", file=sys.stderr)
        return []
    files = set(diff.split()) | set(staged.split())
    stems = {Path(f).stem for f in files if f.endswith(".py")}
    if not stems:
        return []
    # Match test classes whose name contains a changed source stem, or whose
    # own test file is among the changed files.
    selected = []
    for c in classes:
        if any(s and s in c for s in stems):
            selected.append(c)
            continue
        # Also include tests whose own test file changed.
        for f in files:
            if f.endswith(".py") and c in Path(f).stem:
                selected.append(c)
                break
    return selected


def invoke(bin_path, selector, quiet=False):
    """Run FreeCADCmd -t <selector>. Return (returncode, wall_seconds)."""
    cmd = [str(bin_path), "-t", selector]
    start = time.perf_counter()
    if quiet:
        res = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    else:
        res = subprocess.run(cmd)
    return res.returncode, time.perf_counter() - start


def print_benchmark_table(results, history_path=None):
    """results: list of (selector, rc, seconds)."""
    width = max(len(s) for s, _, _ in results)
    total = sum(t for _, _, t in results)
    fails = [s for s, rc, _ in results if rc != 0]

    print()
    print(f"{'TEST':<{width}}  {'STATUS':<6}  {'SECONDS':>8}  {'DELTA':>8}")
    print("-" * (width + 30))

    prev = {}
    if history_path and history_path.exists():
        try:
            prev = json.loads(history_path.read_text())
        except Exception:
            prev = {}

    for selector, rc, secs in sorted(results, key=lambda r: -r[2]):
        status = "OK" if rc == 0 else "FAIL"
        prior = prev.get(selector)
        if prior:
            delta = secs - prior
            pct = (delta / prior * 100) if prior else 0
            marker = " *" if abs(pct) >= 20 else "  "
            delta_s = f"{delta:+.2f}s{marker}"
        else:
            delta_s = "   new"
        print(f"{selector:<{width}}  {status:<6}  {secs:>7.2f}s  {delta_s:>8}")

    print("-" * (width + 30))
    print(f"Total: {total:.2f}s across {len(results)} classes. " f"Failures: {len(fails)}")

    if history_path:
        current = {s: t for s, _, t in results}
        # Merge so we retain timings for classes not run this time.
        merged = {**prev, **current}
        history_path.write_text(json.dumps(merged, indent=2, sort_keys=True))
        print(f"Timings saved to {history_path}")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--bin", default=str(DEFAULT_BIN), help=f"FreeCADCmd binary (default: {DEFAULT_BIN})"
    )
    ap.add_argument(
        "-k",
        "--match",
        action="append",
        default=[],
        help="Run only test classes matching this regex (repeatable)",
    )
    ap.add_argument("--list", action="store_true", help="List registered test classes and exit")
    ap.add_argument(
        "--changed", action="store_true", help="Run only tests related to files changed vs. --base"
    )
    ap.add_argument("--base", default="master", help="Git base for --changed")
    ap.add_argument(
        "--benchmark",
        action="store_true",
        help="Run each test class separately and print timing table",
    )
    ap.add_argument(
        "--history",
        default=str(CAM_ROOT / "CAMTests" / ".test_timings.json"),
        help="JSON file for persisted timing history",
    )
    ap.add_argument("--no-history", action="store_true", help="Do not read or write timing history")
    ap.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Suppress per-test output (useful with --benchmark)",
    )

    for preset in PRESETS:
        ap.add_argument(f"--{preset}", action="store_true", help=f"Preset: run {preset} tests")

    args = ap.parse_args()

    classes = parse_registered_classes()

    # Build selection
    selected = []
    for preset in PRESETS:
        if getattr(args, preset):
            selected.extend(select(classes, PRESETS[preset]))
    if args.match:
        selected.extend(select(classes, args.match))
    if args.changed:
        selected.extend(changed_test_classes(classes, args.base))

    # De-dup preserving order
    seen = set()
    selected = [c for c in selected if not (c in seen or seen.add(c))]

    if args.list:
        for c in selected or classes:
            print(c)
        return 0

    bin_path = Path(args.bin)
    if not bin_path.exists():
        print(f"FreeCADCmd not found at {bin_path}", file=sys.stderr)
        return 2

    # No filter → run everything. If benchmarking, expand to per-class.
    if not selected:
        if args.benchmark:
            selected = classes
        else:
            rc, secs = invoke(bin_path, "TestCAMApp", quiet=args.quiet)
            print(f"\nSuite finished in {secs:.2f}s (rc={rc})")
            return rc

    if not selected:
        print("No tests selected.", file=sys.stderr)
        return 1

    print(f"Selected {len(selected)} test class(es).")

    if args.benchmark:
        results = []
        for cls in selected:
            selector = f"TestCAMApp.{cls}"
            print(f"\n>>> {selector}")
            rc, secs = invoke(bin_path, selector, quiet=args.quiet)
            results.append((selector, rc, secs))
        history = None if args.no_history else Path(args.history)
        print_benchmark_table(results, history)
        return max(rc for _, rc, _ in results)

    # Non-benchmark path: run each selected class sequentially, no timing.
    fail = 0
    for cls in selected:
        rc, _ = invoke(bin_path, f"TestCAMApp.{cls}", quiet=args.quiet)
        if rc:
            fail = rc
    return fail


if __name__ == "__main__":
    sys.exit(main())
