#!/usr/bin/env python3
from __future__ import annotations

import argparse
import logging
import os
import sys
from pathlib import Path

from utils import ensure_tool, setup_logger


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Preinstall Python-based lint tools into the cached lint venv (used by tools/lint/*)."
    )
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument(
        "--venv-dir",
        help="Install into this venv directory (sets DEVSTACK_LINT_VENV_DIR for this run).",
    )
    parser.add_argument(
        "--venv-bin",
        help="Install into this venv bin directory (sets DEVSTACK_LINT_VENV_BIN for this run).",
    )
    ns = parser.parse_args(argv)

    setup_logger(ns.verbose)

    if ns.venv_dir:
        os.environ["DEVSTACK_LINT_VENV_DIR"] = str(Path(ns.venv_dir).expanduser().resolve())
    if ns.venv_bin:
        os.environ["DEVSTACK_LINT_VENV_BIN"] = str(Path(ns.venv_bin).expanduser().resolve())

    venv_dir = os.environ.get("DEVSTACK_LINT_VENV_DIR", "").strip()
    venv_bin = os.environ.get("DEVSTACK_LINT_VENV_BIN", "").strip()
    if venv_dir or venv_bin:
        logging.info("lint venv override: dir=%s bin=%s", venv_dir or "-", venv_bin or "-")

    # Keep this aligned with what devstack lint may invoke.
    tools = [
        ("black", "black", ["--version"]),
        ("pylint", "pylint", ["--version"]),
        ("codespell", "codespell", ["--version"]),
        ("cpplint", "cpplint", ["--help"]),
    ]

    failed: list[str] = []
    for tool, package, check_args in tools:
        try:
            ensure_tool(tool, package=package, prefer_pipx=False, check_args=check_args)
            logging.info("ok: %s", tool)
        except SystemExit as exc:
            failed.append(f"{tool} (exit {exc.code})")

    if failed:
        logging.error("failed: %s", ", ".join(failed))
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
