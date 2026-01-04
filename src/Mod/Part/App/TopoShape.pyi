# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Base.Matrix import Matrix
from Base.BoundBox import BoundBox
from App.ComplexGeoData import ComplexGeoData
from typing import Final, List, Tuple, Union, overload

@export(
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="App/ComplexGeoDataPy.h",
    FatherNamespace="Data",
    Constructor=True,
)
class TopoShape(ComplexGeoData):
    """
    TopoShape is the OpenCasCade topological shape wrapper.
    Sub-elements such as vertices, edges or faces are accessible as:
    * Vertex#, where # is in range(1, number of vertices)
    * Edge#, where # is in range(1, number of edges)
    * Face#, where # is in range(1, number of faces)
    """

    ShapeType: Final[str] = ""
    """Returns the type of the shape."""

    Orientation: str = ""
    """Returns the orientation of the shape."""

    Faces: Final[List] = []
    """List of faces in this shape."""

    Vertexes: Final[List] = []
    """List of vertexes in this shape."""

    Shells: Final[List] = []
    """List of subsequent shapes in this shape."""

    Solids: Final[List] = []
    """List of subsequent shapes in this shape."""

    CompSolids: Final[List] = []
    """List of subsequent shapes in this shape."""

    Edges: Final[List] = []
    """List of Edges in this shape."""

    Wires: Final[List] = []
    """List of wires in this shape."""

    Compounds: Final[List] = []
    """List of compounds in this shape."""

    SubShapes: Final[List] = []
    """List of sub-shapes in this shape."""

    Length: Final[float] = 0.0
    """Total length of the edges of the shape."""

    Area: Final[float] = 0.0
    """Total area of the faces of the shape."""

    Volume: Final[float] = 0.0
    """Total volume of the solids of the shape."""

    @constmethod
    def dumps(self) -> str:
        """
        Serialize the content of this shape to a string in BREP format.
        """
        ...

    def loads(self, brep_str: str, /) -> None:
        """
        Deserialize the content of this shape from a string in BREP format.
        """
        ...

    def read(self, filename: str, /) -> None:
        """
        Read in an IGES, STEP or BREP file.
        read(filename)
        """
        ...

    @constmethod
    def writeInventor(
        self, *, Mode: int, Deviation: float, Angle: float, FaceColors: object
    ) -> str:
        """
        Write the mesh in OpenInventor format to a string.
        writeInventor() -> string
        """
        ...

    @constmethod
    def exportIges(self, filename: str, /) -> None:
        """
        Export the content of this shape to an IGES file.
        exportIges(filename)
        """
        ...

    @constmethod
    def exportStep(self, filename: str, /) -> None:
        """
        Export the content of this shape to an STEP file.
        exportStep(filename)
        """
        ...

    @constmethod
    def exportBrep(self, filename: str, /) -> None:
        """
        Export the content of this shape to an BREP file.
        exportBrep(filename)
        --
        BREP is an OpenCasCade native format.
        """
        ...

    @constmethod
    def exportBinary(self, filename: str, /) -> None:
        """
        Export the content of this shape in binary format to a file.
        exportBinary(filename)
        """
        ...

    @constmethod
    def exportBrepToString(self) -> str:
        """
        Export the content of this shape to a string in BREP format.
        exportBrepToString() -> string
        --
        BREP is an OpenCasCade native format.
        """
        ...

    @constmethod
    def dumpToString(self) -> str:
        """
        Dump information about the shape to a string.
        dumpToString() -> string
        """
        ...

    @constmethod
    def exportStl(self, filename: str, /) -> None:
        """
        Export the content of this shape to an STL mesh file.
        exportStl(filename)
        """
        ...

    def importBrep(self, filename: str, /) -> None:
        """
        Load the shape from a file in BREP format.
        importBrep(filename)
        """
        ...

    def importBinary(self, filename: str, /) -> None:
        """
        Import the content to this shape of a string in BREP format.
        importBinary(filename)
        """
        ...

    def importBrepFromString(self, s: str, displayProgressBar: bool = True, /) -> None:
        """
        Load the shape from a string that keeps the content in BREP format.
        importBrepFromString(string, [displayProgressBar=True])
        --
        importBrepFromString(str, False) to not display a progress bar.
        """
        ...

    @constmethod
    def extrude(self, vector: Vector, /) -> TopoShape:
        """
        Extrude the shape along a vector.
        extrude(vector) -> Shape
        --
        Shp2 = Shp1.extrude(App.Vector(0,0,10)) - extrude the shape 10 mm in the +Z direction.
        """
        ...

    @constmethod
    def revolve(self, base: Vector, direction: Vector, angle: float, /) -> TopoShape:
        """
        Revolve the shape around an Axis to a given degree.
        revolve(base, direction, angle)
        --
        Part.revolve(App.Vector(0,0,0),App.Vector(0,0,1),360) - revolves the shape around the Z Axis 360 degree.

        Hints: Sometimes you want to create a rotation body out of a closed edge or wire.
        Example:
        from FreeCAD import Base
        import Part
        V=Base.Vector

        e=Part.Ellipse()
        s=e.toShape()
        r=s.revolve(V(0,0,0),V(0,1,0), 360)
        Part.show(r)

        However, you may possibly realize some rendering artifacts or that the mesh
        creation seems to hang. This is because this way the surface is created twice.
        Since the curve is a full ellipse it is sufficient to do a rotation of 180 degree
        only, i.e. r=s.revolve(V(0,0,0),V(0,1,0), 180)

        Now when rendering this object you may still see some artifacts at the poles. Now the
        problem seems to be that the meshing algorithm doesn't like to rotate around a point
        where there is no vertex.

        The idea to fix this issue is that you create only half of the ellipse so that its shape
        representation has vertexes at its start and end point.

        from FreeCAD import Base
        import Part
        V=Base.Vector

        e=Part.Ellipse()
        s=e.toShape(e.LastParameter/4,3*e.LastParameter/4)
        r=s.revolve(V(0,0,0),V(0,1,0), 360)
        Part.show(r)
        """
        ...

    @constmethod
    def check(self, runBopCheck: bool = False, /) -> bool:
        """
        Checks the shape and report errors in the shape structure.
        check([runBopCheck = False])
        --
        This is a more detailed check as done in isValid().
        if runBopCheck is True, a BOPCheck analysis is also performed.
        """
        ...

    @constmethod
    def fuse(self, tools: Tuple[TopoShape, ...], tolerance: float = 0.0, /) -> TopoShape:
        """
        Union of this and a given (list of) topo shape.
        fuse(tool) -> Shape
          or
        fuse((tool1,tool2,...),[tolerance=0.0]) -> Shape
        --
        Union of this and a given list of topo shapes.

        Supports (OCCT 6.9.0 and above):
        - Fuzzy Boolean operations (global tolerance for a Boolean operation)
        - Support of multiple arguments for a single Boolean operation
        - Parallelization of Boolean Operations algorithm

        Beginning from OCCT 6.8.1 a tolerance value can be specified.
        """
        ...

    @constmethod
    def multiFuse(self, tools: Tuple[TopoShape, ...], tolerance: float = 0.0, /) -> TopoShape:
        """
        Union of this and a given list of topo shapes.
        multiFuse((tool1,tool2,...),[tolerance=0.0]) -> Shape
        --
        Supports (OCCT 6.9.0 and above):
        - Fuzzy Boolean operations (global tolerance for a Boolean operation)
        - Support of multiple arguments for a single Boolean operation
        - Parallelization of Boolean Operations algorithm

        Beginning from OCCT 6.8.1 a tolerance value can be specified.
        Deprecated: use fuse() instead.
        """
        ...

    @constmethod
    def common(self, tools: Tuple[TopoShape, ...], tolerance: float = 0.0, /) -> TopoShape:
        """
        Intersection of this and a given (list of) topo shape.
        common(tool) -> Shape
          or
        common((tool1,tool2,...),[tolerance=0.0]) -> Shape
        --
        Supports:
        - Fuzzy Boolean operations (global tolerance for a Boolean operation)
        - Support of multiple arguments for a single Boolean operation (s1 AND (s2 OR s3))
        - Parallelization of Boolean Operations algorithm

        OCC 6.9.0 or later is required.
        """
        ...

    @constmethod
    def section(
        self, tool: Tuple[TopoShape, ...], tolerance: float = 0.0, approximation: bool = False, /
    ) -> TopoShape:
        """
        Section of this with a given (list of) topo shape.
        section(tool,[approximation=False]) -> Shape
          or
        section((tool1,tool2,...),[tolerance=0.0, approximation=False]) -> Shape
        --
        If approximation is True, section edges are approximated to a C1-continuous BSpline curve.

        Supports:
        - Fuzzy Boolean operations (global tolerance for a Boolean operation)
        - Support of multiple arguments for a single Boolean operation (s1 AND (s2 OR s3))
        - Parallelization of Boolean Operations algorithm

        OCC 6.9.0 or later is required.
        """
        ...

    @constmethod
    def slices(self, direction: Vector, distancesList: List[float], /) -> List:
        """
        Make slices of this shape.
        slices(direction, distancesList) --> Wires
        """
        ...

    @constmethod
    def slice(self, direction: Vector, distance: float, /) -> List:
        """
        Make single slice of this shape.
        slice(direction, distance) --> Wires
        """
        ...

    @constmethod
    def cut(self, tool: Tuple[TopoShape, ...], tolerance: float = 0.0, /) -> TopoShape:
        """
        Difference of this and a given (list of) topo shape
        cut(tool) -> Shape
          or
        cut((tool1,tool2,...),[tolerance=0.0]) -> Shape
        --
        Supports:
        - Fuzzy Boolean operations (global tolerance for a Boolean operation)
        - Support of multiple arguments for a single Boolean operation
        - Parallelization of Boolean Operations algorithm

        OCC 6.9.0 or later is required.
        """
        ...

    @constmethod
    def generalFuse(
        self, shapes: Tuple[TopoShape, ...], fuzzy_value: float = 0.0, /
    ) -> Tuple[TopoShape, List[List[TopoShape]]]:
        """
        Run general fuse algorithm (GFA) between this and given shapes.
        generalFuse(list_of_other_shapes, [fuzzy_value = 0.0]) -> (result, map)
        --
        list_of_other_shapes: shapes to run the algorithm against (the list is
        effectively prepended by 'self').

        fuzzy_value: extra tolerance to apply when searching for interferences, in
        addition to tolerances of the input shapes.

        Returns a tuple of 2: (result, map).

        result is a compound containing all the pieces generated by the algorithm
        (e.g., for two spheres, the pieces are three touching solids). Pieces that
        touch share elements.

        map is a list of lists of shapes, providing the info on which children of
        result came from which argument. The length of list is equal to length of
        list_of_other_shapes + 1. First element is a list of pieces that came from
        shape of this, and the rest are those that come from corresponding shapes in
        list_of_other_shapes.
        hint: use isSame method to test shape equality

        Parallelization of Boolean Operations algorithm

        OCC 6.9.0 or later is required.
        """
        ...

    def sewShape(self) -> None:
        """
        Sew the shape if there is a gap.
        sewShape()
        """
        ...

    @constmethod
    def childShapes(self, cumOri: bool = True, cumLoc: bool = True, /) -> List:
        """
        Return a list of sub-shapes that are direct children of this shape.
        childShapes([cumOri=True, cumLoc=True]) -> list
        --
        * If cumOri is true, the function composes all
          sub-shapes with the orientation of this shape.
        * If cumLoc is true, the function multiplies all
          sub-shapes by the location of this shape, i.e. it applies to
          each sub-shape the transformation that is associated with this shape.
        """
        ...

    @constmethod
    def ancestorsOfType(self, shape: TopoShape, shape_type: str, /) -> List:
        """
        For a sub-shape of this shape get its ancestors of a type.
        ancestorsOfType(shape, shape type) -> list
        """
        ...

    def removeInternalWires(self, minimalArea: float, /) -> bool:
        """
        Removes internal wires (also holes) from the shape.
        removeInternalWires(minimalArea) -> bool
        """
        ...

    @constmethod
    def mirror(self, base: Vector, norm: Vector, /) -> TopoShape:
        """
        Mirror this shape on a given plane.
        mirror(base, norm) -> Shape
        --
        The plane is given with its base point and its normal direction.
        """
        ...

    @constmethod
    def transformGeometry(self, matrix: Matrix, /) -> TopoShape:
        """
        Apply geometric transformation on this or a copy the shape.
        transformGeometry(matrix) -> Shape
        --
        This method returns a new shape.
        The transformation to be applied is defined as a 4x4 matrix.
        The underlying geometry of the following shapes may change:
        - a curve which supports an edge of the shape, or
        - a surface which supports a face of the shape;

        For example, a circle may be transformed into an ellipse when
        applying an affinity transformation. It may also happen that
        the circle then is represented as a B-spline curve.

        The transformation is applied to:
        - all the curves which support edges of the shape, and
        - all the surfaces which support faces of the shape.

        Note: If you want to transform a shape without changing the
        underlying geometry then use the methods translate or rotate.
        """
        ...

    def transformShape(
        self, matrix: Matrix, copy: bool = False, checkScale: bool = False, /
    ) -> None:
        """
        Apply transformation on a shape without changing the underlying geometry.
        transformShape(Matrix, [boolean copy=False, checkScale=False]) -> None
        --
        If checkScale is True, it will use transformGeometry if non-uniform
        scaling is detected.
        """
        ...

    @constmethod
    def transformed(
        self, matrix: Matrix, *, copy: bool = False, checkScale: bool = False, op: str = None
    ) -> TopoShape:
        """
        Create a new transformed shape
        transformed(Matrix,copy=False,checkScale=False,op=None) -> shape
        """
        ...

    def translate(self, vector: Vector, /) -> None:
        """
        Apply the translation to the current location of this shape.
        translate(vector)
        """
        ...

    @constmethod
    def translated(self, vector: Vector, /) -> TopoShape:
        """
        Create a new shape with translation
        translated(vector) -> shape
        """
        ...

    def rotate(self, base: Vector, dir: Vector, degree: float, /) -> None:
        """
        Apply the rotation (base, dir, degree) to the current location of this shape
        rotate(base, dir, degree)
        --
        Shp.rotate(App.Vector(0,0,0), App.Vector(0,0,1), 180) - rotate the shape around the Z Axis 180 degrees.
        """
        ...

    @constmethod
    def rotated(self, base: Vector, dir: Vector, degree: float, /) -> TopoShape:
        """
        Create a new shape with rotation.
        rotated(base, dir, degree) -> shape
        """
        ...

    def scale(self, factor: float, base: Vector = None, /) -> None:
        """
        Apply scaling with point and factor to this shape.
        scale(factor, [base=App.Vector(0,0,0)])
        """
        ...

    @constmethod
    def scaled(self, factor: float, base: Vector = None, /) -> TopoShape:
        """
        Create a new shape with scale.
        scaled(factor, [base=App.Vector(0,0,0)]) -> shape
        """
        ...

    @overload
    @constmethod
    def makeFillet(self, radius: float, edgeList: List, /) -> TopoShape: ...
    @overload
    @constmethod
    def makeFillet(self, radius1: float, radius2: float, edgeList: List, /) -> TopoShape: ...
    @constmethod
    def makeFillet(self, *args) -> TopoShape:
        """
        Make fillet.
        makeFillet(radius, edgeList) -> Shape
        or
        makeFillet(radius1, radius2, edgeList) -> Shape
        """
        ...

    @overload
    @constmethod
    def makeChamfer(self, radius: float, edgeList: List, /) -> TopoShape: ...
    @overload
    @constmethod
    def makeChamfer(self, radius1: float, radius2: float, edgeList: List, /) -> TopoShape: ...
    @constmethod
    def makeChamfer(self, *args) -> TopoShape:
        """
        Make chamfer.
        makeChamfer(radius, edgeList) -> Shape
        or
        makeChamfer(radius1, radius2, edgeList) -> Shape
        """
        ...

    @constmethod
    def makeThickness(self, faces: List, offset: float, tolerance: float, /) -> TopoShape:
        """
        Hollow a solid according to given thickness and faces.
        makeThickness(List of faces, Offset (Float), Tolerance (Float)) -> Shape
        --
        A hollowed solid is built from an initial solid and a set of faces on this solid,
        which are to be removed. The remaining faces of the solid become the walls of
        the hollowed solid, their thickness defined at the time of construction.
        """
        ...

    @constmethod
    def makeOffsetShape(
        self,
        offset: float,
        tolerance: float,
        *,
        inter: bool = False,
        self_inter: bool = False,
        offsetMode: int = 0,
        join: int = 0,
        fill: bool = False,
    ) -> TopoShape:
        """
        Makes an offset shape (3d offsetting).
        makeOffsetShape(offset, tolerance, [inter=False, self_inter=False, offsetMode=0, join=0, fill=False]) -> Shape
        --
        The function supports keyword arguments.

        * offset: distance to expand the shape by. Negative value will shrink the shape.

        * tolerance: precision of approximation.

        * inter: (parameter to OCC routine; not implemented)

        * self_inter: (parameter to OCC routine; not implemented)

        * offsetMode: 0 = skin; 1 = pipe; 2 = recto-verso

        * join: method of offsetting non-tangent joints. 0 = arcs, 1 = tangent, 2 =
        intersection

        * fill: if true, offsetting a shell is to yield a solid

        Returns: result of offsetting.
        """
        ...

    @constmethod
    def makeOffset2D(
        self,
        offset: float,
        *,
        join: int = 0,
        fill: bool = False,
        openResult: bool = False,
        intersection: bool = False,
    ) -> TopoShape:
        """
        Makes an offset shape (2d offsetting).
        makeOffset2D(offset, [join=0, fill=False, openResult=False, intersection=False]) -> Shape
        --
        The function supports keyword arguments.
        Input shape (self) can be edge, wire, face, or a compound of those.

        * offset: distance to expand the shape by. Negative value will shrink the shape.

        * join: method of offsetting non-tangent joints. 0 = arcs, 1 = tangent, 2 = intersection

        * fill: if true, the output is a face filling the space covered by offset. If
        false, the output is a wire.

        * openResult: affects the way open wires are processed. If False, an open wire
        is made. If True, a closed wire is made from a double-sided offset, with rounds
        around open vertices.

        * intersection: affects the way compounds are processed. If False, all children
        are offset independently. If True, and children are edges/wires, the children
        are offset in a collective manner. If compounding is nested, collectiveness
        does not spread across compounds (only direct children of a compound are taken
        collectively).

        Returns: result of offsetting (wire or face or compound of those). Compounding
        structure follows that of source shape.
        """
        ...

    @constmethod
    def makeEvolved(
        self,
        Profile: TopoShape,
        Join: int,
        AxeProf: bool,
        *,
        Solid: bool,
        ProfOnSpine: bool,
        Tolerance: float,
    ) -> None:
        """
        Profile along the spine
        """
        ...

    @constmethod
    def makeWires(self, op: str = None, /) -> TopoShape:
        """
        Make wire(s) using the edges of this shape
        makeWires([op=None])
        --
        The function will sort any edges inside the current shape, and connect them
        into wire. If more than one wire is found, then it will make a compound out of
        all found wires.

        This function is element mapping aware. If the input shape has non-zero Tag,
        it will map any edge and vertex element name inside the input shape into the
        itself.

        op: an optional string to be appended when auto generates element mapping.
        """
        ...

    def reverse(self) -> None:
        """
        Reverses the orientation of this shape.
        reverse()
        """
        ...

    @constmethod
    def reversed(self) -> TopoShape:
        """
        Reverses the orientation of a copy of this shape.
        reversed() -> Shape
        """
        ...

    def complement(self) -> None:
        """
        Computes the complement of the orientation of this shape,
        i.e. reverses the interior/exterior status of boundaries of this shape.
        complement()
        """
        ...

    def nullify(self) -> None:
        """
        Destroys the reference to the underlying shape stored in this shape.
        As a result, this shape becomes null.
        nullify()
        """
        ...

    @constmethod
    def isClosed(self) -> bool:
        """
        Checks if the shape is closed.
        isClosed() -> bool
        --
        If the shape is a shell it returns True if it has no free boundaries (edges).
        If the shape is a wire it returns True if it has no free ends (vertices).
        (Internal and External sub-shapes are ignored in these checks)
        If the shape is an edge it returns True if its vertices are the same.
        """
        ...

    @constmethod
    def isPartner(self, shape: TopoShape, /) -> bool:
        """
        Checks if both shapes share the same geometry.
        Placement and orientation may differ.
        isPartner(shape) -> bool
        """
        ...

    @constmethod
    def isSame(self, shape: TopoShape, /) -> bool:
        """
        Checks if both shapes share the same geometry
        and placement. Orientation may differ.
        isSame(shape) -> bool
        """
        ...

    @constmethod
    def isEqual(self, shape: TopoShape, /) -> bool:
        """
        Checks if both shapes are equal.
        This means geometry, placement and orientation are equal.
        isEqual(shape) -> bool
        """
        ...

    @constmethod
    def isNull(self) -> bool:
        """
        Checks if the shape is null.
        isNull() -> bool
        """
        ...

    @constmethod
    def isValid(self) -> bool:
        """
        Checks if the shape is valid, i.e. neither null, nor empty nor corrupted.
        isValid() -> bool
        """
        ...

    @constmethod
    def isCoplanar(self, shape: TopoShape, tol: float = None, /) -> bool:
        """
        Checks if this shape is coplanar with the given shape.
        isCoplanar(shape,tol=None) -> bool
        """
        ...

    @constmethod
    def isInfinite(self) -> bool:
        """
        Checks if this shape has an infinite expansion.
        isInfinite() -> bool
        """
        ...

    @constmethod
    def findPlane(self, tol: float = None, /) -> TopoShape:
        """
        Returns a plane if the shape is planar
        findPlane(tol=None) -> Shape
        """
        ...

    def fix(
        self, working_precision: float, minimum_precision: float, maximum_precision: float, /
    ) -> bool:
        """
        Tries to fix a broken shape.
        fix(working precision, minimum precision, maximum precision) -> bool
        --
        True is returned if the operation succeeded, False otherwise.
        """
        ...

    @constmethod
    def hashCode(self) -> int:
        """
        This value is computed from the value of the underlying shape reference and the location.
        hashCode() -> int
        --
        Orientation is not taken into account.
        """
        ...

    @constmethod
    def tessellate(self) -> Tuple[List[Vector], List]:
        """
        Tessellate the shape and return a list of vertices and face indices
        tessellate() -> (vertex,facets)
        """
        ...

    @constmethod
    def project(self, shapeList: List[TopoShape], /) -> TopoShape:
        """
        Project a list of shapes on this shape
        project(shapeList) -> Shape
        """
        ...

    @constmethod
    def makeParallelProjection(self, shape: TopoShape, dir: Vector, /) -> TopoShape:
        """
        Parallel projection of an edge or wire on this shape
        makeParallelProjection(shape, dir) -> Shape
        """
        ...

    @constmethod
    def makePerspectiveProjection(self, shape: TopoShape, pnt: Vector, /) -> TopoShape:
        """
        Perspective projection of an edge or wire on this shape
        makePerspectiveProjection(shape, pnt) -> Shape
        """
        ...

    @constmethod
    def reflectLines(
        self,
        ViewDir: Vector,
        *,
        ViewPos: Vector = None,
        UpDir: Vector = None,
        EdgeType: str = None,
        Visible: bool = True,
        OnShape: bool = False,
    ) -> TopoShape:
        """
        Build projection or reflect lines of a shape according to a view direction.
        reflectLines(ViewDir, [ViewPos, UpDir, EdgeType, Visible, OnShape]) -> Shape (Compound of edges)
        --
        This algorithm computes the projection of the shape in the ViewDir direction.
        If OnShape is False(default), the returned edges are flat on the XY plane defined by
        ViewPos(origin) and UpDir(up direction).
        If OnShape is True, the returned edges are the corresponding 3D reflect lines located on the shape.
        EdgeType is a string defining the type of result edges :
        - IsoLine : isoparametric line
        - OutLine : outline (silhouette) edge
        - Rg1Line : smooth edge of G1-continuity between two surfaces
        - RgNLine : sewn edge of CN-continuity on one surface
        - Sharp : sharp edge (of C0-continuity)
        If Visible is True (default), only visible edges are returned.
        If Visible is False, only invisible edges are returned.
        """
        ...

    def makeShapeFromMesh(self, mesh: Tuple[List[Vector], List], tolerance: float, /) -> TopoShape:
        """
        Make a compound shape out of mesh data.
        makeShapeFromMesh((vertex,facets),tolerance) -> Shape
        --
        Note: This should be used for rather small meshes only.
        """
        ...

    @constmethod
    def toNurbs(self) -> TopoShape:
        """
        Conversion of the complete geometry of a shape into NURBS geometry.
        toNurbs() -> Shape
        --
        For example, all curves supporting edges of the basis shape are converted
        into B-spline curves, and all surfaces supporting its faces are converted
        into B-spline surfaces.
        """
        ...

    @constmethod
    def copy(self, copyGeom: bool = True, copyMesh: bool = False, /) -> TopoShape:
        """
        Create a copy of this shape
        copy(copyGeom=True, copyMesh=False) -> Shape
        --
        If copyMesh is True, triangulation contained in original shape will be
        copied along with geometry.
        If copyGeom is False, only topological objects will be copied, while
        geometry and triangulation will be shared with original shape.
        """
        ...

    @constmethod
    def cleaned(self) -> TopoShape:
        """
        This creates a cleaned copy of the shape with the triangulation removed.
        clean()
        --
        This can be useful to reduce file size when exporting as a BREP file.
        Warning: Use the cleaned shape with care because certain algorithms may work incorrectly
        if the shape has no internal triangulation any more.
        """
        ...

    @constmethod
    def replaceShape(self, tupleList: List[Tuple[TopoShape, TopoShape]], /) -> TopoShape:
        """
        Replace a sub-shape with a new shape and return a new shape.
        replaceShape(tupleList) -> Shape
        --
        The parameter is in the form list of tuples with the two shapes.
        """
        ...

    @constmethod
    def removeShape(self, shapeList: List[TopoShape], /) -> TopoShape:
        """
        Remove a sub-shape and return a new shape.
        removeShape(shapeList) -> Shape
        --
        The parameter is a list of shapes.
        """
        ...

    @constmethod
    def defeaturing(self, shapeList: List[TopoShape], /) -> TopoShape:
        """
        Remove a feature defined by supplied faces and return a new shape.
        defeaturing(shapeList) -> Shape
        --
        The parameter is a list of faces.
        """
        ...

    @constmethod
    def isInside(self, point: Vector, tolerance: float, checkFace: bool, /) -> bool:
        """
        Checks whether a point is inside or outside the shape.
        isInside(point, tolerance, checkFace) => Boolean
        --
        checkFace indicates if the point lying directly on a face is considered to be inside or not
        """
        ...

    @constmethod
    def removeSplitter(self) -> TopoShape:
        """
        Removes redundant edges from the B-REP model
        removeSplitter() -> Shape
        """
        ...

    @constmethod
    def proximity(
        self, shape: TopoShape, tolerance: float = None, /
    ) -> Tuple[List[int], List[int]]:
        """
        Returns two lists of Face indexes for the Faces involved in the intersection.
        proximity(shape,[tolerance]) -> (selfFaces, shapeFaces)
        """
        ...

    @constmethod
    def distToShape(
        self, shape: TopoShape, tol: float = 1e-7, /
    ) -> Tuple[float, List[Tuple[Vector, Vector]], List[Tuple]]:
        """
        Find the minimum distance to another shape.
        distToShape(shape, tol=1e-7) -> (dist, vectors, infos)
        --
        dist is the minimum distance, in mm (float value).

        vectors is a list of pairs of App.Vector. Each pair corresponds to solution.
        Example: [(App.Vector(2.0, -1.0, 2.0), App.Vector(2.0, 0.0, 2.0)),
        (App.Vector(2.0, -1.0, 2.0), App.Vector(2.0, -1.0, 3.0))]
        First vector is a point on self, second vector is a point on s.

        infos contains additional info on the solutions. It is a list of tuples:
        (topo1, index1, params1, topo2, index2, params2)

            topo1, topo2 are strings identifying type of BREP element: 'Vertex',
            'Edge', or 'Face'.

            index1, index2 are indexes of the elements (zero-based).

            params1, params2 are parameters of internal space of the elements. For
            vertices, params is None. For edges, params is one float, u. For faces,
            params is a tuple (u,v).
        """
        ...

    @constmethod
    def getElement(self, elementName: str, silent: bool = False, /) -> TopoShape:
        """
        Returns a SubElement
        getElement(elementName, [silent = False]) -> Face | Edge | Vertex
        elementName:  SubElement name - i.e. 'Edge1', 'Face3' etc.
                      Accepts TNP mitigation mapped names as well
        silent:  True to suppress the exception throw if the shape isn't found.
        """
        ...

    @constmethod
    def countElement(self, type: str, /) -> int:
        """
        Returns the count of a type of element
        countElement(type) -> int
        """
        ...

    def mapSubElement(
        self, shape: Union[TopoShape, Tuple[TopoShape, ...]], op: str = "", /
    ) -> None:
        """
        mapSubElement(shape|[shape...], op='') - maps the sub element of other shape

        shape:  other shape or sequence of shapes to map the sub-elements
        op:     optional string prefix to append before the mapped sub element names
        """
        ...

    def mapShapes(
        self,
        generated: List[Tuple[TopoShape, TopoShape]],
        modified: List[Tuple[TopoShape, TopoShape]],
        op: str = "",
        /,
    ) -> None:
        """
        mapShapes(generated, modified, op='')

        generate element names with user defined mapping

        generated: a list of tuple(src, dst) that indicating src shape or shapes
        generates dst shape or shapes. Note that the dst shape or shapes
        must be sub-shapes of this shape.
        modified: a list of tuple(src, dst) that indicating src shape or shapes
        modifies into dst shape or shapes. Note that the dst shape or
        shapes must be sub-shapes of this shape.
        op: optional string prefix to append before the mapped sub element names
        """
        ...

    @constmethod
    def getElementHistory(self, name: str, /) -> Union[Tuple[str, str, List[str]], None]:
        """
        getElementHistory(name) - returns the element mapped name history

        name: mapped element name belonging to this shape

        Returns tuple(sourceShapeTag, sourceName, [intermediateNames...]),
        or None if no history.
        """
        ...

    @constmethod
    def getTolerance(self, mode: int, ShapeType: str = "Shape", /) -> float:
        """
        Determines a tolerance from the ones stored in a shape
        getTolerance(mode, ShapeType=Shape) -> float
        --
        mode = 0 : returns the average value between sub-shapes,
        mode > 0 : returns the maximal found,
        mode < 0 : returns the minimal found.
        ShapeType defines what kinds of sub-shapes to consider:
        Shape (default) : all : Vertex, Edge, Face,
        Vertex : only vertices,
        Edge   : only edges,
        Face   : only faces,
        Shell  : combined Shell + Face, for each face (and containing
                 shell), also checks edge and Vertex
        """
        ...

    @constmethod
    def overTolerance(self, value: float, ShapeType: str = "Shape", /) -> List[TopoShape]:
        """
        Determines which shapes have a tolerance over the given value
        overTolerance(value, [ShapeType=Shape]) -> ShapeList
        --
        ShapeType is interpreted as in the method getTolerance
        """
        ...

    @constmethod
    def inTolerance(
        self, valmin: float, valmax: float, ShapeType: str = "Shape", /
    ) -> List[TopoShape]:
        """
        Determines which shapes have a tolerance within a given interval
        inTolerance(valmin, valmax, [ShapeType=Shape]) -> ShapeList
        --
        ShapeType is interpreted as in the method getTolerance
        """
        ...

    @constmethod
    def globalTolerance(self, mode: int, /) -> float:
        """
        Returns the computed tolerance according to the mode
        globalTolerance(mode) -> float
        --
        mode = 0 : average
        mode > 0 : maximal
        mode < 0 : minimal
        """
        ...

    @constmethod
    def fixTolerance(self, value: float, ShapeType: str = "Shape", /) -> None:
        """
        Sets (enforces) tolerances in a shape to the given value
        fixTolerance(value, [ShapeType=Shape])
        --
        ShapeType = Vertex : only vertices are set
        ShapeType = Edge   : only edges are set
        ShapeType = Face   : only faces are set
        ShapeType = Wire   : to have edges and their vertices set
        ShapeType = other value : all (vertices,edges,faces) are set
        """
        ...

    @constmethod
    def limitTolerance(self, tmin: float, tmax: float = 0, ShapeType: str = "Shape", /) -> bool:
        """
        Limits tolerances in a shape
        limitTolerance(tmin, [tmax=0, ShapeType=Shape]) -> bool
        --
        tmin = tmax -> as fixTolerance (forces)
        tmin = 0   -> maximum tolerance will be tmax
        tmax = 0 or not given (more generally, tmax < tmin) ->
        tmax ignored, minimum will be tmin
        else, maximum will be max and minimum will be min
        ShapeType = Vertex : only vertices are set
        ShapeType = Edge   : only edges are set
        ShapeType = Face   : only faces are set
        ShapeType = Wire   : to have edges and their vertices set
        ShapeType = other value : all (vertices,edges,faces) are set
        Returns True if at least one tolerance of the sub-shape has been modified
        """
        ...

    @constmethod
    def optimalBoundingBox(
        self, useTriangulation: bool = True, useShapeTolerance: bool = False, /
    ) -> BoundBox:
        """
        Get the optimal bounding box
        optimalBoundingBox([useTriangulation = True, useShapeTolerance = False]) -> bound box
        """
        ...

    def clearCache(self) -> None:
        """
        Clear internal sub-shape cache
        """
        ...

    @constmethod
    def findSubShape(self, shape: TopoShape, /) -> Tuple[Union[str, None], int]:
        """
        findSubShape(shape) -> (type_name, index)

        Find sub shape and return the shape type name and index. If not found,
        then return (None, 0)
        """
        ...

    @constmethod
    def findSubShapesWithSharedVertex(
        self,
        shape: TopoShape,
        *,
        needName: bool = False,
        checkGeometry: bool = True,
        tol: float = 1e-7,
        atol: float = 1e-12,
    ) -> Union[List[Tuple[str, TopoShape]], List[TopoShape]]:
        """
        findSubShapesWithSharedVertex(shape, needName=False, checkGeometry=True, tol=1e-7, atol=1e-12) -> Shape

        shape: input elementary shape, currently only support Face, Edge, or Vertex

        needName: if True, return a list of tuple(name, shape), or else return a list
        of shapes.

        checkGeometry: whether to compare geometry

        tol: distance tolerance

        atol: angular tolerance

        Search sub shape by checking vertex coordinates and comparing the underlying
        geometries, This can find shapes that are copied. It currently only works with
        elementary shapes, Face, Edge, Vertex.
        """
        ...

    @constmethod
    def getChildShapes(self, shapetype: str, avoidtype: str = "", /) -> List[TopoShape]:
        """
        getChildShapes(shapetype, avoidtype='') -> list(Shape)

        Return a list of child sub-shapes of given type.

        shapetype: the type of requesting sub shapes
        avoidtype: optional shape type to skip when exploring
        """
        ...
