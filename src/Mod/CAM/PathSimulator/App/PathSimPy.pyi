from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import export

@export(
    Father="BaseClassPy",
    Name="PathSimPy",
    Twin="PathSim",
    TwinPointer="PathSim",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Include="Mod/CAM/PathSimulator/App/PathSim.h",
    Namespace="PathSimulator",
    ReadOnly=["Tool"],
    Constructor=True,
    Delete=True,
)
class PathSimPy(BaseClass):
    """
    FreeCAD python wrapper of PathSimulator

    PathSimulator.PathSim():

    Create a path simulator object
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
