# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2016 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
This file has the GUI command for checking and catching common errors in FreeCAD
CAM projects.
"""

from Path.Main.Sanity import Sanity
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide.QtGui import QFileDialog
from Path import Preferences
from Path.Post.Utils import apply_path_substitutions
from PathScripts.PathUtils import findParentJob
import FreeCAD
import FreeCADGui
import Path
import Path.Log
import os
import webbrowser

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CommandCAMSanity:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Sanity",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Sanity", "Sanity Check"),
            "Accel": "P, S",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Sanity", "Checks the CAM job for common errors"),
        }

    def IsActive(self):
        if not (selection := FreeCADGui.Selection.getSelection()):
            return False
        return bool(findParentJob(selection[0]))

    def Activated(self):
        if not (selection := FreeCADGui.Selection.getSelection()):
            return False
        if not (job := findParentJob(selection[0])):
            return False

        # Ask the user for a filename to save the report to

        pref_report_file = Preferences.defaultSanityReportOutputFile()
        default_filename = None

        # Use the preference as the template for the filename, or default next to the document
        if pref_report_file.strip():
            # If the preference is an absolute or relative path, expand substitutions
            template_path = pref_report_file
            expanded = apply_path_substitutions(template_path, obj)
            # Ensure .html extension
            if not expanded.lower().endswith(".html"):
                expanded += ".html"
            default_filename = expanded
        else:
            # No preference set: default next to the document
            doc_path = FreeCAD.ActiveDocument.getFileName()
            if doc_path:
                defaultDir = os.path.dirname(doc_path)
                base = os.path.splitext(os.path.basename(doc_path))[0]
                default_filename = os.path.join(defaultDir, f"{base}.html")
            else:
                defaultDir = os.path.expanduser("~")
                default_filename = os.path.join(defaultDir, "setupreport.html")

        file_location = QFileDialog.getSaveFileName(
            None,
            translate("Path", "Save Sanity Check Report"),
            default_filename,
            "HTML files (*.html)",
        )[0]

        if not file_location:
            return

        sanity_checker = Sanity.CAMSanity(job, file_location)
        html = sanity_checker.get_output_report()

        if html is None:
            Path.Log.error("Sanity check failed. No report generated.")
            return

        with open(file_location, "w") as fp:
            fp.write(html)

        FreeCAD.Console.PrintMessage(f"Sanity check report written to: {file_location}\n")
        webbrowser.open_new_tab(file_location)


class CommandCAMQuickValidate:
    """Quick validation command: runs squawk checks without generating images or HTML."""

    def GetResources(self):
        return {
            "Pixmap": "CAM_SanityQuick",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Sanity", "Quick Validate"),
            "Accel": "P, V",
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Sanity",
                "Validates the CAM job for common issues without generating a full report",
            ),
        }

    def IsActive(self):
        if not (selection := FreeCADGui.Selection.getSelection()):
            return False
        return bool(findParentJob(selection[0]))

    def Activated(self):
        if not (selection := FreeCADGui.Selection.getSelection()):
            return False
        if not (job := findParentJob(selection[0])):
            return False

        try:
            all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(job)
        except Exception as e:
            Path.Log.error(f"CAM_QuickValidate: Validation failed: {e}")
            FreeCAD.Console.PrintError(f"Quick Validate failed: {e}\n")
            return

        if not all_squawks:
            FreeCAD.Console.PrintMessage(
                translate("CAM_Sanity", "Quick Validate: No issues found.\n")
            )
            return

        FreeCAD.Console.PrintMessage(translate("CAM_Sanity", "=== Quick Validation Results ===\n"))
        for squawk in all_squawks:
            msg = f"[{squawk['squawkType']}] {squawk['Note']}\n"
            if squawk["squawkType"] in ("WARNING", "CAUTION"):
                FreeCAD.Console.PrintWarning(msg)
            else:
                FreeCAD.Console.PrintMessage(msg)
        FreeCAD.Console.PrintMessage(
            translate(
                "CAM_Sanity",
                f"=== {len(all_squawks)} issue(s) found, {len(critical_squawks)} critical ===\n",
            )
        )


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Sanity", CommandCAMSanity())
    FreeCADGui.addCommand("CAM_QuickValidate", CommandCAMQuickValidate())
