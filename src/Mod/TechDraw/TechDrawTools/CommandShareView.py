# ***************************************************************************
# *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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
"""Provides the TechDraw ShareView GuiCommand."""

__title__ = "TechDrawTools.CommandShareView"
__author__ = "WandererFan"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2022/01/11"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools

class CommandShareView:
    """Shares a View on current Page to another Page."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_ShareView.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_ShareView", "Share View"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_ShareView", "Share a View on a second Page")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
#        print("Activated()")
        sel = Gui.Selection.getSelection()

        vName = ""
        views = list()
        for o in sel:
            if o.isDerivedFrom("TechDraw::DrawView"):
                views.append(o)
        if views:
            vName = views[0].Name

        toPageName = ""
        fromPageName = ""
        pages = list()
        for o in sel:
            if o.isDerivedFrom("TechDraw::DrawPage"):
                pages.append(o)
        if pages:
            fromPageName = pages[0].Name
        if len(pages) > 1:
            toPageName = pages[1].Name

        self.ui  = TechDrawTools.TaskShareView()

        self.ui.setValues(vName, fromPageName, toPageName)
        Gui.Control.showDialog(self.ui)

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TechDrawTools.TDToolsUtil.havePage() and TechDrawTools.TDToolsUtil.haveView()
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_ShareView', CommandShareView())

