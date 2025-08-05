from typing import Any

from Base.Metadata import export

from Fem.FemPostObject import FemPostObject

@export(
    Father="FemPostObjectPy",
    Name="FemPostBranchFilterPy",
    Twin="FemPostBranchFilter",
    TwinPointer="FemPostBranchFilter",
    Include="Mod/Fem/App/FemPostBranchFilter.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostObjectPy.h",
    FatherNamespace="Fem",
)
class FemPostBranchFilterPy(FemPostObject):
    """
    The FemPostBranch class.
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
