# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Daniel Wood <s.d.wood.82@gmail.com>                *
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
import Path.TurningOp.Gui.TurnBaseGui as TurnBaseGui
import Path.TurningOp.TurnProfile as TurnProfile
import Path.Log as PathLog
import Path.Op.Gui.Base as PathOpGui

from PySide import QtCore

__title__ = "CAM Turning Profile Gui"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Gui implementation for turning profiling operations."

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.NOTICE, PathLog.thisModule())


class TaskPanelOpPage(TurnBaseGui.TaskPanelTurnBase):
    """Page controller class for Turning operations."""

    def setOpFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        pass


Command = PathOpGui.SetupOperation(
    "TurnProfile",
    TurnProfile.Create,
    TaskPanelOpPage,
    "CAM_TurnProfile",
    QtCore.QT_TRANSLATE_NOOP("TurnProfile", "Turn Profile"),
    QtCore.QT_TRANSLATE_NOOP(
        "TurnProfile", "Creates a CAM Turninging Profile object from a features of a base object"
    ),
    TurnProfile.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading TurnProfileGui... done\n")
