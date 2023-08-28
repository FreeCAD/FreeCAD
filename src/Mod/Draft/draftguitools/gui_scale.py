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
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftutils.groups as groups
import draftutils.todo as todo
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
import drafttaskpanels.task_scale as task_scale

from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Scale(gui_base_original.Modifier):
    """Gui Command for the Scale tool.

    This tool scales the selected objects from a base point.
    """

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Scale',
                'Accel': "S, C",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Scale", "Scale"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Scale", "Scales the selected objects from a base point.\nCTRL to snap, SHIFT to constrain, ALT to copy.")}

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
        self.call = self.view.addEventCallback("SoEvent",
                                               gui_tool_utils.selectObject)

    def proceed(self):
        """Proceed with execution of the command after selection."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)

        self.selected_objects = Gui.Selection.getSelection()
        self.selected_objects = \
            groups.get_group_contents(self.selected_objects)
        self.selected_subelements = Gui.Selection.getSelectionEx()
        self.refs = []
        self.ui.pointUi(title=translate("draft",self.featureName), icon="Draft_Scale")
        self.ui.isRelative.hide()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.pickmode = False
        self.task = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick base point"))

    def set_ghosts(self):
        """Set the ghost to display."""
        for ghost in self.ghosts:
            ghost.remove()
        if self.task and self.task.isSubelementMode.isChecked():
            self.ghosts = self.get_subelement_ghosts()
        else:
            self.ghosts = [trackers.ghostTracker(self.selected_objects)]

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

    def pickRef(self):
        """Pick a point of reference."""
        self.pickmode = True
        if self.node:
            self.node = self.node[:1]  # remove previous picks
        _msg(translate("draft", "Pick reference distance from base point"))
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
        self.delta = App.Vector(self.task.xValue.value(),
                                self.task.yValue.value(),
                                self.task.zValue.value())
        self.center = self.node[0]
        if self.task.isSubelementMode.isChecked():
            self.scale_subelements()
        elif self.task.isClone.isChecked():
            self.scale_with_clone()
        else:
            self.scale_object()
        self.finish()

    def scale_subelements(self):
        """Scale only the subelements if the appropriate option is set.

        The subelements operations only really work with polylines (Wires)
        because internally the functions `scale_vertex` and `scale_edge`
        only work with polylines that have a `Points` property.

        BUG: the code should not cause an error. It should check that
        the selected object is not a rectangle or another object
        that can't be used with `scale_vertex` and `scale_edge`.
        """
        Gui.addModule("Draft")
        try:
            if self.task.isCopy.isChecked():
                self.commit(translate("draft", "Copy"),
                            self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Scale"),
                            self.build_scale_subelements_command())
        except Exception:
            _err(translate("draft", "Some subelements could not be scaled."))

    def scale_with_clone(self):
        """Scale with clone."""
        if self.task.relative.isChecked():
            self.delta = self.wp.getGlobalCoords(self.delta)

        Gui.addModule("Draft")

        _doc = 'FreeCAD.ActiveDocument.'
        _selected = self.selected_objects

        objects = '['
        objects += ', '.join([_doc + obj.Name for obj in _selected])
        objects += ']'

        if self.task.isCopy.isChecked():
            _cmd_name = translate("draft", "Copy")
        else:
            _cmd_name = translate("draft", "Scale")

        # the correction translation of the clone placement is
        # (node[0] - clone.Placement.Base) - (node[0] - clone.Placement.Base)\
        #   .scale(delta.x,delta.y,delta.z)
        # equivalent to:
        # (node[0] - clone.Placement.Base)\
        #   .scale(1-delta.x,1-delta.y,1-delta.z)
        str_node0 = DraftVecUtils.toString(self.node[0])
        str_delta = DraftVecUtils.toString(self.delta)
        str_delta_corr = DraftVecUtils.toString(App.Vector(1,1,1) - self.delta)

        _cmd = 'Draft.make_clone'
        _cmd += '('
        _cmd += objects + ', '
        _cmd += 'forcedraft=True'
        _cmd += ')'
        _cmd_list = ['clone = ' + _cmd,
                     'clone.Scale = ' + str_delta,
                     'clone_corr = (' + str_node0 + ' - clone.Placement.Base)'\
                        + '.scale(*'+ str_delta_corr + ')',
                     'clone.Placement.move(clone_corr)',
                     'FreeCAD.ActiveDocument.recompute()']
        self.commit(_cmd_name, _cmd_list)

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
                _cmd += DraftVecUtils.toString(self.delta) + ', '
                _cmd += DraftVecUtils.toString(self.center)
                _cmd += ']'
                arguments.append(_cmd)
        all_args = ', '.join(arguments)
        command.append('Draft.copy_scaled_edges([' + all_args + '])')
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_scale_subelements_command(self):
        """Build the strings to commit to scale the subelements."""
        import Part

        command = []
        V = len("Vertex")
        E = len("Edge")
        for obj in self.selected_subelements:
            for index, subelement in enumerate(obj.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    _vertex_index = int(obj.SubElementNames[index][V:]) - 1
                    _cmd = 'Draft.scale_vertex'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_vertex_index) + ', '
                    _cmd += DraftVecUtils.toString(self.delta) + ', '
                    _cmd += DraftVecUtils.toString(self.center)
                    _cmd += ')'
                    command.append(_cmd)
                elif isinstance(subelement, Part.Edge):
                    _edge_index = int(obj.SubElementNames[index][E:]) - 1
                    _cmd = 'Draft.scale_edge'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += obj.ObjectName + ', '
                    _cmd += str(_edge_index) + ', '
                    _cmd += DraftVecUtils.toString(self.delta) + ', '
                    _cmd += DraftVecUtils.toString(self.center)
                    _cmd += ')'
                    command.append(_cmd)
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def is_scalable(self, obj):
        """Return True only for the supported objects.

        Currently it only supports `Rectangle`, `Wire`, `Annotation`
        and `BSpline`.
        """
        t = utils.getType(obj)
        if t in ["Rectangle", "Wire", "Annotation", "BSpline","Image::ImagePlane"]:
            # TODO: support more types in Draft.scale
            return True
        else:
            return False

    def scale_object(self):
        """Scale the object."""
        if self.task.relative.isChecked():
            self.delta =self.wp.getGlobalCoords(self.delta)
        goods = []
        bads = []
        for obj in self.selected_objects:
            if self.is_scalable(obj):
                goods.append(obj)
            else:
                bads.append(obj)
        if bads:
            if len(bads) == 1:
                m = translate("draft", "Unable to scale object:")
                m += " "
                m += bads[0].Label
            else:
                m = translate("draft", "Unable to scale objects:")
                m += " "
                m += ", ".join([o.Label for o in bads])
            m += " - " + translate("draft","This object type cannot be scaled directly. Please use the clone method.")
            _err(m)
        if goods:
            _doc = 'FreeCAD.ActiveDocument.'
            objects = '['
            objects += ', '.join([_doc + obj.Name for obj in goods])
            objects += ']'
            Gui.addModule("Draft")

            if self.task.isCopy.isChecked():
                _cmd_name = translate("draft", "Copy")
            else:
                _cmd_name = translate("draft", "Scale")

            _cmd = 'Draft.scale'
            _cmd += '('
            _cmd += objects + ', '
            _cmd += 'scale=' + DraftVecUtils.toString(self.delta) + ', '
            _cmd += 'center=' + DraftVecUtils.toString(self.center) + ', '
            _cmd += 'copy=' + str(self.task.isCopy.isChecked())
            _cmd += ')'
            _cmd_list = ['ss = ' + _cmd,
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(_cmd_name, _cmd_list)

    def scaleGhost(self, x, y, z, rel):
        """Scale the preview of the object."""
        delta = App.Vector(x, y, z)
        if rel:
            delta = self.wp.getGlobalCoords(delta)
        for ghost in self.ghosts:
            ghost.scale(delta)
        # calculate a correction factor depending on the scaling center
        corr = App.Vector(self.node[0].x, self.node[0].y, self.node[0].z)
        corr.scale(delta.x, delta.y, delta.z)
        corr = (corr.sub(self.node[0])).negative()
        for ghost in self.ghosts:
            ghost.move(corr)
            ghost.on()

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
            _msg(translate("draft", "Pick new distance from base point"))
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
        super().finish()
        for ghost in self.ghosts:
            ghost.finalize()


Gui.addCommand('Draft_Scale', Scale())

## @}
