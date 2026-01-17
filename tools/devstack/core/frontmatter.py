from __future__ import annotations

import re
from pathlib import Path


def title_with_number(title: str, number: str) -> str:
    if not number:
        return title
    m = re.match(r"^([^:]+:)\s*(.*)$", title)
    if m:
        return f"{m.group(1)} [{number}] {m.group(2)}"
    return f"[{number}] {title}"


def _strip_quotes(s: str) -> str:
    s = s.strip()
    if len(s) >= 2 and ((s[0] == s[-1] == '"') or (s[0] == s[-1] == "'")):
        return s[1:-1]
    return s


def title_from_body_frontmatter(path: Path) -> str:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""

    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return ""

    for line in lines[1:60]:
        if line.strip() == "---":
            break
        m = re.match(r"^\s*title:\s*(.*)\s*$", line)
        if m:
            return _strip_quotes(m.group(1))
    return ""


def strip_body_frontmatter(text: str) -> str:
    lines = text.splitlines(keepends=True)
    if not lines or lines[0].strip() != "---":
        return text

    end_idx: int | None = None
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            end_idx = i
            break
    if end_idx is None:
        return text

    remainder = "".join(lines[end_idx + 1 :])
    if remainder.startswith("\n"):
        remainder = remainder[1:]
    return remainder

