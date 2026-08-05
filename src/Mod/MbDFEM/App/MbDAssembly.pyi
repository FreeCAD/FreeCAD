# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.Part import Part
from App.DocumentObject import DocumentObject


@export(
    Include="Mod/MbDFEM/App/MbDAssembly.h",
    Namespace="MbDFEM",
    FatherInclude="App/PartPy.h",
    FatherNamespace="App",
)
class MbDAssembly(Part):
    """A minimal multibody assembly."""

    def addAssembly(self, assembly: DocumentObject, /) -> None:
        """Add an MbDAssembly to assemblies, ignoring duplicates and self-links."""
        ...

    def addPart(self, part: DocumentObject, /) -> None:
        """Add an MbDPart to parts, ignoring duplicates."""
        ...

    def removePart(self, part: DocumentObject, /) -> None:
        """Remove an MbDPart from parts and the Parts tree folder."""
        ...

    def addFixedPart(self, part: DocumentObject, /) -> None:
        """Add an MbDPart to fixedparts, ignoring duplicates."""
        ...

    def removeFixedPart(self, part: DocumentObject, /) -> None:
        """Remove an MbDPart from fixedparts and the FixedParts tree folder."""
        ...

    def groundPart(self, part: DocumentObject, /) -> None:
        """Move an MbDPart from parts to fixedparts."""
        ...

    def addJoint(self, joint: DocumentObject, /) -> None:
        """Add an MbDJoint to joints, ignoring duplicates."""
        ...

    def addMotion(self, motion: DocumentObject, /) -> None:
        """Add an MbDMotion to motions, ignoring duplicates."""
        ...

    def addAction(self, action: DocumentObject, /) -> None:
        """Add an MbDAction to actions, ignoring duplicates."""
        ...

    def getAssembliesFolder(self) -> DocumentObject:
        """Return the lightweight Assemblies tree folder."""
        ...

    def getPartsFolder(self) -> DocumentObject:
        """Return the lightweight Parts tree folder."""
        ...

    def getFixedPartsFolder(self) -> DocumentObject:
        """Return the lightweight FixedParts tree folder."""
        ...

    def getJointsFolder(self) -> DocumentObject:
        """Return the lightweight Joints tree folder."""
        ...

    def getMotionsFolder(self) -> DocumentObject:
        """Return the lightweight Motions tree folder."""
        ...

    def getActionsFolder(self) -> DocumentObject:
        """Return the lightweight Actions tree folder."""
        ...

    def getGravity(self) -> DocumentObject:
        """Return the assembly Gravity object."""
        ...

    def getSimulationParameters(self) -> DocumentObject:
        """Return the assembly SimulationParameters object."""
        ...

    def getAnimationParameters(self) -> DocumentObject:
        """Return the assembly AnimationParameters object."""
        ...

    def ensureSimulationParameters(self) -> DocumentObject:
        """Create and return the assembly SimulationParameters object."""
        ...

    def ensureAnimationParameters(self) -> DocumentObject:
        """Create and return the assembly AnimationParameters object."""
        ...

    def ensureGravity(self) -> DocumentObject:
        """Create and return the assembly Gravity object."""
        ...
