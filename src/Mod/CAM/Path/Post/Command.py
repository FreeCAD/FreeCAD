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

"""Post Process command that will make use of the Output File and Post
Processor entries in PathJob """


import FreeCAD
import FreeCADGui
import Path
import Path.Base.Util as PathUtil
import Path.Main.Job as PathJob
from PathScripts import PathUtils
import os
import re

from Path.Post.Processor import PostProcessor
from PySide import QtCore, QtGui
from datetime import datetime
from PySide.QtCore import QT_TRANSLATE_NOOP

LOG_MODULE = Path.Log.thisModule()

debugmodule = False
if debugmodule:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class _TempObject:
    Path = None
    Name = "Fixture"
    InList = []
    Label = "Fixture"


def processFileNameSubstitutions(
    job,
    subpartname,
    sequencenumber,
    outputpath,
    filename,
    ext,
):
    """Process any substitutions in the outputpath or filename."""

    # The following section allows substitution within the path part
    Path.Log.track(f"path before substitution: {outputpath}")

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

    Path.Log.track(f"path after substitution: {outputpath}")

    # The following section allows substitution within the filename part
    Path.Log.track(f"filename before substitution: {filename}")

    # Use the file label
    if "%d" in filename:
        d = FreeCAD.ActiveDocument.Label
        filename = filename.replace("%d", d)

    # Use the name of the active job object
    if "%j" in filename:
        j = job.Label
        filename = filename.replace("%j", j)

    # Use the sequence number if explicitly called
    if "%S" in filename:
        j = job.Label
        filename = filename.replace("%S", str(sequencenumber))

    # This section handles unique names for splitting output
    if job.SplitOutput:
        Path.Log.track()
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

    Path.Log.track(f"filename after substitution: {filename}")

    if not ext:
        ext = ".nc"
    Path.Log.track(f"file extension: {ext}")

    fullPath = f"{outputpath}{os.path.sep}{filename}{ext}"

    Path.Log.track(f"full filepath: {fullPath}")
    return fullPath


def resolveFileName(job, subpartname, sequencenumber):
    """Generate the file name to use as output."""

    Path.Log.track(subpartname, sequencenumber)

    validPathSubstitutions = ["D", "d", "M", "j"]
    validFilenameSubstitutions = ["j", "d", "T", "t", "W", "O", "S"]

    # Look for preference default
    outputpath, filename = os.path.split(Path.Preferences.defaultOutputFile())
    filename, ext = os.path.splitext(filename)

    # Override with document default if it exists
    if job.PostProcessorOutputFile:
        candidateOutputPath, candidateFilename = os.path.split(
            job.PostProcessorOutputFile
        )

        if candidateOutputPath:
            outputpath = candidateOutputPath

        if candidateFilename:
            filename, ext = os.path.splitext(candidateFilename)

    # Strip any invalid substitutions from the outputpath
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
    Path.Log.track(f"path: {outputpath} name: {filename} ext: {ext}")

    fullPath = processFileNameSubstitutions(
        job,
        subpartname,
        sequencenumber,
        outputpath,
        filename,
        ext,
    )

    # This section determines whether user interaction is necessary
    policy = Path.Preferences.defaultOutputPolicy()

    openDialog = policy == "Open File Dialog"
    # if os.path.isdir(filename) or not os.path.isdir(os.path.dirname(filename)):
    #     # Either the entire filename resolves into a directory or the parent
    #     # directory doesn't exist.
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
            while os.path.isfile(f"{fn}{n:03d}{ext}"):
                n = n + 1
            fullPath = f"{fn}{n:03d}{ext}"

    if openDialog:
        requestedfile = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(), "Output File", fullPath
        )
        if requestedfile[0]:
            fullPath = requestedfile[0]
        else:
            fullPath = None

    if fullPath:
        # remove any unused substitution strings:
        for s in validPathSubstitutions + validFilenameSubstitutions:
            fullPath = fullPath.replace(f"%{s}", "")
        fullPath = os.path.normpath(fullPath)
        Path.Log.track(fullPath)

    return fullPath


def fixtureSetup(order, fixture, job):
    """Convert a Fixure setting to _TempObject instance with a G0 move to a
    safe height every time the fixture coordinate system change.  Skip
    the move for first fixture, to avoid moving before tool and tool
    height compensation is enabled.

    """

    fobj = _TempObject()
    c1 = Path.Command(fixture)
    fobj.Path = Path.Path([c1])
    # Avoid any tool move after G49 in preamble and before tool change
    # and G43 in case tool height compensation is in use, to avoid
    # dangerous move without tool compesation.
    if order != 0:
        c2 = Path.Command(
            "G0 Z"
            + str(
                job.Stock.Shape.BoundBox.ZMax
                + job.SetupSheet.ClearanceHeightOffset.Value
            )
        )
        fobj.Path.addCommands(c2)
    fobj.InList.append(job)
    return fobj


def buildPostList(job):
    """Takes the job and determines the specific objects and order to
    postprocess  Returns a list of objects which can be passed to
    exportObjectsWith() for final posting."""
    wcslist = job.Fixtures
    orderby = job.OrderOutputBy

    postlist = []

    if orderby == "Fixture":
        Path.Log.debug("Ordering by Fixture")
        # Order by fixture means all operations and tool changes will be
        # completed in one fixture before moving to the next.

        currTool = None
        for index, f in enumerate(wcslist):
            # create an object to serve as the fixture path
            sublist = [fixtureSetup(index, f, job)]

            # Now generate the gcode
            for obj in job.Operations.Group:
                tc = PathUtil.toolControllerForOp(obj)
                if tc is not None and PathUtil.opProperty(obj, "Active"):
                    if tc.ToolNumber != currTool:
                        sublist.append(tc)
                        Path.Log.debug(f"Appending TC: {tc.Name}")
                        currTool = tc.ToolNumber
                sublist.append(obj)
            postlist.append((f, sublist))

    elif orderby == "Tool":
        Path.Log.debug("Ordering by Tool")
        # Order by tool means tool changes are minimized.
        # all operations with the current tool are processed in the current
        # fixture before moving to the next fixture.

        toolstring = "None"
        currTool = None

        # Build the fixture list
        fixturelist = []
        for index, f in enumerate(wcslist):
            # create an object to serve as the fixture path
            fixturelist.append(fixtureSetup(index, f, job))

        # Now generate the gcode
        curlist = []  # list of ops for tool, will repeat for each fixture
        sublist = []  # list of ops for output splitting

        Path.Log.track(job.PostProcessorOutputFile)
        for idx, obj in enumerate(job.Operations.Group):
            Path.Log.track(obj.Label)

            # check if the operation is active
            if not getattr(obj, "Active", True):
                Path.Log.track()
                continue

            # Determine the proper string for the Op's TC
            tc = PathUtil.toolControllerForOp(obj)
            if tc is None:
                tcstring = "None"
            elif "%T" in job.PostProcessorOutputFile:
                tcstring = f"{tc.ToolNumber}"
            else:
                tcstring = re.sub(r"[^\w\d-]", "_", tc.Label)
            Path.Log.track(toolstring)

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
        Path.Log.debug("Ordering by Operation")
        # Order by operation means ops are done in each fixture in
        # sequence.
        currTool = None

        # Now generate the gcode
        for obj in job.Operations.Group:

            # check if the operation is active
            if not getattr(obj, "Active", True):
                continue

            sublist = []
            Path.Log.debug(f"obj: {obj.Name}")

            for index, f in enumerate(wcslist):
                sublist.append(fixtureSetup(index, f, job))
                tc = PathUtil.toolControllerForOp(obj)
                if tc is not None:
                    if job.SplitOutput or (tc.ToolNumber != currTool):
                        sublist.append(tc)
                        currTool = tc.ToolNumber
                sublist.append(obj)
            postlist.append((obj.Label, sublist))

    if job.SplitOutput:
        Path.Log.track()
        return postlist

    Path.Log.track()
    finalpostlist = [
        ("allitems", [item for slist in postlist for item in slist[1]])
    ]
    return finalpostlist


class DlgSelectPostProcessor:
    """Provide user with list of available and active post processor
    choices."""
    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgSelectPostProcessor.ui")
        firstItem = None
        for post in Path.Preferences.allEnabledPostProcessors():
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
        if item.text() in self.tooltips:
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
            post = Path.Preferences.defaultPostProcessor()
            if job.PostProcessor:
                post = job.PostProcessor
            if post and PostProcessor.exists(post):
                return post
        dlg = DlgSelectPostProcessor()
        return dlg.exec_()

    def GetResources(self):
        return {
            "Pixmap": "CAM_Post",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Post", "Post Process"),
            "Accel": "P, P",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Post", "Post Process the selected Job"),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            if FreeCADGui.Selection.getCompleteSelection():
                for o in FreeCAD.ActiveDocument.Objects:
                    if o.Name[:3] == "Job":
                        return True

        return False

    def exportObjectsWith(self, objs, partname, job, sequence, extraargs=None):
        Path.Log.track(extraargs)
        # check if the user has a project and has set the default post and
        # output filename
        # extraargs can be passed in at this time
        Path.Log.track(partname, sequence)
        Path.Log.track(objs)

        # partname = objs[0]
        # slist = objs[1]
        Path.Log.track(objs, partname)

        postArgs = Path.Preferences.defaultPostProcessorArgs()
        if hasattr(job, "PostProcessorArgs") and job.PostProcessorArgs:
            postArgs = job.PostProcessorArgs
        elif hasattr(job, "PostProcessor") and job.PostProcessor:
            postArgs = ""

        if extraargs is not None:
            postArgs += f" {extraargs}"

        Path.Log.track(postArgs)

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
        Path.Log.track()
        FreeCAD.ActiveDocument.openTransaction("Post Process the Selected path(s)")
        FreeCADGui.addModule("Path.Post.Command")

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
            Path.Log.debug(f"Possible post objects: {targetlist}")
            if len(targetlist) > 1:
                jobname, result = QtGui.QInputDialog.getItem(
                    None, translate("Path", "Choose a Path Job"), None, targetlist
                )

                if result is False:
                    return
            else:
                jobname = targetlist[0]
            job = FreeCAD.ActiveDocument.getObject(jobname)

        Path.Log.debug(f"about to postprocess job: {job.Name}")

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
            Path.Log.track(result, gcode, name)

            if name is None:
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

        Path.Log.track(success)
        if success:
            if hasattr(job, "LastPostProcessDate"):
                job.LastPostProcessDate = str(datetime.now())
            if hasattr(job, "LastPostProcessOutput"):
                job.LastPostProcessOutput = " \n".join(filenames)
                Path.Log.track(job.LastPostProcessOutput)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Post", CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
