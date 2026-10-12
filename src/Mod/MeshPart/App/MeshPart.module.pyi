# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``MeshPart`` projection and meshing helpers.

This source-adjacent stub file carries the standalone MeshPart functions that
bridge Part topology and Mesh data.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import TypeAlias, overload

from FreeCAD.Base import Vector
from Mesh import Mesh
from Part import Edge, Shape, Wire

_Point2: TypeAlias = tuple[float, float]
_Point3: TypeAlias = tuple[float, float, float]
_Color: TypeAlias = tuple[float, float, float]
_LoftPoint: TypeAlias = _Point2 | _Point3
_VectorLike: TypeAlias = Vector | _Point3
_ProjectedPolyline: TypeAlias = list[Vector]
_ProjectedPolylines: TypeAlias = list[_ProjectedPolyline]

def loftOnCurve(
    curve: Shape, poly: Sequence[_LoftPoint], up_vector: _Point3, max_size: float, /
) -> Mesh:
    """Create one lofted mesh by sweeping a polygon along a curve-like shape."""
    ...

def findSectionParameters(edge: Edge, mesh: Mesh, direction: Vector, /) -> list[float]:
    """Return edge parameters whose projected points land on mesh edges."""
    ...

@overload
def projectShapeOnMesh(Shape: Shape, Mesh: Mesh, MaxDistance: float) -> _ProjectedPolylines:
    """Project one shape to the mesh within a maximum search distance."""
    ...

@overload
def projectShapeOnMesh(Shape: Shape, Mesh: Mesh, Direction: Vector) -> _ProjectedPolylines:
    """Project one shape to the mesh along one explicit direction."""
    ...

@overload
def projectShapeOnMesh(
    Polygons: Sequence[Sequence[_VectorLike]],
    Mesh: Mesh,
    Direction: Vector,
) -> _ProjectedPolylines:
    """Project one sampled polygon sequence to the mesh along one direction."""
    ...

def projectPointsOnMesh(
    points: Sequence[_VectorLike],
    mesh: Mesh,
    direction: Vector,
    precision: float = -1.0,
    /,
) -> list[Vector]:
    """Project points to the mesh along one direction and return the hits."""
    ...

def wireFromSegment(mesh: Mesh, segment: Sequence[int], /) -> list[Wire]:
    """Create boundary wires from one mesh segment given by facet indices."""
    ...

def wireFromMesh(mesh: Mesh, /) -> list[Wire]:
    """Create boundary wires for the full mesh border."""
    ...

@overload
def meshFromShape(Shape: Shape, /) -> Mesh:
    """Create a mesh from one shape using the default mesher settings."""
    ...

@overload
def meshFromShape(
    Shape: Shape,
    LinearDeflection: float,
    AngularDeflection: float = 0.5,
    Relative: bool = False,
    Segments: bool = False,
    GroupColors: Sequence[_Color] | None = None,
) -> Mesh:
    """Create a mesh from one shape using the standard deflection-based mesher."""
    ...

@overload
def meshFromShape(Shape: Shape, /, *, MaxLength: float) -> Mesh:
    """Create a mesh from one shape using a maximum edge-length target."""
    ...

@overload
def meshFromShape(Shape: Shape, /, *, MaxArea: float) -> Mesh:
    """Create a mesh from one shape using a maximum facet-area target."""
    ...

@overload
def meshFromShape(Shape: Shape, /, *, LocalLength: float) -> Mesh:
    """Create a mesh from one shape using one local target edge length."""
    ...

@overload
def meshFromShape(Shape: Shape, /, *, Deflection: float) -> Mesh:
    """Create a mesh from one shape using one mefisto deflection value."""
    ...

@overload
def meshFromShape(Shape: Shape, /, *, MinLength: float, MaxLength: float) -> Mesh:
    """Create a mesh from one shape using explicit minimum and maximum lengths."""
    ...

@overload
def meshFromShape(
    Shape: Shape,
    /,
    *,
    Fineness: int,
    SecondOrder: bool = False,
    Optimize: bool = True,
    AllowQuad: bool = False,
    MinLength: float = 0.0,
    MaxLength: float = 0.0,
) -> Mesh:
    """Create a mesh from one shape using the netgen fineness-driven settings."""
    ...

@overload
def meshFromShape(
    Shape: Shape,
    /,
    *,
    GrowthRate: float = 0.0,
    SegPerEdge: float = 0.0,
    SegPerRadius: float = 0.0,
    SecondOrder: bool = False,
    Optimize: bool = True,
    AllowQuad: bool = False,
    MinLength: float = 0.0,
    MaxLength: float = 0.0,
) -> Mesh:
    """Create a mesh from one shape using the user-tuned netgen settings."""
    ...
