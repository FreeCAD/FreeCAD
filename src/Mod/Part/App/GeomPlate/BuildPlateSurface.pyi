from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from typing import List

@export(
    PythonName="Part.GeomPlate.BuildPlateSurfacePy",
    Twin="GeomPlate_BuildPlateSurface",
    TwinPointer="GeomPlate_BuildPlateSurface",
    Include="GeomPlate_BuildPlateSurface.hxx",
    Constructor=True,
    Delete=True,
)
class BuildPlateSurface(PyObjectBase):
    """
    This class provides an algorithm for constructing such a plate surface.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
        """
        Resets all constraints
        """
        ...

    def setNbBounds(self) -> None:
        """
        Sets the number of bounds
        """
        ...

    def loadInitSurface(self) -> None:
        """
        Loads the initial surface
        """
        ...

    @constmethod
    def surfInit(self) -> object:
        """
        Returns the initial surface
        """
        ...

    @constmethod
    def surface(self) -> object:
        """
        Returns the plate surface
        """
        ...

    def add(self) -> None:
        """
        Adds a linear or point constraint
        """
        ...

    def perform(self) -> None:
        """
        Calls the algorithm and computes the plate surface
        """
        ...

    @constmethod
    def isDone(self) -> bool:
        """
        Tests whether computation of the plate has been completed
        """
        ...

    @constmethod
    def sense(self) -> object:
        """
        Returns the orientation of the curves in the array returned by curves2d
        """
        ...

    @constmethod
    def order(self) -> int:
        """
        Returns the order of the curves in the array returned by curves2d
        """
        ...

    @constmethod
    def curves2d(self) -> List[object]:
        """
        Extracts the array of curves on the plate surface which
        correspond to the curve constraints set in add()
        """
        ...

    @constmethod
    def curveConstraint(self) -> object:
        """
        Returns the curve constraint of order
        """
        ...

    @constmethod
    def pointConstraint(self) -> object:
        """
        Returns the point constraint of order
        """
        ...

    def disc2dContour(self) -> object:
        """
        Returns the 2D contour of the plate surface
        """
        ...

    def disc3dContour(self) -> object:
        """
        Returns the 3D contour of the plate surface
        """
        ...

    @constmethod
    def G0Error(self) -> float:
        """
        Returns the max distance between the result and the constraints
        """
        ...

    @constmethod
    def G1Error(self) -> float:
        """
        Returns the max angle between the result and the constraints
        """
        ...

    @constmethod
    def G2Error(self) -> float:
        """
        Returns the max difference of curvature between the result and the constraints
        """
        ...
