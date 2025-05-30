# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
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
"""Provides GUI tools to highlight subelements of objects.

The highlighting can be used to manipulate shapes with other tools
such as Move, Rotate, and Scale.
"""
## @package gui_subelements
# \ingroup draftguitools
# \brief Provides GUI tools to highlight subelements of objects.

## \addtogroup draftguitools
# @{
import pivy.coin as coin
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftutils import utils
from draftutils import gui_utils
from draftutils.messages import _msg
from draftutils.translate import translate


class SubelementHighlight(gui_base_original.Modifier):
    """Gui Command for the SubelementHighlight tool."""

    def __init__(self):
        super().__init__()
        self.is_running = False
        self.editable_objects = []
        self.original_view_settings = {}

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_SubelementHighlight',
                'Accel': "H, S",
                'MenuText': QT_TRANSLATE_NOOP("Draft_SubelementHighlight","Subelement highlight"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_SubelementHighlight","Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.")}

    def Activated(self):
        """Execute when the command is called."""
        if self.is_running:
            return self.finish()
        self.is_running = True
        super().Activated(name="Subelement highlight")
        self.get_selection()

    def proceed(self):
        """Continue with the command."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.get_editable_objects_from_selection()
        if not self.editable_objects:
            return self.finish()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        self.highlight_editable_objects()

    def finish(self):
        """Terminate the operation.

        Re-initialize by running __init__ again at the end.
        """
        self.end_callbacks(self.call)
        self.restore_editable_objects_graphics()
        super().finish()
        self.__init__()

    def action(self, event):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        event: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        if event["Type"] == "SoKeyboardEvent" and event["Key"] == "ESCAPE":
            self.finish()

    def get_selection(self):
        """Get the selection."""
        if not Gui.Selection.getSelection() and self.ui:
            _msg(translate("draft", "Select an object to edit"))
            self.call = self.view.addEventCallback("SoEvent",
                                                   gui_tool_utils.selectObject)
        else:
            self.proceed()

    def get_editable_objects_from_selection(self):
        """Get editable Draft objects for the selection."""
        for obj in Gui.Selection.getSelection():
            if (obj.isDerivedFrom("Part::Part2DObject")
                or utils.get_type(obj) in ["BezCurve", "BSpline", "Wire"]):
                self.editable_objects.append(obj)
            elif (hasattr(obj, "Base")
                  and (obj.Base.isDerivedFrom("Part::Part2DObject")
                       or utils.get_type(obj.Base) in ["BezCurve", "BSpline", "Wire"])):
                self.editable_objects.append(obj.Base)

    def highlight_editable_objects(self):
        """Highlight editable Draft objects from the selection."""
        for obj in self.editable_objects:
            vobj = obj.ViewObject
            self.original_view_settings[obj.Name] = {
                'Visibility': vobj.Visibility,
                'PointSize': vobj.PointSize,
                'PointColor': vobj.PointColor,
                'LineColor': vobj.LineColor}
            vobj.Visibility = True
            vobj.PointSize = 10
            vobj.PointColor = (1.0, 0.0, 0.0)
            vobj.LineColor = (1.0, 0.0, 0.0)
            xray = coin.SoAnnotation()
            switch = gui_utils.find_coin_node(vobj.RootNode, coin.SoSwitch)
            if switch is not None:
                xray.addChild(switch.getChild(0))
                xray.setName("xray")
                vobj.RootNode.addChild(xray)

    def restore_editable_objects_graphics(self):
        """Restore the editable objects' appearance."""
        for obj in self.editable_objects:
            try:
                for attribute, value in self.original_view_settings[obj.Name].items():
                    vobj = obj.ViewObject
                    setattr(vobj, attribute, value)
                    vobj.RootNode.removeChild(vobj.RootNode.getByName("xray"))
            except Exception:
                # This can occur if objects have had graph changing operations
                pass


Gui.addCommand('Draft_SubelementHighlight', SubelementHighlight())

## @}
