#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


def _ensure_repo_on_syspath() -> None:
    this_file = Path(__file__).resolve()
    repo_root_dir = this_file.parents[2]
    sys.path.insert(0, str(repo_root_dir))


def main(argv: list[str]) -> None:
    _ensure_repo_on_syspath()
    from tools.devstack.cli import main as cli_main

    cli_main(argv)


if __name__ == "__main__":
    main(sys.argv[1:])

