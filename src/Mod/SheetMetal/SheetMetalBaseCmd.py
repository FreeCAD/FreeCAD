########################################################################
#
#  SheetMetalBaseCmd.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os

import FreeCAD
import Part

import SheetMetalTools

translate = FreeCAD.Qt.translate
icons_path = SheetMetalTools.icons_path
panels_path = SheetMetalTools.panels_path

# List of properties to be saved as defaults.
smBaseDefaultVars = ["Radius", "Thickness"]


def modifiedWire(WireList, radius, thk, length, normal, Side, sign):
    # If sketch is one type, make a face by extruding & offset it to
    # correct position.
    wire_extr = WireList.extrude(normal * sign * length)
    # Part.show(wire_extr, "wire_extr")
    if Side == "Inside":
        wire_extr = wire_extr.makeOffsetShape(thk / 2.0 * sign, 0.0, fill=False, join=2)
    elif Side == "Outside":
        wire_extr = wire_extr.makeOffsetShape(-thk / 2.0 * sign, 0.0, fill=False, join=2)
    # Part.show(wire_extr, "wire_extr")
    try:
        filleted_extr = wire_extr.makeFillet((radius + thk / 2.0), wire_extr.Edges)
    except:
        filleted_extr = wire_extr
    # Part.show(filleted_extr, "filleted_extr")
    filleted_extr = filleted_extr.makeOffsetShape(thk / 2.0 * sign, 0.0, fill=False, join=2)
    # Part.show(filleted_extr, "filleted_extr")
    return filleted_extr


def smBase(thk=2.0, length=10.0, radius=1.0, Side="Inside",
           midplane=False, reverse=False, MainObject=None):
    # To Get sketch normal.
    WireList = MainObject.Shape.Wires[0]
    mat = MainObject.getGlobalPlacement().Rotation
    normal = (mat.multVec(FreeCAD.Vector(0, 0, 1))).normalize()
    # print([mat, normal])
    if WireList.isClosed():
        # If Closed sketch is there, make a face & extrude it.
        sketch_face = Part.makeFace(MainObject.Shape.Wires, "Part::FaceMakerBullseye")
        thk = -1.0 * thk if reverse else thk
        wallSolid = sketch_face.extrude(sketch_face.normalAt(0, 0) * thk)
        if midplane:
            wallSolid = Part.Solid(wallSolid.translated(sketch_face.normalAt(0, 0) * thk * -0.5))
    else:
        filleted_extr = modifiedWire(WireList, radius, thk, length, normal, Side, 1.0)
        # Part.show(filleted_extr, "filleted_extr")
        dist = WireList.Vertexes[0].Point.distanceToPlane(FreeCAD.Vector(0, 0, 0), normal)
        # print(dist)
        slice_wire = filleted_extr.slice(normal, dist)
        # print(slice_wire)
        # Part.show(slice_wire[0], "slice_wire")
        traj = slice_wire[0]
        # Part.show(traj, "traj")
        if midplane:
            traj.translate(normal * -length / 2.0)
        elif reverse:
            traj.translate(normal * -length)
        traj_extr = traj.extrude(normal * length)
        # Part.show(traj_extr, "traj_extr")
        solidlist = []
        for face in traj_extr.Faces:
            solid = face.makeOffsetShape(thk, 0.0, fill=True)
            solidlist.append(solid)
        if len(solidlist) > 1:
            wallSolid = solidlist[0].multiFuse(solidlist[1:])
        else:
            wallSolid = solidlist[0]
        # Part.show(wallSolid, "wallSolid")
    # Part.show(wallSolid, "wallSolid")
    return wallSolid


class SMBaseBend:
    def __init__(self, obj, sketch):
        """Add wall or Wall with radius bend."""
        _tip_ = translate("App::Property", "Bend Plane")
        obj.addProperty("App::PropertyEnumeration", "BendSide", "Parameters", _tip_).BendSide = [
                "Outside", "Inside", "Middle"]
        _tip_ = translate("App::Property", "Wall Sketch object")
        obj.addProperty("App::PropertyLink", "BendSketch", "Parameters", _tip_).BendSketch = sketch
        _tip_ = translate("App::Property", "Extrude Symmetric to Plane")
        self.addVerifyProperties(obj)
        SheetMetalTools.taskRestoreDefaults(obj, smBaseDefaultVars)
        obj.Proxy = self

    def addVerifyProperties(self, obj):
        SheetMetalTools.smAddLengthProperty(obj,
            "Radius",
            translate("App::Property", "Bend Radius"),
            1.0)
        SheetMetalTools.smAddLengthProperty(obj,
            "Thickness",
            translate("App::Property", "Thickness of sheetmetal"),
            1.0)
        SheetMetalTools.smAddLengthProperty(obj,
            "Length",
            translate("App::Property", "Length of wall"),
            100.0)
        SheetMetalTools.smAddBoolProperty(obj,
            "MidPlane",
            FreeCAD.Qt.translate("App::Property", "Extrude Symmetric to Plane"),
            False)
        SheetMetalTools.smAddBoolProperty(obj,
            "Reverse",
            FreeCAD.Qt.translate("App::Property", "Reverse Extrusion Direction"),
            False)

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)
        fp.Shape = smBase(thk=fp.Thickness.Value,
                          length=fp.Length.Value,
                          radius=fp.Radius.Value,
                          Side=fp.BendSide,
                          midplane=fp.MidPlane,
                          reverse=fp.Reverse,
                          MainObject=fp.BendSketch)


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    Gui = FreeCAD.Gui


    ###############################################################################################
    # View Provider
    ###############################################################################################

    class SMBaseViewProvider(SheetMetalTools.SMViewProvider):
        """Part / Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddBase.svg")

        def claimChildren(self):
            objs = []
            if hasattr(self, "Object") and hasattr(self.Object, "BendSketch"):
                objs.append(self.Object.BendSketch)
            return objs

        def getTaskPanel(self, obj):
            return SMBaseBendTaskPanel(obj)


    ###############################################################################################
    # Task Panel
    ###############################################################################################

    class SMBaseBendTaskPanel:
        """A TaskPanel for the SheetMetal base bend command."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("CreateBaseShape.ui")
            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.updateDisplay()

            SheetMetalTools.taskConnectSelectionSingle(self.form.pushSketch, self.form.txtSketch,
                                                       obj, "BendSketch",
                                                       ("Sketcher::SketchObject", []))
            SheetMetalTools.taskConnectSpin(obj, self.form.spinRadius, "Radius")
            SheetMetalTools.taskConnectSpin(obj, self.form.spinThickness, "Thickness")
            SheetMetalTools.taskConnectSpin(obj, self.form.spinLength, "Length")
            SheetMetalTools.taskConnectEnum(obj, self.form.comboBendPlane, "BendSide")
            SheetMetalTools.taskConnectCheck(obj, self.form.checkSymetric, "MidPlane",
                                             self.midplaneChanged)
            SheetMetalTools.taskConnectCheck(obj, self.form.checkRevDirection, "Reverse")
            obj.BendSketch.Visibility = True

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def updateDisplay(self):
            self.form.checkRevDirection.setVisible(self.obj.MidPlane is False)

        def midplaneChanged(self, value):
            self.updateDisplay()

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smBaseDefaultVars)
            self.obj.BendSketch.Visibility = False
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)

        # def retranslateUi(self, SMBendTaskPanel):


    ###############################################################################################
    # Command
    ###############################################################################################

    class AddBaseCommandClass:
        """Add Base Wall command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddBase.svg"),
                    "MenuText": translate("SheetMetal", "Make Base Wall"),
                    "Accel": "C, B",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Create a sheetmetal wall from a sketch\n"
                        "1. Select a Sketch to create bends with walls.\n"
                        "2. Use Property editor to modify other parameters",
                        ),
                    }

        def Activated(self):
            selobj = Gui.Selection.getSelectionEx()[0].Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "BaseBend")
            if newObj is None:
                return
            SMBaseBend(newObj, selobj)
            SMBaseViewProvider(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMBaseBendTaskPanel)
            return

        def IsActive(self):
            if len(Gui.Selection.getSelection()) != 1:
                return False
            selobj = Gui.Selection.getSelection()[0]
            if not (
                selobj.isDerivedFrom("Sketcher::SketchObject")
                or selobj.isDerivedFrom("PartDesign::ShapeBinder")
                or selobj.isDerivedFrom("PartDesign::SubShapeBinder")
                or selobj.isDerivedFrom("Part::Part2DObjectPython")
            ):
                return False
            return True

    Gui.addCommand("SheetMetal_AddBase", AddBaseCommandClass())
