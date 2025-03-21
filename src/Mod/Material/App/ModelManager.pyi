from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Final, List, Dict, overload

@export(
    Include="Mod/Material/App/ModelManager.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class ModelManager(BaseClass):
    """
    Material model descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    ModelLibraries: Final[List] = []
    """List of model libraries."""

    Models: Final[Dict] = {}
    """List of model libraries."""

    def getModel(self) -> object:
        """
        Get a model object by specifying its UUID
        """
        ...

    def getModelByPath(self) -> object:
        """
        Get a model object by specifying its path
        """
        ...
