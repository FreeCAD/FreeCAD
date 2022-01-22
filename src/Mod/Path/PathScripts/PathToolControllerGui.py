# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import FreeCADGui
import PathGui as PGui  # ensure Path/Gui/Resources are loaded
import PathScripts
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathToolBitGui as PathToolBitGui
import PathScripts.PathToolEdit as PathToolEdit
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider:
    def __init__(self, vobj):
        vobj.Proxy = self
        self.vobj = vobj

    def attach(self, vobj):
        mode = 2
        vobj.setEditorMode("LineWidth", mode)
        vobj.setEditorMode("MarkerColor", mode)
        vobj.setEditorMode("NormalColor", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("Selectable", mode)
        vobj.setEditorMode("ShapeColor", mode)
        vobj.setEditorMode("Transparency", mode)
        vobj.setEditorMode("Visibility", mode)
        self.vobj = vobj

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path_ToolController.svg"

    def onChanged(self, vobj, prop):
        # pylint: disable=unused-argument
        mode = 2
        vobj.setEditorMode("LineWidth", mode)
        vobj.setEditorMode("MarkerColor", mode)
        vobj.setEditorMode("NormalColor", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("Selectable", mode)

    def onDelete(self, vobj, args=None):
        # pylint: disable=unused-argument
        PathUtil.clearExpressionEngine(vobj.Object)
        self.vobj.Object.Proxy.onDelete(vobj.Object, args)
        return True

    def updateData(self, vobj, prop):
        # this is executed when a property of the APP OBJECT changes
        # pylint: disable=unused-argument
        pass

    def setEdit(self, vobj=None, mode=0):
        if 0 == mode:
            if vobj is None:
                vobj = self.vobj
            FreeCADGui.Control.closeDialog()
            taskd = TaskPanel(vobj.Object)
            FreeCADGui.Control.showDialog(taskd)
            taskd.setupUi()

            FreeCAD.ActiveDocument.recompute()

            return True
        return False

    def unsetEdit(self, vobj, mode):
        # this is executed when the user cancels or terminates edit mode
        # pylint: disable=unused-argument
        return False

    def setupContextMenu(self, vobj, menu):
        # pylint: disable=unused-argument
        PathLog.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate("Path", "Edit"), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)

    def claimChildren(self):
        obj = self.vobj.Object
        if obj and obj.Proxy and not obj.Proxy.usesLegacyTool(obj):
            return [obj.Tool]
        return []


def Create(name="Default Tool", tool=None, toolNumber=1):
    PathLog.track(tool, toolNumber)

    obj = PathScripts.PathToolController.Create(name, tool, toolNumber)
    ViewProvider(obj.ViewObject)
    if not obj.Proxy.usesLegacyTool(obj):
        # ToolBits are visible by default, which is typically not what the user wants
        if tool and tool.ViewObject and tool.ViewObject.Visibility:
            tool.ViewObject.Visibility = False
    return obj


class CommandPathToolController(object):
    # pylint: disable=no-init

    def GetResources(self):
        return {
            "Pixmap": "Path_LengthOffset",
            "MenuText": QtCore.QT_TRANSLATE_NOOP(
                "Path_ToolController", "Add Tool Controller to the Job"
            ),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Path_ToolController", "Add Tool Controller"
            ),
        }

    def selectedJob(self):
        if FreeCAD.ActiveDocument:
            sel = FreeCADGui.Selection.getSelectionEx()
            if sel and sel[0].Object.Name[:3] == "Job":
                return sel[0].Object
        jobs = [o for o in FreeCAD.ActiveDocument.Objects if o.Name[:3] == "Job"]
        if 1 == len(jobs):
            return jobs[0]
        return None

    def IsActive(self):
        return self.selectedJob() is not None

    def Activated(self):
        PathLog.track()
        job = self.selectedJob()
        if job:
            tool = PathToolBitGui.ToolBitSelector().getTool()
            if tool:
                toolNr = None
                for tc in job.Tools.Group:
                    if tc.Tool == tool:
                        toolNr = tc.ToolNumber
                        break
                if not toolNr:
                    toolNr = max([tc.ToolNumber for tc in job.Tools.Group]) + 1
                tc = Create("TC: {}".format(tool.Label), tool, toolNr)
                job.Proxy.addToolController(tc)
                FreeCAD.ActiveDocument.recompute()


class ToolControllerEditor(object):
    def __init__(self, obj, asDialog):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolControllerEdit.ui")
        if not asDialog:
            self.form.buttonBox.hide()
        self.obj = obj

        self.vertFeed = PathGui.QuantitySpinBox(self.form.vertFeed, obj, "VertFeed")
        self.horizFeed = PathGui.QuantitySpinBox(self.form.horizFeed, obj, "HorizFeed")
        self.vertRapid = PathGui.QuantitySpinBox(self.form.vertRapid, obj, "VertRapid")
        self.horizRapid = PathGui.QuantitySpinBox(
            self.form.horizRapid, obj, "HorizRapid"
        )

        if obj.Proxy.usesLegacyTool(obj):
            self.editor = PathToolEdit.ToolEditor(obj.Tool, self.form.toolEditor)
        else:
            self.editor = None
            self.form.toolBox.widget(1).hide()
            self.form.toolBox.removeItem(1)

    def updateUi(self):
        tc = self.obj
        self.form.tcName.setText(tc.Label)
        self.form.tcNumber.setValue(tc.ToolNumber)
        self.horizFeed.updateSpinBox()
        self.horizRapid.updateSpinBox()
        self.vertFeed.updateSpinBox()
        self.vertRapid.updateSpinBox()
        self.form.spindleSpeed.setValue(tc.SpindleSpeed)
        index = self.form.spindleDirection.findText(
            tc.SpindleDir, QtCore.Qt.MatchFixedString
        )
        if index >= 0:
            self.form.spindleDirection.setCurrentIndex(index)

        if self.editor:
            self.editor.updateUI()

    def updateToolController(self):
        tc = self.obj
        try:
            tc.Label = self.form.tcName.text()
            tc.ToolNumber = self.form.tcNumber.value()
            self.horizFeed.updateProperty()
            self.vertFeed.updateProperty()
            self.horizRapid.updateProperty()
            self.vertRapid.updateProperty()
            tc.SpindleSpeed = self.form.spindleSpeed.value()
            tc.SpindleDir = self.form.spindleDirection.currentText()

            if self.editor:
                self.editor.updateTool()
                tc.Tool = self.editor.tool

        except Exception as e:
            PathLog.error(translate("PathToolController", "Error updating TC: %s") % e)

    def refresh(self):
        self.form.blockSignals(True)
        self.updateToolController()
        self.updateUi()
        self.form.blockSignals(False)

    def setupUi(self):
        if self.editor:
            self.editor.setupUI()

        self.form.tcName.editingFinished.connect(self.refresh)
        self.form.horizFeed.editingFinished.connect(self.refresh)
        self.form.vertFeed.editingFinished.connect(self.refresh)
        self.form.horizRapid.editingFinished.connect(self.refresh)
        self.form.vertRapid.editingFinished.connect(self.refresh)
        self.form.spindleSpeed.editingFinished.connect(self.refresh)
        self.form.spindleDirection.currentIndexChanged.connect(self.refresh)


class TaskPanel:
    def __init__(self, obj):
        self.editor = ToolControllerEditor(obj, False)
        self.form = self.editor.form
        self.updating = False
        self.toolrep = None
        self.obj = obj

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        if self.toolrep:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        if self.toolrep:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.editor.updateToolController()
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.editor.updateUi()

        if self.toolrep:
            tool = self.obj.Tool
            radius = float(tool.Diameter) / 2
            length = tool.CuttingEdgeHeight
            t = Part.makeCylinder(radius, length)
            self.toolrep.Shape = t

    def edit(self, item, column):
        # pylint: disable=unused-argument
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
        # pylint: disable=unused-argument
        "transfers the values from the widget to the object"
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
        if self.editor.editor:
            t = Part.makeCylinder(1, 1)
            self.toolrep = FreeCAD.ActiveDocument.addObject("Part::Feature", "tool")
            self.toolrep.Shape = t

        self.setFields()
        self.editor.setupUi()


class DlgToolControllerEdit:
    def __init__(self, obj):
        self.editor = ToolControllerEditor(obj, True)
        self.editor.updateUi()
        self.editor.setupUi()
        self.obj = obj

    def exec_(self):
        restoreTC = self.obj.Proxy.templateAttrs(self.obj)

        rc = False
        if not self.editor.form.exec_():
            PathLog.info("revert")
            self.obj.Proxy.setFromTemplate(self.obj, restoreTC)
            rc = True
        return rc


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_ToolController", CommandPathToolController())

FreeCAD.Console.PrintLog("Loading PathToolControllerGui... done\n")
