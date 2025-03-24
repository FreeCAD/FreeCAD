from Base.Metadata import export
from Gui.ViewProviderPartExt import ViewProviderPartExt
from typing import Final, overload

@export(
    Include="Mod/PartDesign/Gui/ViewProvider.h",
    Namespace="PartDesignGui",
    FatherInclude="Mod/Part/Gui/ViewProviderPartExtPy.h",
    FatherNamespace="PartGui"
)
class ViewProvider(ViewProviderPartExt):
    """
    This is the father of all PartDesign ViewProvider classes

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    def setBodyMode(self, mode: bool) -> None:
        """
        setBodyMode(bool): body mode means that the object is part of a body
        and that the body is used to set the visual properties, not the features. Hence
        setting body mode to true will hide most viewprovider properties.
        """
        ...

    def makeTemporaryVisible(self, visible: bool) -> None:
        """
        makeTemporaryVisible(bool): makes this viewprovider visible in the
        scene graph without changing any properties, not the visibility one and also not
        the display mode. This can be used to show the shape of this viewprovider from
        other viewproviders without doing anything to the document and properties.
        """
        ...
