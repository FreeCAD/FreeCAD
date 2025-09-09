from typing import Any

from Base.Metadata import export

from App.GeoFeature import GeoFeature

@export(
    Include="Mod/Fem/App/FemPostObject.h",
    Namespace="Fem",
    FatherNamespace="App",
    FatherInclude="App/GeoFeaturePy.h",
)
class FemPostObject(GeoFeature):
    """
    The FemPostObject class.

    Author: Mario Passaglia (mpassaglia@cbc.uba.ar)
    License: LGPL-2.1-or-later
    """

    def writeVTK(self) -> Any:
        """writeVTK(filename) -> None

        Write data object to VTK file.

        filename: str
            File extension is automatically detected from data type."""
        ...

    def getDataSet(self) -> Any:
        """getDataset() -> vtkDataSet

        Returns the current output dataset. For normal filters this is equal to the objects Data property output. However, a pipelines Data property could store multiple frames, and hence Data can be of type vtkCompositeData, which is not a vtkDataset. To simplify implementations this function always returns a vtkDataSet, and for a pipeline it will be the dataset of the currently selected frame. Note that the returned value could be None, if no data is set at all.
        """
        ...
