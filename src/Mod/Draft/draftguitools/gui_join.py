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
"""Provides GUI tools to join lines and wires."""
## @package gui_join
# \ingroup draftguitools
# \brief Provides GUI tools to join lines and wires.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftutils import utils
from draftutils.messages import _msg, _err, _toolmsg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Join(gui_base_original.Modifier):
    """Gui Command for the Join tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {"Pixmap": "Draft_Join",
                "Accel": "J, O",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Join", "Join"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Join", "Joins the selected lines or polylines into a single object.\nThe lines must share a common point at the start or at the end for the operation to succeed.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Join")
        if not self.ui:
            return
        if not Gui.Selection.getSelection():
            self.ui.selectUi(on_close_call=self.finish)
            _msg(translate("draft", "Select an object to join"))
            self.call = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with execution of the command after proper selection.

        BUG: It occasionally fails to join lines even if the lines
        visually share a point. This is due to the underlying `joinWires`
        method not handling the points correctly.
        """
        if self.call is not None:
            self.end_callbacks(self.call)
        if Gui.Selection.getSelection():
            self.print_selection()
            if all(utils.get_type(o) == "Wire" for o in Gui.Selection.getSelection()):
                Gui.addModule("Draft")
                _cmd = "Draft.join_wires"
                _cmd += "("
                _cmd += "FreeCADGui.Selection.getSelection()"
                _cmd += ")"
                _cmd_list = ['j = ' + _cmd,
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Join lines"),
                            _cmd_list)
            else:
                _err(translate("draft", "Only Draft Lines and Wires can be joined"))
        self.finish()

    def print_selection(self):
        """Print the selected items."""
        labels = []
        for obj in Gui.Selection.getSelection():
            labels.append(obj.Label)

        labels = ", ".join(labels)
        _toolmsg(translate("draft","Selection:") + " {}".format(labels))


Gui.addCommand('Draft_Join', Join())

## @}
