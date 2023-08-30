# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to create and edit annotation styles."""
## @package gui_annotationstyleeditor
# \ingroup draftguitools
# \brief Provides GUI tools to create and edit annotation styles.

## \addtogroup draftguitools
# @{
import json
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import draftguitools.gui_base as gui_base

from draftutils.translate import translate
from draftutils import utils
from draftutils.utils import ANNOTATION_STYLE as DEFAULT

param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")


class AnnotationStyleEditor(gui_base.GuiCommandSimplest):
    """Annotation style editor for text and dimensions.

    It inherits `GuiCommandSimplest` to set up the document,
    `IsActive`, and other behavior. See this class for more information.

    Attributes
    ----------
    doc: App::Document
        The active document when the command is used, so that the styles
        are saved to this document.

    styles: dict
        A dictionary with key-value pairs that define the new style.

    renamed: dict
        A dictionary that holds the name of the style that is renamed
        by the editor.

    form: PySide.QtWidgets.QDialog
        Holds the loaded interface from the `.ui` file.
    """

    def __init__(self):
        super(AnnotationStyleEditor, self).__init__(name=translate("draft","Annotation style editor"))
        self.doc = None
        self.styles = {}
        self.renamed = {}
        self.current_style = None
        self.form = None

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': ":icons/Draft_Annotation_Style.svg",
                'MenuText': QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor",
                                              "Annotation styles..."),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor",
                                             "Manage or create annotation styles")}

    def Activated(self):
        """Execute when the command is called.

        The document attribute is set here by the parent class.
        """
        super(AnnotationStyleEditor, self).Activated()
        # reset rename table and current style
        self.renamed = {}
        self.current_style = None

        # load dialog
        ui_file = ":/ui/dialog_AnnotationStyleEditor.ui"
        self.form = Gui.PySideUic.loadUi(ui_file)

        # restore stored size
        w = param.GetInt("AnnotationStyleEditorWidth", 450)
        h = param.GetInt("AnnotationStyleEditorHeight", 450)
        self.form.resize(w, h)

        # center the dialog over FreeCAD window
        mw = Gui.getMainWindow()
        self.form.move(mw.frameGeometry().topLeft()
                       + mw.rect().center()
                       - self.form.rect().center())

        # set icons
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Annotation_Style.svg"))
        self.form.pushButtonDelete.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
        self.form.pushButtonRename.setIcon(QtGui.QIcon(":/icons/accessories-text-editor.svg"))
        self.form.pushButtonDelete.resize(self.form.pushButtonDelete.sizeHint())
        self.form.pushButtonRename.resize(self.form.pushButtonRename.sizeHint())
        self.form.pushButtonImport.setIcon(QtGui.QIcon(":/icons/Std_Import.svg"))
        self.form.pushButtonExport.setIcon(QtGui.QIcon(":/icons/Std_Export.svg"))

        # fill the styles combo
        self.styles = self.read_meta()
        for style in self.styles.keys():
            self.form.comboBoxStyles.addItem(style)

        # connect signals/slots
        self.form.comboBoxStyles.currentIndexChanged.connect(self.on_style_changed)
        self.form.pushButtonDelete.clicked.connect(self.on_delete)
        self.form.pushButtonRename.clicked.connect(self.on_rename)
        self.form.pushButtonImport.clicked.connect(self.on_import)
        self.form.pushButtonExport.clicked.connect(self.on_export)
        self.form.buttonBox.accepted.connect(self.update_style)

        # show editor dialog
        self.fill_editor()
        result = self.form.exec_()

        # process if OK was clicked
        if result:
            self.save_meta(self.styles)

        # store dialog size
        param.SetInt("AnnotationStyleEditorWidth", self.form.width())
        param.SetInt("AnnotationStyleEditorHeight", self.form.height())

    def read_meta(self):
        """Read the document Meta attribute and return a dict."""
        styles = {}
        meta = self.doc.Meta
        for key, value in meta.items():
            if key.startswith("Draft_Style_"):
                styles[key[12:]] = self.repair_style(json.loads(value))
        return styles

    def save_meta(self, styles):
        """Save a dict to the document Meta attribute and update objects."""
        # save meta
        changedstyles = []
        meta = self.doc.Meta
        for key, value in styles.items():
            try:
                strvalue = json.dumps(value)
            except Exception:
                print("debug: unable to serialize this:", value)
            if ("Draft_Style_" + key in meta
                    and meta["Draft_Style_" + key] != strvalue):
                changedstyles.append(key)
            meta["Draft_Style_" + key] = strvalue

        # remove deleted styles
        todelete = []
        for key, value in meta.items():
            if key.startswith("Draft_Style_"):
                if key[12:] not in styles:
                    todelete.append(key)
        for key in todelete:
            del meta[key]

        self.doc.Meta = meta

        # propagate changes to all annotations
        for obj in self.get_annotations():
            vobj = obj.ViewObject
            try:
                current = vobj.AnnotationStyle
            except AssertionError:
                # empty annotation styles list
                pass
            else:
                if vobj.AnnotationStyle in self.renamed:
                    # the style has been renamed
                    # temporarily add the new style and switch to it
                    vobj.AnnotationStyle = vobj.AnnotationStyle + [self.renamed[vobj.AnnotationStyle]]
                    vobj.AnnotationStyle = self.renamed[vobj.AnnotationStyle]
                if vobj.AnnotationStyle in styles:
                    if vobj.AnnotationStyle in changedstyles:
                        # the style has changed
                        for attr, value in styles[vobj.AnnotationStyle].items():
                            if hasattr(vobj, attr):
                                try:
                                    if vobj.getTypeIdOfProperty(attr) == "App::PropertyColor":
                                        value = value & 0xFFFFFF00
                                    setattr(vobj, attr, value)
                                except:
                                    pass
                else:
                    # the style has been removed
                    vobj.AnnotationStyle = ""
            vobj.AnnotationStyle = [""] + list(styles)

    def on_style_changed(self, index):
        """Execute as a callback when the styles combobox changes."""
        if index == 0:
            # nothing happens
            self.form.pushButtonDelete.setEnabled(False)
            self.form.pushButtonRename.setEnabled(False)
        elif index == 1:
            # Add new... entry
            reply = QtGui.QInputDialog.getText(None,
                                               "Create new style",
                                               "Style name:")
            if reply[1]:
                # OK or Enter pressed
                name = reply[0].strip()
                if name == "":
                    QtGui.QMessageBox.information(None,
                                                  "Style name required",
                                                  "No style name specified")
                    self.form.comboBoxStyles.setCurrentIndex(0)
                elif name in self.styles:
                    QtGui.QMessageBox.information(None,
                                                  "Style exists",
                                                  "This style name already exists")
                    self.form.comboBoxStyles.setCurrentIndex(0)
                else:
                    # create new style from current editor values
                    self.styles[name] = self.get_editor_values()
                    self.form.comboBoxStyles.addItem(name)
                    self.form.comboBoxStyles.setCurrentIndex(self.form.comboBoxStyles.count() - 1)
                    self.current_style = name
            else:
                # Cancel or Escape pressed
                self.form.comboBoxStyles.setCurrentIndex(0)
        else:
            # Existing style
            if self.current_style is not None:
                # save editor values to current style first
                self.styles[self.current_style] = self.get_editor_values()
            self.current_style = self.form.comboBoxStyles.itemText(index)
            self.fill_editor(self.current_style)
            self.form.pushButtonDelete.setEnabled(True)
            self.form.pushButtonRename.setEnabled(True)

    def on_delete(self):
        """Execute as a callback when the delete button is pressed."""
        index = self.form.comboBoxStyles.currentIndex()
        style = self.form.comboBoxStyles.itemText(index)

        if self.get_style_users(style):
            reply = QtGui.QMessageBox.question(None,
                                               "Style in use",
                                               "This style is used by some objects in this document. Are you sure?",
                                               QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                               QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.No:
                return
        self.form.comboBoxStyles.removeItem(index)
        del self.styles[style]

    def on_rename(self):
        """Execute as a callback when the rename button is pressed."""
        index = self.form.comboBoxStyles.currentIndex()
        style = self.form.comboBoxStyles.itemText(index)

        reply = QtGui.QInputDialog.getText(None,
                                           "Rename style",
                                           "New name:",
                                           QtGui.QLineEdit.Normal,
                                           style)
        if reply[1]:
            # OK or Enter pressed
            newname = reply[0]
            if newname in self.styles:
                reply = QtGui.QMessageBox.information(None,
                                                      "Style exists",
                                                      "This style name already exists")
            else:
                self.form.comboBoxStyles.setItemText(index, newname)
                value = self.styles[style]
                del self.styles[style]
                self.styles[newname] = value
                self.renamed[style] = newname

    def on_import(self):
        """Import styles from a json file."""
        filename = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            translate("draft","Open styles file"),
            None,
            translate("draft","JSON file (*.json)"))
        if filename and filename[0]:
            nstyles = {}
            with open(filename[0]) as f:
                for key, val in json.load(f).items():
                    nstyles[key] = self.repair_style(val)
            if nstyles:
                self.styles.update(nstyles)
                for style in self.styles.keys():
                    if self.form.comboBoxStyles.findText(style) == -1:
                        self.form.comboBoxStyles.addItem(style)
                self.fill_editor(self.current_style) # The current style may have changed.
                print("Styles updated from " + filename[0])

    def on_export(self):
        """Export styles to a json file."""
        filename = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(),
            translate("draft","Save styles file"),
            None,
            translate("draft","JSON file (*.json)"))
        if filename and filename[0]:
            self.update_style()
            with open(filename[0],"w") as f:
                json.dump(self.styles,f,indent=4)
            print("Styles saved to " + filename[0])

    def repair_style(self, style):
        """Repair a V0.19 or V0.20 style.

        Some properties were missing or misspelled.
        Some float values were wrongly stored as strings.
        """
        new = {}
        for key, val in DEFAULT.items():
            if style.get(key) is None:
                new[key] = val[1]
            elif type(style[key]) == type(val[1]):
                new[key] = style[key]
            elif isinstance(style[key], str):
                new[key] = float(style[key].replace(",", "."))
            else:
                new[key] = val[1]
        return new

    def fill_editor(self, style=None):
        """Fill the editor fields with the contents of a style."""
        if style is None or style == "":
            style = {}
            for key, val in DEFAULT.items():
                style[key] = val[1]
        elif isinstance(style, dict):
            pass
        elif isinstance(style, str) and style in self.styles:
            style = self.styles[style]
        else:
            print("debug: unable to fill dialog from style", style)

        for key, value in style.items():
            control = getattr(self.form, key)
            if DEFAULT[key][0] == "str":
                control.setText(value)
            elif DEFAULT[key][0] == "font":
                control.setCurrentFont(QtGui.QFont(value))
            elif DEFAULT[key][0] == "color":
                color = QtGui.QColor(utils.rgba_to_argb(value))
                control.setProperty("color", color)
            elif DEFAULT[key][0] == "int":
                control.setValue(value)
            elif DEFAULT[key][0] == "float":
                if hasattr(control, "setText"):
                    control.setText(App.Units.Quantity(value, App.Units.Length).UserString)
                else:
                    control.setValue(value)
            elif DEFAULT[key][0] == "bool":
                control.setChecked(value)
            elif DEFAULT[key][0] == "index":
                control.setCurrentIndex(value)

    def update_style(self):
        """Update the current style with the values from the editor."""
        index = self.form.comboBoxStyles.currentIndex()
        if index > 1:
            style = self.form.comboBoxStyles.itemText(index)
            self.styles[style] = self.get_editor_values()

    def get_editor_values(self):
        values = {}
        for key in DEFAULT.keys():
            control = getattr(self.form, key)
            if DEFAULT[key][0] == "str":
                values[key] = control.text()
            elif DEFAULT[key][0] == "font":
                values[key] = control.currentFont().family()
            elif DEFAULT[key][0] == "color":
                values[key] = utils.argb_to_rgba(control.property("color").rgba())
            elif DEFAULT[key][0] == "int":
                values[key] = control.value()
            elif DEFAULT[key][0] == "float":
                if hasattr(control, "setText"):
                    values[key] = App.Units.Quantity(control.text()).Value
                else:
                    values[key] = control.value()
            elif DEFAULT[key][0] == "bool":
                values[key] = control.isChecked()
            elif DEFAULT[key][0] == "index":
                values[key] = control.currentIndex()
        return values

    def get_annotations(self):
        """Get all objects that support annotation styles."""
        users = []
        for obj in self.doc.Objects:
            vobj = obj.ViewObject
            if "AnnotationStyle" in vobj.PropertiesList:
                users.append(obj)
        return users

    def get_style_users(self, style):
        """Get all objects using a certain style."""
        users = []
        for obj in self.get_annotations():
            if obj.ViewObject.AnnotationStyle == style:
                users.append(obj)
        return users


Gui.addCommand('Draft_AnnotationStyleEditor', AnnotationStyleEditor())

## @}
