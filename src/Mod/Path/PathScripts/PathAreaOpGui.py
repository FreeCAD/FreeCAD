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
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathContour as PathContour
import PathScripts.PathLog as PathLog
import PathScripts.PathSelection as PathSelection

from PathScripts import PathUtils
from PySide import QtCore, QtGui

# TaskPanelLayout
#  0 ... existing toolbox layout
#  1 ... reverse order
#  2 ... multi panel layout
TaskPanelLayout = 2


PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider(object):

    def __init__(self, vobj):
        PathLog.track()
        vobj.Proxy = self

        #for sel in FreeCADGui.Selection.getSelectionEx():
        #    if sel[0].HasSubObjects:

    def attach(self, vobj):
        PathLog.track()
        self.Object = vobj.Object
        return

    def deleteObjectsOnReject(self):
        PathLog.track()
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        PathLog.track()
        FreeCADGui.Control.closeDialog()
        page = self.getTaskPanelOpPage(vobj.Object)
        selection = self.getSelectionFactory()
        taskd = TaskPanel(vobj.Object, self.deleteObjectsOnReject(), page, selection)
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        self.deleteOnReject = False
        return True

    def getIcon(self):
        return ":/icons/Path-Contour.svg"

    def __getstate__(self):
        PathLog.track()
        return None

    def __setstate__(self, state):
        PathLog.track()
        return None

class TaskPanelPage(object):

    # task panel interaction framework
    def __init__(self, obj):
        self.obj = obj
        self.initPage(obj)
        self.form = self.getForm()
        self.setDirty()
        self.setTitle('-')

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

    def setTitle(self, title):
        self.title = title
    def getTitle(self, obj):
        return self.title

    # subclass interface
    def initPage(self, obj):
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

class TaskPanelBaseGeometryPage(TaskPanelPage):
    DataObject    = QtCore.Qt.ItemDataRole.UserRole
    DataObjectSub = QtCore.Qt.ItemDataRole.UserRole + 1

    def initPage(self, obj):
        self.supports = PathAreaOp.FeatureBaseGeometry

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageBaseGeometryEdit.ui")
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

    def supportsEdges(self):
        return self.supports & PathAreaOp.FeatureBaseEdges
    def supportsFaces(self):
        return self.supports & PathAreaOp.FeatureBaseFaces
    def featureName(self):
        if self.supportsEdges() and self.supportsFaces():
            return 'features'
        if self.supportsFaces():
            return 'faces'
        if self.supportsEdges():
            return 'edges'
        return 'nothing'

    def addBaseGeometry(self, selection):
        if len(selection) != 1:
            PathLog.error(translate("PathProject", "Please select %s from a single solid" % self.featureName()))
            return False
        sel = selection[0]
        if not sel.HasSubObjects:
            PathLog.error(translate("PathProject", "Please select %s of a solid" % self.featureName()))
            return False
        if not self.supportsEdges() and selection[0].SubObjects[0].ShapeType == "Edge":
            PathLog.error(translate("PathProject", "Please select only %s of a solid" % self.featureName()))
            return False
        if not self.supportsFaces() and selection[0].SubObjects[0].ShapeType == "Face":
            PathLog.error(translate("PathProject", "Please select only %s of a solid" % self.featureName()))
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

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.updateBase.clicked.connect(self.updateBase)

class TaskPanelHeightsPage(TaskPanelPage):
    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageHeightsEdit.ui")
    def getTitle(self, obj):
        return translate("Path_AreaOp", "Heights")
    def getFields(self, obj):
        obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
        obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
    def setFields(self,  obj):
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
    def getSignalsForUpdate(self, obj):
        return [self.form.safeHeight.editingFinished, self.form.clearanceHeight.editingFinished]

class TaskPanelDepthsWoFinishPage(TaskPanelPage):
    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageDepthsEdit.ui")
        return self.setupForm(form)

    def setupForm(self, form):
        form.finishDepth.hide()
        form.finishDepthLabel.hide()
        return form

    def getTitle(self, obj):
        return translate("PathAreaOp", "Depths")
    def getFields(self, obj):
        obj.StartDepth = FreeCAD.Units.Quantity(self.form.startDepth.text()).Value
        obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
        obj.StepDown = FreeCAD.Units.Quantity(self.form.stepDown.text()).Value
    def setFields(self, obj):
        self.form.startDepth.setText(FreeCAD.Units.Quantity(obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.stepDown.setText(FreeCAD.Units.Quantity(obj.StepDown.Value, FreeCAD.Units.Length).UserString)
    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.startDepth.editingFinished)
        signals.append(self.form.finalDepth.editingFinished)
        signals.append(self.form.stepDown.editingFinished)
        return signals

class TaskPanelDepthsPage(TaskPanelDepthsWoFinishPage):
    def setupForm(self, form):
        form.finishDepth.setEnabled(True)
        form.finishDepthLabel.setEnabled(True)
        return form
    def getFields(self, obj):
        super(self.__class__, self).getFields(obj)
        obj.FinishDepth = FreeCAD.Units.Quantity(self.form.finishDepth.text()).Value
    def setFields(self, obj):
        super(self.__class__, self).setFields(obj)
        self.form.finishDepth.setText(FreeCAD.Units.Quantity(obj.FinishDepth.Value, FreeCAD.Units.Length).UserString)
    def getSignalsForUpdate(self, obj):
        signals = super(self.__class__, self).getSignalsForUpdate(obj)
        signals.append(self.form.finishDepth.editingFinished)
        return signals

class TaskPanel(object):

    def __init__(self, obj, deleteOnReject, opPage, selectionFactory):
        PathLog.track(obj.Label, deleteOnReject, opPage, selectionFactory)
        FreeCAD.ActiveDocument.openTransaction(translate("Path_AreaOp", "AreaOp Operation"))
        self.deleteOnReject = deleteOnReject
        self.featurePages = []

        if PathAreaOp.FeatureBaseGeometry & obj.Proxy.opFeatures(obj):
            basePage = TaskPanelBaseGeometryPage(obj)
            basePage.supports = obj.Proxy.opFeatures(obj) & PathAreaOp.FeatureBaseGeometry
            self.featurePages.append(basePage)

        if PathAreaOp.FeatureDepths & obj.Proxy.opFeatures(obj):
            if PathAreaOp.FeatureFinishDepth & obj.Proxy.opFeatures(obj):
                depthPage = TaskPanelDepthsPage(obj)
            else:
                depthPage = TaskPanelDepthsWoFinishPage(obj)
            self.featurePages.append(depthPage)

        if PathAreaOp.FeatureHeights & obj.Proxy.opFeatures(obj):
            self.featurePages.append(TaskPanelHeightsPage(obj))

        opPage.setTitle(translate('PathAreaOp', 'Operation'))
        self.featurePages.append(opPage)

        if TaskPanelLayout < 2:
            self.form = QtGui.QToolBox()
            if TaskPanelLayout == 0:
                for page in self.featurePages:
                    self.form.addItem(page.form, page.getTitle(obj))
                self.form.setCurrentIndex(len(self.featurePages)-1)
            else:
                for page in reversed(self.featurePages):
                    self.form.addItem(page.form, page.getTitle(obj))
        else:
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
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Selection.removeObserver(self.s)
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate("Path_AreaOp", "Uncreate AreaOp Operation"))
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.panelGetFields()
            self.setClean()
            FreeCAD.ActiveDocument.recompute()

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
        PathLog.track()

        if self.deleteOnReject and PathAreaOp.FeatureBaseGeometry & self.obj.Proxy.opFeatures(self.obj):
            sel = FreeCADGui.Selection.getSelectionEx()
            if len(sel) == 1 and sel[0].Object != self.obj:
                for page in self.featurePages:
                    if hasattr(page, 'addBase'):
                        page.addBaseGeometry(sel)

        self.panelSetFields()
        for page in self.featurePages:
            page.pageRegisterSignalHandlers()

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

FreeCADGui.addCommand('Set_StartPoint', _CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathAreaOpGui... done\n")
