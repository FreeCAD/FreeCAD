#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

"""
Manage baselines for the Coin node visual snapshot test.

This script runs the existing FreeCAD unittest `TestCoinNodeSnapshots` via FreeCADCmd,
configuring it through environment variables (so we don't depend on FreeCAD forwarding
CLI args to Python).

Examples:

  # Update baselines in-tree (recommended: do this on a controlled setup)
  tools/rendering/manage_coin_node_baselines.py update \
    --freecadcmd build/<preset>/bin/FreeCADCmd

  # Compare current renders against baselines (writes actual/expected/diff under --out-dir)
  tools/rendering/manage_coin_node_baselines.py compare \
    --out-dir /tmp/FreeCADTesting/CoinNodeSnapshots \
    --freecadcmd build/<preset>/bin/FreeCADCmd
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

# pylint: disable=broad-exception-caught,duplicate-code


def _default_freecadcmd() -> str | None:
    # Allow explicit override from the environment.
    for env_var in ("FREECADCMD", "FC_FREECADCMD", "FREECAD_CMD"):
        val = os.environ.get(env_var, "").strip()
        if val:
            return val

    # Common local build layouts.
    candidates: list[Path] = [
        Path("build/bin/FreeCADCmd"),
        *sorted(Path("build").glob("*/bin/FreeCADCmd")),
        *sorted(Path("build").glob("*/bin/FreeCADCmd.exe")),
    ]
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)

    return shutil.which("FreeCADCmd")


def _default_baseline_dir() -> str:
    # This script lives in-tree under `tools/rendering/`.
    repo_root = Path(__file__).resolve().parents[2]
    return str(repo_root / "tests" / "visual" / "baselines" / "coin-nodes")


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "--freecadcmd",
        default=_default_freecadcmd(),
        help=(
            "Path to FreeCADCmd (default: $FREECADCMD or auto-detect under build/*/bin/FreeCADCmd or PATH)"
        ),
    )
    common.add_argument(
        "--qt-platform",
        default="",
        help=(
            "Value for QT_QPA_PLATFORM. By default this script does not override it; "
            "if no DISPLAY/WAYLAND is available, it auto-sets QT_QPA_PLATFORM=offscreen."
        ),
    )
    common.add_argument(
        "--baseline-dir",
        default=_default_baseline_dir(),
        help="Baseline directory containing/writing *.png files (default: %(default)s)",
    )
    common.add_argument(
        "--out-dir",
        default=os.path.join("/tmp", "FreeCADTesting", "CoinNodeSnapshots"),
        help="Artifact output directory (default: %(default)s)",
    )
    common.add_argument("--nodes", default="", help="Comma-separated node type list (optional)")
    common.add_argument("--width", type=int, default=512, help="Image width (default: %(default)s)")
    common.add_argument(
        "--height",
        type=int,
        default=512,
        help="Image height (default: %(default)s)",
    )
    common.add_argument(
        "--tolerance",
        type=int,
        default=8,
        help="Per-channel tolerance (default: %(default)s)",
    )
    common.add_argument(
        "--max-mismatch-pct",
        type=float,
        default=0.20,
        help="Allowed mismatch percent (default: %(default)s)",
    )
    common.add_argument(
        "--ignore-alpha",
        default="1",
        choices=["0", "1"],
        help="Ignore alpha channel (default: %(default)s)",
    )

    sub.add_parser("update", parents=[common], help="Write/update baselines")
    sub.add_parser("compare", parents=[common], help="Compare against baselines")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    """Entry point."""
    args = _parse_args(argv)

    if not args.freecadcmd:
        print(
            "ERROR: could not auto-detect FreeCADCmd; pass --freecadcmd "
            "build/<preset>/bin/FreeCADCmd (or set $FREECADCMD)",
            file=sys.stderr,
        )
        return 2

    freecadcmd = Path(args.freecadcmd)
    if not freecadcmd.is_file():
        print(f"ERROR: FreeCADCmd not found: {freecadcmd}", file=sys.stderr)
        return 2

    baseline_dir = Path(args.baseline_dir)
    if args.mode == "update":
        baseline_dir.mkdir(parents=True, exist_ok=True)
    elif not baseline_dir.is_dir():
        print(
            f"ERROR: baseline directory not found: {baseline_dir} "
            "(pass --baseline-dir or run `update` first)",
            file=sys.stderr,
        )
        return 2

    env = os.environ.copy()
    if args.qt_platform.strip():
        env["QT_QPA_PLATFORM"] = args.qt_platform.strip()
    else:
        has_display = bool(env.get("DISPLAY") or env.get("WAYLAND_DISPLAY"))
        if not has_display and not env.get("QT_QPA_PLATFORM"):
            env["QT_QPA_PLATFORM"] = "offscreen"
    env["FC_VISUAL_BASELINE_DIR"] = str(baseline_dir.resolve())
    env["FC_VISUAL_OUT_DIR"] = str(Path(args.out_dir).resolve())
    env["FC_VISUAL_WIDTH"] = str(int(args.width))
    env["FC_VISUAL_HEIGHT"] = str(int(args.height))
    env["FC_VISUAL_TOLERANCE"] = str(int(args.tolerance))
    env["FC_VISUAL_MAX_MISMATCH_PCT"] = str(float(args.max_mismatch_pct))
    env["FC_VISUAL_IGNORE_ALPHA"] = args.ignore_alpha
    if args.nodes.strip():
        env["FC_VISUAL_NODES"] = args.nodes

    if args.mode == "update":
        env["FC_VISUAL_UPDATE_BASELINE"] = "1"
        cmd = [str(freecadcmd), "-t", "TestCoinNodeSnapshots"]
    else:
        env.pop("FC_VISUAL_UPDATE_BASELINE", None)
        cmd = [str(freecadcmd), "-t", "TestCoinNodeSnapshots"]

    print(f"Running: {' '.join(cmd)}")
    if "QT_QPA_PLATFORM" in env:
        print(f"  QT_QPA_PLATFORM={env['QT_QPA_PLATFORM']}")
    print(f"  FC_VISUAL_BASELINE_DIR={env['FC_VISUAL_BASELINE_DIR']}")
    print(f"  FC_VISUAL_OUT_DIR={env['FC_VISUAL_OUT_DIR']}")
    if "FC_VISUAL_NODES" in env:
        print(f"  FC_VISUAL_NODES={env['FC_VISUAL_NODES']}")

    proc = subprocess.run(cmd, env=env, cwd=str(Path.cwd()), check=False)
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
