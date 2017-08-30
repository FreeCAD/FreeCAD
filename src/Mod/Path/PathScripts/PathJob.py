# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import ArchPanel
import Draft
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathToolController as PathToolController
import PathScripts.PathUtil as PathUtil
import glob
import xml.etree.ElementTree as xml
import os
import sys

from PySide import QtCore, QtGui
from PathScripts.PathPostProcessor import PostProcessor
from PathScripts.PathPreferences import PathPreferences

# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule()

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Job object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class JobTemplate:
    '''Attribute and sub element strings for template export/import.'''
    Job = 'Job'
    PostProcessor = 'post'
    PostProcessorArgs = 'post_args'
    PostProcessorOutputFile = 'output'
    GeometryTolerance = 'tol'
    Description = 'desc'
    ToolController = 'ToolController'

class ObjectPathJob:

    def __init__(self, obj, base, template = None):
        self.obj = obj
        obj.addProperty("App::PropertyFile", "PostProcessorOutputFile", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","The NC output file for this project"))
        obj.PostProcessorOutputFile = PathPreferences.defaultOutputFile()
        obj.setEditorMode("PostProcessorOutputFile", 0)  # set to default mode

        obj.addProperty("App::PropertyEnumeration", "PostProcessor", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","Select the Post Processor"))
        obj.PostProcessor = postProcessors = PathPreferences.allEnabledPostProcessors()
        defaultPostProcessor = PathPreferences.defaultPostProcessor()
        # Check to see if default post processor hasn't been 'lost' (This can happen when Macro dir has changed)
        if defaultPostProcessor in postProcessors:
            obj.PostProcessor = defaultPostProcessor
        else:
            obj.PostProcessor = postProcessors[0]
        obj.addProperty("App::PropertyString", "PostProcessorArgs", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property", "Arguments for the Post Processor (specific to the script)"))
        obj.PostProcessorArgs = PathPreferences.defaultPostProcessorArgs()

        obj.addProperty("App::PropertyString", "Description", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional description for this job"))
        obj.addProperty("App::PropertyDistance", "GeometryTolerance", "Geometry",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "For computing Paths; smaller increases accuracy, but slows down computation"))
        obj.GeometryTolerance = PathPreferences.defaultGeometryTolerance()

        obj.addProperty("App::PropertyLink", "Base", "Base", "The base object for all operations")
        obj.Base = base

        obj.Proxy = self

        if FreeCAD.GuiUp:
            ViewProviderJob(obj.ViewObject)
        self.assignTemplate(obj, template)

    def assignTemplate(self, obj, template):
        '''assignTemplate(obj, template) ... extract the properties from the given template file and assign to receiver.
        This will also create any TCs stored in the template.'''
        if template:
            tree = xml.parse(template)
            for job in tree.getroot().iter(JobTemplate.Job):
                if job.get(JobTemplate.GeometryTolerance):
                    obj.GeometryTolerance = float(job.get(JobTemplate.GeometryTolerance))
                if job.get(JobTemplate.PostProcessor):
                    obj.PostProcessor = job.get(JobTemplate.PostProcessor)
                    if job.get(JobTemplate.PostProcessorArgs):
                        obj.PostProcessorArgs = job.get(JobTemplate.PostProcessorArgs)
                    else:
                        obj.PostProcessorArgs = ''
                if job.get(JobTemplate.PostProcessorOutputFile):
                    obj.PostProcessorOutputFile = job.get(JobTemplate.PostProcessorOutputFile)
                if job.get(JobTemplate.Description):
                    obj.Description = job.get(JobTemplate.Description)
            for tc in tree.getroot().iter(JobTemplate.ToolController):
                PathToolController.CommandPathToolController.FromTemplate(obj, tc)
        else:
            PathToolController.CommandPathToolController.Create(obj.Name)

    def templateAttrs(self, obj):
        '''templateAttrs(obj) ... answer a dictionary with all properties of the receiver that should be stored in a template file.'''
        attrs = {}
        if obj.PostProcessor:
            attrs[JobTemplate.PostProcessor]           = obj.PostProcessor
            attrs[JobTemplate.PostProcessorArgs]       = obj.PostProcessorArgs
        if obj.PostProcessorOutputFile:
            attrs[JobTemplate.PostProcessorOutputFile] = obj.PostProcessorOutputFile
        attrs[JobTemplate.GeometryTolerance]           = str(obj.GeometryTolerance.Value)
        if obj.Description:
            attrs[JobTemplate.Description]             = obj.Description
        return attrs

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        mode = 2
        obj.setEditorMode('Placement', mode)

        if prop == "PostProcessor" and obj.PostProcessor:
            processor = PostProcessor.load(obj.PostProcessor)
            self.tooltip = processor.tooltip
            self.tooltipArgs = processor.tooltipArgs

    def execute(self, obj):
        cmds = []
        for child in obj.Group:
            if child.isDerivedFrom("Path::Feature"):
                if obj.UsePlacements:
                    for c in child.Path.Commands:
                        cmds.append(c.transform(child.Placement))
                else:
                    cmds.extend(child.Path.Commands)
        if cmds:
            path = Path.Path(cmds)
            obj.Path = path

    @classmethod
    def baseCandidates(cls):
        '''Answer all objects in the current document which could serve as a Base for a job.'''
        return sorted(filter(lambda obj: cls.isBaseCandidate(obj) , FreeCAD.ActiveDocument.Objects), key=lambda o: o.Label)

    @classmethod
    def isBaseCandidate(cls, obj):
        '''Answer true if the given object can be used as a Base for a job.'''
        return PathUtil.isValidBaseObject(obj) or (hasattr(obj, 'Proxy') and isinstance(obj.Proxy, ArchPanel.PanelSheet))


class ViewProviderJob:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        self.taskPanel = None

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


class TaskPanel:
    def __init__(self, vobj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Edit Job"))
        self.vobj = vobj
        self.obj = vobj.Object
        self.deleteOnReject = deleteOnReject
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/JobEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/JobEdit.ui")

        currentPostProcessor = self.obj.PostProcessor
        postProcessors = PathPreferences.allEnabledPostProcessors(['', currentPostProcessor])
        for post in postProcessors:
            self.form.cboPostProcessor.addItem(post)
        # update the enumeration values, just to make sure all selections are valid
        self.obj.PostProcessor = postProcessors
        self.obj.PostProcessor = currentPostProcessor

        for o in ObjectPathJob.baseCandidates():
            self.form.cboBaseObject.addItem(o.Label, o)


        self.postProcessorDefaultTooltip = self.form.cboPostProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.cboPostProcessorArgs.toolTip()

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
            for child in self.obj.Group:
                FreeCAD.ActiveDocument.removeObject(child.Name)
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        self.vobj.Proxy.resetTaskPanel()
        return True

    def updateTooltips(self):
        if hasattr(self.obj, "Proxy") and hasattr(self.obj.Proxy, "tooltip") and self.obj.Proxy.tooltip:
            self.form.cboPostProcessor.setToolTip(self.obj.Proxy.tooltip)
            if hasattr(self.obj.Proxy, "tooltipArgs") and self.obj.Proxy.tooltipArgs:
                self.form.cboPostProcessorArgs.setToolTip(self.obj.Proxy.tooltipArgs)
            else:
                self.form.cboPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)
        else:
            self.form.cboPostProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.cboPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)

    def getFields(self):
        '''sets properties in the object to match the form'''
        if self.obj:
            self.obj.PostProcessor = str(self.form.cboPostProcessor.currentText())
            self.obj.PostProcessorArgs = str(self.form.cboPostProcessorArgs.displayText())
            self.obj.Label = str(self.form.leLabel.text())
            self.obj.PostProcessorOutputFile = str(self.form.leOutputFile.text())

            oldlist = self.obj.Group
            newlist = []

            for index in xrange(self.form.PathsList.count()):
                item = self.form.PathsList.item(index)
                for olditem in oldlist:
                    if olditem.Label == item.text():
                        newlist.append(olditem)
            self.obj.Group = newlist

            selObj = self.form.cboBaseObject.itemData(self.form.cboBaseObject.currentIndex())
            if self.form.chkCreateClone.isChecked():
                selObj = Draft.clone(selObj)
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

        self.form.leLabel.setText(self.obj.Label)
        self.form.leOutputFile.setText(self.obj.PostProcessorOutputFile)

        self.selectComboBoxText(self.form.cboPostProcessor, self.obj.PostProcessor)
        self.form.cboPostProcessorArgs.setText(self.obj.PostProcessorArgs)
        self.obj.Proxy.onChanged(self.obj, "PostProcessor")
        self.updateTooltips()

        self.form.PathsList.clear()
        for child in self.obj.Group:
            self.form.PathsList.addItem(child.Label)

        baseindex = -1
        if self.obj.Base:
            baseindex = self.form.cboBaseObject.findText(self.obj.Base.Label, QtCore.Qt.MatchFixedString)
        else:
            for o in FreeCADGui.Selection.getCompleteSelection():
                baseindex = self.form.cboBaseObject.findText(o.Label, QtCore.Qt.MatchFixedString)
        if baseindex >= 0:
            self.form.cboBaseObject.setCurrentIndex(baseindex)


    def open(self):
        pass

    def setFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("Path_Job", "Select Output File"), None, translate("Path_Job", "All Files (*.*)"))
        if filename and filename[0]:
            self.obj.PostProcessorOutputFile = str(filename[0])
            self.setFields()

    def setupUi(self):
        # Connect Signals and Slots
        self.form.cboPostProcessor.currentIndexChanged.connect(self.getFields)
        self.form.cboPostProcessorArgs.editingFinished.connect(self.getFields)
        self.form.leOutputFile.editingFinished.connect(self.getFields)
        self.form.leLabel.editingFinished.connect(self.getFields)
        self.form.btnSelectFile.clicked.connect(self.setFile)
        self.form.PathsList.indexesMoved.connect(self.getFields)
        self.form.cboBaseObject.currentIndexChanged.connect(self.getFields)

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
        for base in ObjectPathJob.baseCandidates():
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
        FreeCADGui.addModule('PathScripts.PathJob')
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
        try:
            FreeCADGui.doCommand('App.ActiveDocument.addObject("Path::FeatureCompoundPython", "Job")')
            if template:
                template = "'%s'" % template
            else:
                template = 'None'
            FreeCADGui.doCommand('PathScripts.PathJob.ObjectPathJob(App.ActiveDocument.ActiveObject, App.ActiveDocument.%s, %s)' % (base.Name, template))
            FreeCAD.ActiveDocument.commitTransaction()
        except:
            PathLog.error(sys.exc_info())
            FreeCAD.ActiveDocument.abortTransaction()

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
