from typing import Any

from Base.Metadata import export

from Gui.ViewProviderDocumentObject import ViewProviderDocumentObject

@export(
    Father="ViewProviderDocumentObjectPy",
    Name="ViewProviderFemPostPipelinePy",
    Twin="ViewProviderFemPostPipeline",
    TwinPointer="ViewProviderFemPostPipeline",
    Include="Mod/Fem/Gui/ViewProviderFemPostPipeline.h",
    Namespace="FemGui",
    FatherInclude="Gui/ViewProviderDocumentObjectPy.h",
    FatherNamespace="Gui",
)
class ViewProviderFemPostPipelinePy(ViewProviderDocumentObject):
    """
    ViewProviderFemPostPipeline class
    """

    def transformField(self) -> Any:
        """Scales values of given result mesh field by given factor"""
        ...

    def updateColorBars(self) -> Any:
        """Update coloring of pipeline and its childs"""
        ...
