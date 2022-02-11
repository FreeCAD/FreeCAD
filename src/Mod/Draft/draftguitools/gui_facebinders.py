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
"""Provides GUI tools to create Facebinder objects.

A facebinder is a surface or shell created from the face of a solid object.
This tool allows extracting such faces to be used for other purposes
including extruding solids from faces.
"""
## @package gui_facebinders
# \ingroup draftguitools
# \brief Provides GUI tools to create Facebinder objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Facebinder(gui_base_original.Creator):
    """Gui Command for the Facebinder tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_Facebinder',
             'Accel': "F,F",
             'MenuText': QT_TRANSLATE_NOOP("Draft_Facebinder", "Facebinder"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_Facebinder", "Creates a facebinder object from selected faces.")}
        return d

    def Activated(self):
        """Execute when the command is called."""
        super(Facebinder, self).Activated(name="Facebinder")

        if not Gui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select faces from existing objects"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed when a valid selection has been made."""
        if Gui.Selection.getSelection():
            App.ActiveDocument.openTransaction("Create Facebinder")
            Gui.addModule("Draft")
            Gui.doCommand("s = FreeCADGui.Selection.getSelectionEx()")
            Gui.doCommand("facebinder = Draft.make_facebinder(s)")
            Gui.doCommand('Draft.autogroup(facebinder)')
            Gui.doCommand('FreeCAD.ActiveDocument.recompute()')
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()
        self.finish()


Draft_Facebinder = Facebinder
Gui.addCommand('Draft_Facebinder', Facebinder())

## @}
