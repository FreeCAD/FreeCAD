import FreeCAD
from FreeCAD import Console as FCC
import Draft
import DraftGeomUtils

if FreeCAD.GuiUp:
    import FreeCADGui
    import DraftGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    from DraftTools import translate
    from DraftTools import Line
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
        Create the chamfer.

    Returns
    -------
    Part::Feature
        The objects of type Fillet.
    """
    if len(edge_list) > 2:
        FCC.PrintError("makeFillet: edge_list too many elements")
    e1, e2 = edge_list
    if "Proxy" in e1.PropertiesList:
        if hasattr(e1.Proxy, "Type"):
            if e1.Proxy.Type in "Wire":
                e1 = e1.Shape.Edges[0]
    elif "Shape" in e1.PropertiesList:
        if e1.Shape.ShapeType in "Edge":
            e1 = e1.Shape
    if "Proxy" in e2.PropertiesList:
        if hasattr(e2.Proxy, "Type"):
            if e2.Proxy.Type in "Wire":
                e2 = e2.Shape.Edges[0]
    elif "Shape" in e2.PropertiesList:
        if e2.Shape.ShapeType in "Edge":
            e2 = e2.Shape

    edges = DraftGeomUtils.fillet([e1, e2], radius, chamfer)
    add, delete = Draft.upgrade(edges, delete=True)
    newobj = add[0]
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",
                                           "LineFillet")
    Fillet(obj)
    obj.Shape = newobj.Shape
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

    def execute(self, obj):
        pass

    def onChanged(self, obj, prop):
        # Change the radius of fillet
        pass


class CommandFillet(Line):
    def __init__(self):
        Line.__init__(self, wiremode=True)

    def GetResources(self):
        return {'Pixmap': 'Draft_Fillet.svg',
                'MenuText': QT_TRANSLATE_NOOP("draft", "Fillet"),
                'ToolTip': QT_TRANSLATE_NOOP("draft", "Creates a fillet between two wires or edges.")
        }

    def Activated(self):
        if len(FreeCADGui.Selection.getSelection()) > 1:
            edges = []
            edges = FreeCADGui.Selection.getSelection()
            for o in edges:
                # Choose only Wires
                if Draft.getType(o) != "Wire":
                    edges = []
                    break
                # edges.extend(o.Shape.Edges)
                FCC.PrintMessage("makeFillet: " + Draft.getType(o) + "\n")
            if edges:
                if len(edges) > 2:
                    FCC.PrintError("CommandFillet: too many elements")
                doc = 'FreeCAD.ActiveDocument.'
                _edges = '[' + doc + edges[0].Name + ', ' + doc + edges[1].Name + ']'
                rems = [doc + 'removeObject("' + o.Name + '")' for o in FreeCADGui.Selection.getSelection()]
                FreeCADGui.addModule("Draft")
                func = translate("draft", "Create fillet")
                arg = ['arc = DraftFillet.makeFillet(' + _edges + ')'] + rems + ['Draft.autogroup(arc)', 'FreeCAD.ActiveDocument.recompute()']
                DraftGui.todo.delayCommit([(func, arg)])


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Draft_Fillet', CommandFillet())
