# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Mesh`` module-level helpers.

This source-adjacent stub file carries the file I/O helpers, primitive mesh
constructors, and point-cloud utilities exposed by the Mesh application
module.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import TypeAlias, TypedDict, overload

from FreeCAD import DocumentObject
from FreeCAD.Base import BoundBox, Placement, Vector

_Point3: TypeAlias = tuple[float, float, float]
_PolynomialCoefficients: TypeAlias = tuple[float, float, float, float, float, float]
_MinimumVolumeBoxResult: TypeAlias = tuple[Vector, Vector, Vector, Vector, float, float, float]
PolynomialFitResult = TypedDict(
    "PolynomialFitResult",
    {
        "Sigma": float,
        "Coefficients": _PolynomialCoefficients,
        "Residuals": tuple[float, ...],
    },
)

# File and document I/O
def read(name: str, /) -> Mesh:
    """Read one mesh file and return the resulting `Mesh` object."""
    ...

def open(name: str, /) -> None:
    """Open one mesh file into a new document."""
    ...

@overload
def insert(name: str, /) -> None:
    """Insert one mesh file into the active document, creating one if needed."""
    ...

@overload
def insert(name: str, doc_name: str, /) -> None:
    """Insert one mesh file into the named document, creating it if needed."""
    ...

def export(
    objectList: Sequence[DocumentObject],
    filename: str,
    tolerance: float = 0.1,
    exportAmfCompressed: bool = True,
) -> None:
    """Export document objects to one mesh file."""
    ...

def show(mesh: Mesh, name: str = "Mesh", /) -> Feature:
    """Create one mesh feature in the active document from a `Mesh` object."""
    ...

# Primitive mesh builders
@overload
def createBox(
    length: float = 10.0,
    width: float = 10.0,
    height: float = 10.0,
    edge_length: float = -1.0,
    /,
) -> Mesh:
    """Create a box mesh from dimensions and an optional target edge length."""
    ...

@overload
def createBox(box: BoundBox, /) -> Mesh:
    """Create a box mesh from one existing bounding box."""
    ...

def createPlane(x: float = 1.0, y: float = 0.0, z: float = 0.0, /) -> Mesh:
    """Create a two-triangle XY plane; when `y` is zero it defaults to `x`."""
    ...

def createSphere(radius: float = 5.0, sampling: int = 50, /) -> Mesh:
    """Create a tessellated sphere mesh."""
    ...

def createEllipsoid(radius1: float = 2.0, radius2: float = 4.0, sampling: int = 50, /) -> Mesh:
    """Create a tessellated ellipsoid mesh."""
    ...

def createCylinder(
    radius: float = 2.0,
    length: float = 10.0,
    closed: bool = True,
    edge_length: float = 1.0,
    sampling: int = 50,
    /,
) -> Mesh:
    """Create a tessellated cylinder mesh."""
    ...

def createCone(
    radius1: float = 2.0,
    radius2: float = 4.0,
    length: float = 10.0,
    closed: bool = True,
    edge_length: float = 1.0,
    sampling: int = 50,
    /,
) -> Mesh:
    """Create a tessellated cone mesh."""
    ...

def createTorus(radius1: float = 10.0, radius2: float = 2.0, sampling: int = 50, /) -> Mesh:
    """Create a tessellated torus mesh."""
    ...

# Point-cloud helpers
def calculateEigenTransform(points: Sequence[Vector], /) -> Placement:
    """Return the principal-axis placement for one point set."""
    ...

def polynomialFit(points: Sequence[Vector], /) -> PolynomialFitResult:
    """Fit a quadratic surface to one point set and return fit diagnostics."""
    ...

def minimumVolumeOrientedBox(points: Sequence[Vector], /) -> _MinimumVolumeBoxResult:
    """Return the oriented minimum-volume box center, axes, and extents."""
    ...
