from __future__ import annotations

import os
import re
from dataclasses import dataclass
from pathlib import Path

from .git import current_branch, resolve_conf_path, sanitize_branch_to_conf_name, sanitize_key_to_filename
from .proc import die


@dataclass(frozen=True)
class StackEntry:
    key: str
    branch: str
    sha: str
    body: str  # may be empty (convention-based)


@dataclass(frozen=True)
class StackConfig:
    path: Path
    base_remote_ref: str
    pr_prefix: str
    cut_prefix: str
    body_dir: str
    ignore: list[str]  # commit-ish or ref (resolved at runtime)
    entries: list[StackEntry]


def stack_name_from_conf(conf_path: Path) -> str:
    return conf_path.stem


def default_body_dir(root: Path, conf_path: Path, pr_prefix: str = "") -> str:
    # Stable defaults: stack.conf uses a pr_prefix-based name (when present), otherwise fall back.
    if pr_prefix:
        p = pr_prefix.rstrip("/")
        if p.startswith("pr/"):
            p = p[3:]
        if p:
            return f".devstack/pr-bodies/{sanitize_branch_to_conf_name(p)}"

    b = current_branch(root)
    if b:
        return f".devstack/pr-bodies/{sanitize_branch_to_conf_name(b)}"

    return f".devstack/pr-bodies/{stack_name_from_conf(conf_path)}"


def base_branch_name(base_remote_ref: str) -> str:
    return base_remote_ref.split("/", 1)[1] if "/" in base_remote_ref else base_remote_ref


def read_conf(root: Path) -> StackConfig:
    conf_path = resolve_conf_path(root)
    if not conf_path.is_file():
        die(f"missing config: {conf_path}")

    base_remote_ref = ""
    pr_prefix = ""
    cut_prefix = ""
    body_dir = ""
    ignore: list[str] = []
    entries: list[StackEntry] = []

    for raw in conf_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()

        # New directive-style format (supports stacking metadata).
        if parts[0] == "base":
            if len(parts) != 2:
                die(f"bad base directive in {conf_path}: {raw}")
            base_remote_ref = parts[1]
            continue
        if parts[0] in ("pr_prefix", "prefix"):
            if len(parts) != 2:
                die(f"bad pr_prefix directive in {conf_path}: {raw}")
            pr_prefix = parts[1]
            if pr_prefix and not pr_prefix.endswith("/"):
                pr_prefix += "/"
            continue
        if parts[0] == "body_dir":
            if len(parts) != 2:
                die(f"bad body_dir directive in {conf_path}: {raw}")
            body_dir = parts[1].rstrip("/")
            continue
        if parts[0] == "cut_prefix":
            if len(parts) != 2:
                die(f"bad cut_prefix directive in {conf_path}: {raw}")
            cut_prefix = parts[1]
            if cut_prefix and not cut_prefix.endswith("/"):
                cut_prefix += "/"
            continue
        if parts[0] == "ignore":
            # Allow both the strict (1 item) and relaxed (many items) forms.
            if len(parts) < 2:
                die(f"bad ignore directive in {conf_path}: {raw}")
            ignore.extend(parts[1:])
            continue
        if parts[0] in ("commit", "c"):
            if len(parts) < 4:
                die(f"bad commit directive in {conf_path}: {raw}")
            key, branch, sha = parts[1], parts[2], parts[3]
            body = parts[4] if len(parts) >= 5 else ""
            entries.append(StackEntry(key=key, branch=branch, sha=sha, body=body))
            continue

        # Legacy format:
        #   <key> <sha> [body.md]
        if len(parts) < 2:
            die(f"bad line in {conf_path}: {raw}")
        key = parts[0]
        sha = parts[1]
        body = parts[2] if len(parts) >= 3 else ""

        branch = key
        if pr_prefix and not branch.startswith("pr/"):
            branch = f"{pr_prefix}{branch}"
        entries.append(StackEntry(key=key, branch=branch, sha=sha, body=body))

    if not body_dir:
        body_dir = default_body_dir(root, conf_path, pr_prefix)

        # If this is a stack.conf and the pr_prefix-derived directory doesn't exist, but there is
        # exactly one bodies directory present, use it (helps when the config was migrated/copied).
        if conf_path.name == "stack.conf":
            candidate_path = (root / body_dir).resolve()
            if not candidate_path.is_dir():
                bodies_root = root / ".devstack" / "pr-bodies"
                try:
                    dirs = [p for p in bodies_root.iterdir() if p.is_dir()]
                except OSError:
                    dirs = []
                if len(dirs) == 1:
                    body_dir = f".devstack/pr-bodies/{dirs[0].name}"

    if not base_remote_ref:
        remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
        main = os.environ.get("DEVSTACK_STACK_MAIN_BRANCH", "main")
        base_remote_ref = f"{remote}/{main}"

    if not cut_prefix:
        base = "stack"
        if pr_prefix:
            p = pr_prefix.rstrip("/")
            if p.startswith("pr/"):
                p = p[3:]
            if p:
                base = p
        else:
            b = current_branch(root)
            if b:
                base = sanitize_branch_to_conf_name(b.split("/")[-1])
        cut_prefix = f"cp/{sanitize_branch_to_conf_name(base)}/"

    return StackConfig(
        path=conf_path,
        base_remote_ref=base_remote_ref,
        pr_prefix=pr_prefix,
        cut_prefix=cut_prefix,
        body_dir=body_dir,
        ignore=ignore,
        entries=entries,
    )


def filtered_mode(conf: StackConfig) -> bool:
    return bool(conf.ignore)


def cut_branch_for_entry(conf: StackConfig, entry: StackEntry) -> str:
    prefix = conf.cut_prefix
    if prefix and not prefix.endswith("/"):
        prefix += "/"
    return f"{prefix}{sanitize_key_to_filename(entry.key)}"


def conf_root(conf: StackConfig) -> Path:
    from .git import repo_root

    p = conf.path.resolve()
    # <root>/.devstack/stack.conf
    if p.parent.name == ".devstack":
        return p.parent.parent
    return repo_root(p.parent)


def resolved_body_file(conf: StackConfig, entry: StackEntry) -> Path:
    root = conf_root(conf)
    return resolved_body_path(conf, entry, root=root)


def key_number(key: str) -> str:
    m = re.match(r"^([0-9]+)[-_]", key)
    return m.group(1) if m else ""


def resolved_body_path(conf: StackConfig, entry: StackEntry, *, root: Path) -> Path:
    if entry.body:
        p = Path(entry.body)
        return p if p.is_absolute() else (root / p).resolve()
    filename = f"{sanitize_key_to_filename(entry.key)}.md"
    return (root / conf.body_dir / filename).resolve()
