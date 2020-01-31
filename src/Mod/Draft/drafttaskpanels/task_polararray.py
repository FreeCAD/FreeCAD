"""This module provides the task panel for the Draft PolarArray tool.
"""
## @package task_polararray
# \ingroup DRAFT
# \brief This module provides the task panel code for the PolarArray tool.

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

import FreeCAD as App
import FreeCADGui as Gui
# import Draft
import Draft_rc
import DraftVecUtils

import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
# import DraftTools
from DraftGui import translate
# from DraftGui import displayExternal

_Quantity = App.Units.Quantity


def _Msg(text, end="\n"):
    """Print message with newline"""
    App.Console.PrintMessage(text + end)


def _Wrn(text, end="\n"):
    """Print warning with newline"""
    App.Console.PrintWarning(text + end)


def _tr(text):
    """Function to translate with the context set"""
    return translate("Draft", text)


# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class TaskPanelPolarArray:
    """TaskPanel for the PolarArray command.

    The names of the widgets are defined in the `.ui` file.
    In this class all those widgets are automatically created
    under the name `self.form.<name>`

    The `.ui` file may use special FreeCAD widgets such as
    `Gui::InputField` (based on `QLineEdit`) and
    `Gui::QuantitySpinBox` (based on `QAbstractSpinBox`).
    See the Doxygen documentation of the corresponding files in `src/Gui/`,
    for example, `InputField.h` and `QuantitySpinBox.h`.
    """

    def __init__(self):
        ui_file = ":/ui/TaskPanel_PolarArray.ui"
        self.form = Gui.PySideUic.loadUi(ui_file)
        self.name = self.form.windowTitle()

        icon_name = "Draft_PolarArray"
        svg = ":/icons/" + icon_name
        pix = QtGui.QPixmap(svg)
        icon = QtGui.QIcon.fromTheme(icon_name, QtGui.QIcon(svg))
        self.form.setWindowIcon(icon)
        self.form.label_icon.setPixmap(pix.scaled(32, 32))

        start_angle = _Quantity(180.0, App.Units.Angle)
        angle_unit = start_angle.getUserPreferred()[2]
        self.form.spinbox_angle.setProperty('rawValue', start_angle.Value)
        self.form.spinbox_angle.setProperty('unit', angle_unit)
        self.form.spinbox_number.setValue(4)

        self.angle_str = self.form.spinbox_angle.text()
        self.angle = start_angle.Value

        self.number = self.form.spinbox_number.value()

        start_point = _Quantity(0.0, App.Units.Length)
        length_unit = start_point.getUserPreferred()[2]
        self.form.input_c_x.setProperty('rawValue', start_point.Value)
        self.form.input_c_x.setProperty('unit', length_unit)
        self.form.input_c_y.setProperty('rawValue', start_point.Value)
        self.form.input_c_y.setProperty('unit', length_unit)
        self.form.input_c_z.setProperty('rawValue', start_point.Value)
        self.form.input_c_z.setProperty('unit', length_unit)
        self.valid_input = True

        self.c_x_str = ""
        self.c_y_str = ""
        self.c_z_str = ""
        self.center = App.Vector(0, 0, 0)

        # Old style for Qt4
        # QtCore.QObject.connect(self.form.button_reset,
        #                        QtCore.SIGNAL("clicked()"),
        #                        self.reset_point)
        # New style for Qt5
        self.form.button_reset.clicked.connect(self.reset_point)

        # The mask is not used at the moment, but could be used in the future
        # by a callback to restrict the coordinates of the pointer.
        self.mask = ""

        # When the checkbox changes, change the fuse value
        self.fuse = False
        QtCore.QObject.connect(self.form.checkbox_fuse,
                               QtCore.SIGNAL("stateChanged(int)"),
                               self.set_fuse)

        self.use_link = False
        QtCore.QObject.connect(self.form.checkbox_link,
                               QtCore.SIGNAL("stateChanged(int)"),
                               self.set_link)

    def accept(self):
        """Function that executes when clicking the OK button"""
        selection = Gui.Selection.getSelection()
        self.number = self.form.spinbox_number.value()
        self.valid_input = self.validate_input(selection,
                                               self.number)
        if self.valid_input:
            self.create_object(selection)
            self.print_messages(selection)
            self.finish()

    def validate_input(self, selection, number):
        """Check that the input is valid"""
        if not selection:
            _Wrn(_tr("At least one element must be selected"))
            return False
        if number < 2:
            _Wrn(_tr("Number of elements must be at least 2"))
            return False
        # Todo: each of the elements of the selection could be tested,
        # not only the first one.
        if selection[0].isDerivedFrom("App::FeaturePython"):
            _Wrn(_tr("Selection is not suitable for array"))
            _Wrn(_tr("Object:") + " {}".format(selection[0].Label))
            return False
        return True

    def create_object(self, selection):
        """Create the actual object"""
        self.angle_str = self.form.spinbox_angle.text()
        self.angle = _Quantity(self.angle_str).Value

        self.center = self.set_point()

        if len(selection) == 1:
            sel_obj = selection[0]
        else:
            # This can be changed so a compound of multiple
            # selected objects is produced
            sel_obj = selection[0]

        self.fuse = self.form.checkbox_fuse.isChecked()
        self.use_link = self.form.checkbox_link.isChecked()

        # This creates the object immediately
        # obj = Draft.makeArray(sel_obj,
        #                       self.center, self.angle, self.number)
        # if obj:
        #     obj.Fuse = self.fuse

        # Instead, we build the commands to execute through the parent
        # of this class, the GuiCommand.
        # This is needed to schedule geometry manipulation
        # that would crash Coin3D if done in the event callback.
        _cmd = "obj = Draft.makeArray("
        _cmd += "FreeCAD.ActiveDocument." + sel_obj.Name + ", "
        _cmd += "arg1=" + DraftVecUtils.toString(self.center) + ", "
        _cmd += "arg2=" + str(self.angle) + ", "
        _cmd += "arg3=" + str(self.number) + ", "
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        _cmd_list = ["FreeCADGui.addModule('Draft')",
                     _cmd,
                     "obj.Fuse = " + str(self.fuse),
                     "Draft.autogroup(obj)",
                     "FreeCAD.ActiveDocument.recompute()"]
        self.source_command.commit("Polar array", _cmd_list)

    def set_point(self):
        """Assign the values to the center"""
        self.c_x_str = self.form.input_c_x.text()
        self.c_y_str = self.form.input_c_y.text()
        self.c_z_str = self.form.input_c_z.text()
        center = App.Vector(_Quantity(self.c_x_str).Value,
                            _Quantity(self.c_y_str).Value,
                            _Quantity(self.c_z_str).Value)
        return center

    def reset_point(self):
        """Reset the point to the original distance"""
        self.form.input_c_x.setProperty('rawValue', 0)
        self.form.input_c_y.setProperty('rawValue', 0)
        self.form.input_c_z.setProperty('rawValue', 0)

        self.center = self.set_point()
        _Msg(_tr("Center reset:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))

    def print_fuse_state(self):
        """Print the state translated"""
        if self.fuse:
            translated_state = QT_TRANSLATE_NOOP("Draft", "True")
        else:
            translated_state = QT_TRANSLATE_NOOP("Draft", "False")
        _Msg(_tr("Fuse:") + " {}".format(translated_state))

    def set_fuse(self):
        """This function is called when the fuse checkbox changes"""
        self.fuse = self.form.checkbox_fuse.isChecked()
        self.print_fuse_state()

    def print_link_state(self):
        """Print the state translated"""
        if self.use_link:
            translated_state = QT_TRANSLATE_NOOP("Draft", "True")
        else:
            translated_state = QT_TRANSLATE_NOOP("Draft", "False")
        _Msg(_tr("Use Link object:") + " {}".format(translated_state))

    def set_link(self):
        """This function is called when the fuse checkbox changes"""
        self.use_link = self.form.checkbox_link.isChecked()
        self.print_link_state()

    def print_messages(self, selection):
        """Print messages about the operation"""
        if len(selection) == 1:
            sel_obj = selection[0]
        else:
            # This can be changed so a compound of multiple
            # selected objects is produced
            sel_obj = selection[0]
        _Msg("{}".format(16*"-"))
        _Msg("{}".format(self.name))
        _Msg(_tr("Object:") + " {}".format(sel_obj.Label))
        _Msg(_tr("Start angle:") + " {}".format(self.angle_str))
        _Msg(_tr("Number of elements:") + " {}".format(self.number))
        _Msg(_tr("Center of rotation:")
             + " ({0}, {1}, {2})".format(self.center.x,
                                         self.center.y,
                                         self.center.z))
        self.print_fuse_state()
        self.print_link_state()

    def display_point(self, point=None, plane=None, mask=None):
        """Displays the coordinates in the x, y, and z widgets.

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
        """Set the focus on the widget that receives the key signal"""
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
        """Function that executes when clicking the Cancel button"""
        _Msg(_tr("Aborted:") + " {}".format(self.name))
        self.finish()

    def finish(self):
        """Function that runs at the end after OK or Cancel"""
        # App.ActiveDocument.commitTransaction()
        Gui.ActiveDocument.resetEdit()
        # Runs the parent command to complete the call
        self.source_command.completed()
