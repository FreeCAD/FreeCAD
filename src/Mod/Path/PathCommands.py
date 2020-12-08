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
import PathScripts
import PathScripts.PathLog as PathLog
import traceback

import json
import tempfile
import os
import Mesh
import string
import random

from PathScripts.PathUtils import loopdetect
from PathScripts.PathUtils import horizontalEdgeLoop
from PathScripts.PathUtils import horizontalFaceLoop
from PathScripts.PathUtils import addToJob
from PathScripts.PathUtils import findParentJob
from PathScripts.PathPost  import CommandPathPost

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore
    from PySide import QtGui
else:
    def translate(ctxt, txt):
        return txt

__title__ = "FreeCAD Path Commands"
__author__ = "sliptonic"
__url__ = "https://www.freecadweb.org"

class _CommandCamoticsSimulate:

    def __init__(self):
        self.obj = None
        self.sub = []
        self.active = False

        self.tooltemplate = {
            "units": "metric",
            "shape": "cylindrical",
            "length": 10,
            "diameter": 3.125,
            "description": ""
        }

        self.workpiecetemplate = {
            "automatic": "false",
            "margin": 0,
            "bounds": {
                "min": [0, 0, 0],
                "max": [0, 0, 0]}
        }

        self.camoticstemplate = {
            "units": "metric",
            "resolution-mode": "medium",
            "resolution": 1,
            "tools": {},
            "workpiece": {},
            "files": []
        }

    def GetResources(self):
        return {'Pixmap': 'Path_Camotics',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Camotics", "Camotics"),
                'Accel': "P, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PatCamoticsop", "Simulate using Camotics"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            job = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(job.Proxy, PathScripts.PathJob.ObjectJob)
        except:
            return False

    def Activated(self):
        pp = CommandPathPost()
        job = FreeCADGui.Selection.getSelectionEx()[0].Object

        with tempfile.TemporaryDirectory() as td:
            postlist = pp.getPostObjects(job)

            if hasattr(job, "SplitOutput"):
                split = job.SplitOutput
            else:
                split = False

            fnames = []
            if split:
                for i, slist in enumerate(postlist):
                    filename = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(6))
                    fnames.append(pp.exportObjectsWith(slist, job, "{}{}{}-{}{}".format(td,os.sep, filename)))
            else:
                finalpostlist = [item for slist in postlist for item in slist]
                filename = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(6))
                fnames.append(pp.exportObjectsWith(finalpostlist, job, "{}{}{}".format(td,os.sep,filename)))

            data = self.buildproject(job, fnames)
            f_name = os.path.join(td, 'project.camotics')
            meshname = os.path.join(td, 'output.stl')

            with open(f_name, 'w') as fh:
                fh.write(data)
                fh.close()
                os.system('camsim {} {}'.format(f_name, meshname))

                Mesh.insert(meshname, FreeCAD.ActiveDocument.Name)
                import time
                time.sleep(60)

    def buildproject(self, job, files=[]):

        unitstring = "imperial" if FreeCAD.Units.getSchema() in [2,3,5,7] else "metric"

        templ = self.camoticstemplate

        templ["units"] = unitstring
        templ["resolution-mode"] = "medium"
        templ["resolution"] = 1

        toollist = {}
        for t in job.ToolController:
            ttemplate = self.tooltemplate
            ttemplate["units"] = unitstring
            if hasattr(t.Tool, 'Camotics'):
                ttemplate["shape"] =  t.Tool.Camotics
            else:
                ttemplate["shape"] =  "cylindrical"
            ttemplate["length"] = t.Tool.Length.Value
            ttemplate["diameter"] = t.Tool.Diameter.Value
            ttemplate["description"] = t.Label
            toollist[t.ToolNumber] = ttemplate

        templ['tools'] = toollist

        bb = job.Stock.Shape.BoundBox
        workpiecetmpl = self.workpiecetemplate
        workpiecetmpl['bounds']['min'] = [bb.XMin, bb.YMin, bb.ZMin]
        workpiecetmpl['bounds']['max'] = [bb.XMax, bb.YMax, bb.ZMax]
        templ['workpiece'] = workpiecetmpl

        templ['files'] = files

        return json.dumps(templ, indent=2)

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_Camotics', _CommandCamoticsSimulate())


class _CommandSelectLoop:
    "the Path command to complete loop selection definition"
    def __init__(self):
        self.obj = None
        self.sub = []
        self.active = False

    def GetResources(self):
        return {'Pixmap': 'Path_SelectLoop',
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
                #self.active = self.formsPartOfALoop(sel.Object, sel.SubObjects[0], sel.SubElementNames)
                self.active = True
            else:
                self.active = False
            return self.active
        except Exception as exc:
            PathLog.error(exc)
            traceback.print_exc(exc)
            return False

    def Activated(self):
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
                        FreeCADGui.Selection.addSelection(obj, "Edge" + str(elist.index(e) + 1))
        elif FreeCAD.GuiUp:
            QtGui.QMessageBox.information(None,
                    QtCore.QT_TRANSLATE_NOOP('Path_SelectLoop', 'Feature Completion'),
                    QtCore.QT_TRANSLATE_NOOP('Path_SelectLoop', 'Closed loop detection failed.'))

    def formsPartOfALoop(self, obj, sub, names):
        try:
            if names[0][0:4] != 'Edge':
                if names[0][0:4] == 'Face' and horizontalFaceLoop(obj, sub, names):
                    return True
                return False
            if len(names) == 1 and horizontalEdgeLoop(obj, sub):
                return True
            if len(names) == 1 or names[1][0:4] != 'Edge':
                return False
            return True
        except Exception:
            return False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_SelectLoop', _CommandSelectLoop())


class _ToggleOperation:
    "command definition to toggle Operation Active state"
    def GetResources(self):
        return {'Pixmap': 'Path_OpActive',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_OpActiveToggle", "Toggle the Active State of the Operation"),
                'Accel': "P, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_OpActiveToggle", "Toggle the Active State of the Operation"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            for sel in FreeCADGui.Selection.getSelectionEx():
                if not isinstance(PathScripts.PathDressup.baseOp(sel.Object).Proxy, PathScripts.PathOp.ObjectOp):
                    return False
            return True
        except(IndexError, AttributeError):
            return False

    def Activated(self):
        for sel in FreeCADGui.Selection.getSelectionEx():
            op = PathScripts.PathDressup.baseOp(sel.Object)
            op.Active = not op.Active
            op.ViewObject.Visibility = op.Active

        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_OpActiveToggle', _ToggleOperation())


class _CopyOperation:
    "the Path Copy Operation command definition"
    def GetResources(self):
        return {'Pixmap': 'Path_OpCopy',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_OperationCopy", "Copy the operation in the job"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_OperationCopy", "Copy the operation in the job"),
                'CmdType': "ForEdit"}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            for sel in FreeCADGui.Selection.getSelectionEx():
                if not isinstance(sel.Object.Proxy, PathScripts.PathOp.ObjectOp):
                    return False
            return True
        except(IndexError, AttributeError):
            return False

    def Activated(self):
        for sel in FreeCADGui.Selection.getSelectionEx():
            jobname = findParentJob(sel.Object).Name
            addToJob(FreeCAD.ActiveDocument.copyObject(sel.Object, False), jobname)


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
