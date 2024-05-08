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
from PathScripts import PathUtils
from Path.Post.Utils import FilenameGenerator
import os
from Path.Post.Processor import PostProcessor, PostProcessorFactory
from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

LOG_MODULE = Path.Log.thisModule()

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


def _resolve_post_processor_name(job):
    Path.Log.debug("_resolve_post_processor_name()")
    if job.PostProcessor:
        valid_name = job.PostProcessor
    elif Path.Preferences.defaultPostProcessor():
        valid_name = Path.Preferences.defaultPostProcessor()
    elif FreeCAD.GuiUp:
        valid_name = (
            DlgSelectPostProcessor().exec_()
        )  # Ensure DlgSelectPostProcessor is defined
    else:
        valid_name = None

    if valid_name and PostProcessor.exists(valid_name):
        return valid_name
    else:
        raise ValueError(f"Post processor not identified.")


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
    def GetResources(self):
        return {
            "Pixmap": "CAM_Post",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Post", "Post Process"),
            "Accel": "P, P",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Post", "Post Process the selected Job"),
        }

    def IsActive(self):
        selected = FreeCADGui.Selection.getSelectionEx()
        if len(selected) != 1:
            return False

        selected_object = selected[0].Object
        self.candidate = PathUtils.findParentJob(selected_object)

        return self.candidate is not None

    def _write_file(self, filename, gcode, policy):
        if policy == "Open File Dialog":
            dlg = QtGui.QFileDialog()
            dlg.setFileMode(QtGui.QFileDialog.FileMode.AnyFile)
            dlg.setAcceptMode(QtGui.QFileDialog.AcceptMode.AcceptSave)
            dlg.setDirectory(os.path.dirname(filename))
            dlg.selectFile(os.path.basename(filename))
            if dlg.exec_():
                filename = dlg.selectedFiles()[0]
                Path.Log.debug(filename)
                with open(filename, "w") as f:
                    f.write(gcode)

        elif policy == "Append Unique ID on conflict":
            while os.path.isfile(filename):
                base, ext = os.path.splitext(filename)
                filename = f"{base}-1{ext}"
            with open(filename, "w") as f:
                f.write(gcode)

        elif policy == "Open File Dialog on conflict":
            if os.path.isfile(filename):
                dlg = QtGui.QFileDialog()
                dlg.setFileMode(QtGui.QFileDialog.FileMode.AnyFile)
                dlg.setAcceptMode(QtGui.QFileDialog.AcceptSave)
                dlg.setDirectory(os.path.dirname(filename))
                dlg.selectFile(os.path.basename(filename))
                if dlg.exec_():
                    filename = dlg.selectedFiles()[0]
                    Path.Log.debug(filename)
                    with open(filename, "w") as f:
                        f.write(gcode)
            else:
                with open(filename, "w") as f:
                    f.write(gcode)
        else:  # Overwrite
            with open(filename, "w") as f:
                f.write(gcode)
        FreeCAD.Console.PrintMessage(f"File written to {filename}\n")

    def Activated(self):
        """
        Handles the activation of post processing, initiating the process based
        on user selection and document context.
        """
        Path.Log.debug(self.candidate.Name)
        FreeCAD.ActiveDocument.openTransaction("Post Process the Selected Job")

        postprocessor_name = _resolve_post_processor_name(self.candidate)
        Path.Log.debug(f"Post Processor: {postprocessor_name}")

        if not postprocessor_name:
            FreeCAD.ActiveDocument.abortTransaction()
            return

        # get a postprocessor
        postprocessor = PostProcessorFactory.get_post_processor(
            self.candidate, postprocessor_name
        )

        post_data = postprocessor.export()
        if not post_data:
            FreeCAD.ActiveDocument.abortTransaction()
            return

        policy = Path.Preferences.defaultOutputPolicy()
        generator = FilenameGenerator(job=self.candidate)
        generated_filename = generator.generate_filenames()

        for item in post_data:
            subpart, gcode = item

            # get a name for the file
            subpart = "" if subpart == "allitems" else subpart
            Path.Log.debug(subpart)
            generator.set_subpartname(subpart)
            fname = next(generated_filename)

            # write the results to the file
            self._write_file(fname, gcode, policy)

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Post", CommandPathPost())

FreeCAD.Console.PrintLog("Loading PathPost... done\n")
