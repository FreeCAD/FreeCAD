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

def QT_TRANSLATE_NOOP(ctx,txt): return txt

param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

DEFAULT = {
    "FontName":("font",param.GetString("textfont","Sans")),
    "FontSize":("str",str(param.GetFloat("textheight",100))),
    "LineSpacing":("str","1 cm"),
    "ScaleMultiplier":("float",1),
    "ShowUnit":("bool",False),
    "UnitOverride":("str",""),
    "Decimals":("int",2),
    "ShowLines":("bool",True),
    "LineWidth":("int",param.GetInt("linewidth",1)),
    "LineColor":("color",param.GetInt("color",255)),
    "ArrowType":("index",param.GetInt("dimsymbol",0)),
    "ArrowSize":("str",str(param.GetFloat("arrowsize",20))),
    "DimensionOvershoot":("str",str(param.GetFloat("dimovershoot",20))),
    "ExtensionLines":("str",str(param.GetFloat("extlines",300))),
    "ExtensionOvershoot":("str",str(param.GetFloat("extovershoot",20))),
    }


class Draft_AnnotationStyleEditor:

    def __init__(self):

        self.styles = {}
        self.renamed = {}

    def GetResources(self):

        return {'Pixmap'  : ":icons/Draft_Annotation_Style.svg",
                'MenuText': QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor", "Annotation styles..."),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_AnnotationStyleEditor", "Manage or create annotation styles")}

    def IsActive(self):

        return bool(FreeCAD.ActiveDocument)

    def Activated(self):

        from PySide import QtGui

        # reset rename table
        self.renamed = {}

        # load dialog
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialog_AnnotationStyleEditor.ui")

        # restore stored size
        w = param.GetInt("AnnotationStyleEditorWidth",450)
        h = param.GetInt("AnnotationStyleEditorHeight",450)
        self.form.resize(w,h)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.form.rect().center())

        # set icons
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Annotation_Style.svg"))
        self.form.pushButtonDelete.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
        self.form.pushButtonRename.setIcon(QtGui.QIcon(":/icons/accessories-text-editor.svg"))

        # fill the styles combo
        self.styles = self.read_meta()
        for style in self.styles.keys():
            self.form.comboBoxStyles.addItem(style)

        # connect signals/slots
        self.form.comboBoxStyles.currentIndexChanged.connect(self.on_style_changed)
        self.form.pushButtonDelete.clicked.connect(self.on_delete)
        self.form.pushButtonRename.clicked.connect(self.on_rename)
        for attr in DEFAULT.keys():
            control = getattr(self.form,attr)
            for signal in ["clicked","textChanged","valueChanged","stateChanged","currentIndexChanged"]:
                if hasattr(control,signal):
                    getattr(control,signal).connect(self.update_style)
                    break

        # show editor dialog
        result = self.form.exec_()

        # process if OK was clicked
        if result:
            self.save_meta(self.styles)

        # store dialog size
        param.SetInt("AnnotationStyleEditorWidth",self.form.width())
        param.SetInt("AnnotationStyleEditorHeight",self.form.height())

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
            try:
                strvalue = json.dumps(value)
            except:
                print("debug: unable to serialize this:",value)
            if ("Draft_Style_"+key in meta) and (meta["Draft_Style_"+key] != strvalue):
                changedstyles.append(key)
            meta["Draft_Style_"+key] = strvalue
        # remove deleted styles
        todelete = []
        for key,value in meta.items():
            if key.startswith("Draft_Style_"):
                if key[12:] not in styles:
                    todelete.append(key)
        for key in todelete:
            del meta[key]
        
        FreeCAD.ActiveDocument.Meta = meta

        # propagate changes to all annotations
        for obj in self.get_annotations():
            if obj.ViewObject.AnnotationStyle in self.renamed.keys():
                # temporarily add the new style and switch to it
                obj.ViewObject.AnnotationStyle = obj.ViewObject.AnnotationStyle+[self.renamed[obj.ViewObject.AnnotationStyle]]
                obj.ViewObject.AnnotationStyle = self.renamed[obj.ViewObject.AnnotationStyle]
            if obj.ViewObject.AnnotationStyle in styles.keys():
                if obj.ViewObject.AnnotationStyle in changedstyles:
                    for attr,attrvalue in styles[obj.ViewObject.AnnotationStyle].items():
                        if hasattr(obj.ViewObject,attr):
                            setattr(obj.ViewObject,attr,attrvalue)
            else:
                obj.ViewObject.AnnotationStyle = ""
            obj.ViewObject.AnnotationStyle == [""] + styles.keys()

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
                    self.styles[name] = {}
                    for key,val in DEFAULT.items():
                        self.styles[name][key] = val[1]
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

        index = self.form.comboBoxStyles.currentIndex()
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

        index = self.form.comboBoxStyles.currentIndex()
        style = self.form.comboBoxStyles.itemText(index)
        reply = QtGui.QInputDialog.getText(None, "Rename style","New name:",QtGui.QLineEdit.Normal,style)
        if reply[1]:
            # OK or Enter pressed
            newname = reply[0]
            if newname in self.styles:
                reply = QtGui.QMessageBox.information(None,"Style exists","This style name already exists")
            else:
                self.form.comboBoxStyles.setItemText(index,newname)
                value = self.styles[style]
                del self.styles[style]
                self.styles[newname] = value
                self.renamed[style] = newname

    def fill_editor(self,style):

        """fills the editor fields with the contents of a style"""

        from PySide import QtGui

        if style is None:
            style = {}
            for key,val in DEFAULT.items():
                style[key] = val[1]
        if not isinstance(style,dict):
            if style in self.styles:
                style = self.styles[style]
            else:
                print("debug: unable to fill dialog from style",style)
        for key,value in style.items():
            control = getattr(self.form,key)
            if DEFAULT[key][0] == "str":
                control.setText(value)
            elif DEFAULT[key][0] == "font":
                control.setCurrentFont(QtGui.QFont(value))
            elif DEFAULT[key][0] == "color":
                r = ((value>>24)&0xFF)/255.0
                g = ((value>>16)&0xFF)/255.0
                b = ((value>>8)&0xFF)/255.0
                color = QtGui.QColor.fromRgbF(r,g,b)
                control.setProperty("color",color)
            elif DEFAULT[key][0] in ["int","float"]:
                control.setValue(value)
            elif DEFAULT[key][0] == "bool":
                control.setChecked(value)
            elif DEFAULT[key][0] == "index":
                control.setCurrentIndex(value)

    def update_style(self,arg=None):

        """updates the current style with the values from the editor"""

        index = self.form.comboBoxStyles.currentIndex()
        if index > 1:
            values = {}
            style = self.form.comboBoxStyles.itemText(index)
            for key in DEFAULT.keys():
                control = getattr(self.form,key)
                if DEFAULT[key][0] == "str":
                    values[key] = control.text()
                elif DEFAULT[key][0] == "font":
                    values[key] = control.currentFont().family()
                elif DEFAULT[key][0] == "color":
                    values[key] = control.property("color").rgb()<<8
                elif DEFAULT[key][0] in ["int","float"]:
                    values[key] = control.value()
                elif DEFAULT[key][0] == "bool":
                    values[key] = control.isChecked()
                elif DEFAULT[key][0] == "index":
                    values[key] = control.currentIndex()
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
