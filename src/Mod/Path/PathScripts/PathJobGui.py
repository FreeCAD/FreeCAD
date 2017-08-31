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
import PathScripts.PathLog as PathLog
import PathScripts.PathToolController as PathToolController
import PathScripts.PathToolLibraryManager as PathToolLibraryManager
import math
import sys

from PathScripts.PathGeom import PathGeom
from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore, QtGui

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class ViewProvider:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.taskPanel = None

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        self.taskPanel = TaskPanel(vobj, self.deleteObjectsOnReject())
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.taskPanel)
        self.taskPanel.setupUi()
        self.deleteOnReject = False
        return True

    def resetTaskPanel(self):
        self.taskPanel = None

    def unsetEdit(self, arg1, arg2):
        if self.taskPanel:
            self.taskPanel.reject(False)

    def getIcon(self):
        return ":/icons/Path-Job.svg"

    def claimChildren(self):
        children = self.obj.ToolController
        children.append(self.obj.Operations)
        if self.obj.Base:
            children.append(self.obj.Base)
        if self.obj.Stock:
            children.append(self.obj.Stock)
        return children

class TaskPanel:
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataProperty = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, vobj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Edit Job"))
        self.vobj = vobj
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

        for o in PathJob.ObjectJob.baseCandidates():
            if o != self.obj.Base:
                self.form.infoModel.addItem(o.Label, o)
        self.selectComboBoxText(self.form.infoModel, self.obj.Proxy.baseObject(self.obj).Label)

        self.postProcessorDefaultTooltip = self.form.postProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.postProcessorArguments.toolTip()

        self.baseVisibility = False
        self.baseOrigVisibilty = False
        if self.obj.Base and self.obj.Base.ViewObject:
            self.baseVisibility = self.obj.Base.ViewObject.Visibility
            self.baseObjectSaveVisibility(self.obj)

    def baseObjectViewObject(self, obj):
        base = obj.Proxy.baseObject(obj)
        body = base.getParentGeoFeatureGroup()
        return body.ViewObject if body else base.ViewObject

    def baseObjectSaveVisibility(self, obj):
        baseVO = self.baseObjectViewObject(self.obj)
        self.baseOrigVisibility = baseVO.Visibility
        baseVO.Visibility = False
        obj.Base.ViewObject.Visibility = True

    def baseObjectRestoreVisibility(self, obj):
        baseVO = self.baseObjectViewObject(self.obj)
        baseVO.Visibility = self.baseOrigVisibility

    def preCleanup(self):
        PathLog.track()
        FreeCADGui.Selection.removeObserver(self)
        if self.obj.Base and self.obj.Base.ViewObject:
            self.obj.Base.ViewObject.Visibility = self.baseVisibility
            self.baseObjectRestoreVisibility(self.obj)

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
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)
        return True

    def cleanup(self, resetEdit):
        PathLog.track()
        self.vobj.Proxy.resetTaskPanel()
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

            self.obj.Label = str(self.form.infoLabel.text())
            self.obj.Operations.Group = [self.form.operationsList.item(i).data(self.DataObject) for i in range(self.form.operationsList.count())]

            selObj = self.form.infoModel.itemData(self.form.infoModel.currentIndex())
            if self.obj.Proxy.baseObject(self.obj) != selObj:
                self.baseObjectRestoreVisibility(self.obj)
                self.obj.Document.removeObject(self.obj.Base.Name)
                self.obj.Proxy.createResourceClone(self.obj, selObj, 'Base')
                self.baseObjectSaveVisibility(self.obj)

            self.updateTooltips()

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

            item = QtGui.QTableWidgetItem("%g" % tc.HorizFeed)
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, 'HorizFeed')
            self.form.toolControllerList.setItem(row, 2, item)

            item = QtGui.QTableWidgetItem("%g" % tc.VertFeed)
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

        self.form.infoLabel.setText(self.obj.Label)
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
            baseindex = self.form.infoModel.findText(self.obj.Base.Label, QtCore.Qt.MatchFixedString)
        else:
            for o in FreeCADGui.Selection.getCompleteSelection():
                baseindex = self.form.infoModel.findText(o.Label, QtCore.Qt.MatchFixedString)
        if baseindex >= 0:
            self.form.infoModel.setCurrentIndex(baseindex)

        self.updateToolController()

    def setPostProcessorOutputFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("Path_Job", "Select Output File"), None, translate("Path_Job", "All Files (*.*)"))
        if filename and filename[0]:
            self.obj.PostProcessorOutputFile = str(filename[0])
            self.setFields()

    def operationSelect(self):
        if self.form.operationsList.selectedItems():
            self.form.operationModify.setEnabled(True)
        else:
            self.form.operationModify.setEnabled(False)

    def objectDelete(self, widget):
        for item in widget.selectedItems():
            obj = item.data(self.DataObject)
            if obj.ViewObject and hasattr(obj.ViewObject, 'Proxy') and hasattr(obj.ViewObject.Proxy, 'onDelete'):
                obj.ViewObject.Proxy.onDelete(obj.ViewObject, None)
            FreeCAD.ActiveDocument.removeObject(obj.Name)
        self.setFields()

    def operationDelete(self):
        self.objectDelete(self.form.operationsList)

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

    def updateSelection(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            if 'Vertex' == sel[0].SubObjects[0].ShapeType:
                self.form.orientGroup.setEnabled(False)
                self.form.alignGroup.setEnabled(True)
                self.form.setOrigin.setEnabled(True)
                self.form.moveToOrigin.setEnabled(True)
            else:
                self.form.orientGroup.setEnabled(True)
                self.form.setOrigin.setEnabled(False)
                self.form.moveToOrigin.setEnabled(False)
        else:
            self.form.orientGroup.setEnabled(False)
            self.form.alignGroup.setEnabled(False)

    def setupUi(self):
        self.setFields()

        # Info
        self.form.infoLabel.editingFinished.connect(self.getFields)
        self.form.infoModel.currentIndexChanged.connect(self.getFields)

        # Post Processor
        self.form.postProcessor.currentIndexChanged.connect(self.getFields)
        self.form.postProcessorArguments.editingFinished.connect(self.getFields)
        self.form.postProcessorOutputFile.editingFinished.connect(self.getFields)
        self.form.postProcessorSetOutputFile.clicked.connect(self.setPostProcessorOutputFile)

        self.form.operationsList.itemSelectionChanged.connect(self.operationSelect)
        self.form.operationsList.indexesMoved.connect(self.getFields)
        self.form.operationDelete.clicked.connect(self.operationDelete)

        self.form.toolControllerList.itemSelectionChanged.connect(self.toolControllerSelect)
        self.form.toolControllerList.itemChanged.connect(self.toolControllerChanged)
        self.form.toolControllerEdit.clicked.connect(self.toolControllerEdit)
        self.form.toolControllerDelete.clicked.connect(self.toolControllerDelete)
        self.form.toolControllerAdd.clicked.connect(self.toolControllerAdd)

        self.operationSelect()
        self.toolControllerSelect()

        # Stock, Orientation and Alignment
        self.form.stockGroup.hide()
        self.form.centerInStock.hide()
        self.form.centerInStockXY.hide()

        self.form.orientXAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(1, 0, 0)))
        self.form.orientYAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(0, 1, 0)))
        self.form.orientZAxis.clicked.connect(lambda: self.orientSelected(FreeCAD.Vector(0, 0, 1)))

        self.form.setOrigin.clicked.connect(self.alignSetOrigin)
        self.form.moveToOrigin.clicked.connect(self.alignMoveToOrigin)
        self.updateSelection()

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
    except:
        PathLog.error(sys.exc_info())
        FreeCAD.ActiveDocument.abortTransaction()

