from Base.Metadata import export
from Base.BaseClass import BaseClass
from App.Material import Material
from typing import List, Dict, Final, overload

@export(
    Include="Mod/Material/App/MaterialManager.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True
)
class MaterialManager(BaseClass):
    """
    Material descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    MaterialLibraries: Final[List[str]]
    """List of Material libraries."""

    Materials: Final[Dict[str, Material]]
    """List of Materials."""

    def getMaterial(self) -> Material:
        """
        Get a material object by specifying its UUID
        """
        ...

    def getMaterialByPath(self) -> Material:
        """
        Get a material object by specifying its path and library name
        """
        ...

    def inheritMaterial(self) -> Material:
        """
        Create a new material object by specifying the UUID of its parent
        """
        ...

    def materialsWithModel(self) -> List[Material]:
        """
        Get a list of materials implementing the specified model
        """
        ...

    def materialsWithModelComplete(self) -> List[Material]:
        """
        Get a list of materials implementing the specified model, with values for all properties
        """
        ...

    def save(self, **kwargs) -> None:
        """
        Save the material in the specified library
        """
        ...

    def filterMaterials(self, **kwargs) -> List[Material]:
        """
        Returns a filtered material list
        """
        ...

    def refresh(self) -> None:
        """
        Refreshes the material tree. Use sparingly as this is an expensive operation.
        """
        ...
