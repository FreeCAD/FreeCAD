from typing import Any

from Base.Metadata import export

from App import object

@export(
    Father="DocumentObjectPy",
    Name="DrawViewPy",
    Twin="DrawView",
    TwinPointer="DrawView",
    Include="Mod/TechDraw/App/DrawView.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawViewPy(object):
    """
    Feature for creating and manipulating Technical Drawing Views
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...

    def getScale(self) -> Any:
        """
        float scale = getScale().  Returns the correct scale for this view.  Handles whether to
        use this view's scale property or a parent's view (as in a projection group).
        """
        ...
