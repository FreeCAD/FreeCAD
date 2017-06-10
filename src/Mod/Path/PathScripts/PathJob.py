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

import Draft
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathToolController as PathToolController
import lxml.etree as xml
import sys

from PySide import QtCore, QtGui
from PathScripts.PathPostProcessor import PostProcessor
from PathScripts.PathPreferences import PathPreferences

# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

LOG_MODULE = PathLog.thisModule()
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Job object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectPathJob:

    def __init__(self, obj, base, template = ""):
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
        self.assignTemplate(template)

    def assignTemplate(self, template):
        if template:
            tree = xml.parse(template)
            for tc in tree.getroot().iter('ToolController'):
                PathToolController.CommandPathToolController.FromTemplate(self.obj, tc)
        else:
            PathToolController.CommandPathToolController.Create(self.obj.Name)

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


class ViewProviderJob:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel(vobj.Object, self.deleteObjectsOnReject())
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        self.deleteOnReject = False
        return True

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
    def __init__(self, obj, deleteOnReject):
        PathLog.error("Edit Job")
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Edit Job"))
        self.obj = obj
        self.deleteOnReject = deleteOnReject
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/JobEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/JobEdit.ui")

        currentPostProcessor = obj.PostProcessor
        postProcessors = PathPreferences.allEnabledPostProcessors(['', currentPostProcessor])
        for post in postProcessors:
            self.form.cboPostProcessor.addItem(post)
        # update the enumeration values, just to make sure all selections are valid
        self.obj.PostProcessor = postProcessors
        self.obj.PostProcessor = currentPostProcessor

        self.form.cboBaseObject.addItem("")
        for o in FreeCAD.ActiveDocument.Objects:
            if hasattr(o, "Shape"):
                self.form.cboBaseObject.addItem(o.Name)


        self.postProcessorDefaultTooltip = self.form.cboPostProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.cboPostProcessorArgs.toolTip()

    def accept(self):
        self.getFields()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject:
            PathLog.error("Uncreate Job")
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Uncreate Job"))
            for child in self.obj.Group:
                FreeCAD.ActiveDocument.removeObject(child.Name)
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

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
                    if olditem.Name == item.text():
                        newlist.append(olditem)
            self.obj.Group = newlist

            objName = self.form.cboBaseObject.currentText()
            selObj = FreeCAD.ActiveDocument.getObject(objName)
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
            self.form.PathsList.addItem(child.Name)

        baseindex = -1
        if self.obj.Base:
            baseindex = self.form.cboBaseObject.findText(self.obj.Base.Name, QtCore.Qt.MatchFixedString)
        else:
            for o in FreeCADGui.Selection.getCompleteSelection():
                baseindex = self.form.cboBaseObject.findText(o.Name, QtCore.Qt.MatchFixedString)
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
        for solid in sorted(filter(lambda obj: hasattr(obj, 'Shape') and obj.Shape.isClosed(), FreeCAD.ActiveDocument.Objects), key=lambda o: o.Label):
            if solid.Label == selected:
                index = self.dialog.cbModel.count()
            self.dialog.cbModel.addItem(solid.Label)
        self.dialog.cbModel.setCurrentIndex(index)

    def getModel(self):
        label = self.dialog.cbModel.currentText()
        return filter(lambda obj: obj.Label == label, FreeCAD.ActiveDocument.Objects)[0]

    def getTemplate(self):
        return self.dialog.cbTemplate.currentText()

    def exec_(self):
        return self.dialog.exec_()

class CommandJobCreate:

    def GetResources(self):
        return {'Pixmap': 'Path-Job',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Job"),
                #'Accel': "P, J",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Creates a Path Job object")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        dialog = DlgJobCreate()
        if dialog.exec_() == 1:
            self.Create(dialog.getModel(), dialog.getTemplate())
            FreeCAD.ActiveDocument.recompute()

    @classmethod
    def Create(cls, base, template):
        FreeCADGui.addModule('PathScripts.PathJob')
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
        snippet = '''App.ActiveDocument.addObject("Path::FeatureCompoundPython", "Job")
PathScripts.PathJob.ObjectPathJob(App.ActiveDocument.ActiveObject, App.ActiveDocument.%s, "%s")''' % (base.Name, template)
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()

class CommandJobExportTemplate:

    def GetResources(self):
        return {'Pixmap': 'Path-Job',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Export Template"),
                #'Accel': "P, T",
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
        root = xml.Element('PathJobConfiguration')
        for obj in job.Group:
            if hasattr(obj, 'Tool') and hasattr(obj, 'SpindleDir'):
                attrs = {}
                attrs['label']  = ("%s" % (obj.Label))
                attrs['nr']     = ("%d" % (obj.ToolNumber))
                attrs['vfeed']  = ("%s" % (obj.VertFeed))
                attrs['hfeed']  = ("%s" % (obj.HorizFeed))
                attrs['vrapid'] = ("%s" % (obj.VertRapid))
                attrs['hrapid'] = ("%s" % (obj.HorizRapid))
                attrs['speed']  = ("%f" % (obj.SpindleSpeed))
                attrs['dir']    = ("%s" % (obj.SpindleDir))

                tc = xml.SubElement(root, 'ToolController', attrs)
                tc.append(xml.fromstring(obj.Tool.Content))
        xml.ElementTree(root).write(path, pretty_print=True)

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Job', CommandJobCreate())
    FreeCADGui.addCommand('Path_ExportTemplate', CommandJobExportTemplate())

FreeCAD.Console.PrintLog("Loading PathJob... done\n")
