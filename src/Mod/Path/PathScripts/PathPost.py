# -*- coding: utf-8 -*-
# ***************************************************************************
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

""" Post Process command that will make use of the Output File and Post Processor entries in PathJob """

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
from datetime import datetime
from PySide.QtCore import QT_TRANSLATE_NOOP

LOG_MODULE = PathLog.thisModule()

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


class _TempObject:
    Path = None
    Name = "Fixture"
    Label = "Fixture"


class DlgSelectPostProcessor:
    def __init__(self, parent=None):
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgSelectPostProcessor.ui")
        firstItem = None
        for post in PathPreferences.allEnabledPostProcessors():
            item = QtGui.QListWidgetItem(post)
            item.setFlags(
                QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsEnabled
            )
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

        if "%D" in filename:
            D = FreeCAD.ActiveDocument.FileName
            if D:
                D = os.path.dirname(D)
                # in case the document is in the current working directory
                if not D:
                    D = "."
            else:
                FreeCAD.Console.PrintError(
                    "Please save document in order to resolve output path!\n"
                )
                return None
            filename = filename.replace("%D", D)

        if "%d" in filename:
            d = FreeCAD.ActiveDocument.Label
            filename = filename.replace("%d", d)

        if "%j" in filename:
            j = job.Label
            filename = filename.replace("%j", j)

        if "%M" in filename:
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
            M = pref.GetString("MacroPath", FreeCAD.getUserAppDataDir())
            filename = filename.replace("%M", M)

        if "%s" in filename:
            if job.SplitOutput:
                filename = filename.replace("%s", "_" + str(self.subpart))
                self.subpart += 1
            else:
                filename = filename.replace("%s", "")

        policy = PathPreferences.defaultOutputPolicy()

        openDialog = policy == "Open File Dialog"
        if os.path.isdir(filename) or not os.path.isdir(os.path.dirname(filename)):
            # Either the entire filename resolves into a directory or the parent directory doesn't exist.
            # Either way I don't know what to do - ask for help
            openDialog = True

        if os.path.isfile(filename) and not openDialog:
            if policy == "Open File Dialog on conflict":
                openDialog = True
            elif policy == "Append Unique ID on conflict":
                fn, ext = os.path.splitext(filename)
                nr = fn[-3:]
                n = 1
                if nr.isdigit():
                    n = int(nr)
                while os.path.isfile("%s%03d%s" % (fn, n, ext)):
                    n = n + 1
                filename = "%s%03d%s" % (fn, n, ext)

        if openDialog:
            foo = QtGui.QFileDialog.getSaveFileName(
                QtGui.QApplication.activeWindow(), "Output File", filename
            )
            if foo[0]:
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
        return {
            "Pixmap": "Path_Post",
            "MenuText": QT_TRANSLATE_NOOP("Path_Post", "Post Process"),
            "Accel": "P, P",
            "ToolTip": QT_TRANSLATE_NOOP("Path_Post", "Post Process the selected Job"),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            if FreeCADGui.Selection.getCompleteSelection():
                for o in FreeCAD.ActiveDocument.Objects:
                    if o.Name[:3] == "Job":
                        return True

        return False

    def exportObjectsWith(self, objs, partname, job, sequence, extraargs=None):
        PathLog.debug(f"{objs}, {partname}, {sequence}, {extraargs}")
        # check if the user has a project and has set the default post and
        # output filename
        # extraargs can be passed in at this time

        postArgs = PathPreferences.defaultPostProcessorArgs()
        if hasattr(job, "PostProcessorArgs") and job.PostProcessorArgs:
            postArgs = job.PostProcessorArgs
        elif hasattr(job, "PostProcessor") and job.PostProcessor:
            postArgs = ""

        if extraargs is not None:
            postArgs += " {}".format(extraargs)

        PathLog.debug(postArgs)

        postname = self.resolvePostProcessor(job)
        filename = resolveFileName(job, partname, sequence)

        if postname and filename:
            print("post: %s(%s, %s)" % (postname, filename, postArgs))
            processor = PostProcessor.load(postname)
            gcode = processor.export(objs, filename, postArgs)
            return (False, gcode, filename)
        else:
            return (True, "", filename)

    def buildPostList(self, job):
        """
        Parses the job and returns the list(s) of objects to be written by the post
        Postlist is a list of lists.  Each sublist is intended to be a separate file
        """
        orderby = job.OrderOutputBy

        fixturelist = []
        for f in job.Fixtures:
            # create an object to serve as the fixture path
            fobj = _TempObject()
            fobj.Label = f
            c1 = Path.Command(f)
            c2 = Path.Command(
                "G0 Z"
                + str(
                    job.Stock.Shape.BoundBox.ZMax
                    + job.SetupSheet.ClearanceHeightOffset.Value
                )
            )
            fobj.Path = Path.Path([c1, c2])
            # fobj.InList.append(job)
            fixturelist.append(fobj)

        postlist = []

        if orderby == "Fixture":
            PathLog.debug("Ordering by Fixture")
            # Order by fixture means all operations and tool changes will be completed in one
            # fixture before moving to the next.

            for f in fixturelist:
                scratchpad = [(f, None)]

                # Now generate the gcode
                for obj in job.Operations.Group:
                    if not PathUtil.opProperty(obj, "Active"):
                        continue
                    tc = PathUtil.toolControllerForOp(obj)
                    scratchpad.append((obj, tc))

                sublist = []
                temptool = None
                for item in scratchpad:
                    if item[1] in [temptool, None]:
                        sublist.append(item[0])
                    else:
                        sublist.append(item[1])
                        temptool = item[1]
                        sublist.append(item[0])
                postlist.append(sublist)

        elif orderby == "Tool":
            PathLog.debug("Ordering by Tool")
            # Order by tool means tool changes are minimized.
            # all operations with the current tool are processed in the current
            # fixture before moving to the next fixture.

            currTool = None

            # Now generate the gcode
            curlist = []  # list of ops for tool, will repeat for each fixture
            # sublist = []  # list of ops for output splitting

            for idx, obj in enumerate(job.Operations.Group):

                # check if the operation is active
                active = PathUtil.opProperty(obj, "Active")

                tc = PathUtil.toolControllerForOp(obj)

                if not active:  # pass on any inactive ops
                    continue

                if tc is None:
                    curlist.append((obj, None))
                    continue

                if tc == currTool:
                    curlist.append((obj, tc))
                    continue

                if tc != currTool and currTool is None:  # first TC
                    currTool = tc
                    curlist.append((obj, tc))
                    continue

                if tc != currTool and currTool is not None:  # TC changed
                    if tc.ToolNumber == currTool.ToolNumber:  # Same tool /diff params
                        curlist.append((obj, tc))
                        currTool = tc
                    else:  # Actual Toolchange
                        # dump current state to postlist
                        sublist = []
                        t = None
                        for fixture in fixturelist:
                            sublist.append(fixture)
                            for item in curlist:
                                if item[1] == t:
                                    sublist.append(item[0])
                                else:
                                    sublist.append(item[1])
                                    t = item[1]
                                    sublist.append(item[0])

                        postlist.append(sublist)

                        # set up for next tool group
                        currTool = tc
                        curlist = [(obj, tc)]

            # flush remaining curlist to output
            sublist = []
            t = None
            for fixture in fixturelist:
                sublist.append(fixture)
                for item in curlist:
                    if item[1] == t:
                        sublist.append(item[0])
                    else:
                        sublist.append(item[1])
                        t = item[1]
                        sublist.append(item[0])
            postlist.append(sublist)

        elif orderby == "Operation":
            PathLog.debug("Ordering by Operation")
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None
            # firstFixture = True

            # Now generate the gcode
            for obj in job.Operations.Group:
                scratchpad = []
                tc = PathUtil.toolControllerForOp(obj)
                if not PathUtil.opProperty(obj, "Active"):
                    continue

                PathLog.debug("obj: {}".format(obj.Name))
                for f in fixturelist:

                    scratchpad.append((f, None))
                    scratchpad.append((obj, tc))

                sublist = []
                temptool = None
                for item in scratchpad:
                    if item[1] in [temptool, None]:
                        sublist.append(item[0])
                    else:
                        sublist.append(item[1])
                        temptool = item[1]
                        sublist.append(item[0])
                postlist.append(sublist)

        if job.SplitOutput:
            return postlist
        else:
            finalpostlist = [item for slist in postlist for item in slist]
            return [finalpostlist]

    def Activated(self):
        PathLog.track()
        FreeCAD.ActiveDocument.openTransaction("Post Process the Selected path(s)")
        FreeCADGui.addModule("PathScripts.PathPost")

        # Attempt to figure out what the user wants to post-process
        # If a job is selected, post that.
        # If there's only one job in a document, post it.
        # If a user has selected a subobject of a job, post the job.
        # If multiple jobs and can't guess, ask them.

        selected = FreeCADGui.Selection.getSelectionEx()
        if len(selected) > 1:
            FreeCAD.Console.PrintError(
                "Please select a single job or other path object\n"
            )
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
            PathLog.debug(f"Possible post objects: {targetlist}")
            if len(targetlist) > 1:
                jobname, result = QtGui.QInputDialog.getItem(
                    None, translate("Path", "Choose a Path Job"), None, targetlist
                )

                if result is False:
                    return
            else:
                jobname = targetlist[0]
            job = FreeCAD.ActiveDocument.getObject(jobname)

        PathLog.debug(f"about to postprocess job: {job.Name}")

        postlist = self.buildPostList(job)

        filenames = []

        success = True
        finalgcode = ""
        for idx, section in enumerate(postlist):
            partname = section[0]
            sublist = section[1]

            result, gcode, name = self.exportObjectsWith(sublist, partname, job, idx)
            filenames.append(name)
            PathLog.debug(f"{result}, {gcode}, {name}")

            if result is None:
                success = False
            else:
                finalgcode += gcode

        if success:
            if hasattr(job, "LastPostProcessDate"):
                job.LastPostProcessDate = str(datetime.now())
            if hasattr(job, "LastPostProcessOutput"):
                job.LastPostProcessOutput = " \n".join(filenames)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Post", CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
