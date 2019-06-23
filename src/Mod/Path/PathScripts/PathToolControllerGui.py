# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import Part
import PathScripts
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathToolEdit as PathToolEdit
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ViewProvider:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)
        self.vobj = vobj

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path-ToolController.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)

    def onDelete(self, vobj, args=None):
        PathUtil.clearExpressionEngine(vobj.Object)
        return True

    def updateData(self, vobj, prop):
        # this is executed when a property of the APP OBJECT changes
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
        return False

    def setupContextMenu(self, vobj, menu):
        PathLog.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate('Path', 'Edit'), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)

def Create(name = 'Default Tool', tool=None, toolNumber=1):
    PathLog.track(tool, toolNumber)

    obj = PathScripts.PathToolController.Create(name, tool, toolNumber)
    ViewProvider(obj.ViewObject)
    return obj


class CommandPathToolController:
    def GetResources(self):
        return {'Pixmap': 'Path-LengthOffset',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolController", "Add Tool Controller to the Job"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolController", "Add Tool Controller")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        PathLog.track()
        Create()

class ToolControllerEditor:

    def __init__(self, obj, asDialog):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolControllerEdit.ui")
        if not asDialog:
            self.form.buttonBox.hide()
        self.obj = obj

        self.vertFeed = PathGui.QuantitySpinBox(self.form.vertFeed, obj, 'VertFeed')
        self.horizFeed = PathGui.QuantitySpinBox(self.form.horizFeed, obj, 'HorizFeed')
        self.vertRapid = PathGui.QuantitySpinBox(self.form.vertRapid, obj, 'VertRapid')
        self.horizRapid = PathGui.QuantitySpinBox(self.form.horizRapid, obj, 'HorizRapid')

        self.editor = PathToolEdit.ToolEditor(obj.Tool, self.form.toolEditor)

    def updateUi(self):
        tc = self.obj
        self.form.tcName.setText(tc.Label)
        self.form.tcNumber.setValue(tc.ToolNumber)
        self.horizFeed.updateSpinBox()
        self.horizRapid.updateSpinBox()
        self.vertFeed.updateSpinBox()
        self.vertRapid.updateSpinBox()
        self.form.spindleSpeed.setValue(tc.SpindleSpeed)
        index = self.form.spindleDirection.findText(tc.SpindleDir, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.spindleDirection.setCurrentIndex(index)

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
        self.editor.setupUI()

        self.form.tcName.editingFinished.connect(self.refresh)
        self.form.horizFeed.editingFinished.connect(self.refresh)
        self.form.vertFeed.editingFinished.connect(self.refresh)
        self.form.horizRapid.editingFinished.connect(self.refresh)
        self.form.vertRapid.editingFinished.connect(self.refresh)


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
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.editor.updateToolController()
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.editor.updateUi()

        tool = self.obj.Tool
        radius = tool.Diameter / 2
        length = tool.CuttingEdgeHeight
        t = Part.makeCylinder(radius, length)
        self.toolrep.Shape = t

    def edit(self, item, column):
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
        "transfers the values from the widget to the object"
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
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
        restoreTC   = self.obj.Proxy.templateAttrs(self.obj)

        rc = False
        if not self.editor.form.exec_():
            PathLog.info("revert")
            self.obj.Proxy.setFromTemplate(self.obj, restoreTC)
            rc = True
        return rc

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolController', CommandPathToolController())

FreeCAD.Console.PrintLog("Loading PathToolControllerGui... done\n")
