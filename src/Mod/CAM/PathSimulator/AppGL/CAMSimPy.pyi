from typing import Any

from Base.BaseClass import BaseClass
from Base.Metadata import export
from Metadata import no_args

@export(
    Father="BaseClassPy",
    Name="CAMSimPy",
    Twin="CAMSim",
    TwinPointer="CAMSim",
    Include="Mod/CAM/PathSimulator/AppGL/CAMSim.h",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Namespace="CAMSimulator",
    Constructor=True,
    Delete=True,
)
class CAMSimPy(BaseClass):
    """
    FreeCAD python wrapper of CAMSimulator

          CAMSimulator.CAMSim():

          Create a path simulator object
    """

    def BeginSimulation(self, **kwargs) -> Any:
        """
        BeginSimulation(stock, resolution):

                  Start a simulation process on a box shape stock with given resolution"""
        ...

    @no_args
    def ResetSimulation(self) -> Any:
        """
        ResetSimulation():

                  Clear the simulation and all gcode commands"""
        ...

    def AddTool(self, **kwargs) -> Any:
        """
        AddTool(shape, toolnumber, diameter, resolution):

                  Set the shape of the tool to be used for simulation"""
        ...

    def SetBaseShape(self, **kwargs) -> Any:
        """
        SetBaseShape(shape, resolution):

                  Set the shape of the base object of the job"""
        ...

    def AddCommand(self) -> Any:
        """
        AddCommand(command):

                  Add a path command to the simulation."""
        ...
