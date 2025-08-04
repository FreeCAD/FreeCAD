# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Utilities to help people verify and update their version of ifcopenshell"""

from packaging.version import Version

import FreeCAD
import FreeCADGui
from addonmanager_utilities import create_pip_call

translate = FreeCAD.Qt.translate
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class IFC_UpdateIOS:
    """Shows a dialog to update IfcOpenShell"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP("IFC_UpdateIOS", "Shows a dialog to update IfcOpenShell")
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_UpdateIOS", "IfcOpenShell Update"),
            "ToolTip": tt,
        }

    def Activated(self):
        """Shows the updater UI"""

        version = self.get_current_version()
        avail = self.get_avail_version()
        if avail:
            if version:
                if Version(version) < Version(avail):
                    self.show_dialog("update", avail)
                else:
                    self.show_dialog("uptodate")
            else:
                self.show_dialog("install", avail)
        else:
            if version:
                self.show_dialog("uptodate")
            else:
                self.show_dialog("failed")


    def show_dialog(self, mode, version=None):
        """Shows a dialog to the user"""

        from PySide import QtGui
        title = translate("BIM", "IfcOpenShell update")
        note = translate("BIM", "The update is installed in your FreeCAD's user directory and will not affect the rest of your system.")
        if mode == "update":
            text = translate("BIM", "An update to your installed IfcOpenShell version is available")
            text += ": " + version + ". "
            text += translate("BIM", "Would you like to install that update?")
            text += " " + note
            buttons = QtGui.QMessageBox.Cancel | QtGui.QMessageBox.Ok
        elif mode == "uptodate":
            text = translate("BIM", "Your version of IfcOpenShell is already up to date")
            buttons = QtGui.QMessageBox.Ok
        elif mode == "install":
            text = translate("BIM", "No existing IfcOpenShell installation found on this system.")
            text += " "
            text += translate("BIM", "Would you like to install the most recent version?")
            text += " (" + version + ") " + note
            buttons = QtGui.QMessageBox.Cancel | QtGui.QMessageBox.Ok
        elif mode == "failed":
            text = translate("BIM", "IfcOpenShell is not installed, and FreeCAD failed to find a suitable version to install. You can still install IfcOpenShell manually, visit https://wiki.freecad.org/IfcOpenShell for further instructions.")
            buttons = QtGui.QMessageBox.Ok
        reply = QtGui.QMessageBox.information(None, title, text, buttons)
        if reply == QtGui.QMessageBox.Ok:
            if mode in ["update", "install"]:
                result = self.install()
                if result:
                    FreeCAD.Console.PrintLog(f"{result.stdout}\n")
                    text = translate("BIM", "IfcOpenShell update successfully installed.")
                    buttons = QtGui.QMessageBox.Ok
                    reply = QtGui.QMessageBox.information(None, title, text, buttons)


    def install(self):
        """Installs the given version"""

        import addonmanager_utilities as utils
        from PySide import QtCore, QtGui
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        vendor_path = utils.get_pip_target_directory()
        args = ["install", "--upgrade", "--disable-pip-version-check", "--target", vendor_path, "ifcopenshell"]
        result = self.run_pip(args)
        QtGui.QApplication.restoreOverrideCursor()
        return result


    def run_pip(self, args):
        """Runs a pip command"""

        import addonmanager_utilities as utils
        import freecad.utils
        from subprocess import CalledProcessError

        cmd = create_pip_call(args)
        result = None
        try:
            result = utils.run_interruptable_subprocess(cmd)
        except CalledProcessError as pe:
            FreeCAD.Console.PrintError(pe.stderr)
        except Exception as e:
            text = translate("BIM","Unable to run pip. Ensure pip is installed on your system.")
            FreeCAD.Console.PrintError(f"{text} {str(e)}\n")
        return result


    def get_current_version(self):
        """Retrieves the current ifcopenshell version"""

        import addonmanager_utilities as utils
        from packaging.version import InvalidVersion

        try:
            import ifcopenshell
            version = ifcopenshell.version
            try:
                Version(version)
            except InvalidVersion:
                FreeCAD.Console.PrintWarning(f"Invalid IfcOpenShell version: {version}\n")
                version = ""
        except:
            version = ""

        return version


    def get_avail_version(self):
        """Retrieves an available ifcopenshell version"""

        result = self.run_pip(["index", "versions", "ifcopenshell"])
        if result:
            if result.stdout and "versions" in result.stdout:
                result = result.stdout.split()
                result = result[result.index("versions:")+1:]
                result = [r.strip(",") for r in result]
                return result[0]  # we return the biggest
        return None


FreeCADGui.addCommand("IFC_UpdateIOS", IFC_UpdateIOS())



# >>> utils.get_pip_target_directory()
# '/home/yorik/.local/share/FreeCAD/Mod/../AdditionalPythonPackages/py311'
# >>> import freecad.utils
# >>> freecad.utils
# <module 'freecad.utils' from '/home/yorik/Apps/FreeCAD/Ext/freecad/utils.py'>
# >>> freecad.utils.get_python_exe
# <function get_python_exe at 0x7efdebf5ede0>
# >>> freecad.utils.get_python_exe()
# '/usr/bin/python3'
# ...
# >>> run_pip(["index", "versions", "ifcopenshell"])
# CompletedProcess(args=['/usr/bin/python3', '-m', 'pip', 'index', 'versions', 'ifcopenshell'], returncode=0, stdout='ifcopenshell (0.7.0.240423)\nAvailable versions: 0.7.0.240423, 0.7.0.240418, 0.7.0.240406\n', stderr='WARNING: pip index is currently an experimental command. It may be removed/changed in a future release without prior warning.\n')
# pip install --disable-pip-version-check --target vendor_path ifcopenshell
