from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/CAM/App/FeatureArea.h",
    Namespace="Path",
)
class FeatureArea(DocumentObject):
    """
    This class handles Path Area features

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    License: LGPL-2.1-or-later
    """

    def getArea(self) -> Any:
        """Return a copy of the encapsulated Python Area object."""
        ...

    def setParams(self, **kwargs) -> Any:
        """setParams(key=value...): Convenient function to configure this feature.

        Same usage as Path.Area.setParams(). This function stores the parameters in the properties.
        """
        ...
    WorkPlane: Any
    """The current workplane. If no plane is set, it is derived from the added shapes."""
