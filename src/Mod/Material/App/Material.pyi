# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, no_args, sequence_protocol
from Base.BaseClass import BaseClass
from typing import Final


@export(
    Include="Mod/Material/App/Materials.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
@sequence_protocol(sq_length=True, sq_item=True, sq_contains=True, mp_subscript=True)
class Material(BaseClass):
    """
    Material descriptions.

    Author: David Carter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    LibraryName: Final[str] = ...
    """Material library name."""

    LibraryRoot: Final[str] = ...
    """Material library path."""

    LibraryIcon: Final[bytes] = ...
    """Material icon."""

    Name: str = ...
    """Material name."""

    Directory: str = ...
    """Material directory relative to the library root."""

    UUID: Final[str] = ...
    """Unique material identifier. This is only valid after the material is saved."""

    Description: str = ...
    """Description of the material."""

    URL: str = ...
    """URL to a material reference."""

    Reference: str = ...
    """Reference for material data."""

    Parent: str = ...
    """Parent material UUID."""

    AuthorAndLicense: Final[str] = ...
    """deprecated -- Author and license information."""

    Author: str = ...
    """Author information."""

    License: str = ...
    """License information."""

    PhysicalModels: Final[list] = ...
    """List of implemented models."""

    AppearanceModels: Final[list] = ...
    """List of implemented models."""

    Tags: Final[list] = ...
    """List of searchable tags."""

    Properties: Final[dict] = ...
    """deprecated -- Dictionary of all material properties."""

    PhysicalProperties: Final[dict] = ...
    """deprecated -- Dictionary of material physical properties."""

    AppearanceProperties: Final[dict] = ...
    """deprecated -- Dictionary of material appearance properties."""

    LegacyProperties: Final[dict] = ...
    """deprecated -- Dictionary of material legacy properties."""

    PropertyObjects: Final[dict] = ...
    """Dictionary of MaterialProperty objects."""

    def addPhysicalModel(self) -> None:
        """Add the physical model with the given UUID"""
        ...

    def removePhysicalModel(self) -> None:
        """Remove the physical model with the given UUID"""
        ...

    def hasPhysicalModel(self) -> bool:
        """Check if the material implements the physical model with the given UUID"""
        ...

    def addAppearanceModel(self) -> None:
        """Add the appearance model with the given UUID"""
        ...

    def removeAppearanceModel(self) -> None:
        """Remove the appearance model with the given UUID"""
        ...

    def hasAppearanceModel(self) -> bool:
        """Check if the material implements the appearance model with the given UUID"""
        ...

    def isPhysicalModelComplete(self) -> bool:
        """Check if the material implements the physical model with the given UUID, and has values defined for each property"""
        ...

    def isAppearanceModelComplete(self) -> bool:
        """Check if the material implements the appearance model with the given UUID, and has values defined for each property"""
        ...

    def hasPhysicalProperty(self) -> bool:
        """Check if the material implements the physical property with the given name"""
        ...

    def hasAppearanceProperty(self) -> bool:
        """Check if the material implements the appearance property with the given name"""
        ...

    def hasLegacyProperties(self) -> bool:
        """Returns true of there are legacy properties"""
        ...

    def getPhysicalValue(self) -> str:
        """Get the value associated with the property"""
        ...

    def setPhysicalValue(self) -> None:
        """Set the value associated with the property"""
        ...

    def getAppearanceValue(self) -> str:
        """Get the value associated with the property"""
        ...

    def setAppearanceValue(self) -> None:
        """Set the value associated with the property"""
        ...

    def setValue(self) -> None:
        """Set the value associated with the property"""
        ...

    @no_args
    def keys(self) -> list:
        """Property keys"""
        ...

    @no_args
    def values(self) -> list:
        """Property values"""
        ...
