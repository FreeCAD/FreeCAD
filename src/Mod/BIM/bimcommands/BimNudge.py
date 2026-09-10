# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""BIM nudge commands"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


_IMPERIAL_SCHEMAS = {2, 3, 5, 7}
_METRIC_NUDGE_PRESETS = ("1 mm", "5 mm", "1 cm", "5 cm", "10 cm", "50 cm")
_IMPERIAL_NUDGE_PRESETS = ('1/16"', '1/8"', '1/4"', '1"', '6"', "1'")

def get_unit_schema():
    doc = FreeCAD.ActiveDocument
    if doc is None:
        return FreeCAD.Units.getSchema()
    return doc.getEnumerationsOfProperty("UnitSystem").index(doc.UnitSystem)

def get_nudge_presets():
    """Return display labels and quantities for the active unit schema."""

    presets = (
        _IMPERIAL_NUDGE_PRESETS
        if get_unit_schema() in _IMPERIAL_SCHEMAS
        else _METRIC_NUDGE_PRESETS
    )
    return [(label, FreeCAD.Units.Quantity(label)) for label in presets]


class BIM_Nudge:
    # base class for the different nudge commands

    def getNudgeValue(self, mode):
        "mode can be dist, up, down, left, right. dist returns a float in mm, other modes return a 3D vector"

        from PySide import QtGui
        import WorkingPlane

        mw = FreeCADGui.getMainWindow()
        if mw:
            st = mw.statusBar()
            statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
            if statuswidget:
                nudgeValue = statuswidget.nudge.text().replace("&", "")
                dist = 0
                if "auto" in nudgeValue.lower():
                    distances = [quantity for _label, quantity in get_nudge_presets()]
                    if hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph"):
                        viewsize = (
                            FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                            .getViewVolume()
                            .getWidth()
                        )
                    else:
                        viewsize = 4000
                    if viewsize < 250:
                        dist = distances[0].Value
                    elif viewsize < 750:
                        dist = distances[1].Value
                    elif viewsize < 4500:
                        dist = distances[2].Value
                    elif viewsize < 8000:
                        dist = distances[3].Value
                    elif viewsize < 25000:
                        dist = distances[4].Value
                    else:
                        dist = distances[5].Value
                    # u = FreeCAD.Units.Quantity(dist,FreeCAD.Units.Length).UserString
                    statuswidget.nudge.setText(translate("BIM", "Auto"))
                else:
                    try:
                        dist = FreeCAD.Units.Quantity(nudgeValue)
                    except ValueError:
                        try:
                            dist = float(nudgeValue)
                        except ValueError:
                            return None
                    else:
                        dist = dist.Value
                if not dist:
                    return None
                if mode == "dist":
                    return dist
                wp = WorkingPlane.get_working_plane()
                if mode == "up":
                    return FreeCAD.Vector(wp.v).multiply(dist)
                if mode == "down":
                    return FreeCAD.Vector(wp.v).negative().multiply(dist)
                if mode == "right":
                    return FreeCAD.Vector(wp.u).multiply(dist)
                if mode == "left":
                    return FreeCAD.Vector(wp.u).negative().multiply(dist)
        return None

    def toStr(self, objs):
        "builds a string which is a list of objects"

        return "[" + ",".join(["FreeCAD.ActiveDocument." + obj.Name for obj in objs]) + "]"

    def getCenter(self, objs):
        "returns the center point of a group of objects"

        bb = None
        for obj in objs:
            if hasattr(obj, "Shape") and hasattr(obj.Shape, "BoundBox"):
                if not bb:
                    bb = obj.Shape.BoundBox
                else:
                    bb.add(obj.Shape.BoundBox)
        if bb:
            return bb.Center
        else:
            return None


class BIM_Nudge_Switch(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Switch", "Nudge Switch"),
            "Accel": "Alt+/",
        }

    def Activated(self):
        from PySide import QtGui

        mw = FreeCADGui.getMainWindow()
        if mw:
            st = mw.statusBar()
            statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
            if statuswidget:
                nudgeValue = statuswidget.nudge.text()
                nudge = self.getNudgeValue("dist")
                if nudge:
                    u = FreeCAD.Units.Quantity(nudge, FreeCAD.Units.Length).UserString
                    if "auto" in nudgeValue.lower():
                        statuswidget.nudge.setText(u)
                    else:
                        statuswidget.nudge.setText(translate("BIM", "Auto"))


class BIM_Nudge_Up(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Up", "Nudge Up"),
            "Accel": "Alt+'",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("up")
            if nudge:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.move(" + self.toStr(sel) + ",FreeCAD." + str(nudge) + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_Down(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Down", "Nudge Down"),
            "Accel": "Alt+;",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("down")
            if nudge:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.move(" + self.toStr(sel) + ",FreeCAD." + str(nudge) + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_Left(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Left", "Nudge Left"),
            "Accel": "Alt+[",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("left")
            if nudge:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.move(" + self.toStr(sel) + ",FreeCAD." + str(nudge) + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_Right(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Right", "Nudge Right"),
            "Accel": "Alt+]",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("right")
            if nudge:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.move(" + self.toStr(sel) + ",FreeCAD." + str(nudge) + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_Extend(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Extend", "Nudge Extend"),
            "Accel": "Alt+PgUp",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("dist")
            if nudge:
                for obj in sel:
                    if hasattr(obj, "Height"):
                        FreeCADGui.doCommand(
                            "FreeCAD.ActiveDocument."
                            + obj.Name
                            + ".Height="
                            + str(obj.Height.Value + nudge)
                        )
                        FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_Shrink(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_Shrink", "Nudge Shrink"),
            "Accel": "Alt+PgDown",
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            nudge = self.getNudgeValue("dist")
            if nudge:
                for obj in sel:
                    if hasattr(obj, "Height"):
                        FreeCADGui.doCommand(
                            "FreeCAD.ActiveDocument."
                            + obj.Name
                            + ".Height="
                            + str(obj.Height.Value - nudge)
                        )
                        FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_RotateLeft(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_RotateLeft", "Nudge Rotate Left"),
            "Accel": "Alt+,",
        }

    def Activated(self):

        import WorkingPlane

        sel = FreeCADGui.Selection.getSelection()
        if sel:
            center = self.getCenter(sel)
            if center:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.rotate("
                    + self.toStr(sel)
                    + ",45,FreeCAD."
                    + str(center)
                    + ",FreeCAD."
                    + str(WorkingPlane.get_working_plane().axis)
                    + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


class BIM_Nudge_RotateRight(BIM_Nudge):

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("BIM_Nudge_RotateRight", "Nudge Rotate Right"),
            "Accel": "Alt+.",
        }

    def Activated(self):

        import WorkingPlane

        sel = FreeCADGui.Selection.getSelection()
        if sel:
            center = self.getCenter(sel)
            if center:
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand(
                    "Draft.rotate("
                    + self.toStr(sel)
                    + ",-45,FreeCAD."
                    + str(center)
                    + ",FreeCAD."
                    + str(WorkingPlane.get_working_plane().axis)
                    + ")"
                )
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")


FreeCADGui.addCommand("BIM_Nudge_Switch", BIM_Nudge_Switch())
FreeCADGui.addCommand("BIM_Nudge_Up", BIM_Nudge_Up())
FreeCADGui.addCommand("BIM_Nudge_Down", BIM_Nudge_Down())
FreeCADGui.addCommand("BIM_Nudge_Left", BIM_Nudge_Left())
FreeCADGui.addCommand("BIM_Nudge_Right", BIM_Nudge_Right())
FreeCADGui.addCommand("BIM_Nudge_Extend", BIM_Nudge_Extend())
FreeCADGui.addCommand("BIM_Nudge_Shrink", BIM_Nudge_Shrink())
FreeCADGui.addCommand("BIM_Nudge_RotateLeft", BIM_Nudge_RotateLeft())
FreeCADGui.addCommand("BIM_Nudge_RotateRight", BIM_Nudge_RotateRight())
