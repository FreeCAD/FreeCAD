# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides GUI tools to create Circle objects."""
## @package gui_circles
# \ingroup draftguitools
# \brief Provides GUI tools to create Circle objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_arcs as gui_arcs

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Circle(gui_arcs.Arc):
    """Gui command for the Circle tool.

    It inherits the entire `Arc` class.
    The only difference is that the `closedCircle` attribute
    is already set to `True`, and the `featureName` attribute
    is `'Circle'`.

    This will result in an arc that describes a complete circumference
    so the starting angle and end angle will be the same.

    Internally, both circular arcs and circles are `'Circle'` objects.

    Discussion
    ----------
    Both arcs and circles are `'Circle'` objects, but when it comes to the
    Gui Commands, the relationships are reversed, and both launch the `Arc`
    command.

    Maybe the relationship should be changed: the base Gui Command
    should be `Circle`, and an arc would launch the same command,
    as both are internally `'Circle'` objects.

    Another possibility is to rename the `'Circle'` object to `'Arc'`.
    Then both a circle and an arc would internally be `'Arc'` objects,
    and in the Gui Commands they both would use the `Arc` command.
    """

    def __init__(self):
        super().__init__()
        self.closedCircle = True
        self.featureName = "Circle"

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Circle',
                'Accel': "C, I",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Circle", "Circle"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Circle", "Creates a circle (full circular arc).\nCTRL to snap, ALT to select tangent objects.")}


Gui.addCommand('Draft_Circle', Circle())

## @}
