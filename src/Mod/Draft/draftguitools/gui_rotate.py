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
import Draft_rc
import DraftVecUtils
import draftutils.groups as groups
import draftutils.todo as todo
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from FreeCAD import Units as U
from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Rotate(gui_base_original.Modifier):
    """Gui Command for the Rotate tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ()

        return {'Pixmap': 'Draft_Rotate',
                'Accel': "R, O",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Rotate", "Rotate"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Rotate", "Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.\nIf the \"copy\" option is active, it will create rotated copies.\nCTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.")}

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
        if Gui.Selection.getSelection():
            return self.proceed()
        self.ui.selectUi(on_close_call=self.finish)
        _msg(translate("draft", "Select an object to rotate"))
        self.call = \
            self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def proceed(self):
        """Continue with the command after a selection has been made."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selected_objects = Gui.Selection.getSelection()
        self.selected_objects = \
            groups.get_group_contents(self.selected_objects,
                                      addgroups=True,
                                      spaces=True,
                                      noarchchild=True)
        self.selected_subelements = Gui.Selection.getSelectionEx()
        self.step = 0
        self.center = None
        self.ui.rotateSetCenterUi()
        self.arctrack = trackers.arcTracker()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick rotation center"))

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
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"):
            self.handle_mouse_click_event(arg)

    def handle_mouse_move_event(self, arg):
        """Handle the mouse when moving."""
        for ghost in self.ghosts:
            ghost.off()
        self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
        # this is to make sure radius is what you see on screen
        if self.center and DraftVecUtils.dist(self.point, self.center):
            viewdelta = DraftVecUtils.project(self.point.sub(self.center),
                                              self.wp.axis)
            if not DraftVecUtils.isNull(viewdelta):
                self.point = self.point.add(viewdelta.negative())
        if self.extendedCopy:
            if not gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                self.step = 3
                self.finish()
        if self.step == 0:
            pass
        elif self.step == 1:
            currentrad = DraftVecUtils.dist(self.point, self.center)
            if currentrad != 0:
                angle = DraftVecUtils.angle(self.wp.u,
                                            self.point.sub(self.center),
                                            self.wp.axis)
            else:
                angle = 0
            self.ui.setRadiusValue(math.degrees(angle), unit="Angle")
            self.firstangle = angle
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
        elif self.step == 2:
            currentrad = DraftVecUtils.dist(self.point, self.center)
            if currentrad != 0:
                angle = DraftVecUtils.angle(self.wp.u,
                                            self.point.sub(self.center),
                                            self.wp.axis)
            else:
                angle = 0
            if angle < self.firstangle:
                sweep = (2 * math.pi - self.firstangle) + angle
            else:
                sweep = angle - self.firstangle
            self.arctrack.setApertureAngle(sweep)
            for ghost in self.ghosts:
                ghost.rotate(self.wp.axis, sweep)
                ghost.on()
            self.ui.setRadiusValue(math.degrees(sweep), 'Angle')
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
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText(translate("draft", "Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft", "The base angle you wish to start the rotation from"))
        self.arctrack.setCenter(self.center)
        for ghost in self.ghosts:
            ghost.center(self.center)
        self.step = 1
        _msg(translate("draft", "Pick base angle"))
        if self.planetrack:
            self.planetrack.set(self.point)

    def set_start_point(self):
        """Set the starting point of the rotation."""
        self.ui.labelRadius.setText(translate("draft", "Rotation"))
        self.ui.radiusValue.setToolTip(translate("draft", "The amount of rotation you wish to perform.\nThe final angle will be the base angle plus this amount."))
        self.rad = DraftVecUtils.dist(self.point, self.center)
        self.arctrack.on()
        self.arctrack.setStartPoint(self.point)
        for ghost in self.ghosts:
            ghost.on()
        self.step = 2
        _msg(translate("draft", "Pick rotation angle"))

    def set_rotation_angle(self, arg):
        """Set the rotation angle."""

        # currentrad = DraftVecUtils.dist(self.point, self.center)
        angle = self.point.sub(self.center).getAngle(self.wp.u)
        _v = DraftVecUtils.project(self.point.sub(self.center), self.wp.v)
        if _v.getAngle(self.wp.v) > 1:
            angle = -angle
        if angle < self.firstangle:
            self.angle = (2 * math.pi - self.firstangle) + angle
        else:
            self.angle = angle - self.firstangle
        self.rotate(self.ui.isCopy.isChecked()
                    or gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT))
        if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
            self.extendedCopy = True
        else:
            self.finish(cont=None)

    def set_ghosts(self):
        """Set the ghost to display."""
        for ghost in self.ghosts:
            ghost.remove()
        if self.ui.isSubelementMode.isChecked():
            self.ghosts = self.get_subelement_ghosts()
        else:
            self.ghosts = [trackers.ghostTracker(self.selected_objects)]
        if self.center:
            for ghost in self.ghosts:
                ghost.center(self.center)

    def get_subelement_ghosts(self):
        """Get ghost for the subelements (vertices, edges)."""
        import Part

        ghosts = []
        for sel in Gui.Selection.getSelectionEx("", 0):
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                if "Vertex" in sub or "Edge" in sub:
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
        if self.arctrack:
            self.arctrack.finalize()
        for ghost in self.ghosts:
            ghost.finalize()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            todo.ToDo.delayAfter(self.Activated, [])
        super().finish()
        if self.doc:
            self.doc.recompute()

    def rotate(self, is_copy=False):
        """Perform the rotation of the subelements or the entire object."""
        if self.ui.isSubelementMode.isChecked():
            self.rotate_subelements(is_copy)
        else:
            self.rotate_object(is_copy)

    def rotate_subelements(self, is_copy):
        """Rotate the subelements."""
        Gui.addModule("Draft")
        try:
            if is_copy:
                self.commit(translate("draft", "Copy"),
                            self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Rotate"),
                            self.build_rotate_subelements_command())
        except Exception:
            _err(translate("draft", "Some subelements could not be moved."))

    def build_copy_subelements_command(self):
        """Build the string to commit to copy the subelements."""
        import Part

        command = []
        arguments = []
        E = len("Edge")
        for obj in self.selected_subelements:
            for index, subelement in enumerate(obj.SubObjects):
                if not isinstance(subelement, Part.Edge):
                    continue
                _edge_index = int(obj.SubElementNames[index][E:]) - 1
                _cmd = '['
                _cmd += 'FreeCAD.ActiveDocument.'
                _cmd += obj.ObjectName + ', '
                _cmd += str(_edge_index) + ', '
                _cmd += str(math.degrees(self.angle)) + ', '
                _cmd += DraftVecUtils.toString(self.center) + ', '
                _cmd += DraftVecUtils.toString(self.wp.axis)
                _cmd += ']'
                arguments.append(_cmd)

        all_args = ', '.join(arguments)
        command.append('Draft.copy_rotated_edges([' + all_args + '])')
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_rotate_subelements_command(self):
        """Build the string to commit to rotate the subelements."""
        import Part

        command = []
        V = len("Vertex")
        E = len("Edge")
        for obj in self.selected_subelements:
            for index, subelement in enumerate(obj.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    _vertex_index = int(obj.SubElementNames[index][V:]) - 1
                    _cmd = 'Draft.rotate_vertex'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_vertex_index) + ', '
                    _cmd += str(math.degrees(self.angle)) + ', '
                    _cmd += DraftVecUtils.toString(self.center) + ', '
                    _cmd += DraftVecUtils.toString(self.wp.axis)
                    _cmd += ')'
                    command.append(_cmd)
                elif isinstance(subelement, Part.Edge):
                    _edge_index = int(obj.SubElementNames[index][E:]) - 1
                    _cmd = 'Draft.rotate_edge'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_edge_index) + ', '
                    _cmd += str(math.degrees(self.angle)) + ', '
                    _cmd += DraftVecUtils.toString(self.center) + ', '
                    _cmd += DraftVecUtils.toString(self.wp.axis)
                    _cmd += ')'
                    command.append(_cmd)
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def rotate_object(self, is_copy):
        """Move the object."""
        _doc = 'FreeCAD.ActiveDocument.'
        _selected = self.selected_objects

        objects = '['
        objects += ','.join([_doc + obj.Name for obj in _selected])
        objects += ']'

        _cmd = 'Draft.rotate'
        _cmd += '('
        _cmd += objects + ', '
        _cmd += str(math.degrees(self.angle)) + ', '
        _cmd += DraftVecUtils.toString(self.center) + ', '
        _cmd += 'axis=' + DraftVecUtils.toString(self.wp.axis) + ', '
        _cmd += 'copy=' + str(is_copy)
        _cmd += ')'
        _cmd_list = [_cmd,
                     'FreeCAD.ActiveDocument.recompute()']

        _mode = "Copy" if is_copy else "Rotate"
        Gui.addModule("Draft")
        self.commit(translate("draft", _mode),
                    _cmd_list)

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
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText(translate("draft", "Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft", "The base angle you wish to start the rotation from"))
        self.ui.radiusValue.setText(U.Quantity(0, U.Angle).UserString)
        self.step = 1
        _msg(translate("draft", "Pick base angle"))

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
            _msg(translate("draft", "Pick rotation angle"))
        else:
            self.angle = math.radians(rad)
            self.rotate(self.ui.isCopy.isChecked())
            self.finish(cont=None)


Gui.addCommand('Draft_Rotate', Rotate())

## @}
