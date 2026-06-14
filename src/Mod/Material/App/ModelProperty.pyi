# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Final


@export(
    Include="Mod/Material/App/Model.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class ModelProperty(BaseClass):
    """
    Material property descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Name: str = ...
    """Property name."""

    DisplayName: str = ...
    """Property display friendly name."""

    Type: str = ...
    """Property type."""

    Units: str = ...
    """Property units category."""

    URL: str = ...
    """URL to a detailed description of the property."""

    Description: str = ...
    """Property description."""

    Columns: Final[list] = ...
    """List of array columns."""

    Inheritance: Final[str] = ...
    """UUID of the model in which the property is defined."""

    Inherited: Final[bool] = ...
    """True if the property is inherited."""

    def addColumn(self) -> None:
        """
        Add a model property column.
        """
        ...
