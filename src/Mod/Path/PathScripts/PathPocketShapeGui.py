# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathPocketBaseGui as PathPocketBaseGui

from PySide import QtCore

__title__ = "Path Pocket Shape Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Pocket Shape operation page controller and command implementation."

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    '''Page controller class for Pocket operation'''

    def pocketFeatures(self):
        '''pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)'''
        return PathPocketBaseGui.FeaturePocket

Command = PathOpGui.SetupOperation('Pocket Shape',
        PathPocketShape.Create,
        TaskPanelOpPage,
        'Path-Pocket',
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pocket Shape"),
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Creates a Path Pocket object from a face or faces"))

FreeCAD.Console.PrintLog("Loading PathPocketShapeGui... done\n")
