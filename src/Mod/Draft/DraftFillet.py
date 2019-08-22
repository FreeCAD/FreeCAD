import FreeCAD
from FreeCAD import Console as FCC
import Draft
import DraftGeomUtils
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    import DraftGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import DraftTools
else:
    def QT_TRANSLATE_NOOP(context, text):
        return text

    def translate(context, text):
        return text


def makeFillet(edge_list, radius=100, chamfer=False):
    """Create a fillet between two lines or edges.

    Parameters
    ----------
    edge_list : list
        List of two objects of type wire, or edges.
    radius : float, optional
        It defaults to 100 mm. The curvature of the fillet.
    chamfer : bool
        Defaults to `False`. If `True` it corrects
        the value of the `radius` so that the chamfer is exactly the radius.

    Returns
    -------
    Part::Feature
        The objects of type Fillet.
    """
    if len(edge_list) != 2:
        FCC.PrintError("makeFillet: two elements needed" + "\n")
        return None

    e1, e2 = edge_list
    if "Proxy" in e1.PropertiesList:
        if hasattr(e1.Proxy, "Type"):
            if e1.Proxy.Type in "Wire":
                FCC.PrintMessage("e1 : " + e1.Label + "\n")
                e1 = e1.Shape.Edges[0]
    elif "Shape" in e1.PropertiesList:
        if e1.Shape.ShapeType in "Edge":
            e1 = e1.Shape
    if "Proxy" in e2.PropertiesList:
        if hasattr(e2.Proxy, "Type"):
            if e2.Proxy.Type in "Wire":
                FCC.PrintMessage("e2 : " + e2.Label + "\n")
                e2 = e2.Shape.Edges[0]
    elif "Shape" in e2.PropertiesList:
        if e2.Shape.ShapeType in "Edge":
            e2 = e2.Shape

    edges = DraftGeomUtils.fillet([e1, e2], radius, chamfer)
    FCC.PrintMessage("E1 :" + str(edges[0]) + "\n")
    FCC.PrintMessage("E2 :" + str(edges[1]) + "\n")
    FCC.PrintMessage("E3 :" + str(edges[2]) + "\n")
    # add, delete = Draft.upgrade(edges, delete=True)

    try:
        wire = Part.Wire(edges)
    except Part.OCCError:
        return None

    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",
                                           "Fillet")
    Fillet(obj)
    obj.Shape = wire
    obj.Length = wire.Length
    obj.Start = wire.Vertexes[0].Point
    obj.End = wire.Vertexes[-1].Point
    obj.FilletRadius = radius

    if FreeCAD.GuiUp:
        Draft._ViewProviderWire(obj.ViewObject)
        Draft.formatObject(obj)
        Draft.select(obj)
    return obj


class Fillet(Draft._DraftObject):
    """The fillet object"""

    def __init__(self, obj):
        Draft._DraftObject.__init__(self, obj, "Fillet")
        obj.addProperty("App::PropertyVectorDistance", "Start", "Draft", QT_TRANSLATE_NOOP("App::Property", "The start point of this line"))
        obj.addProperty("App::PropertyVectorDistance", "End", "Draft", QT_TRANSLATE_NOOP("App::Property", "The end point of this line"))
        obj.addProperty("App::PropertyLength", "Length", "Draft", QT_TRANSLATE_NOOP("App::Property", "The length of this line"))
        obj.addProperty("App::PropertyLength", "FilletRadius", "Draft", QT_TRANSLATE_NOOP("App::Property", "Radius to use to fillet the corners"))
        obj.setEditorMode("Start", 1)
        obj.setEditorMode("End", 1)
        obj.setEditorMode("Length", 1)
        # Change to 0 to make it editable
        obj.setEditorMode("FilletRadius", 1)

    def execute(self, obj):
        if hasattr(obj, "Length"):
            obj.Length = obj.Shape.Length
        if hasattr(obj, "Start"):
            obj.Start = obj.Shape.Vertexes[0].Point
        if hasattr(obj, "End"):
            obj.End = obj.Shape.Vertexes[-1].Point

    def onChanged(self, obj, prop):
        # Change the radius of fillet. NOT IMPLEMENTED.
        if prop in "FilletRadius":
            pass


class CommandFillet(DraftTools.Line):
    def __init__(self):
        DraftTools.Line.__init__(self, wiremode=True)

    def GetResources(self):
        return {'Pixmap': 'Draft_Fillet.svg',
                'MenuText': QT_TRANSLATE_NOOP("draft", "Fillet"),
                'ToolTip': QT_TRANSLATE_NOOP("draft", "Creates a fillet between two wires or edges.")
                }

    def Activated(self):
        wires = FreeCADGui.Selection.getSelection()
        if not wires:
            FCC.PrintError("CommandFillet: two elements needed" + "\n")
            return

        if len(wires) != 2:
            FCC.PrintError("CommandFillet: two elements needed" + "\n")
            return

        for o in wires:
            FCC.PrintMessage("CommandFillet: " + Draft.getType(o) + "\n")

            # Choose only wires.
            # A test could be used to chose edges in general.
            if Draft.getType(o) not in "Wire":
                FCC.PrintError("CommandFillet: wires needed" + "\n")
                return

        doc = 'FreeCAD.ActiveDocument.'
        _wires = '[' + doc + wires[0].Name + ', ' + doc + wires[1].Name + ']'
        rems = [doc + 'removeObject("' + o.Name + '")' for o in wires]
        FreeCADGui.addModule("Draft")
        func = DraftTools.translate("draft", "Create fillet")

        arg = ['arc = DraftFillet.makeFillet(' + _wires + ')',
               *rems,
               'Draft.autogroup(arc)',
               'FreeCAD.ActiveDocument.recompute()']
        DraftGui.todo.delayCommit([(func, arg)])


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Draft_Fillet', CommandFillet())
