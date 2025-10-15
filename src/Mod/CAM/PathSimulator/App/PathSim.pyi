from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import export

@export(
    FatherInclude="Base/BaseClassPy.h",
    Include="Mod/CAM/PathSimulator/App/PathSim.h",
    Namespace="PathSimulator",
    Constructor=True,
    Delete=True,
)
class PathSim(BaseClass):
    """
    FreeCAD python wrapper of PathSimulator

    PathSimulator.PathSim():

    Create a path simulator object

    Author: Shai Seger (shaise_at_g-mail)
    License: LGPL-2.1-or-later
    """

    def BeginSimulation(self, **kwargs) -> Any:
        """BeginSimulation(stock, resolution):

        Start a simulation process on a box shape stock with given resolution"""
        ...

    def SetToolShape(self) -> Any:
        """SetToolShape(shape):

        Set the shape of the tool to be used for simulation"""
        ...

    def GetResultMesh(self) -> Any:
        """
        GetResultMesh():

                  Return the current mesh result of the simulation."""
        ...

    def ApplyCommand(self, **kwargs) -> Any:
        """
        ApplyCommand(placement, command):

                  Apply a single path command on the stock starting from placement."""
        ...
    Tool: Final[Any]
    """Return current simulation tool."""
