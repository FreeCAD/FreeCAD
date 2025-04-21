from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from Part.App.TopoShape import TopoShape
from typing import Final, overload

@export(
    PythonName="Part.ShapeFix.ShapeTolerance",
    Include="ShapeFix_ShapeTolerance.hxx",
    Constructor=True,
    Delete=True,
)
class ShapeFix_ShapeTolerance(PyObjectBase):
    """
    Modifies tolerances of sub-shapes (vertices, edges, faces)

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def limitTolerance(self, shape: TopoShape, tmin: float) -> None: ...
    
    @overload
    def limitTolerance(self, shape: TopoShape, tmin: float, tmax: float, ShapeEnum: str = None) -> None: ...
    
    def limitTolerance(self, shape: TopoShape, tmin: float, tmax: float = 0, ShapeEnum: str = None) -> None:
        """
        limitTolerance(shape, tmin, [tmax=0, ShapeEnum=SHAPE])
        """
        ...

    @overload
    def setTolerance(self, shape: TopoShape, precision: float) -> None: ...
    
    @overload
    def setTolerance(self, shape: TopoShape, precision: float, ShapeEnum: str = None) -> None: ...
    
    def setTolerance(self, shape: TopoShape, precision: float, ShapeEnum: str = None) -> None:
        """
        setTolerance(shape, precision, [ShapeEnum=SHAPE])
        """
        ...
