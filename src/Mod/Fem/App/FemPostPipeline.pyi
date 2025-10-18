# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, overload, TypeAlias

from Base.Metadata import export
from Base.Unit import Unit
from Fem.FemPostObject import FemPostObject
from App.DocumentObject import DocumentObject

vtkAlgorithm: TypeAlias = object

@export(
    Include="Mod/Fem/App/FemPostPipeline.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostObjectPy.h",
    FatherNamespace="Fem",
)
class FemPostPipeline(FemPostObject):
    """
    The FemPostPipeline class.

    Author: Stefan Tröger (stefantroeger@gmx.net)
    License: LGPL-2.1-or-later
    """

    @overload
    def read(self, file_name: str, /) -> None: ...
    @overload
    def read(
        self,
        files: list[str] | tuple[str],
        values: list[int] | tuple[int],
        unit: Unit,
        frame_type: str,
        /,
    ) -> None: ...
    def read(self, *args) -> None:
        """
        Reads in a single vtk file or creates a multiframe result by reading in multiple result files.

        If multiframe is wanted, 4 argumenhts are needed:
        1. List of result files each being one frame,
        2. List of values valid for each frame (e.g. [s] if time data),
        3. the unit of the value as FreeCAD.Units.Unit,
        4. the Description of the frame type
        """
        ...

    def scale(self, scale: float, /) -> None:
        """scale the points of a loaded vtk file"""
        ...

    @overload
    def load(self, obj: DocumentObject, /) -> None: ...
    @overload
    def load(
        self,
        result: list[DocumentObject] | tuple[DocumentObject],
        values: list[float] | tuple[float],
        unit: Unit,
        frame_type: str,
        /,
    ) -> None: ...
    def load(self, *args) -> Any:
        """
        Load a single result object or create a multiframe result by loading multiple result frames.

        If multiframe is wanted, 4 argumenhts are needed:
        1. List of result objects each being one frame,
        2. List of values valid for each frame (e.g. [s] if time data),
        3. the unit of the value as FreeCAD.Units.Unit,
        4. the Description of the frame type
        """
        ...

    def getFilter(self) -> list[object]:
        """Returns all filters, that this pipeline uses (non recursive, result does not contain branch child filters)"""
        ...

    def recomputeChildren(self) -> None:
        """Recomputes all children of the pipeline"""
        ...

    def getLastPostObject(self) -> DocumentObject | None:
        """Get the last post-processing object"""
        ...

    def holdsPostObject(self, obj: DocumentObject, /) -> bool:
        """Check if this pipeline holds a given post-processing object"""
        ...

    def renameArrays(self, names: dict[str, str], /) -> None:
        """Change name of data arrays"""
        ...

    def getOutputAlgorithm(self) -> vtkAlgorithm:
        """Returns the pipeline vtk algorithm, which generates the data passed to the pipelines filters.

        Note that the output algorithm may change depending on pipeline settings.
        """
        ...
