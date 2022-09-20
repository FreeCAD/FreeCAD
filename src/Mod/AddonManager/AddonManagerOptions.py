# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Contains a the Addon Manager's preferences dialog management class """

import os

import FreeCAD
import FreeCADGui

from PySide2 import QtCore
from PySide2.QtWidgets import (
    QWidget,
    QCheckBox,
    QComboBox,
    QRadioButton,
    QLineEdit,
    QTextEdit,
)


class AddonManagerOptions:
    """ A class containing a form element that is inserted as a FreeCAD preference page. """

    def __init__(self, _=None):
        self.form = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManagerOptions.ui")
        )

    def saveSettings(self):
        """Required function: called by the preferences dialog when Apply or Save is clicked,
        saves out the preference data by reading it from the widgets."""
        for widget in self.form.children():
            self.recursive_widget_saver(widget)

    def recursive_widget_saver(self, widget):
        """Writes out the data for this widget and all of its children, recursively."""
        if isinstance(widget, QWidget):
            # See if it's one of ours:
            pref_path = widget.property("prefPath")
            pref_entry = widget.property("prefEntry")
            if pref_path and pref_entry:
                pref_access_string = (
                    f"User parameter:BaseApp/Preferences/{str(pref_path,'utf-8')}"
                )
                pref = FreeCAD.ParamGet(pref_access_string)
                if isinstance(widget, QCheckBox):
                    checked = widget.isChecked()
                    pref.SetBool(str(pref_entry, "utf-8"), checked)
                elif isinstance(widget, QRadioButton):
                    checked = widget.isChecked()
                    pref.SetBool(str(pref_entry, "utf-8"), checked)
                elif isinstance(widget, QComboBox):
                    new_index = widget.currentIndex()
                    pref.SetInt(str(pref_entry, "utf-8"), new_index)
                elif isinstance(widget, QTextEdit):
                    text = widget.toPlainText()
                    pref.SetString(str(pref_entry, "utf-8"), text)
                elif isinstance(widget, QLineEdit):
                    text = widget.text()
                    pref.SetString(str(pref_entry, "utf-8"), text)
                elif widget.metaObject().className() == "Gui::PrefFileChooser":
                    filename = str(widget.property("fileName"), "utf-8")
                    filename = pref.SetString(str(pref_entry, "utf-8"), filename)

        # Recurse over children
        if isinstance(widget, QtCore.QObject):
            for child in widget.children():
                self.recursive_widget_saver(child)

    def loadSettings(self):
        """Required function: called by the preferences dialog when it is launched,
        loads the preference data and assigns it to the widgets."""
        for widget in self.form.children():
            self.recursive_widget_loader(widget)

    def recursive_widget_loader(self, widget):
        """Loads the data for this widget and all of its children, recursively."""
        if isinstance(widget, QWidget):
            # See if it's one of ours:
            pref_path = widget.property("prefPath")
            pref_entry = widget.property("prefEntry")
            if pref_path and pref_entry:
                pref_access_string = (
                    f"User parameter:BaseApp/Preferences/{str(pref_path,'utf-8')}"
                )
                pref = FreeCAD.ParamGet(pref_access_string)
                if isinstance(widget, QCheckBox):
                    widget.setChecked(pref.GetBool(str(pref_entry, "utf-8")))
                elif isinstance(widget, QRadioButton):
                    if pref.GetBool(str(pref_entry, "utf-8")):
                        widget.setChecked(True)
                elif isinstance(widget, QComboBox):
                    new_index = pref.GetInt(str(pref_entry, "utf-8"))
                    widget.setCurrentIndex(new_index)
                elif isinstance(widget, QTextEdit):
                    text = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setText(text)
                elif isinstance(widget, QLineEdit):
                    text = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setText(text)
                elif widget.metaObject().className() == "Gui::PrefFileChooser":
                    filename = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setProperty("fileName", filename)

        # Recurse over children
        if isinstance(widget, QtCore.QObject):
            for child in widget.children():
                self.recursive_widget_loader(child)
