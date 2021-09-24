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
# *   Major modifications: 2020 Russell Johnson <russ4262@gmail.com>        *

import FreeCAD
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathProfile as PathProfile
import PathScripts.PathProfileGui as PathProfileGui
from PySide import QtCore


__title__ = "Path Contour Operation UI (depreciated)"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Contour operation page controller and command implementation (depreciated)."


class TaskPanelOpPage(PathProfileGui.TaskPanelOpPage):
    '''Psuedo page controller class for Profile operation,
    allowing for backward compatibility with pre-existing "Contour" operations.'''
    pass
# Eclass


Command = PathOpGui.SetupOperation('Profile',
        PathProfile.Create,
        TaskPanelOpPage,
        'Path_Contour',
        QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Profile"),
        QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Profile entire model, selected face(s) or selected edge(s)"),
        PathProfile.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathProfileContourGui... done\n")
