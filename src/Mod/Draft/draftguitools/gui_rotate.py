# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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
"""Provides GUI tools to rotate objects in the 3D space."""
## @package gui_rotate
# \ingroup draftguitools
# \brief Provides GUI tools to rotate objects in the 3D space.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
from draftgeoutils import geometry
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers
from draftutils import utils
from draftutils import todo
from draftutils.messages import _msg, _err, _toolmsg
from draftutils.translate import translate
from FreeCAD import Units as U


class Rotate(gui_base_original.Modifier):
    """Gui Command for the Rotate tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Rotate",
                "Accel": "R, O",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Rotate", "Rotate"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Rotate", "Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.\nIf the \"copy\" option is active, it will create rotated copies.\nSHIFT to constrain. Hold ALT and click to create a copy with each click.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Rotate")
        if not self.ui:
            return
        self.ghosts = []
        self.arctrack = None
        self.get_object_selection()

    def get_object_selection(self):
        """Get the object selection."""
        if Gui.Selection.hasSelection():
            return self.proceed()
        self.ui.selectUi(on_close_call=self.finish)
        _msg(translate("draft", "Select an object to rotate"))
        self.call = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def proceed(self):
        """Continue with the command after a selection has been made."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selection = Gui.Selection.getSelectionEx("", 0)
        Gui.doCommand("selection = FreeCADGui.Selection.getSelectionEx(\"\", 0)")
        self.step = 0
        self.center = None
        self.point = None
        self.firstangle = None
        self.ui.rotateSetCenterUi()
        self.arctrack = trackers.arcTracker()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick rotation center"))

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        elif not self.ui.mouse:
            pass
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"):
            self.handle_mouse_click_event(arg)

    def _get_angle(self):
        if self.center is None:
            return 0
        if self.point is None:
            return 0
        if DraftVecUtils.dist(self.point, self.center) < 1e-7:
            return 0
        angle = DraftVecUtils.angle(self.wp.u, self.point.sub(self.center), self.wp.axis)
        if self.firstangle is None:
            return angle
        if angle < self.firstangle:
            return (2 * math.pi - self.firstangle) + angle
        return angle - self.firstangle

    def handle_mouse_move_event(self, arg):
        """Handle the mouse when moving."""
        for ghost in self.ghosts:
            ghost.off()
        self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
        if self.center is not None:
            # Project self.point on a plane that is parallel to the wp and that
            # passes through self.center.
            self.point = geometry.project_point_on_plane(
                self.point,
                self.center,
                self.wp.axis,
                direction=None,
                force_projection=True
            )
        if self.extendedCopy:
            if not gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key()):
                self.step = 3
                self.finish()
        if self.step == 0:
            pass
        elif self.step == 1:
            angle = self._get_angle()
            self.ui.setRadiusValue(math.degrees(angle), unit="Angle")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
        elif self.step == 2:
            angle = self._get_angle()
            self.arctrack.setApertureAngle(angle)
            for ghost in self.ghosts:
                ghost.rotate(self.wp.axis, angle)
                ghost.on()
            self.ui.setRadiusValue(math.degrees(angle), unit="Angle")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
        gui_tool_utils.redraw3DView()

    def handle_mouse_click_event(self, arg):
        """Handle the mouse when the first button is clicked."""
        if not self.point:
            return
        if self.step == 0:
            self.set_center()
        elif self.step == 1:
            self.set_start_point()
        else:
            self.set_rotation_angle(arg)

    def set_center(self):
        """Set the center of the rotation."""
        if not self.ghosts:
            self.set_ghosts()
        self.center = self.point
        self.node = [self.point]
        self.ui.radiusUi()
        self.ui.radiusValue.setText(U.Quantity(0, U.Angle).UserString)
        self.ui.makeFace.hide()
        self.ui.labelRadius.setText(translate("draft", "Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft", "The base angle you wish to start the rotation from"))
        self.arctrack.setCenter(self.center)
        for ghost in self.ghosts:
            ghost.center(self.center)
        self.step = 1
        _toolmsg(translate("draft", "Pick base angle"))
        if self.planetrack:
            self.planetrack.set(self.point)

    def set_start_point(self):
        """Set the starting point of the rotation."""
        self.firstangle = self._get_angle()
        self.ui.labelRadius.setText(translate("draft", "Rotation"))
        self.ui.radiusValue.setToolTip(translate("draft", "The amount of rotation you wish to perform.\nThe final angle will be the base angle plus this amount."))
        self.arctrack.on()
        self.arctrack.setStartPoint(self.point)
        for ghost in self.ghosts:
            ghost.on()
        self.step = 2
        _toolmsg(translate("draft", "Pick rotation angle"))

    def set_rotation_angle(self, arg):
        """Set the rotation angle."""
        self.angle = self._get_angle()
        if self.angle != 0:
            self.rotate(self.ui.isCopy.isChecked()
                        or gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key()))
        if gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key()):
            self.extendedCopy = True
        else:
            self.finish(cont=None)

    def set_ghosts(self):
        """Set the ghost to display."""
        for ghost in self.ghosts:
            ghost.remove()
        copy = self.ui.isCopy.isChecked()
        if self.ui.isSubelementMode.isChecked():
            self.ghosts = self.get_subelement_ghosts(self.selection, copy)
            if not self.ghosts:
                _err(translate("draft", "No valid subelements selected"))
        else:
            objs, places, _ = utils._modifiers_process_selection(self.selection, copy, add_movable_children=(not copy))
            self.ghosts = [trackers.ghostTracker(objs, parent_places=places)]
        if self.center:
            for ghost in self.ghosts:
                ghost.center(self.center)

    def get_subelement_ghosts(self, selection, copy):
        """Get ghost for the subelements (vertices, edges)."""
        import Part
        ghosts = []
        for sel in selection:
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                if (not copy and "Vertex" in sub) or "Edge" in sub:
                    shape = Part.getShape(sel.Object, sub, needSubElement=True, retType=0)
                    ghosts.append(trackers.ghostTracker(shape))
        return ghosts

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        self.end_callbacks(self.call)
        if self.arctrack:
            self.arctrack.finalize()
        for ghost in self.ghosts:
            ghost.finalize()
        super().finish()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            todo.ToDo.delayAfter(self.Activated, [])

    def rotate(self, copy):
        """Perform the rotation of the subelement(s) or the entire object(s)."""
        if copy:
            cmd_name = translate("draft", "Copy")
        else:
            cmd_name = translate("draft", "Rotate")
        Gui.addModule("Draft")
        cmd = "Draft.rotate(selection, "
        cmd += str(math.degrees(self.angle)) + ", "
        cmd += "center=" + DraftVecUtils.toString(self.center) + ", "
        cmd += "axis=" + DraftVecUtils.toString(self.wp.axis) + ", "
        cmd += "copy=" + str(copy) + ", "
        cmd += "subelements=" + str(self.ui.isSubelementMode.isChecked()) + ")"
        cmd_list = [cmd, "FreeCAD.ActiveDocument.recompute()"]
        self.commit(cmd_name, cmd_list)

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.center = App.Vector(numx, numy, numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        for ghost in self.ghosts:
            ghost.center(self.center)
        self.ui.radiusUi()
        self.ui.makeFace.hide()
        self.ui.labelRadius.setText(translate("draft", "Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft", "The base angle you wish to start the rotation from"))
        self.ui.radiusValue.setText(U.Quantity(0, U.Angle).UserString)
        self.step = 1
        _toolmsg(translate("draft", "Pick base angle"))

    def numericRadius(self, rad):
        """Validate the radius entry field in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        if self.step == 1:
            self.ui.labelRadius.setText(translate("draft", "Rotation"))
            self.ui.radiusValue.setToolTip(translate("draft", "The amount of rotation you wish to perform.\nThe final angle will be the base angle plus this amount."))
            self.ui.radiusValue.setText(U.Quantity(0, U.Angle).UserString)
            self.firstangle = math.radians(rad)
            self.arctrack.setStartAngle(self.firstangle)
            self.arctrack.on()
            for ghost in self.ghosts:
                ghost.on()
            self.step = 2
            _toolmsg(translate("draft", "Pick rotation angle"))
        else:
            self.angle = math.radians(rad)
            self.rotate(self.ui.isCopy.isChecked())
            self.finish(cont=None)


Gui.addCommand('Draft_Rotate', Rotate())

## @}
