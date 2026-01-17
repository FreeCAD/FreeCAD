#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from collections import defaultdict

from utils import (
    add_common_arguments,
    append_file,
    expand_files,
    init_environment,
    write_file,
)


def find_case_collisions(paths: list[str]) -> list[list[str]]:
    buckets: dict[str, list[str]] = defaultdict(list)
    for p in paths:
        buckets[p.casefold()].append(p)

    collisions: list[list[str]] = []
    for items in buckets.values():
        uniq = sorted(set(items))
        if len(uniq) > 1:
            collisions.append(uniq)

    collisions.sort(key=lambda g: (g[0].casefold(), g))
    return collisions


def generate_markdown_report(collisions: list[list[str]]) -> str:
    if not collisions:
        return ":heavy_check_mark: No case-colliding paths found\n\n"

    lines: list[str] = []
    lines.append(
        f"<details><summary>:warning: Found {len(collisions)} case-colliding path group(s)</summary>"
    )
    lines.append("")
    lines.append("````")
    for group in collisions:
        lines.append(" / ".join(group))
    lines.append("````")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Detect file path collisions on case-insensitive filesystems."
    )
    add_common_arguments(parser)
    args = parser.parse_args()
    init_environment(args)

    files = expand_files(args.files, only_existing=False)
    if not files:
        sys.exit(0)

    collisions = find_case_collisions(files)

    log_path = f"{args.log_dir}/case-collisions.log"
    log_text = "\n".join(" / ".join(group) for group in collisions) + ("\n" if collisions else "")
    write_file(log_path, log_text)

    append_file(args.report_file, generate_markdown_report(collisions))
    sys.exit(1 if collisions else 0)


if __name__ == "__main__":
    main()

