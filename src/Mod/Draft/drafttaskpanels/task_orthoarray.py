# ***************************************************************************
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
"""Provides the task panel code for the Draft OrthoArray tool."""
## @package task_orthoarray
# \ingroup drafttaskpanels
# \brief Provides the task panel code for the Draft OrthoArray tool.

## \addtogroup drafttaskpanels
# @{
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc  # include resources, icons, ui files
import DraftVecUtils
from FreeCAD import Units as U
from draftutils import params
from draftutils.messages import _err, _log, _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class TaskPanelOrthoArray:
    """TaskPanel code for the OrthoArray command.

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
    * https://forum.freecad.org/viewtopic.php?f=10&t=40007
    * https://forum.freecad.org/viewtopic.php?t=5374#p43038
    """

    def __init__(self):
        self.name = "Orthogonal array"
        _log(translate("draft","Task panel:") + " {}".format(translate("draft","Orthogonal array")))

        # The .ui file must be loaded into an attribute
        # called `self.form` so that it is displayed in the task panel.
        ui_file = ":/ui/TaskPanel_OrthoArray.ui"
        self.form = Gui.PySideUic.loadUi(ui_file)

        icon_name = "Draft_Array"
        svg = ":/icons/" + icon_name
        pix = QtGui.QPixmap(svg)
        icon = QtGui.QIcon.fromTheme(icon_name, QtGui.QIcon(svg))
        self.form.setWindowIcon(icon)
        self.form.setWindowTitle(translate("draft","Orthogonal array"))

        self.form.label_icon.setPixmap(pix.scaled(32, 32))

        # -------------------------------------------------------------------
        # Default values for the internal function,
        # and for the task panel interface
        start_x = U.Quantity(100.0, App.Units.Length)
        start_y = start_x
        start_z = start_x
        length_unit = start_x.getUserPreferred()[2]

        self.v_x = App.Vector(start_x.Value, 0, 0)
        self.v_y = App.Vector(0, start_y.Value, 0)
        self.v_z = App.Vector(0, 0, start_z.Value)

        self.form.input_X_x.setProperty('rawValue', self.v_x.x)
        self.form.input_X_x.setProperty('unit', length_unit)
        self.form.input_X_y.setProperty('rawValue', self.v_x.y)
        self.form.input_X_y.setProperty('unit', length_unit)
        self.form.input_X_z.setProperty('rawValue', self.v_x.z)
        self.form.input_X_z.setProperty('unit', length_unit)

        self.form.input_Y_x.setProperty('rawValue', self.v_y.x)
        self.form.input_Y_x.setProperty('unit', length_unit)
        self.form.input_Y_y.setProperty('rawValue', self.v_y.y)
        self.form.input_Y_y.setProperty('unit', length_unit)
        self.form.input_Y_z.setProperty('rawValue', self.v_y.z)
        self.form.input_Y_z.setProperty('unit', length_unit)

        self.form.input_Z_x.setProperty('rawValue', self.v_z.x)
        self.form.input_Z_x.setProperty('unit', length_unit)
        self.form.input_Z_y.setProperty('rawValue', self.v_z.y)
        self.form.input_Z_y.setProperty('unit', length_unit)
        self.form.input_Z_z.setProperty('rawValue', self.v_z.z)
        self.form.input_Z_z.setProperty('unit', length_unit)

        self.n_x = 2
        self.n_y = 2
        self.n_z = 1

        self.form.spinbox_n_X.setValue(self.n_x)
        self.form.spinbox_n_Y.setValue(self.n_y)
        self.form.spinbox_n_Z.setValue(self.n_z)

        self.fuse = params.get_param("Draft_array_fuse")
        self.use_link = params.get_param("Draft_array_Link")

        self.form.checkbox_fuse.setChecked(self.fuse)
        self.form.checkbox_link.setChecked(self.use_link)
        # -------------------------------------------------------------------

        # Initialize 1-Axis mode variables
        self.one_axis_mode = False
        self.active_axis = "X"  # Default to X axis when in 1-Axis mode

        # Check if the UI elements exist before trying to access them
        self.has_axis_ui = hasattr(self.form, 'button_one_axis_mode') and \
                          hasattr(self.form, 'checkbox_x_axis') and \
                          hasattr(self.form, 'checkbox_y_axis') and \
                          hasattr(self.form, 'checkbox_z_axis')

        # Hide the axis checkboxes initially (they're only visible in 1-Axis mode)
        if self.has_axis_ui:
            print("DO WE TOGGLE THIS?")
            self.toggle_axis_checkboxes(False)
        # -------------------------------------------------------------------

        # Some objects need to be selected before we can execute the function.
        self.selection = None

        # This is used to test the input of the internal function.
        # It should be changed to True before we can execute the function.
        self.valid_input = False

        self.set_widget_callbacks()

        self.tr_true = QT_TRANSLATE_NOOP("Draft", "True")
        self.tr_false = QT_TRANSLATE_NOOP("Draft", "False")

    def set_widget_callbacks(self):
        """Set up the callbacks (slots) for the widget signals."""
        # New style for Qt5
        self.form.button_reset_X.clicked.connect(lambda: self.reset_v("X"))
        self.form.button_reset_Y.clicked.connect(lambda: self.reset_v("Y"))
        self.form.button_reset_Z.clicked.connect(lambda: self.reset_v("Z"))

        # When the checkbox changes, change the internal value
        self.form.checkbox_fuse.stateChanged.connect(self.set_fuse)
        self.form.checkbox_link.stateChanged.connect(self.set_link)

        # 1-Axis mode callbacks - only set up if the UI elements exist
        if self.has_axis_ui:
            self.form.button_one_axis_mode.clicked.connect(self.toggle_one_axis_mode)
            self.form.checkbox_x_axis.stateChanged.connect(lambda: self.set_active_axis("X"))
            self.form.checkbox_y_axis.stateChanged.connect(lambda: self.set_active_axis("Y"))
            self.form.checkbox_z_axis.stateChanged.connect(lambda: self.set_active_axis("Z"))


    def accept(self):
        """Execute when clicking the OK button or Enter key."""
        self.selection = Gui.Selection.getSelection()

        (self.v_x,
         self.v_y,
         self.v_z) = self.get_intervals()

        (self.n_x,
         self.n_y,
         self.n_z) = self.get_numbers()

        self.valid_input = self.validate_input(self.selection,
                                               self.v_x, self.v_y, self.v_z,
                                               self.n_x, self.n_y, self.n_z)
        if self.valid_input:
            self.create_object()
            # The internal function already displays messages
            self.finish()

    def validate_input(self, selection,
                       v_x, v_y, v_z,
                       n_x, n_y, n_z):
        """Check that the input is valid.

        Some values may not need to be checked because
        the interface may not allow one to input wrong data.
        """
        if not selection:
            _err(translate("draft","At least one element must be selected."))
            return False

        if n_x < 1 or n_y < 1 or n_z < 1:
            _err(translate("draft","Number of elements must be at least 1."))
            return False

        # TODO: this should handle multiple objects.
        # Each of the elements of the selection should be tested.
        obj = selection[0]
        if obj.isDerivedFrom("App::FeaturePython"):
            _err(translate("draft","Selection is not suitable for array."))
            _err(translate("draft","Object:") + " {0} ({1})".format(obj.Label, obj.TypeId))
            return False

        # we should not ever do this but maybe a sanity check here?
        if self.one_axis_mode and self.has_axis_ui:
            if not (self.form.checkbox_x_axis.isChecked() or 
                    self.form.checkbox_y_axis.isChecked() or 
                    self.form.checkbox_z_axis.isChecked()):
                _err(translate("draft","In 1-Axis mode, at least one axis must be selected."))
                return False

        # The other arguments are not tested but they should be present.
        if v_x and v_y and v_z:
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

        # clear this stuff out to be sure we don't preserve something
        # from 3-axis mode
        if self.one_axis_mode and self.has_axis_ui:
            if not self.form.checkbox_x_axis.isChecked():
                self.n_x = 1
                self.v_x = App.Vector(0, 0, 0)
            if not self.form.checkbox_y_axis.isChecked():
                self.n_y = 1
                self.v_y = App.Vector(0, 0, 0)
            if not self.form.checkbox_z_axis.isChecked():
                self.n_z = 1
                self.v_z = App.Vector(0, 0, 0)

        # This creates the object immediately
        # obj = Draft.make_ortho_array(sel_obj,
        #                              self.v_x, self.v_y, self.v_z,
        #                              self.n_x, self.n_y, self.n_z,
        #                              self.use_link)

        # Instead, we build the commands to execute through the caller
        # of this class, the GuiCommand.
        # This is needed to schedule geometry manipulation
        # that would crash Coin3D if done in the event callback.
        _cmd = "Draft.make_ortho_array"
        _cmd += "("
        _cmd += "App.ActiveDocument." + sel_obj.Name + ", "
        _cmd += "v_x=" + DraftVecUtils.toString(self.v_x) + ", "
        _cmd += "v_y=" + DraftVecUtils.toString(self.v_y) + ", "
        _cmd += "v_z=" + DraftVecUtils.toString(self.v_z) + ", "
        _cmd += "n_x=" + str(self.n_x) + ", "
        _cmd += "n_y=" + str(self.n_y) + ", "
        _cmd += "n_z=" + str(self.n_z) + ", "
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        Gui.addModule('Draft')

        _cmd_list = ["_obj_ = " + _cmd,
                     "_obj_.Fuse = " + str(self.fuse),
                     "Draft.autogroup(_obj_)",
                     "App.ActiveDocument.recompute()"]

        # We commit the command list through the parent command
        self.source_command.commit(translate("draft","Orthogonal array"), _cmd_list)

    def get_numbers(self):
        """Get the number of elements from the widgets."""
        return (self.form.spinbox_n_X.value(),
                self.form.spinbox_n_Y.value(),
                self.form.spinbox_n_Z.value())

    def get_intervals(self):
        """Get the interval vectors from the widgets."""
        v_x_x_str = self.form.input_X_x.text()
        v_x_y_str = self.form.input_X_y.text()
        v_x_z_str = self.form.input_X_z.text()
        v_x = App.Vector(U.Quantity(v_x_x_str).Value,
                         U.Quantity(v_x_y_str).Value,
                         U.Quantity(v_x_z_str).Value)

        v_y_x_str = self.form.input_Y_x.text()
        v_y_y_str = self.form.input_Y_y.text()
        v_y_z_str = self.form.input_Y_z.text()
        v_y = App.Vector(U.Quantity(v_y_x_str).Value,
                         U.Quantity(v_y_y_str).Value,
                         U.Quantity(v_y_z_str).Value)

        v_z_x_str = self.form.input_Z_x.text()
        v_z_y_str = self.form.input_Z_y.text()
        v_z_z_str = self.form.input_Z_z.text()
        v_z = App.Vector(U.Quantity(v_z_x_str).Value,
                         U.Quantity(v_z_y_str).Value,
                         U.Quantity(v_z_z_str).Value)
        return v_x, v_y, v_z

    def reset_v(self, interval):
        """Reset the interval to zero distance.

        Parameters
        ----------
        interval: str
            Either "X", "Y", "Z", to reset the interval vector
            for that direction.
        """
        if interval == "X":
            self.form.input_X_x.setProperty('rawValue', 100)
            self.form.input_X_y.setProperty('rawValue', 0)
            self.form.input_X_z.setProperty('rawValue', 0)
            self.v_x, self.v_y, self.v_z = self.get_intervals()
        elif interval == "Y":
            self.form.input_Y_x.setProperty('rawValue', 0)
            self.form.input_Y_y.setProperty('rawValue', 100)
            self.form.input_Y_z.setProperty('rawValue', 0)
            self.v_x, self.v_y, self.v_z = self.get_intervals()
        elif interval == "Z":
            self.form.input_Z_x.setProperty('rawValue', 0)
            self.form.input_Z_y.setProperty('rawValue', 0)
            self.form.input_Z_z.setProperty('rawValue', 100)
            self.v_x, self.v_y, self.v_z = self.get_intervals()

    def print_fuse_state(self, fuse):
        """Print the fuse state translated."""
        if fuse:
            state = self.tr_true
        else:
            state = self.tr_false
        _msg(translate("draft","Fuse:") + " {}".format(state))

    def set_fuse(self):
        """Execute as a callback when the fuse checkbox changes."""
        self.fuse = self.form.checkbox_fuse.isChecked()
        params.set_param("Draft_array_fuse", self.fuse)

    def print_link_state(self, use_link):
        """Print the link state translated."""
        if use_link:
            state = self.tr_true
        else:
            state = self.tr_false
        _msg(translate("draft","Create Link array:") + " {}".format(state))

    def set_link(self):
        """Execute as a callback when the link checkbox changes."""
        self.use_link = self.form.checkbox_link.isChecked()
        params.set_param("Draft_array_Link", self.use_link)

    def print_messages(self):
        """Print messages about the operation."""
        if len(self.selection) == 1:
            sel_obj = self.selection[0]
        else:
            # TODO: this should handle multiple objects.
            # For example, it could take the shapes of all objects,
            # make a compound and then use it as input for the array function.
            sel_obj = self.selection[0]
        _msg(translate("draft","Object:") + " {}".format(sel_obj.Label))

        if self.one_axis_mode and self.has_axis_ui:
            _msg(translate("draft","Using 1-Axis mode"))

        _msg(translate("draft","Number of X elements:") + " {}".format(self.n_x))
        _msg(translate("draft","Interval X:")
             + " ({0}, {1}, {2})".format(self.v_x.x,
                                         self.v_x.y,
                                         self.v_x.z))
        _msg(translate("draft","Number of Y elements:") + " {}".format(self.n_y))
        _msg(translate("draft","Interval Y:")
             + " ({0}, {1}, {2})".format(self.v_y.x,
                                         self.v_y.y,
                                         self.v_y.z))
        _msg(translate("draft","Number of Z elements:") + " {}".format(self.n_z))
        _msg(translate("draft","Interval Z:")
             + " ({0}, {1}, {2})".format(self.v_z.x,
                                         self.v_z.y,
                                         self.v_z.z))
        self.print_fuse_state(self.fuse)
        self.print_link_state(self.use_link)

    def reject(self):
        """Execute when clicking the Cancel button or pressing Escape."""
        self.finish()

    def toggle_one_axis_mode(self):
        """Toggle between 1-Axis mode and 3-Axis mode."""
        self.one_axis_mode = self.form.button_one_axis_mode.isChecked()
        self.toggle_axis_checkboxes(self.one_axis_mode)

        if self.one_axis_mode:
            _msg(translate("draft","Switched to 1-Axis mode"))
            print(translate("draft","Orthogonal array (1-axis mode)"))
            self.form.setWindowTitle(translate("draft","Orthogonal array (1-axis mode)"))
            self.update_axis_ui()
            self.update_interval_visibility()
        else:
            # toggle everything back on
            self.form.setWindowTitle(translate("draft","Orthogonal array"))
            _msg(translate("draft","Switched to 3-Axis mode"))
            self.reset_axis_ui()
            self.show_all_intervals()

    def toggle_axis_checkboxes(self, show):
        """Show or hide the axis checkboxes."""
        if self.has_axis_ui:
            for checkbox in [self.form.checkbox_x_axis,
                            self.form.checkbox_y_axis,
                            self.form.checkbox_z_axis]:
                checkbox.setVisible(show)

    def set_active_axis(self, axis):
        """Set the active axis when a checkbox is changed."""
        if self.one_axis_mode and self.has_axis_ui:
            # get current checkbox that was supposed to have state changed
            checkbox = getattr(self.form, f"checkbox_{axis.lower()}_axis")

            # if we have checked the checkbox, go through other checkboxes
            # and deselect them
            if checkbox.isChecked():
                self.active_axis = axis
                for other_axis in ["X", "Y", "Z"]:
                    if other_axis != axis:
                        other_checkbox = getattr(self.form, f"checkbox_{other_axis.lower()}_axis")
                        other_checkbox.blockSignals(True)
                        other_checkbox.setChecked(False)
                        other_checkbox.blockSignals(False)

                # update visibility of intervals based on axis
                self.update_interval_visibility()
            else:
                # haha!!! you wanted to deselect all of the checkboxes? hell no, we are selecting it back because we
                # do not allow having no axis selected hue hue
                checkbox.setChecked(True)

    def update_axis_ui(self):
        """Update the UI to reflect the current axis selection."""
        # Make sure only one axis is selected
        if self.has_axis_ui:
            self.form.checkbox_x_axis.setChecked(self.active_axis == "X")
            self.form.checkbox_y_axis.setChecked(self.active_axis == "Y")
            self.form.checkbox_z_axis.setChecked(self.active_axis == "Z")

    def reset_axis_ui(self):
        """Reset the UI to 3-Axis mode."""
        # Hide the axis checkboxes
        if self.has_axis_ui:
            self.toggle_axis_checkboxes(False)

    def update_interval_visibility(self):
        """Show only the interval inputs for the selected axis in 1-Axis mode."""
        if self.one_axis_mode and hasattr(self.form, 'group_X') and hasattr(self.form, 'group_Y') and hasattr(self.form, 'group_Z'):
            self.form.group_X.setVisible(self.active_axis == "X")
            self.form.group_Y.setVisible(self.active_axis == "Y")
            self.form.group_Z.setVisible(self.active_axis == "Z")

            if hasattr(self.form, 'label_n_X') and hasattr(self.form, 'spinbox_n_X'):
                self.form.label_n_X.setVisible(self.active_axis == "X")
                self.form.spinbox_n_X.setVisible(self.active_axis == "X")

            if hasattr(self.form, 'label_n_Y') and hasattr(self.form, 'spinbox_n_Y'):
                self.form.label_n_Y.setVisible(self.active_axis == "Y")
                self.form.spinbox_n_Y.setVisible(self.active_axis == "Y")

            if hasattr(self.form, 'label_n_Z') and hasattr(self.form, 'spinbox_n_Z'):
                self.form.label_n_Z.setVisible(self.active_axis == "Z")
                self.form.spinbox_n_Z.setVisible(self.active_axis == "Z")

    def show_all_intervals(self):
        """Show all interval inputs for 3-Axis mode."""
        if hasattr(self.form, 'group_X') and hasattr(self.form, 'group_Y') and hasattr(self.form, 'group_Z'):
            self.form.group_X.setVisible(True)
            self.form.group_Y.setVisible(True)
            self.form.group_Z.setVisible(True)

            if hasattr(self.form, 'label_n_X') and hasattr(self.form, 'spinbox_n_X'):
                self.form.label_n_X.setVisible(True)
                self.form.spinbox_n_X.setVisible(True)

            if hasattr(self.form, 'label_n_Y') and hasattr(self.form, 'spinbox_n_Y'):
                self.form.label_n_Y.setVisible(True)
                self.form.spinbox_n_Y.setVisible(True)

            if hasattr(self.form, 'label_n_Z') and hasattr(self.form, 'spinbox_n_Z'):
                self.form.label_n_Z.setVisible(True)
                self.form.spinbox_n_Z.setVisible(True)

    def finish(self):
        """Finish the command, after accept or reject.

        It finally calls the parent class to execute
        the delayed functions, and perform cleanup.
        """
        # App.ActiveDocument.commitTransaction()
        if Gui.ActiveDocument is not None:
            Gui.ActiveDocument.resetEdit()
        # Runs the parent command to complete the call
        self.source_command.completed()

## @}
