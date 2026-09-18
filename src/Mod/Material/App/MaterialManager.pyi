# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from Material import Material
from typing import Final, List, Dict


@export(Include="Mod/Material/App/MaterialManager.h", Namespace="Materials", Constructor=True)
class MaterialManager(BaseClass):
    """
    Material descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    MaterialLibraries: Final[List] = ...
    """List of Material libraries."""

    Materials: Final[Dict] = ...
    """List of Materials."""

    def getMaterial(self, uuid: str, /) -> Material:
        """
        Get a material object by specifying its UUID
        """
        ...

    def getMaterialByPath(self, path: str, library: str = "", /) -> Material:
        """
        Get a material object by specifying its path and library name
        """
        ...

    def inheritMaterial(self, uuid: str, /) -> Material:
        """
        Create a new material object by specifying the UUID of its parent
        """
        ...

    def materialsWithModel(self, uuid: str, /) -> Dict[str, Material]:
        """
        Get a dictionary of materials implementing the specified model, keyed by material UUID
        """
        ...

    def materialsWithModelComplete(self, uuid: str, /) -> Dict[str, Material]:
        """
        Get a dictionary of materials implementing the specified model, with values for all properties, keyed by material UUID
        """
        ...

    def save(
        self,
        library: str,
        material: Material,
        path: str,
        overwrite: bool = False,
        saveAsCopy: bool = False,
        saveInherited: bool = False,
    ) -> None:
        """
        Save the material in the specified library
        """
        ...

    def filterMaterials(self, **kwargs) -> None:
        """
        Returns a filtered material list
        """
        ...

    def refresh(self) -> None:
        """
        Refreshes the material tree. Use sparingly as this is an expensive operation.
        """
        ...
