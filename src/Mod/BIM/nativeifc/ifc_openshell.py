# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Utilities to help people verify and update their version of ifcopenshell"""

import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class IFC_UpdateIOS:
    """Shows a dialog to update IfcOpenShell"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP("IFC_UpdateIOS", "Shows a dialog to update IfcOpenShell")
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_UpdateIOS", "IfcOpenShell update"),
            "ToolTip": tt,
        }

    def Activated(self):
        """Shows the updater UI"""

        version = self.get_current_version()
        avail = self.get_avail_version()
        if avail:
            if version:
                comp = self.compare_versions(avail, version)
                if comp > 0:
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
        note = translate("BIM", "The update is installed in your FreeCAD's user directory and won't affect the rest of your system.")
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
                    text = translate("BIM", "IfcOpenShell update successfully installed.")
                    buttons = QtGui.QMessageBox.Ok
                    reply = QtGui.QMessageBox.information(None, title, text, buttons)


    def install(self):
        """Installs the given version"""

        import addonmanager_utilities as utils
        from PySide import QtCore, QtGui
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        vendor_path = utils.get_pip_target_directory()
        args = ["install", "--disable-pip-version-check", "--target", vendor_path, "ifcopenshell"]
        result = self.run_pip(args)
        QtGui.QApplication.restoreOverrideCursor()
        return result


    def run_pip(self, args):
        """Runs a pip command"""

        import addonmanager_utilities as utils
        import freecad.utils
        cmd = [freecad.utils.get_python_exe(), "-m", "pip"]
        cmd.extend(args)
        result = None
        try:
            result = utils.run_interruptable_subprocess(cmd)
        except:
            text = translate("BIM","Unable to run pip. Please ensure pip is installed on your system.")
            FreeCAD.Console.PrintError(text + "\n")
        return result


    def get_current_version(self):
        """Retrieves the current ifcopenshell version"""

        import addonmanager_utilities as utils
        try:
            import ifcopenshell
            version = ifcopenshell.version
        except:
            version = ""
        if version.startswith("v"):
            # this is a pip version
            vendor_path = utils.get_pip_target_directory()
            result = self.run_pip(["list", "--path", vendor_path])
            if result:
                result = result.stdout.split()
                if "ifcopenshell" in result:
                    version = result[result.index("ifcopenshell")+1]
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


    def compare_versions(self, v1, v2):
        """Compare two version strings in the form '0.7.0' or v0.7.0"""

        # code from https://www.geeksforgeeks.org/compare-two-version-numbers

        arr1 = v1.replace("v","").split(".")
        arr2 = v2.replace("v","").split(".")
        n = len(arr1)
        m = len(arr2)
        arr1 = [int(i) for i in arr1]
        arr2 = [int(i) for i in arr2]
        if n > m:
          for i in range(m, n):
             arr2.append(0)
        elif m > n:
          for i in range(n, m):
             arr1.append(0)
        for i in range(len(arr1)):
          if arr1[i] > arr2[i]:
             return 1
          elif arr2[i] > arr1[i]:
             return -1
        return 0


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
