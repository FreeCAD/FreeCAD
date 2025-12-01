# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
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

    def getMaterial(self) -> None:
        """
        Get a material object by specifying its UUID
        """
        ...

    def getMaterialByPath(self) -> None:
        """
        Get a material object by specifying its path and library name
        """
        ...

    def inheritMaterial(self) -> None:
        """
        Create a new material object by specifying the UUID of its parent
        """
        ...

    def materialsWithModel(self) -> None:
        """
        Get a list of materials implementing the specified model
        """
        ...

    def materialsWithModelComplete(self) -> None:
        """
        Get a list of materials implementing the specified model, with values for all properties
        """
        ...

    def save(self, **kwargs) -> None:
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
