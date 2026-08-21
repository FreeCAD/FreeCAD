# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export

from App.DocumentObject import DocumentObject
from PartFeature import PartFeature


@export(
    Include="Mod/MbDFEM/App/MbDPart.h",
    Namespace="MbDFEM",
    FatherInclude="Mod/Part/App/PartFeaturePy.h",
    FatherNamespace="Part",
)
class MbDPart(PartFeature):
    """A minimal multibody part."""

    def addMarker(self, marker: DocumentObject, /) -> None:
        """Add an MbDMarker to markers, ignoring duplicates."""
        ...

    def removeMarker(self, marker: DocumentObject, /) -> None:
        """Remove an MbDMarker from markers without deleting it."""
        ...

    def getMassMarker(self) -> DocumentObject:
        """Return the part center-of-mass/principal-axes marker."""
        ...

    def ensureMassMarker(self) -> DocumentObject:
        """Create and return the part center-of-mass/principal-axes marker."""
        ...

    def populateMassMarkerFromShape(self) -> DocumentObject:
        """Populate massMarker from this part's shape center of mass and principal axes."""
        ...

    def getMarkersFolder(self) -> DocumentObject:
        """Return the lightweight Markers tree folder."""
        ...
