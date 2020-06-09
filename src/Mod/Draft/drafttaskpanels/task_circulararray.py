# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides the task panel code for the Draft CircularArray tool."""
## @package task_circulararray
# \ingroup DRAFT
# \brief This module provides the task panel code for the CircularArray tool.

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc  # include resources, icons, ui files
import DraftVecUtils

from FreeCAD import Units as U
from drafttaskpanels.task_base import TaskPanelPolarCircularBase
from draftutils.messages import _msg, _wrn, _err, _log
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class TaskPanelCircularArray(TaskPanelPolarCircularBase):
    """TaskPanel code for the CircularArray command.

    The names of the widgets are defined in the `.ui` file.
    This `.ui` file `must` be loaded into an attribute
    called `self.form` so that it is loaded into the task panel correctly.

    In this class all widgets are automatically created
    as `self.form.<widget_name>`.

    The `.ui` file may use special FreeCAD widgets such as
    `Gui::InputField` (based on `QLineEdit`) and
    `Gui::QuantitySpinBox` (based on `QAbstractSpinBox`).
    See the Doxygen documentation of the corresponding files in `src/Gui/`,
    for example, `InputField.h` and `QuantitySpinBox.h`.

    Attributes
    ----------
    source_command: gui_base.GuiCommandBase
        This attribute holds a reference to the calling class
        of this task panel.
        This parent class, which is derived from `gui_base.GuiCommandBase`,
        is responsible for calling this task panel, for installing
        certain callbacks, and for removing them.

        It also delays the execution of the internal creation commands
        by using the `draftutils.todo.ToDo` class.

    See Also
    --------
    * https://forum.freecadweb.org/viewtopic.php?f=10&t=40007
    * https://forum.freecadweb.org/viewtopic.php?t=5374#p43038
    """

    def __init__(self):
        self.name = "Circular array"
        _log(_tr("Task panel:") + " {}".format(_tr(self.name)))

        # The .ui file must be loaded into an attribute
        # called `self.form` so that it is displayed in the task panel.
        ui_file = ":/ui/TaskPanel_CircularArray.ui"
        icon_name = "Draft_CircularArray"

        # -------------------------------------------------------------------
        # Default values for the internal function,
        # and for the task panel interface
        start_distance = U.Quantity(50.0, App.Units.Length)
        length_unit = start_distance.getUserPreferred()[2]

        self.r_distance = 2 * start_distance.Value
        self.tan_distance = start_distance.Value

        super(TaskPanelCircularArray, self).__init__(ui_file, icon_name)

        self.form.spinbox_r_distance.setProperty('rawValue',
                                                 self.r_distance)
        self.form.spinbox_r_distance.setProperty('unit', length_unit)
        self.form.spinbox_tan_distance.setProperty('rawValue',
                                                   self.tan_distance)
        self.form.spinbox_tan_distance.setProperty('unit', length_unit)

        self.number = 3
        self.symmetry = 1

        self.form.spinbox_number.setValue(self.number)
        self.form.spinbox_symmetry.setValue(self.symmetry)

        # TODO: the axis is currently fixed, it should be editable
        # or selectable from the task panel
        self.axis = App.Vector(0, 0, 1)

    def accept(self):
        """Execute when clicking the OK button or Enter key."""
        self.selection = Gui.Selection.getSelection()

        (self.r_distance,
         self.tan_distance) = self.get_distances()

        (self.number,
         self.symmetry) = self.get_number_symmetry()

        self.axis = self.get_axis()
        self.center = self.get_center()

        self.valid_input = self.validate_input(self.selection,
                                               self.r_distance,
                                               self.tan_distance,
                                               self.number,
                                               self.symmetry,
                                               self.axis,
                                               self.center)
        if self.valid_input:
            self.create_object()
            # The internal function already displays messages
            # self.print_messages()
            self.finish()

    def validate_input(self, selection,
                       r_distance, tan_distance,
                       number, symmetry,
                       axis, center):
        """Check that the input is valid.

        Some values may not need to be checked because
        the interface may not allow to input wrong data.
        """
        if not selection:
            _err(_tr("At least one element must be selected."))
            return False

        if number < 2:
            _err(_tr("Number of layers must be at least 2."))
            return False

        # TODO: this should handle multiple objects.
        # Each of the elements of the selection should be tested.
        obj = selection[0]
        if obj.isDerivedFrom("App::FeaturePython"):
            _err(_tr("Selection is not suitable for array."))
            _err(_tr("Object:") + " {}".format(selection[0].Label))
            return False

        if r_distance == 0:
            _wrn(_tr("Radial distance is zero. "
                     "Resulting array may not look correct."))
        elif r_distance < 0:
            _wrn(_tr("Radial distance is negative. "
                     "It is made positive to proceed."))
            self.r_distance = abs(r_distance)

        if tan_distance == 0:
            _err(_tr("Tangential distance cannot be zero."))
            return False
        elif tan_distance < 0:
            _wrn(_tr("Tangential distance is negative. "
                     "It is made positive to proceed."))
            self.tan_distance = abs(tan_distance)

        # The other arguments are not tested but they should be present.
        if symmetry and axis and center:
            pass

        self.fuse = self.form.checkbox_fuse.isChecked()
        self.use_link = self.form.checkbox_link.isChecked()
        return True

    def create_object(self):
        """Create the new object.

        At this stage we already tested that the input is correct
        so the necessary attributes are already set.
        Then we proceed with the internal function to create the new object.
        """
        if len(self.selection) == 1:
            sel_obj = self.selection[0]
        else:
            # TODO: this should handle multiple objects.
            # For example, it could take the shapes of all objects,
            # make a compound and then use it as input for the array function.
            sel_obj = self.selection[0]

        # This creates the object immediately
        # obj = Draft.make_circular_array(sel_obj,
        #                                 self.r_distance, self.tan_distance,
        #                                 self.number, self.symmetry,
        #                                 self.axis, self.center,
        #                                 self.use_link)

        # Instead, we build the commands to execute through the caller
        # of this class, the GuiCommand.
        # This is needed to schedule geometry manipulation
        # that would crash Coin3D if done in the event callback.
        _cmd = "Draft.make_circular_array"
        _cmd += "("
        _cmd += "App.ActiveDocument." + sel_obj.Name + ", "
        _cmd += "r_distance=" + str(self.r_distance) + ", "
        _cmd += "tan_distance=" + str(self.tan_distance) + ", "
        _cmd += "number=" + str(self.number) + ", "
        _cmd += "symmetry=" + str(self.symmetry) + ", "
        _cmd += "axis=" + DraftVecUtils.toString(self.axis) + ", "
        _cmd += "center=" + DraftVecUtils.toString(self.center) + ", "
        if self.axis_name and self.edge_name:
            _cmd += "axis_object='" + str(self.axis_name) + "', "
            _cmd += "axis_edge='" + str(self.edge_name) + "', "
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        Gui.addModule('Draft')

        _cmd_list = ["_obj_ = " + _cmd,
                     "_obj_.Fuse = " + str(self.fuse),
                     "Draft.autogroup(_obj_)",
                     "App.ActiveDocument.recompute()"]

        # We commit the command list through the parent command
        self.source_command.commit(_tr(self.name), _cmd_list)

    def get_distances(self):
        """Get the distance parameters from the widgets."""
        r_d_str = self.form.spinbox_r_distance.text()
        tan_d_str = self.form.spinbox_tan_distance.text()
        return (U.Quantity(r_d_str).Value,
                U.Quantity(tan_d_str).Value)

    def get_number_symmetry(self):
        """Get the number and symmetry parameters from the widgets."""
        number = self.form.spinbox_number.value()
        symmetry = self.form.spinbox_symmetry.value()
        return number, symmetry

    def get_axis(self):
        """Get the axis that will be used for the array. NOT IMPLEMENTED.

        It should consider a second selection of an edge or wire to use
        as an axis.
        """
        return self.axis

    def print_messages(self):
        """Print messages about the operation."""
        if len(self.selection) == 1:
            sel_obj = self.selection[0]
        else:
            # TODO: this should handle multiple objects.
            # For example, it could take the shapes of all objects,
            # make a compound and then use it as input for the array function.
            sel_obj = self.selection[0]
        _msg(_tr("Object:") + " {}".format(sel_obj.Label))
        _msg(_tr("Radial distance:") + " {}".format(self.r_distance))
        _msg(_tr("Tangential distance:") + " {}".format(self.tan_distance))
        _msg(_tr("Number of circular layers:") + " {}".format(self.number))
        _msg(_tr("Symmetry parameter:") + " {}".format(self.symmetry))
        _msg(_tr("Center of rotation:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))
        self.print_fuse_state(self.fuse)
        self.print_link_state(self.use_link)
