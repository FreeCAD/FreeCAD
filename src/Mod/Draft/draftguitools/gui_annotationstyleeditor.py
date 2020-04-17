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

"""
Provides all gui and tools to create and edit annotation styles
Provides Draft_AnnotationStyleEditor command
"""

import FreeCAD,FreeCADGui
import json

EMPTYSTYLE = {
    "FontName":"Sans",
    "FontSize":0,
    "LineSpacing":0,
    "ScaleMultiplier":1,
    "ShowUnit":False,
    "UnitOverride":"",
    "Decimals":0,
    "ShowLines":True,
    "LineWidth":1,
    "LineColor":255,
    "ArrowType":0,
    "ArrowSize":0,
    "DimensionOvershoot":0,
    "ExtensionLines":0,
    "ExtensionOvershoot":0,
    }


class Draft_AnnotationStyleEditor:

    def __init__(self):

        self.styles = {}

    def GetResources(self):

        return {'Pixmap'  : ":icons/Draft_AnnotationStyleEditor.svg",
                'MenuText': QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor", "Annotation styles..."),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor", "Manage or create annotation styles")}

    def IsActive(self):

        return bool(FreeCAD.ActiveDocument)

    def Activated(self):

        from PySide import QtGui

        # load dialog
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialog_AnnotationStyleEditor.ui")

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.form.rect().center())

        # set icons
        self.form.pushButtonDelete.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
        self.form.pushButtonRename.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))

        # fill the styles combo
        self.styles = self.read_meta()
        for style in self.styles.keys():
            self.form.comboBoxStyles.addItem(style)

        # connect signals/slots
        self.form.comboBoxStyles.currentIndexChanged.connect(self.on_style_changed)
        self.form.pushButtonDelete.clicked.connect(self.on_delete)
        self.form.pushButtonRename.clicked.connect(self.on_rename)
        # TODO connect all other controls to a function that saves to self.styles

        # show editor dialog
        result = self.form.exec_()

        # process if OK was clicked
        if result:
            self.save_meta(self.styles)

        return

    def read_meta(self):

        styles = {}
        meta = FreeCAD.ActiveDocument.Meta
        for key,value in meta.keys:
            if key.startswith("Draft_Style_"):
                styles[key[12:]] = json.loads(value)
        return styles

    def save_meta(self,styles):

        meta = FreeCAD.ActiveDocument.Meta
        for key,value in styles:
            meta["Draft_Style_"+key] = json.dumps(value)
        FreeCAD.ActiveDocument.Meta = meta

        # TODO must also save meta and also update the styles
        # of comboboxes of all dimensions and texts
        # found in the doc. If a dimension/text uses a style
        # that has now been deleted, that case must also be handled
        # maybe warn the user if the style is in use in on_delete?


    def on_style_changed(self,index):

        from PySide import QtGui

        if index <= 1:
            self.form.pushButtonDelete.setEnabled(False)
            self.form.pushButtonRename.setEnabled(False)
            self.fill_editor(None)
        if index == 1:
            reply = QtGui.QInputDialog.getText(None, "Create new style","Style name:")
            if reply[1]: # OK or Enter pressed
                name = reply[0]
                self.form.comboBoxStyles.addItem(name)
                self.form.comboBoxStyles.setCurrentIndex(self.form.comboBoxStyles.count()-1)
        elif index > 1:
            self.form.pushButtonDelete.setEnabled(True)
            self.form.pushButtonRename.setEnabled(True)
            self.fill_editor(self.form.comboBoxStyles.itemText(index))

    def on_delete(self):

        index = self.form.comboBox.currentIndex()
        if index > 1:
            style = self.form.comboBoxStyles.itemText(index)
            self.form.comboBoxStyles.removeItem(index)
            del self.styles[style]

    def on_rename(self):

        from PySide import QtGui

        index = self.form.comboBox.currentIndex()
        if index > 1:
            style = self.form.comboBoxStyles.itemText(index)
            reply = QtGui.QInputDialog.getText(None, "Rename style","New name:",QtGui.QLineEdit.Normal,style)
            if reply[1]: # OK or Enter pressed
                newname = reply[0]
                self.form.comboBoxStyles.setItemText(index,newname)
                value = self.styles[style]
                del self.styles[style]
                self.styles[newname] = value

    def fill_editor(self,style):

        if style is None:
            style = EMPTYSTYLE
        for key,value in style:
            setattr(self.form,key,value)


FreeCADGui.addCommand('Draft_AnnotationStyleEditor', Draft_AnnotationStyleEditor())
