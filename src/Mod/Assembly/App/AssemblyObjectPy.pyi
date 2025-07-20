from typing import Any, Final

from Base.Metadata import constmethod, export

from App.Part import Part

@export(
    Father="PartPy",
    Name="AssemblyObjectPy",
    Twin="AssemblyObject",
    TwinPointer="AssemblyObject",
    Include="Mod/Assembly/App/AssemblyObject.h",
    Namespace="Assembly",
    FatherInclude="App/PartPy.h",
    FatherNamespace="App",
)
class AssemblyObjectPy(Part):
    """
    This class handles document objects in Assembly
    """

    @constmethod
    def solve(self) -> Any:
        """Solve the assembly and update part placements.

        solve(enableRedo=False) -> int

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
        -2 if redundant constraints."""
        ...

    @constmethod
    def generateSimulation(self) -> Any:
        """Generate the simulation.

        solve(simulationObject) -> int

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
        -2 if redundant constraints."""
        ...

    @constmethod
    def updateForFrame(self) -> Any:
        """Update entire assembly to frame number specified.

        updateForFrame(index)

        Args: index of frame.

        Returns: None"""
        ...

    @constmethod
    def numberOfFrames(self) -> Any:
        """numberOfFrames()

        Args: None

        Returns: Number of frames"""
        ...

    @constmethod
    def undoSolve(self) -> Any:
        """Undo the last solve of the assembly and return part placements to their initial position.

        undoSolve()

        Returns: None"""
        ...

    @constmethod
    def ensureIdentityPlacements(self) -> Any:
        """Makes sure that LinkGroups or sub-assemblies have identity placements.

        ensureIdentityPlacements()

        Returns: None"""
        ...

    @constmethod
    def clearUndo(self) -> Any:
        """Clear the registered undo positions.

        clearUndo()

        Returns: None"""
        ...

    @constmethod
    def isPartConnected(self) -> Any:
        """Check if a part is connected to the ground through joints.

        isPartConnected(obj) -> bool

        Args: document object to check.

        Returns: True if part is connected to ground"""
        ...

    @constmethod
    def isJointConnectingPartToGround(self) -> Any:
        """Check if a joint is connecting a part to the ground.

        isJointConnectingPartToGround(joint, propName) -> bool

        Args:
        - joint: document object of the joint to check.
        - propName: string 'Part1' or 'Part2' of the joint.

        Returns: True if part is connected to ground"""
        ...

    @constmethod
    def isPartGrounded(self) -> Any:
        """Check if a part has a grounded joint.

        isPartGrounded(obj) -> bool

        Args:
        - obj: document object of the part to check.

        Returns: True if part has grounded joint"""
        ...

    @constmethod
    def exportAsASMT(self) -> Any:
        """Export the assembly in a text format called ASMT.

        exportAsASMT(fileName:str)

        Args:
        fileName: The name of the file where the ASMT will be exported."""
        ...
    Joints: Final[list]
    """A list of all joints this assembly has."""
