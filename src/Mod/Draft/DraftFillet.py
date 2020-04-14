"""This module provides the Draft fillet tool.
"""
## @package DraftFillet
# \ingroup DRAFT
# \brief This module provides the Draft fillet tool.

import FreeCAD
from FreeCAD import Console as FCC
import Draft
import DraftGeomUtils
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    from PySide import QtCore
    import DraftTools
    import draftguitools.gui_trackers as trackers
    from DraftGui import translate
else:
    def QT_TRANSLATE_NOOP(context, text):
        return text

    def translate(context, text):
        return text


def _extract_edges(objs):
    """Extract the edges from the given objects (Draft lines or Edges).

    objs : list of Draft Lines or Part.Edges
        The list of edges from which to create the fillet.
    """
    o1, o2 = objs
    if hasattr(o1, "PropertiesList"):
        if "Proxy" in o1.PropertiesList:
            if hasattr(o1.Proxy, "Type"):
                if o1.Proxy.Type in ("Wire", "Fillet"):
                    e1 = o1.Shape.Edges[0]
        elif "Shape" in o1.PropertiesList:
            if o1.Shape.ShapeType in ("Wire", "Edge"):
                e1 = o1.Shape
    elif hasattr(o1, "ShapeType"):
        if o1.ShapeType in "Edge":
            e1 = o1

    if hasattr(o1, "Label"):
        FCC.PrintMessage("o1: " + o1.Label)
    else:
        FCC.PrintMessage("o1: 1")
    FCC.PrintMessage(", length: " + str(e1.Length) + "\n")

    if hasattr(o2, "PropertiesList"):
        if "Proxy" in o2.PropertiesList:
            if hasattr(o2.Proxy, "Type"):
                if o2.Proxy.Type in ("Wire", "Fillet"):
                    e2 = o2.Shape.Edges[0]
        elif "Shape" in o2.PropertiesList:
            if o2.Shape.ShapeType in ("Wire", "Edge"):
                e2 = o2.Shape
    elif hasattr(o2, "ShapeType"):
        if o2.ShapeType in "Edge":
            e2 = o2

    if hasattr(o2, "Label"):
        FCC.PrintMessage("o2: " + o2.Label)
    else:
        FCC.PrintMessage("o2: 2")
    FCC.PrintMessage(", length: " + str(e2.Length) + "\n")

    return e1, e2


def makeFillet(objs, radius=100, chamfer=False, delete=False):
    """Create a fillet between two lines or edges.

    Parameters
    ----------
    objs : list
        List of two objects of type wire, or edges.
    radius : float, optional
        It defaults to 100 mm. The curvature of the fillet.
    chamfer : bool, optional
        It defaults to `False`. If it is `True` it no longer produces
        a rounded fillet but a chamfer (straight edge)
        with the value of the `radius`.
    delete : bool, optional
        It defaults to `False`. If it is `True` it will delete
        the pair of objects that are used to create the fillet.
        Otherwise, the original objects will still be there.

    Returns
    -------
    Part::Part2DObject
        The object of type `'Fillet'`.
        It returns `None` if it fails producing the object.
    """
    if len(objs) != 2:
        FCC.PrintError("makeFillet: "
                       + translate("draft", "two elements needed") + "\n")
        return None

    e1, e2 = _extract_edges(objs)

    edges = DraftGeomUtils.fillet([e1, e2], radius, chamfer)
    if len(edges) < 3:
        FCC.PrintError("makeFillet: "
                       + translate("draft", "radius too large"))
        FCC.PrintError(", r=" + str(radius) + "\n")
        return None

    _d = translate("draft", "length: ")
    FCC.PrintMessage("e1, " + _d + str(edges[0].Length) + "\n")
    FCC.PrintMessage("e2, " + _d + str(edges[1].Length) + "\n")
    FCC.PrintMessage("e3, " + _d + str(edges[2].Length) + "\n")

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

    if delete:
        FreeCAD.ActiveDocument.removeObject(objs[0].Name)
        FreeCAD.ActiveDocument.removeObject(objs[1].Name)
        _r = translate("draft", "removed original objects")
        FCC.PrintMessage("makeFillet: " + _r + "\n")
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


class CommandFillet(DraftTools.Creator):
    """The Fillet GUI command definition"""

    def __init__(self):
        DraftTools.Creator.__init__(self)
        self.featureName = "Fillet"

    def GetResources(self):
        return {'Pixmap': 'Draft_Fillet.svg',
                'MenuText': QT_TRANSLATE_NOOP("draft", "Fillet"),
                'ToolTip': QT_TRANSLATE_NOOP("draft", "Creates a fillet between two wires or edges.")
                }

    def Activated(self, name=translate("draft", "Fillet")):
        DraftTools.Creator.Activated(self, name)
        if not self.doc:
            FCC.PrintWarning(translate("draft", "No active document") + "\n")
            return
        if self.ui:
            self.rad = 100
            self.chamfer = False
            self.delete = False
            label = translate("draft", "Fillet radius")
            tooltip = translate("draft", "Radius of fillet")

            # Call the Task panel for a radius
            # The graphical widgets are defined in DraftGui
            self.ui.taskUi(title=name, icon="Draft_Fillet")
            self.ui.radiusUi()
            self.ui.sourceCmd = self
            self.ui.labelRadius.setText(label)
            self.ui.radiusValue.setToolTip(tooltip)
            self.ui.setRadiusValue(self.rad, "Length")
            self.ui.check_delete = self.ui._checkbox("isdelete",
                                                     self.ui.layout,
                                                     checked=self.delete)
            self.ui.check_delete.setText(translate("draft",
                                                   "Delete original objects"))
            self.ui.check_delete.show()
            self.ui.check_chamfer = self.ui._checkbox("ischamfer",
                                                      self.ui.layout,
                                                      checked=self.chamfer)
            self.ui.check_chamfer.setText(translate("draft",
                                                    "Create chamfer"))
            self.ui.check_chamfer.show()

            QtCore.QObject.connect(self.ui.check_delete,
                                   QtCore.SIGNAL("stateChanged(int)"),
                                   self.set_delete)
            QtCore.QObject.connect(self.ui.check_chamfer,
                                   QtCore.SIGNAL("stateChanged(int)"),
                                   self.set_chamfer)
            self.linetrack = trackers.lineTracker(dotted=True)
            self.arctrack = trackers.arcTracker()
            # self.call = self.view.addEventCallback("SoEvent", self.action)
            FCC.PrintMessage(translate("draft", "Enter radius") + "\n")

    def action(self, arg):
        """Scene event handler. CURRENTLY NOT USED.

        Here the displaying of the trackers (previews)
        should be implemented by considering the current value of the
        `ui.radiusValue`.
        """
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point, ctrlPoint, info = DraftTools.getPoint(self, arg)
            DraftTools.redraw3DView()

    def set_delete(self):
        """This function is called when the delete checkbox changes"""
        self.delete = self.ui.check_delete.isChecked()
        FCC.PrintMessage(translate("draft", "Delete original objects: ")
                         + str(self.delete) + "\n")

    def set_chamfer(self):
        """This function is called when the chamfer checkbox changes"""
        self.chamfer = self.ui.check_chamfer.isChecked()
        FCC.PrintMessage(translate("draft", "Chamfer mode: ")
                         + str(self.chamfer) + "\n")

    def numericRadius(self, rad):
        """This function is called when a valid radius is entered"""
        self.rad = rad
        self.draw_arc(rad, self.chamfer, self.delete)
        self.finish()

    def draw_arc(self, rad, chamfer, delete):
        """Processes the selection and draws the actual object"""
        wires = FreeCADGui.Selection.getSelection()
        _two = translate("draft", "two elements needed")
        if not wires:
            FCC.PrintError("CommandFillet: " + _two + "\n")
            return
        if len(wires) != 2:
            FCC.PrintError("CommandFillet: " + _two + "\n")
            return

        for o in wires:
            FCC.PrintMessage("CommandFillet: " + Draft.getType(o) + "\n")

        _test = translate("draft", "Test object")
        _test_off = translate("draft", "Test object removed")
        _cant = translate("draft", "fillet cannot be created")

        FCC.PrintMessage(4*"=" + _test + "\n")
        arc = makeFillet(wires, rad)
        if not arc:
            FCC.PrintError("CommandFillet: " + _cant + "\n")
            return
        self.doc.removeObject(arc.Name)
        FCC.PrintMessage(4*"=" + _test_off + "\n")

        doc = 'FreeCAD.ActiveDocument.'
        _wires = '[' + doc + wires[0].Name + ', ' + doc + wires[1].Name + ']'

        FreeCADGui.addModule("DraftFillet")
        name = translate("draft", "Create fillet")

        args = _wires + ', radius=' + str(rad)
        if chamfer:
            args += ', chamfer=' + str(chamfer)
        if delete:
            args += ', delete=' + str(delete)
        func = ['arc = DraftFillet.makeFillet(' + args + ')']
        func.append('Draft.autogroup(arc)')

        # Here we could remove the old objects, but the makeFillet()
        # command already includes an option to remove them.
        # Therefore, the following is not necessary
        # rems = [doc + 'removeObject("' + o.Name + '")' for o in wires]
        # func.extend(rems)
        func.append('FreeCAD.ActiveDocument.recompute()')
        self.commit(name, func)

    def finish(self, close=False):
        """Terminates the operation."""
        DraftTools.Creator.finish(self)
        if self.ui:
            self.linetrack.finalize()
            self.arctrack.finalize()
            self.doc.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Draft_Fillet', CommandFillet())
