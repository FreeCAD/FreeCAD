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
import PathScripts.PathToolController as PathToolController

import glob
import os
import sys
import xml.etree.ElementTree as xml

from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore, QtGui

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ViewProvider:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        self.taskPanel = None

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        self.taskPanel = TaskPanel(vobj, self.deleteObjectsOnReject())
        FreeCADGui.Control.showDialog(self.taskPanel)
        self.taskPanel.setupUi()
        self.deleteOnReject = False
        return True

    def unsetEdit(self, vobj, mode):
        if self.taskPanel:
            self.taskPanel.reject()

    def resetTaskPanel(self):
        self.taskPanel = None

    def getIcon(self):
        return ":/icons/Path-Job.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)

    def claimChildren(self):
        children = self.obj.ToolController
        children.append(self.obj.Operations)
        return children

class TaskPanel:
    def __init__(self, vobj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Edit Job"))
        self.vobj = vobj
        self.obj = vobj.Object
        self.deleteOnReject = deleteOnReject
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/PathEdit.ui")

        currentPostProcessor = self.obj.PostProcessor
        postProcessors = PathPreferences.allEnabledPostProcessors(['', currentPostProcessor])
        for post in postProcessors:
            self.form.postProcessor.addItem(post)
        # update the enumeration values, just to make sure all selections are valid
        self.obj.PostProcessor = postProcessors
        self.obj.PostProcessor = currentPostProcessor

        for o in PathJob.ObjectJob.baseCandidates():
            self.form.infoModel.addItem(o.Label, o)


        self.postProcessorDefaultTooltip = self.form.postProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.postProcessorArguments.toolTip()

    def accept(self):
        PathLog.debug('accept')
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.vobj.Proxy.resetTaskPanel()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        PathLog.debug('reject')
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject:
            PathLog.info("Uncreate Job")
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Uncreate Job"))
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        self.vobj.Proxy.resetTaskPanel()
        return True

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
            self.obj.Group = [self.form.operationsList.item(i).data() for i in range(self.form.operationsList.count())]

            selObj = self.form.infoModel.itemData(self.form.infoModel.currentIndex())
            #if self.form.chkCreateClone.isChecked():
            #    selObj = Draft.clone(selObj)
            self.obj.Base = selObj

            self.updateTooltips()

        self.obj.Proxy.execute(self.obj)

    def selectComboBoxText(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def setFields(self):
        '''sets fields in the form to match the object'''

        self.form.infoLabel.setText(self.obj.Label)
        self.form.postProcessorOutputFile.setText(self.obj.PostProcessorOutputFile)

        self.selectComboBoxText(self.form.postProcessor, self.obj.PostProcessor)
        self.form.postProcessorArguments.setText(self.obj.PostProcessorArgs)
        self.obj.Proxy.onChanged(self.obj, "PostProcessor")
        self.updateTooltips()

        self.form.operationsList.clear()
        for child in self.obj.Group:
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


    def open(self):
        pass

    def setPostProcessorOutputFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("Path_Job", "Select Output File"), None, translate("Path_Job", "All Files (*.*)"))
        if filename and filename[0]:
            self.obj.PostProcessorOutputFile = str(filename[0])
            self.setFields()

    def setupUi(self):
        # Info
        self.form.infoLabel.editingFinished.connect(self.getFields)
        self.form.infoModel.currentIndexChanged.connect(self.getFields)

        # Post Processor
        self.form.postProcessor.currentIndexChanged.connect(self.getFields)
        self.form.postProcessorArguments.editingFinished.connect(self.getFields)
        self.form.postProcessorOutputFile.editingFinished.connect(self.getFields)
        self.form.postProcessorSetOutputFile.clicked.connect(self.setPostProcessorOutputFile)

        self.form.operationsList.indexesMoved.connect(self.getFields)

        self.setFields()

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
        PathJob template files are name job_*.xml'''
        PathLog.track(path)
        return glob.glob(path + '/job_*.xml')

    def getModel(self):
        '''answer the base model selected for the job'''
        label = self.dialog.cbModel.currentText()
        return filter(lambda obj: obj.Label == label, FreeCAD.ActiveDocument.Objects)[0]

    def getTemplate(self):
        '''answer the file name of the template to be assigned'''
        return self.dialog.cbTemplate.itemData(self.dialog.cbTemplate.currentIndex())

    def exec_(self):
        return self.dialog.exec_()

def Create(base, template):
    FreeCADGui.addModule('PathScripts.PathJob')
    FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
    #try:
    obj = PathJob.Create('Job', base, template)
    ViewProvider(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    #except:
    #    PathLog.error(sys.exc_info())
    #    FreeCAD.ActiveDocument.abortTransaction()

class CommandJobCreate:
    '''
    Command used to creat a command.
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

class CommandJobExportTemplate:
    '''
    Command to export a template of a given job.
    Opens a dialog to select the file to store the template in. If the template is stored in Path's
    file path (see preferences) and named in accordance with job_*.xml it will automatically be found
    on Job creation and be available for selection.
    '''

    def GetResources(self):
        return {'Pixmap': 'Path-Job',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Export Template"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Exports Path Job as a template to be used for other jobs")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        job = FreeCADGui.Selection.getSelection()[0]
        foo = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(),
                "Path - Job Template",
                PathPreferences.filePath(),
                "job_*.xml")[0]
        if foo: 
            self.Execute(job, foo)

    @classmethod
    def Execute(cls, job, path):
        root = xml.Element('PathJobTemplate')
        xml.SubElement(root, JobTemplate.Job, job.Proxy.templateAttrs(job))
        for obj in job.Group:
            if hasattr(obj, 'Tool') and hasattr(obj, 'SpindleDir'):
                tc = xml.SubElement(root, JobTemplate.ToolController, obj.Proxy.templateAttrs(obj))
                tc.append(xml.fromstring(obj.Tool.Content))
        xml.ElementTree(root).write(path)

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Job', CommandJobCreate())
    FreeCADGui.addCommand('Path_ExportTemplate', CommandJobExportTemplate())

FreeCAD.Console.PrintLog("Loading PathJob... done\n")

