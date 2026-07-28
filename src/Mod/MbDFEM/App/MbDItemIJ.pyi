# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.DocumentObject import DocumentObject


@export(Include="Mod/MbDFEM/App/MbDItemIJ.h", Namespace="MbDFEM")
class MbDItemIJ(DocumentObject):
    """A multibody item that references marker I and marker J."""

    def setMarkerI(self, marker: DocumentObject, /) -> None:
        """Set markerI to an MbDMarker."""
        ...

    def setMarkerJ(self, marker: DocumentObject, /) -> None:
        """Set markerJ to an MbDMarker."""
        ...

    def setMarkers(self, markerI: DocumentObject, markerJ: DocumentObject, /) -> None:
        """Set markerI and markerJ to MbDMarker objects."""
        ...
