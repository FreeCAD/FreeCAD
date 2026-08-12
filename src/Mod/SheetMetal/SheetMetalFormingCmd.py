########################################################################
#
#  SheetMetalFormingCmd.py
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

import math
import os

import FreeCAD
import Part

import SheetMetalTools

SMException = SheetMetalTools.SMException
smEpsilon = SheetMetalTools.smEpsilon
translate = FreeCAD.Qt.translate


def angleBetween(ve1, ve2):
    """Find angle between two vectors in degrees."""
    return math.degrees(ve1.getAngle(ve2))


def face_direction(face):
    yL = face.CenterOfMass
    uv = face.Surface.parameter(yL)
    nv = face.normalAt(uv[0], uv[1])
    direction = yL.sub(nv + yL)
    # print([direction, yL])
    return direction, yL


def transform_tool(tool, base_face, tool_face, point=FreeCAD.Vector(0, 0, 0), angle=0.0):
    # Find normal of faces & center to align faces.
    direction1, yL1 = face_direction(base_face)
    direction2, yL2 = face_direction(tool_face)
    # Find angle between faces, axis of rotation & center of axis.
    rot_angle = angleBetween(direction1, direction2)
    rot_axis = direction1.cross(direction2)
    if rot_axis.isEqual(FreeCAD.Vector(0.0, 0.0, 0.0), 0.001):
        rot_axis = FreeCAD.Vector(0, 1, 0).cross(direction2)
    rot_center = yL2
    # print([rot_center, rot_axis, rot_angle])
    if not rot_axis.isEqual(FreeCAD.Vector(0.0, 0.0, 0.0), 0.001):
        tool.rotate(rot_center, rot_axis, -rot_angle)
    tool.translate(-yL2 + yL1)
    # Part.show(tool, "tool")
    tool.rotate(yL1, direction1, angle)
    tool.translate(point)
    # Part.show(tool, "tool")
    return tool


def combine_solids(base, cut_tool, form_tool):
    form_tool = form_tool.cut(base)
    base = base.cut(cut_tool)
    return base.fuse(form_tool)


def makeforming(tool, base, base_face, thk, tool_faces=None,
                point=FreeCAD.Vector(0, 0, 0), angle=0.0):
    # Create a shell from all faces but the selected ones.
    cutSolid = tool.copy()
    cutSolid_tran = transform_tool(cutSolid, base_face, tool_faces[0], point, angle)
    base = base.copy()
    try:
        faces = []
        for face in tool.Faces:
            use_tool = True
            for selface in tool_faces:
                if face.isSame(selface):
                    use_tool = False
                    break
            if use_tool:
                faces.append(face)
        tool_shell = Part.makeShell(faces)
        offsetshell = tool_shell.makeOffsetShape(thk, 0.0, inter=False, self_inter=False,
                                                 offsetMode=0, join=2, fill=True)
        offsetshell_tran = transform_tool(offsetshell, base_face, tool_faces[0], point, angle)
        base = combine_solids(base, cutSolid_tran, offsetshell_tran)
    except:
        FreeCAD.Console.PrintWarning("Forming faild. Trying alternate way.")
        offsetshell = tool.makeThickness(tool_faces, thk, 0.0001, False, False, 0, 0)
        offsetshell_tran = transform_tool(offsetshell, base_face, tool_faces[0], point, angle)
        base = combine_solids(base, cutSolid_tran, offsetshell_tran)
    # Part.show(base, "base")
    return base


class SMBendWall:
    def __init__(self, obj, selobj, selobj_items, seltool, seltool_items):
        """Add Forming Wall."""
        _tip_ = translate("App::Property", "Suppress Forming Feature")
        obj.addProperty("App::PropertyBool", "SuppressFeature",
                        "Parameters", _tip_).SuppressFeature = False
        _tip_ = translate("App::Property", "Tool Position Angle")
        obj.addProperty("App::PropertyAngle", "angle", "Parameters", _tip_).angle = 0.0
        _tip_ = translate(
            "App::Property", "Thickness of Sheetmetal")
        obj.addProperty("App::PropertyDistance", "thickness", "Parameters", _tip_)
        _tip_ = translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters",
                        _tip_).baseObject = (selobj, selobj_items)
        _tip_ = translate("App::Property", "Forming Tool Object")
        obj.addProperty("App::PropertyLinkSub", "toolObject", "Parameters",
                        _tip_).toolObject = (seltool, seltool_items)
        _tip_ = translate(
            "App::Property",
            "Sketch containing points to locate multiple instances of the embossed feature")
        obj.addProperty("App::PropertyLink", "Sketch", "Parameters1", _tip_)
        # Add other properties (is necessary this way to not cause
        # errors on old files).
        self.addVerifyProperties(obj)
        obj.Proxy = self
        self.count = 0

    def addVerifyProperties(self, obj, seltool=None, seltool_items=None):
        SheetMetalTools.smAddProperty(obj,
            "App::PropertyLinkSub",
            "toolShearFaces",
            translate("SheetMetal", "Tool shear faces"),
            None)

        SheetMetalTools.smAddDistanceProperty(obj,
            "OffsetX",
            translate("App::Property", "X Offset from Center of Face"),
            0.0)
        SheetMetalTools.smAddDistanceProperty(obj,
            "OffsetY",
            translate("App::Property", "Y Offset from Center of Face"),
            0.0)
        if hasattr(obj, "offset"):
            obj.OffsetX = obj.offset.x
            obj.OffsetY = obj.offset.y
            obj.removeProperty("offset")

        seltool, seltool_items = obj.toolObject
        if len(seltool_items) > 1:
            obj.toolObject = (seltool, seltool_items[0])
            obj.toolShearFaces = (seltool, seltool_items[1:])
        elif obj.toolShearFaces is None:
            obj.toolShearFaces = (seltool, [])

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)
        base = fp.baseObject[0].Shape
        base_face = base.getElement(SheetMetalTools.getElementFromTNP(fp.baseObject[1][0]))
        thk = SheetMetalTools.smGetThickness(base, base_face)
        fp.thickness = thk
        tool = fp.toolObject[0].Shape
        tool_faces = [fp.toolObject[1][0]]
        if fp.toolShearFaces:
            tool_faces += fp.toolShearFaces[1]
        tool_faces = [
            tool.getElement(SheetMetalTools.getElementFromTNP(face)) for face in tool_faces
            ]
        offsetlist = []
        if fp.Sketch:
            pt1 = base_face.CenterOfMass
            sketch = fp.Sketch.Shape
            for e in sketch.Edges:
                # print(type(e.Curve))
                if isinstance(e.Curve, (Part.Circle, Part.ArcOfCircle)):
                    pt2 = e.Curve.Center
                    offsetPoint = pt2 - pt1
                    #print(offsetPoint, pt2, pt1)
                    offsetlist.append(offsetPoint)

            for v in sketch.Vertexes:
                # print(type(e.Curve))
                if len(sketch.ancestorsOfType(v, Part.Edge)) == 0:
                    pt2 = v.Point
                    offsetPoint = pt2 - pt1
                    #print(offsetPoint, pt2, pt1)
                    offsetlist.append(offsetPoint)
            if len(offsetlist) == 0:
                error = translate("SheetMetal", 
                                  "No points/circles found in sketch to place forming feature.")
                raise SMException(error)
        else:
            offsetlist.append(FreeCAD.Vector(fp.OffsetX, fp.OffsetY, 0))

        if not fp.SuppressFeature:
            for offset in offsetlist:
                a = makeforming(tool, base, base_face, thk, tool_faces, offset, fp.angle.Value)
                # Part.show(a)
                base = a
        else:
            a = base
        fp.Shape = a
        SheetMetalTools.smHideObjects(fp.baseObject[0], fp.toolObject[0], fp.Sketch)


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    from PySide import QtCore, QtGui

    Gui = FreeCAD.Gui

    icons_path = SheetMetalTools.icons_path


    ###############################################################################################
    # View providers
    ###############################################################################################

    class SMFormingVP(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddBend.svg")

        def getTaskPanel(self, obj):
            return SMFormingWallTaskPanel(obj)

        def claimChildren(self):
            objs = []
            if not SheetMetalTools.smIsPartDesign(self.Object) and hasattr(self.Object,
                                                                           "baseObject"):
                objs.append(self.Object.baseObject[0])
            if hasattr(self.Object, "toolObject"):
                objs.append(self.Object.toolObject[0])
            if hasattr(self.Object, "Sketch"):
                objs.append(self.Object.Sketch)
            return objs


    class SMFormingPDVP(SMFormingVP):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    ###############################################################################################
    # Task Panel
    ###############################################################################################

    class SMFormingWallTaskPanel:
        """A TaskPanel for the SheetMetal."""

        def __init__(self, obj):
            QtCore.QDir.addSearchPath("Icons", SheetMetalTools.icons_path)
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("StampPanel.ui")
            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.sheerSelParams = SheetMetalTools.taskConnectSelection(
                self.form.pushSelShear, self.form.treeShear, self.obj, ["Face"],
                self.form.pushClearShear, "toolShearFaces", False
            )
            if obj.toolObject is not None:
                self.sheerSelParams.ConstrainToObject = obj.toolObject[0]
            self.sheerSelParams.AllowZeroSelection = True
            self.toolSelParams = SheetMetalTools.taskConnectSelectionSingle(self.form.pushSelTool,
                self.form.txtSelectedTool, self.obj, "toolObject", ["Face"])
            self.toolSelParams.ValueChangedCallback = self.toolChanged
            self.targetSelParams = SheetMetalTools.taskConnectSelectionSingle(
                self.form.pushSelFace, self.form.txtSelectedFace, self.obj, "baseObject", ["Face"])
            self.sketchSelParams = SheetMetalTools.taskConnectSelectionToggle(
                self.form.pushSelSketch, self.form.txtSketch, self.obj, "Sketch", ("Sketch", []))
            self.sketchSelParams.setVisibilityControlledWidgets([],
                                                                [(self.form.unitOffsetY, False),
                                                                 (self.form.unitOffsetX, False)])
            SheetMetalTools.taskConnectSpin(obj, self.form.unitOffsetX, "OffsetX")
            SheetMetalTools.taskConnectSpin(obj, self.form.unitOffsetY, "OffsetY")
            SheetMetalTools.taskConnectSpin(obj, self.form.unitAngle, "angle")

        def toolChanged(self, _sp, selobj, _selobj_items):
            if self.obj.toolShearFaces is None or self.obj.toolShearFaces[0] is not selobj:
                self.obj.toolShearFaces = (selobj, [])
                SheetMetalTools.taskPopulateSelectionList(self.form.treeShear,
                                                          self.obj.toolShearFaces)
                self.sheerSelParams.ConstrainToObject = selobj

        def accept(self):
            SheetMetalTools.taskAccept(self)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddFormingWallCommand:
        """Add Forming Wall command."""

        def GetResources(self):
            return {"Pixmap": os.path.join(icons_path, "SheetMetal_Forming.svg"),
                    "MenuText": translate("SheetMetal", "Make Forming in Wall"),
                    "Accel": "M, F",
                    "ToolTip": translate(
                        "SheetMetal", "Make a forming using tool in metal sheet\n"
                        "1. Select a flat face on sheet metal and\n"
                        "2. Select face(s) on forming tool Shape to create Formed sheetmetal.\n"
                        "3. Use Suppress in Property editor to disable during unfolding\n"
                        "4. Use Property editor to modify other parameters"
                        )
                    }

        def Activated(self):
            # doc = FreeCAD.ActiveDocument
            # view = Gui.ActiveDocument.ActiveView
            # activeBody = None
            sel = Gui.Selection.getSelectionEx()
            selobj = Gui.Selection.getSelectionEx()[0].Object
            # viewConf = SheetMetalTools.GetViewConfig(selobj)
            # if hasattr(view, "getActiveObject"):
            #     activeBody = view.getActiveObject("pdbody")
            # if not SheetMetalTools.smIsOperationLegal(activeBody, selobj):
            #     return
            # doc.openTransaction("WallForming")
            # if activeBody is None or not SheetMetalTools.smIsPartDesign(selobj):
            #     a = doc.addObject("Part::FeaturePython", "WallForming")
            #     SMBendWall(a, selobj, sel[0].SubElementNames, sel[1].Object,
            #                sel[1].SubElementNames)
            #     SMFormingVP(a.ViewObject)
            # else:
            #     # FreeCAD.Console.PrintLog("found active body: " + activeBody.Name)
            #     a = doc.addObject("PartDesign::FeaturePython", "WallForming")
            #     SMBendWall(a, selobj, sel[0].SubElementNames, sel[1].Object,
            #                sel[1].SubElementNames)
            #     SMFormingPDVP(a.ViewObject)
            #     activeBody.addObject(a)
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "WallForming")
            if newObj is None:
                return
            SMBendWall(newObj, selobj, sel[0].SubElementNames, sel[1].Object,
                       sel[1].SubElementNames)
            SMFormingVP(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMFormingWallTaskPanel)
            # SheetMetalTools.SetViewConfig(a, viewConf)
            # doc.recompute()
            # doc.commitTransaction()
            return

        def IsActive(self):
            if (len(Gui.Selection.getSelection()) < 2
                    or len(Gui.Selection.getSelectionEx()[0].SubElementNames) < 1):
                return False
            selobj = Gui.Selection.getSelection()[0]
            if str(type(selobj)) == "<type 'Sketcher.SketchObject'>":
                return False
            for selFace in Gui.Selection.getSelectionEx()[0].SubObjects:
                if type(selFace) != Part.Face:
                    return False
            return True


    Gui.addCommand("SheetMetal_Forming", AddFormingWallCommand())
