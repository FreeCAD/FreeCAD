# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCADDbg`` helper module.

This source-adjacent stub file carries the lightweight Python-debugger hooks
and stream proxies exposed by the GUI debugger module.
"""

from __future__ import annotations

class _DebuggerStdOut:
    """Opaque stdout proxy used by the FreeCAD Python debugger."""

    def write(self, message: str, /) -> None:
        """Write one message chunk to the debugger stdout stream."""
        ...

    def flush(self) -> None:
        """Flush the debugger stdout stream."""
        ...

class _DebuggerStdErr:
    """Opaque stderr proxy used by the FreeCAD Python debugger."""

    def write(self, message: str, /) -> None:
        """Write one message chunk to the debugger stderr stream."""
        ...

StdOut: _DebuggerStdOut | None
StdErr: _DebuggerStdErr | None

def getFunctionCallCount() -> None:
    """Return the debugger function-call counters when available; currently returns `None`."""
    ...

def getExceptionCount() -> None:
    """Return the debugger exception counters when available; currently returns `None`."""
    ...

def getLineCount() -> None:
    """Return the debugger line-execution counters when available; currently returns `None`."""
    ...

def getFunctionReturnCount() -> None:
    """Return the debugger function-return counters when available; currently returns `None`."""
    ...
