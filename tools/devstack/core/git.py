from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path
from typing import Optional

from .proc import die, run


def git(args: list[str], *, cwd: Optional[Path] = None, capture: bool = True) -> str:
    proc = run(["git", *args], cwd=cwd, capture=capture)
    return proc.stdout.strip() if capture and proc.stdout is not None else ""


def repo_root(cwd: Optional[Path] = None) -> Path:
    return Path(git(["rev-parse", "--show-toplevel"], cwd=cwd)).resolve()


def bisect_start_branch(root: Path) -> str:
    try:
        bisect_path = Path(git(["rev-parse", "--git-path", "BISECT_START"], cwd=root))
    except subprocess.CalledProcessError:
        return ""
    if not bisect_path.is_file():
        return ""
    try:
        return bisect_path.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return ""


def current_branch(root: Path) -> str:
    b = bisect_start_branch(root)
    if b:
        return b
    try:
        b = git(["branch", "--show-current"], cwd=root)
    except subprocess.CalledProcessError:
        return ""
    return b


def sanitize_branch_to_conf_name(branch: str) -> str:
    branch = branch.replace("/", "-")
    return re.sub(r"[^A-Za-z0-9._-]+", "-", branch)


def sanitize_key_to_filename(key: str) -> str:
    key = key.replace("/", "-")
    return re.sub(r"[^A-Za-z0-9._-]+", "-", key)


def resolve_conf_path(root: Path) -> Path:
    env_conf = os.environ.get("DEVSTACK_STACK_CONF", "").strip()
    if env_conf:
        return Path(env_conf).expanduser().resolve()

    stack_conf = root / ".devstack" / "stack.conf"
    return stack_conf


def ensure_commit_exists(root: Path, sha: str) -> None:
    try:
        run(["git", "cat-file", "-e", f"{sha}^{{commit}}"], cwd=root, capture=True)
    except subprocess.CalledProcessError:
        die(f"unknown commit-ish: {sha}")


def resolve_commitish(root: Path, commitish: str) -> str:
    try:
        return git(["rev-parse", "--verify", f"{commitish}^{{commit}}"], cwd=root)
    except subprocess.CalledProcessError:
        die(f"unknown commit-ish/ref: {commitish}")


def ensure_clean_worktree(root: Path) -> None:
    st = git(["status", "--porcelain"], cwd=root)
    if st.strip():
        die("worktree is not clean; commit/stash before running this command")
