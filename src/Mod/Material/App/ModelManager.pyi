# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Final, List, Dict


@export(Include="Mod/Material/App/ModelManager.h", Namespace="Materials", Constructor=True)
class ModelManager(BaseClass):
    """
    Material model descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    ModelLibraries: Final[List] = ...
    """List of model libraries."""

    LocalModelLibraries: Final[List] = ...
    """List of local model libraries."""

    Models: Final[Dict] = ...
    """List of model libraries."""

    def getModel(self) ->...:
        """
        Get a model object by specifying its UUID
        """
        ...

    def getModelByPath(self) ->...:
        """
        Get a model object by specifying its path
        """
        ...
