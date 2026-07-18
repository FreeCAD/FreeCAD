# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Part`` module-level shape helpers.

This source-adjacent stub file carries the large factory-style function surface
that is implemented in the Part application module. Helper aliases and simple
module-level declarations that the callable surface depends on live here.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import Literal, TypeAlias, TypedDict, overload

Point3: TypeAlias = tuple[float, float, float]
ShapeSequence: TypeAlias = Sequence[Shape] | Shape
EdgeSequence: TypeAlias = Sequence[Edge]
OCCError: type[Exception]
OCCDomainError: type[Exception]
OCCRangeError: type[Exception]
OCCConstructionError: type[Exception]
OCCDimensionError: type[Exception]
_RevolutionShapeClass: TypeAlias = type[Shape]
_SingleShapeCompoundCreationPolicy: TypeAlias = int
_FilledSupportPairs: TypeAlias = Sequence[tuple[Shape, Shape]]
_FilledOrderPairs: TypeAlias = Sequence[tuple[Shape, int]]
ExportUnits = TypedDict(
    "ExportUnits",
    {
        "write.iges.unit": str,
        "write.step.unit": str,
    },
)

# File and document I/O
def open(name: str, /) -> None:
    """Open one Part-supported file into the active document context."""
    ...

def insert(name: str, doc_name: str, /) -> None:
    """Insert one Part-supported file into an existing document."""
    ...

def export(objects: Sequence[DocumentObject], name: str, /) -> None:
    """Export document objects to one Part-supported file."""
    ...

def read(name: str, /) -> Shape:
    """Read one Part-supported file and return the resulting shape."""
    ...

def show(shape: Shape, name: str = "Shape", /) -> Feature:
    """Create a document feature that shows one shape."""
    ...

# Shape extraction and inspection
def getFacets(shape: Shape, /) -> list[tuple[Point3, Point3, Point3]]:
    """Return the triangulated facet vertices of one shape."""
    ...

@overload
def getShape(
    obj: DocumentObject,
    subname: str | None = None,
    mat: Matrix | None = None,
    needSubElement: bool = False,
    transform: bool = True,
    retType: Literal[0] = 0,
    noElementMap: bool = False,
    refine: bool = False,
) -> Shape:
    """Return only the resolved shape."""
    ...

@overload
def getShape(
    obj: DocumentObject,
    subname: str | None,
    mat: Matrix | None,
    needSubElement: bool,
    transform: bool,
    retType: Literal[1, 2],
    noElementMap: bool = False,
    refine: bool = False,
) -> tuple[Shape, Matrix, DocumentObject | None]:
    """Return the shape together with placement and resolved subobject context."""
    ...

@overload
def getShape(
    obj: DocumentObject,
    subname: str | None = None,
    mat: Matrix | None = None,
    needSubElement: bool = False,
    transform: bool = True,
    retType: int = 0,
    noElementMap: bool = False,
    refine: bool = False,
) -> Shape | tuple[Shape, Matrix, DocumentObject | None]:
    """Accept any retType and reflect the broad union used at runtime."""
    ...

def cast_to_shape(shape: Shape, /) -> Shape:
    """Normalize a shape-like proxy to the public `Shape` wrapper."""
    ...

# Surface, sweep, and topology builders
@overload
def makeRevolution(
    curve: Geometry,
    vmin: float = ...,
    vmax: float = ...,
    angle: float = 360,
    point: Vector | None = None,
    direction: Vector | None = None,
    type: _RevolutionShapeClass | None = None,
    /,
) -> Shape:
    """Revolve a curve geometry into a shape, optionally choosing the result class."""
    ...

@overload
def makeRevolution(
    edge: Edge,
    vmin: float = ...,
    vmax: float = ...,
    angle: float = 360,
    point: Vector | None = None,
    direction: Vector | None = None,
    type: _RevolutionShapeClass | None = None,
    /,
) -> Shape:
    """Revolve an edge into a shape, optionally choosing the result class."""
    ...

def makeCompound(
    shapes: ShapeSequence,
    force: _SingleShapeCompoundCreationPolicy = ...,
    op: str | None = None,
) -> Compound:
    """Build a compound from one shape or a shape sequence."""
    ...

def makeShell(shapes: ShapeSequence, op: str | None = None) -> Shell:
    """Build a shell from one shape or a shape sequence."""
    ...

def makeFace(
    shapes: ShapeSequence,
    class_name: str | None = None,
    op: str | None = None,
    *,
    noElementMap: bool = False,
) -> Face:
    """Build a face from one shape or a compatible shape sequence."""
    ...

def makeFilledSurface(
    shapes: ShapeSequence,
    surface: Shape | None = None,
    supports: _FilledSupportPairs | None = None,
    orders: _FilledOrderPairs | None = None,
    degree: int = ...,
    ptsOnCurve: int = ...,
    numIter: int = ...,
    anisotropy: bool = ...,
    tol2d: float = ...,
    tol3d: float = ...,
    tolG1: float = ...,
    tolG2: float = ...,
    maxDegree: int = ...,
    maxSegments: int = ...,
    op: str | None = None,
) -> Face:
    """Build a filled surface face from boundary shapes and optional supports."""
    ...

def makeFilledFace(
    shapes: ShapeSequence,
    surface: Shape | None = None,
    supports: _FilledSupportPairs | None = None,
    orders: _FilledOrderPairs | None = None,
    degree: int = ...,
    ptsOnCurve: int = ...,
    numIter: int = ...,
    anisotropy: bool = ...,
    tol2d: float = ...,
    tol3d: float = ...,
    tolG1: float = ...,
    tolG2: float = ...,
    maxDegree: int = ...,
    maxSegments: int = ...,
    op: str | None = None,
) -> Face:
    """Build a filled face from boundary shapes and optional supports."""
    ...

def makeSolid(shape: Shape, op: str | None = None) -> Solid:
    """Convert one shell-like shape into a solid."""
    ...

def makeRuledSurface(
    path: Edge | Wire,
    profile: Edge | Wire,
    orientation: int = 0,
    op: str | None = None,
) -> Face | Shell:
    """Create a ruled surface between two path shapes."""
    ...

def makeShellFromWires(shape: ShapeSequence, op: str | None = None) -> Shell:
    """Create a shell from a compatible wire sequence."""
    ...

def makeTube(
    pshape: Shape,
    radius: float,
    scont: str = "C0",
    maxdegree: int = 3,
    maxsegment: int = 30,
    /,
) -> Face:
    """Create a tube surface around one path shape."""
    ...

def makeSweepSurface(
    path: Shape,
    profile: Shape,
    tolerance: float = 0.001,
    fillMode: int = 0,
    /,
) -> Shape:
    """Sweep one profile along one path and return the resulting shape."""
    ...

def makeLoft(
    shapes: list[Shape],
    solid: bool = False,
    ruled: bool = False,
    closed: bool = False,
    max_degree: int = 5,
    op: str | None = None,
) -> Shape:
    """Loft a sequence of section shapes into a new shape."""
    ...

# Text and OCC static helpers
@overload
def makeWireString(
    intext: str | bytes,
    dir: str,
    fontfile: str,
    height: float,
    track: float = 0,
    /,
) -> list[list[Wire]]:
    """Use the legacy ``(text, fontdir, fontfile, ...)`` calling convention."""
    ...

@overload
def makeWireString(
    intext: str | bytes, fontspec: str, height: float, track: float = 0, /
) -> list[list[Wire]]:
    """Use the newer ``(text, fontspec, ...)`` calling convention."""
    ...

@overload
def setStaticValue(name: str, cval: str, /) -> None:
    """Set an OCC interface static from a string value."""
    ...

@overload
def setStaticValue(name: str, value: int | float, /) -> None:
    """Set an OCC interface static from an integer or floating-point value."""
    ...

# Primitive constructors
def makePlane(
    length: float,
    width: float,
    pPnt: Vector | None = None,
    pDirZ: Vector | None = None,
    pDirX: Vector | None = None,
    /,
) -> Face:
    """Create a planar face from dimensions and optional orientation vectors."""
    ...

def makeBox(
    length: float,
    width: float,
    height: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    /,
) -> Solid:
    """Create a box solid from dimensions and optional placement."""
    ...

def makeWedge(
    xmin: float,
    ymin: float,
    zmin: float,
    z2min: float,
    x2min: float,
    xmax: float,
    ymax: float,
    zmax: float,
    z2max: float,
    x2max: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    /,
) -> Solid:
    """Create a wedge solid from the OCC wedge parameters."""
    ...

def makeLine(obj1: Vector | Point3, obj2: Vector | Point3, /) -> Edge:
    """Create a line edge between two points."""
    ...

def makePolygon(pcObj: Sequence[Vector | Point3], pclosed: bool = False, /) -> Wire:
    """Create a polygon wire from a point sequence."""
    ...

def makeCircle(
    radius: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    angle1: float = 0.0,
    angle2: float = 360,
    /,
) -> Edge:
    """Create a circular edge or arc."""
    ...

def makeSphere(
    radius: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    angle1: float = -90,
    angle2: float = 90,
    angle3: float = 360,
    /,
) -> Solid:
    """Create a sphere solid or spherical segment."""
    ...

def makeCylinder(
    radius: float,
    height: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    angle: float = 360,
    /,
) -> Solid:
    """Create a cylinder solid or cylindrical segment."""
    ...

def makeCone(
    radius1: float,
    radius2: float,
    height: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    angle: float = 360,
    /,
) -> Solid:
    """Create a cone or frustum solid."""
    ...

def makeTorus(
    radius1: float,
    radius2: float,
    pPnt: Vector | None = None,
    pDir: Vector | None = None,
    angle1: float = 0.0,
    angle2: float = 360,
    angle: float = 360,
    /,
) -> Solid:
    """Create a torus solid or toroidal segment."""
    ...

def makeHelix(
    pitch: float,
    height: float,
    radius: float,
    angle: float = -1.0,
    pleft: bool = False,
    pvertHeight: bool = False,
    /,
) -> Wire:
    """Create a helix wire."""
    ...

def makeLongHelix(
    pitch: float,
    height: float,
    radius: float,
    angle: float = -1.0,
    pleft: bool = False,
    /,
) -> Wire:
    """Create a long helix wire using the extended implementation."""
    ...

def makeThread(pitch: float, depth: float, height: float, radius: float, /) -> Wire:
    """Create a thread-profile wire."""
    ...

# Shape utilities and interoperability
def makeSplitShape(
    shape: Shape,
    splits: Sequence[tuple[Shape, Shape]],
    checkInterior: bool = True,
    /,
) -> tuple[list[Shape], list[Shape]]:
    """Split a shape by splitter pairs and return outside and inside fragments."""
    ...

def exportUnits(unit: str | None = None, /) -> ExportUnits:
    """Return the current Part export-unit configuration."""
    ...

def getSortedClusters(obj: EdgeSequence, /) -> list[list[Edge]]:
    """Group edges into connected clusters."""
    ...

def __sortEdges__(obj: EdgeSequence, /) -> list[Edge]:
    """Return one legacy sorted edge sequence."""
    ...

def sortEdges(obj: EdgeSequence, tol3d: float | None = None, /) -> list[list[Edge]]:
    """Group and order connected edges with an optional tolerance override."""
    ...

def __toPythonOCC__(shape: Shape, /) -> object:
    """Convert one Part shape to a PythonOCC proxy object."""
    ...

def __fromPythonOCC__(proxy: object, /) -> Shape:
    """Convert one PythonOCC proxy object to a Part shape."""
    ...

def clearShapeCache() -> None:
    """Clear the process-wide Part shape conversion cache."""
    ...

def splitSubname(subname: str, /) -> list[str]:
    """Split one mapped subname string into its path components."""
    ...

def joinSubname(sub: str, mapped: str, element: str, /) -> str:
    """Join mapped subname components into one canonical string."""
    ...
