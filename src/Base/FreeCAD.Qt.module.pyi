# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCAD.Qt`` translation helpers.

These helpers are surfaced as a small Python module. The noop variants return
their source text unchanged while still marking strings for translation tools.
"""

from __future__ import annotations

from typing import TypeVar

_T = TypeVar("_T")

# Translation helpers
def translate(
    context: str, sourcetext: str, disambiguation: str | None = None, n: int = -1, /
) -> str:
    """Translate one source string in a Qt translation context."""
    ...

def QT_TRANSLATE_NOOP(context: str, sourcetext: _T, /) -> _T:
    """Return one string unchanged while marking it for contextual translation."""
    ...

def QT_TRANSLATE_NOOP3(context: str, sourcetext: _T, disambiguation: str, /) -> _T:
    """Return one string unchanged while marking it with disambiguation metadata."""
    ...

def QT_TRANSLATE_NOOP_UTF8(context: str, sourcetext: _T, /) -> _T:
    """Return one UTF-8 string unchanged while marking it for contextual translation."""
    ...

def QT_TR_NOOP(sourcetext: _T, /) -> _T:
    """Return one string unchanged while marking it for default-context translation."""
    ...

def QT_TR_NOOP_UTF8(sourcetext: _T, /) -> _T:
    """Return one UTF-8 string unchanged while marking it for default-context translation."""
    ...

# Translator installation
def installTranslator(filename: str, /) -> bool:
    """Load and install one translator file."""
    ...

def removeTranslators() -> bool:
    """Remove all translators that FreeCAD installed through this helper."""
    ...
