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
import DraftVecUtils
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftutils.messages import _toolmsg
from draftutils.translate import translate


class Split(gui_base_original.Modifier):
    """Gui Command for the Split tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {"Pixmap": "Draft_Split",
                "Accel": "S, P",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Split", "Split"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Split", "Splits the selected line or polyline into two independent lines\nor polylines by clicking anywhere along the original object.\nIt works best when choosing a point on a straight segment and not a corner vertex.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Split")
        if not self.ui:
            return
        _toolmsg(translate("draft", "Click anywhere on a line to split it."))
        self.view.graphicsView().setFocus()  # Make sure using Esc works.
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
            if info is not None and "Edge" in info["Component"]:
                return self.proceed(info)

    def proceed(self, info):
        """Proceed with execution of the command after click on an edge."""
        self.end_callbacks(self.call)
        wire = info["Object"]
        index = info["Component"][4:]
        point = DraftVecUtils.toString(self.point)

        Gui.addModule("Draft")
        cmd_list = [
            "obj = FreeCAD.ActiveDocument." + wire,
            "new = Draft.split(obj, " + point + ", " + index + ")",
            "Draft.format_object(new, obj)",
            "FreeCAD.ActiveDocument.recompute()"
        ]

        self.commit(translate("draft", "Split line"), cmd_list)
        self.finish()

    def finish(self, cont=False):
        """Terminate the operation."""
        self.end_callbacks(self.call)
        super().finish()


Gui.addCommand('Draft_Split', Split())

## @}
