from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Wire import Wire
from Vertex import Vertex
from TopoShape import TopoShape
from typing import Final, Tuple, Dict, List, overload


@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Constructor=True,
)
class TopoShapeEdge(TopoShape):
    """
    TopoShapeEdge is the OpenCasCade topological edge wrapper

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    """

    Tolerance: float = 0.0
    """Set or get the tolerance of the vertex"""

    Length: Final[float] = 0.0
    """Returns the cartesian length of the curve"""

    ParameterRange: Final[Tuple[float, float]] = (0.0, 0.0)
    """
    Returns a 2 tuple with the range of the primary parameter
    defining the curve. This is the same as would be returned by
    the FirstParameter and LastParameter properties, i.e.
    
    (LastParameter,FirstParameter)
    
    What the parameter is depends on what type of edge it is. For a
    Line the parameter is simply its cartesian length. Some other
    examples are shown below:
    
    Type                 Parameter
    ---------------------------------------------------------------
    Circle               Angle swept by circle (or arc) in radians
    BezierCurve          Unitless number in the range 0.0 to 1.0
    Helix                Angle swept by helical turns in radians
    """

    FirstParameter: Final[float] = 0.0
    """
    Returns the start value of the range of the primary parameter
    defining the curve.
    
    What the parameter is depends on what type of edge it is. For a
    Line the parameter is simply its cartesian length. Some other
    examples are shown below:
    
    Type                 Parameter
    -----------------------------------------------------------
    Circle               Angle swept by circle (or arc) in radians
    BezierCurve          Unitless number in the range 0.0 to 1.0
    Helix                Angle swept by helical turns in radians
    """

    LastParameter: Final[float] = 0.0
    """
    Returns the end value of the range of the primary parameter
    defining the curve.
    
    What the parameter is depends on what type of edge it is. For a
    Line the parameter is simply its cartesian length. Some other
    examples are shown below:
    
    Type                 Parameter
    -----------------------------------------------------------
    Circle               Angle swept by circle (or arc) in radians
    BezierCurve          Unitless number in the range 0.0 to 1.0
    Helix                Angle swept by helical turns in radians
    """

    Curve: Final[object] = object()
    """Returns the 3D curve of the edge"""

    Closed: Final[bool] = False
    """Returns true if the edge is closed"""

    Degenerated: Final[bool] = False
    """Returns true if the edge is degenerated"""

    Mass: Final[object] = object()
    """Returns the mass of the current system."""

    CenterOfMass: Final[object] = object()
    """
    Returns the center of mass of the current system.
    If the gravitational field is uniform, it is the center of gravity.
    The coordinates returned for the center of mass are expressed in the
    absolute Cartesian coordinate system.
    """

    MatrixOfInertia: Final[object] = object()
    """
    Returns the matrix of inertia. It is a symmetrical matrix.
    The coefficients of the matrix are the quadratic moments of
    inertia.

     | Ixx Ixy Ixz 0 |
     | Ixy Iyy Iyz 0 |
     | Ixz Iyz Izz 0 |
     | 0   0   0   1 |

    The moments of inertia are denoted by Ixx, Iyy, Izz.
    The products of inertia are denoted by Ixy, Ixz, Iyz.
    The matrix of inertia is returned in the central coordinate
    system (G, Gx, Gy, Gz) where G is the centre of mass of the
    system and Gx, Gy, Gz the directions parallel to the X(1,0,0)
    Y(0,1,0) Z(0,0,1) directions of the absolute cartesian
    coordinate system.
    """

    StaticMoments: Final[object] = object()
    """
    Returns Ix, Iy, Iz, the static moments of inertia of the
    current system; i.e. the moments of inertia about the
    three axes of the Cartesian coordinate system.
    """

    PrincipalProperties: Final[Dict] = {}
    """
    Computes the principal properties of inertia of the current system.
    There is always a set of axes for which the products
    of inertia of a geometric system are equal to 0; i.e. the
    matrix of inertia of the system is diagonal. These axes
    are the principal axes of inertia. Their origin is
    coincident with the center of mass of the system. The
    associated moments are called the principal moments of inertia.
    This function computes the eigen values and the
    eigen vectors of the matrix of inertia of the system.
    """

    Continuity: Final[str] = ""
    """Returns the continuity"""

    @constmethod
    def getParameterByLength(self, pos: float, tolerance: float = 1e-7) -> float:
        """
        Get the value of the primary parameter at the given distance along the cartesian length of the edge.
        getParameterByLength(pos, [tolerance = 1e-7]) -> Float
        --
        Args:
            pos (float or int): The distance along the length of the edge at which to
                determine the primary parameter value. See help for the FirstParameter or
                LastParameter properties for more information on the primary parameter.
                If the given value is positive, the distance from edge start is used.
                If the given value is negative, the distance from edge end is used.
            tol (float): Computing tolerance. Optional, defaults to 1e-7.

        Returns:
            paramval (float): the value of the primary parameter defining the edge at the
                given position along its cartesian length.
        """
        ...

    @constmethod
    def tangentAt(self, paramval: float) -> Vector:
        """
        Get the tangent direction at the given primary parameter value along the Edge if it is defined
        tangentAt(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the tangent direction e.g:

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.tangentAt(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is the Vector (-0.7071067811865475, 0.7071067811865476, 0.0)

                Values with magnitude greater than the Edge length return
                values of the tangent on the curve extrapolated beyond its
                length. This may not be valid for all Edges. Negative values
                similarly return a tangent on the curve extrapolated backwards
                (before the start point of the Edge). For example, using the
                same shape as above:

                >>> x.tangentAt(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865477, 0.7071067811865474, 0.0)

                Which gives the same result as

                >>> x.tangentAt(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865475, 0.7071067811865476, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the tangent to the Edge at the given
               location along its length (or extrapolated length)
        """
        ...

    @constmethod
    def valueAt(self, paramval: float) -> Vector:
        """
        Get the value of the cartesian parameter value at the given parameter value along the Edge
        valueAt(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the value in terms of the main parameter defining
                the edge, what the parameter value is depends on the type of
                edge. See  e.g:

                For a circle value

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.valueAt(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is theVector (0.7071067811865476, 0.7071067811865475, 0.0)

                Values with magnitude greater than the Edge length return
                values on the curve extrapolated beyond its length. This may
                not be valid for all Edges. Negative values similarly return
                a parameter value on the curve extrapolated backwards (before the
                start point of the Edge). For example, using the same shape
                as above:

                >>> x.valueAt(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865474, -0.7071067811865477, 0.0)

                Which gives the same result as

                >>> x.valueAt(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865476, -0.7071067811865475, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the cartesian location on the Edge at the given
               distance along its length (or extrapolated length)
        """
        ...

    @constmethod
    def parameters(self, face: object = ...) -> List[float]:
        """
        Get the list of parameters of the tessellation of an edge.
        parameters([face]) -> list
        --
        If the edge is part of a face then this face is required as argument.
        An exception is raised if the edge has no polygon.
        """
        ...

    @constmethod
    def parameterAt(self, vertex: object) -> float:
        """
        Get the parameter at the given vertex if lying on the edge
        parameterAt(Vertex) -> Float
        """
        ...

    @constmethod
    def normalAt(self, paramval: float) -> Vector:
        """
        Get the normal direction at the given parameter value along the Edge if it is defined
        normalAt(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the normal direction e.g:

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.normalAt(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is the Vector (-0.7071067811865476, -0.7071067811865475, 0.0)

                Values with magnitude greater than the Edge length return
                values of the normal on the curve extrapolated beyond its
                length. This may not be valid for all Edges. Negative values
                similarly return a normal on the curve extrapolated backwards
                (before the start point of the Edge). For example, using the
                same shape as above:

                >>> x.normalAt(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865474, 0.7071067811865477, 0.0)

                Which gives the same result as

                >>> x.normalAt(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865476, 0.7071067811865475, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the normal to the Edge at the given
               location along its length (or extrapolated length)
        """
        ...

    @constmethod
    def derivative1At(self, paramval: float) -> Vector:
        """
        Get the first derivative at the given parameter value along the Edge if it is defined
        derivative1At(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the first derivative e.g:

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.derivative1At(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is the Vector (-0.7071067811865475, 0.7071067811865476, 0.0)

                Values with magnitude greater than the Edge length return
                values of the first derivative on the curve extrapolated
                beyond its length. This may not be valid for all Edges.
                Negative values similarly return a first derivative on the
                curve extrapolated backwards (before the start point of the
                Edge). For example, using the same shape as above:

                >>> x.derivative1At(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865477, 0.7071067811865474, 0.0)

                Which gives the same result as

                >>> x.derivative1At(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (0.7071067811865475, 0.7071067811865476, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the first derivative to the Edge at the
               given location along its length (or extrapolated length)
        """
        ...

    @constmethod
    def derivative2At(self, paramval: float) -> Vector:
        """
        Get the second derivative at the given parameter value along the Edge if it is defined
        derivative2At(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the second derivative e.g:

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.derivative2At(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is the Vector (-0.7071067811865476, -0.7071067811865475, 0.0)

                Values with magnitude greater than the Edge length return
                values of the second derivative on the curve extrapolated
                beyond its length. This may not be valid for all Edges.
                Negative values similarly return a second derivative on the
                curve extrapolated backwards (before the start point of the
                Edge). For example, using the same shape as above:

                >>> x.derivative2At(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865474, 0.7071067811865477, 0.0)

                Which gives the same result as

                >>> x.derivative2At(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865476, 0.7071067811865475, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the second derivative to the Edge at the
               given location along its length (or extrapolated length)
        """
        ...

    @constmethod
    def derivative3At(self, paramval: float) -> Vector:
        """
        Get the third derivative at the given parameter value along the Edge if it is defined
        derivative3At(paramval) -> Vector
        --
        Args:
            paramval (float or int): The parameter value along the Edge at which to
                determine the third derivative e.g:

                x = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                y = x.derivative3At(x.FirstParameter + 0.5 * (x.LastParameter - x.FirstParameter))

                y is the Vector (0.7071067811865475, -0.7071067811865476, -0.0)

                Values with magnitude greater than the Edge length return
                values of the third derivative on the curve extrapolated
                beyond its length. This may not be valid for all Edges.
                Negative values similarly return a third derivative on the
                curve extrapolated backwards (before the start point of the
                Edge). For example, using the same shape as above:

                >>> x.derivative3At(x.FirstParameter + 3.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865477, -0.7071067811865474, 0.0)

                Which gives the same result as

                >>> x.derivative3At(x.FirstParameter -0.5*(x.LastParameter - x.FirstParameter))
                Vector (-0.7071067811865475, -0.7071067811865476, 0.0)

                Since it is a circle

        Returns:
            Vector: representing the third derivative to the Edge at the
               given location along its length (or extrapolated length)
        """
        ...

    @constmethod
    def curvatureAt(self, paramval: float) -> float:
        """
        Get the curvature at the given parameter [First|Last] if defined
        curvatureAt(paramval) -> Float
        """
        ...

    @constmethod
    def centerOfCurvatureAt(self, paramval: float) -> Vector:
        """
        Get the center of curvature at the given parameter [First|Last] if defined
        centerOfCurvatureAt(paramval) -> Vector
        """
        ...

    @constmethod
    def firstVertex(self, Orientation: bool = False) -> Vertex:
        """
        Returns the Vertex of orientation FORWARD in this edge.
        firstVertex([Orientation=False]) -> Vertex
        --
        If there is none a Null shape is returned.
        Orientation = True : taking into account the edge orientation
        """
        ...

    @constmethod
    def lastVertex(self, Orientation: bool = False) -> Vertex:
        """
        Returns the Vertex of orientation REVERSED in this edge.
        lastVertex([Orientation=False]) -> Vertex
        --
        If there is none a Null shape is returned.
        Orientation = True : taking into account the edge orientation
        """
        ...

    @constmethod
    @overload
    def discretize(
        self, Number: int, First: float = ..., Last: float = ...
    ) -> List[Vector]: ...

    @constmethod
    @overload
    def discretize(
        self, QuasiNumber: int, First: float = ..., Last: float = ...
    ) -> List[Vector]: ...

    @constmethod
    @overload
    def discretize(
        self, Distance: float, First: float = ..., Last: float = ...
    ) -> List[Vector]: ...

    @constmethod
    @overload
    def discretize(
        self, Deflection: float, First: float = ..., Last: float = ...
    ) -> List[Vector]: ...

    @constmethod
    @overload
    def discretize(
        self, QuasiDeflection: float, First: float = ..., Last: float = ...
    ) -> List[Vector]: ...

    @constmethod
    @overload
    def discretize(
        self,
        Angular: float,
        Curvature: float,
        Minimum: int = ...,
        First: float = ...,
        Last: float = ...,
    ) -> List[Vector]: ...

    @constmethod
    def discretize(self, **kwargs) -> List[Vector]:
        """
        Discretizes the edge and returns a list of points.
        discretize(kwargs) -> list
        --
        The function accepts keywords as argument:
        discretize(Number=n) => gives a list of 'n' equidistant points
        discretize(QuasiNumber=n) => gives a list of 'n' quasi equidistant points (is faster than the method above)
        discretize(Distance=d) => gives a list of equidistant points with distance 'd'
        discretize(Deflection=d) => gives a list of points with a maximum deflection 'd' to the edge
        discretize(QuasiDeflection=d) => gives a list of points with a maximum deflection 'd' to the edge (faster)
        discretize(Angular=a,Curvature=c,[Minimum=m]) => gives a list of points with an angular deflection of 'a'
                                            and a curvature deflection of 'c'. Optionally a minimum number of points
                                            can be set which by default is set to 2.

        Optionally you can set the keywords 'First' and 'Last' to define a sub-range of the parameter range
        of the edge.

        If no keyword is given then it depends on whether the argument is an int or float.
        If it's an int then the behaviour is as if using the keyword 'Number', if it's float
        then the behaviour is as if using the keyword 'Distance'.

        Example:

        import Part
        e=Part.makeCircle(5)
        p=e.discretize(Number=50,First=3.14)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)

        p=e.discretize(Angular=0.09,Curvature=0.01,Last=3.14,Minimum=100)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)
        """
        ...

    @constmethod
    def countNodes(self) -> int:
        """
        Returns the number of nodes of the 3D polygon of the edge.
        """
        ...

    @constmethod
    def split(self, paramval: float) -> Wire:
        """
        Splits the edge at the given parameter values and builds a wire out of it
        split(paramval) -> Wire
        --
        Args:
            paramval (float or list_of_floats): The parameter values along the Edge at which to
                split it e.g:

                edge = Part.makeCircle(1, FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 0, 90)
                wire = edge.split([0.5, 1.0])

        Returns:
            Wire: wire made up of two Edges
        """
        ...

    @constmethod
    def isSeam(self, Face: object) -> bool:
        """
        Checks whether the edge is a seam edge.
        isSeam(Face)
        """
        ...

    @constmethod
    def curveOnSurface(self, idx: int) -> Tuple[object, object, object, float, float]:
        """
        Returns the 2D curve, the surface, the placement and the parameter range of index idx.
        curveOnSurface(idx) -> None or tuple
        --
        Returns None if index idx is out of range.
        Returns a 5-items tuple of a curve, a surface, a placement, first parameter and last parameter.
        """
        ...
