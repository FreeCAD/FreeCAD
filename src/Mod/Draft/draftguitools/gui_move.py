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
"""Provides tools for moving objects in the 3D space."""
## @package gui_move
# \ingroup DRAFT
# \brief Provides tools for moving objects in the 3D space.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftutils.todo as todo
import draftguitools.gui_base_original as gui_base_original
from draftguitools.gui_subelements import SubelementHighlight
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Move(gui_base_original.Modifier):
    """Gui Command for the Move tool."""

    def __init__(self):
        super(Move, self).__init__()

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ("Moves the selected objects from one base point "
                "to another point.\n"
                'If the "copy" option is active, it will create '
                "displaced copies.\n"
                "CTRL to snap, SHIFT to constrain.")

        return {'Pixmap': 'Draft_Move',
                'Accel': "M, V",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Move", "Move"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Move", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        self.name = translate("draft", "Move")
        super(Move, self).Activated(self.name,
                                    is_subtool=isinstance(App.activeDraftCommand,
                                                          SubelementHighlight))
        if not self.ui:
            return
        self.ghosts = []
        self.get_object_selection()

    def get_object_selection(self):
        """Get the object selection."""
        if Gui.Selection.getSelectionEx():
            return self.proceed()
        self.ui.selectUi()
        _msg(translate("draft", "Select an object to move"))
        self.call = \
            self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def proceed(self):
        """Continue with the command after a selection has been made."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selected_objects = Gui.Selection.getSelection()
        self.selected_objects = utils.getGroupContents(self.selected_objects,
                                                       addgroups=True,
                                                       spaces=True,
                                                       noarchchild=True)
        self.selected_subelements = Gui.Selection.getSelectionEx()
        self.ui.lineUi(self.name)
        self.ui.modUi()
        if self.copymode:
            self.ui.isCopy.setChecked(True)
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick start point"))

    def finish(self, closed=False, cont=False):
        """Finish the move operation."""
        for ghost in self.ghosts:
            ghost.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                todo.ToDo.delayAfter(self.Activated, [])
        super(Move, self).finish()

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
        if len(self.node) > 0:
            last = self.node[len(self.node) - 1]
            self.vector = self.point.sub(last)
            for ghost in self.ghosts:
                ghost.move(self.vector)
                ghost.on()
        if self.extendedCopy:
            if not gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                self.finish()
        gui_tool_utils.redraw3DView()

    def handle_mouse_click_event(self, arg):
        """Handle the mouse when the first button is clicked."""
        if not self.ghosts:
            self.set_ghosts()
        if not self.point:
            return
        self.ui.redraw()
        if self.node == []:
            self.node.append(self.point)
            self.ui.isRelative.show()
            for ghost in self.ghosts:
                ghost.on()
            _msg(translate("draft", "Pick end point"))
            if self.planetrack:
                self.planetrack.set(self.point)
        else:
            last = self.node[0]
            self.vector = self.point.sub(last)
            self.move()
            if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                self.extendedCopy = True
            else:
                self.finish(cont=True)

    def set_ghosts(self):
        """Set the ghost to display."""
        if self.ui.isSubelementMode.isChecked():
            return self.set_subelement_ghosts()
        self.ghosts = [trackers.ghostTracker(self.selected_objects)]

    def set_subelement_ghosts(self):
        """Set ghost for the subelements."""
        import Part

        for object in self.selected_subelements:
            for subelement in object.SubObjects:
                if (isinstance(subelement, Part.Vertex)
                        or isinstance(subelement, Part.Edge)):
                    self.ghosts.append(trackers.ghostTracker(subelement))

    def move(self):
        """Perform the move of the subelements or the entire object."""
        if self.ui.isSubelementMode.isChecked():
            self.move_subelements()
        else:
            self.move_object()

    def move_subelements(self):
        """Move the subelements."""
        try:
            if self.ui.isCopy.isChecked():
                self.commit(translate("draft", "Copy"),
                            self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Move"),
                            self.build_move_subelements_command())
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
                _cmd += DraftVecUtils.toString(self.vector)
                _cmd += ']'
                arguments.append(_cmd)

        all_args = ', '.join(arguments)
        command.append('Draft.copyMovedEdges([' + all_args + '])')
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_move_subelements_command(self):
        """Build the string to commit to move the subelements."""
        import Part

        command = []
        V = len("Vertex")
        E = len("Edge")
        for obj in self.selected_subelements:
            for index, subelement in enumerate(obj.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    _vertex_index = int(obj.SubElementNames[index][V:]) - 1
                    _cmd = 'Draft.moveVertex'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_vertex_index) + ', '
                    _cmd += DraftVecUtils.toString(self.vector)
                    _cmd += ')'
                    command.append(_cmd)
                elif isinstance(subelement, Part.Edge):
                    _edge_index = int(obj.SubElementNames[index][E:]) - 1
                    _cmd = 'Draft.moveEdge'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_edge_index) + ', '
                    _cmd += DraftVecUtils.toString(self.vector)
                    _cmd += ')'
                    command.append(_cmd)
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def move_object(self):
        """Move the object."""
        _doc = 'FreeCAD.ActiveDocument.'
        _selected = self.selected_objects

        objects = '['
        objects += ', '.join([_doc + obj.Name for obj in _selected])
        objects += ']'
        Gui.addModule("Draft")

        _cmd = 'Draft.move'
        _cmd += '('
        _cmd += objects + ', '
        _cmd += DraftVecUtils.toString(self.vector) + ', '
        _cmd += 'copy=' + str(self.ui.isCopy.isChecked())
        _cmd += ')'
        _cmd_list = [_cmd,
                     'FreeCAD.ActiveDocument.recompute()']

        _mode = "Copy" if self.ui.isCopy.isChecked() else "Move"
        self.commit(translate("draft", _mode),
                    _cmd_list)

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        if not self.node:
            self.node.append(self.point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            for ghost in self.ghosts:
                ghost.on()
            _msg(translate("draft", "Pick end point"))
        else:
            last = self.node[-1]
            self.vector = self.point.sub(last)
            self.move()
            self.finish()


Gui.addCommand('Draft_Move', Move())
