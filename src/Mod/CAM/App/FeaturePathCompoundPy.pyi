from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="FeaturePathCompoundPy",
    Twin="FeaturePathCompound",
    TwinPointer="FeatureCompound",
    Include="Mod/CAM/App/FeaturePathCompound.h",
    Namespace="Path",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class FeaturePathCompoundPy(DocumentObject):
    """
    This class handles Path Compound features
    """

    def addObject(self) -> Any:
        """Add an object to the group"""
        ...

    def removeObject(self) -> Any:
        """Remove an object from the group"""
        ...
