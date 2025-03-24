from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import List


@export(
    Include="Mod/Material/App/MaterialFilter.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class MaterialFilter(BaseClass):
    """
    Material filters.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Name: str = ...
    """Name of the filter used to select a filter in a list"""

    RequiredModels: List = ...
    """Materials must include the specified models."""

    RequiredCompleteModels: List = ...
    """Materials must have complete versions of the specified models."""