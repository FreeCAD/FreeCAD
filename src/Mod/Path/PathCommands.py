# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts
import PathScripts.PathLog as PathLog
import traceback

from PathScripts.PathUtils import loopdetect
from PathScripts.PathUtils import horizontalEdgeLoop
from PathScripts.PathUtils import horizontalFaceLoop
from PathScripts.PathUtils import addToJob
from PathScripts.PathUtils import findParentJob

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore
    from DraftTools import translate
else:
    def translate(ctxt, txt):
        return txt

__title__="FreeCAD Path Commands"
__author__ = "sliptonic"
__url__ = "http://www.freecadweb.org"


class _CommandSelectLoop:
    "the Path command to complete loop selection definition"
    def __init__(self):
        self.obj = None
        self.sub = []
        self.active = False

    def GetResources(self):
        return {'Pixmap': 'Path-SelectLoop',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_SelectLoop", "Finish Selecting Loop"),
                'Accel': "P, L",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_SelectLoop", "Complete loop selection from two edges"),
                'CmdType': "ForEdit"}

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
                self.active = self.formsPartOfALoop(sel.Object, sel.SubObjects[0], sel.SubElementNames)
            else:
                self.active = False
            return self.active
        except Exception as exc:
            PathLog.error(exc)
            traceback.print_exc(exc)
            return False

    def Activated(self):
        #from PathScripts.PathUtils import loopdetect
        from PathScripts.PathUtils import horizontalEdgeLoop
        from PathScripts.PathUtils import horizontalFaceLoop
        sel = FreeCADGui.Selection.getSelectionEx()[0]
        obj = sel.Object
        edge1 = sel.SubObjects[0]
        if 'Face' in sel.SubElementNames[0]:
            loop = horizontalFaceLoop(sel.Object, sel.SubObjects[0], sel.SubElementNames)
            if loop:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(sel.Object, loop)
            loopwire = []
        elif len(sel.SubObjects) == 1:
            loopwire = horizontalEdgeLoop(obj, edge1)
        else:
            edge2 = sel.SubObjects[1]
            loopwire = loopdetect(obj, edge1, edge2)

        if loopwire:
            FreeCADGui.Selection.clearSelection()
            elist = obj.Shape.Edges
            for e in elist:
                for i in loopwire.Edges:
                    if e.hashCode() == i.hashCode():
                        FreeCADGui.Selection.addSelection(obj, "Edge"+str(elist.index(e)+1))

    def formsPartOfALoop(self, obj, sub, names):
        if names[0][0:4] != 'Edge':
            if names[0][0:4] == 'Face' and horizontalFaceLoop(obj, sub, names):
                return True
            return False
        if len(names) == 1 and horizontalEdgeLoop(obj, sub):
            return True
        if len(names) == 1 or names[1][0:4] != 'Edge':
            return False
        return True

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_SelectLoop', _CommandSelectLoop())

class _ToggleOperation:
    "command definition to toggle Operation Active state"
    def GetResources(self):
        return {'Pixmap': 'Path-OpActive',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_OpActiveToggle", "Toggle the Active State of the Operation"),
                'Accel': "P, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_OpActiveToggle", "Toggle the Active State of the Operation"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            obj = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(obj.Proxy, PathScripts.PathOp.ObjectOp)
        except:
            return False

    def Activated(self):
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        obj.Active = not(obj.Active)
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_OpActiveToggle', _ToggleOperation())

class _CopyOperation:
    "the Path Copy Operation command definition"
    def GetResources(self):
        return {'Pixmap': 'Path-OpCopy',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_OperationCopy", "Copy the operation in the job"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_OperationCopy", "Copy the operation in the job"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            obj = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(obj.Proxy, PathScripts.PathOp.ObjectOp)
        except:
            return False

    def Activated(self):
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        jobname = findParentJob(obj).Name
        addToJob(FreeCAD.ActiveDocument.copyObject(obj, False), jobname)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_OperationCopy', _CopyOperation())


# \c findShape() is referenced from Gui/Command.cpp and used by Path.Area commands.
# Do not remove!
def findShape(shape, subname=None, subtype=None):
    '''To find a higher order shape containing the subshape with subname.
        E.g. to find the wire containing 'Edge1' in shape,
            findShape(shape,'Edge1','Wires')
    '''
    if not subname:
        return shape
    ret = shape.getElement(subname)
    if not subtype or not ret or ret.isNull():
        return ret
    if subname.startswith('Face'):
        tp = 'Faces'
    elif subname.startswith('Edge'):
        tp = 'Edges'
    elif subname.startswith('Vertex'):
        tp = 'Vertex'
    else:
        return ret
    for obj in getattr(shape, subtype):
        for sobj in getattr(obj, tp):
            if sobj.isEqual(ret):
                return obj
    return ret
