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
"""Provides GUI tools to move objects in the 3D space."""
## @package gui_move
# \ingroup draftguitools
# \brief Provides GUI tools to move objects in the 3D space.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers
from draftguitools.gui_subelements import SubelementHighlight
from draftutils import utils
from draftutils import todo
from draftutils.messages import _msg, _err, _toolmsg
from draftutils.translate import translate


class Move(gui_base_original.Modifier):
    """Gui Command for the Move tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Move",
                "Accel": "M, V",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Move", "Move"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Move", "Moves the selected objects.\nIf the \"copy\" option is active, it will create displaced copies.\nSHIFT to constrain. Hold ALT and click to create a copy with each click.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Move",
                          is_subtool=isinstance(App.activeDraftCommand,
                                                SubelementHighlight))
        if not self.ui:
            return
        self.ghosts = []
        self.get_object_selection()

    def get_object_selection(self):
        """Get the object selection."""
        if Gui.Selection.hasSelection():
            return self.proceed()
        self.ui.selectUi(on_close_call=self.finish)
        _msg(translate("draft", "Select an object to move"))
        self.call = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def proceed(self):
        """Continue with the command after a selection has been made."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selection = Gui.Selection.getSelectionEx("", 0)
        Gui.doCommand("selection = FreeCADGui.Selection.getSelectionEx(\"\", 0)")
        self.ui.lineUi(title=translate("draft", self.featureName), icon="Draft_Move")
        self.ui.modUi()
        if self.copymode:
            self.ui.isCopy.setChecked(True)
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick start point"))

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        self.end_callbacks(self.call)
        for ghost in self.ghosts:
            ghost.finalize()
        super().finish()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            todo.ToDo.delayAfter(self.Activated, [])

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
            if self.point:
                self.vector = self.point.sub(last)
            else:
                self.vector = None
            for ghost in self.ghosts:
                if self.vector:
                    ghost.move(self.vector)
                ghost.on()
        if self.extendedCopy:
            if not gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key()):
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
            _toolmsg(translate("draft", "Pick end point"))
            if self.planetrack:
                self.planetrack.set(self.point)
        else:
            last = self.node[0]
            self.vector = self.point.sub(last)
            self.move(self.ui.isCopy.isChecked()
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

    def move(self, copy):
        """Perform the move of the subelement(s) or the entire object(s)."""
        if copy:
            cmd_name = translate("draft", "Copy")
        else:
            cmd_name = translate("draft", "Move")
        Gui.addModule("Draft")
        cmd = "Draft.move(selection, "
        cmd += DraftVecUtils.toString(self.vector) + ", "
        cmd += "copy=" + str(copy) + ", "
        cmd += "subelements=" + str(self.ui.isSubelementMode.isChecked()) + ")"
        cmd_list = [cmd, "FreeCAD.ActiveDocument.recompute()"]
        self.commit(cmd_name, cmd_list)

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
            _toolmsg(translate("draft", "Pick end point"))
        else:
            last = self.node[-1]
            self.vector = self.point.sub(last)
            self.move(self.ui.isCopy.isChecked())
            self.finish(cont=None)


Gui.addCommand('Draft_Move', Move())

## @}
