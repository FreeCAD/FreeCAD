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
"""Provides the task panel base for task panels that share functionality."""
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc  # include resources, icons, ui files
import DraftVecUtils
import draftutils.utils as utils

from FreeCAD import Units as U
from draftutils.messages import _msg
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class TaskPanelPolarCircularBase:
    """TaskPanel base code for the PolarArray and CircularArray
    command.

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

    def __init__(self, ui_file, icon_name):
        self.form = Gui.PySideUic.loadUi(ui_file)

        svg = ":/icons/" + icon_name
        pix = QtGui.QPixmap(svg)
        icon = QtGui.QIcon.fromTheme(icon_name, QtGui.QIcon(svg))
        self.form.setWindowIcon(icon)
        self.form.setWindowTitle(_tr(self.name))

        self.form.label_icon.setPixmap(pix.scaled(32, 32))

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

        self.form.label_axis_name.setText("")
        self.axis_name = None
        self.edge_index = None

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
        self.form.button_axis.clicked.connect(self.select_axis)
        self.form.button_reset.clicked.connect(self.reset_point)

        # When the checkbox changes, change the internal value
        self.form.checkbox_fuse.stateChanged.connect(self.set_fuse)
        self.form.checkbox_link.stateChanged.connect(self.set_link)

        # Old style for Qt4, avoid!
        # QtCore.QObject.connect(self.form.button_reset,
        #                        QtCore.SIGNAL("clicked()"),
        #                        self.reset_point)
        # QtCore.QObject.connect(self.form.checkbox_fuse,
        #                        QtCore.SIGNAL("stateChanged(int)"),
        #                        self.set_fuse)
        # QtCore.QObject.connect(self.form.checkbox_link,
        #                        QtCore.SIGNAL("stateChanged(int)"),
        #                        self.set_link)

    def get_center(self):
        """Get the value of the center from the widgets."""
        c_x_str = self.form.input_c_x.text()
        c_y_str = self.form.input_c_y.text()
        c_z_str = self.form.input_c_z.text()
        center = App.Vector(U.Quantity(c_x_str).Value,
                            U.Quantity(c_y_str).Value,
                            U.Quantity(c_z_str).Value)
        return center

    def select_axis(self):
        """Execute as callback when the axis button is clicked.

        The axis selection process and button has the three stages
        select (triggers selection), abort (aborts selection),
        discard (set after axis is selected successfully and discards
        the selected and saved axis)
        """
        if self.form.button_axis.text() == "select":
            self.enable_axis_selection()
            return
        elif (self.form.button_axis.text() == "abort"
              or self.form.button_axis.text() == "discard"):
            self.disable_axis_selection()

    def display_axis(self, axis_label, axis_name, edge_name, edge_index):
        """Show the selected axis in the GUI. Add derived values for the center
        coordinates GUI.
        """
        _msg("Selected: {} with "
             "edge {} (index: {})".format(axis_label, edge_name, edge_index))
        self.axis_name = axis_name
        self.edge_index = edge_index
        self.form.label_axis_name.setText("{} {}".format(axis_label, edge_name))
        self.form.button_axis.setText("discard")

    def enable_axis_selection(self):
        """Enable axis selection GUI elements and callbacks"""
        self.form.button_axis.setText("abort")
        self.axis_name = self.edge_index = None
        self.source_command.add_axis_selection_observer()
        self.disable_point()
        _msg(_tr("Selecting axis"))

    def disable_axis_selection(self):
        """Disable axis selection GUI elements and callbacks"""
        self.enable_point()
        self.axis_name = self.edge_index = None
        self.form.button_axis.setText("select")
        self.form.label_axis_name.setText("")
        self.source_command.remove_axis_selection_observer()
        _msg(_tr("Selecting axis disabled"))

    def disable_point(self):
        """Disable point selection GUI elements and callbacks"""
        self.reset_point()
        self.source_command.remove_center_callbacks()
        self.form.input_c_x.setProperty('readOnly', True)
        self.form.input_c_y.setProperty('readOnly', True)
        self.form.input_c_z.setProperty('readOnly', True)

    def enable_point(self):
        """Enable point selection GUI elements and callbacks"""
        self.reset_point()
        self.source_command.remove_center_callbacks()
        self.source_command.add_center_callbacks()
        self.form.input_c_x.setProperty('readOnly', False)
        self.form.input_c_y.setProperty('readOnly', False)
        self.form.input_c_z.setProperty('readOnly', False)

    def reset_point(self):
        """Reset the center point to the original distance."""
        self.form.input_c_x.setProperty('rawValue', 0)
        self.form.input_c_y.setProperty('rawValue', 0)
        self.form.input_c_z.setProperty('rawValue', 0)

        self.center = self.get_center()
        _msg(_tr("Center reset:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))

    def print_fuse_state(self, fuse):
        """Print the fuse state translated."""
        if fuse:
            state = self.tr_true
        else:
            state = self.tr_false
        _msg(_tr("Fuse:") + " {}".format(state))

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
        _msg(_tr("Create Link array:") + " {}".format(state))

    def set_link(self):
        """Execute as a callback when the link checkbox changes."""
        self.use_link = self.form.checkbox_link.isChecked()
        self.print_link_state(self.use_link)
        utils.set_param("Draft_array_Link", self.use_link)

    def display_point(self, point=None, plane=None, mask=None):
        """Display the coordinates in the x, y, and z widgets.

        This function should be used in a Coin callback so that
        the coordinate values are automatically updated when the
        mouse pointer moves.
        This was copied from `DraftGui.py` but needs to be improved
        for this particular command.

        point :
            is a vector that arrives by the callback.
        plane :
            is a `WorkingPlane` instance, for example,
            `App.DraftWorkingPlane`. It is not used at the moment,
            but could be used to set up the grid.
        mask :
            is a string that specifies which coordinate is being
            edited. It is used to restrict edition of a single coordinate.
            It is not used at the moment but could be used with a callback.
        """
        # Get the coordinates to display
        dp = None
        if point:
            dp = point

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
        if dp:
            if self.mask in ('y', 'z'):
                # sbx.setText(displayExternal(dp.x, None, 'Length'))
                self.form.input_c_x.setProperty('rawValue', dp.x)
            else:
                # sbx.setText(displayExternal(dp.x, None, 'Length'))
                self.form.input_c_x.setProperty('rawValue', dp.x)
            if self.mask in ('x', 'z'):
                # sby.setText(displayExternal(dp.y, None, 'Length'))
                self.form.input_c_y.setProperty('rawValue', dp.y)
            else:
                # sby.setText(displayExternal(dp.y, None, 'Length'))
                self.form.input_c_y.setProperty('rawValue', dp.y)
            if self.mask in ('x', 'y'):
                # sbz.setText(displayExternal(dp.z, None, 'Length'))
                self.form.input_c_z.setProperty('rawValue', dp.z)
            else:
                # sbz.setText(displayExternal(dp.z, None, 'Length'))
                self.form.input_c_z.setProperty('rawValue', dp.z)

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
        _msg(_tr("Aborted:") + " {}".format(_tr(self.name)))
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
