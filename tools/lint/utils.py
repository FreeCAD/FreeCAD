#!/usr/bin/env python3
from __future__ import annotations

import argparse
import glob
import logging
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path
from typing import Tuple

from cache import devstack_cache_home


def run_command(cmd, check=False) -> Tuple[str, str, int]:
    """Run a command and return (stdout, stderr, exit_code).

    When check=True, exits with a readable error message on failure.
    """
    try:
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            text=True,
        )
        stdout, stderr, exit_code = result.stdout, result.stderr, result.returncode
    except FileNotFoundError as exc:
        stdout, stderr, exit_code = "", str(exc), 127

    if check and exit_code != 0:
        cmd_display = cmd if isinstance(cmd, str) else " ".join(str(x) for x in cmd)
        logging.error("Command failed (exit=%s): %s", exit_code, cmd_display)
        if stdout.strip():
            logging.error(stdout.strip())
        if stderr.strip():
            logging.error(stderr.strip())
        sys.exit(exit_code)

    return stdout, stderr, exit_code


def setup_logger(verbose: bool):
    """
    Setup the logging level based on the verbose flag.
    """
    env_level = os.environ.get("DEVSTACK_LINT_LOG_LEVEL", "").strip().upper()
    if env_level:
        level = getattr(logging, env_level, logging.INFO)
    else:
        mode = os.environ.get("DEVSTACK_LINT_MODE", "").strip().lower()
        if verbose:
            level = logging.DEBUG
        elif mode == "ci" or os.environ.get("GITHUB_ACTIONS") or os.environ.get("RUNNER_WORKSPACE"):
            level = logging.INFO
        else:
            # Default: quiet-ish for local runs; the caller (devstack) should provide summaries.
            level = logging.WARNING
    logging.basicConfig(level=level, format="%(levelname)s: %(message)s")


def write_file(file_path: str, content: str):
    """Write content to the specified file."""
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)
    logging.debug("Wrote file: %s", file_path)


def append_file(file_path: str, content: str):
    """Append content to the specified file."""
    with open(file_path, "a", encoding="utf-8") as f:
        f.write(content + "\n")
    logging.debug("Appended content to file: %s", file_path)


def emit_problem_matchers(log_path: str, matcher_filename: str, remove_owner: str):
    """
    Emit GitHub Actions problem matcher commands using the given matcher file.

    This function will:
      1. Check if the log file exists.
      2. Use the RUNNER_WORKSPACE environment variable to construct the matcher path.
      3. Print the add-matcher command, then the log content, then the remove-matcher command.
    """
    if os.path.isfile(log_path):
        runner_workspace = os.getenv("RUNNER_WORKSPACE")
        if not runner_workspace:
            return
        matcher_path = os.path.join(
            runner_workspace, "FreeCAD", ".github", "problemMatcher", matcher_filename
        )
        print(f"::add-matcher::{matcher_path}")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print(f"::remove-matcher owner={remove_owner}::")


def in_github_actions() -> bool:
    return bool(os.getenv("GITHUB_ACTIONS") or os.getenv("RUNNER_WORKSPACE"))


def ensure_tool(
    tool: str,
    *,
    package: str,
    prefer_pipx: bool = True,
    check_args: list[str] | None = None,
):
    """Ensure a CLI tool is available for this process.

    Strategy:
      - Use PATH if present.
      - If installed into the current interpreter's scripts dir, prepend that dir to PATH.
      - If installation into the system interpreter is blocked (PEP 668), install into a cached venv and prepend its bin dir.
    """

    def scripts_dir() -> str:
        try:
            return sysconfig.get_path("scripts") or ""
        except Exception:
            return ""

    def prepend_path(dir_path: str) -> None:
        if not dir_path:
            return
        cur = os.environ.get("PATH", "")
        parts = cur.split(os.pathsep) if cur else []
        if parts and parts[0] == dir_path:
            return
        os.environ["PATH"] = dir_path + (os.pathsep + cur if cur else "")

    def tool_works() -> bool:
        if shutil.which(tool) is None:
            return False
        if not check_args:
            return True
        _, _, rc = run_command([tool, *check_args], check=False)
        return rc == 0

    if tool_works():
        return

    force_venv = bool(os.environ.get("DEVSTACK_LINT_VENV_DIR", "").strip() or os.environ.get("DEVSTACK_LINT_VENV_BIN", "").strip())

    def lint_venv_dir() -> Path:
        env_dir = os.environ.get("DEVSTACK_LINT_VENV_DIR", "").strip()
        if env_dir:
            return Path(env_dir).expanduser().resolve()

        env_bin = os.environ.get("DEVSTACK_LINT_VENV_BIN", "").strip()
        if env_bin:
            # If pointing at .../venv/bin, venv dir is parent.
            p = Path(env_bin).expanduser().resolve()
            return p.parent if p.name == "bin" else p

        return devstack_cache_home() / "python-lint" / "venv"

    def lint_venv_bin_dir() -> Path:
        env_bin = os.environ.get("DEVSTACK_LINT_VENV_BIN", "").strip()
        if env_bin:
            p = Path(env_bin).expanduser().resolve()
            return p if p.name == "bin" else (p / "bin")
        return lint_venv_dir() / "bin"

    if not force_venv:
        sd = scripts_dir()
        if sd and (Path(sd) / tool).is_file():
            prepend_path(sd)
            if tool_works():
                return

    venv_bin = lint_venv_bin_dir()
    if (venv_bin / tool).is_file():
        prepend_path(str(venv_bin))
        if tool_works():
            return

    if force_venv:
        venv_dir = lint_venv_dir()
        venv_py = venv_dir / "bin" / "python"
        if not venv_py.is_file():
            logging.info("Creating lint venv: %s", venv_dir)
            run_command([sys.executable, "-m", "venv", str(venv_dir)], check=True)

        run_command([str(venv_py), "-m", "pip", "install", "-q", package], check=True)
        prepend_path(str(venv_bin))
        if tool_works():
            return

        logging.error("Installed %s into lint venv but it still fails to run", tool)
        sys.exit(1)

    logging.info("Installing %s (package=%s)...", tool, package)

    if prefer_pipx and shutil.which("pipx") is not None:
        _, stderr, rc = run_command(["pipx", "install", "-q", package], check=False)
        if rc == 0 and tool_works():
            return
        logging.error(stderr.strip() or "pipx install failed")
        if rc != 0:
            sys.exit(rc)

    # Try installing into the current interpreter.
    _, stderr, rc = run_command([sys.executable, "-m", "pip", "install", "-q", package], check=False)
    if rc == 0:
        sd = scripts_dir()
        if sd and (Path(sd) / tool).is_file():
            prepend_path(sd)
        if tool_works():
            return

    # If the environment is externally managed, fall back to a cached venv.
    stderr_lower = (stderr or "").lower()
    if "externally-managed-environment" in stderr_lower or "externally managed" in stderr_lower:
        venv_dir = lint_venv_dir()
        venv_py = venv_dir / "bin" / "python"
        venv_bin = venv_dir / "bin"

        if not venv_py.is_file():
            logging.info("Creating lint venv: %s", venv_dir)
            run_command([sys.executable, "-m", "venv", str(venv_dir)], check=True)

        run_command([str(venv_py), "-m", "pip", "install", "-q", package], check=True)
        prepend_path(str(venv_bin))
        if tool_works():
            return

        logging.error("Installed %s but it still fails to run", tool)
        sys.exit(1)

    logging.error(stderr.strip() or "pip install failed")
    sys.exit(rc)



def add_common_arguments(parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    """
    Add common command-line arguments shared between tools.
    """
    parser.add_argument(
        "--files",
        required=True,
        nargs="+",
        help="File list or glob patterns to check (supports CI-style quoted space-separated lists).",
    )
    parser.add_argument(
        "--log-dir", required=True, help="Directory where log files will be written."
    )
    parser.add_argument(
        "--report-file",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    return parser


def init_environment(args: argparse.Namespace):
    """
    Perform common initialization tasks:
      - Set up logging.
      - Create the log directory.
      - Create the directory for the report file.
    """
    setup_logger(args.verbose)
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)


def normalize_files_args(files: object) -> list[str]:
    """
    Normalize `--files` input from argparse into a flat list of tokens.

    This accepts either:
      - `--files a b c` (argparse list)
      - `--files "a b c"` (single string; common in CI YAML)
    """
    if files is None:
        return []
    if isinstance(files, str):
        parts = [files]
    else:
        try:
            parts = list(files)  # type: ignore[arg-type]
        except TypeError:
            parts = [str(files)]

    out: list[str] = []
    for part in parts:
        out.extend(str(part).split())
    return out


def expand_files(files: object, *, recursive: bool = True, only_existing: bool = True) -> list[str]:
    tokens = normalize_files_args(files)
    paths: list[str] = []
    for token in tokens:
        matches = glob.glob(token, recursive=recursive)
        if matches:
            paths.extend(matches)
        else:
            paths.append(token)

    seen: set[str] = set()
    out: list[str] = []
    for p in paths:
        if only_existing and not os.path.isfile(p):
            continue
        if p in seen:
            continue
        out.append(p)
        seen.add(p)
    return out
