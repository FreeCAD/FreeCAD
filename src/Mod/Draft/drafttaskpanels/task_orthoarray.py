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
from PySide import QtWidgets
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

        self.form = Gui.PySideUic.loadUi(":/ui/TaskPanel_OrthoArray.ui")
        self.form.setWindowTitle(translate("draft", "Orthogonal array"))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Array.svg"))

        # -------------------------------------------------------------------
        # Default values for the internal function,
        # and for the task panel interface
        start_x = params.get_param("XInterval", "Mod/Draft/OrthoArrayLinearMode")
        start_y = params.get_param("YInterval", "Mod/Draft/OrthoArrayLinearMode")
        start_z = params.get_param("ZInterval", "Mod/Draft/OrthoArrayLinearMode")

        self.v_x = App.Vector(start_x, 0, 0)
        self.v_y = App.Vector(0, start_y, 0)
        self.v_z = App.Vector(0, 0, start_z)

        self.form.input_X_x.setProperty('rawValue', self.v_x.x)
        self.form.input_X_y.setProperty('rawValue', self.v_x.y)
        self.form.input_X_z.setProperty('rawValue', self.v_x.z)

        self.form.input_Y_x.setProperty('rawValue', self.v_y.x)
        self.form.input_Y_y.setProperty('rawValue', self.v_y.y)
        self.form.input_Y_z.setProperty('rawValue', self.v_y.z)

        self.form.input_Z_x.setProperty('rawValue', self.v_z.x)
        self.form.input_Z_y.setProperty('rawValue', self.v_z.y)
        self.form.input_Z_z.setProperty('rawValue', self.v_z.z)

        self.n_x = params.get_param("XNumOfElements", "Mod/Draft/OrthoArrayLinearMode")
        self.n_y = params.get_param("YNumOfElements", "Mod/Draft/OrthoArrayLinearMode")
        self.n_z = params.get_param("ZNumOfElements", "Mod/Draft/OrthoArrayLinearMode")

        self.form.spinbox_n_X.setValue(self.n_x)
        self.form.spinbox_n_Y.setValue(self.n_y)
        self.form.spinbox_n_Z.setValue(self.n_z)

        self.fuse = params.get_param("Draft_array_fuse")
        self.use_link = params.get_param("Draft_array_Link")

        self.form.checkbox_fuse.setChecked(self.fuse)
        self.form.checkbox_link.setChecked(self.use_link)
        # -------------------------------------------------------------------

        # Initialize Linear mode variables
        self.linear_mode = params.get_param("LinearModeOn", "Mod/Draft/OrthoArrayLinearMode")
        self.active_axis = params.get_param("AxisSelected", "Mod/Draft/OrthoArrayLinearMode")

        # Hide the axis radiobuttons initially (they're only visible in Linear mode)
        self.toggle_axis_radiobuttons(False)
        if self.linear_mode:
            self.form.button_linear_mode.setChecked(True)
            self.toggle_linear_mode()
        else:
            self.form.group_linearmode.hide()
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
        if hasattr(self.form.checkbox_fuse, "checkStateChanged"): # Qt version >= 6.7.0
            self.form.checkbox_fuse.checkStateChanged.connect(self.set_fuse)
            self.form.checkbox_link.checkStateChanged.connect(self.set_link)
        else: # Qt version < 6.7.0
            self.form.checkbox_fuse.stateChanged.connect(self.set_fuse)
            self.form.checkbox_link.stateChanged.connect(self.set_link)

        # Linear mode callbacks - only set up if the UI elements exist
        self.form.button_linear_mode.clicked.connect(self.toggle_linear_mode)
        self.form.radiobutton_x_axis.clicked.connect(lambda: self.set_active_axis("X"))
        self.form.radiobutton_y_axis.clicked.connect(lambda: self.set_active_axis("Y"))
        self.form.radiobutton_z_axis.clicked.connect(lambda: self.set_active_axis("Z"))


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
            axis_values = {
                'X': (self.v_x.x, self.n_x),
                'Y': (self.v_y.y, self.n_y),
                'Z': (self.v_z.z, self.n_z),
            }

            values = axis_values.get(self.active_axis)
            if values is not None:
                interval, num_elements = values
                # Set interval
                params.set_param(f"{self.active_axis}Interval", interval, "Mod/Draft/OrthoArrayLinearMode")
                # Set number of elements
                params.set_param(f"{self.active_axis}NumOfElements", num_elements, "Mod/Draft/OrthoArrayLinearMode")

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
        if self.linear_mode:
            if not (self.form.radiobutton_x_axis.isChecked() or
                    self.form.radiobutton_y_axis.isChecked() or
                    self.form.radiobutton_z_axis.isChecked()):
                _err(translate("draft","In Linear mode, at least one axis must be selected."))
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
        if self.linear_mode:
            start_val = App.Units.Quantity(100.0, App.Units.Length).Value
            if not self.form.radiobutton_x_axis.isChecked():
                self.n_x = 1
                self.v_x = App.Vector(start_val, 0, 0)
            if not self.form.radiobutton_y_axis.isChecked():
                self.n_y = 1
                self.v_y = App.Vector(0, start_val, 0)
            if not self.form.radiobutton_z_axis.isChecked():
                self.n_z = 1
                self.v_z = App.Vector(0, 0, start_val)

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

    def toggle_linear_mode(self):
        """Toggle between Linear mode and Orthogonal mode."""
        self.linear_mode = self.form.button_linear_mode.isChecked()
        self.toggle_axis_radiobuttons(self.linear_mode)
        params.set_param("LinearModeOn" , self.linear_mode, "Mod/Draft/OrthoArrayLinearMode")

        if self.linear_mode:
            self.form.button_linear_mode.setText(translate("draft", "Switch to ortho mode"))

            # check radiobutton based on current cfg
            self.update_axis_ui()

            # For linear mode we're hiding all group boxes for X, Y, Z axis and the one
            # with number of elements as we will reparent those spinboxes under newly
            # created group
            self._set_orthomode_groups_visibility(hide=True)
            self._reparent_groups(mode="Linear")

            # Set the appropriate title for the group (we flip it back and forth after changing mode)
            # and show the group
            self.form.group_linearmode.show()
            self.form.group_linearmode.setTitle(f"{self.active_axis} Axis")
        else: # ortho mode
            self.form.button_linear_mode.setText(translate("draft", "Switch to linear mode"))

            # For ortho mode we're showing back default groupboxes and we reparent everything
            # back to them, as we reuse spinboxes in both modes
            self._set_orthomode_groups_visibility(hide=False)
            self._reparent_groups(mode="Ortho")

            self.form.group_linearmode.hide()

    def toggle_axis_radiobuttons(self, show):
        """Show or hide the axis radio buttons."""
        for radiobutton in [self.form.radiobutton_x_axis,
                        self.form.radiobutton_y_axis,
                        self.form.radiobutton_z_axis]:
            radiobutton.setVisible(show)

    def set_active_axis(self, axis):
        """Set the active axis when a radio button is changed."""
        if self.linear_mode:
            # get current radiobutton that was supposed to have state changed
            radiobutton = getattr(self.form, f"radiobutton_{axis.lower()}_axis")

            # If we're checking a different radio button than the current active axis
            if radiobutton.isChecked() and axis != self.active_axis:
                self.active_axis = axis
                params.set_param("AxisSelected", self.active_axis, "Mod/Draft/OrthoArrayLinearMode")
                self._setup_linear_mode_layout()
                self.form.group_linearmode.setTitle(f"{self.active_axis} Axis")


    def update_axis_ui(self):
        """Update the UI to reflect the current axis selection."""
        # Make sure only one axis is selected
        self.form.radiobutton_x_axis.setChecked(self.active_axis == "X")
        self.form.radiobutton_y_axis.setChecked(self.active_axis == "Y")
        self.form.radiobutton_z_axis.setChecked(self.active_axis == "Z")

    def _get_axis_widgets(self, axis):
        """Get all widgets for a specific axis."""
        return {
            'spinbox_elements': getattr(self.form, f"spinbox_n_{axis}", None),
            'spinbox_interval': getattr(self.form, f"input_{axis}_{axis.lower()}", None),
            'button_reset': getattr(self.form, f"button_reset_{axis}", None)
        }

    def _clear_linear_mode_layout(self):
        """Clear all widgets from the linear mode layout."""
        group_layout = self.form.group_linearmode.layout()

        # Remove all items from the layout
        while group_layout.count():
            item = group_layout.takeAt(0)
            if item.widget():
                item.widget().setParent(None)

    def _setup_linear_mode_layout(self):
        """Set up the Linear mode layout with widgets for the active axis."""
        # Clear existing layout first
        self._clear_linear_mode_layout()

        group_layout = self.form.group_linearmode.layout()

        # Create labels
        label_elements = QtWidgets.QLabel(translate("draft", "Number of elements"))
        label_interval = QtWidgets.QLabel(translate("draft", "Interval"))

        # Get widgets for active axis
        widgets = self._get_axis_widgets(self.active_axis)

        # Add widgets to layout
        widget_pairs = [
            (label_elements, widgets['spinbox_elements']),
            (label_interval, widgets['spinbox_interval'])
        ]

        for row_index, (label, widget) in enumerate(widget_pairs):
            label.setParent(self.form.group_linearmode)
            widget.setParent(self.form.group_linearmode)
            group_layout.addWidget(label, row_index, 0)
            group_layout.addWidget(widget, row_index, 1)

        # Add reset button spanning both columns
        widgets['button_reset'].setParent(self.form.group_linearmode)
        group_layout.addWidget(widgets['button_reset'], 2, 0, 1, 2)

    def _restore_axis_to_ortho_layout(self, axis, row_index):
        """Restore widgets for a specific axis back to their original Ortho layout positions."""
        widgets = self._get_axis_widgets(axis)

        # Restore spinbox elements to grid layout
        grid_number_layout = self.form.findChild(QtWidgets.QGridLayout, "grid_number")
        grid_number_layout.addWidget(widgets['spinbox_elements'], row_index, 1)

        # Restore interval input to its axis-specific inner grid layout
        inner_grid_layout = self.form.findChild(QtWidgets.QGridLayout, f"grid_{axis}")
        inner_grid_layout.addWidget(widgets['spinbox_interval'], row_index, 1)

        # Restore reset button to its axis group
        group_box = self.form.findChild(QtWidgets.QGroupBox, f"group_{axis}")
        group_axis_layout = group_box.layout()
        group_axis_layout.addWidget(widgets['button_reset'], 1, 0)

    def _setup_ortho_mode_layout(self):
        """Restore all widgets back to their original Ortho mode layout positions."""
        for row_index, axis in enumerate(["X", "Y", "Z"]):
            self._restore_axis_to_ortho_layout(axis, row_index)

    def _reparent_groups(self, mode="Linear"):
        """Reparent widgets between Linear and Ortho mode layouts."""
        if mode == "Linear":
            self._setup_linear_mode_layout()
        else:
            self._setup_ortho_mode_layout()

    def _set_orthomode_groups_visibility(self, hide=True):
        """Change visibility of ortho mode groups"""
        for axis in ["X", "Y", "Z"]:
            group_name = f"group_{axis}"
            group_box = self.form.findChild(QtWidgets.QGroupBox, group_name)
            if hide:
                group_box.hide()
                self.form.group_copies.hide()
            else:
                group_box.show()
                self.form.group_copies.show()

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
