# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``TechDrawGui`` module-level helpers.

This source-adjacent stub file carries GUI export helpers and the lightweight
scene/item bridge functions exposed directly by the TechDraw GUI module.
"""

from __future__ import annotations

from collections.abc import Sequence

from TechDraw import DrawPage, DrawView

class QGSPage:
    """Opaque Python wrapper for the Qt graphics scene used by one TechDraw page."""

# Page export helpers
def export(objects: Sequence[DrawPage], filename: str, /) -> None:
    """Export one or more selected TechDraw pages through the GUI export hook."""
    ...

def exportPageAsPdf(page: DrawPage, filename: str, /) -> None:
    """Export one TechDraw page as one PDF file."""
    ...

def exportPageAsSvg(page: DrawPage, filename: str, /) -> None:
    """Export one TechDraw page as one SVG file."""
    ...

# Scene and item bridge helpers
def addQGIToView(view: DrawView, item: object, /) -> None:
    """Insert one Qt graphics item into a TechDraw view's graphics item tree."""
    ...

def addQGObjToView(view: DrawView, item: object, /) -> None:
    """Insert one Qt graphics object into a TechDraw view's graphics item tree."""
    ...

def addQGIToScene(page: DrawPage, item: object, /) -> None:
    """Insert one Qt graphics item directly into a TechDraw page scene."""
    ...

def addQGObjToScene(page: DrawPage, item: object, /) -> None:
    """Insert one Qt graphics object directly into a TechDraw page scene."""
    ...

def getSceneForPage(page: DrawPage, /) -> QGSPage | None:
    """Return the scene wrapper associated with one TechDraw page."""
    ...
