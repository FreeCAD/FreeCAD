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
"""Provides GUI tools to split line and wire objects."""
## @package gui_split
# \ingroup draftguitools
# \brief Provides GUI tools to split line and wire objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Split(gui_base_original.Modifier):
    """Gui Command for the Split tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Split',
                'Accel': "S, P",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Split", "Split"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Split", "Splits the selected line or polyline into two independent lines\nor polylines by clicking anywhere along the original object.\nIt works best when choosing a point on a straight segment and not a corner vertex.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Split, self).Activated(name="Split")
        if not self.ui:
            return
        _msg(translate("draft", "Click anywhere on a line to split it."))
        self.call = self.view.addEventCallback("SoEvent", self.action)

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            gui_tool_utils.getPoint(self, arg)
            gui_tool_utils.redraw3DView()
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["Button"] == "BUTTON1"
              and arg["State"] == "DOWN"):
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            if "Edge" in info["Component"]:
                return self.proceed(info)

    def proceed(self, info):
        """Proceed with execution of the command after click on an edge."""
        wire = App.ActiveDocument.getObject(info["Object"])
        edge_index = int(info["Component"][4:])

        Gui.addModule("Draft")
        _cmd = "Draft.split"
        _cmd += "("
        _cmd += "FreeCAD.ActiveDocument." + wire.Name + ", "
        _cmd += DraftVecUtils.toString(self.point) + ", "
        _cmd += str(edge_index)
        _cmd += ")"
        _cmd_list = ["s = " + _cmd,
                     "FreeCAD.ActiveDocument.recompute()"]

        self.commit(translate("draft", "Split line"),
                    _cmd_list)

        self.finish()


Gui.addCommand('Draft_Split', Split())

## @}
