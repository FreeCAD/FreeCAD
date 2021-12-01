# ***************************************************************************
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the task panel code for the Draft SelectPlane tool.

As it is right now this code only loads the task panel .ui file.
All logic on how to use the widgets is located in the GuiCommand class
itself.
On the other hand, the newer tools introduced in v0.19 like OrthoArray,
PolarArray, and CircularArray include the logic and manipulation
of the widgets in this task panel class.
In addition, the task panel code launches the actual function
using the delayed mechanism defined by the `todo.ToDo` class.
Therefore, at some point this class should be refactored
to be more similar to OrthoArray and the new tools.
"""
## @package task_selectplane
# \ingroup drafttaskpanels
# \brief Provides the task panel code for the Draft SelectPlane tool.

## \addtogroup drafttaskpanels
# @{
import FreeCADGui as Gui


class SelectPlaneTaskPanel:
    """The task panel definition of the Draft_SelectPlane command."""

    def __init__(self):
        self.form = Gui.PySideUic.loadUi(":/ui/TaskSelectPlane.ui")

    def getStandardButtons(self):
        """Execute to set the standard buttons."""
        return 2097152  # int(QtGui.QDialogButtonBox.Close)

## @}
