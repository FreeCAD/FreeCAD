# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import export
from Base.Placement import Placement
from Part.App.TopoShape import TopoShape
from Mesh.App.Mesh import Mesh
from CAM.App.Command import Command

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

    def BeginSimulation(self, stock: TopoShape, resolution: float) -> None:
        """
        Start a simulation process on a box shape stock with given resolution
        """
        ...

    def SetToolShape(self, tool: TopoShape, resolution: float, /) -> None:
        """
        Set the shape of the tool to be used for simulation
        """
        ...

    def GetResultMesh(self) -> tuple[Mesh, Mesh]:
        """
        Return the current mesh result of the simulation.
        """
        ...

    def ApplyCommand(self, placement: Placement, command: Command) -> Placement:
        """
        Apply a single path command on the stock starting from placement.
        """
        ...
    Tool: Final[Any]
    """Return current simulation tool."""
