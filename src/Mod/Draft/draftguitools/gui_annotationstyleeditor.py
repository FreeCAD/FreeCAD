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
        for attr in EMPTYSTYLE.keys():
            control = getattr(self.form,attr)
            for signal in ["textChanged","valueChanged","stateChanged"]:
                if hasattr(control,signal):
                    getattr(control,signal).connect(self.update_style)
                    break

        # show editor dialog
        result = self.form.exec_()

        # process if OK was clicked
        if result:
            self.save_meta(self.styles)

        return

    def read_meta(self):

        """reads the document Meta property and returns a dict"""

        styles = {}
        meta = FreeCAD.ActiveDocument.Meta
        for key,value in meta.items():
            if key.startswith("Draft_Style_"):
                styles[key[12:]] = json.loads(value)
        return styles

    def save_meta(self,styles):

        """saves a dict to the document Meta property and updates objects"""

        # save meta
        changedstyles = []
        meta = FreeCAD.ActiveDocument.Meta
        for key,value in styles.items():
            strvalue = json.dumps(value)
            if meta["Draft_Style_"+key] and (meta["Draft_Style_"+key] != strvalue):
                changedstyles.append(style)
            meta["Draft_Style_"+key] = strvalue
        FreeCAD.ActiveDocument.Meta = meta

        # propagate changes to all annotations
        for obj in self.get_annotations():
            if obj.ViewObject.AnnotationStyle in styles.keys():
                if obj.ViewObject.AnnotationStyle in changedstyles:
                    for attr,attrvalue in styles[obj.ViewObject.AnnotationStyle].items():
                        if hasattr(obj.ViewObject,attr):
                            setattr(obj.ViewObject,attr,attrvalue)
            else:
                obj.ViewObject.AnnotationStyle = " "
            obj.ViewObject.AnnotationStyle == [" "] + styles.keys()

    def on_style_changed(self,index):
        
        """called when the styles combobox is changed"""

        from PySide import QtGui

        if index <= 1: 
            # nothing happens
            self.form.pushButtonDelete.setEnabled(False)
            self.form.pushButtonRename.setEnabled(False)
            self.fill_editor(None)
        if index == 1: 
            # Add new... entry
            reply = QtGui.QInputDialog.getText(None, "Create new style","Style name:")
            if reply[1]: 
                # OK or Enter pressed
                name = reply[0]
                if name in self.styles:
                    reply = QtGui.QMessageBox.information(None,"Style exists","This style name already exists")
                else:
                    # create new default style
                    self.styles[name] = EMPTYSTYLE
                    self.form.comboBoxStyles.addItem(name)
                    self.form.comboBoxStyles.setCurrentIndex(self.form.comboBoxStyles.count()-1)
        elif index > 1: 
            # Existing style
            self.form.pushButtonDelete.setEnabled(True)
            self.form.pushButtonRename.setEnabled(True)
            self.fill_editor(self.form.comboBoxStyles.itemText(index))

    def on_delete(self):
        
        """called when the Delete button is pressed"""

        from PySide import QtGui

        index = self.form.comboBox.currentIndex()
        style = self.form.comboBoxStyles.itemText(index)
        if self.get_style_users(style):
            reply = QtGui.QMessageBox.question(None, "Style in use", "This style is used by some objects in this document. Are you sure?",
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.No:
                return
        self.form.comboBoxStyles.removeItem(index)
        del self.styles[style]

    def on_rename(self):
        
        """called when the Rename button is pressed"""

        from PySide import QtGui

        index = self.form.comboBox.currentIndex()
        style = self.form.comboBoxStyles.itemText(index)
        reply = QtGui.QInputDialog.getText(None, "Rename style","New name:",QtGui.QLineEdit.Normal,style)
        if reply[1]: 
            # OK or Enter pressed
            newname = reply[0]
            self.form.comboBoxStyles.setItemText(index,newname)
            value = self.styles[style]
            del self.styles[style]
            self.styles[newname] = value

    def fill_editor(self,style):
        
        """fills the editor fields with the contents of a style"""

        if style is None:
            style = EMPTYSTYLE
        for key,value in style.items():
            setattr(self.form,key,value)

    def update_style(self,arg=None):
        
        """updates the current style with the values from the editor"""
        
        index = self.form.comboBox.currentIndex()
        if index > 1:
            values = {}
            style = self.form.comboBoxStyles.itemText(index)
            for key in EMPTYSTYLE.keys():
                control = getattr(self.form,key)
                for attr in ["text","value","state"]:
                    if hasattr(control,attr):
                        values[key] = getattr(control,attr)
            self.styles[style] = values

    def get_annotations(self):
        
        """gets all the objects that support annotation styles"""

        users = []
        for obj in FreeCAD.ActiveDocument.Objects:
            vobj = obj.ViewObject
            if hasattr(vobj,"AnnotationStyle"):
                users.append(obj)
        return users

    def get_style_users(self,style):
        
        """get all objects using a certain style"""
        
        users = []
        for obj in self.get_annotations():
            if obj.ViewObject.AnnotationStyle == style:
                users.append(obj)
        return users
        

FreeCADGui.addCommand('Draft_AnnotationStyleEditor', Draft_AnnotationStyleEditor())
