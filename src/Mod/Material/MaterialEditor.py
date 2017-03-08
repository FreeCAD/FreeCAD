#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Yorik van Havre <yorik@uncreated.net>            *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
from __future__ import print_function

import FreeCAD, FreeCADGui, os
from PySide import QtCore, QtGui, QtUiTools, QtSvg

__title__="FreeCAD material editor"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


class MaterialEditor:


    def __init__(self, obj = None, prop = None, material = None):
        """Initializes, optionally with an object name and a material property name to edit, or directly
        with a material dictionary."""
        self.obj = obj
        self.prop = prop
        self.material = material
        self.customprops = []
        # load the UI file from the same directory as this script
        self.widget = FreeCADGui.PySideUic.loadUi(os.path.dirname(__file__)+os.sep+"materials-editor.ui")
        # additional UI fixes and tweaks
        self.widget.ButtonURL.setIcon(QtGui.QIcon(":/icons/internet-web-browser.svg"))
        self.widget.ButtonDeleteProperty.setEnabled(False)
        self.widget.standardButtons.button(QtGui.QDialogButtonBox.Ok).setAutoDefault(False)
        self.widget.standardButtons.button(QtGui.QDialogButtonBox.Cancel).setAutoDefault(False)
        self.updateCards()
        self.widget.Editor.header().resizeSection(0,200)
        self.widget.Editor.expandAll()
        self.widget.Editor.setFocus()
        # TODO allow to enter a custom property by pressing Enter in the lineedit (currently closes the dialog)
        self.widget.Editor.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        QtCore.QObject.connect(self.widget.ComboMaterial, QtCore.SIGNAL("currentIndexChanged(QString)"), self.updateContents)
        QtCore.QObject.connect(self.widget.ButtonURL, QtCore.SIGNAL("clicked()"), self.openProductURL)
        QtCore.QObject.connect(self.widget.standardButtons, QtCore.SIGNAL("accepted()"), self.accept)
        QtCore.QObject.connect(self.widget.standardButtons, QtCore.SIGNAL("rejected()"), self.reject)
        QtCore.QObject.connect(self.widget.ButtonAddProperty, QtCore.SIGNAL("clicked()"), self.addCustomProperty)
        QtCore.QObject.connect(self.widget.EditProperty, QtCore.SIGNAL("returnPressed()"), self.addCustomProperty)
        QtCore.QObject.connect(self.widget.ButtonDeleteProperty, QtCore.SIGNAL("clicked()"), self.deleteCustomProperty)
        QtCore.QObject.connect(self.widget.Editor, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem*,int)"), self.itemClicked)
        QtCore.QObject.connect(self.widget.Editor, QtCore.SIGNAL("itemChanged(QTreeWidgetItem*,int)"), self.itemChanged)
        QtCore.QObject.connect(self.widget.Editor, QtCore.SIGNAL("currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)"), self.checkDeletable)
        QtCore.QObject.connect(self.widget.ButtonOpen, QtCore.SIGNAL("clicked()"), self.openfile)
        QtCore.QObject.connect(self.widget.ButtonSave, QtCore.SIGNAL("clicked()"), self.savefile)
        # update the editor with the contents of the property, if we have one
        d = None
        if self.prop and self.obj:
            d = FreeCAD.ActiveDocument.getObject(self.obj).getPropertyByName(self.prop)
        elif self.material:
            d = self.material
        if d:
            self.updateContents(d)


    def updateCards(self):
        "updates the contents of the materials combo with existing material cards"
        # look for cards in both resources dir and a Materials sub-folder in the user folder.
        # User cards with same name will override system cards
        paths = [FreeCAD.getResourceDir() + os.sep + "Mod" + os.sep + "Material" + os.sep + "StandardMaterial"]
        ap = FreeCAD.ConfigGet("UserAppData") + os.sep + "Materials"
        if os.path.exists(ap):
            paths.append(ap)
        self.cards = {}
        for p in paths:
            for f in os.listdir(p):
                b,e = os.path.splitext(f)
                if e.upper() == ".FCMAT":
                    self.cards[b] = p + os.sep + f
        if self.cards:
            self.widget.ComboMaterial.clear()
            self.widget.ComboMaterial.addItem("") # add a blank item first
            for k,i in self.cards.items():
                self.widget.ComboMaterial.addItem(k)


    def updateContents(self,data):
        "updates the contents of the editor with the given data (can be the name of a card or a dictionary)"
        #print type(data)
        if isinstance(data,dict):
            self.clearEditor()
            for k,i in data.items():
                k = self.expandKey(k)
                slot = self.widget.Editor.findItems(k,QtCore.Qt.MatchRecursive,0)
                if len(slot) == 1:
                    slot = slot[0]
                    slot.setText(1,i)
                else:
                    self.addCustomProperty(k,i)
        elif isinstance(data,unicode):
            k = str(data)
            if k:
                if k in self.cards:
                    import importFCMat
                    d = importFCMat.read(self.cards[k])
                    if d:
                        self.updateContents(d)


    def openProductURL(self):
        "opens the contents of the ProductURL field in an external browser"
        url = str(self.widget.Editor.findItems(translate("Material","Product URL"),QtCore.Qt.MatchRecursive,0)[0].text(1))
        if url:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(url, QtCore.QUrl.TolerantMode))


    def accept(self):
        "if we are editing a property, set the property values"
        if self.prop and self.obj:
            d = self.getDict()
            o = FreeCAD.ActiveDocument.getObject(self.obj)
            setattr(o,self.prop,d)
        QtGui.QDialog.accept(self.widget)


    def reject(self):
        QtGui.QDialog.reject(self.widget)


    def expandKey(self, key):
        "adds spaces before caps in a KeyName"
        nk = ""
        for l in key:
            if l.isupper():
                if nk:
                    # this allows for series of caps, such as ProductURL
                    if not nk[-1].isupper():
                        nk += " "
            nk += l
        return nk


    def collapseKey(self, key):
        "removes the spaces in a Key Name"
        nk = ""
        for l in key:
            if l != " ":
                nk += l
        return nk


    def clearEditor(self):
        "Clears the contents of the editor"
        for i1 in range(self.widget.Editor.topLevelItemCount()):
            w = self.widget.Editor.topLevelItem(i1)
            for i2 in range(w.childCount()):
                c = w.child(i2)
                c.setText(1,"")
        for k in self.customprops:
            self.deleteCustomProperty(k)


    def addCustomProperty(self, key = None, value = None):
        "Adds a custom property to the editor, optionally with a value"
        if not key:
            key = str(self.widget.EditProperty.text())
        if key:
            if not key in self.customprops:
                if not self.widget.Editor.findItems(key,QtCore.Qt.MatchRecursive,0):
                    top = self.widget.Editor.findItems(translate("Material","User defined"),QtCore.Qt.MatchExactly,0)
                    if top:
                        i = QtGui.QTreeWidgetItem(top[0])
                        i.setFlags(QtCore.Qt.ItemIsSelectable|QtCore.Qt.ItemIsEditable|QtCore.Qt.ItemIsDragEnabled|QtCore.Qt.ItemIsUserCheckable|QtCore.Qt.ItemIsEnabled)
                        i.setText(0,key)
                        self.customprops.append(key)
                        self.widget.EditProperty.setText("")
                        if value:
                            i.setText(1,value)


    def deleteCustomProperty(self, key = None):
        "Deletes a custom property from the editor"
        if not key:
            key = str(self.widget.Editor.currentItem().text(0))
        if key:
            if key in self.customprops:
                i = self.widget.Editor.findItems(key,QtCore.Qt.MatchRecursive,0)
                if i:
                    top = self.widget.Editor.findItems(translate("Material","User defined"),QtCore.Qt.MatchExactly,0)
                    if top:
                        top = top[0]
                        ii = top.indexOfChild(i[0])
                        if ii >= 0:
                            top.takeChild(ii)
                            self.customprops.remove(key)


    def itemClicked(self, item, column):
        "Edits an item if it is not in the first column"
        if column > 0:
            self.widget.Editor.editItem(item, column)
            
            
    def itemChanged(self, item, column):
        "Handles text changes"
        if item.text(0) == "Section Fill Pattern":
            if column == 1:
                self.setTexture(item.text(1))


    def checkDeletable(self,current,previous):
        "Checks if the current item is a custom property, if yes enable the delete button"
        if str(current.text(0)) in self.customprops:
            self.widget.ButtonDeleteProperty.setEnabled(True)
        else:
            self.widget.ButtonDeleteProperty.setEnabled(False)


    def getDict(self):
        "returns a dictionary from the contents of the editor"
        d = {}
        for i1 in range(self.widget.Editor.topLevelItemCount()):
            w = self.widget.Editor.topLevelItem(i1)
            for i2 in range(w.childCount()):
                c = w.child(i2)
                # TODO the following should be translated back to english,since text(0) could be translated
                d[self.collapseKey(str(c.text(0)))] = unicode(c.text(1))
        return d


        if d:
            self.updateContents(d)
        self.widget.Editor.topLevelItem(6).child(4).setToolTip(1,self.getPatternsList())


    def setTexture(self,pattern):
        "displays a texture preview if needed"
        self.widget.PreviewVector.hide()
        if pattern:
            try:
                import DrawingPatterns
            except:
                print("DrawingPatterns not found")
            else:
                pattern = DrawingPatterns.buildFileSwatch(pattern,size=96,png=True)
                if pattern:
                    self.widget.PreviewVector.setPixmap(QtGui.QPixmap(pattern))
                    self.widget.PreviewVector.show()


    def openfile(self):
        "Opens a FCMat file"
        filename = QtGui.QFileDialog.getOpenFileName(QtGui.qApp.activeWindow(),'Open FreeCAD Material file','*.FCMat')
        if filename:
            self.clearEditor()
            import importFCMat
            d = importFCMat.read(filename[0])
            if d:
                self.updateContents(d)


    def savefile(self):
        "Saves a FCMat file"
        name = str(self.widget.Editor.findItems(translate("Material","Name"),QtCore.Qt.MatchRecursive,0)[0].text(1))
        if not name:
            name = "Material"
        filename = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(),'Save FreeCAD Material file',name+'.FCMat')
        if filename:
            d = self.getDict()
            if d:
                import importFCMat
                importFCMat.write(filename,d)


    def show(self):
        return self.widget.show()


    def exec_(self):
        return self.widget.exec_()




def translate(context,text):
    "translates text"
    return text #TODO use Qt translation mechanism here


def openEditor(obj = None, prop = None):
    """openEditor([obj,prop]): opens the editor, optionally with
    an object name and material property name to edit"""
    editor = MaterialEditor(obj,prop)
    editor.show()


def editMaterial(material):
    """editMaterial(material): opens the editor to edit the contents
    of the given material dictionary. Returns the modified material."""
    editor = MaterialEditor(material=material)
    result = editor.exec_()
    if result:
        return editor.getDict()
    else:
        return material

