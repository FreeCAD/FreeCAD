from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="FeatureAreaPy",
    Twin="FeatureArea",
    TwinPointer="FeatureArea",
    Include="Mod/CAM/App/FeatureArea.h",
    Namespace="Path",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class FeatureAreaPy(DocumentObject):
    """
    This class handles Path Area features
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
