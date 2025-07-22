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
"""Provides GUI tools to scale objects in the 3D space.

The scale operation can also be done with subelements.

The subelements operations only really work with polylines (Wires)
because internally the functions `scale_vertex` and `scale_edge`
only work with polylines that have a `Points` property.
"""
## @package gui_scale
# \ingroup draftguitools
# \brief Provides GUI tools to scale objects in the 3D space.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers
from draftutils import groups
from draftutils import params
from draftutils import utils
from draftutils import todo
from draftutils.messages import _msg, _err, _toolmsg
from draftutils.translate import translate
from drafttaskpanels import task_scale


class Scale(gui_base_original.Modifier):
    """Gui Command for the Scale tool.

    This tool scales the selected objects from a base point.
    """

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_Scale",
                "Accel": "S, C",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Scale", "Scale"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Scale", "Scales the selected objects from a base point.\nSHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Scale")
        if not self.ui:
            return
        self.ghosts = []
        self.get_object_selection()

    def get_object_selection(self):
        """Get object selection and proceed if successful."""
        if Gui.Selection.getSelection():
            return self.proceed()
        self.ui.selectUi(on_close_call=self.finish)
        _msg(translate("draft", "Select an object to scale"))
        self.call = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def proceed(self):
        """Proceed with execution of the command after selection."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selection = Gui.Selection.getSelectionEx("", 0)
        Gui.doCommand("selection = FreeCADGui.Selection.getSelectionEx(\"\", 0)")
        self.refs = []
        self.ui.pointUi(title=translate("draft",self.featureName), icon="Draft_Scale")
        self.ui.isRelative.hide()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.pickmode = False
        self.task = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick base point"))

    def set_ghosts(self):
        """Set the ghost to display."""
        for ghost in self.ghosts:
            ghost.remove()
        if self.task is None:
            copy = params.get_param("ScaleCopy")
            clone = params.get_param("ScaleClone")
            subelements = params.get_param("SubelementMode")
        else:
            copy = self.task.isCopy.isChecked()
            clone = self.task.isClone.isChecked()
            subelements = self.task.isSubelementMode.isChecked()
        if subelements:
            self.ghosts = self.get_subelement_ghosts(self.selection, copy)
            if not self.ghosts:
                _err(translate("draft", "No valid subelements selected"))
        else:
            objs, places, _ = utils._modifiers_process_selection(self.selection, (copy or clone), scale=True)
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

    def scale_ghosts(self, x, y, z, rel):
        """Scale the preview of the object."""
        delta = App.Vector(x, y, z)
        if rel:
            delta = self.wp.get_global_coords(delta)
        for ghost in self.ghosts:
            ghost.scale(delta)
        # calculate a correction factor depending on the scaling center
        corr = App.Vector(self.node[0])
        corr.scale(*delta)
        corr = (corr.sub(self.node[0])).negative()
        for ghost in self.ghosts:
            ghost.flip_normals(x * y * z < 0)
            ghost.move(corr)
            ghost.on()

    def pick_ref(self):
        """Pick a point of reference."""
        self.pickmode = True
        if self.node:
            self.node = self.node[:1]  # remove previous picks
        _toolmsg(translate("draft", "Pick reference distance from base point"))
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
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"
              and self.point):
            self.handle_mouse_click_event()

    def handle_mouse_move_event(self, arg):
        """Handle the mouse event of movement."""
        for ghost in self.ghosts:
            ghost.off()
        self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)

    def handle_mouse_click_event(self):
        """Handle the mouse click event."""
        if not self.ghosts:
            self.set_ghosts()
        self.numericInput(self.point.x, self.point.y, self.point.z)

    def scale(self):
        """Perform the scale of the object.

        Scales the subelements, or with a clone, or just general scaling.
        """
        sx = self.task.xValue.value()
        sy = self.task.yValue.value()
        sz = self.task.zValue.value()
        if sx * sy * sz == 0:
            _err(translate("draft", "Zero scale factor not allowed"))
            self.finish()
            return

        self.delta = App.Vector(sx, sy, sz)
        if self.task.relative.isChecked():
            self.delta = self.wp.get_global_coords(self.delta)
        self.center = self.node[0]
        if self.task.isCopy.isChecked():
            cmd_name = translate("draft", "Copy")
        else:
            cmd_name = translate("draft", "Scale")
        Gui.addModule("Draft")
        cmd = "Draft.scale(selection, "
        cmd += "scale=" + DraftVecUtils.toString(self.delta) + ", "
        cmd += "center=" + DraftVecUtils.toString(self.center) + ", "
        cmd += "copy=" + str(self.task.isCopy.isChecked()) + ", "
        cmd += "clone=" + str(self.task.isClone.isChecked()) + ", "
        cmd += "subelements=" + str(self.task.isSubelementMode.isChecked()) + ")"
        cmd_list = [cmd, "FreeCAD.ActiveDocument.recompute()"]
        self.commit(cmd_name, cmd_list)
        self.finish()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        self.node.append(self.point)
        if not self.pickmode:
            if not self.ghosts:
                self.set_ghosts()
            self.ui.offUi()
            if self.call:
                self.view.removeEventCallback("SoEvent", self.call)
            self.task = task_scale.ScaleTaskPanel()
            self.task.sourceCmd = self
            todo.ToDo.delay(Gui.Control.showDialog, self.task)
            todo.ToDo.delay(self.task.xValue.selectAll, None)
            todo.ToDo.delay(self.task.xValue.setFocus, None)
            for ghost in self.ghosts:
                ghost.on()
        elif len(self.node) == 2:
            _toolmsg(translate("draft", "Pick new distance from base point"))
        elif len(self.node) == 3:
            if hasattr(Gui, "Snapper"):
                Gui.Snapper.off()
            if self.call:
                self.view.removeEventCallback("SoEvent", self.call)
            d1 = (self.node[1].sub(self.node[0])).Length
            d2 = (self.node[2].sub(self.node[0])).Length
            # print ("d2/d1 = {}".format(d2/d1))
            if hasattr(self, "task"):
                if self.task:
                    self.task.lock.setChecked(True)
                    self.task.setValue(d2/d1)

    def finish(self, cont=False):
        """Terminate the operation."""
        self.end_callbacks(self.call)
        for ghost in self.ghosts:
            ghost.finalize()
        super().finish()


Gui.addCommand('Draft_Scale', Scale())

## @}
