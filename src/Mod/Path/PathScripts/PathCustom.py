# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import PathScripts.PathOp as PathOp

from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "Path Custom Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Custom object and FreeCAD command"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ObjectCustom(PathOp.ObjectOp):
    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureCoolant

    def initOperation(self, obj):
        obj.addProperty(
            "App::PropertyStringList",
            "Gcode",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The gcode to be inserted"),
        )

        obj.Proxy = self

    def opExecute(self, obj):
        self.commandlist.append(Path.Command("(Begin Custom)"))
        if obj.Gcode:
            for l in obj.Gcode:
                newcommand = Path.Command(str(l))
                self.commandlist.append(newcommand)

        self.commandlist.append(Path.Command("(End Custom)"))


def SetupProperties():
    setup = []
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Custom operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectCustom(obj, name, parentJob)
    return obj
