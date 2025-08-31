from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from Part.App.Point import Point
from Part.App.TopoShape import TopoShape
from Part.App.TopoShapeEdge import TopoShapeEdge
from Part.App.TopoShapeFace import TopoShapeFace
from typing import overload, Final


@export(
    PythonName="Part.BRepOffsetAPI_MakeFilling",
    Include="BRepOffsetAPI_MakeFilling.hxx",
    Constructor=True,
    Delete=True,
)
class BRepOffsetAPI_MakeFilling(PyObjectBase):
    """
    N-Side Filling

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    def setConstrParam(self, *, Tol2d: float = 0.00001, Tol3d: float = 0.0001, TolAng: float = 0.01, TolCurv: float = 0.1) -> None:
        """
        setConstrParam(Tol2d=0.00001, Tol3d=0.0001, TolAng=0.01, TolCurv=0.1)
        Sets the values of Tolerances used to control the constraint.
        """
        ...

    def setResolParam(self, *, Degree: int = 3, NbPtsOnCur: int = 15, NbIter: int = 2, Anisotropy: bool = False) -> None:
        """
        setResolParam(Degree=3, NbPtsOnCur=15, NbIter=2, Anisotropy=False)
        Sets the parameters used for resolution.
        """
        ...

    def setApproxParam(self, *, MaxDeg: int = 8, MaxSegments: int = 9) -> None:
        """
        setApproxParam(MaxDeg=8, MaxSegments=9)
        Sets the parameters used to approximate the filling the surface
        """
        ...

    def loadInitSurface(self, face: TopoShapeFace) -> None:
        """
        loadInitSurface(face)
        Loads the initial surface.
        """
        ...

    @overload
    def add(self, Edge: TopoShapeEdge, Order: int, *, IsBound: bool = True) -> None:
        ...

    @overload
    def add(self, Edge: TopoShapeEdge, Support: TopoShapeFace, Order: int, *, IsBound: bool = True) -> None:
        ...

    @overload
    def add(self, Support: TopoShapeFace, Order: int) -> None:
        ...

    @overload
    def add(self, Point: Point) -> None:
        ...

    @overload
    def add(self, U: float, V: float, Support: TopoShapeFace, Order: int) -> None:
        ...

    def add(self, **kwargs) -> None:
        """
        add(Edge, Order, IsBound=True)
        add(Edge, Support, Order, IsBound=True)
        add(Support, Order)
        add(Point)
        add(U, V, Support, Order)
        Adds a new constraint.
        """
        ...

    def build(self) -> None:
        """
        Builds the resulting faces.
        """
        ...

    def isDone(self) -> bool:
        """
        Tests whether computation of the filling plate has been completed.
        """
        ...

    @overload
    def G0Error(self) -> float:
        ...

    @overload
    def G0Error(self, arg: int) -> float:
        ...

    def G0Error(self, arg: int = 0) -> float:
        """
        G0Error([int])
        Returns the maximum distance between the result and the constraints.
        """
        ...

    @overload
    def G1Error(self) -> float:
        ...

    @overload
    def G1Error(self, arg: int) -> float:
        ...

    def G1Error(self, arg: int = 0) -> float:
        """
        G1Error([int])
        Returns the maximum angle between the result and the constraints.
        """
        ...

    @overload
    def G2Error(self) -> float:
        ...

    @overload
    def G2Error(self, arg: int) -> float:
        ...

    def G2Error(self, arg: int = 0) -> float:
        """
        G2Error([int])
        Returns the greatest difference in curvature between the result and the constraints.
        """
        ...

    def shape(self) -> TopoShape:
        """
        shape()
        Returns the resulting shape.
        """
        ...
