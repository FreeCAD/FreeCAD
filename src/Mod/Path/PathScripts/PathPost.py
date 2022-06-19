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
import re

from PathScripts.PathPostProcessor import PostProcessor
from PySide import QtCore, QtGui
from datetime import datetime
from PySide.QtCore import QT_TRANSLATE_NOOP

LOG_MODULE = PathLog.thisModule()

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


translate = FreeCAD.Qt.translate


class _TempObject:
    Path = None
    Name = "Fixture"
    InList = []
    Label = "Fixture"


def resolveFileName(job, subpartname, sequencenumber):
    PathLog.track(subpartname, sequencenumber)

    validPathSubstitutions = ["D", "d", "M", "j"]
    validFilenameSubstitutions = ["j", "d", "T", "t", "W", "O", "S"]

    # Look for preference default
    outputpath, filename = os.path.split(PathPreferences.defaultOutputFile())
    filename, ext = os.path.splitext(filename)

    # Override with document default if it exists
    if job.PostProcessorOutputFile:
        matchstring = job.PostProcessorOutputFile
        candidateOutputPath, candidateFilename = os.path.split(matchstring)

        if candidateOutputPath:
            outputpath = candidateOutputPath

        if candidateFilename:
            filename, ext = os.path.splitext(candidateFilename)

    # Strip any invalid substitutions from the ouputpath
    for match in re.findall("%(.)", outputpath):
        if match not in validPathSubstitutions:
            outputpath = outputpath.replace(f"%{match}", "")

    # if nothing else, use current directory
    if not outputpath:
        outputpath = "."

    # Strip any invalid substitutions from the filename
    for match in re.findall("%(.)", filename):
        if match not in validFilenameSubstitutions:
            filename = filename.replace(f"%{match}", "")

    # if no filename, use the active document label
    if not filename:
        filename = FreeCAD.ActiveDocument.Label

    # if no extension, use something sensible
    if not ext:
        ext = ".nc"

    # By now we should have a sanitized path, filename and extension to work with
    PathLog.track(f"path: {outputpath} name: {filename} ext: {ext}")

    # The following section allows substitution within the path part
    PathLog.track(f"path before substitution: {outputpath}")

    if "%D" in outputpath:  # Directory of active document
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
        outputpath = outputpath.replace("%D", D)

    if "%M" in outputpath:
        M = FreeCAD.getUserMacroDir()
        outputpath = outputpath.replace("%M", M)

    # Use the file label
    if "%d" in outputpath:
        d = FreeCAD.ActiveDocument.Label
        outputpath = outputpath.replace("%d", d)

    # Use the name of the active job object
    if "%j" in outputpath:
        j = job.Label
        outputpath = outputpath.replace("%j", j)

    PathLog.track(f"path after substitution: {outputpath}")

    # The following section allows substitution within the filename part
    PathLog.track(f"filename before substitution: {filename}")

    # Use the file label
    if "%d" in filename:
        d = FreeCAD.ActiveDocument.Label
        filename = filename.replace("%d", d)

    # Use the name of the active job object
    if "%j" in filename:
        j = job.Label
        filename = filename.replace("%j", j)

    # Use the sequnce number if explicitly called
    if "%S" in filename:
        j = job.Label
        filename = filename.replace("%S", str(sequencenumber))

    # This section handles unique names for splitting output
    if job.SplitOutput:
        PathLog.track()
        if "%T" in filename and job.OrderOutputBy == "Tool":
            filename = filename.replace("%T", subpartname)

        if "%t" in filename and job.OrderOutputBy == "Tool":
            filename = filename.replace("%t", subpartname)

        if "%W" in filename and job.OrderOutputBy == "Fixture":
            filename = filename.replace("%W", subpartname)

        if "%O" in filename and job.OrderOutputBy == "Operation":
            filename = filename.replace("%O", subpartname)

        if (
            "%S" in filename
        ):  # We always add a sequence number but the user can say where
            filename = filename.replace("%S", str(sequencenumber))
        else:
            filename = f"{filename}-{sequencenumber}"

    PathLog.track(f"filename after substitution: {filename}")

    if not ext:
        ext = ".nc"
    PathLog.track(f"file extension: {ext}")

    fullPath = f"{outputpath}{os.path.sep}{filename}{ext}"

    PathLog.track(f"full filepath: {fullPath}")

    # This section determines whether user interaction is necessary
    policy = PathPreferences.defaultOutputPolicy()

    openDialog = policy == "Open File Dialog"
    # if os.path.isdir(filename) or not os.path.isdir(os.path.dirname(filename)):
    #     # Either the entire filename resolves into a directory or the parent directory doesn't exist.
    #     # Either way I don't know what to do - ask for help
    #     openDialog = True

    if not FreeCAD.GuiUp:  # if testing, or scripting, never open dialog.
        policy = "Append Unique ID on conflict"
        openDialog = False

    if os.path.isfile(fullPath) and not openDialog:
        if policy == "Open File Dialog on conflict":
            openDialog = True
        elif policy == "Append Unique ID on conflict":
            fn, ext = os.path.splitext(fullPath)
            nr = fn[-3:]
            n = 1
            if nr.isdigit():
                n = int(nr)
            while os.path.isfile("%s%03d%s" % (fn, n, ext)):
                n = n + 1
            fullPath = "%s%03d%s" % (fn, n, ext)

    if openDialog:
        foo = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(), "Output File", filename
        )
        if foo[0]:
            fullPath = foo[0]
        else:
            fullPath = None

    # remove any unused substitution strings:
    for s in validPathSubstitutions + validFilenameSubstitutions:
        fullPath = fullPath.replace(f"%{s}", "")

    fullPath = os.path.normpath(fullPath)
    PathLog.track(fullPath)
    return fullPath


def buildPostList(job):
    """Takes the job and determines the specific objects and order to
    postprocess  Returns a list of objects which can be passed to
    exportObjectsWith() for final posting"""
    wcslist = job.Fixtures
    orderby = job.OrderOutputBy

    postlist = []

    if orderby == "Fixture":
        PathLog.debug("Ordering by Fixture")
        # Order by fixture means all operations and tool changes will be completed in one
        # fixture before moving to the next.

        currTool = None
        for index, f in enumerate(wcslist):
            # create an object to serve as the fixture path
            fobj = _TempObject()
            c1 = Path.Command(f)
            fobj.Path = Path.Path([c1])
            if index != 0:
                c2 = Path.Command(
                    "G0 Z"
                    + str(
                        job.Stock.Shape.BoundBox.ZMax
                        + job.SetupSheet.ClearanceHeightOffset.Value
                    )
                )
                fobj.Path.addCommands(c2)
            fobj.InList.append(job)
            sublist = [fobj]

            # Now generate the gcode
            for obj in job.Operations.Group:
                tc = PathUtil.toolControllerForOp(obj)
                if tc is not None and PathUtil.opProperty(obj, "Active"):
                    if tc.ToolNumber != currTool:
                        sublist.append(tc)
                        PathLog.debug("Appending TC: {}".format(tc.Name))
                        currTool = tc.ToolNumber
                sublist.append(obj)
            postlist.append((f, sublist))

    elif orderby == "Tool":
        PathLog.debug("Ordering by Tool")
        # Order by tool means tool changes are minimized.
        # all operations with the current tool are processed in the current
        # fixture before moving to the next fixture.

        toolstring = "None"
        currTool = None

        # Build the fixture list
        fixturelist = []
        for f in wcslist:
            # create an object to serve as the fixture path
            fobj = _TempObject()
            c1 = Path.Command(f)
            c2 = Path.Command(
                "G0 Z"
                + str(
                    job.Stock.Shape.BoundBox.ZMax
                    + job.SetupSheet.ClearanceHeightOffset.Value
                )
            )
            fobj.Path = Path.Path([c1, c2])
            fobj.InList.append(job)
            fixturelist.append(fobj)

        # Now generate the gcode
        curlist = []  # list of ops for tool, will repeat for each fixture
        sublist = []  # list of ops for output splitting

        PathLog.track(job.PostProcessorOutputFile)
        for idx, obj in enumerate(job.Operations.Group):
            PathLog.track(obj.Label)

            # check if the operation is active
            if not getattr(obj, "Active", True):
                PathLog.track()
                continue

            # Determine the proper string for the Op's TC
            tc = PathUtil.toolControllerForOp(obj)
            if tc is None:
                tcstring = "None"
            elif "%T" in job.PostProcessorOutputFile:
                tcstring = f"{tc.ToolNumber}"
            else:
                tcstring = re.sub(r"[^\w\d-]", "_", tc.Label)
            PathLog.track(toolstring)

            if tc is None or tc.ToolNumber == currTool:
                curlist.append(obj)
            elif tc.ToolNumber != currTool and currTool is None:  # first TC
                sublist.append(tc)
                curlist.append(obj)
                currTool = tc.ToolNumber
                toolstring = tcstring

            elif tc.ToolNumber != currTool and currTool is not None:  # TC
                for fixture in fixturelist:
                    sublist.append(fixture)
                    sublist.extend(curlist)
                postlist.append((toolstring, sublist))
                sublist = [tc]
                curlist = [obj]
                currTool = tc.ToolNumber
                toolstring = tcstring

            if idx == len(job.Operations.Group) - 1:  # Last operation.
                for fixture in fixturelist:
                    sublist.append(fixture)
                    sublist.extend(curlist)

                postlist.append((toolstring, sublist))

    elif orderby == "Operation":
        PathLog.debug("Ordering by Operation")
        # Order by operation means ops are done in each fixture in
        # sequence.
        currTool = None
        firstFixture = True

        # Now generate the gcode
        for obj in job.Operations.Group:

            # check if the operation is active
            if not getattr(obj, "Active", True):
                continue

            sublist = []
            PathLog.debug("obj: {}".format(obj.Name))

            for f in wcslist:
                fobj = _TempObject()
                c1 = Path.Command(f)
                fobj.Path = Path.Path([c1])
                if not firstFixture:
                    c2 = Path.Command(
                        "G0 Z"
                        + str(
                            job.Stock.Shape.BoundBox.ZMax
                            + job.SetupSheet.ClearanceHeightOffset.Value
                        )
                    )
                    fobj.Path.addCommands(c2)
                fobj.InList.append(job)
                sublist.append(fobj)
                firstFixture = False
                tc = PathUtil.toolControllerForOp(obj)
                if tc is not None:
                    if job.SplitOutput or (tc.ToolNumber != currTool):
                        sublist.append(tc)
                        currTool = tc.ToolNumber
                sublist.append(obj)
            postlist.append((obj.Label, sublist))

    if job.SplitOutput:
        PathLog.track()
        return postlist
    else:
        PathLog.track()
        finalpostlist = [
            ("allitems", [item for slist in postlist for item in slist[1]])
        ]
        return finalpostlist


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
        PathLog.track(extraargs)
        # check if the user has a project and has set the default post and
        # output filename
        # extraargs can be passed in at this time
        PathLog.track(partname, sequence)
        PathLog.track(objs)

        # partname = objs[0]
        # slist = objs[1]
        PathLog.track(objs, partname)

        postArgs = PathPreferences.defaultPostProcessorArgs()
        if hasattr(job, "PostProcessorArgs") and job.PostProcessorArgs:
            postArgs = job.PostProcessorArgs
        elif hasattr(job, "PostProcessor") and job.PostProcessor:
            postArgs = ""

        if extraargs is not None:
            postArgs += " {}".format(extraargs)

        PathLog.track(postArgs)

        postname = self.resolvePostProcessor(job)
        # filename = "-"
        filename = resolveFileName(job, partname, sequence)
        # if postname and needFilename:
        #     filename = resolveFileName(job)

        if postname and filename:
            print("post: %s(%s, %s)" % (postname, filename, postArgs))
            processor = PostProcessor.load(postname)
            gcode = processor.export(objs, filename, postArgs)
            return (False, gcode, filename)
        else:
            return (True, "", filename)

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
            PathLog.debug("Possible post objects: {}".format(targetlist))
            if len(targetlist) > 1:
                jobname, result = QtGui.QInputDialog.getItem(
                    None, translate("Path", "Choose a Path Job"), None, targetlist
                )

                if result is False:
                    return
            else:
                jobname = targetlist[0]
            job = FreeCAD.ActiveDocument.getObject(jobname)

        PathLog.debug("about to postprocess job: {}".format(job.Name))

        postlist = buildPostList(job)
        # filename = resolveFileName(job, "allitems", 0)

        filenames = []

        success = True
        finalgcode = ""
        for idx, section in enumerate(postlist):
            partname = section[0]
            sublist = section[1]

            result, gcode, name = self.exportObjectsWith(sublist, partname, job, idx)
            filenames.append(name)
            PathLog.track(result, gcode, name)

            if result is None:
                success = False
            else:
                finalgcode += gcode

        # if job.SplitOutput:
        #     for idx, sublist in enumerate(postlist):  # name, slist in postlist:
        #         result = self.exportObjectsWith(sublist[1], sublist[0], job, idx)

        #         if result is None:
        #             success = False
        #         else:
        #             gcode += result

        # else:
        #     finalpostlist = [item for (_, slist) in postlist for item in slist]
        #     gcode = self.exportObjectsWith(finalpostlist, "allitems", job, 1)
        #     success = gcode is not None

        PathLog.track(success)
        if success:
            if hasattr(job, "LastPostProcessDate"):
                job.LastPostProcessDate = str(datetime.now())
            if hasattr(job, "LastPostProcessOutput"):
                job.LastPostProcessOutput = " \n".join(filenames)
                PathLog.track(job.LastPostProcessOutput)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Post", CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
