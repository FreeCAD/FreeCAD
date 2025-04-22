from Base.Metadata import export
from Part.App.Part2DObject import Part2DObject

@export(
    Include="Mod/Sketcher/App/SketchObjectSF.h",
    FatherInclude="Mod/Part/App/Part2DObjectPy.h",
)
class SketchObjectSF(Part2DObject):
    """
    With this objects you can handle sketches

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    ...
