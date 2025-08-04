from typing import Any

from Base.Metadata import export

from Gui.ViewProviderDocumentObject import ViewProviderDocumentObject

@export(
    Father="ViewProviderDocumentObjectPy",
    Name="ViewProviderFemPostFilterPy",
    Twin="ViewProviderFemPostObject",
    TwinPointer="ViewProviderFemPostObject",
    Include="Mod/Fem/Gui/ViewProviderFemPostObject.h",
    Namespace="FemGui",
    FatherInclude="Gui/ViewProviderDocumentObjectPy.h",
    FatherNamespace="Gui",
)
class ViewProviderFemPostFilterPy(ViewProviderDocumentObject):
    """
    ViewProviderFemPostPipeline class
    """

    def createDisplayTaskWidget(self) -> Any:
        """Returns the display option task panel for a post processing edit task dialog."""
        ...

    def createExtractionTaskWidget(self) -> Any:
        """Returns the data extraction task panel for a post processing edit task dialog."""
        ...
