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
"""Provides GUI tools to downgrade objects.

Downgrades 2D objects to simpler objects until it reaches
simple Edge primitives. For example, a Draft Line to wire, and then
to a series of edges.
"""
## @package gui_downgrade
# \ingroup draftguitools
# \brief Provides GUI tools to downgrade objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Downgrade(gui_base_original.Modifier):
    """Gui Command for the Downgrade tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Downgrade',
                'Accel': "D, N",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Downgrade", "Downgrade"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Downgrade", "Downgrades the selected objects into simpler shapes.\nThe result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.\nFor example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Downgrade, self).Activated(name="Downgrade")
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to upgrade"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
            else:
                self.proceed()

    def proceed(self):
        """Proceed with execution of the command after selection."""
        if Gui.Selection.getSelection():
            Gui.addModule("Draft")
            _cmd = 'Draft.downgrade'
            _cmd += '('
            _cmd += 'FreeCADGui.Selection.getSelection(), '
            _cmd += 'delete=True'
            _cmd += ')'
            _cmd_list = ['_objs_ = ' + _cmd,
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Downgrade"),
                        _cmd_list)
        self.finish()


Gui.addCommand('Draft_Downgrade', Downgrade())

## @}
