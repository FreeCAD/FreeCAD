# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocket as PathPocket
import PathScripts.PathPocketBaseGui as PathPocketBaseGui
import PathScripts.PathLog as PathLog

from PySide.QtCore import QT_TRANSLATE_NOOP

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


__title__ = "Path Pocket Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Pocket operation page controller and command implementation."


class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    """Page controller class for Pocket operation"""

    def pocketFeatures(self):
        """pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)"""
        return PathPocketBaseGui.FeaturePocket


Command = PathOpGui.SetupOperation(
    "Pocket3D",
    PathPocket.Create,
    TaskPanelOpPage,
    "Path_3DPocket",
    QT_TRANSLATE_NOOP("Path_Pocket3D", "3D Pocket"),
    QT_TRANSLATE_NOOP(
        "Path_Pocket3D", "Creates a Path 3D Pocket object from a face or faces"
    ),
    PathPocket.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathPocketGui... done\n")
