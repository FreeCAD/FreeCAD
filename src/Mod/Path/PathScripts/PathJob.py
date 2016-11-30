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

import FreeCAD
import Path
from PySide import QtCore, QtGui
from PathScripts.PathPostProcessor import PostProcessor
from PathScripts.PathPreferences import PathPreferences
import Draft
import os
import glob


FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Job object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class ObjectPathJob:

    def __init__(self, obj):
        #        obj.addProperty("App::PropertyFile", "PostProcessor", "CodeOutput", "Select the Post Processor file for this project")
        obj.addProperty("App::PropertyFile", "OutputFile", "CodeOutput", QtCore.QT_TRANSLATE_NOOP("App::Property","The NC output file for this project"))
        obj.OutputFile = PathPreferences.defaultOutputFile()
        obj.setEditorMode("OutputFile", 0)  # set to default mode

        obj.addProperty("App::PropertyString", "Description", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional description for this job"))
        obj.addProperty("App::PropertyEnumeration", "PostProcessor", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","Select the Post Processor"))
        obj.PostProcessor = PathPreferences.allEnabledPostProcessors([''])
        obj.PostProcessor = PathPreferences.defaultPostProcessor()
        obj.addProperty("App::PropertyString", "PostProcessorArgs", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property", "Arguments for the Post Processor (specific to the script)"))
        obj.PostProcessorArgs = PathPreferences.defaultPostProcessorArgs()
        obj.addProperty("App::PropertyString",    "MachineName", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","Name of the Machine that will use the CNC program"))

        obj.addProperty("Path::PropertyTooltable", "Tooltable", "Base", QtCore.QT_TRANSLATE_NOOP("App::Property","The tooltable used for this CNC program"))

        obj.addProperty("App::PropertyEnumeration", "MachineUnits", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","Units that the machine works in, ie Metric or Inch"))
        obj.MachineUnits = ['Metric', 'Inch']

        obj.addProperty("App::PropertyDistance", "X_Max", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Maximum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Y_Max", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Maximum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Z_Max", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Maximum distance in X the machine can travel"))

        obj.addProperty("App::PropertyDistance", "X_Min", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Minimum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Y_Min", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Minimum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Z_Min", "Limits", QtCore.QT_TRANSLATE_NOOP("App::Property","The Minimum distance in X the machine can travel"))

        obj.addProperty("App::PropertyLink", "Base", "Base", "The base object for all operations")

        obj.Proxy = self

        if FreeCAD.GuiUp:
            ViewProviderJob(obj.ViewObject)


    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        mode = 2
        obj.setEditorMode('Placement', mode)

        if prop == "PostProcessor" and obj.PostProcessor:
            processor = PostProcessor.load(obj.PostProcessor)
            if processor.units:
                obj.MachineUnits = processor.units
            if processor.machineName:
                obj.MachineName = processor.machineName
            if processor.cornerMax:
                obj.X_Max = processor.cornerMax['x']
                obj.Y_Max = processor.cornerMax['y']
                obj.Z_Max = processor.cornerMax['z']
            if processor.cornerMin:
                obj.X_Min = processor.cornerMin['x']
                obj.Y_Min = processor.cornerMin['y']
                obj.Z_Min = processor.cornerMin['z']
            self.tooltip = processor.tooltip
            self.tooltipArgs = processor.tooltipArgs

            self.PostProcessorArgs = ''

    # def getToolControllers(self, obj):
    #     '''returns a list of ToolControllers for the current job'''
    #     controllers = []
    #     for o in obj.Group:
    #         if "Proxy" in o.PropertiesList:
    #             if isinstance(o.Proxy, PathLoadTool.LoadTool):
    #                 controllers.append (o.Name)
    #     return controllers


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


class CommandJob:

    def GetResources(self):
        return {'Pixmap': 'Path-Job',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Job"),
                'Accel': "P, J",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Creates a Path Job object")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        CommandJob.Create()
        FreeCAD.ActiveDocument.recompute()

    @staticmethod
    def Create():
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
        FreeCADGui.addModule('PathScripts.PathUtils')
        FreeCADGui.addModule('PathScripts.PathLoadTool')
        snippet = '''
import PathScripts.PathLoadTool as PathLoadTool
obj = FreeCAD.ActiveDocument.addObject("Path::FeatureCompoundPython", "Job")
PathScripts.PathJob.ObjectPathJob(obj)
PathLoadTool.CommandPathLoadTool.Create(obj.Name)
tl = obj.Group[0]
tool = Path.Tool()
tool.Diameter = 5.0
tool.Name = "Default Tool"
tool.CuttingEdgeHeight = 15.0
tool.ToolType = "EndMill"
tool.Material = "HighSpeedSteel"
obj.Tooltable.addTools(tool)
tl.ToolNumber = 1
obj.ViewObject.Proxy.deleteOnReject = True
obj.ViewObject.startEditing()
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()


class TaskPanel:
    def __init__(self, obj, deleteOnReject):
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
                self.form.cboPostProcessorArgs.setText(self.obj.PostProcessorArgs)
            else:
                self.form.cboPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)
                self.form.cboPostProcessorArgs.setText('')
        else:
            self.form.cboPostProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.cboPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)
            self.form.cboPostProcessorArgs.setText('')

    def getFields(self):
        '''sets properties in the object to match the form'''
        if self.obj:
            self.obj.PostProcessor = str(self.form.cboPostProcessor.currentText())
            self.obj.PostProcessorArgs = str(self.form.cboPostProcessorArgs.displayText())
            self.obj.Label = str(self.form.leLabel.text())
            self.obj.OutputFile = str(self.form.leOutputFile.text())

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
        self.form.leOutputFile.setText(self.obj.OutputFile)

        self.selectComboBoxText(self.form.cboPostProcessor, self.obj.PostProcessor)
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
        print baseindex
        if baseindex >= 0:
            self.form.cboBaseObject.blockSignals(True)
            self.form.cboBaseObject.setCurrentIndex(baseindex)
            self.form.cboBaseObject.blockSignals(False)


    def open(self):
        pass

    def setFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(self.form, translate("PathJob", "Select Output File", None), None, translate("Path Job", "All Files (*.*)", None))
        if filename and filename[0]:
            self.obj.OutputFile = str(filename[0])
            self.setFields()

    def setupUi(self):
        # Connect Signals and Slots
        self.form.cboPostProcessor.currentIndexChanged.connect(self.getFields)
        self.form.leOutputFile.editingFinished.connect(self.getFields)
        self.form.leLabel.editingFinished.connect(self.getFields)
        self.form.btnSelectFile.clicked.connect(self.setFile)
        self.form.PathsList.indexesMoved.connect(self.getFields)
        self.form.cboBaseObject.currentIndexChanged.connect(self.getFields)

        self.setFields()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Job', CommandJob())

FreeCAD.Console.PrintLog("Loading PathJob... done\n")
