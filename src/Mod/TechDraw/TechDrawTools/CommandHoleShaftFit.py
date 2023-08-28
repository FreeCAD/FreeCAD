# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""Provides the TechDraw HoleShaftFit GuiCommand."""

__title__ = "TechDrawTools.CommandHoleShaftFit"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/02/07"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools

class CommandHoleShaftFit:
    """Adds a hole or shaft fit to a selected dimension."""

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_HoleShaftFit.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_HoleShaftFit", "Add hole or shaft fit"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_HoleShaftFit", "Add a hole or shaft fit to a dimension<br>\
                - select one length dimension or diameter dimension<br>\
                - click the tool button, a panel opens<br>\
                - select shaft fit / hole fit<br>\
                - select the desired ISO 286 fit field using the combo box")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        sel = Gui.Selection.getSelectionEx()
        #if sel and sel[0].Object.TypeId == 'TechDraw::DrawViewDimension':
        if sel[0].Object.TypeId == 'TechDraw::DrawViewDimension':
            self.ui = TechDrawTools.TaskHoleShaftFit(sel)
            Gui.Control.showDialog(self.ui)

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TechDrawTools.TDToolsUtil.havePage() and TechDrawTools.TDToolsUtil.haveView()
        else:
            return False

#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_HoleShaftFit', CommandHoleShaftFit())