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
import Path
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import os

from PathScripts.PathPostProcessor import PostProcessor
from PySide import QtCore, QtGui


LOG_MODULE = PathLog.thisModule()

PathLog.setLevel(PathLog.Level.DEBUG, LOG_MODULE)


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class _TempObject:
        Path = None
        Name = "Fixture"
        InList = []
        Label = "Fixture"


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
    subpart = 1

    def resolveFileName(self, job):
        path = PathPreferences.defaultOutputFile()
        if job.PostProcessorOutputFile:
            path = job.PostProcessorOutputFile
        filename = path

        if '%D' in filename:
            D = FreeCAD.ActiveDocument.FileName
            if D:
                D = os.path.dirname(D)
                # in case the document is in the current working directory
                if not D:
                    D = '.'
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

        if '%s' in filename:
            if job.SplitOutput:
                filename = filename.replace('%s', '_'+str(self.subpart))
                self.subpart += 1
            else:
                filename = filename.replace('%s', '')

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
            foo = QtGui.QFileDialog.getSaveFileName(QtGui.QApplication.activeWindow(), "Output File", filename)
            if foo:
                filename = foo[0]
            else:
                filename = None

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

    def exportObjectsWith(self, objs, job, needFilename=True, filepart=None):
        PathLog.track()
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
        PathLog.track()
        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Post", "Post Process the Selected path(s)"))
        FreeCADGui.addModule("PathScripts.PathPost")

        # Attempt to figure out what the user wants to post-process
        # If a job is selected, post that.
        # If there's only one job in a document, post it.
        # If a user has selected a subobject of a job, post the job.
        # If multiple jobs and can't guess, ask them.

        selected = FreeCADGui.Selection.getSelectionEx()
        if len(selected) > 1:
            FreeCAD.Console.PrintError("Please select a single job or other path object\n")
            return
        elif len(selected) == 1:
            sel = selected[0].Object
            if sel.Name[:3] == "Job":
                job = sel
            elif hasattr(sel, "Path"):
                try:
                    job = PathUtils.findParentJob(sel)
                except Exception:
                    job = None
            else:
                job = None
        if job is None:
            targetlist = []
            for o in FreeCAD.ActiveDocument.Objects:
                if hasattr(o, "Proxy"):
                    if isinstance(o.Proxy, PathJob.ObjectJob):
                        targetlist.append(o.Label)
            PathLog.debug("Possible post objects: {}".format(targetlist))
            if len(targetlist) > 1:
                form = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobChooser.ui")
                form.cboProject.addItems(targetlist)
                r = form.exec_()
                if r is False:
                    return
                else:
                    jobname = form.cboProject.currentText()
            else:
                jobname = targetlist[0]
            job = FreeCAD.ActiveDocument.getObject(jobname)

        PathLog.debug("about to postprocess job: {}".format(job.Name))

        # Build up an ordered list of operations and tool changes.
        # Then post-the ordered list
        if hasattr(job, "Fixtures"):
            wcslist = job.Fixtures
        else:
            wcslist = ['G54']

        if hasattr(job, "OrderOutputBy"):
            orderby = job.OrderOutputBy
        else:
            orderby = "Operation"

        if hasattr(job, "SplitOutput"):
            split = job.SplitOutput
        else:
            split = False

        postlist = []

        if orderby == 'Fixture':
            PathLog.debug("Ordering by Fixture")
            # Order by fixture means all operations and tool changes will be completed in one
            # fixture before moving to the next.

            currTool = None
            for f in wcslist:
                # create an object to serve as the fixture path
                fobj = _TempObject()
                c1 = Path.Command(f)
                c2 = Path.Command("G0 Z" + str(job.Stock.Shape.BoundBox.ZMax))
                fobj.Path = Path.Path([c1, c2])
                fobj.InList.append(job)
                sublist = [fobj]

                # Now generate the gcode
                for obj in job.Operations.Group:
                    tc = PathUtil.toolControllerForOp(obj)
                    if tc is not None:
                        if tc.ToolNumber != currTool:
                            sublist.append(tc)
                            PathLog.debug("Appending TC: {}".format(tc.Name))
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append(sublist)

        elif orderby == 'Tool':
            PathLog.debug("Ordering by Tool")
            # Order by tool means tool changes are minimized.
            # all operations with the current tool are processed in the current
            # fixture before moving to the next fixture.

            currTool = None
            fixturelist = []
            for f in wcslist:
                # create an object to serve as the fixture path
                fobj = _TempObject()
                c1 = Path.Command(f)
                c2 = Path.Command("G0 Z" + str(job.Stock.Shape.BoundBox.ZMax))
                fobj.Path = Path.Path([c1, c2])
                fobj.InList.append(job)
                fixturelist.append(fobj)

            # Now generate the gcode
            curlist = []  # list of ops for tool, will repeat for each fixture
            sublist = []  # list of ops for output splitting

            for idx, obj in enumerate(job.Operations.Group):
                tc = PathUtil.toolControllerForOp(obj)
                if tc is None or tc.ToolNumber == currTool:
                    curlist.append(obj)
                elif tc.ToolNumber != currTool and currTool is None:  # first TC
                    sublist.append(tc)
                    curlist.append(obj)
                    currTool = tc.ToolNumber
                elif tc.ToolNumber != currTool and currTool is not None:  # TC
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append(sublist)
                    sublist = [tc]
                    curlist = [obj]
                    currTool = tc.ToolNumber

                if idx == len(job.Operations.Group) - 1:  # Last operation.
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append(sublist)

        elif orderby == 'Operation':
            PathLog.debug("Ordering by Operation")
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None
            fixturelist = []
            for f in wcslist:
                # create an object to serve as the fixture path
                fobj = _TempObject()
                c1 = Path.Command(f)
                c2 = Path.Command("G0 Z" + str(job.Stock.Shape.BoundBox.ZMax))
                fobj.Path = Path.Path([c1, c2])
                fobj.InList.append(job)
                fixturelist.append(fobj)

            # Now generate the gcode
            for obj in job.Operations.Group:
                sublist = []
                PathLog.debug("obj: {}".format(obj.Name))
                for fixture in fixturelist:
                    sublist.append(fixture)
                    tc = PathUtil.toolControllerForOp(obj)
                    if tc is not None:
                        if tc.ToolNumber != currTool:
                            sublist.append(tc)
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append(sublist)

        fail = True
        rc = ''
        if split:
            for slist in postlist:
                (fail, rc) = self.exportObjectsWith(slist, job)
        else:
            finalpostlist = [item for slist in postlist for item in slist]
            (fail, rc) = self.exportObjectsWith(finalpostlist, job)

        self.subpart = 1

        if fail:
            FreeCAD.ActiveDocument.abortTransaction()
        else:
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Post', CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
