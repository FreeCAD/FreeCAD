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
"""Provides the task panel for the Draft PolarArray tool."""
## @package task_polararray
# \ingroup DRAFT
# \brief This module provides the task panel code for the PolarArray tool.

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


class TaskPanelPolarArray(TaskPanelPolarCircularBase):
    """TaskPanel code for the PolarArray command.

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
        self.name = "Polar array"
        _log(_tr("Task panel:") + " {}".format(_tr(self.name)))

        # The .ui file must be loaded into an attribute
        # called `self.form` so that it is displayed in the task panel.
        ui_file = ":/ui/TaskPanel_PolarArray.ui"
        icon_name = "Draft_PolarArray"

        # -------------------------------------------------------------------
        # Default values for the internal function,
        # and for the task panel interface
        start_angle = U.Quantity(360.0, App.Units.Angle)
        angle_unit = start_angle.getUserPreferred()[2]

        self.angle = start_angle.Value
        self.number = 5

        super(TaskPanelPolarArray, self).__init__(ui_file, icon_name)

        self.form.spinbox_angle.setProperty('rawValue', self.angle)
        self.form.spinbox_angle.setProperty('unit', angle_unit)

        self.form.spinbox_number.setValue(self.number)

    def accept(self):
        """Execute when clicking the OK button or Enter key."""
        self.selection = Gui.Selection.getSelection()

        (self.number,
         self.angle) = self.get_number_angle()

        self.center = self.get_center()

        self.valid_input = self.validate_input(self.selection,
                                               self.number,
                                               self.angle,
                                               self.center)
        if self.valid_input:
            self.create_object()
            # The internal function already displays messages
            # self.print_messages()
            self.finish()

    def validate_input(self, selection,
                       number, angle, center):
        """Check that the input is valid.

        Some values may not need to be checked because
        the interface may not allow to input wrong data.
        """
        if not selection:
            _err(_tr("At least one element must be selected."))
            return False

        # TODO: this should handle multiple objects.
        # Each of the elements of the selection should be tested.
        obj = selection[0]
        if obj.isDerivedFrom("App::FeaturePython"):
            _err(_tr("Selection is not suitable for array."))
            _err(_tr("Object:") + " {}".format(selection[0].Label))
            return False

        if number < 2:
            _err(_tr("Number of elements must be at least 2."))
            return False

        if angle > 360:
            _wrn(_tr("The angle is above 360 degrees. "
                     "It is set to this value to proceed."))
            self.angle = 360
        elif angle < -360:
            _wrn(_tr("The angle is below -360 degrees. "
                     "It is set to this value to proceed."))
            self.angle = -360

        # The other arguments are not tested but they should be present.
        if center:
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
        # obj = Draft.make_polar_array(sel_obj,
        #                              self.number, self.angle, self.center,
        #                              self.use_link)

        # Instead, we build the commands to execute through the caller
        # of this class, the GuiCommand.
        # This is needed to schedule geometry manipulation
        # that would crash Coin3D if done in the event callback.
        _cmd = "Draft.make_polar_array"
        _cmd += "("
        _cmd += "App.ActiveDocument." + sel_obj.Name + ", "
        _cmd += "number=" + str(self.number) + ", "
        _cmd += "angle=" + str(self.angle) + ", "
        _cmd += "center=" + DraftVecUtils.toString(self.center) + ", "
        if self.axis_name and self.edge_index:
            _cmd += "axis_object='" + str(self.axis_name) + "', "
            _cmd += "axis_edge=" + str(self.edge_index) + ", "
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        Gui.addModule('Draft')

        _cmd_list = ["_obj_ = " + _cmd,
                     "_obj_.Fuse = " + str(self.fuse),
                     "Draft.autogroup(_obj_)",
                     "App.ActiveDocument.recompute()"]

        # We commit the command list through the parent command
        self.source_command.commit(_tr(self.name), _cmd_list)

    def get_number_angle(self):
        """Get the number and angle parameters from the widgets."""
        number = self.form.spinbox_number.value()

        angle_str = self.form.spinbox_angle.text()
        angle = U.Quantity(angle_str).Value
        return number, angle

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
        _msg(_tr("Number of elements:") + " {}".format(self.number))
        _msg(_tr("Polar angle:") + " {}".format(self.angle))
        _msg(_tr("Center of rotation:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))
        self.print_fuse_state(self.fuse)
        self.print_link_state(self.use_link)
