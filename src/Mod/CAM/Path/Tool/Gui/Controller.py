# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                 2025 Samuel Abels <knipknap@gmail.com>                  *
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

from lazy_loader.lazy_loader import LazyLoader
from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Base.Util as PathUtil
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit.ui.selector import ToolBitSelector

Part = LazyLoader("Part", globals(), "Part")


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


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
        vobj.setEditorMode("ShapeAppearance", mode)
        vobj.setEditorMode("Transparency", mode)
        vobj.setEditorMode("Visibility", mode)
        self.vobj = vobj

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getIcon(self):
        return ":/icons/CAM_ToolController.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode("LineWidth", mode)
        vobj.setEditorMode("MarkerColor", mode)
        vobj.setEditorMode("NormalColor", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("Selectable", mode)

    def onDelete(self, vobj, args=None):
        PathUtil.clearExpressionEngine(vobj.Object)
        self.vobj.Object.Proxy.onDelete(vobj.Object, args)
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
        Path.Log.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate("CAM", "Edit"), menu)
        action.triggered.connect(self._editInContextMenuTriggered)
        menu.addAction(action)

    def _editInContextMenuTriggered(self, checked):
        self.setEdit()

    def claimChildren(self):
        obj = self.vobj.Object
        if obj and obj.Proxy and obj.Tool:
            return [obj.Tool]
        return []


def Create(name="Default Tool", tool=None, toolNumber=1):
    Path.Log.track(tool, toolNumber)

    obj = PathToolController.Create(name, tool, toolNumber)
    ViewProvider(obj.ViewObject)
    # ToolBits are visible by default, which is typically not what the user wants
    if tool and tool.ViewObject and tool.ViewObject.Visibility:
        tool.ViewObject.Visibility = False
    return obj


class CommandPathToolController(object):
    def GetResources(self):
        return {
            "Pixmap": "CAM_LengthOffset",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolController", "Tool Controller"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolController", "Adds a new tool controller to the active job"
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
        Path.Log.track()
        job = self.selectedJob()
        if not job:
            return

        # Let the user select a toolbit
        selector = ToolBitSelector()
        if not selector.exec_():
            return
        tool = selector.get_selected_tool()
        if not tool:
            return

        # Find a tool number
        toolNr = None
        for tc in job.Tools.Group:
            if tc.Tool == tool:
                toolNr = tc.ToolNumber
                break
        if not toolNr:
            toolNr = max([tc.ToolNumber for tc in job.Tools.Group]) + 1

        # Create the new tool controller with the tool.
        tc = Create("TC: {}".format(tool.Label), tool, toolNr)
        job.Proxy.addToolController(tc)
        FreeCAD.ActiveDocument.recompute()


class BlockScrollWheel(QtCore.QObject):
    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.Type.Wheel:
            if not obj.hasFocus():
                return True
        return super().eventFilter(obj, event)


class ToolControllerEditor(object):

    def __init__(
        self, obj, asDialog, notifyChanged=None, showCountLabel=False, disableToolNumber=False
    ):
        self.notifyChanged = notifyChanged
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolControllerEdit.ui")
        self.controller = FreeCADGui.PySideUic.loadUi(":/panels/ToolControllerEdit.ui")
        self.form.tc_layout.addWidget(self.controller)
        if not asDialog:
            self.form.buttonBox.hide()
        if not showCountLabel:
            self.controller.tcOperationCountLabel.hide()
        self.obj = obj

        comboToPropertyMap = [("spindleDirection", "SpindleDir")]
        enumTups = PathToolController.ToolController.propertyEnumerations(dataType="raw")

        PathGuiUtil.populateCombobox(self.controller, enumTups, comboToPropertyMap)
        self.vertFeed = PathGuiUtil.QuantitySpinBox(self.controller.vertFeed, obj, "VertFeed")
        self.horizFeed = PathGuiUtil.QuantitySpinBox(self.controller.horizFeed, obj, "HorizFeed")
        self.vertRapid = PathGuiUtil.QuantitySpinBox(self.controller.vertRapid, obj, "VertRapid")
        self.horizRapid = PathGuiUtil.QuantitySpinBox(self.controller.horizRapid, obj, "HorizRapid")

        self.blockScrollWheel = BlockScrollWheel()
        self.controller.tcNumber.installEventFilter(self.blockScrollWheel)
        self.controller.spindleDirection.installEventFilter(self.blockScrollWheel)
        self.controller.tcNumber.setReadOnly(disableToolNumber)

        self.editor = None

    def selectInComboBox(self, name, combo):
        """selectInComboBox(name, combo) ...
        helper function to select a specific value in a combo box."""
        with QtCore.QSignalBlocker(combo):
            index = combo.currentIndex()  # Save initial index

            # Search using currentData and return if found
            newindex = combo.findData(name)
            if newindex >= 0:
                combo.setCurrentIndex(newindex)
                return

            # if not found, search using current text
            newindex = combo.findText(name, QtCore.Qt.MatchFixedString)
            if newindex >= 0:
                combo.setCurrentIndex(newindex)
                return

            # not found, return unchanged
            combo.setCurrentIndex(index)
        return

    def updateUi(self):
        tc = self.obj

        with (
            QtCore.QSignalBlocker(self.controller.tcName),
            QtCore.QSignalBlocker(self.controller.tcNumber),
            QtCore.QSignalBlocker(self.horizFeed.widget),
            QtCore.QSignalBlocker(self.horizRapid.widget),
            QtCore.QSignalBlocker(self.vertFeed.widget),
            QtCore.QSignalBlocker(self.vertRapid.widget),
            QtCore.QSignalBlocker(self.controller.spindleSpeed),
            QtCore.QSignalBlocker(self.controller.spindleDirection),
        ):
            self.controller.tcName.setText(tc.Label)
            self.controller.tcNumber.setValue(tc.ToolNumber)
            self.horizFeed.updateWidget()
            self.horizRapid.updateWidget()
            self.vertFeed.updateWidget()
            self.vertRapid.updateWidget()
            self.controller.spindleSpeed.setValue(tc.SpindleSpeed)

            self.selectInComboBox(tc.SpindleDir, self.controller.spindleDirection)

            if self.editor:
                self.editor.updateUI()

    def updateToolController(self):
        tc = self.obj
        try:
            tc.Label = self.controller.tcName.text()
            tc.ToolNumber = self.controller.tcNumber.value()
            self.horizFeed.updateProperty()
            self.vertFeed.updateProperty()
            self.horizRapid.updateProperty()
            self.vertRapid.updateProperty()
            tc.SpindleSpeed = self.controller.spindleSpeed.value()
            tc.SpindleDir = self.controller.spindleDirection.currentData()

            if self.editor:
                self.editor.updateTool()
                tc.Tool = self.editor.tool

        except Exception as e:
            Path.Log.error("Error updating TC: {}".format(e))

    def changed(self):
        self.form.blockSignals(True)
        self.controller.blockSignals(True)
        self.updateToolController()
        self.updateUi()
        self.controller.blockSignals(False)
        self.form.blockSignals(False)

        if self.notifyChanged:
            self.notifyChanged()

    def setupUi(self):
        if self.editor:
            self.editor.setupUI()

        self.controller.tcName.textChanged.connect(self.changed)
        self.controller.tcNumber.editingFinished.connect(self.changed)
        self.vertFeed.widget.textChanged.connect(self.changed)
        self.horizFeed.widget.textChanged.connect(self.changed)
        self.vertRapid.widget.textChanged.connect(self.changed)
        self.horizRapid.widget.textChanged.connect(self.changed)
        self.controller.spindleSpeed.editingFinished.connect(self.changed)
        self.controller.spindleDirection.currentIndexChanged.connect(self.changed)


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
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
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
            Path.Log.info("revert")
            self.obj.Proxy.setFromTemplate(self.obj, restoreTC)
            rc = True
        else:
            self.editor.updateToolController()
            self.obj.Proxy.execute(self.obj)
        return rc


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_ToolController", CommandPathToolController())

FreeCAD.Console.PrintLog("Loading PathToolControllerGui… done\n")
