#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import argparse
import difflib
import os
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Line:
    content: bytes
    eol: bytes


def split_lines(data: bytes) -> list[Line]:
    """Split bytes into content/EOL pairs without normalizing line endings."""
    lines: list[Line] = []
    start = 0
    length = len(data)

    while start < length:
        end = start
        while end < length and data[end] not in (10, 13):
            end += 1

        if end >= length:
            lines.append(Line(data[start:end], b""))
            break

        if data[end] == 13 and end + 1 < length and data[end + 1] == 10:
            eol = b"\r\n"
            next_start = end + 2
        else:
            eol = data[end : end + 1]
            next_start = end + 1

        lines.append(Line(data[start:end], eol))
        start = next_start

    if not lines and data == b"":
        return []

    return lines


def dominant_eol(lines: Iterable[Line]) -> bytes:
    counts = Counter(line.eol for line in lines if line.eol)
    if not counts:
        return b"\n"
    return counts.most_common(1)[0][0]


def first_nonempty(*eols: bytes) -> bytes:
    for eol in eols:
        if eol:
            return eol
    return b""


def infer_insert_eol(old_lines: list[Line], i1: int, i2: int, dominant: bytes) -> bytes:
    prev_eol = b""
    if i1 > 0:
        prev_eol = old_lines[i1 - 1].eol

    next_eol = b""
    if i2 < len(old_lines):
        next_eol = old_lines[i2].eol

    return first_nonempty(prev_eol, next_eol, dominant, b"\n")


def assign_replacement_eols(
    old_chunk: list[Line],
    new_chunk: list[Line],
    insert_eol: bytes,
    is_final_chunk: bool,
) -> list[bytes]:
    if not new_chunk:
        return []

    base_eols = [line.eol for line in old_chunk if line.eol]
    default_eol = first_nonempty(insert_eol, (base_eols[-1] if base_eols else b""), b"\n")

    assigned: list[bytes] = []
    for idx, new_line in enumerate(new_chunk):
        if idx < len(old_chunk) and old_chunk[idx].eol:
            eol = old_chunk[idx].eol
        elif idx > 0 and assigned[idx - 1]:
            eol = assigned[idx - 1]
        elif idx < len(base_eols):
            eol = base_eols[idx]
        else:
            eol = default_eol

        is_final_output_line = is_final_chunk and idx == len(new_chunk) - 1
        if is_final_output_line and new_line.eol == b"":
            eol = b""
        elif not eol:
            eol = default_eol

        assigned.append(eol)

    return assigned


def has_terminal_newline(data: bytes) -> bool:
    return data.endswith((b"\n", b"\r"))


def rewrite_with_preserved_eol(original: bytes, modified: bytes) -> bytes:
    """Rewrite only changed regions so their EOLs inherit historical local context."""
    old_lines = split_lines(original)
    new_lines = split_lines(modified)
    old_content = [line.content for line in old_lines]
    new_content = [line.content for line in new_lines]
    eol_default = dominant_eol(old_lines)

    matcher = difflib.SequenceMatcher(a=old_content, b=new_content, autojunk=False)
    output = bytearray()

    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for line in old_lines[i1:i2]:
                output.extend(line.content)
                output.extend(line.eol)
            continue

        if tag == "delete":
            continue

        old_chunk = old_lines[i1:i2]
        new_chunk = new_lines[j1:j2]
        insert_eol = infer_insert_eol(old_lines, i1, i2, eol_default)
        assigned_eols = assign_replacement_eols(
            old_chunk,
            new_chunk,
            insert_eol,
            is_final_chunk=(j2 == len(new_lines)),
        )

        for line, eol in zip(new_chunk, assigned_eols):
            output.extend(line.content)
            output.extend(eol)

    rewritten = bytes(output)
    # Preserve an added final newline using the surrounding historical EOL style.
    # This keeps the hook compatible with end-of-file-fixer instead of toggling back
    # to HEAD's missing-final-newline state on every run.
    if has_terminal_newline(modified) and rewritten and not has_terminal_newline(rewritten):
        rewritten += infer_insert_eol(old_lines, len(old_lines), len(old_lines), eol_default)

    return rewritten


def is_binary(data: bytes) -> bool:
    return b"\0" in data


def needs_eol_preservation(data: bytes) -> bool:
    return b"\r\n" in data


def git_show(repo: Path, revision: str, path: str) -> bytes | None:
    proc = subprocess.run(
        ["git", "-C", str(repo), "show", f"{revision}:{path}"],
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        return None
    return proc.stdout


def git_text(repo: Path, args: list[str]) -> str | None:
    proc = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        check=False,
        text=True,
    )
    if proc.returncode != 0:
        return None
    return proc.stdout.strip()


def has_staged_changes(repo: Path, path: str) -> bool:
    """Return True when the index differs from HEAD for this path."""
    proc = subprocess.run(
        ["git", "-C", str(repo), "diff", "--cached", "--quiet", "--", path],
        check=False,
    )
    return proc.returncode == 1


def merge_base_with_upstream(repo: Path) -> str | None:
    """Resolve the branch point against the configured upstream, if any."""
    upstream = git_text(repo, ["rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{upstream}"])
    if not upstream:
        return None
    return git_text(repo, ["merge-base", "HEAD", upstream])


def resolve_revision(repo: Path, revision: str, path: str) -> tuple[str, bool]:
    """Choose the comparison baseline and whether to inspect the index.

    `auto` deliberately uses different baselines for two workflows:
    - staged local pre-commit runs compare against `HEAD`, because the hook is fixing
      what is about to be committed from the index/working tree
    - unstaged or post-commit checks compare against merge-base with upstream, so CI
      can still detect an already-committed EOL regression on the branch
    """
    if revision != "auto":
        return revision, has_staged_changes(repo, path)

    staged = has_staged_changes(repo, path)
    if staged:
        return "HEAD", True

    merge_base = merge_base_with_upstream(repo)
    if merge_base:
        return merge_base, False

    return "HEAD", False


def renamed_from_path(repo: Path, revision: str, path: str, staged: bool) -> str | None:
    """Map a renamed path back to its baseline name, if Git can detect one.

    The low rename threshold is intentional: once line endings have changed, the file
    may look heavily rewritten even when it is logically the same path after `git mv`.
    """
    args = ["diff", "--find-renames=1%", "--name-status"]
    if staged:
        args.extend(["--cached", revision])
    else:
        args.append(f"{revision}..HEAD")

    output = git_text(repo, args)
    if not output:
        return None

    for line in output.splitlines():
        parts = line.split("\t")
        if len(parts) != 3 or not parts[0].startswith("R"):
            continue
        _, old_path, new_path = parts
        if new_path == path:
            return old_path

    return None


def process_file(repo: Path, path_str: str, revision: str, write: bool) -> tuple[str, str]:
    """Preserve historical EOLs for one path when the baseline contains CRLF."""
    path = repo / path_str
    if not path.is_file():
        return path_str, "skip-missing"

    resolved_revision, staged = resolve_revision(repo, revision, path_str)
    original = git_show(repo, resolved_revision, path_str)
    if original is None:
        old_path = renamed_from_path(repo, resolved_revision, path_str, staged)
        if old_path is None:
            return path_str, "skip-untracked"
        original = git_show(repo, resolved_revision, old_path)
        if original is None:
            return path_str, "skip-untracked"

    modified = path.read_bytes()
    if is_binary(original) or is_binary(modified):
        return path_str, "skip-binary"
    if not needs_eol_preservation(original):
        return path_str, "skip-lf-only"

    rewritten = rewrite_with_preserved_eol(original, modified)
    if rewritten == modified:
        return path_str, "unchanged"

    if write:
        path.write_bytes(rewritten)
    return path_str, "rewritten"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Preserve historical line endings for modified files, including mixed-EOL files."
    )
    parser.add_argument("--repo", default=".", help="Path to the git repo")
    parser.add_argument(
        "--revision",
        default="auto",
        help="Revision to compare against, or `auto` to choose HEAD/merge-base as appropriate",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Do not write files, only report whether rewrites would occur",
    )
    parser.add_argument("paths", nargs="*", help="Paths to process")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo = Path(args.repo).resolve()
    if not args.paths:
        return 0

    exit_code = 0
    seen: set[str] = set()
    for path in args.paths:
        if path in seen:
            continue
        seen.add(path)
        relpath = os.path.relpath((repo / path).resolve(), repo)
        file_path, status = process_file(repo, relpath, args.revision, write=not args.check)
        if status == "rewritten":
            print(f"rewrote line endings for {file_path}")
        if args.check and status == "rewritten":
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
