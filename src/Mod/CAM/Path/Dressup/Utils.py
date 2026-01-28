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


def selection():
    """selection() ... return object if selected one operation or dressup."""
    if FreeCAD.ActiveDocument and FreeCAD.GuiUp:
        import FreeCADGui

        selection = FreeCADGui.Selection.getSelection()
        if (
            len(selection) == 1
            and selection[0].isDerivedFrom("Path::Feature")
            and isOp(selection[0])
        ):
            return selection[0]
    return None


def isOp(obj):
    """isOp(obj) ... return true if obj is operation or dressup."""
    if not getattr(obj, "Proxy", None):
        return False
    proxy = obj.Proxy.__module__
    if "Path.Op" not in proxy and "Path.Dressup" not in proxy:
        return False
    return True


def hasEntryMethod(path):
    """hasEntryDressup(path) ... returns true if the given object already has an entry method attached."""
    if "RampEntry" in path.Name or "LeadInOut" in path.Name:
        return True
    if "Dressup" in path.Name and hasattr(path, "Base"):
        return hasEntryMethod(path.Base)
    return False


def baseOp(path):
    """baseOp(path) ... return the base operation underlying the given path"""
    if "Dressup" in path.Name:
        return baseOp(path.Base)
    return path


def toolController(path):
    """toolController(path) ... return the tool controller from the base op."""
    return baseOp(path).ToolController
