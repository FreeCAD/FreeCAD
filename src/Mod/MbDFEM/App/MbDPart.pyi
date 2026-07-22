# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.DocumentObject import DocumentObject
from App.DocumentObjectGroup import DocumentObjectGroup


@export(Include="Mod/MbDFEM/App/MbDPart.h", Namespace="MbDFEM")
class MbDPart(DocumentObjectGroup):
    """A minimal multibody part."""

    def addMarker(self, marker: DocumentObject, /) -> None:
        """Add an MbDMarker to markers, ignoring duplicates."""
        ...
