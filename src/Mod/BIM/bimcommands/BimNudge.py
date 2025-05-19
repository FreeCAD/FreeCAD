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
                    unit = FreeCAD.ParamGet(
                        "User parameter:BaseApp/Preferences/Units"
                    ).GetInt("UserSchema", 0)
                    if unit in [2, 3, 5, 7]:
                        scale = [1.5875, 3.175, 6.35, 25.4, 152.4, 304.8]
                    else:
                        scale = [1, 5, 10, 50, 100, 500]
                    viewsize = (
                        FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                        .getViewVolume()
                        .getWidth()
                    )
                    if viewsize < 250:
                        dist = scale[0]
                    elif viewsize < 750:
                        dist = scale[1]
                    elif viewsize < 4500:
                        dist = scale[2]
                    elif viewsize < 8000:
                        dist = scale[3]
                    elif viewsize < 25000:
                        dist = scale[4]
                    else:
                        dist = scale[5]
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

        return (
            "[" + ",".join(["FreeCAD.ActiveDocument." + obj.Name for obj in objs]) + "]"
        )

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
            "Accel": "Ctrl+/",
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
            "Accel": "Ctrl+Up",
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
            "Accel": "Ctrl+Down",
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
            "Accel": "Ctrl+Left",
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
            "Accel": "Ctrl+Right",
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
            "Accel": "Ctrl+PgUp",
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
            "Accel": "Ctrl+PgDown",
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
            "Accel": "Ctrl+,",
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
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_Nudge_RotateRight", "Nudge Rotate Right"
            ),
            "Accel": "Ctrl+.",
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
