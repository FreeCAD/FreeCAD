# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export

from Fem.FemPostFilter import FemPostFilter

@export(
    Include="Mod/Fem/App/FemPostBranchFilter.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostFilterPy.h",
    FatherNamespace="Fem",
)
class FemPostBranchFilter(FemPostFilter):
    """
    The FemPostBranch class.

    Author: Stefan Tröger (stefantroeger@gmx.net)
    License: LGPL-2.1-or-later
    """

    def getFilter(self) -> Any:
        """Returns all filters, that this pipeline uses (non recursive, result does not contain branch child filters)"""
        ...

    def recomputeChildren(self) -> Any:
        """Recomputes all children of the pipeline"""
        ...

    def getLastPostObject(self) -> Any:
        """Get the last post-processing object"""
        ...

    def holdsPostObject(self) -> Any:
        """Check if this pipeline holds a given post-processing object"""
        ...
