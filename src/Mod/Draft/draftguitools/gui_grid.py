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
"""Provide the Draft_ToggleGrid command to show the Draft grid."""
## @package gui_grid
# \ingroup DRAFT
# \brief Provide the Draft_ToggleGrid command to show the Draft grid.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import draftguitools.gui_base as gui_base
from draftutils.translate import _tr


class ToggleGrid(gui_base.GuiCommandSimplest):
    """The Draft ToggleGrid command definition.

    If the grid tracker is invisible (hidden), it makes it visible (shown);
    and if it is visible, it hides it.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def __init__(self):
        super(ToggleGrid, self).__init__(name=_tr("Toggle grid"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Toggles the Draft grid on and off."

        d = {'Pixmap': 'Draft_Grid',
             'Accel': "G,R",
             'MenuText': QT_TRANSLATE_NOOP("Draft_ToggleGrid",
                                           "Toggle grid"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_ToggleGrid",
                                          _tip),
             'CmdType': 'ForEdit'}

        return d

    def Activated(self):
        """Execute when the command is called."""
        super(ToggleGrid, self).Activated()

        if hasattr(Gui, "Snapper"):
            Gui.Snapper.setTrackers()
            if Gui.Snapper.grid:
                if Gui.Snapper.grid.Visible:
                    Gui.Snapper.grid.off()
                    Gui.Snapper.forceGridOff = True
                else:
                    Gui.Snapper.grid.on()
                    Gui.Snapper.forceGridOff = False


Gui.addCommand('Draft_ToggleGrid', ToggleGrid())
