# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import constmethod, export

from App.Part import Part
from App.DocumentObject import DocumentObject

@export(Include="Mod/Assembly/App/AssemblyObject.h", Namespace="Assembly")
class AssemblyObject(Part):
    """
    This class handles document objects in Assembly

    Author: Ondsel (development@ondsel.com)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def solve(self, enableUndo: bool = False, /) -> int:
        """
        Solve the assembly and update part placements.

        Args:
        enableRedo: Whether the solve save the initial position of parts
        to enable undoing it even without a transaction.
        Defaults to `False` ie the solve cannot be undone if called
        outside of a transaction.

        Returns:
        0 in case of success, otherwise the following codes in this order of
        priority:
        -6 if no parts are fixed.
        -4 if over-constrained,
        -3 if conflicting constraints,
        -5 if malformed constraints
        -1 if solver error,
        -2 if redundant constraints.
        """
        ...

    @constmethod
    def generateSimulation(self, simulationObject: DocumentObject, /) -> int:
        """
        Generate the simulation.

        Args:
        simulationObject: The simulation Object.

        Returns:
        0 in case of success, otherwise the following codes in this order of
        priority:
        -6 if no parts are fixed.
        -4 if over-constrained,
        -3 if conflicting constraints,
        -5 if malformed constraints
        -1 if solver error,
        -2 if redundant constraints.
        """
        ...

    @constmethod
    def updateForFrame(self, index: int, /) -> None:
        """
        Update entire assembly to frame number specified.

        Args:
            index: index of frame.

        Returns: None
        """
        ...

    @constmethod
    def numberOfFrames(self) -> int:
        """Return Number of frames"""
        ...

    @constmethod
    def undoSolve(self) -> None:
        """
        Undo the last solve of the assembly and return part placements to their initial position.
        """
        ...

    @constmethod
    def ensureIdentityPlacements(self) -> None:
        """
        Makes sure that LinkGroups or sub-assemblies have identity placements.
        """
        ...

    @constmethod
    def clearUndo(self) -> None:
        """
        Clear the registered undo positions.
        """
        ...

    @constmethod
    def isPartConnected(self, obj: DocumentObject, /) -> bool:
        """
        Check if a part is connected to the ground through joints.
        Returns: True if part is connected to ground.
        """
        ...

    @constmethod
    def isJointConnectingPartToGround(self, joint: DocumentObject, prop_name: str, /) -> Any:
        """
        Check if a joint is connecting a part to the ground.

        Args:
        - joint: document object of the joint to check.
        - prop_name: string 'Part1' or 'Part2' of the joint.

        Returns: True if part is connected to ground.
        """
        ...

    @constmethod
    def isPartGrounded(self, obj: DocumentObject, /) -> Any:
        """
        Check if a part has a grounded joint.

        Args:
        - obj: document object of the part to check.

        Returns: True if part has grounded joint.
        """
        ...

    @constmethod
    def exportAsASMT(self, file_name: str, /) -> None:
        """
        Export the assembly in a text format called ASMT.

        Args:
        - fileName: The name of the file where the ASMT will be exported.
        """
        ...

    @constmethod
    def getDownstreamParts(
        self, start_part: DocumentObject, joint_to_ignore: DocumentObject, /
    ) -> list[DocumentObject]:
        """
        Finds all parts connected to a start_part that are not connected to ground
        when a specific joint is ignored.

        This is used to find the entire rigid group of unconstrained components that
        should be moved together during a pre-solve operation or a drag.

        Args:
            start_part: The App.DocumentObject to begin the search from.
            joint_to_ignore: The App.DocumentObject (a joint) to temporarily
                             suppress during the connectivity check.

        Returns:
            A list of App.DocumentObject instances representing the downstream parts.
        """
        ...
    Joints: Final[list]
    """A list of all joints this assembly has."""
