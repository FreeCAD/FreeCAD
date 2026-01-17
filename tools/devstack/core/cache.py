from __future__ import annotations

import os
from pathlib import Path


def devstack_cache_home() -> Path:
    """Return the cache root for devstack-managed tools.

    Resolution order:
    - DEVSTACK_CACHE_HOME
    - XDG_CACHE_HOME/devstack
    - ~/.cache/devstack
    """
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

