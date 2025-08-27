from typing import Any

from Base.Metadata import export

from Fem.FemPostObject import FemPostObject

@export(
    Father="FemPostObjectPy",
    Name="FemPostFilterPy",
    Twin="FemPostFilter",
    TwinPointer="FemPostFilter",
    Include="Mod/Fem/App/FemPostFilter.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostObjectPy.h",
    FatherNamespace="Fem",
)
class FemPostFilterPy(FemPostObject):
    """
    The FemPostFilter class.
    """

    def addFilterPipeline(self) -> Any:
        """Registers a new vtk filter pipeline for data processing. Arguments are (name, source algorithm, target algorithm)."""
        ...

    def setActiveFilterPipeline(self) -> Any:
        """Sets the filter pipeline that shall be used for data processing. Argument is the name of the filter pipeline to activate."""
        ...

    def getParentPostGroup(self) -> Any:
        """Returns the postprocessing group the filter is in (e.g. a pipeline or branch object). None is returned if not in any."""
        ...

    def getInputData(self) -> Any:
        """Returns the dataset available at the filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getInputVectorFields(self) -> Any:
        """Returns the names of all vector fields available on this filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getInputScalarFields(self) -> Any:
        """Returns the names of all scalar fields available on this filter's input.
        Note: Can lead to a full recompute of the whole pipeline, hence best to call this only in "execute", where the user expects long calculation cycles.
        """
        ...

    def getOutputAlgorithm(self) -> Any:
        """Returns the filters vtk algorithm currently used as output (the one generating the Data field). Note that the output algorithm may change depending on filter settings."""
        ...
