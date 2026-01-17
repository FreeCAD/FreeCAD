#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path


def devstack_cache_home() -> Path:
    """Return the cache root for devstack-managed tooling.

    Resolution order:
    - DEVSTACK_CACHE_HOME
    - XDG_CACHE_HOME/devstack
    - ~/.cache/devstack

    This file lives under tools/lint/ so lint scripts can use it even when they
    run outside of the devstack Python package context.
    """
    try:
        from tools.devstack.core.cache import devstack_cache_home as _devstack_cache_home

        return _devstack_cache_home()
    except Exception:
        pass

    env = os.environ.get("DEVSTACK_CACHE_HOME", "").strip()
    if env:
        return Path(env).expanduser().resolve()

    xdg = os.environ.get("XDG_CACHE_HOME", "").strip()
    base = Path(xdg).expanduser().resolve() if xdg else (Path.home() / ".cache")
    return (base / "devstack").resolve()


def devstack_cache_dir(*parts: str) -> Path:
    p = devstack_cache_home()
    for part in parts:
        if part:
            p = p / part
    return p

