from typing import Any

from Base.Metadata import export

from Fem.FemPostObject import FemPostObject

@export(
    Father="FemPostObjectPy",
    Name="FemPostPipelinePy",
    Twin="FemPostPipeline",
    TwinPointer="FemPostPipeline",
    Include="Mod/Fem/App/FemPostPipeline.h",
    Namespace="Fem",
    FatherInclude="Mod/Fem/App/FemPostObjectPy.h",
    FatherNamespace="Fem",
)
class FemPostPipelinePy(FemPostObject):
    """
    The FemPostPipeline class.
    """

    def read(self) -> Any:
        """read(filepath)
        read([filepaths], [values], unit, frame_type)

        Reads in a single vtk file or creates a multiframe result by reading in multiple result files. If multiframe is wanted, 4 argumenhts are needed:
        1. List of result files each being one frame,
        2. List of values valid for each frame (e.g. [s] if time data),
        3. the unit of the value as FreeCAD.Units.Unit,
        4. the Description of the frame type"""
        ...

    def scale(self) -> Any:
        """scale the points of a loaded vtk file"""
        ...

    def load(self) -> Any:
        """load(result_object)
        load([result_objects], [values], unit, frame_type)

        Load a single result object or create a multiframe result by loading multiple result frames. If multiframe is wanted, 4 argumenhts are needed:
        1. List of result files each being one frame,
        2. List of values valid for each frame (e.g. [s] if time data),
        3. the unit of the value as FreeCAD.Units.Unit,
        4. the Description of the frame type"""
        ...

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

    def renameArrays(self) -> Any:
        """Change name of data arrays"""
        ...

    def getOutputAlgorithm(self) -> Any:
        """Returns the pipeline vtk algorithm, which generates the data passed to the pipelines filters. Note that the output algorithm may change depending on pipeline settings."""
        ...
