# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from ModelProperty import ModelProperty
from typing import Final


@export(
    Include="Mod/Material/App/Materials.h",
    Namespace="Materials",
    FatherInclude="Mod/Material/App/Model.h",
    FatherNamespace="Materials",
    Constructor=True,
    Delete=False,
)
class MaterialProperty(ModelProperty):
    """
    Material property descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Value: Final[object] = None
    """The value of the material property."""

    Empty: Final[bool] = False
    """The property value is undefined."""
