# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Part
import Path

import Path.Dressup.Utils as PathDressup

from PathScripts.PathUtils import loopdetect
from PathScripts.PathUtils import wiredetect
from PathScripts.PathUtils import wiresdetect
from PathScripts.PathUtils import horizontalEdgeLoop
from PathScripts.PathUtils import tangentEdgeLoop
from PathScripts.PathUtils import horizontalFaceLoop
from PathScripts.PathUtils import addToJob
from PathScripts.PathUtils import findParentJob

from PySide.QtCore import QT_TRANSLATE_NOOP

if FreeCAD.GuiUp:
    import FreeCADGui

translate = FreeCAD.Qt.translate

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
                "CAM_SelectLoop",
                "Completes the selection of edges or faces that form a loop"
                "\n\nSelect faces: searching loop faces which form the walls."
                "\n\nSelect one edge: searching loop edges in horizontal plane"
                "\nor wire which contain selected edge."
                "\n\nSelect two edges: searching loop edges in wires of the shape"
                "\nor tangent edges."
                "\n\nSelect three or more edges: searching horizontal wires",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelectionEx()
        if not selection:
            return False

        if not selection[0].SubObjects:
            return False

        return True

    def Activated(self):
        selection = FreeCADGui.Selection.getSelectionEx()
        if not selection:
            return

        sel = selection[0]
        if not sel.SubObjects:
            return

        obj = sel.Object
        subs = sel.SubObjects
        subNames = sel.SubElementNames
        edges = None
        names = None

        if isinstance(subs[0], Part.Face):
            # face(s) selected
            if all(Path.Geom.isVertical(face) for face in subs):
                names = horizontalFaceLoop(obj, subs, subNames)

        elif isinstance(subs[0], Part.Edge):
            if len(subs) == 1:
                # one edge selected: searching horizontal edge loop
                edges = horizontalEdgeLoop(obj, subs[0])
            elif len(subs) == 2:
                # two edges selected: searching wire in shape which contain both edges
                edges = loopdetect(obj, subs[0], subs[1])
                if not edges:
                    # two edges selected: searching edges in tangency
                    edges = tangentEdgeLoop(obj, subs[0])
            else:
                # more than two edges selected: searching horizontal wires which contain edges
                edges = wiresdetect(obj, subs)

            if not edges:
                # searching any wire with first selected edge
                edges = wiredetect(obj, subs[0])

        if edges and not names:
            hashList = [e.hashCode() for e in edges]
            objEdges = obj.Shape.Edges
            names = [f"Edge{i + 1}" for i, e in enumerate(objEdges) if e.hashCode() in hashList]

        if names:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(obj, names)
            return

        Path.Log.warning(translate("CAM_SelectLoop", "Closed loop detection failed."))


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
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False

        if len(selection) == 1:
            # allows to toggle all operations in Job
            sel = selection[0]
            if hasattr(sel, "Group") and sel.Name.startswith("Job"):
                return True
            if hasattr(sel, "Group") and sel.Name.startswith("Operations"):
                return True

        for sel in selection:
            baseOp = Path.Dressup.Utils.baseOp(sel)
            if not hasattr(baseOp, "Active"):
                return False

        return True

    def Activated(self):
        selection = FreeCADGui.Selection.getSelection()
        if (len(selection) == 1 and hasattr(selection[0], "Group")) and (
            selection[0].Name.startswith("Job") or selection[0].Name.startswith("Operations")
        ):
            sel = selection[0]
            # process all Operations in Job
            if sel.Name.startswith("Job"):
                selection = sel.Operations.Group
            elif sel.Name.startswith("Operations"):
                selection = sel.Group

            states = [Path.Dressup.Utils.baseOp(sel).Active for sel in selection]
            if all(states) or not any(states):
                # all operations in one state (active or inactive) - toggle state
                for sel in selection:
                    baseOp = Path.Dressup.Utils.baseOp(sel)
                    baseOp.Active = not baseOp.Active
            else:
                # operations in different states - set Active state
                for sel in selection:
                    baseOp = Path.Dressup.Utils.baseOp(sel)
                    baseOp.Active = True

        else:
            for sel in selection:
                baseOp = Path.Dressup.Utils.baseOp(sel)
                baseOp.Active = not baseOp.Active

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
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False
        if any([not hasattr(sel, "Path") for sel in selection]):
            return False
        if any([sel.Name.startswith("Job") for sel in selection]):
            return False

        return True

    def Activated(self):
        selection = FreeCADGui.Selection.getSelection()
        for sel in selection:
            job = findParentJob(sel)
            prevOp = PathDressup.baseOp(sel)
            prevOpCopy = FreeCAD.ActiveDocument.copyObject(prevOp, False)
            while prevOp != sel:
                # recursive processing Dressup
                op = sel
                while op.Base != prevOp:
                    # get higher level operation
                    op = op.Base
                opCopy = FreeCAD.ActiveDocument.copyObject(op, False)
                opCopy.Base = prevOpCopy
                prevOpCopy = opCopy
                prevOp = op

            # add to Job top object
            addToJob(prevOpCopy, job.Name)


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
