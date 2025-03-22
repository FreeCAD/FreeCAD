from Base.Metadata import export, constmethod
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

    Name: Final[str] = ""
    """Property name."""

    Type: Final[str] = ""
    """Property type."""

    Units: Final[str] = ""
    """Property units category."""

    URL: Final[str] = ""
    """URL to a detailed description of the property."""

    Description: Final[str] = ""
    """Property description."""