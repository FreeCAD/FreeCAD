"""Provide the task panel for the Draft OrthoArray tool."""
## @package task_orthoarray
# \ingroup DRAFT
# \brief Provide the task panel for the Draft OrthoArray tool.

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

import FreeCAD as App
import FreeCADGui as Gui
# import Draft
import Draft_rc
import DraftVecUtils

import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
# import DraftTools
from draftutils.translate import translate
# from DraftGui import displayExternal

_Quantity = App.Units.Quantity


def _Msg(text, end="\n"):
    """Print message with newline."""
    App.Console.PrintMessage(text + end)


def _Wrn(text, end="\n"):
    """Print warning with newline."""
    App.Console.PrintWarning(text + end)


def _tr(text):
    """Translate with the context set."""
    return translate("Draft", text)


# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc else False


class TaskPanelOrthoArray:
    """TaskPanel for the OrthoArray command.

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
        ui_file = ":/ui/TaskPanel_OrthoArray.ui"
        self.form = Gui.PySideUic.loadUi(ui_file)
        self.name = self.form.windowTitle()

        icon_name = "Draft_Array"
        svg = ":/icons/" + icon_name
        pix = QtGui.QPixmap(svg)
        icon = QtGui.QIcon.fromTheme(icon_name, QtGui.QIcon(svg))
        self.form.setWindowIcon(icon)
        self.form.label_icon.setPixmap(pix.scaled(32, 32))

        start_x = _Quantity(100.0, App.Units.Length)
        start_y = start_x
        start_z = start_x
        start_zero = _Quantity(0.0, App.Units.Length)
        length_unit = start_x.getUserPreferred()[2]

        self.form.input_X_x.setProperty('rawValue', start_x.Value)
        self.form.input_X_x.setProperty('unit', length_unit)
        self.form.input_X_y.setProperty('rawValue', start_zero.Value)
        self.form.input_X_y.setProperty('unit', length_unit)
        self.form.input_X_z.setProperty('rawValue', start_zero.Value)
        self.form.input_X_z.setProperty('unit', length_unit)

        self.form.input_Y_x.setProperty('rawValue', start_zero.Value)
        self.form.input_Y_x.setProperty('unit', length_unit)
        self.form.input_Y_y.setProperty('rawValue', start_y.Value)
        self.form.input_Y_y.setProperty('unit', length_unit)
        self.form.input_Y_z.setProperty('rawValue', start_zero.Value)
        self.form.input_Y_z.setProperty('unit', length_unit)

        self.form.input_Z_x.setProperty('rawValue', start_zero.Value)
        self.form.input_Z_x.setProperty('unit', length_unit)
        self.form.input_Z_y.setProperty('rawValue', start_zero.Value)
        self.form.input_Z_y.setProperty('unit', length_unit)
        self.form.input_Z_z.setProperty('rawValue', start_z.Value)
        self.form.input_Z_z.setProperty('unit', length_unit)

        self.v_X = App.Vector(100, 0, 0)
        self.v_Y = App.Vector(0, 100, 0)
        self.v_Z = App.Vector(0, 0, 100)

        # Old style for Qt4, avoid!
        # QtCore.QObject.connect(self.form.button_reset,
        #                        QtCore.SIGNAL("clicked()"),
        #                        self.reset_point)
        # New style for Qt5
        self.form.button_reset_X.clicked.connect(lambda: self.reset_v("X"))
        self.form.button_reset_Y.clicked.connect(lambda: self.reset_v("Y"))
        self.form.button_reset_Z.clicked.connect(lambda: self.reset_v("Z"))

        self.n_X = 2
        self.n_Y = 2
        self.n_Z = 1

        self.form.spinbox_n_X.setValue(self.n_X)
        self.form.spinbox_n_Y.setValue(self.n_Y)
        self.form.spinbox_n_Z.setValue(self.n_Z)

        self.valid_input = False

        # When the checkbox changes, change the fuse value
        self.fuse = False
        self.form.checkbox_fuse.stateChanged.connect(self.set_fuse)

        self.use_link = False
        self.form.checkbox_link.stateChanged.connect(self.set_link)

    def accept(self):
        """Execute when clicking the OK button."""
        selection = Gui.Selection.getSelection()
        n_X = self.form.spinbox_n_X.value()
        n_Y = self.form.spinbox_n_Y.value()
        n_Z = self.form.spinbox_n_Z.value()
        self.valid_input = self.validate_input(selection,
                                               n_X,
                                               n_Y,
                                               n_Z)
        if self.valid_input:
            self.create_object(selection)
            self.print_messages(selection)
            self.finish()

    def validate_input(self, selection, n_X, n_Y, n_Z):
        """Check that the input is valid."""
        if not selection:
            _Wrn(_tr("At least one element must be selected"))
            return False
        if n_X < 1 or n_Y < 1 or n_Z < 1:
            _Wrn(_tr("Number of elements must be at least 1"))
            return False
        # Todo: each of the elements of the selection could be tested,
        # not only the first one.
        obj = selection[0]
        if obj.isDerivedFrom("App::FeaturePython"):
            _Wrn(_tr("Selection is not suitable for array"))
            _Wrn(_tr("Object:") + " {0} ({1})".format(obj.Label, obj.TypeId))
            return False
        return True

    def create_object(self, selection):
        """Create the actual object."""
        self.v_X, self.v_Y, self.v_Z = self.set_intervals()
        self.n_X, self.n_Y, self.n_Z = self.set_numbers()

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
        #                       self.v_X, self.v_Y, self.v_Z,
        #                       self.n_X, self.n_Y, self.n_Z)
        # if obj:
        #     obj.Fuse = self.fuse

        # Instead, we build the commands to execute through the parent
        # of this class, the GuiCommand.
        # This is needed to schedule geometry manipulation
        # that would crash Coin3D if done in the event callback.
        _cmd = "obj = Draft.makeArray("
        _cmd += "FreeCAD.ActiveDocument." + sel_obj.Name + ", "
        _cmd += "arg1=" + DraftVecUtils.toString(self.v_X) + ", "
        _cmd += "arg2=" + DraftVecUtils.toString(self.v_Y) + ", "
        _cmd += "arg3=" + DraftVecUtils.toString(self.v_Z) + ", "
        _cmd += "arg4=" + str(self.n_X) + ", "
        _cmd += "arg5=" + str(self.n_Y) + ", "
        _cmd += "arg6=" + str(self.n_Z) + ", "
        _cmd += "use_link=" + str(self.use_link)
        _cmd += ")"

        _cmd_list = ["FreeCADGui.addModule('Draft')",
                     _cmd,
                     "obj.Fuse = " + str(self.fuse),
                     "Draft.autogroup(obj)",
                     "FreeCAD.ActiveDocument.recompute()"]
        self.source_command.commit("Ortho array", _cmd_list)

    def set_numbers(self):
        """Assign the number of elements."""
        self.n_X = self.form.spinbox_n_X.value()
        self.n_Y = self.form.spinbox_n_Y.value()
        self.n_Z = self.form.spinbox_n_Z.value()
        return self.n_X, self.n_Y, self.n_Z

    def set_intervals(self):
        """Assign the interval vectors."""
        v_X_x_str = self.form.input_X_x.text()
        v_X_y_str = self.form.input_X_y.text()
        v_X_z_str = self.form.input_X_z.text()
        self.v_X = App.Vector(_Quantity(v_X_x_str).Value,
                              _Quantity(v_X_y_str).Value,
                              _Quantity(v_X_z_str).Value)

        v_Y_x_str = self.form.input_Y_x.text()
        v_Y_y_str = self.form.input_Y_y.text()
        v_Y_z_str = self.form.input_Y_z.text()
        self.v_Y = App.Vector(_Quantity(v_Y_x_str).Value,
                              _Quantity(v_Y_y_str).Value,
                              _Quantity(v_Y_z_str).Value)

        v_Z_x_str = self.form.input_Z_x.text()
        v_Z_y_str = self.form.input_Z_y.text()
        v_Z_z_str = self.form.input_Z_z.text()
        self.v_Z = App.Vector(_Quantity(v_Z_x_str).Value,
                              _Quantity(v_Z_y_str).Value,
                              _Quantity(v_Z_z_str).Value)
        return self.v_X, self.v_Y, self.v_Z

    def reset_v(self, interval):
        """Reset the interval to zero distance."""
        if interval == "X":
            self.form.input_X_x.setProperty('rawValue', 100)
            self.form.input_X_y.setProperty('rawValue', 0)
            self.form.input_X_z.setProperty('rawValue', 0)
            _Msg(_tr("Interval X reset:")
                 + " ({0}, {1}, {2})".format(self.v_X.x,
                                             self.v_X.y,
                                             self.v_X.z))
        elif interval == "Y":
            self.form.input_Y_x.setProperty('rawValue', 0)
            self.form.input_Y_y.setProperty('rawValue', 100)
            self.form.input_Y_z.setProperty('rawValue', 0)
            _Msg(_tr("Interval Y reset:")
                 + " ({0}, {1}, {2})".format(self.v_Y.x,
                                             self.v_Y.y,
                                             self.v_Y.z))
        elif interval == "Z":
            self.form.input_Z_x.setProperty('rawValue', 0)
            self.form.input_Z_y.setProperty('rawValue', 0)
            self.form.input_Z_z.setProperty('rawValue', 100)
            _Msg(_tr("Interval Z reset:")
                 + " ({0}, {1}, {2})".format(self.v_Z.x,
                                             self.v_Z.y,
                                             self.v_Z.z))

        self.n_X, self.n_Y, self.n_Z = self.set_intervals()

    def print_fuse_state(self):
        """Print the state translated."""
        if self.fuse:
            translated_state = QT_TRANSLATE_NOOP("Draft", "True")
        else:
            translated_state = QT_TRANSLATE_NOOP("Draft", "False")
        _Msg(_tr("Fuse:") + " {}".format(translated_state))

    def set_fuse(self):
        """Run callback when the fuse checkbox changes."""
        self.fuse = self.form.checkbox_fuse.isChecked()
        self.print_fuse_state()

    def print_link_state(self):
        """Print the state translated."""
        if self.use_link:
            translated_state = QT_TRANSLATE_NOOP("Draft", "True")
        else:
            translated_state = QT_TRANSLATE_NOOP("Draft", "False")
        _Msg(_tr("Use Link object:") + " {}".format(translated_state))

    def set_link(self):
        """Run callback when the link checkbox changes."""
        self.use_link = self.form.checkbox_link.isChecked()
        self.print_link_state()

    def print_messages(self, selection):
        """Print messages about the operation."""
        if len(selection) == 1:
            sel_obj = selection[0]
        else:
            # This can be changed so a compound of multiple
            # selected objects is produced
            sel_obj = selection[0]
        _Msg("{}".format(16*"-"))
        _Msg("{}".format(self.name))
        _Msg(_tr("Object:") + " {}".format(sel_obj.Label))
        _Msg(_tr("Number of X elements:") + " {}".format(self.n_X))
        _Msg(_tr("Interval X:")
             + " ({0}, {1}, {2})".format(self.v_X.x,
                                         self.v_X.y,
                                         self.v_X.z))
        _Msg(_tr("Number of Y elements:") + " {}".format(self.n_Y))
        _Msg(_tr("Interval Y:")
             + " ({0}, {1}, {2})".format(self.v_Y.x,
                                         self.v_Y.y,
                                         self.v_Y.z))
        _Msg(_tr("Number of Z elements:") + " {}".format(self.n_Z))
        _Msg(_tr("Interval Z:")
             + " ({0}, {1}, {2})".format(self.v_Z.x,
                                         self.v_Z.y,
                                         self.v_Z.z))
        self.print_fuse_state()
        self.print_link_state()

    def reject(self):
        """Run when clicking the Cancel button."""
        _Msg(_tr("Aborted:") + " {}".format(self.name))
        self.finish()

    def finish(self):
        """Run at the end after OK or Cancel."""
        # App.ActiveDocument.commitTransaction()
        Gui.ActiveDocument.resetEdit()
        # Runs the parent command to complete the call
        self.source_command.completed()
