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
import draftutils.utils as utils

from FreeCAD import Units as U
from draftutils.messages import _msg, _err, _log
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

        self.fuse = utils.get_param("Draft_array_fuse", False)
        self.use_link = utils.get_param("Draft_array_Link", True)

        self.form.checkbox_fuse.setChecked(self.fuse)
        self.form.checkbox_link.setChecked(self.use_link)
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
            # self.print_messages()
            self.finish()

    def validate_input(self, selection,
                       v_x, v_y, v_z,
                       n_x, n_y, n_z):
        """Check that the input is valid.

        Some values may not need to be checked because
        the interface may not allow to input wrong data.
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
            _msg(translate("draft","Interval X reset:")
                 + " ({0}, {1}, {2})".format(self.v_x.x,
                                             self.v_x.y,
                                             self.v_x.z))
        elif interval == "Y":
            self.form.input_Y_x.setProperty('rawValue', 0)
            self.form.input_Y_y.setProperty('rawValue', 100)
            self.form.input_Y_z.setProperty('rawValue', 0)
            self.v_x, self.v_y, self.v_z = self.get_intervals()
            _msg(translate("draft","Interval Y reset:")
                 + " ({0}, {1}, {2})".format(self.v_y.x,
                                             self.v_y.y,
                                             self.v_y.z))
        elif interval == "Z":
            self.form.input_Z_x.setProperty('rawValue', 0)
            self.form.input_Z_y.setProperty('rawValue', 0)
            self.form.input_Z_z.setProperty('rawValue', 100)
            self.v_x, self.v_y, self.v_z = self.get_intervals()
            _msg(translate("draft","Interval Z reset:")
                 + " ({0}, {1}, {2})".format(self.v_z.x,
                                             self.v_z.y,
                                             self.v_z.z))

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
        self.print_fuse_state(self.fuse)
        utils.set_param("Draft_array_fuse", self.fuse)

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
        self.print_link_state(self.use_link)
        utils.set_param("Draft_array_Link", self.use_link)

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
        _msg(translate("draft","Aborted:") + " {}".format(translate("draft","Orthogonal array")))
        self.finish()

    def finish(self):
        """Finish the command, after accept or reject.

        It finally calls the parent class to execute
        the delayed functions, and perform cleanup.
        """
        # App.ActiveDocument.commitTransaction()
        Gui.ActiveDocument.resetEdit()
        # Runs the parent command to complete the call
        self.source_command.completed()

## @}
