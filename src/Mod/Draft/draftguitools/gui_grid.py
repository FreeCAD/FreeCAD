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
"""Provides GUI tools to enable and disable the working plane grid."""
## @package gui_grid
# \ingroup draftguitools
# \brief Provides GUI tools to enable and disable the working plane grid.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import WorkingPlane

from draftguitools import gui_base
from draftutils import gui_utils
from draftutils.translate import translate


class ToggleGrid(gui_base.GuiCommandSimplest):
    """The Draft ToggleGrid command definition.

    If the grid tracker is invisible (hidden), it makes it visible (shown);
    and if it is visible, it hides it.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def __init__(self):
        super().__init__(name=translate("draft", "Toggle grid"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Grid",
                "Accel": "G, R",
                "MenuText": QT_TRANSLATE_NOOP("Draft_ToggleGrid", "Toggle grid"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_ToggleGrid",
                                             "Toggles the Draft grid on and off."),
                "CmdType": "ForEdit"}

    def IsActive(self):
        """Return True when this command should be available."""
        return bool(gui_utils.get_3d_view())

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        if not hasattr(Gui, "Snapper"):
            return
        Gui.Snapper.setTrackers(update_grid=False)
        grid = Gui.Snapper.grid
        # This command is never set as App.activeDraftCommand.
        cmdactive = hasattr(App, "activeDraftCommand") and App.activeDraftCommand

        if grid.Visible:
            grid.off()
            grid.show_always = False
            if cmdactive:
                grid.show_during_command = False
        elif cmdactive:
            grid.set()  # set() required: the grid must be updated to match the current WP
            grid.show_during_command = True
        else:
            grid.set()
            WorkingPlane.get_working_plane()
            grid.show_always = True

Gui.addCommand("Draft_ToggleGrid", ToggleGrid())

## @}
