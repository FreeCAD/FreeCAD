# pyright: strict

"""Small naming helpers shared across the stub generation pipeline."""

from __future__ import annotations

import keyword
import re


def valid_identifier(name: str) -> bool:
    return name.isidentifier() and not keyword.iskeyword(name)


def safe_stub_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip(".") or "unknown"
