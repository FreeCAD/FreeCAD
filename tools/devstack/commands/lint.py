from __future__ import annotations

import argparse

from tools.devstack.core.lint_runner import run_fix, run_lint


def cmd_lint(args: argparse.Namespace) -> None:
    run_lint(args)


def cmd_fix(args: argparse.Namespace) -> None:
    run_fix(args)

