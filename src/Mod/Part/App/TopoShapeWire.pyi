from Base.Metadata import export, constmethod
from TopoShape import TopoShape
from typing import Dict, List, Final, overload, Optional


@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Include="Mod/Part/App/TopoShape.h",
    Constructor=True,
)
class TopoShapeWire(TopoShape):
    """
    TopoShapeWire is the OpenCasCade topological wire wrapper

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    DeveloperDocu: TopoShapeWire is the OpenCasCade topological wire wrapper
    """

    Mass: Final[object] = ...
    """Returns the mass of the current system."""

    CenterOfMass: Final[object] = ...
    """
    Returns the center of mass of the current system.
    If the gravitational field is uniform, it is the center of gravity.
    The coordinates returned for the center of mass are expressed in the
    absolute Cartesian coordinate system.
    """

    MatrixOfInertia: Final[object] = ...
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

    StaticMoments: Final[object] = ...
    """
    Returns Ix, Iy, Iz, the static moments of inertia of the
    current system; i.e. the moments of inertia about the
    three axes of the Cartesian coordinate system.
    """

    PrincipalProperties: Final[Dict] = ...
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

    OrderedEdges: Final[List] = ...
    """List of ordered edges in this shape."""

    Continuity: Final[str] = ...
    """Returns the continuity"""

    OrderedVertexes: Final[List] = ...
    """List of ordered vertexes in this shape."""

    @constmethod
    def makeOffset(self) -> object:
        """
        Offset the shape by a given amount. DEPRECATED - use makeOffset2D instead.
        """
        ...

    def add(self, edge: object) -> None:
        """
        Add an edge to the wire
        add(edge)
        """
        ...

    def fixWire(
        self, face: Optional[object] = None, tolerance: Optional[float] = None
    ) -> None:
        """
        Fix wire
        fixWire([face, tolerance])
        --
        A face and a tolerance can optionally be supplied to the algorithm:
        """
        ...

    @constmethod
    def makeHomogenousWires(self, wire: object) -> object:
        """
        Make this and the given wire homogeneous to have the same number of edges
        makeHomogenousWires(wire) -> Wire
        """
        ...

    @constmethod
    def makePipe(self, profile: object) -> object:
        """
        Make a pipe by sweeping along a wire.
        makePipe(profile) -> Shape
        """
        ...

    @constmethod
    def makePipeShell(
        self,
        shapeList: List[object],
        isSolid: bool = False,
        isFrenet: bool = False,
        transition: int = 0,
    ) -> object:
        """
        Make a loft defined by a list of profiles along a wire.
        makePipeShell(shapeList,[isSolid=False,isFrenet=False,transition=0]) -> Shape
        --
        Transition can be 0 (default), 1 (right corners) or 2 (rounded corners).
        """
        ...

    @constmethod
    def makeEvolved(self, *, Profile: TopoShape, Join: int, AxeProf: bool, Solid: bool,
                    ProfOnSpine: bool, Tolerance: float) -> TopoShape:
        """
        Profile along the spine
        """
        ...

    @constmethod
    def approximate(
        self,
        *,
        Tol2d: float = None,
        Tol3d: float = 0.0001,
        MaxSegments: int = 10,
        MaxDegree: int = 3,
    ) -> object:
        """
        Approximate B-Spline-curve from this wire
        approximate([Tol2d,Tol3d=1e-4,MaxSegments=10,MaxDegree=3]) -> BSpline
        """
        ...

    @overload
    @constmethod
    def discretize(self, Number: int) -> List[object]:
        """
        discretize(Number=n) -> list
        """
        ...

    @overload
    @constmethod
    def discretize(self, QuasiNumber: int) -> List[object]:
        """
        discretize(QuasiNumber=n) -> list
        """
        ...

    @overload
    @constmethod
    def discretize(self, Distance: float) -> List[object]:
        """
        discretize(Distance=d) -> list
        """
        ...

    @overload
    @constmethod
    def discretize(self, Deflection: float) -> List[object]:
        """
        discretize(Deflection=d) -> list
        """
        ...

    @overload
    @constmethod
    def discretize(self, QuasiDeflection: float) -> List[object]:
        """
        discretize(QuasiDeflection=d) -> list
        """
        ...

    @overload
    @constmethod
    def discretize(
        self, Angular: float, Curvature: float, Minimum: int = 2
    ) -> List[object]:
        """
        discretize(Angular=a,Curvature=c,[Minimum=m]) -> list
        """
        ...

    @constmethod
    def discretize(
        self,
        **kwargs
    ) -> List[object]:
        """
        Discretizes the wire and returns a list of points.
        discretize(kwargs) -> list
        --
        The function accepts keywords as argument:
        discretize(Number=n) => gives a list of 'n' equidistant points
        discretize(QuasiNumber=n) => gives a list of 'n' quasi equidistant points (is faster than the method above)
        discretize(Distance=d) => gives a list of equidistant points with distance 'd'
        discretize(Deflection=d) => gives a list of points with a maximum deflection 'd' to the wire
        discretize(QuasiDeflection=d) => gives a list of points with a maximum deflection 'd' to the wire (faster)
        discretize(Angular=a,Curvature=c,[Minimum=m]) => gives a list of points with an angular deflection of 'a'
                                            and a curvature deflection of 'c'. Optionally a minimum number of points
                                            can be set which by default is set to 2.

        Optionally you can set the keywords 'First' and 'Last' to define a sub-range of the parameter range
        of the wire.

        If no keyword is given then it depends on whether the argument is an int or float.
        If it's an int then the behaviour is as if using the keyword 'Number', if it's float
        then the behaviour is as if using the keyword 'Distance'.

        Example:

        import Part
        V=App.Vector

        e1=Part.makeCircle(5,V(0,0,0),V(0,0,1),0,180)
        e2=Part.makeCircle(5,V(10,0,0),V(0,0,1),180,360)
        w=Part.Wire([e1,e2])

        p=w.discretize(Number=50)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)


        p=w.discretize(Angular=0.09,Curvature=0.01,Minimum=100)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)
        """
        ...
