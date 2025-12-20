# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import math
import Path.Base.Gui.Util as PathGuiUtil
import PathScripts.PathUtils as PathUtils
import Path.Dressup.Utils as PathDressup
import Path.Post.Utils as PostUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


if FreeCAD.GuiUp:
    import FreeCADGui

__doc__ = """Axis remapping Dressup object and FreeCAD command.  This dressup remaps one axis of motion to another.
For example, you can re-map the Y axis to A to control a 4th axis rotary."""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj):
        maplist = ["X->A", "Y->A", "X->B", "Y->B", "X->C", "Y->C"]
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "AxisMap",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The input mapping axis"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "Radius",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The radius of the wrapped axis"),
        )
        obj.AxisMap = maplist
        obj.AxisMap = "Y->A"
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def execute(self, obj):

        inAxis = obj.AxisMap[0]
        outAxis = obj.AxisMap[3]
        d = 0.1

        if obj.Base:
            if obj.Base.isDerivedFrom("Path::Feature"):
                if obj.Base.Path:
                    if obj.Base.Path.Commands:
                        pp = PathUtils.getPathWithPlacement(obj.Base)
                        pp = PostUtils.splitArcs(pp, deflection=d)

                        newcommandlist = []
                        currLocation = {"X": 0, "Y": 0, "Z": 0, "F": 0}

                        for c in pp.Commands:
                            newparams = dict(c.Parameters)
                            remapvar = newparams.pop(inAxis, None)
                            if remapvar is not None:
                                newparams[outAxis] = math.degrees(remapvar / obj.Radius.Value)
                                locdiff = dict(set(newparams.items()) - set(currLocation.items()))
                                if (
                                    len(locdiff) == 1 and outAxis in locdiff
                                ):  # pure rotation.  Calculate rotational feed rate
                                    if "F" in c.Parameters:
                                        feed = c.Parameters["F"]
                                    else:
                                        feed = currLocation["F"]
                                    newparams.update({"F": math.degrees(feed / obj.Radius.Value)})
                                newcommand = Path.Command(c.Name, newparams)
                                newcommandlist.append(newcommand)
                                currLocation.update(newparams)
                            else:
                                newcommandlist.append(c)
                                currLocation.update(c.Parameters)

                        path = Path.Path(newcommandlist)
                        path.Center = self.center(obj)
                        obj.Path = path

    def onChanged(self, obj, prop):
        if "Restore" not in obj.State and prop == "Radius":
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.setCenterOfRotation(self.center(obj))
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def center(self, obj):
        return FreeCAD.Vector(0, 0, 0 - obj.Radius.Value)


class TaskPanel:
    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/AxisMapEdit.ui")
        self.radius = PathGuiUtil.QuantitySpinBox(self.form.radius, obj, "Radius")
        FreeCAD.ActiveDocument.openTransaction("Edit Dragknife Dress-up")

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.radius.updateProperty()
        self.obj.AxisMap = self.form.axisMapInput.currentText()
        self.obj.Proxy.execute(self.obj)

    def updateUI(self):
        self.radius.updateWidget()
        self.form.axisMapInput.setCurrentText(self.obj.AxisMap)
        self.updateModel()

    def updateModel(self):
        self.getFields()
        FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.updateUI()

    def open(self):
        pass

    def setupUi(self):
        self.setFields()
        self.form.radius.valueChanged.connect(self.updateModel)
        self.form.axisMapInput.currentIndexChanged.connect(self.updateModel)


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object

    def attach(self, vobj):
        self.obj = vobj.Object
        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
            # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return

    def unsetEdit(self, vobj, mode=0):
        return False

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def claimChildren(self):
        return [self.obj.Base]

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(arg1.Object)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupAxisMap", "Axis Map"),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_DressupAxisMap", "Remaps one axis to another"),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("CAM_Dressup", "Select one toolpath object\n"))
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("CAM_Dressup", "The selected object is not a toolpath\n")
            )
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("CAM_Dressup", "Select a toolpath object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.AxisMap")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "AxisMapDressup")'
        )
        FreeCADGui.doCommand("Path.Dressup.Gui.AxisMap.ObjectDressup(obj)")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("obj.Radius = 45")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.AxisMap.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("Gui.ActiveDocument.getObject(base.Name).Visibility = False")
        FreeCADGui.doCommand("obj.ViewObject.Document.setEdit(obj.ViewObject, 0)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupAxisMap", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup… done\n")
