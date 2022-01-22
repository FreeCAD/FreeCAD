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
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathPocketBaseGui as PathPocketBaseGui
import PathScripts.PathFeatureExtensionsGui as PathFeatureExtensionsGui
from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Path Pocket Shape Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Pocket Shape operation page controller and command implementation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    """Page controller class for Pocket operation"""

    def pocketFeatures(self):
        """pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)"""
        return PathPocketBaseGui.FeaturePocket | PathPocketBaseGui.FeatureOutline

    def taskPanelBaseLocationPage(self, obj, features):
        if not hasattr(self, "extensionsPanel"):
            self.extensionsPanel = PathFeatureExtensionsGui.TaskPanelExtensionPage(
                obj, features
            )  # pylint: disable=attribute-defined-outside-init
        return self.extensionsPanel


Command = PathOpGui.SetupOperation(
    "Pocket Shape",
    PathPocketShape.Create,
    TaskPanelOpPage,
    "Path_Pocket",
    QT_TRANSLATE_NOOP("Path_Pocket_Shape", "Pocket Shape"),
    QT_TRANSLATE_NOOP(
        "Path_Pocket_Shape", "Creates a Path Pocket object from a face or faces"
    ),
    PathPocketShape.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathPocketShapeGui... done\n")
