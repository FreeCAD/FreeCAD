from Base.Metadata import export
from Gui.ViewProviderExtension import ViewProviderExtension
from typing import Any, List

@export(
    Namespace="FemGui",
)
class ViewProviderShapeExtension(ViewProviderExtension):
    """
    Extension class which adds visualizations for FEM shape objects
    Author: Stefan Tröger (stefantroeger@gmx.net)
    Licence: LGPL
    """

    def createControlWidget(self) -> Any:
        """
        Creates a QWidget which allows manipulation of the shape properties
        """
        ...
