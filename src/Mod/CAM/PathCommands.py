# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import PathScripts
import traceback

from PathScripts.PathUtils import loopdetect
from PathScripts.PathUtils import horizontalEdgeLoop
from PathScripts.PathUtils import horizontalFaceLoop
from PathScripts.PathUtils import addToJob
from PathScripts.PathUtils import findParentJob

from PySide.QtCore import QT_TRANSLATE_NOOP

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore
    from PySide import QtGui

# translate = FreeCAD.Qt.translate

__title__ = "FreeCAD Path Commands"
__author__ = "sliptonic"
__url__ = "https://www.freecad.org"


class _CommandSelectLoop:
    "the Path command to complete loop selection definition"

    def __init__(self):
        self.obj = None
        self.sub = []
        self.active = False

    def GetResources(self):
        return {
            "Pixmap": "CAM_SelectLoop",
            "MenuText": QT_TRANSLATE_NOOP("CAM_SelectLoop", "Finish Selecting Loop"),
            "Accel": "P, L",
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_SelectLoop", "Completes the selection of edges that form a loop"
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            sel = FreeCADGui.Selection.getSelectionEx()[0]
            if sel.Object == self.obj and sel.SubElementNames == self.sub:
                return self.active
            self.obj = sel.Object
            self.sub = sel.SubElementNames
            if sel.SubObjects:
                # self.active = self.formsPartOfALoop(sel.Object, sel.SubObjects[0], sel.SubElementNames)
                self.active = True
            else:
                self.active = False
            return self.active
        except Exception as exc:
            Path.Log.error(exc)
            traceback.print_exc(exc)
            return False

    def Activated(self):
        from PathScripts.PathUtils import horizontalEdgeLoop, horizontalFaceLoop

        if not FreeCADGui.Selection.getSelectionEx():
            return

        sel = FreeCADGui.Selection.getSelectionEx()[0]
        if not sel.SubObjects:
            return

        obj = sel.Object
        sub = sel.SubObjects
        names = sel.SubElementNames
        loopwire = None

        # Face selection
        if "Face" in names[0]:
            loop = horizontalFaceLoop(obj, sub[0], names)
            if loop:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(obj, loop)
                return

        # One edge selected
        elif len(sub) == 1:
            loopwire = horizontalEdgeLoop(obj, sub[0])

        # Two edges selected
        elif len(sub) >= 2:
            loopwire = loopdetect(obj, sub[0], sub[1])

        if hasattr(loopwire, "Edges") and loopwire.Edges:
            elist = obj.Shape.Edges
            FreeCADGui.Selection.clearSelection()
            for i in loopwire.Edges:
                for e in elist:
                    if e.hashCode() == i.hashCode():
                        FreeCADGui.Selection.addSelection(obj, f"Edge{elist.index(e) + 1}")
            return

        # Final fallback
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.information(
                None,
                QT_TRANSLATE_NOOP("CAM_SelectLoop", "Feature Completion"),
                QT_TRANSLATE_NOOP("CAM_SelectLoop", "Closed loop detection failed."),
            )

    def formsPartOfALoop(self, obj, sub, names):
        try:
            if names[0][0:4] != "Edge":
                if names[0][0:4] == "Face" and horizontalFaceLoop(obj, sub, names):
                    return True
                return False
            if len(names) == 1 and horizontalEdgeLoop(obj, sub):
                return True
            if len(names) == 1 or names[1][0:4] != "Edge":
                return False
            return True
        except Exception:
            return False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_SelectLoop", _CommandSelectLoop())


class _ToggleOperation:
    "command definition to toggle Operation Active state"

    def GetResources(self):
        return {
            "Pixmap": "CAM_OpActive",
            "MenuText": QT_TRANSLATE_NOOP("CAM_OpActiveToggle", "Toggle Operation"),
            "Accel": "P, X",
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_OpActiveToggle", "Toggles the active state of the operation"
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            for sel in FreeCADGui.Selection.getSelectionEx():
                selProxy = Path.Dressup.Utils.baseOp(sel.Object).Proxy
                if not isinstance(selProxy, Path.Op.Base.ObjectOp) and not isinstance(
                    selProxy, Path.Op.Gui.Array.ObjectArray
                ):
                    return False
            return True
        except (IndexError, AttributeError):
            return False

    def Activated(self):
        for sel in FreeCADGui.Selection.getSelectionEx():
            op = Path.Dressup.Utils.baseOp(sel.Object)
            op.Active = not op.Active
            op.ViewObject.Visibility = op.Active

        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_OpActiveToggle", _ToggleOperation())


class _CopyOperation:
    "the Path Copy Operation command definition"

    def GetResources(self):
        return {
            "Pixmap": "CAM_OpCopy",
            "MenuText": QT_TRANSLATE_NOOP("CAM_OperationCopy", "Copy Operation"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_OperationCopy", "Copies the operation in the job"),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            for sel in FreeCADGui.Selection.getSelectionEx():
                if not isinstance(sel.Object.Proxy, Path.Op.Base.ObjectOp):
                    return False
            return True
        except (IndexError, AttributeError):
            return False

    def Activated(self):
        for sel in FreeCADGui.Selection.getSelectionEx():
            jobname = findParentJob(sel.Object).Name
            addToJob(FreeCAD.ActiveDocument.copyObject(sel.Object, False), jobname)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_OperationCopy", _CopyOperation())


# \c findShape() is referenced from Gui/Command.cpp and used by Path.Area commands.
# Do not remove!
def findShape(shape, subname=None, subtype=None):
    """To find a higher order shape containing the subshape with subname.
    E.g. to find the wire containing 'Edge1' in shape,
        findShape(shape,'Edge1','Wires')
    """
    if not subname:
        return shape
    ret = shape.getElement(subname)
    if not subtype or not ret or ret.isNull():
        return ret
    if subname.startswith("Face"):
        tp = "Faces"
    elif subname.startswith("Edge"):
        tp = "Edges"
    elif subname.startswith("Vertex"):
        tp = "Vertex"
    else:
        return ret
    for obj in getattr(shape, subtype):
        for sobj in getattr(obj, tp):
            if sobj.isEqual(ret):
                return obj
    return ret
