# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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


def baseOp(path):
    """baseOp(path) ... return the base operation underlying the given path.

    A dressup is known by its shape, not its name: its Base is the one path
    object it dresses. An operation's Base, if it has one, is a list of
    geometry."""
    base = getattr(path, "Base", None)
    if base is not None and not isinstance(base, (list, tuple)) and hasattr(base, "Path"):
        return baseOp(base)
    return path


def placeWithBase(obj):
    """placeWithBase(obj) ... carry the base operation's frame.

    A dressup generates in the frame its base operation generates in: it
    reads the base's stored path, which is in the base's work plane's frame,
    and stores its own path in that frame. Its Placement positions it, as an
    operation's does, so the simulators, Inspect and the posts read a dressup
    exactly as they read an operation. The frame is never stored on the
    dressup: it is read through the base on every execute and written here."""
    import Path.Base.Util as PathUtil

    placement = getattr(obj, "Placement", None)
    if placement is None:
        return  # a test double without one
    frame = PathUtil.workplaneForOp(obj)
    if not placement.isSame(frame, 1e-9):
        obj.Placement = frame
    if hasattr(obj, "setEditorMode"):
        obj.setEditorMode("Placement", 1)  # derived from the base operation


def toolController(path, default=None):
    """toolController(path) ... return the tool controller from the base op."""
    return getattr(baseOp(path), "ToolController", default)
