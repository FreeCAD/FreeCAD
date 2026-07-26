# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``TechDraw`` module-level helpers.

This source-adjacent stub file carries the projection, export, and dimension
helper APIs exposed directly by the TechDraw application module.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from typing import Literal, TypeAlias, overload

from FreeCAD.Base import Vector
from Part import Compound, Edge, Face, Shape, Wire

_ExtentDirection: TypeAlias = Literal[0, 1]
_ProjectionSvgStyle: TypeAlias = Mapping[str, str]
_ProjectedParts: TypeAlias = tuple[Shape, Shape, Shape, Shape]
_ProjectedPartsEx: TypeAlias = tuple[Shape, Shape, Shape, Shape, Shape, Shape, Shape, Shape, Shape, Shape]

# Wire and outline helpers
def edgeWalker(edgePile: Sequence[Edge], inclBiggest: bool = True, /) -> list[Wire] | None:
    """Partition one edge pile into one or more planar wires."""
    ...

def findOuterWire(edgeList: Sequence[Edge], /) -> Wire | None:
    """Return the outer wire detected in one planar edge pile."""
    ...

def findShapeOutline(shape: Shape, scale: float, direction: Vector, /) -> Wire | None:
    """Project one shape and return the outer wire of the resulting outline."""
    ...

# View export helpers
def viewPartAsDxf(view: DrawViewPart, /) -> str:
    """Return the projected edges of one view part as DXF text."""
    ...

def viewPartAsSvg(view: DrawViewPart, /) -> str:
    """Return the projected edges of one view part as SVG markup."""
    ...

def writeDXFView(view: DrawViewPart, filename: str, alignPage: bool = True, /) -> None:
    """Export one view part directly to one DXF file."""
    ...

def writeDXFPage(page: DrawPage, filename: str, /) -> None:
    """Export one TechDraw page and its supported child views to one DXF file."""
    ...

# Geometry and dimension helpers
def findCentroid(shape: Shape, direction: Vector, /) -> Vector | None:
    """Return the projected geometric centroid of one shape in one view direction."""
    ...

def makeExtentDim(
    view: DrawViewPart,
    edges: Sequence[str],
    direction: _ExtentDirection | int,
    /,
) -> DrawViewDimension | None:
    """Create one horizontal or vertical extent dimension on a view part."""
    ...

def makeDistanceDim(
    view: DrawViewPart,
    dimType: str,
    fromPoint: Vector,
    toPoint: Vector,
    /,
) -> DrawViewDimension:
    """Create one 2D distance dimension between two view-space points."""
    ...

def makeDistanceDim3d(
    view: DrawViewPart,
    dimType: str,
    fromPoint: Vector,
    toPoint: Vector,
    /,
) -> None:
    """Create one distance dimension from two 3D model-space points."""
    ...

def makeGeomHatch(
    face: Face,
    scale: float = 1.0,
    patName: str = "",
    patFile: str = "",
    /,
) -> Compound | None:
    """Create a compound of hatch edges trimmed to one face."""
    ...

# Projection helpers
@overload
def project(shape: Shape, /) -> _ProjectedParts:
    """Project one shape with the default direction and return four result classes."""
    ...

@overload
def project(shape: Shape, direction: Vector, /) -> _ProjectedParts:
    """Project one shape in one explicit direction and return four result classes."""
    ...

@overload
def projectEx(shape: Shape, /) -> _ProjectedPartsEx:
    """Project one shape with the default direction and return ten result classes."""
    ...

@overload
def projectEx(shape: Shape, direction: Vector, /) -> _ProjectedPartsEx:
    """Project one shape in one explicit direction and return ten result classes."""
    ...

def projectToSVG(
    topoShape: Shape,
    direction: Vector | None = None,
    type: str | None = None,
    tolerance: float = 0.1,
    vStyle: _ProjectionSvgStyle | None = None,
    v0Style: _ProjectionSvgStyle | None = None,
    v1Style: _ProjectionSvgStyle | None = None,
    hStyle: _ProjectionSvgStyle | None = None,
    h0Style: _ProjectionSvgStyle | None = None,
    h1Style: _ProjectionSvgStyle | None = None,
) -> str:
    """Project one shape and return its SVG representation."""
    ...

def projectToDXF(
    topoShape: Shape,
    direction: Vector | None = None,
    type: str | None = None,
    scale: float = 1.0,
    tolerance: float = 0.1,
    /,
) -> str:
    """Project one shape and return its DXF representation."""
    ...

def removeSvgTags(svgcode: str, /) -> str:
    """Strip outer SVG and metadata tags from one SVG fragment."""
    ...

def exportSVGEdges(topoShape: Shape, /) -> str:
    """Export one shape's edges as SVG path content."""
    ...

def build3dCurves(topoShape: Shape, /) -> Shape:
    """Rebuild 3D curves on one shape and return the updated shape."""
    ...

def makeCanonicalPoint(view: DrawViewPart, point: Vector, unscale: bool = True, /) -> Vector | None:
    """Convert one point into the canonical unrotated TechDraw view-space location."""
    ...

def makeLeader(
    parent: DrawViewPart,
    points: Sequence[Vector],
    startSymbol: int = 0,
    endSymbol: int = 0,
    /,
) -> DrawLeaderLine:
    """Create one leader line attached to a view part from page-space points."""
    ...

def nearestFraction(valueWithDecimals: float, /) -> tuple[int, int]:
    """Return the nearest numerator and denominator pair for one decimal value."""
    ...
