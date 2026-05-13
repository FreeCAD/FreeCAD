# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Web`` module-level helpers.

This source-adjacent stub file carries the lightweight TCP server helpers
exposed directly by the Web application module.
"""

from __future__ import annotations

from collections.abc import Callable
from typing import TypeAlias

_ServerAddress: TypeAlias = tuple[str, int]
_ServerFirewall: TypeAlias = Callable[[str], bool]

def startServer(address: str = "127.0.0.1", port: int = 0, /) -> _ServerAddress:
    """Start one listening server and return its bound address and port."""
    ...

def waitForConnection(
    address: str = "127.0.0.1",
    port: int = 0,
    timeout: int = 0,
    /,
) -> bool:
    """Listen for one connection and report whether a client connected before timeout."""
    ...

def registerServerFirewall(firewall: _ServerFirewall | None, /) -> None:
    """Register or clear the callback that filters incoming server requests."""
    ...
