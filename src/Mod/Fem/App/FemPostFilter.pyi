# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, TypeAlias

from Base.Metadata import export
from Fem.FemPostObject import FemPostObject

vtkAlgorithm: TypeAlias = object

@export(
    Include="Mod/Fem/App/FemPostFilter.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostObjectPy.h",
    FatherNamespace="Fem",
)
class FemPostFilter(FemPostObject):
    """
    The FemPostFilter class.

    Author: Stefan Tröger (stefantroeger@gmx.net)
    License: LGPL-2.1-or-later
    """

    def addFilterPipeline(self, name: str, source: vtkAlgorithm, target: vtkAlgorithm, /) -> None:
        """Registers a new vtk filter pipeline for data processing. Arguments are (name, source algorithm, target algorithm)."""
        ...

    def setActiveFilterPipeline(self, name: str, /) -> None:
        """Sets the filter pipeline that shall be used for data processing. Argument is the name of the filter pipeline to activate."""
        ...

    def getParentPostGroup(self) -> object:
        """Returns the postprocessing group the filter is in (e.g. a pipeline or branch object). None is returned if not in any."""
        ...

    def getInputData(self) -> object:
        """
        Returns the dataset available at the filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getInputVectorFields(self) -> list[str]:
        """
        Returns the names of all vector fields available on this filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getInputScalarFields(self) -> list[str]:
        """
        Returns the names of all scalar fields available on this filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getOutputAlgorithm(self) -> vtkAlgorithm:
        """Returns the filters vtk algorithm currently used as output (the one generating the Data field). Note that the output algorithm may change depending on filter settings."""
        ...
