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
import glob
import os
import xml.etree.ElementTree as xml

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

FreeCAD.Console.PrintLog("Loading PathJobGui... done\n")

