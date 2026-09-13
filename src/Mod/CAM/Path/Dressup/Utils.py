# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 sliptonic <shopinthewoods@gmail.com>
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

import FreeCAD
import Path

translate = FreeCAD.Qt.translate


def selection(verbose=False):
    """selection() ... return object if selected one operation or dressup.
    Allow to send error messages to Report view if verbose=True"""
    if FreeCAD.ActiveDocument and FreeCAD.GuiUp:
        import FreeCADGui

        selected = FreeCADGui.Selection.getSelection()
        if len(selected) != 1:
            if verbose:
                Path.Log.warning(translate("CAM_Dressup", "Select one toolpath object\n"))
            return None
        if not selected[0].isDerivedFrom("Path::Feature"):
            if verbose:
                Path.Log.warning(
                    translate("CAM_Dressup", "The selected object is not a toolpath\n")
                )
            return None
        if not isOp(selected[0]):
            if verbose:
                Path.Log.warning(
                    translate("CAM_Dressup", "The selected object is not an operation or dressup\n")
                )
            return None
        return selected[0]

    return None


def isOp(obj):
    """isOp(obj) ... return true if obj is operation or dressup."""
    if not getattr(obj, "Proxy", None):
        return False
    proxy = obj.Proxy.__module__
    return "Path.Op" in proxy or "Path.Dressup" in proxy


def baseOp(obj):
    """baseOp(obj) ... return the base operation underlying the given path object"""
    if (
        getattr(obj, "Proxy", None)
        and obj.Proxy.__module__.startswith("Path.Dressup")
        and getattr(obj, "Base", None)
    ):
        return baseOp(obj.Base)
    return obj


def toolController(path, default=None):
    """toolController(path) ... return the tool controller from the base op."""
    return getattr(baseOp(path), "ToolController", default)
