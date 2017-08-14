# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathGetPoint as PathGetPoint
import PathScripts.PathLog as PathLog
import PathScripts.PathSelection as PathSelection
import PathScripts.PathOp as PathOp
import importlib

from PathScripts import PathUtils
from PySide import QtCore, QtGui

# TaskPanelLayout
#  0 ... existing toolbox layout
#  1 ... reverse order
#  2 ... multi panel layout
#  3 ... multi panel layout reverse
TaskPanelLayout = 0


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider(object):

    def __init__(self, vobj, resources):
        PathLog.track()
        vobj.Proxy = self
        self.deleteOnReject = True
        self.OpIcon = ":/icons/%s.svg" % resources.pixmap
        self.OpName = resources.name
        self.OpPageModule = resources.opPageClass.__module__
        self.OpPageClass = resources.opPageClass.__name__

    def attach(self, vobj):
        PathLog.track()
        self.Object = vobj.Object
        self.panel = None
        return

    def deleteObjectsOnReject(self):
        PathLog.track()
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        PathLog.track()
        page = self.getTaskPanelOpPage(vobj.Object)
        selection = self.getSelectionFactory()
        self.setupTaskPanel(TaskPanel(vobj.Object, self.deleteObjectsOnReject(), page, selection))
        self.deleteOnReject = False
        return True

    def setupTaskPanel(self, panel):
        self.panel = panel
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()

    def clearTaskPanel(self):
        self.panel = None

    def __getstate__(self):
        PathLog.track()
        state = {}
        state['OpName'] = self.OpName
        state['OpIcon'] = self.OpIcon
        state['OpPageModule'] = self.OpPageModule
        state['OpPageClass'] = self.OpPageClass
        return state

    def __setstate__(self, state):
        self.OpName = state['OpName'] 
        self.OpIcon = state['OpIcon']
        self.OpPageModule = state['OpPageModule']
        self.OpPageClass = state['OpPageClass']

    def getIcon(self):
        return self.OpIcon

    def getTaskPanelOpPage(self, obj):
        mod = importlib.import_module(self.OpPageModule)
        cls = getattr(mod, self.OpPageClass)
        return cls(obj, 0)

    def getSelectionFactory(self):
        return PathSelection.select(self.OpName)

    def updateData(self, obj, prop):
        # PathLog.track(obj.Label, prop) # Creates a lot of noise
        if self.panel:
            self.panel.updateData(obj, prop)

class TaskPanelPage(object):

    # task panel interaction framework
    def __init__(self, obj, features):
        self.obj = obj
        self.form = self.getForm()
        self.setDirty()
        self.setTitle('-')
        self.features = features

    def setDirty(self):
        self.isdirty = True
    def setClean(self):
        self.isdirty = False

    def pageGetFields(self):
        self.getFields(self.obj)
        self.setDirty()

    def pageSetFields(self):
        self.setFields(self.obj)

    def pageRegisterSignalHandlers(self):
        for signal in self.getSignalsForUpdate(self.obj):
            signal.connect(self.pageGetFields)
        self.registerSignalHandlers(self.obj)

    def pageUpdateData(self, obj, prop):
        self.updateData(obj, prop)

    def setTitle(self, title):
        self.title = title
    def getTitle(self, obj):
        return self.title

    # subclass interface
    def initPage(self, obj):
        pass
    def modifyStandardButtons(self, buttonBox):
        pass
    def getForm(self):
        pass
    def getFields(self, obj):
        pass
    def setFields(self, obj):
        pass
    def getSignalsForUpdate(self, obj):
        return []
    def registerSignalHandlers(self, obj):
        pass
    def updateData(self, obj, prop):
        pass

    # helpers
    def selectInComboBox(self, name, combo):
        index = combo.findText(name, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.blockSignals(True)
            combo.setCurrentIndex(index)
            combo.blockSignals(False)

    def setupToolController(self, obj, combo):
        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        combo.blockSignals(True)
        combo.addItems(labels)
        combo.blockSignals(False)

        if obj.ToolController is None:
            obj.ToolController = PathUtils.findToolController(obj)
        if obj.ToolController is not None:
            self.selectInComboBox(obj.ToolController.Label, combo)

    def updateToolController(self, obj, combo):
        tc = PathUtils.findToolController(obj, combo.currentText())
        if obj.ToolController != tc:
            obj.ToolController = tc

    def updateInputField(self, obj, prop, widget):
        value = FreeCAD.Units.Quantity(widget.text()).Value
        if getattr(obj, prop) != value:
            PathLog.debug("updateInputField(%s, %s): %.2f -> %.2f" % (obj.Label, prop, getattr(obj, prop), value))
            setattr(obj, prop, value)

class TaskPanelBaseGeometryPage(TaskPanelPage):
    DataObject    = QtCore.Qt.ItemDataRole.UserRole
    DataObjectSub = QtCore.Qt.ItemDataRole.UserRole + 1

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageBaseGeometryEdit.ui")
    def getTitle(self, obj):
        return translate("PathOp", "Base Geometry")
    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.form.baseList.blockSignals(True)
        self.form.baseList.clear()
        for base in self.obj.Base:
            for sub in base[1]:
                item = QtGui.QListWidgetItem("%s.%s" % (base[0].Label, sub))
                item.setData(self.DataObject, base[0])
                item.setData(self.DataObjectSub, sub)
                self.form.baseList.addItem(item)
        self.form.baseList.blockSignals(False)

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        for item in self.form.baseList.selectedItems():
            obj = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if sub:
                FreeCADGui.Selection.addSelection(obj, sub)
            else:
                FreeCADGui.Selection.addSelection(obj)
        #FreeCADGui.updateGui()

    def supportsVertexes(self):
        return self.features & PathOp.FeatureBaseVertexes
    def supportsEdges(self):
        return self.features & PathOp.FeatureBaseEdges
    def supportsFaces(self):
        return self.features & PathOp.FeatureBaseFaces
    def supportsPanels(self):
        return self.features & PathOp.FeatureBasePanels

    def featureName(self):
        if self.supportsEdges() and self.supportsFaces():
            return 'features'
        if self.supportsFaces():
            return 'faces'
        if self.supportsEdges():
            return 'edges'
        return 'nothing'

    def addBaseGeometry(self, selection):
        PathLog.track(selection)
        if len(selection) != 1:
            PathLog.error(translate("PathProject", "Please select %s from a single solid" % self.featureName()))
            return False
        sel = selection[0]
        if sel.HasSubObjects:
            if not self.supportsVertexes() and selection[0].SubObjects[0].ShapeType == "Vertex":
                PathLog.error(translate("PathProject", "Vertexes are not supported"))
                return False
            if not self.supportsEdges() and selection[0].SubObjects[0].ShapeType == "Edge":
                PathLog.error(translate("PathProject", "Edges are not supported"))
                return False
            if not self.supportsFaces() and selection[0].SubObjects[0].ShapeType == "Face":
                PathLog.error(translate("PathProject", "Faces are not supported"))
                return False
        else:
            if not self.supportsPanels() or not 'Panel' in sel.Object.Name:
                PathLog.error(translate("PathProject", "Please select %s of a solid" % self.featureName()))
                return False

        for sub in sel.SubElementNames:
            self.obj.Proxy.addBase(self.obj, sel.Object, sub)
        return True

    def addBase(self):
        if self.addBaseGeometry(FreeCADGui.Selection.getSelectionEx()):
            #self.obj.Proxy.execute(self.obj)
            self.setFields(self.obj)

    def deleteBase(self):
        PathLog.track()
        selected = self.form.baseList.selectedItems()
        for item in selected:
            self.form.baseList.takeItem(self.form.baseList.row(item))
        self.updateBase()
        #self.obj.Proxy.execute(self.obj)
        #FreeCAD.ActiveDocument.recompute()

    def updateBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            obj = item.data(self.DataObject)
            sub = str(item.data(self.DataObjectSub))
            base = (obj, sub)
            newlist.append(base)
        PathLog.debug("Setting new base: %s -> %s" % (self.obj.Base, newlist))
        self.obj.Base = newlist

        #self.obj.Proxy.execute(self.obj)
        #FreeCAD.ActiveDocument.recompute()

    def clearBase(self):
        self.obj.Base = []

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.clearBase.clicked.connect(self.clearBase)

    def pageUpdateData(self, obj, prop):
        if prop in ['Base']:
            self.setFields(obj)


class TaskPanelBaseLocationPage(TaskPanelPage):

    DataLocation = QtCore.Qt.ItemDataRole.UserRole

    def getForm(self):
        self.formLoc = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseLocationEdit.ui")
        self.formPts = FreeCADGui.PySideUic.loadUi(":/panels/PointEdit.ui")
        self.formLoc.baseList.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.formPts.setParent(self.formLoc.addRemoveEdit.parent())
        self.formLoc.addRemoveEdit.parent().layout().addWidget(self.formPts)
        self.formPts.hide()
        self.getPoint = PathGetPoint.TaskPanel(self.formLoc.addRemoveEdit, self.formPts)
        return self.formLoc

    def modifyStandardButtons(self, buttonBox):
        self.getPoint.buttonBox = buttonBox

    def getTitle(self, obj):
        return translate("PathOp", "Base Location")
    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.formLoc.baseList.blockSignals(True)
        self.formLoc.baseList.clearContents()
        self.formLoc.baseList.setRowCount(0)
        for location in self.obj.Locations:
            self.formLoc.baseList.insertRow(self.formLoc.baseList.rowCount())

            item = QtGui.QTableWidgetItem("%.2f" % location.x)
            item.setData(self.DataLocation, location.x)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount()-1, 0, item)

            item = QtGui.QTableWidgetItem("%.2f" % location.y)
            item.setData(self.DataLocation, location.y)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount()-1, 1, item)
        self.formLoc.baseList.resizeColumnToContents(0)
        self.formLoc.baseList.blockSignals(False)
        self.itemActivated()

    def removeLocation(self):
        deletedRows = []
        selected = self.formLoc.baseList.selectedItems()
        for item in selected:
            row = self.formLoc.baseList.row(item)
            if not row in deletedRows:
                deletedRows.append(row)
                self.formLoc.baseList.removeRow(row)
        self.updateLocations()
        FreeCAD.ActiveDocument.recompute()

    def updateLocations(self):
        PathLog.track()
        locations = []
        for i in range(self.formLoc.baseList.rowCount()):
            x = self.formLoc.baseList.item(i, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(i, 1).data(self.DataLocation)
            location = FreeCAD.Vector(x, y, 0)
            locations.append(location)
        self.obj.Locations = locations

    def addLocation(self):
        self.getPoint.getPoint(self.addLocationAt)

    def addLocationAt(self, point, obj):
        if point:
            locations = self.obj.Locations
            locations.append(point)
            self.obj.Locations = locations
            FreeCAD.ActiveDocument.recompute()

    def editLocation(self):
        selected = self.formLoc.baseList.selectedItems()
        if selected:
            row = self.formLoc.baseList.row(selected[0])
            self.editRow = row
            x = self.formLoc.baseList.item(row, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(row, 1).data(self.DataLocation)
            start = FreeCAD.Vector(x, y, 0)
            self.getPoint.getPoint(self.editLocationAt, start)

    def editLocationAt(self, point, obj):
        if point:
            self.formLoc.baseList.item(self.editRow, 0).setData(self.DataLocation, point.x)
            self.formLoc.baseList.item(self.editRow, 1).setData(self.DataLocation, point.y)
            self.updateLocations()
            FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        if self.formLoc.baseList.selectedItems():
            self.form.removeLocation.setEnabled(True)
            self.form.editLocation.setEnabled(True)
        else:
            self.form.removeLocation.setEnabled(False)
            self.form.editLocation.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.formLoc.addLocation.clicked.connect(self.addLocation)
        self.formLoc.removeLocation.clicked.connect(self.removeLocation)
        self.formLoc.editLocation.clicked.connect(self.editLocation)

    def pageUpdateData(self, obj, prop):
        if prop in ['Locations']:
            self.setFields(obj)


class TaskPanelHeightsPage(TaskPanelPage):
    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageHeightsEdit.ui")
    def getTitle(self, obj):
        return translate("Path_AreaOp", "Heights")
    def getFields(self, obj):
        self.updateInputField(obj, 'SafeHeight',      self.form.safeHeight)
        self.updateInputField(obj, 'ClearanceHeight', self.form.clearanceHeight)
    def setFields(self,  obj):
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.safeHeight.editingFinished)
        signals.append(self.form.clearanceHeight.editingFinished)
        return signals

    def pageUpdateData(self, obj, prop):
        if prop in ['SafeHeight', 'ClearanceHeight']:
            self.setFields(obj)


class TaskPanelDepthsPage(TaskPanelPage):

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageDepthsEdit.ui")

    def initPage(self, obj):
        if not PathOp.FeatureStepDown & self.features:
            self.form.stepDown.hide()
            self.form.stepDownLabel.hide()

        if not PathOp.FeatureFinishDepth & self.features:
            self.form.finishDepth.hide()
            self.form.finishDepthLabel.hide()

    def getTitle(self, obj):
        return translate("PathOp", "Depths")

    def getFields(self, obj):
        self.updateInputField(obj, 'StartDepth', self.form.startDepth)
        self.updateInputField(obj, 'FinalDepth', self.form.finalDepth)
        if PathOp.FeatureStepDown & self.features:
            self.updateInputField(obj, 'StepDown', self.form.stepDown)
        if PathOp.FeatureFinishDepth & self.features:
            self.updateInputField(obj, 'FinishDepth', self.form.finishDepth)
    def setFields(self, obj):
        self.form.startDepth.setText(FreeCAD.Units.Quantity(obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        if PathOp.FeatureStepDown & self.features:
            self.form.stepDown.setText(FreeCAD.Units.Quantity(obj.StepDown.Value, FreeCAD.Units.Length).UserString)
        if PathOp.FeatureFinishDepth & self.features:
            self.form.finishDepth.setText(FreeCAD.Units.Quantity(obj.FinishDepth.Value, FreeCAD.Units.Length).UserString)
    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.startDepth.editingFinished)
        signals.append(self.form.finalDepth.editingFinished)
        if PathOp.FeatureStepDown & self.features:
            signals.append(self.form.stepDown.editingFinished)
        if PathOp.FeatureFinishDepth & self.features:
            signals.append(self.form.finishDepth.editingFinished)
        return signals

    def pageUpdateData(self, obj, prop):
        if prop in ['StartDepth', 'FinalDepth', 'StepDown', 'FinishDepth']:
            self.setFields(obj)

class TaskPanel(object):

    def __init__(self, obj, deleteOnReject, opPage, selectionFactory):
        PathLog.track(obj.Label, deleteOnReject, opPage, selectionFactory)
        FreeCAD.ActiveDocument.openTransaction(translate("Path_AreaOp", "AreaOp Operation"))
        self.deleteOnReject = deleteOnReject
        self.featurePages = []

        features = obj.Proxy.opFeatures(obj)
        opPage.features = features

        if PathOp.FeatureBaseGeometry & features:
            if hasattr(opPage, 'taskPanelBaseGeometryPage'):
                self.featurePages.append(opPage.taskPanelBaseGeometryPage(obj, features))
            else:
                self.featurePages.append(TaskPanelBaseGeometryPage(obj, features))

        if PathOp.FeatureLocations & features:
            if hasattr(opPage, 'taskPanelBaseLocationPage'):
                self.featurePages.append(opPage.taskPanelBaseLocationPage(obj, features))
            else:
                self.featurePages.append(TaskPanelBaseLocationPage(obj, features))

        if PathOp.FeatureDepths & features:
            if hasattr(opPage, 'taskPanelDepthsPage'):
                self.featurePages.append(opPage.taskPanelDepthsPage(obj, features))
            else:
                self.featurePages.append(TaskPanelDepthsPage(obj, features))

        if PathOp.FeatureHeights & features:
            if hasattr(opPage, 'taskPanelHeightsPage'):
                self.featurePages.append(opPage.taskPanelHeightsPage(obj, features))
            else:
                self.featurePages.append(TaskPanelHeightsPage(obj, features))

        opPage.setTitle(translate('PathOp', 'Operation'))
        self.featurePages.append(opPage)

        for page in self.featurePages:
            page.initPage(obj)

        if TaskPanelLayout < 2:
            self.form = QtGui.QToolBox()
            if TaskPanelLayout == 0:
                for page in self.featurePages:
                    self.form.addItem(page.form, page.getTitle(obj))
                self.form.setCurrentIndex(len(self.featurePages)-1)
            else:
                for page in reversed(self.featurePages):
                    self.form.addItem(page.form, page.getTitle(obj))
        elif TaskPanelLayout == 2:
            forms = []
            for page in self.featurePages:
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms
        elif TaskPanelLayout == 3:
            forms = []
            for page in reversed(self.featurePages):
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms

        self.selectionFactory = selectionFactory
        self.obj = obj
        self.isdirty = True

    def isDirty(self):
        for page in self.featurePages:
            if page.isdirty:
                return True
        return self.isdirty

    def setClean(self):
        self.isdirty = False
        for page in self.featurePages:
            page.setClean()

    def accept(self):
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup()
        if self.isDirty:
            self.panelGetFields()
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate("Path_AreaOp", "Uncreate AreaOp Operation"))
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def cleanup(self):
        self.obj.ViewObject.Proxy.clearTaskPanel()
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Selection.removeObserver(self.s)

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.panelGetFields()
            self.setClean()
            FreeCAD.ActiveDocument.recompute()

    def modifyStandardButtons(self, buttonBox):
        for page in self.featurePages:
            page.modifyStandardButtons(buttonBox)

    def panelGetFields(self):
        PathLog.track()
        for page in self.featurePages:
            page.pageGetFields()

    def panelSetFields(self):
        PathLog.track()
        for page in self.featurePages:
            page.pageSetFields()

    def open(self):
        self.s = SelObserver(self.selectionFactory)
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)

    def setupUi(self):
        PathLog.track(self.deleteOnReject)

        if self.deleteOnReject and PathOp.FeatureBaseGeometry & self.obj.Proxy.opFeatures(self.obj):
            sel = FreeCADGui.Selection.getSelectionEx()
            if len(sel) == 1 and sel[0].Object != self.obj:
                for page in self.featurePages:
                    if hasattr(page, 'addBase'):
                        page.addBaseGeometry(sel)

        self.panelSetFields()
        for page in self.featurePages:
            page.pageRegisterSignalHandlers()

    def updateData(self, obj, prop):
        # PathLog.track(obj.Label, prop) # creates a lot of noise
        for page in self.featurePages:
            page.pageUpdateData(obj, prop)

    def needsFullSpace(self):
        return True

class SelObserver:

    def __init__(self, factory):
        factory()

    def __del__(self):
        PathSelection.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

class _CommandSetStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_AreaOp", "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_AreaOp", "Pick Start Point")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        obj = sel[0]
        return obj and hasattr(obj, 'StartPoint')

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        obj.StartPoint.z = obj.ClearanceHeight.Value

    def Activated(self):
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)

def Create(res):
    FreeCAD.ActiveDocument.openTransaction("Create %s" % res.name)
    obj  = res.objFactory(res.name)
    vobj = ViewProvider(obj.ViewObject, res)

    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.startEditing()
    return obj

class CommandPathOp:
    def __init__(self, resources):
        self.res = resources

    def GetResources(self):
        ress = {'Pixmap':   self.res.pixmap,
                'MenuText': self.res.menuText,
                'ToolTip':  self.res.toolTip}
        if self.res.accelKey:
                ress['Accel'] = self.res.accelKey
        return ress

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        return Create(self.res)

class CommandResources:
    def __init__(self, name, objFactory, opPageClass, pixmap, menuText, accelKey, toolTip):
        self.name = name
        self.objFactory = objFactory
        self.opPageClass = opPageClass
        self.pixmap = pixmap
        self.menuText = menuText
        self.accelKey = accelKey
        self.toolTip = toolTip

def SetupOperation(name,
        objFactory,
        opPageClass,
        pixmap,
        menuText,
        toolTip,
        accelKey = None):
    res = CommandResources(name, objFactory, opPageClass, pixmap, menuText, accelKey, toolTip)

    FreeCADGui.addCommand("Path_%s" % name.replace(' ', '_'), CommandPathOp(res))


FreeCADGui.addCommand('Set_StartPoint', _CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathOpGui... done\n")
