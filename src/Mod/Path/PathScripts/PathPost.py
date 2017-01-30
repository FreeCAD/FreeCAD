# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Post Process command that will make use of the Output File and Post Processor entries in PathJob '''
from __future__ import print_function
import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui
from PathScripts import PathUtils
from PathScripts.PathPreferences import PathPreferences
from PathScripts.PathPostProcessor import PostProcessor
import os
import sys

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class DlgSelectPostProcessor:

    def __init__(self, parent=None):
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgSelectPostProcessor.ui")
        firstItem = None
        for post in PathPreferences.allEnabledPostProcessors():
            item = QtGui.QListWidgetItem(post)
            item.setFlags(QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsEnabled)
            self.dialog.lwPostProcessor.addItem(item)
            if not firstItem:
                firstItem = item
        if firstItem:
            self.dialog.lwPostProcessor.setCurrentItem(firstItem)
        else:
            self.dialog.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(False)
        self.tooltips = {}
        self.dialog.lwPostProcessor.itemDoubleClicked.connect(self.dialog.accept)
        self.dialog.lwPostProcessor.setMouseTracking(True)
        self.dialog.lwPostProcessor.itemEntered.connect(self.updateTooltip)

    def updateTooltip(self, item):
        if item.text() in self.tooltips.keys():
            tooltip = self.tooltips[item.text()]
        else:
            processor = PostProcessor.load(item.text())
            self.tooltips[item.text()] = processor.tooltip
            tooltip = processor.tooltip
        self.dialog.lwPostProcessor.setToolTip(tooltip)

    def exec_(self):
        if self.dialog.exec_() == 1:
            posts = self.dialog.lwPostProcessor.selectedItems()
            return posts[0].text()
        return None

class CommandPathPost:

    def resolveFileName(self, job):
        #print("resolveFileName(%s)" % job.Label)
        path = PathPreferences.defaultOutputFile()
        if job.OutputFile:
            path = job.OutputFile
        filename = path
        if '%D' in filename:
            D = FreeCAD.ActiveDocument.FileName
            if D:
                D = os.path.dirname(D)
            else:
                FreeCAD.Console.PrintError("Please save document in order to resolve output path!\n")
                return None
            filename = filename.replace('%D', D)

        if '%d' in filename:
            d = FreeCAD.ActiveDocument.Label
            filename = filename.replace('%d', d)

        if '%j' in filename:
            j = job.Label
            filename = filename.replace('%j', j)

        if '%M' in filename:
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
            M = pref.GetString("MacroPath", FreeCAD.getUserAppDataDir())
            filename = filename.replace('%M', M)

        policy = PathPreferences.defaultOutputPolicy()

        openDialog = policy == 'Open File Dialog'
        if os.path.isdir(filename) or not os.path.isdir(os.path.dirname(filename)):
            # Either the entire filename resolves into a directory or the parent directory doesn't exist.
            # Either way I don't know what to do - ask for help
            openDialog = True

        if os.path.isfile(filename) and not openDialog:
            if policy == 'Open File Dialog on conflict':
                openDialog = True
            elif policy == 'Append Unique ID on conflict':
                fn, ext = os.path.splitext(filename)
                nr = fn[-3:]
                n = 1
                if nr.isdigit():
                    n = int(nr)
                while os.path.isfile("%s%03d%s" % (fn, n, ext)):
                    n = n + 1
                filename = "%s%03d%s" % (fn, n, ext)

        if openDialog:
            foo = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), "Output File", filename)
            if foo:
                filename = foo[0]
            else:
                filename = None

        #print("resolveFileName(%s, %s) -> '%s'" % (path, policy, filename))
        return filename

    def resolvePostProcessor(self, job):
        if hasattr(job, "PostProcessor"):
            post = PathPreferences.defaultPostProcessor()
            if job.PostProcessor:
                post = job.PostProcessor
            if post and PostProcessor.exists(post):
                return post
        dlg = DlgSelectPostProcessor()
        return dlg.exec_()


    def GetResources(self):
        return {'Pixmap': 'Path-Post',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Post", "Post Process"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Post", "Post Process the selected Job")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            if FreeCADGui.Selection.getCompleteSelection():
                for o in FreeCAD.ActiveDocument.Objects:
                    if o.Name[:3] == "Job":
                        return True
        return False

    def exportObjectsWith(self, objs, job, needFilename = True):
        # check if the user has a project and has set the default post and
        # output filename
        postArgs = PathPreferences.defaultPostProcessorArgs()
        if hasattr(job, "PostProcessorArgs") and job.PostProcessorArgs:
            postArgs = job.PostProcessorArgs
        elif hasattr(job, "PostProcessor") and job.PostProcessor:
            postArgs = ''

        postname = self.resolvePostProcessor(job)
        filename = '-'
        if postname and needFilename:
            filename = self.resolveFileName(job)

        if postname and filename:
            print("post: %s(%s, %s)" % (postname, filename, postArgs))
            processor = PostProcessor.load(postname)
            gcode = processor.export(objs, filename, postArgs)
            return (False, gcode)
        else:
            return (True, '')

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Post", "Post Process the Selected path(s)"))
        FreeCADGui.addModule("PathScripts.PathPost")
        # select the Path Job that you want to post output from
        selected = FreeCADGui.Selection.getCompleteSelection()
        print("in activated %s" %(selected))

        # try to find the job, if it's not directly selected ...
        jobs = set()
        for obj in selected:
            if hasattr(obj, 'OutputFile') or hasattr(obj, 'PostProcessor'):
                jobs.add(obj)
            elif hasattr(obj, 'Path') or hasattr(obj, 'ToolNumber'):
                job = PathUtils.findParentJob(obj)
                if job:
                    jobs.add(job)

        fail = True
        rc = ''
        if len(jobs) != 1:
            FreeCAD.Console.PrintError("Please select a single job or other path object\n")
        else:
            job = jobs.pop()
            print("Job for selected objects = %s" % job.Name)
            (fail, rc) = self.exportObjectsWith(selected, job)

        if fail:
            FreeCAD.ActiveDocument.abortTransaction()
        else:
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()



if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Post', CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
