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
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathStock as PathStock
import PathScripts.PathToolController as PathToolController
import PathScripts.PathToolLibraryManager as PathToolLibraryManager
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import math
import sys

from PathScripts.PathGeom import PathGeom
from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore, QtGui
from pivy import coin

# Qt tanslation handling
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

    def setEdit(self, vobj, mode=0):
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
            if obj == self.obj.Base:
                return self.openTaskPanel('Base')
            if obj == self.obj.Stock:
                return self.openTaskPanel('Stock')
            PathLog.info("Expected a specific object to edit - %s not recognized" % obj.Label)
        return self.openTaskPanel()

    def uneditObject(self):
        self.unsetEdit(None, None)

    def getIcon(self):
        return ":/icons/Path-Job.svg"

    def claimChildren(self):
        children = self.obj.ToolController
        children.append(self.obj.Operations)
        if self.obj.Base:
            children.append(self.obj.Base)
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
        if prop == 'Base' and self.obj.Base and self.obj.Base.ViewObject and self.obj.Base.ViewObject.Proxy:
            if not PathJob.isArchPanelSheet(self.obj.Base):
                self.obj.Base.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)
        if prop == 'Stock' and self.obj.Stock and self.obj.Stock.ViewObject and self.obj.Stock.ViewObject.Proxy:
            self.obj.Stock.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)

    def baseObjectViewObject(self, obj):
        return PathUtil.getPublicObject(self.obj.Proxy.baseObject(obj)).ViewObject

    def baseObjectSaveVisibility(self, obj):
        baseVO = self.baseObjectViewObject(self.obj)
        if baseVO:
            self.baseOrigVisibility = baseVO.Visibility
            baseVO.Visibility = False
        if obj.Base and obj.Base.ViewObject:
            obj.Base.ViewObject.Visibility = True

    def baseObjectRestoreVisibility(self, obj):
        baseVO = self.baseObjectViewObject(self.obj)
        if baseVO:
            baseVO.Visibility = self.baseOrigVisibility

    def setupEditVisibility(self, obj):
        self.baseVisibility = False
        self.baseOrigVisibility = False
        if obj.Base and obj.Base.ViewObject:
            self.baseVisibility = obj.Base.ViewObject.Visibility
            self.baseObjectSaveVisibility(obj)

        self.stockVisibility = False
        if obj.Stock and obj.Stock.ViewObject:
            self.stockVisibility = obj.Stock.ViewObject.Visibility
            self.obj.Stock.ViewObject.Visibility = True

    def resetEditVisibility(self, obj):
        if obj.Base and obj.Base.ViewObject:
            obj.Base.ViewObject.Visibility = self.baseVisibility
        self.baseObjectRestoreVisibility(obj)
        if obj.Stock and obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = self.stockVisibility

class StockEdit(object):
    Index = -1
    StockType = PathStock.StockType.Unknown

    def __init__(self, obj, form):
        self.obj = obj
        self.form = form
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
        if obj.Stock:
            obj.Document.removeObject(self.obj.Stock.Name)
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
        return self.form.stockFromBase

    def getFields(self, obj, fields = ['xneg', 'xpos', 'yneg', 'ypos', 'zneg', 'zpos']):
        PathLog.track(obj.Label, fields)
        if self.IsStock(obj):
            try:
                if 'xneg' in fields:
                    obj.Stock.ExtXneg = FreeCAD.Units.Quantity(self.form.stockExtXneg.text())
                if 'xpos' in fields:
                    obj.Stock.ExtXpos = FreeCAD.Units.Quantity(self.form.stockExtXpos.text())
                if 'yneg' in fields:
                    obj.Stock.ExtYneg = FreeCAD.Units.Quantity(self.form.stockExtYneg.text())
                if 'ypos' in fields:
                    obj.Stock.ExtYpos = FreeCAD.Units.Quantity(self.form.stockExtYpos.text())
                if 'zneg' in fields:
                    obj.Stock.ExtZneg = FreeCAD.Units.Quantity(self.form.stockExtZneg.text())
                if 'zpos' in fields:
                    obj.Stock.ExtZpos = FreeCAD.Units.Quantity(self.form.stockExtZpos.text())
            except:
                pass
        else:
            PathLog.error(translate('PathJob', 'Stock not from Base bound box!'))

    def setFields(self, obj):
        if not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateFromBase(obj))
        self.setLengthField(self.form.stockExtXneg, obj.Stock.ExtXneg)
        self.setLengthField(self.form.stockExtXpos, obj.Stock.ExtXpos)
        self.setLengthField(self.form.stockExtYneg, obj.Stock.ExtYneg)
        self.setLengthField(self.form.stockExtYpos, obj.Stock.ExtYpos)
        self.setLengthField(self.form.stockExtZneg, obj.Stock.ExtZneg)
        self.setLengthField(self.form.stockExtZpos, obj.Stock.ExtZpos)

    def setupUi(self, obj):
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
        if not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateBox(obj))
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
        if not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateCylinder(obj))
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
        if obj.Base in solids and PathJob.isResourceClone(obj, 'Base'):
            solids.remove(obj.Base)
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

        base = self.obj.Base if PathJob.isResourceClone(self.obj, 'Base') else None
        stock = self.obj.Stock
        for o in PathJob.ObjectJob.baseCandidates():
            if o != base and o != stock:
                self.form.jobModel.addItem(o.Label, o)
        self.selectComboBoxText(self.form.jobModel, self.obj.Proxy.baseObject(self.obj).Label)

        self.postProcessorDefaultTooltip = self.form.postProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.postProcessorArguments.toolTip()

        self.vproxy.setupEditVisibility(self.obj)

        self.stockFromBase = None
        self.stockFromExisting = None
        self.stockCreateBox = None
        self.stockCreateCylinder = None
        self.stockEdit = None

    def preCleanup(self):
        PathLog.track()
        FreeCADGui.Selection.removeObserver(self)
        self.vproxy.resetEditVisibility(self.obj)
        self.vproxy.resetTaskPanel()

    def accept(self, resetEdit=True):
        PathLog.track()
        self.preCleanup()
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)

    def reject(self, resetEdit=True):
        PathLog.track()
        self.preCleanup()
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

            selObj = self.form.jobModel.itemData(self.form.jobModel.currentIndex())
            if self.obj.Proxy.baseObject(self.obj) != selObj:
                self.vproxy.baseObjectRestoreVisibility(self.obj)
                if PathJob.isResourceClone(self.obj, 'Base'):
                    self.obj.Document.removeObject(self.obj.Base.Name)
                self.obj.Base = PathJob.createResourceClone(self.obj, selObj, 'Base', 'Base')
                self.vproxy.baseObjectSaveVisibility(self.obj)

            self.updateTooltips()
            self.stockEdit.getFields(self.obj)

            self.obj.Proxy.execute(self.obj)

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

        baseindex = -1
        if self.obj.Base:
            baseindex = self.form.jobModel.findText(self.obj.Base.Label, QtCore.Qt.MatchFixedString)
        else:
            for o in FreeCADGui.Selection.getCompleteSelection():
                baseindex = self.form.jobModel.findText(o.Label, QtCore.Qt.MatchFixedString)
        if baseindex >= 0:
            self.form.jobModel.setCurrentIndex(baseindex)

        self.updateToolController()
        self.stockEdit.setFields(self.obj)

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

    def orientSelected(self, axis):
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

    def alignSetOrigin(self):
        (obj, by) = self.alignMoveToOrigin()
        if obj == self.obj.Base and self.obj.Stock:
            Draft.move(self.obj.Stock, by)
        if obj == self.obj.Stock and self.obj.Base:
            Draft.move(self.obj.Base, by)
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

    def updateStockEditor(self, index):
        def setupFromBaseEdit():
            if not self.stockFromBase:
                self.stockFromBase = StockFromBaseBoundBoxEdit(self.obj, self.form)
            self.stockEdit = self.stockFromBase
        def setupCreateBoxEdit():
            if not self.stockCreateBox:
                self.stockCreateBox = StockCreateBoxEdit(self.obj, self.form)
            self.stockEdit = self.stockCreateBox
        def setupCreateCylinderEdit():
            if not self.stockCreateCylinder:
                self.stockCreateCylinder = StockCreateCylinderEdit(self.obj, self.form)
            self.stockEdit = self.stockCreateCylinder
        def setupFromExisting():
            if not self.stockFromExisting:
                self.stockFromExisting = StockFromExistingEdit(self.obj, self.form)
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

    def centerInStock(self):
        bbb = self.obj.Base.Shape.BoundBox
        bbs = self.obj.Stock.Shape.BoundBox
        by = bbs.Center - bbb.Center
        Draft.move(self.obj.Base, by)

    def centerInStockXY(self):
        bbb = self.obj.Base.Shape.BoundBox
        bbs = self.obj.Stock.Shape.BoundBox
        by = bbs.Center - bbb.Center
        by.z = 0
        Draft.move(self.obj.Base, by)

    def updateSelection(self):
        sel = FreeCADGui.Selection.getSelectionEx()

        PathLog.track(len(sel))
        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            if 'Vertex' == sel[0].SubObjects[0].ShapeType:
                self.form.orientGroup.setEnabled(False)
                self.form.setOrigin.setEnabled(True)
                self.form.moveToOrigin.setEnabled(True)
            else:
                self.form.orientGroup.setEnabled(True)
                self.form.setOrigin.setEnabled(False)
                self.form.moveToOrigin.setEnabled(False)
        else:
            self.form.orientGroup.setEnabled(False)
            self.form.setOrigin.setEnabled(False)
            self.form.moveToOrigin.setEnabled(False)

        if len(sel) == 1 and sel[0].Object == self.obj.Base:
            self.form.centerInStock.setEnabled(True)
            self.form.centerInStockXY.setEnabled(True)
        else:
            if len(sel) == 1 and self.obj.Base:
                PathLog.debug("sel = %s / %s" % (sel[0].Object.Label, self.obj.Base.Label))
            else:
                PathLog.debug("sel len = %d" % len(sel))
            self.form.centerInStock.setEnabled(False)
            self.form.centerInStockXY.setEnabled(False)


    def setupUi(self, activate):
        self.updateStockEditor(-1)
        self.setFields()

        # Info
        self.form.jobLabel.editingFinished.connect(self.getFields)
        self.form.jobModel.currentIndexChanged.connect(self.getFields)

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
        self.form.centerInStock.clicked.connect(self.centerInStock)
        self.form.centerInStockXY.clicked.connect(self.centerInStockXY)

        self.form.stock.currentIndexChanged.connect(self.updateStockEditor)

        self.form.orientXAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(1, 0, 0)))
        self.form.orientYAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(0, 1, 0)))
        self.form.orientZAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(0, 0, 1)))

        self.form.setOrigin.clicked.connect(self.alignSetOrigin)
        self.form.moveToOrigin.clicked.connect(self.alignMoveToOrigin)
        self.updateSelection()

        # set active page
        if activate in ['General', 'Base']:
            self.form.setCurrentIndex(0)
        if activate in ['Output', 'Post Processor']:
            self.form.setCurrentIndex(1)
        if activate in ['Layout', 'Stock']:
            self.form.setCurrentIndex(2)
        if activate in ['Tools', 'Tool Controller']:
            self.form.setCurrentIndex(3)
        if activate in ['Workplan', 'Operations']:
            self.form.setCurrentIndex(4)

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
    except:
        PathLog.error(sys.exc_info())
        FreeCAD.ActiveDocument.abortTransaction()

