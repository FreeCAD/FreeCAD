# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.DocumentObject import DocumentObject
from App.DocumentObjectGroup import DocumentObjectGroup


@export(Include="Mod/MbDFEM/App/MbDAssembly.h", Namespace="MbDFEM")
class MbDAssembly(DocumentObjectGroup):
    """A minimal multibody assembly container."""

    def addPart(self, part: DocumentObject, /) -> None:
        """Add an MbDPart to parts, ignoring duplicates."""
        ...

    def addMarker(self, marker: DocumentObject, /) -> None:
        """Add an MbDMarker to markers, ignoring duplicates."""
        ...
