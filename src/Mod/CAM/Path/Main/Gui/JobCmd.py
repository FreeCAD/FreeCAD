# -*- coding: utf-8 -*-
# ***************************************************************************
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

from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Main.Gui.JobDlg as PathJobDlg
import Path.Main.Job as PathJob
import Path.Main.Stock as PathStock
import json
import os


translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CommandJobCreate:
    """
    Command used to create a command.
    When activated the command opens a dialog allowing the user to select a base object (has to be a solid)
    and a template to be used for the initial creation.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_Job",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Job", "Job"),
            "Accel": "P, J",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Job", "Creates a CAM Job"),
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        dialog = PathJobDlg.JobCreate()
        dialog.setupTemplate()
        dialog.setupModel()
        if dialog.exec_() == 1:
            models = dialog.getModels()
            if models:
                self.Execute(models, dialog.getTemplate())
                FreeCAD.ActiveDocument.recompute()

    @classmethod
    def Execute(cls, base, template):
        FreeCADGui.addModule("Path.Main.Gui.Job")
        if template:
            template = "'%s'" % template
        else:
            template = "None"
        FreeCADGui.doCommand(
            "Path.Main.Gui.Job.Create(%s, %s)" % ([o.Name for o in base], template)
        )


class CommandJobTemplateExport:
    """
    Command to export a template of a given job.
    Opens a dialog to select the file to store the template in. If the template is stored in Path's
    file path (see preferences) and named in accordance with job_*.json it will automatically be found
    on Job creation and be available for selection.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ExportTemplate",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ExportTemplate", "Export Template"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ExportTemplate",
                "Exports CAM Job as a template to be used for other jobs",
            ),
        }

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
            if hasattr(job, "Proxy") and isinstance(job.Proxy, PathJob.ObjectJob):
                return job
        return None

    def IsActive(self):
        return self.GetJob() is not None

    def Activated(self):
        job = self.GetJob()
        dialog = PathJobDlg.JobTemplateExport(job)
        if dialog.exec_() == 1:
            self.SaveDialog(job, dialog)

    @classmethod
    def SaveDialog(cls, job, dialog):
        foo = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(),
            "Path - Job Template",
            Path.Preferences.filePath(),
            "job_*.json",
        )[0]
        if foo:
            if not os.path.basename(foo).startswith("job_"):
                foo = os.path.join(os.path.dirname(foo), "job_" + os.path.basename(foo))
            if not foo.endswith(".json"):
                foo = foo + ".json"
            cls.Execute(job, foo, dialog)

    @classmethod
    def Execute(cls, job, path, dialog=None):
        attrs = job.Proxy.templateAttrs(job)

        # post processor settings
        if dialog and not dialog.includePostProcessing():
            attrs.pop(PathJob.JobTemplate.PostProcessor, None)
            attrs.pop(PathJob.JobTemplate.PostProcessorArgs, None)
            attrs.pop(PathJob.JobTemplate.PostProcessorOutputFile, None)

        # tool controller settings
        toolControllers = dialog.includeToolControllers() if dialog else job.Tools.Group
        if toolControllers:
            tcAttrs = [tc.Proxy.templateAttrs(tc) for tc in toolControllers]
            attrs[PathJob.JobTemplate.ToolController] = tcAttrs

        # stock settings
        stockAttrs = None
        if dialog:
            if dialog.includeStock():
                stockAttrs = PathStock.TemplateAttributes(
                    job.Stock,
                    dialog.includeStockExtent(),
                    dialog.includeStockPlacement(),
                )
        else:
            stockAttrs = PathStock.TemplateAttributes(job.Stock)
        if stockAttrs:
            attrs[PathJob.JobTemplate.Stock] = stockAttrs

        # setup sheet
        setupSheetAttrs = None
        if dialog:
            setupSheetAttrs = job.Proxy.setupSheet.templateAttributes(
                dialog.includeSettingToolRapid(),
                dialog.includeSettingCoolant(),
                dialog.includeSettingOperationHeights(),
                dialog.includeSettingOperationDepths(),
                dialog.includeSettingOpsSettings(),
            )
        else:
            setupSheetAttrs = job.Proxy.setupSheet.templateAttributes(True, True, True)
        if setupSheetAttrs:
            attrs[PathJob.JobTemplate.SetupSheet] = setupSheetAttrs

        encoded = job.Proxy.setupSheet.encodeTemplateAttributes(attrs)
        # write template
        with open(str(path), "w") as fp:
            json.dump(encoded, fp, sort_keys=True, indent=2)


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Job", CommandJobCreate())
    FreeCADGui.addCommand("CAM_ExportTemplate", CommandJobTemplateExport())

FreeCAD.Console.PrintLog("Loading PathJobCmd... done\n")
