from __future__ import annotations

import importlib
import os
import pkgutil
from pathlib import Path

from .proc import die


def iter_adapters() -> list[object]:
    adapters_pkg = importlib.import_module("tools.devstack.adapters")
    modules: list[object] = []
    for mod in pkgutil.iter_modules(getattr(adapters_pkg, "__path__", [])):
        if mod.name.startswith("_"):
            continue
        module = importlib.import_module(f"{adapters_pkg.__name__}.{mod.name}")
        if getattr(module, "ADAPTER_NAME", ""):
            modules.append(module)
    return modules


def load_adapter(root: Path, name: str):
    want = (name or "").strip() or os.environ.get("DEVSTACK_ADAPTER", "auto")
    want = want.strip() or "auto"

    adapters = iter_adapters()
    names = sorted({str(getattr(a, "ADAPTER_NAME", "")) for a in adapters if getattr(a, "ADAPTER_NAME", "")})

    if want == "auto":
        matches = []
        for a in adapters:
            is_repo = getattr(a, "is_repo", None)
            if callable(is_repo) and bool(is_repo(root)):
                matches.append(a)
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            die(
                f"multiple adapters match this repo: {', '.join(sorted(m.ADAPTER_NAME for m in matches))}; pass --adapter"
            )
        die(f"could not auto-detect adapter; pass --adapter (available: {', '.join(names) or '(none)'})")

    for a in adapters:
        if getattr(a, "ADAPTER_NAME", "") == want:
            return a

    die(f"unknown adapter: {want} (available: {', '.join(names) or '(none)'})")

