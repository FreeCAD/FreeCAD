# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.DocumentObject import DocumentObject


@export(Include="Mod/MbDFEM/App/MbDAssembly.h", Namespace="MbDFEM")
class MbDAssembly(DocumentObject):
    """A minimal multibody assembly."""

    def addAssembly(self, assembly: DocumentObject, /) -> None:
        """Add an MbDAssembly to assemblies, ignoring duplicates and self-links."""
        ...

    def addPart(self, part: DocumentObject, /) -> None:
        """Add an MbDPart to parts, ignoring duplicates."""
        ...

    def addMarker(self, marker: DocumentObject, /) -> None:
        """Add an MbDMarker to markers, ignoring duplicates."""
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

    def getMarkersFolder(self) -> DocumentObject:
        """Return the lightweight Markers tree folder."""
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
