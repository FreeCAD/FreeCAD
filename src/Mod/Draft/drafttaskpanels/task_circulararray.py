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
# \ingroup drafttaskpanels
# \brief Provides the task panel code for the Draft CircularArray tool.

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
from draftutils.messages import _msg, _wrn, _err, _log
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class TaskPanelCircularArray:
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
    * https://forum.freecad.org/viewtopic.php?f=10&t=40007
    * https://forum.freecad.org/viewtopic.php?t=5374#p43038
    """

    def __init__(self):
        self.name = "Circular array"
        _log(translate("draft","Task panel:") + " {}".format(translate("draft","Circular array")))

        # The .ui file must be loaded into an attribute
        # called `self.form` so that it is displayed in the task panel.
        ui_file = ":/ui/TaskPanel_CircularArray.ui"
        self.form = Gui.PySideUic.loadUi(ui_file)

        icon_name = "Draft_CircularArray"
        svg = ":/icons/" + icon_name
        pix = QtGui.QPixmap(svg)
        icon = QtGui.QIcon.fromTheme(icon_name, QtGui.QIcon(svg))
        self.form.setWindowIcon(icon)
        self.form.setWindowTitle(translate("draft","Circular array"))

        self.form.label_icon.setPixmap(pix.scaled(32, 32))

        # -------------------------------------------------------------------
        # Default values for the internal function,
        # and for the task panel interface
        start_distance = U.Quantity(50.0, App.Units.Length)
        length_unit = start_distance.getUserPreferred()[2]

        self.r_distance = 2 * start_distance.Value
        self.tan_distance = start_distance.Value

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

        start_point = U.Quantity(0.0, App.Units.Length)
        length_unit = start_point.getUserPreferred()[2]

        self.center = App.Vector(start_point.Value,
                                 start_point.Value,
                                 start_point.Value)

        self.form.input_c_x.setProperty('rawValue', self.center.x)
        self.form.input_c_x.setProperty('unit', length_unit)
        self.form.input_c_y.setProperty('rawValue', self.center.y)
        self.form.input_c_y.setProperty('unit', length_unit)
        self.form.input_c_z.setProperty('rawValue', self.center.z)
        self.form.input_c_z.setProperty('unit', length_unit)

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

        # The mask is not used at the moment, but could be used in the future
        # by a callback to restrict the coordinates of the pointer.
        self.mask = ""

    def set_widget_callbacks(self):
        """Set up the callbacks (slots) for the widget signals."""
        # New style for Qt5
        self.form.button_reset.clicked.connect(self.reset_point)

        # When the checkbox changes, change the internal value
        self.form.checkbox_fuse.stateChanged.connect(self.set_fuse)
        self.form.checkbox_link.stateChanged.connect(self.set_link)


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
            _err(translate("draft","At least one element must be selected."))
            return False

        if number < 2:
            _err(translate("draft","Number of layers must be at least 2."))
            return False

        # TODO: this should handle multiple objects.
        # Each of the elements of the selection should be tested.
        obj = selection[0]
        if obj.isDerivedFrom("App::FeaturePython"):
            _err(translate("draft","Selection is not suitable for array."))
            _err(translate("draft","Object:") + " {}".format(selection[0].Label))
            return False

        if r_distance == 0:
            _wrn(translate("draft","Radial distance is zero. Resulting array may not look correct."))
        elif r_distance < 0:
            _wrn(translate("draft","Radial distance is negative. It is made positive to proceed."))
            self.r_distance = abs(r_distance)

        if tan_distance == 0:
            _err(translate("draft","Tangential distance cannot be zero."))
            return False
        elif tan_distance < 0:
            _wrn(translate("draft","Tangential distance is negative. It is made positive to proceed."))
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
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        Gui.addModule('Draft')

        _cmd_list = ["_obj_ = " + _cmd,
                     "_obj_.Fuse = " + str(self.fuse),
                     "Draft.autogroup(_obj_)",
                     "App.ActiveDocument.recompute()"]

        # We commit the command list through the parent command
        self.source_command.commit(translate("draft","Circular array"), _cmd_list)

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

    def get_center(self):
        """Get the value of the center from the widgets."""
        c_x_str = self.form.input_c_x.text()
        c_y_str = self.form.input_c_y.text()
        c_z_str = self.form.input_c_z.text()
        center = App.Vector(U.Quantity(c_x_str).Value,
                            U.Quantity(c_y_str).Value,
                            U.Quantity(c_z_str).Value)
        return center

    def get_axis(self):
        """Get the axis that will be used for the array. NOT IMPLEMENTED.

        It should consider a second selection of an edge or wire to use
        as an axis.
        """
        return self.axis

    def reset_point(self):
        """Reset the center point to the original distance."""
        self.form.input_c_x.setProperty('rawValue', 0)
        self.form.input_c_y.setProperty('rawValue', 0)
        self.form.input_c_z.setProperty('rawValue', 0)

        self.center = self.get_center()
        _msg(translate("draft","Center reset:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))

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
        _msg(translate("draft","Radial distance:") + " {}".format(self.r_distance))
        _msg(translate("draft","Tangential distance:") + " {}".format(self.tan_distance))
        _msg(translate("draft","Number of circular layers:") + " {}".format(self.number))
        _msg(translate("draft","Symmetry parameter:") + " {}".format(self.symmetry))
        _msg(translate("draft","Center of rotation:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))
        self.print_fuse_state(self.fuse)
        self.print_link_state(self.use_link)

    def display_point(self, point=None, plane=None, mask=None):
        """Display the coordinates in the x, y, and z widgets.

        This function should be used in a Coin callback so that
        the coordinate values are automatically updated when the
        mouse pointer moves.
        This was copied from `DraftGui.py` but needs to be improved
        for this particular command.

        point: Base::Vector3
            is a vector that arrives by the callback.
        plane: WorkingPlane
            is a `WorkingPlane` instance, for example,
            `App.DraftWorkingPlane`. It is not used at the moment,
            but could be used to set up the grid.
        mask: str
            is a string that specifies which coordinate is being
            edited. It is used to restrict edition of a single coordinate.
            It is not used at the moment but could be used with a callback.
        """
        # Get the coordinates to display
        d_p = None
        if point:
            d_p = point

        # Set the widgets to the value of the mouse pointer.
        #
        # setProperty() is used if the widget is a FreeCAD widget like
        # Gui::InputField or Gui::QuantitySpinBox, which are based on
        # QLineEdit and QAbstractSpinBox.
        #
        # setText() is used to set the text inside the widget, this may be
        # useful in some circumstances.
        #
        # The mask allows editing only one field, that is, only one coordinate.
        # sbx = self.form.spinbox_c_x
        # sby = self.form.spinbox_c_y
        # sbz = self.form.spinbox_c_z
        if d_p:
            if self.mask in ('y', 'z'):
                # sbx.setText(displayExternal(d_p.x, None, 'Length'))
                self.form.input_c_x.setProperty('rawValue', d_p.x)
            else:
                # sbx.setText(displayExternal(d_p.x, None, 'Length'))
                self.form.input_c_x.setProperty('rawValue', d_p.x)
            if self.mask in ('x', 'z'):
                # sby.setText(displayExternal(d_p.y, None, 'Length'))
                self.form.input_c_y.setProperty('rawValue', d_p.y)
            else:
                # sby.setText(displayExternal(d_p.y, None, 'Length'))
                self.form.input_c_y.setProperty('rawValue', d_p.y)
            if self.mask in ('x', 'y'):
                # sbz.setText(displayExternal(d_p.z, None, 'Length'))
                self.form.input_c_z.setProperty('rawValue', d_p.z)
            else:
                # sbz.setText(displayExternal(d_p.z, None, 'Length'))
                self.form.input_c_z.setProperty('rawValue', d_p.z)

        if plane:
            pass

        # Set masks
        if (mask == "x") or (self.mask == "x"):
            self.form.input_c_x.setEnabled(True)
            self.form.input_c_y.setEnabled(False)
            self.form.input_c_z.setEnabled(False)
            self.set_focus("x")
        elif (mask == "y") or (self.mask == "y"):
            self.form.input_c_x.setEnabled(False)
            self.form.input_c_y.setEnabled(True)
            self.form.input_c_z.setEnabled(False)
            self.set_focus("y")
        elif (mask == "z") or (self.mask == "z"):
            self.form.input_c_x.setEnabled(False)
            self.form.input_c_y.setEnabled(False)
            self.form.input_c_z.setEnabled(True)
            self.set_focus("z")
        else:
            self.form.input_c_x.setEnabled(True)
            self.form.input_c_y.setEnabled(True)
            self.form.input_c_z.setEnabled(True)
            self.set_focus()

    def set_focus(self, key=None):
        """Set the focus on the widget that receives the key signal."""
        if key is None or key == "x":
            self.form.input_c_x.setFocus()
            self.form.input_c_x.selectAll()
        elif key == "y":
            self.form.input_c_y.setFocus()
            self.form.input_c_y.selectAll()
        elif key == "z":
            self.form.input_c_z.setFocus()
            self.form.input_c_z.selectAll()

    def reject(self):
        """Execute when clicking the Cancel button or pressing Escape."""
        _msg(translate("draft","Aborted:") + " {}".format(translate("draft","Circular array")))
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
