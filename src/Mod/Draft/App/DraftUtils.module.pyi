# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``DraftUtils`` compatibility helpers.

This source-adjacent stub file carries the deprecated Draft utility shim that
still exists for DXF import compatibility.
"""

from __future__ import annotations

def readDXF(
    filename: str,
    document: str | None = None,
    ignore_errors: bool = True,
    /,
) -> None:
    """Warn that the legacy Draft DXF helper is removed and return `None`."""
    ...
