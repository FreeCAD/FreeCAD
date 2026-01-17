from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path

from tools.devstack.core.frontmatter import title_with_number
from tools.devstack.core.git import ensure_commit_exists, git, repo_root
from tools.devstack.core.proc import die, run
from tools.devstack.core.stackconf import base_branch_name, filtered_mode, key_number, read_conf, resolved_body_file


def autogen_block(
    base_ref: str,
    pr_base: str,
    stack_pos: int,
    stack_total: int,
    from_ref: str,
    to_ref: str,
    commits: str,
) -> str:
    commits_lines = "- (none)"
    if commits.strip():
        formatted: list[str] = []
        for line in commits.splitlines():
            parts = line.split(maxsplit=1)
            if parts and re.fullmatch(r"[0-9a-f]{7,40}", parts[0]):
                rest = parts[1] if len(parts) > 1 else ""
                formatted.append(f"- `{parts[0]}` {rest}".rstrip())
            else:
                formatted.append(f"- {line}")
        commits_lines = "\n".join(formatted)
    return "\n".join(
        [
            "<!-- AUTOGEN:BEGIN -->",
            "### Patch Set",
            "",
            "> [!IMPORTANT]",
            f"> This PR is part of a stacked series (`{stack_pos}/{stack_total}`) and depends on `{pr_base}` (the PR base branch). Review/merge in order.",
            "",
            f"- Base: `{base_ref}`",
            f"- PR base (depends-on): `{pr_base}`",
            f"- Stack: `{stack_pos}/{stack_total}`",
            f"- Range: `{from_ref}..{to_ref}`",
            "",
            "#### Commits",
            commits_lines,
            "",
            "<!-- AUTOGEN:END -->",
        ]
    )


AUTOGEN_RE = re.compile(r"<!-- AUTOGEN:BEGIN -->[\s\S]*?<!-- AUTOGEN:END -->", re.MULTILINE)


def update_body_file(body_path: Path, autogen: str, *, title: str = "") -> None:
    if not body_path.exists():
        body_path.parent.mkdir(parents=True, exist_ok=True)
        frontmatter = ""
        if title:
            frontmatter = "\n".join(["---", f"title: {json.dumps(title)}", "---", ""])
        content = "\n".join(
            [
                frontmatter + "## Summary" if frontmatter else "## Summary",
                "",
                "## Why",
                "",
                "## Changes",
                "",
                "## Testing",
                "",
                autogen,
                "",
            ]
        )
        body_path.write_text(content + "\n", encoding="utf-8")
        print(f"created {body_path}")
        return

    content = body_path.read_text(encoding="utf-8", errors="replace")
    if "<!-- AUTOGEN:BEGIN -->" in content and "<!-- AUTOGEN:END -->" in content:
        stripped = AUTOGEN_RE.sub("", content).rstrip()
        new_content = (stripped + "\n\n" + autogen + "\n") if stripped else (autogen + "\n")
        body_path.write_text(new_content, encoding="utf-8")
        print(f"updated {body_path}")
        return

    body_path.write_text(content.rstrip() + "\n\n" + autogen + "\n", encoding="utf-8")
    print(f"appended {body_path}")


def cmd_body_refresh(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    base_display = base_branch_name(conf.base_remote_ref)
    prev = conf.base_remote_ref
    pr_base = base_display
    total = len(conf.entries)
    for idx, entry in enumerate(conf.entries, start=1):
        from_ref = prev
        to_ref = entry.branch if filtered_mode(conf) else entry.sha
        if filtered_mode(conf):
            try:
                run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{entry.branch}"], cwd=root)
            except subprocess.CalledProcessError:
                die(f"missing local branch: {entry.branch} (run: devstack.sh update)")
        else:
            ensure_commit_exists(root, entry.sha)
        commits = git(["log", "--oneline", "--no-decorate", f"{from_ref}..{to_ref}"], cwd=root)
        autogen = autogen_block(
            base_ref=base_display,
            pr_base=pr_base,
            stack_pos=idx,
            stack_total=total,
            from_ref=from_ref,
            to_ref=to_ref,
            commits=commits,
        )
        body_path = resolved_body_file(conf, entry)
        if entry.body and not body_path.exists():
            die(f"missing body file for {entry.branch}: {body_path}")
        raw_title = git(["show", "-s", "--format=%s", to_ref], cwd=root) or entry.branch
        title = title_with_number(raw_title, key_number(entry.key))
        update_body_file(body_path, autogen, title=title)
        prev = entry.branch if filtered_mode(conf) else entry.sha
        pr_base = entry.branch


def cmd_body_context(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    want = args.branch or ""
    base_display = base_branch_name(conf.base_remote_ref)
    prev = conf.base_remote_ref
    for entry in conf.entries:
        if want and want not in (entry.branch, entry.key):
            prev = entry.branch if filtered_mode(conf) else entry.sha
            continue
        from_ref = prev
        to_ref = entry.branch if filtered_mode(conf) else entry.sha
        if filtered_mode(conf):
            try:
                run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{entry.branch}"], cwd=root)
            except subprocess.CalledProcessError:
                die(f"missing local branch: {entry.branch} (run: devstack.sh update)")
        else:
            ensure_commit_exists(root, entry.sha)
        print()
        print(f"## Context: `{entry.branch}`")
        print()
        print(f"- Base: `{base_display}`")
        print(f"- Range: `{from_ref}..{to_ref}`")
        print()
        print("### Commits")
        run(["git", "log", "--oneline", "--no-decorate", f"{from_ref}..{to_ref}"], cwd=root, check=False)
        print()
        print("### Top Files By Churn")
        numstat = git(["diff", "--numstat", f"{from_ref}..{to_ref}"], cwd=root)
        churn: list[tuple[int, int, int, str]] = []
        for line in numstat.splitlines():
            parts = line.split("\t")
            if len(parts) != 3:
                continue
            a, d, p = parts
            if not a.isdigit() or not d.isdigit():
                continue
            ai = int(a)
            di = int(d)
            churn.append((ai + di, ai, di, p))
        for total, ai, di, p in sorted(churn, reverse=True)[:20]:
            print(f"- {p} (+{ai} -{di})")
        print()
        print("### Areas Touched")
        names = git(["diff", "--name-only", f"{from_ref}..{to_ref}"], cwd=root)
        counts: dict[str, int] = {}
        for p in names.splitlines():
            segs = p.split("/")
            prefix = "/".join(segs[:2]) if len(segs) >= 2 else p
            counts[prefix] = counts.get(prefix, 0) + 1
        for prefix, c in sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))[:15]:
            print(f"- {c} files in {prefix}")
        prev = entry.branch if filtered_mode(conf) else entry.sha


def _is_under_dir(path: Path, root_dir: Path) -> bool:
    try:
        path.resolve().relative_to(root_dir.resolve())
        return True
    except Exception:
        return False


def cmd_body_prune(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)

    body_dir_cfg = Path(conf.body_dir)
    body_dir = body_dir_cfg if body_dir_cfg.is_absolute() else (root / body_dir_cfg)
    body_dir = body_dir.resolve()

    if not body_dir.exists():
        print(f"body dir does not exist: {body_dir}")
        return
    if not body_dir.is_dir():
        die(f"body dir is not a directory: {body_dir}")

    keep: set[Path] = set()
    for entry in conf.entries:
        rp = resolved_body_file(conf, entry).resolve()
        if _is_under_dir(rp, body_dir):
            keep.add(rp)

    candidates = [p.resolve() for p in body_dir.rglob("*.md") if p.is_file()]
    stale = [p for p in candidates if p not in keep]

    if not stale:
        print(f"no stale body files under {body_dir}")
        return

    stale.sort()
    print(f"stale body files under {body_dir}:")
    for pth in stale:
        rel = pth.relative_to(body_dir)
        print(f"  {rel}")

    if not getattr(args, "apply", False):
        print("\n(dry-run) pass --apply to delete these files")
        return

    for pth in stale:
        try:
            pth.unlink()
        except OSError as exc:
            die(f"failed to delete {pth}: {exc}")
    print(f"\ndeleted {len(stale)} file(s)")

