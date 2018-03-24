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
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathScripts.PathStock as PathStock
import PathScripts.PathUtil as PathUtil
import glob
import json
import os

from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore, QtGui

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class DlgJobCreate:

    def __init__(self, parent=None):
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobCreate.ui")
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            selected = sel[0].Label
        else:
            selected = None
        index = 0
        for base in PathJob.ObjectJob.baseCandidates():
            if base.Label == selected:
                index = self.dialog.cbModel.count()
            self.dialog.cbModel.addItem(base.Label)
        self.dialog.cbModel.setCurrentIndex(index)

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
        self.dialog.cbTemplate.addItem('<none>', '')
        for name in sorted(template.keys()):
            if template[name] == selectTemplate:
                index = self.dialog.cbTemplate.count()
            self.dialog.cbTemplate.addItem(name, template[name])
        self.dialog.cbTemplate.setCurrentIndex(index)

    def templateFilesIn(self, path):
        '''templateFilesIn(path) ... answer all file in the given directory which fit the job template naming convention.
        PathJob template files are name job_*.json'''
        PathLog.track(path)
        return glob.glob(path + '/job_*.json')

    def getModel(self):
        '''answer the base model selected for the job'''
        label = self.dialog.cbModel.currentText()
        return filter(lambda obj: obj.Label == label, FreeCAD.ActiveDocument.Objects)[0]

    def getTemplate(self):
        '''answer the file name of the template to be assigned'''
        return self.dialog.cbTemplate.itemData(self.dialog.cbTemplate.currentIndex())

    def exec_(self):
        return self.dialog.exec_()

class CommandJobCreate:
    '''
    Command used to create a command.
    When activated the command opens a dialog allowing the user to select a base object (has to be a solid)
    and a template to be used for the initial creation.
    '''

    def GetResources(self):
        return {'Pixmap': 'Path-Job',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Job"),
                'Accel': "P, J",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Creates a Path Job object")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        dialog = DlgJobCreate()
        if dialog.exec_() == 1:
            self.Execute(dialog.getModel(), dialog.getTemplate())
            FreeCAD.ActiveDocument.recompute()

    @classmethod
    def Execute(cls, base, template):
        FreeCADGui.addModule('PathScripts.PathJobGui')
        if template:
            template = "'%s'" % template
        else:
            template = 'None'
        FreeCADGui.doCommand('PathScripts.PathJobGui.Create(App.ActiveDocument.%s, %s)' % (base.Name, template))


class DlgJobTemplateExport:
    DataObject = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, job, parent=None):
        self.job = job
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobTemplateExport.ui")

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
        settingsChanged = rapidChanged or depthsChanged or heightsChanged
        self.dialog.settingsGroup.setChecked(settingsChanged)
        self.dialog.settingToolRapid.setChecked(rapidChanged)
        self.dialog.settingOperationDepths.setChecked(depthsChanged)
        self.dialog.settingOperationHeights.setChecked(heightsChanged)

        for tc in sorted(job.ToolController, key=lambda o: o.Label):
            item = QtGui.QListWidgetItem(tc.Label)
            item.setData(self.DataObject, tc)
            item.setCheckState(QtCore.Qt.CheckState.Checked)
            self.dialog.toolsList.addItem(item)

        self.dialog.toolsGroup.clicked.connect(self.checkUncheckTools)

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

    def exec_(self):
        return self.dialog.exec_()

class CommandJobTemplateExport:
    '''
    Command to export a template of a given job.
    Opens a dialog to select the file to store the template in. If the template is stored in Path's
    file path (see preferences) and named in accordance with job_*.json it will automatically be found
    on Job creation and be available for selection.
    '''

    def GetResources(self):
        return {'Pixmap': 'Path-ExportTemplate',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Export Template"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Exports Path Job as a template to be used for other jobs")}

    def GetJob(self):
        # if there's only one Job in the document ...
        jobs = PathJob.Instances()
        if not jobs:
            return None
        if len(jobs) == 1:
            return jobs[0]
        # more than one job, is one of them selected?
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            job = sel[0]
            if hasattr(job, 'Proxy') and isinstance(job.Proxy, PathJob.ObjectJob):
                return job
        return None


    def IsActive(self):
        return self.GetJob() is not None

    def Activated(self):
        job = self.GetJob()
        dialog = DlgJobTemplateExport(job)
        if dialog.exec_() == 1:
            foo = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(),
                    "Path - Job Template",
                    PathPreferences.filePath(),
                    "job_*.json")[0]
            if foo: 
                self.Execute(job, foo, dialog)

    @classmethod
    def Execute(cls, job, path, dialog=None):
        attrs = job.Proxy.templateAttrs(job)

        # post processor settings
        if dialog and not dialog.includePostProcessing():
            attrs.pop(PathJob.JobTemplate.PostProcessor, None)
            attrs.pop(PathJob.JobTemplate.PostProcessorArgs, None)
            attrs.pop(PathJob.JobTemplate.PostProcessorOutputFile, None)

        # tool controller settings
        toolControllers = dialog.includeToolControllers() if dialog else job.ToolController
        if toolControllers:
            tcAttrs = [tc.Proxy.templateAttrs(tc) for tc in toolControllers]
            attrs[PathJob.JobTemplate.ToolController] = tcAttrs

        # stock settings
        stockAttrs = None
        if dialog:
            if dialog.includeStock():
                stockAttrs = PathStock.TemplateAttributes(job.Stock, dialog.includeStockExtent(), dialog.includeStockPlacement())
        else:
            stockAttrs = PathStock.TemplateAttributes(job.Stock)
        if stockAttrs:
            attrs[PathJob.JobTemplate.Stock] = stockAttrs

        # setup sheet
        setupSheetAttrs = None
        if dialog:
            setupSheetAttrs = job.Proxy.setupSheet.templateAttributes(dialog.includeSettingToolRapid(), dialog.includeSettingOperationHeights(), dialog.includeSettingOperationDepths())
        else:
            setupSheetAttrs = job.Proxy.setupSheet.templateAttributes(True, True, True)
        if setupSheetAttrs:
            attrs[PathJob.JobTemplate.SetupSheet] = setupSheetAttrs

        encoded = job.Proxy.setupSheet.encodeTemplateAttributes(attrs)
        # write template
        with open(PathUtil.toUnicode(path), 'wb') as fp:
            json.dump(encoded, fp, sort_keys=True, indent=2)

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Job', CommandJobCreate())
    FreeCADGui.addCommand('Path_ExportTemplate', CommandJobTemplateExport())

FreeCAD.Console.PrintLog("Loading PathJobGui... done\n")

