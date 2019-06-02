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

import Draft
import DraftVecUtils
import FreeCAD
import FreeCADGui
import PathScripts.PathJob as PathJob
import PathScripts.PathJobCmd as PathJobCmd
import PathScripts.PathJobDlg as PathJobDlg
import PathScripts.PathGeom as PathGeom
import PathScripts.PathGui as PathGui
import PathScripts.PathGuiInit as PathGuiInit
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSetupSheetGui as PathSetupSheetGui
import PathScripts.PathStock as PathStock
import PathScripts.PathToolController as PathToolController
import PathScripts.PathToolLibraryManager as PathToolLibraryManager
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import math
import sys
import traceback

from PySide import QtCore, QtGui
from collections import Counter
from contextlib import contextmanager
from pivy import coin

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def _OpenCloseResourceEditor(obj, vobj, edit):
    job = PathUtils.findParentJob(obj)
    if job and job.ViewObject and job.ViewObject.Proxy:
        if edit:
            job.ViewObject.Proxy.editObject(obj)
        else:
            job.ViewObject.Proxy.uneditObject(obj)
    else:
        missing = 'Job'
        if job:
            missing = 'ViewObject'
            if job.ViewObject:
                missing = 'Proxy'
        PathLog.warning("Cannot edit %s - no %s" % (obj.Label, missing))

@contextmanager
def selectionEx():
    sel = FreeCADGui.Selection.getSelectionEx()
    try:
        yield sel
    finally:
        FreeCADGui.Selection.clearSelection()
        for s in sel:
            if s.SubElementNames:
                FreeCADGui.Selection.addSelection(s.Object, s.SubElementNames)
            else:
                FreeCADGui.Selection.addSelection(s.Object)


class ViewProvider:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        self.deleteOnReject = True

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.taskPanel = None

        # setup the axis display at the origin
        self.switch = coin.SoSwitch()
        self.sep = coin.SoSeparator()
        self.axs = coin.SoType.fromName('SoAxisCrossKit').createInstance()
        self.axs.set('xHead.transform', 'scaleFactor 2 3 2')
        self.axs.set('yHead.transform', 'scaleFactor 2 3 2')
        self.axs.set('zHead.transform', 'scaleFactor 2 3 2')
        self.sca = coin.SoType.fromName('SoShapeScale').createInstance()
        self.sca.setPart('shape', self.axs)
        self.sca.scaleFactor.setValue(0.5)
        self.mat = coin.SoMaterial()
        self.mat.diffuseColor = coin.SbColor(0.9, 0, 0.9)
        self.mat.transparency = 0.85
        self.sph = coin.SoSphere()
        self.scs = coin.SoType.fromName('SoShapeScale').createInstance()
        self.scs.setPart('shape', self.sph)
        self.scs.scaleFactor.setValue(10)
        self.sep.addChild(self.sca)
        self.sep.addChild(self.mat)
        self.sep.addChild(self.scs)
        self.switch.addChild(self.sep)
        vobj.RootNode.addChild(self.switch)
        self.showOriginAxis(False)

    def showOriginAxis(self, yes):
        sw = coin.SO_SWITCH_ALL if yes else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj=None, mode=0):
        PathLog.track(mode)
        if 0 == mode:
            self.openTaskPanel()
        return True

    def openTaskPanel(self, activate=None):
        self.taskPanel = TaskPanel(self.vobj, self.deleteObjectsOnReject())
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.taskPanel)
        self.taskPanel.setupUi(activate)
        self.deleteOnReject = False
        self.showOriginAxis(True)

    def resetTaskPanel(self):
        self.showOriginAxis(False)
        self.taskPanel = None

    def unsetEdit(self, arg1, arg2):
        if self.taskPanel:
            self.taskPanel.reject(False)

    def editObject(self, obj):
        if obj:
            if obj in self.obj.Model.Group:
                return self.openTaskPanel('Model')
            if obj == self.obj.Stock:
                return self.openTaskPanel('Stock')
            PathLog.info("Expected a specific object to edit - %s not recognized" % obj.Label)
        return self.openTaskPanel()

    def uneditObject(self, obj = None):
        self.unsetEdit(None, None)

    def getIcon(self):
        return ":/icons/Path-Job.svg"

    def claimChildren(self):
        children = self.obj.ToolController
        children.append(self.obj.Operations)
        if hasattr(self.obj, 'Model'):
            # unfortunately this function is called before the object has been fully loaded
            # which means we could be dealing with an old job which doesn't have the new Model
            # yet.
            children.append(self.obj.Model)
        if self.obj.Stock:
            children.append(self.obj.Stock)
        if hasattr(self.obj, 'SetupSheet'):
            # when loading a job that didn't have a setup sheet they might not've been created yet
            children.append(self.obj.SetupSheet)
        return children

    def onDelete(self, vobj, arg2=None):
        PathLog.track(vobj.Object.Label, arg2)
        self.obj.Proxy.onDelete(self.obj, arg2)
        return True

    def updateData(self, obj, prop):
        PathLog.track(obj.Label, prop)
        # make sure the resource view providers are setup properly
        if prop == 'Model' and self.obj.Model:
            for base in self.obj.Model.Group:
                if base.ViewObject and base.ViewObject.Proxy and not PathJob.isArchPanelSheet(base):
                    base.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)
        if prop == 'Stock' and self.obj.Stock and self.obj.Stock.ViewObject and self.obj.Stock.ViewObject.Proxy:
            self.obj.Stock.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)

    def rememberBaseVisibility(self, obj, base):
        if base.ViewObject:
            orig = PathUtil.getPublicObject(obj.Proxy.baseObject(obj, base))
            self.baseVisibility[base.Name] = (base, base.ViewObject.Visibility, orig, orig.ViewObject.Visibility)
            orig.ViewObject.Visibility = False
            base.ViewObject.Visibility = True

    def forgetBaseVisibility(self, obj, base):
        if self.baseVisibility.get(base.Name):
            visibility = self.baseVisibility[base.Name]
            visibility[0].ViewObject.Visibility = visibility[1]
            visibility[2].ViewObject.Visibility = visibility[3]
            del self.baseVisibility[base.Name]

    def setupEditVisibility(self, obj):
        self.baseVisibility = {}
        for base in obj.Model.Group:
            self.rememberBaseVisibility(obj, base)

        self.stockVisibility = False
        if obj.Stock and obj.Stock.ViewObject:
            self.stockVisibility = obj.Stock.ViewObject.Visibility
            self.obj.Stock.ViewObject.Visibility = True

    def resetEditVisibility(self, obj):
        for base in obj.Model.Group:
            self.forgetBaseVisibility(obj, base)
        if obj.Stock and obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = self.stockVisibility

    def setupContextMenu(self, vobj, menu):
        PathLog.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate('Path', 'Edit'), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)

class StockEdit(object):
    Index = -1
    StockType = PathStock.StockType.Unknown

    def __init__(self, obj, form, force):
        PathLog.track(obj.Label, force)
        self.obj = obj
        self.form = form
        self.force = force
        self.setupUi(obj)

    @classmethod
    def IsStock(cls, obj):
        return PathStock.StockType.FromStock(obj.Stock) == cls.StockType

    def activate(self, obj, select = False):
        PathLog.track(obj.Label, select)
        def showHide(widget, activeWidget):
            if widget == activeWidget:
                widget.show()
            else:
                widget.hide()
        if select:
            self.form.stock.setCurrentIndex(self.Index)
        editor = self.editorFrame()
        showHide(self.form.stockFromExisting, editor)
        showHide(self.form.stockFromBase, editor)
        showHide(self.form.stockCreateBox, editor)
        showHide(self.form.stockCreateCylinder, editor)
        self.setFields(obj)

    def setStock(self, obj, stock):
        PathLog.track(obj.Label, stock)
        if obj.Stock:
            PathLog.track(obj.Stock.Name)
            obj.Document.removeObject(obj.Stock.Name)
        PathLog.track(stock.Name)
        obj.Stock = stock
        if stock.ViewObject and stock.ViewObject.Proxy:
            stock.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)

    def setLengthField(self, widget, prop):
        widget.setText(FreeCAD.Units.Quantity(prop.Value, FreeCAD.Units.Length).UserString)

    # the following members must be overwritten by subclasses
    def editorFrame(self):
        return None
    def setFields(self, obj):
        pass
    def setupUi(self, obj):
        pass

class StockFromBaseBoundBoxEdit(StockEdit):
    Index = 2
    StockType = PathStock.StockType.FromBase

    def editorFrame(self):
        PathLog.track()
        return self.form.stockFromBase

    def getFieldsStock(self, stock, fields = ['xneg', 'xpos', 'yneg', 'ypos', 'zneg', 'zpos']):
        try:
            if 'xneg' in fields:
                stock.ExtXneg = FreeCAD.Units.Quantity(self.form.stockExtXneg.text())
            if 'xpos' in fields:
                stock.ExtXpos = FreeCAD.Units.Quantity(self.form.stockExtXpos.text())
            if 'yneg' in fields:
                stock.ExtYneg = FreeCAD.Units.Quantity(self.form.stockExtYneg.text())
            if 'ypos' in fields:
                stock.ExtYpos = FreeCAD.Units.Quantity(self.form.stockExtYpos.text())
            if 'zneg' in fields:
                stock.ExtZneg = FreeCAD.Units.Quantity(self.form.stockExtZneg.text())
            if 'zpos' in fields:
                stock.ExtZpos = FreeCAD.Units.Quantity(self.form.stockExtZpos.text())
        except:
            pass

    def getFields(self, obj, fields = ['xneg', 'xpos', 'yneg', 'ypos', 'zneg', 'zpos']):
        PathLog.track(obj.Label, fields)
        if self.IsStock(obj):
            self.getFieldsStock(obj.Stock, fields)
        else:
            PathLog.error(translate('PathJob', 'Stock not from Base bound box!'))

    def setFields(self, obj):
        PathLog.track()
        if self.force or not self.IsStock(obj):
            PathLog.track()
            stock = PathStock.CreateFromBase(obj)
            if self.force and self.editorFrame().isVisible():
                self.getFieldsStock(stock)
            self.setStock(obj, stock)
            self.force = False
        self.setLengthField(self.form.stockExtXneg, obj.Stock.ExtXneg)
        self.setLengthField(self.form.stockExtXpos, obj.Stock.ExtXpos)
        self.setLengthField(self.form.stockExtYneg, obj.Stock.ExtYneg)
        self.setLengthField(self.form.stockExtYpos, obj.Stock.ExtYpos)
        self.setLengthField(self.form.stockExtZneg, obj.Stock.ExtZneg)
        self.setLengthField(self.form.stockExtZpos, obj.Stock.ExtZpos)

    def setupUi(self, obj):
        PathLog.track()
        self.setFields(obj)
        self.checkXpos()
        self.checkYpos()
        self.checkZpos()
        self.form.stockExtXneg.textChanged.connect(self.updateXpos)
        self.form.stockExtYneg.textChanged.connect(self.updateYpos)
        self.form.stockExtZneg.textChanged.connect(self.updateZpos)
        self.form.stockExtXpos.textChanged.connect(self.checkXpos)
        self.form.stockExtYpos.textChanged.connect(self.checkYpos)
        self.form.stockExtZpos.textChanged.connect(self.checkZpos)

    def checkXpos(self):
        self.trackXpos = self.form.stockExtXneg.text() == self.form.stockExtXpos.text()
        self.getFields(self.obj, ['xpos'])
    def checkYpos(self):
        self.trackYpos = self.form.stockExtYneg.text() == self.form.stockExtYpos.text()
        self.getFields(self.obj, ['ypos'])
    def checkZpos(self):
        self.trackZpos = self.form.stockExtZneg.text() == self.form.stockExtZpos.text()
        self.getFields(self.obj, ['zpos'])

    def updateXpos(self):
        fields = ['xneg']
        if self.trackXpos:
            self.form.stockExtXpos.setText(self.form.stockExtXneg.text())
            fields.append('xpos')
        self.getFields(self.obj, fields)
    def updateYpos(self):
        fields = ['yneg']
        if self.trackYpos:
            self.form.stockExtYpos.setText(self.form.stockExtYneg.text())
            fields.append('ypos')
        self.getFields(self.obj, fields)
    def updateZpos(self):
        fields = ['zneg']
        if self.trackZpos:
            self.form.stockExtZpos.setText(self.form.stockExtZneg.text())
            fields.append('zpos')
        self.getFields(self.obj, fields)

class StockCreateBoxEdit(StockEdit):
    Index = 0
    StockType = PathStock.StockType.CreateBox

    def editorFrame(self):
        return self.form.stockCreateBox

    def getFields(self, obj, fields = ['length', 'widht', 'height']):
        try:
            if self.IsStock(obj):
                if 'length' in fields:
                    obj.Stock.Length = FreeCAD.Units.Quantity(self.form.stockBoxLength.text())
                if 'width' in fields:
                    obj.Stock.Width  = FreeCAD.Units.Quantity(self.form.stockBoxWidth.text())
                if 'height' in fields:
                    obj.Stock.Height = FreeCAD.Units.Quantity(self.form.stockBoxHeight.text())
            else:
                PathLog.error(translate('PathJob', 'Stock not a box!'))
        except:
            pass

    def setFields(self, obj):
        if self.force or not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateBox(obj))
            self.force = False
        self.setLengthField(self.form.stockBoxLength, obj.Stock.Length)
        self.setLengthField(self.form.stockBoxWidth,  obj.Stock.Width)
        self.setLengthField(self.form.stockBoxHeight, obj.Stock.Height)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockBoxLength.textChanged.connect(lambda: self.getFields(obj, ['length']))
        self.form.stockBoxWidth.textChanged.connect(lambda:  self.getFields(obj, ['width']))
        self.form.stockBoxHeight.textChanged.connect(lambda: self.getFields(obj, ['height']))

class StockCreateCylinderEdit(StockEdit):
    Index = 1
    StockType = PathStock.StockType.CreateCylinder

    def editorFrame(self):
        return self.form.stockCreateCylinder

    def getFields(self, obj, fields = ['radius', 'height']):
        try:
            if self.IsStock(obj):
                if 'radius' in fields:
                    obj.Stock.Radius = FreeCAD.Units.Quantity(self.form.stockCylinderRadius.text())
                if 'height' in fields:
                    obj.Stock.Height = FreeCAD.Units.Quantity(self.form.stockCylinderHeight.text())
            else:
                PathLog.error(translate('PathJob', 'Stock not a cylinder!'))
        except:
            pass

    def setFields(self, obj):
        if self.force or not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateCylinder(obj))
            self.force = False
        self.setLengthField(self.form.stockCylinderRadius, obj.Stock.Radius)
        self.setLengthField(self.form.stockCylinderHeight, obj.Stock.Height)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockCylinderRadius.textChanged.connect(lambda: self.getFields(obj, ['radius']))
        self.form.stockCylinderHeight.textChanged.connect(lambda: self.getFields(obj, ['height']))

class StockFromExistingEdit(StockEdit):
    Index = 3
    StockType = PathStock.StockType.Unknown

    def editorFrame(self):
        return self.form.stockFromExisting

    def getFields(self, obj):
        stock = self.form.stockExisting.itemData(self.form.stockExisting.currentIndex())
        if not (hasattr(obj.Stock, 'Objects') and len(obj.Stock.Objects) == 1 and obj.Stock.Objects[0] == stock): 
            if stock:
                stock = PathJob.createResourceClone(obj, stock, 'Stock', 'Stock')
                stock.ViewObject.Visibility = True
                PathStock.SetupStockObject(stock, PathStock.StockType.Unknown)
                stock.Proxy.execute(stock)
                self.setStock(obj, stock)

    def candidates(self, obj):
        solids = [o for o in obj.Document.Objects if PathUtil.isSolid(o)]
        for base in obj.Model.Group:
            if base in solids and PathJob.isResourceClone(obj, base, 'Model'):
                solids.remove(base)
        if obj.Stock in solids:
            # regardless, what stock is/was, it's not a valid choice
            solids.remove(obj.Stock)
        return sorted(solids, key=lambda c: c.Label)

    def setFields(self, obj):
        self.form.stockExisting.clear()
        stockName = obj.Stock.Label if obj.Stock else None
        index = -1
        for i, solid in enumerate(self.candidates(obj)):
            self.form.stockExisting.addItem(solid.Label, solid)
            if solid.Label == stockName:
                index = i
        self.form.stockExisting.setCurrentIndex(index if index != -1 else 0)

        if not self.IsStock(obj):
            self.getFields(obj)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockExisting.currentIndexChanged.connect(lambda: self.getFields(obj))

class TaskPanel:
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataProperty = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, vobj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Edit Job"))
        self.vobj = vobj
        self.vproxy = vobj.Proxy
        self.obj = vobj.Object
        self.deleteOnReject = deleteOnReject
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/PathEdit.ui")
        self.template = PathJobDlg.JobTemplateExport(self.obj, self.form.jobBox.widget(1))

        vUnit = FreeCAD.Units.Quantity(1, FreeCAD.Units.Velocity).getUserPreferred()[2]
        self.form.toolControllerList.horizontalHeaderItem(1).setText('#')
        self.form.toolControllerList.horizontalHeaderItem(2).setText(vUnit)
        self.form.toolControllerList.horizontalHeaderItem(3).setText(vUnit)
        self.form.toolControllerList.horizontalHeader().setResizeMode(0, QtGui.QHeaderView.Stretch)
        self.form.toolControllerList.resizeColumnsToContents()

        currentPostProcessor = self.obj.PostProcessor
        postProcessors = PathPreferences.allEnabledPostProcessors(['', currentPostProcessor])
        for post in postProcessors:
            self.form.postProcessor.addItem(post)
        # update the enumeration values, just to make sure all selections are valid
        self.obj.PostProcessor = postProcessors
        self.obj.PostProcessor = currentPostProcessor

        self.postProcessorDefaultTooltip = self.form.postProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.postProcessorArguments.toolTip()

        self.vproxy.setupEditVisibility(self.obj)

        self.stockFromBase = None
        self.stockFromExisting = None
        self.stockCreateBox = None
        self.stockCreateCylinder = None
        self.stockEdit = None

        self.setupGlobal = PathSetupSheetGui.GlobalEditor(self.obj.SetupSheet, self.form)
        self.setupOps = PathSetupSheetGui.OpsDefaultEditor(self.obj.SetupSheet, self.form)

    def preCleanup(self):
        PathLog.track()
        FreeCADGui.Selection.removeObserver(self)
        self.vproxy.resetEditVisibility(self.obj)
        self.vproxy.resetTaskPanel()

    def accept(self, resetEdit=True):
        PathLog.track()
        self.preCleanup()
        self.getFields()
        self.setupGlobal.accept()
        self.setupOps.accept()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)

    def reject(self, resetEdit=True):
        PathLog.track()
        self.preCleanup()
        self.setupGlobal.reject()
        self.setupOps.reject()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject:
            PathLog.info("Uncreate Job")
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Uncreate Job"))
            if self.obj.ViewObject.Proxy.onDelete(self.obj.ViewObject, None):
                FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)
        return True

    def cleanup(self, resetEdit):
        PathLog.track()
        FreeCADGui.Control.closeDialog()
        if resetEdit:
            FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()

    def updateTooltips(self):
        if hasattr(self.obj, "Proxy") and hasattr(self.obj.Proxy, "tooltip") and self.obj.Proxy.tooltip:
            self.form.postProcessor.setToolTip(self.obj.Proxy.tooltip)
            if hasattr(self.obj.Proxy, "tooltipArgs") and self.obj.Proxy.tooltipArgs:
                self.form.postProcessorArguments.setToolTip(self.obj.Proxy.tooltipArgs)
            else:
                self.form.postProcessorArguments.setToolTip(self.postProcessorArgsDefaultTooltip)
        else:
            self.form.postProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.postProcessorArguments.setToolTip(self.postProcessorArgsDefaultTooltip)

    def getFields(self):
        '''sets properties in the object to match the form'''
        if self.obj:
            self.obj.PostProcessor = str(self.form.postProcessor.currentText())
            self.obj.PostProcessorArgs = str(self.form.postProcessorArguments.displayText())
            self.obj.PostProcessorOutputFile = str(self.form.postProcessorOutputFile.text())

            self.obj.Label = str(self.form.jobLabel.text())
            self.obj.Description = str(self.form.jobDescription.toPlainText())
            self.obj.Operations.Group = [self.form.operationsList.item(i).data(self.DataObject) for i in range(self.form.operationsList.count())]
            try:
                self.obj.SplitOutput = self.form.splitOutput.isChecked()
                self.obj.OrderOutputBy = str(self.form.orderBy.currentText())

                flist = []
                for i in range(self.form.wcslist.count()):
                    if self.form.wcslist.item(i).checkState() == QtCore.Qt.CheckState.Checked:
                        flist.append(self.form.wcslist.item(i).text())
                self.obj.Fixtures = flist
            except:
                FreeCAD.Console.PrintWarning("The Job was created without fixture support.  Please delete and recreate the job\r\n") 

            self.updateTooltips()
            self.stockEdit.getFields(self.obj)

            self.obj.Proxy.execute(self.obj)

        self.setupGlobal.getFields()
        self.setupOps.getFields()

    def selectComboBoxText(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def updateToolController(self):
        tcRow = self.form.toolControllerList.currentRow()
        tcCol = self.form.toolControllerList.currentColumn()

        self.form.toolControllerList.blockSignals(True)
        self.form.toolControllerList.clearContents()
        self.form.toolControllerList.setRowCount(0)

        self.form.activeToolController.blockSignals(True)
        index = self.form.activeToolController.currentIndex()
        select = None if index == -1 else self.form.activeToolController.itemData(index)
        self.form.activeToolController.clear()

        vUnit = FreeCAD.Units.Quantity(1, FreeCAD.Units.Velocity).getUserPreferred()[2]

        for row,tc in enumerate(sorted(self.obj.ToolController, key=lambda tc: tc.Label)):
            self.form.activeToolController.addItem(tc.Label, tc)
            if tc == select:
                index = row

            self.form.toolControllerList.insertRow(row)

            item = QtGui.QTableWidgetItem(tc.Label)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'Label')
            self.form.toolControllerList.setItem(row, 0, item)

            item = QtGui.QTableWidgetItem("%d" % tc.ToolNumber)
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'Number')
            self.form.toolControllerList.setItem(row, 1, item)

            item = QtGui.QTableWidgetItem("%g" % tc.HorizFeed.getValueAs(vUnit))
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'HorizFeed')
            self.form.toolControllerList.setItem(row, 2, item)

            item = QtGui.QTableWidgetItem("%g" % tc.VertFeed.getValueAs(vUnit))
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'VertFeed')
            self.form.toolControllerList.setItem(row, 3, item)

            item = QtGui.QTableWidgetItem("%s%g" % ('+' if tc.SpindleDir == 'Forward' else '-', tc.SpindleSpeed))
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'Spindle')
            self.form.toolControllerList.setItem(row, 4, item)

        if index != -1:
            self.form.activeToolController.setCurrentIndex(index)
        if tcRow != -1 and tcCol != -1:
            self.form.toolControllerList.setCurrentCell(tcRow, tcCol)

        self.form.activeToolController.blockSignals(False)
        self.form.toolControllerList.blockSignals(False)

    def setFields(self):
        '''sets fields in the form to match the object'''

        self.form.jobLabel.setText(self.obj.Label)
        self.form.jobDescription.setPlainText(self.obj.Description)

        if hasattr(self.obj, "SplitOutput"):
            self.form.splitOutput.setChecked(self.obj.SplitOutput)
        if hasattr(self.obj, "OrderOutputBy"):
            self.selectComboBoxText(self.form.orderBy, self.obj.OrderOutputBy)

        if hasattr(self.obj, "Fixtures"):
            for f in self.obj.Fixtures:
                item = self.form.wcslist.findItems(f, QtCore.Qt.MatchExactly)[0]
                item.setCheckState(QtCore.Qt.Checked)


        self.form.postProcessorOutputFile.setText(self.obj.PostProcessorOutputFile)
        self.selectComboBoxText(self.form.postProcessor, self.obj.PostProcessor)
        self.form.postProcessorArguments.setText(self.obj.PostProcessorArgs)
        #self.obj.Proxy.onChanged(self.obj, "PostProcessor")
        self.updateTooltips()

        self.form.operationsList.clear()
        for child in self.obj.Operations.Group:
            item = QtGui.QListWidgetItem(child.Label)
            item.setData(self.DataObject, child)
            self.form.operationsList.addItem(item)

        self.form.jobModel.clear()
        for name, count in PathUtil.keyValueIter(Counter([self.obj.Proxy.baseObject(self.obj, o).Label for o in self.obj.Model.Group])):
            if count == 1:
                self.form.jobModel.addItem(name)
            else:
                self.form.jobModel.addItem("%s (%d)" % (name, count))

        self.updateToolController()
        self.stockEdit.setFields(self.obj)
        self.setupGlobal.setFields()
        self.setupOps.setFields()

    def setPostProcessorOutputFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("Path_Job", "Select Output File"), None, translate("Path_Job", "All Files (*.*)"))
        if filename and filename[0]:
            self.obj.PostProcessorOutputFile = str(filename[0])
            self.setFields()

    def operationSelect(self):
        if self.form.operationsList.selectedItems():
            self.form.operationModify.setEnabled(True)
            self.form.operationMove.setEnabled(True)
            row = self.form.operationsList.currentRow()
            self.form.operationUp.setEnabled(row > 0)
            self.form.operationDown.setEnabled(row < self.form.operationsList.count() - 1)
        else:
            self.form.operationModify.setEnabled(False)
            self.form.operationMove.setEnabled(False)

    def objectDelete(self, widget):
        for item in widget.selectedItems():
            obj = item.data(self.DataObject)
            if obj.ViewObject and hasattr(obj.ViewObject, 'Proxy') and hasattr(obj.ViewObject.Proxy, 'onDelete'):
                obj.ViewObject.Proxy.onDelete(obj.ViewObject, None)
            FreeCAD.ActiveDocument.removeObject(obj.Name)
        self.setFields()

    def operationDelete(self):
        self.objectDelete(self.form.operationsList)

    def operationMoveUp(self):
        row = self.form.operationsList.currentRow()
        if row > 0:
            item = self.form.operationsList.takeItem(row)
            self.form.operationsList.insertItem(row-1, item)
            self.form.operationsList.setCurrentRow(row-1)
            self.getFields()

    def operationMoveDown(self):
        row = self.form.operationsList.currentRow()
        if row < self.form.operationsList.count() - 1:
            item = self.form.operationsList.takeItem(row)
            self.form.operationsList.insertItem(row+1, item)
            self.form.operationsList.setCurrentRow(row+1)
            self.getFields()

    def toolControllerSelect(self):
        def canDeleteTC(tc):
            # if the TC is referenced anywhere but the job we don't want to delete it
            return len(tc.InList) == 1

        # if anything is selected it can be edited
        edit = True if self.form.toolControllerList.selectedItems() else False
        self.form.toolControllerEdit.setEnabled(edit)

        # can only delete what is selected
        delete = edit
        # ... but we want to make sure there's at least one TC left
        if len(self.obj.ToolController) == len(self.form.toolControllerList.selectedItems()):
            delete = False
        # ... also don't want to delete any TCs that are already used
        if delete:
            for item in self.form.toolControllerList.selectedItems():
                if not canDeleteTC(item.data(self.DataObject)):
                    delete = False
                    break
        self.form.toolControllerDelete.setEnabled(delete)

    def toolControllerEdit(self):
        for item in self.form.toolControllerList.selectedItems():
            tc = item.data(self.DataObject)
            dlg = PathToolController.DlgToolControllerEdit(tc)
            dlg.exec_()
        self.setFields()
        self.toolControllerSelect()

    def toolControllerAdd(self):
        PathToolLibraryManager.CommandToolLibraryEdit().edit(self.obj, self.updateToolController)

    def toolControllerDelete(self):
        self.objectDelete(self.form.toolControllerList)

    def toolControllerChanged(self, item):
        tc = item.data(self.DataObject)
        prop = item.data(self.DataProperty)
        if 'Label' == prop:
            tc.Label = item.text()
            item.setText(tc.Label)
        elif 'Number' == prop:
            try:
                tc.ToolNumber = int(item.text())
            except:
                pass
            item.setText("%d" % tc.ToolNumber)
        elif 'Spindle' == prop:
            try:
                speed = float(item.text())
                rot = 'Forward'
                if speed < 0:
                    rot = 'Reverse'
                    speed = -speed
                tc.SpindleDir = rot
                tc.SpindleSpeed = speed
            except:
                pass
            item.setText("%s%g" % ('+' if tc.SpindleDir == 'Forward' else '-', tc.SpindleSpeed))
        elif 'HorizFeed' == prop or 'VertFeed' == prop:
            vUnit = FreeCAD.Units.Quantity(1, FreeCAD.Units.Velocity).getUserPreferred()[2]
            try:
                val = FreeCAD.Units.Quantity(item.text())
                if FreeCAD.Units.Velocity == val.Unit:
                    setattr(tc, prop, val)
                elif FreeCAD.Units.Unit() == val.Unit:
                    val = FreeCAD.Units.Quantity(item.text()+vUnit);
                    setattr(tc, prop, val)
            except:
                pass
            item.setText("%g" % getattr(tc, prop).getValueAs(vUnit))
        else:
            try:
                val = FreeCAD.Units.Quantity(item.text())
                setattr(tc, prop, val)
            except:
                pass
            item.setText("%g" % getattr(tc, prop).Value)

        self.template.updateUI()

    def modelSetAxis(self, axis):
        def flipSel(sel):
            PathLog.debug("flip")
            p = sel.Object.Placement
            loc = sel.Object.Placement.Base
            rot = FreeCAD.Rotation(FreeCAD.Vector(1-axis.x, 1-axis.y, 1-axis.z), 180)
            sel.Object.Placement = FreeCAD.Placement(loc, p.Rotation.multiply(rot))

        def rotateSel(sel, n):
            p = sel.Object.Placement
            loc = sel.Object.Placement.Base
            r = axis.cross(n) # rotation axis
            a = DraftVecUtils.angle(n, axis, r) * 180 / math.pi
            PathLog.debug("oh boy: (%.2f, %.2f, %.2f) -> %.2f" % (r.x, r.y, r.z, a))
            Draft.rotate(sel.Object, a, axis=r)

        selObject = None
        selFeature = None
        for sel in FreeCADGui.Selection.getSelectionEx():
            selObject = sel.Object
            for feature in sel.SubElementNames:
                selFeature = feature
                sub = sel.Object.Shape.getElement(feature)
                if 'Face' == sub.ShapeType:
                    n = sub.Surface.Axis
                    if sub.Orientation == 'Reversed':
                        n = FreeCAD.Vector() - n
                        PathLog.debug("(%.2f, %.2f, %.2f) -> reversed (%s)" % (n.x, n.y, n.z, sub.Orientation))
                    else:
                        PathLog.debug("(%.2f, %.2f, %.2f) -> forward  (%s)" % (n.x, n.y, n.z, sub.Orientation))

                    if PathGeom.pointsCoincide(axis, n):
                        PathLog.debug("face properly oriented (%.2f, %.2f, %.2f)" % (n.x, n.y, n.z))
                    else:
                        if PathGeom.pointsCoincide(axis, FreeCAD.Vector() - n):
                            flipSel(sel)
                        else:
                            rotateSel(sel, n)
                if 'Edge' == sub.ShapeType:
                    n = (sub.Vertexes[1].Point - sub.Vertexes[0].Point).normalize()
                    if PathGeom.pointsCoincide(axis, n) or PathGeom.pointsCoincide(axis, FreeCAD.Vector() - n):
                        # Don't really know the orientation of an edge, so let's just flip the object
                        # and if the user doesn't like it they can flip again
                        flipSel(sel)
                    else:
                        rotateSel(sel, n)
        if selObject and selFeature:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(selObject, selFeature)

    def restoreSelection(self, selection):
        FreeCADGui.Selection.clearSelection()
        for sel in selection:
            FreeCADGui.Selection.addSelection(sel.Object, sel.SubElementNames)

    def modelSet0(self, axis):
        with selectionEx() as selection:
            for sel in selection:
                model = sel.Object
                for name in sel.SubElementNames:
                    feature = model.Shape.getElement(name)
                    bb = feature.BoundBox
                    offset = FreeCAD.Vector(axis.x * bb.XMax, axis.y * bb.YMax, axis.z * bb.ZMax)
                    PathLog.track(feature.BoundBox.ZMax, offset)
                    p = model.Placement
                    p.move(offset)
                    model.Placement = p

    def modelMove(self, axis):
        scale = self.form.modelMoveValue.value()
        with selectionEx() as selection:
            for sel in selection:
                offset = axis * scale
                Draft.move(sel.Object, offset)

    def modelRotate(self, axis):
        angle = self.form.modelRotateValue.value()
        with selectionEx() as selection:
            if self.form.modelRotateCompound.isChecked() and len(selection) > 1:
                bb = PathStock.shapeBoundBox([sel.Object for sel in selection])
                for sel in selection:
                    Draft.rotate(sel.Object, angle, bb.Center, axis)
            else:
                for sel in selection:
                    Draft.rotate(sel.Object, angle, sel.Object.Shape.BoundBox.Center, axis)

    def alignSetOrigin(self):
        (obj, by) = self.alignMoveToOrigin()

        for base in self.obj.Model.Group:
            if base != obj:
                Draft.move(base, by)

        if obj != self.obj.Stock and self.obj.Stock:
            Draft.move(self.obj.Stock, by)

        placement = FreeCADGui.ActiveDocument.ActiveView.viewPosition()
        placement.Base = placement.Base + by
        FreeCADGui.ActiveDocument.ActiveView.viewPosition(placement, 0)

    def alignMoveToOrigin(self):
        selObject = None
        selFeature = None
        p = None
        for sel in FreeCADGui.Selection.getSelectionEx():
            selObject = sel.Object
            for feature in sel.SubElementNames:
                selFeature = feature
                sub = sel.Object.Shape.getElement(feature)
                if 'Vertex' == sub.ShapeType:
                    p = FreeCAD.Vector() - sub.Point
                    Draft.move(sel.Object, p)
        if selObject and selFeature:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(selObject, selFeature)
        return (selObject, p)

    def updateStockEditor(self, index, force = False):
        def setupFromBaseEdit():
            PathLog.track(index, force)
            if force or not self.stockFromBase:
                self.stockFromBase = StockFromBaseBoundBoxEdit(self.obj, self.form, force)
            self.stockEdit = self.stockFromBase
        def setupCreateBoxEdit():
            PathLog.track(index, force)
            if force or not self.stockCreateBox:
                self.stockCreateBox = StockCreateBoxEdit(self.obj, self.form, force)
            self.stockEdit = self.stockCreateBox
        def setupCreateCylinderEdit():
            PathLog.track(index, force)
            if force or not self.stockCreateCylinder:
                self.stockCreateCylinder = StockCreateCylinderEdit(self.obj, self.form, force)
            self.stockEdit = self.stockCreateCylinder
        def setupFromExisting():
            PathLog.track(index, force)
            if force or not self.stockFromExisting:
                self.stockFromExisting = StockFromExistingEdit(self.obj, self.form, force)
            if self.stockFromExisting.candidates(self.obj):
                self.stockEdit = self.stockFromExisting
                return True
            return False

        if index == -1:
            if self.obj.Stock is None or StockFromBaseBoundBoxEdit.IsStock(self.obj):
                setupFromBaseEdit()
            elif StockCreateBoxEdit.IsStock(self.obj):
                setupCreateBoxEdit()
            elif StockCreateCylinderEdit.IsStock(self.obj):
                setupCreateCylinderEdit()
            elif StockFromExistingEdit.IsStock(self.obj):
                setupFromExisting()
            else:
                PathLog.error(translate('PathJob', "Unsupported stock object %s") % self.obj.Stock.Label)
        else:
            if index == StockFromBaseBoundBoxEdit.Index:
                setupFromBaseEdit()
            elif index == StockCreateBoxEdit.Index:
                setupCreateBoxEdit()
            elif index == StockCreateCylinderEdit.Index:
                setupCreateCylinderEdit()
            elif index == StockFromExistingEdit.Index:
                if not setupFromExisting():
                    setupFromBaseEdit()
                    index = -1
            else:
                PathLog.error(translate('PathJob', "Unsupported stock type %s (%d)") % (self.form.stock.currentText(), index))
        self.stockEdit.activate(self.obj, index == -1)

        if -1 != index:
            self.template.updateUI()

    def refreshStock(self):
        self.updateStockEditor(self.form.stock.currentIndex(), True)

    def alignCenterInStock(self):
        bbs = self.obj.Stock.Shape.BoundBox
        for sel in FreeCADGui.Selection.getSelectionEx():
            bbb = sel.Object.Shape.BoundBox
            by = bbs.Center - bbb.Center
            Draft.move(sel.Object, by)

    def alignCenterInStockXY(self):
        bbs = self.obj.Stock.Shape.BoundBox
        for sel in FreeCADGui.Selection.getSelectionEx():
            bbb = sel.Object.Shape.BoundBox
            by = bbs.Center - bbb.Center
            by.z = 0
            Draft.move(sel.Object, by)

    def updateSelection(self):
        sel = FreeCADGui.Selection.getSelectionEx()

        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            if 'Vertex' == sel[0].SubObjects[0].ShapeType:
                self.form.modelSetXAxis.setEnabled(False)
                self.form.modelSetYAxis.setEnabled(False)
                self.form.modelSetZAxis.setEnabled(False)
                self.form.setOrigin.setEnabled(True)
                self.form.moveToOrigin.setEnabled(True)
            else:
                self.form.modelSetXAxis.setEnabled(True)
                self.form.modelSetYAxis.setEnabled(True)
                self.form.modelSetZAxis.setEnabled(True)
                self.form.setOrigin.setEnabled(False)
                self.form.moveToOrigin.setEnabled(False)
        else:
            self.form.modelSetXAxis.setEnabled(False)
            self.form.modelSetYAxis.setEnabled(False)
            self.form.modelSetZAxis.setEnabled(False)
            self.form.setOrigin.setEnabled(False)
            self.form.moveToOrigin.setEnabled(False)

        if len(sel) == 0 or self.obj.Stock in [s.Object for s in sel]:
            self.form.centerInStock.setEnabled(False)
            self.form.centerInStockXY.setEnabled(False)
        else:
            self.form.centerInStock.setEnabled(True)
            self.form.centerInStockXY.setEnabled(True)

        if len(sel) > 0:
            self.form.modelSetX0.setEnabled(True)
            self.form.modelSetY0.setEnabled(True)
            self.form.modelSetZ0.setEnabled(True)
            self.form.modelMoveGroup.setEnabled(True)
            self.form.modelRotateGroup.setEnabled(True)
            self.form.modelRotateCompound.setEnabled(len(sel) > 1)
        else:
            self.form.modelSetX0.setEnabled(False)
            self.form.modelSetY0.setEnabled(False)
            self.form.modelSetZ0.setEnabled(False)
            self.form.modelMoveGroup.setEnabled(False)
            self.form.modelRotateGroup.setEnabled(False)

    def jobModelEdit(self):
        dialog = PathJobDlg.JobCreate()
        dialog.setupTitle(translate("Path_Job", "Model Selection"))
        dialog.setupModel(self.obj)
        if dialog.exec_() == 1:
            models = dialog.getModels()
            if models:
                obj = self.obj
                proxy = obj.Proxy

                want = Counter(models)
                have = Counter([proxy.baseObject(obj, o) for o in obj.Model.Group])

                obsolete  = have - want
                additions = want - have

                # first remove all obsolete base models
                for model, count in PathUtil.keyValueIter(obsolete):
                    for i in range(count):
                        # it seems natural to remove the last of all the base objects for a given model
                        base = [b for b in obj.Model.Group if proxy.baseObject(obj, b) == model][-1]
                        self.vproxy.forgetBaseVisibility(obj, base)
                        self.obj.Proxy.removeBase(obj, base, True)
                # do not access any of the retired objects after this point, they don't exist anymore

                # then add all rookie base models
                for model, count in PathUtil.keyValueIter(additions):
                    for i in range(count):
                        base = PathJob.createModelResourceClone(obj, model)
                        obj.Model.addObject(base)
                        self.vproxy.rememberBaseVisibility(obj, base)

                # refresh the view
                if obsolete or additions:
                    self.setFields()
                else:
                    PathLog.track('no changes to model')


    def tabPageChanged(self, index):
        if index == 0:
            # update the template with potential changes
            self.getFields()
            self.setupGlobal.accept()
            self.setupOps.accept()
            self.obj.Document.recompute()
            self.template.updateUI()

    def setupUi(self, activate):
        self.setupGlobal.setupUi()
        self.setupOps.setupUi()
        self.updateStockEditor(-1, False)
        self.setFields()

        # Info
        self.form.jobLabel.editingFinished.connect(self.getFields)
        self.form.jobModelEdit.clicked.connect(self.jobModelEdit)

        # Post Processor
        self.form.postProcessor.currentIndexChanged.connect(self.getFields)
        self.form.postProcessorArguments.editingFinished.connect(self.getFields)
        self.form.postProcessorOutputFile.editingFinished.connect(self.getFields)
        self.form.postProcessorSetOutputFile.clicked.connect(self.setPostProcessorOutputFile)

        # Workplan
        self.form.operationsList.itemSelectionChanged.connect(self.operationSelect)
        self.form.operationsList.indexesMoved.connect(self.getFields)
        self.form.operationDelete.clicked.connect(self.operationDelete)
        self.form.operationUp.clicked.connect(self.operationMoveUp)
        self.form.operationDown.clicked.connect(self.operationMoveDown)

        self.form.operationEdit.hide() # not supported yet
        self.form.activeToolGroup.hide() # not supported yet

        # Tool controller
        self.form.toolControllerList.itemSelectionChanged.connect(self.toolControllerSelect)
        self.form.toolControllerList.itemChanged.connect(self.toolControllerChanged)
        self.form.toolControllerEdit.clicked.connect(self.toolControllerEdit)
        self.form.toolControllerDelete.clicked.connect(self.toolControllerDelete)
        self.form.toolControllerAdd.clicked.connect(self.toolControllerAdd)

        self.operationSelect()
        self.toolControllerSelect()

        # Stock, Orientation and Alignment
        self.form.centerInStock.clicked.connect(self.alignCenterInStock)
        self.form.centerInStockXY.clicked.connect(self.alignCenterInStockXY)

        self.form.stock.currentIndexChanged.connect(self.updateStockEditor)
        self.form.refreshStock.clicked.connect(self.refreshStock)

        self.form.modelSetXAxis.clicked.connect(lambda: self.modelSetAxis(FreeCAD.Vector(1, 0, 0)))
        self.form.modelSetYAxis.clicked.connect(lambda: self.modelSetAxis(FreeCAD.Vector(0, 1, 0)))
        self.form.modelSetZAxis.clicked.connect(lambda: self.modelSetAxis(FreeCAD.Vector(0, 0, 1)))
        self.form.modelSetX0.clicked.connect(lambda: self.modelSet0(FreeCAD.Vector(-1,  0,  0)))
        self.form.modelSetY0.clicked.connect(lambda: self.modelSet0(FreeCAD.Vector( 0, -1,  0)))
        self.form.modelSetZ0.clicked.connect(lambda: self.modelSet0(FreeCAD.Vector( 0,  0, -1)))

        self.form.setOrigin.clicked.connect(self.alignSetOrigin)
        self.form.moveToOrigin.clicked.connect(self.alignMoveToOrigin)

        self.form.modelMoveLeftUp.clicked.connect(      lambda: self.modelMove(FreeCAD.Vector(-1,  1, 0)))
        self.form.modelMoveLeft.clicked.connect(        lambda: self.modelMove(FreeCAD.Vector(-1,  0, 0)))
        self.form.modelMoveLeftDown.clicked.connect(    lambda: self.modelMove(FreeCAD.Vector(-1, -1, 0)))

        self.form.modelMoveUp.clicked.connect(          lambda: self.modelMove(FreeCAD.Vector( 0,  1, 0)))
        self.form.modelMoveDown.clicked.connect(        lambda: self.modelMove(FreeCAD.Vector( 0, -1, 0)))

        self.form.modelMoveRightUp.clicked.connect(     lambda: self.modelMove(FreeCAD.Vector( 1,  1, 0)))
        self.form.modelMoveRight.clicked.connect(       lambda: self.modelMove(FreeCAD.Vector( 1,  0, 0)))
        self.form.modelMoveRightDown.clicked.connect(   lambda: self.modelMove(FreeCAD.Vector( 1, -1, 0)))

        self.form.modelRotateLeft.clicked.connect(      lambda: self.modelRotate(FreeCAD.Vector(0, 0,  1)))
        self.form.modelRotateRight.clicked.connect(     lambda: self.modelRotate(FreeCAD.Vector(0, 0, -1)))

        self.updateSelection()

        # set active page
        if activate in ['General', 'Model']:
            self.form.setCurrentIndex(0)
        if activate in ['Output', 'Post Processor']:
            self.form.setCurrentIndex(1)
        if activate in ['Layout', 'Stock']:
            self.form.setCurrentIndex(2)
        if activate in ['Tools', 'Tool Controller']:
            self.form.setCurrentIndex(3)
        if activate in ['Workplan', 'Operations']:
            self.form.setCurrentIndex(4)

        self.form.currentChanged.connect(self.tabPageChanged)
        self.template.exportButton().clicked.connect(self.templateExport)

    def templateExport(self):
        self.getFields()
        PathJobCmd.CommandJobTemplateExport.SaveDialog(self.obj, self.template)

    def open(self):
        FreeCADGui.Selection.addObserver(self)

    # SelectionObserver interface
    def addSelection(self, doc, obj, sub, pnt):
        self.updateSelection()
    def removeSelection(self, doc, obj, sub):
        self.updateSelection()
    def setSelection(self, doc):
        self.updateSelection()
    def clearSelection(self, doc):
        self.updateSelection()

def Create(base, template=None):
    '''Create(base, template) ... creates a job instance for the given base object
    using template to configure it.'''
    FreeCADGui.addModule('PathScripts.PathJob')
    FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
    try:
        obj = PathJob.Create('Job', base, template)
        ViewProvider(obj.ViewObject)
        FreeCAD.ActiveDocument.commitTransaction()
        obj.Document.recompute()
        obj.ViewObject.Proxy.editObject(obj.Stock)
        return obj
    except Exception as exc:
        PathLog.error(exc)
        traceback.print_exc(exc)
        FreeCAD.ActiveDocument.abortTransaction()

# make sure the UI has been initialized
PathGuiInit.Startup()

