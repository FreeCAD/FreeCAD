# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import FreeCADGui
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathStock as PathStock
import PathScripts.PathUtil as PathUtil
import glob
import os

from PySide import QtCore, QtGui
from collections import Counter

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class _ItemDelegate(QtGui.QStyledItemDelegate):

    def __init__(self, controller, parent):
        self.controller = controller
        QtGui.QStyledItemDelegate.__init__(self, parent)

    def createEditor(self, parent, option, index):
        editor = QtGui.QSpinBox(parent)
        self.controller.setupColumnEditor(index, editor)
        return editor

class JobCreate:
    DataObject = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, parent=None, sel=None):
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobCreate.ui")
        self.itemsSolid = QtGui.QStandardItem(translate('PathJob', 'Solids'))
        self.items2D    = QtGui.QStandardItem(translate('PathJob', '2D'))
        self.itemsJob   = QtGui.QStandardItem(translate('PathJob', 'Jobs'))
        self.dialog.templateGroup.hide()
        self.dialog.modelGroup.hide()

    def setupTitle(self, title):
        self.dialog.setWindowTitle(title)

    def setupModel(self, job = None):

        if job:
            preSelected = Counter([PathUtil.getPublicObject(job.Proxy.baseObject(job, obj)).Label for obj in job.Model.Group])
            jobResources = job.Model.Group + [job.Stock]
        else:
            preSelected = Counter([obj.Label for obj in FreeCADGui.Selection.getSelection()])
            jobResources = []

        self.candidates = sorted(PathJob.ObjectJob.baseCandidates(), key=lambda o: o.Label)

        # If there is only one possibility we might as well make sure it's selected
        if not preSelected and 1 == len(self.candidates):
            preSelected = Counter([self.candidates[0].Label])

        expandSolids = False
        expand2Ds    = False
        expandJobs   = False

        for i, base in enumerate(self.candidates):
            if not base in jobResources and not PathJob.isResourceClone(job, base, None) and not hasattr(base, 'StockType'):
                item0 = QtGui.QStandardItem()
                item1 = QtGui.QStandardItem()

                item0.setData(base.Label, QtCore.Qt.EditRole)
                item0.setData(base, self.DataObject)
                item0.setCheckable(True)
                item0.setEditable(False)

                item1.setEnabled(True)
                item1.setEditable(True)

                if base.Label in preSelected:
                    itemSelected = True
                    item0.setCheckState(QtCore.Qt.CheckState.Checked)
                    item1.setData(preSelected[base.Label], QtCore.Qt.EditRole)
                else:
                    itemSelected = False
                    item0.setCheckState(QtCore.Qt.CheckState.Unchecked)
                    item1.setData(0, QtCore.Qt.EditRole)

                if PathUtil.isSolid(base):
                    self.itemsSolid.appendRow([item0, item1])
                    if itemSelected:
                        expandSolids = True
                else:
                    self.items2D.appendRow([item0, item1])
                    if itemSelected:
                        expand2Ds = True

        for j in sorted(PathJob.Instances(), key=lambda x: x.Label):
            if j != job:
                item0 = QtGui.QStandardItem()
                item1 = QtGui.QStandardItem()

                item0.setData(j.Label, QtCore.Qt.EditRole)
                item0.setData(j, self.DataObject)
                item0.setCheckable(True)
                item0.setEditable(False)

                item1.setEnabled(True)
                item1.setEditable(True)

                if j.Label in preSelected:
                    expandJobs = True
                    item0.setCheckState(QtCore.Qt.CheckState.Checked)
                    item1.setData(preSelected[j.Label], QtCore.Qt.EditRole)
                else:
                    item0.setCheckState(QtCore.Qt.CheckState.Unchecked)
                    item1.setData(0, QtCore.Qt.EditRole)

                self.itemsJob.appendRow([item0, item1])

        self.delegate = _ItemDelegate(self, self.dialog.modelTree)
        self.model = QtGui.QStandardItemModel(self.dialog)
        self.model.setHorizontalHeaderLabels(['Model', 'Count'])


        if self.itemsSolid.hasChildren():
            self.model.appendRow(self.itemsSolid)
            if expandSolids or not (expand2Ds or expandJobs):
                expandSolids = True
        if self.items2D.hasChildren():
            self.model.appendRow(self.items2D)
        if self.itemsJob.hasChildren():
            self.model.appendRow(self.itemsJob)

        self.dialog.modelTree.setModel(self.model)
        self.dialog.modelTree.setItemDelegateForColumn(1, self.delegate)
        self.dialog.modelTree.expandAll()
        self.dialog.modelTree.resizeColumnToContents(0)
        self.dialog.modelTree.resizeColumnToContents(1)
        self.dialog.modelTree.collapseAll()

        if expandSolids:
            self.dialog.modelTree.setExpanded(self.itemsSolid.index(), True)
        if expand2Ds:
            self.dialog.modelTree.setExpanded(self.items2D.index(), True)
        if expandJobs:
            self.dialog.modelTree.setExpanded(self.itemsJob.index(), True)

        self.dialog.modelTree.setEditTriggers(QtGui.QAbstractItemView.AllEditTriggers)
        self.dialog.modelTree.setSelectionBehavior(QtGui.QAbstractItemView.SelectItems)

        self.dialog.modelGroup.show()

    def updateData(self, topLeft, bottomRight):
        if topLeft.column() == bottomRight.column() == 0:
            item0 = self.model.itemFromIndex(topLeft)
            item1 = self.model.itemFromIndex(topLeft.sibling(topLeft.row(), 1))
            if item0.checkState() == QtCore.Qt.Checked:
                if item1.data(QtCore.Qt.EditRole) == 0:
                    item1.setData(1, QtCore.Qt.EditRole)
            else:
                item1.setData(0, QtCore.Qt.EditRole)

        if topLeft.column() == bottomRight.column() == 1:
            item0 = self.model.itemFromIndex(topLeft.sibling(topLeft.row(), 0))
            item1 = self.model.itemFromIndex(topLeft)
            if item1.data(QtCore.Qt.EditRole) == 0:
                item0.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item0.setCheckState(QtCore.Qt.CheckState.Checked)

    def item1ValueChanged(self, v):
        item0 = self.model.itemFromIndex(self.index.sibling(self.index.row(), 0))
        if 0 == v:
            item0.setCheckState(QtCore.Qt.CheckState.Unchecked)
        else:
            item0.setCheckState(QtCore.Qt.CheckState.Checked)

    def setupColumnEditor(self, index, editor):
        editor.setMinimum(0)
        editor.setMaximum(999999)
        self.index = index
        editor.valueChanged.connect(self.item1ValueChanged)

    def setupTemplate(self):
        templateFiles = []
        for path in PathPreferences.searchPaths():
            templateFiles.extend(self.templateFilesIn(path))

        template = {}
        for tFile in templateFiles:
            name = os.path.split(os.path.splitext(tFile)[0])[1][4:]
            if name in template:
                basename = name
                i = 0
                while name in template:
                    i = i + 1
                    name = basename + " (%s)" % i
            PathLog.track(name, tFile)
            template[name] = tFile
        selectTemplate = PathPreferences.defaultJobTemplate()
        index = 0
        self.dialog.jobTemplate.addItem('<none>', '')
        for name in sorted(template.keys()):
            if template[name] == selectTemplate:
                index = self.dialog.jobTemplate.count()
            self.dialog.jobTemplate.addItem(name, template[name])
        self.dialog.jobTemplate.setCurrentIndex(index)
        self.dialog.templateGroup.show()

    def templateFilesIn(self, path):
        '''templateFilesIn(path) ... answer all file in the given directory which fit the job template naming convention.
        PathJob template files are name job_*.json'''
        PathLog.track(path)
        return glob.glob(path + '/job_*.json')

    def getModels(self):
        models = []

        for i in range(self.itemsSolid.rowCount()):
            for j in range(self.itemsSolid.child(i, 1).data(QtCore.Qt.EditRole)):
                models.append(self.itemsSolid.child(i).data(self.DataObject))

        for i in range(self.items2D.rowCount()):
            for j in range(self.items2D.child(i, 1).data(QtCore.Qt.EditRole)):
                models.append(self.items2D.child(i).data(self.DataObject))

        for i in range(self.itemsJob.rowCount()):
            for j in range(self.itemsJob.child(i, 1).data(QtCore.Qt.EditRole)):
                # Note that we do want to use the models (resource clones) of the
                # source job as base objects for the new job in order to get the
                # identical placement, and anything else that's been customized.
                models.extend(self.itemsJob.child(i, 0).data(self.DataObject).Model.Group)

        return models

    def getTemplate(self):
        '''answer the file name of the template to be assigned'''
        return self.dialog.jobTemplate.itemData(self.dialog.jobTemplate.currentIndex())

    def exec_(self):
        # ml: For some reason the callback has to be unregistered, otherwise there is a
        # segfault when python is shutdown. To keep it symmetric I also put the callback
        # registration here
        self.model.dataChanged.connect(self.updateData)
        rc = self.dialog.exec_()
        self.model.dataChanged.disconnect()
        return rc


class JobTemplateExport:
    DataObject = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, job, parent=None):
        self.job = job
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobTemplateExport.ui")
        if parent:
            self.dialog.setParent(parent)
            parent.layout().addWidget(self.dialog)
            self.dialog.dialogButtonBox.hide()
        else:
            self.dialog.exportButtonBox.hide()
        self.updateUI()
        self.dialog.toolsGroup.clicked.connect(self.checkUncheckTools)

    def exportButton(self):
        return self.dialog.exportButton

    def updateUI(self):
        job = self.job
        if job.PostProcessor:
            ppHint = "%s %s %s" % (job.PostProcessor, job.PostProcessorArgs, job.PostProcessorOutputFile)
            self.dialog.postProcessingHint.setText(ppHint)
        else:
            self.dialog.postProcessingGroup.setEnabled(False)
            self.dialog.postProcessingGroup.setChecked(False)

        if job.Stock and not PathJob.isResourceClone(job, 'Stock', 'Stock'):
            stockType = PathStock.StockType.FromStock(job.Stock)
            if stockType == PathStock.StockType.FromBase:
                seHint = translate('PathJob', "Base -/+ %.2f/%.2f %.2f/%.2f %.2f/%.2f") % (job.Stock.ExtXneg, job.Stock.ExtXpos, job.Stock.ExtYneg, job.Stock.ExtYpos, job.Stock.ExtZneg, job.Stock.ExtZpos)
                self.dialog.stockPlacement.setChecked(False)
            elif stockType == PathStock.StockType.CreateBox:
                seHint = translate('PathJob', "Box: %.2f x %.2f x %.2f") % (job.Stock.Length, job.Stock.Width, job.Stock.Height)
            elif stockType == PathStock.StockType.CreateCylinder:
                seHint = translate('PathJob', "Cylinder: %.2f x %.2f") % (job.Stock.Radius, job.Stock.Height)
            else:
                seHint = '-'
                PathLog.error(translate('PathJob', 'Unsupported stock type'))
            self.dialog.stockExtentHint.setText(seHint)
            spHint = "%s" % job.Stock.Placement
            self.dialog.stockPlacementHint.setText(spHint)

        rapidChanged = not job.SetupSheet.Proxy.hasDefaultToolRapids()
        depthsChanged = not job.SetupSheet.Proxy.hasDefaultOperationDepths()
        heightsChanged = not job.SetupSheet.Proxy.hasDefaultOperationHeights()
        opsWithSettings = job.SetupSheet.Proxy.operationsWithSettings()
        settingsChanged = rapidChanged or depthsChanged or heightsChanged or 0 != len(opsWithSettings)
        self.dialog.settingsGroup.setChecked(settingsChanged)
        self.dialog.settingToolRapid.setChecked(rapidChanged)
        self.dialog.settingOperationDepths.setChecked(depthsChanged)
        self.dialog.settingOperationHeights.setChecked(heightsChanged)

        self.dialog.settingsOpsList.clear()
        for op in opsWithSettings:
            item = QtGui.QListWidgetItem(op)
            item.setCheckState(QtCore.Qt.CheckState.Checked)
            self.dialog.settingsOpsList.addItem(item)

        self.dialog.toolsList.clear()
        for tc in sorted(job.ToolController, key=lambda o: o.Label):
            item = QtGui.QListWidgetItem(tc.Label)
            item.setData(self.DataObject, tc)
            item.setCheckState(QtCore.Qt.CheckState.Checked)
            self.dialog.toolsList.addItem(item)

    def checkUncheckTools(self):
        state = QtCore.Qt.CheckState.Checked if self.dialog.toolsGroup.isChecked() else QtCore.Qt.CheckState.Unchecked
        for i in range(self.dialog.toolsList.count()):
            self.dialog.toolsList.item(i).setCheckState(state)

    def includePostProcessing(self):
        return self.dialog.postProcessingGroup.isChecked()

    def includeToolControllers(self):
        tcs = []
        for i in range(self.dialog.toolsList.count()):
            item = self.dialog.toolsList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Checked:
                tcs.append(item.data(self.DataObject))
        return tcs

    def includeStock(self):
        return self.dialog.stockGroup.isChecked()
    def includeStockExtent(self):
        return self.dialog.stockExtent.isChecked()
    def includeStockPlacement(self):
        return self.dialog.stockPlacement.isChecked()

    def includeSettings(self):
        return self.dialog.settingsGroup.isChecked()
    def includeSettingToolRapid(self):
        return self.dialog.settingToolRapid.isChecked()
    def includeSettingOperationHeights(self):
        return self.dialog.settingOperationHeights.isChecked()
    def includeSettingOperationDepths(self):
        return self.dialog.settingOperationDepths.isChecked()

    def includeSettingOpsSettings(self):
        ops = []
        for i in range(self.dialog.settingsOpsList.count()):
            item = self.dialog.settingsOpsList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Checked:
                ops.append(item.text())
        return ops

    def exec_(self):
        return self.dialog.exec_()

