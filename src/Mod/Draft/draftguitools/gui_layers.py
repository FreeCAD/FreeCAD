# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
"""Provides GUI tools to create Layer objects."""
## @package gui_layers
# \ingroup draftguitools
# \brief Provides GUI tools to create Layer objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import os
import FreeCAD
import FreeCADGui as Gui
import Draft
import Draft_rc
import draftguitools.gui_base as gui_base

from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


def getColorIcon(color):

    "returns a QtGui.QIcon from a color 3-float tuple"

    from PySide import QtCore,QtGui
    c = QtGui.QColor(int(color[0]*255),int(color[1]*255),int(color[2]*255))
    im = QtGui.QImage(48,48,QtGui.QImage.Format_ARGB32)
    im.fill(c)
    px = QtGui.QPixmap.fromImage(im)
    return QtGui.QIcon(px)


class Layer(gui_base.GuiCommandSimplest):
    """GuiCommand to create a Layer object in the document."""

    def __init__(self):
        super(Layer, self).__init__(name=translate("draft","Layer"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_Layer',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Layer", "Layer"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Layer", "Adds a layer to the document.\nObjects added to this layer can share the same visual properties such as line color, line width, and shape color.")}

    def Activated(self):
        """Execute when the command is called.

        It calls the `finish(False)` method of the active Draft command.
        """
        super(Layer, self).Activated()

        self.doc.openTransaction("Create Layer")
        Gui.addModule("Draft")
        Gui.doCommand('_layer_ = Draft.make_layer()')
        Gui.doCommand('FreeCAD.ActiveDocument.recompute()')
        self.doc.commitTransaction()


class LayerManager:

    """GuiCommand that displays a Layers manager dialog"""

    def GetResources(self):

        return {'Pixmap'  : 'Draft_LayerManager',
                'MenuText': QT_TRANSLATE_NOOP("Draft_LayerManager", "Manage layers..."),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_LayerManager", "Set/modify the different layers of this document")}

    def Activated(self):

        from PySide import QtCore, QtGui

        # store changes to be committed
        self.deleteList = []

        # create the dialog
        self.dialog = Gui.PySideUic.loadUi(":/ui/dialogLayers.ui")

        # set nice icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/Draft_Layer.svg"))
        self.dialog.buttonNew.setIcon(QtGui.QIcon(":/icons/document-new.svg"))
        self.dialog.buttonDelete.setIcon(QtGui.QIcon(":/icons/delete.svg"))
        self.dialog.buttonSelectAll.setIcon(QtGui.QIcon(":/icons/edit-select-all.svg"))
        self.dialog.buttonToggle.setIcon(QtGui.QIcon(":/icons/dagViewVisible.svg"))
        self.dialog.buttonIsolate.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
        self.dialog.buttonCancel.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
        self.dialog.buttonOK.setIcon(QtGui.QIcon(":/icons/edit_OK.svg"))

        # restore window geometry from stored state
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        w = pref.GetInt("LayersManagerWidth",640)
        h = pref.GetInt("LayersManagerHeight",320)
        self.dialog.resize(w,h)

        # center the dialog over FreeCAD window
        mw = Gui.getMainWindow()
        self.dialog.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.dialog.rect().center())

        # connect signals/slots
        self.dialog.buttonNew.clicked.connect(self.addItem)
        self.dialog.buttonDelete.clicked.connect(self.onDelete)
        self.dialog.buttonSelectAll.clicked.connect(self.dialog.tree.selectAll)
        self.dialog.buttonToggle.clicked.connect(self.onToggle)
        self.dialog.buttonCancel.clicked.connect(self.dialog.reject)
        self.dialog.buttonIsolate.clicked.connect(self.onIsolate)
        self.dialog.buttonOK.clicked.connect(self.accept)
        self.dialog.rejected.connect(self.reject)

        # set the model up
        self.model = QtGui.QStandardItemModel()
        self.dialog.tree.setModel(self.model)
        self.dialog.tree.setUniformRowHeights(True)
        self.dialog.tree.setItemDelegate(Layers_Delegate())
        self.dialog.tree.setItemsExpandable(False)
        self.dialog.tree.setRootIsDecorated(False) # removes spacing in first column
        self.dialog.tree.setSelectionMode(QtGui.QTreeView.ExtendedSelection) # allow to select many

        # fill the tree view
        self.update()

        # rock 'n roll!!!
        self.dialog.exec_()

    def accept(self):

        "when OK button is pressed"

        changed = False

        # delete layers
        for name in self.deleteList:
            if not changed:
                FreeCAD.ActiveDocument.openTransaction("Layers change")
                changed = True
            FreeCAD.ActiveDocument.removeObject(name)

        # apply changes
        for row in range(self.model.rowCount()):

            # get or create layer
            name = self.model.item(row,1).toolTip()
            obj = None
            if name:
                obj = FreeCAD.ActiveDocument.getObject(name)
            if not obj:
                if not changed:
                    FreeCAD.ActiveDocument.openTransaction("Layers change")
                    changed = True
                obj = Draft.make_layer()

            # visibility
            checked = True if self.model.item(row,0).checkState() == QtCore.Qt.Checked else False
            if checked != obj.ViewObject.Visibility:
                if not changed:
                    FreeCAD.ActiveDocument.openTransaction("Layers change")
                    changed = True
                obj.ViewObject.Visibility = checked

            # label
            label = self.model.item(row,1).text()
            if label:
                if obj.Label != label:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.Label = label

            # line width
            width = self.model.item(row,2).data(QtCore.Qt.DisplayRole)
            if width:
                if obj.ViewObject.LineWidth != width:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.ViewObject.LineWidth = width

            # draw style
            style = self.model.item(row,3).text()
            if style:
                if obj.ViewObject.DrawStyle != style:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.ViewObject.DrawStyle = style

            # line color
            color = self.model.item(row,4).data(QtCore.Qt.UserRole)
            if color:
                if obj.ViewObject.LineColor[3:] != color:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.ViewObject.LineColor = color

            # shape color
            color = self.model.item(row,5).data(QtCore.Qt.UserRole)
            if color:
                if obj.ViewObject.ShapeColor[3:] != color:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.ViewObject.ShapeColor = color

            # transparency
            transparency = self.model.item(row,6).data(QtCore.Qt.DisplayRole)
            if transparency:
                if obj.ViewObject.Transparency != transparency:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Layers change")
                        changed = True
                    obj.ViewObject.Transparency = transparency

            # line print color
            color = self.model.item(row,7).data(QtCore.Qt.UserRole)
            if color:
                if not "LinePrintColor" in obj.ViewObject.PropertiesList:
                    if hasattr(obj.ViewObject.Proxy,"set_properties"):
                        obj.ViewObject.Proxy.set_properties(obj.ViewObject)
                if "LinePrintColor" in obj.ViewObject.PropertiesList:
                    if obj.ViewObject.LinePrintColor[3:] != color:
                        if not changed:
                            FreeCAD.ActiveDocument.openTransaction("Layers change")
                            changed = True
                        obj.ViewObject.LinePrintColor = color

        # recompute
        if changed:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

        # exit
        self.dialog.reject()

    def reject(self):

        "when Cancel button is pressed or dialog is closed"

        # save dialog size
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        pref.SetInt("LayersManagerWidth",self.dialog.width())
        pref.SetInt("LayersManagerHeight",self.dialog.height())

        return True

    def update(self):

        "rebuild the model from document contents"

        self.model.clear()

        # set header
        self.model.setHorizontalHeaderLabels([translate("Draft","On"),
                                              translate("Draft","Name"),
                                              translate("Draft","Line width"),
                                              translate("Draft","Draw style"),
                                              translate("Draft","Line color"),
                                              translate("Draft","Face color"),
                                              translate("Draft","Transparency"),
                                              translate("Draft","Line print color")])
        self.dialog.tree.header().setDefaultSectionSize(72)
        self.dialog.tree.setColumnWidth(0,32) # on/off column
        self.dialog.tree.setColumnWidth(1,128) # name column

        # populate
        objs = [obj for obj in FreeCAD.ActiveDocument.Objects if Draft.getType(obj) == "Layer"]
        objs.sort(key=lambda o:o.Label)
        for obj in objs:
            self.addItem(obj)

    def addItem(self,obj=None):

        "adds a row to the model"

        from PySide import QtCore, QtGui

        # create row with default values
        onItem = QtGui.QStandardItem()
        onItem.setCheckable(True)
        onItem.setCheckState(QtCore.Qt.Checked)
        nameItem = QtGui.QStandardItem(translate("Draft","New Layer"))
        widthItem = QtGui.QStandardItem()
        widthItem.setData(self.getPref("DefaultShapeLineWidth",2,"Integer"),QtCore.Qt.DisplayRole)
        styleItem = QtGui.QStandardItem("Solid")
        lineColorItem = QtGui.QStandardItem()
        lineColorItem.setData(self.getPref("DefaultShapeLineColor",421075455),QtCore.Qt.UserRole)
        shapeColorItem = QtGui.QStandardItem()
        shapeColorItem.setData(self.getPref("DefaultShapeColor",3435973887),QtCore.Qt.UserRole)
        transparencyItem = QtGui.QStandardItem()
        transparencyItem.setData(0,QtCore.Qt.DisplayRole)
        linePrintColorItem = QtGui.QStandardItem()
        linePrintColorItem.setData(self.getPref("DefaultPrintColor",0),QtCore.Qt.UserRole)

        # populate with object data
        if obj:
            onItem.setCheckState(QtCore.Qt.Checked if obj.ViewObject.Visibility else QtCore.Qt.Unchecked)
            nameItem.setText(obj.Label)
            nameItem.setToolTip(obj.Name)
            widthItem.setData(obj.ViewObject.LineWidth,QtCore.Qt.DisplayRole)
            styleItem.setText(obj.ViewObject.DrawStyle)
            lineColorItem.setData(obj.ViewObject.LineColor[:3],QtCore.Qt.UserRole)
            shapeColorItem.setData(obj.ViewObject.ShapeColor[:3],QtCore.Qt.UserRole)
            transparencyItem.setData(obj.ViewObject.Transparency,QtCore.Qt.DisplayRole)
            if hasattr(obj.ViewObject,"LinePrintColor"):
                linePrintColorItem.setData(obj.ViewObject.LinePrintColor[:3],QtCore.Qt.UserRole)
        lineColorItem.setIcon(getColorIcon(lineColorItem.data(QtCore.Qt.UserRole)))
        shapeColorItem.setIcon(getColorIcon(shapeColorItem.data(QtCore.Qt.UserRole)))
        linePrintColorItem.setIcon(getColorIcon(linePrintColorItem.data(QtCore.Qt.UserRole)))

        # append row
        self.model.appendRow([onItem,
                              nameItem,
                              widthItem,
                              styleItem,
                              lineColorItem,
                              shapeColorItem,
                              transparencyItem,
                              linePrintColorItem])

    def getPref(self,value,default,valuetype="Unsigned"):

        "retrieves a view pref value"

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        if valuetype == "Unsigned":
            c = p.GetUnsigned(value,default)
            r = float((c>>24)&0xFF)/255.0
            g = float((c>>16)&0xFF)/255.0
            b = float((c>>8)&0xFF)/255.0
            return (r,g,b,)
        elif valuetype == "Integer":
            return p.GetInt(value,default)

    def onDelete(self):

        "delete selected rows"

        rows = []
        for index in self.dialog.tree.selectedIndexes():
            if not index.row() in rows:
                rows.append(index.row())

            # append layer name to the delete list
            if index.column() == 1:
                name = self.model.itemFromIndex(index).toolTip()
                if name:
                    if not name in self.deleteList:
                        self.deleteList.append(name)

        # delete rows starting from the lowest, to not alter row indexes while deleting
        rows.sort()
        rows.reverse()
        for row in rows:
            self.model.takeRow(row)

    def onToggle(self):

        "toggle selected layers on/off"

        from PySide import QtCore, QtGui

        state = None
        for index in self.dialog.tree.selectedIndexes():
            if index.column() == 0:
                # get state from first selected row
                if state is None:
                    if self.model.itemFromIndex(index).checkState() == QtCore.Qt.Checked:
                        state = QtCore.Qt.Unchecked
                    else:
                        state = QtCore.Qt.Checked
                self.model.itemFromIndex(index).setCheckState(state)

    def onIsolate(self):

        "isolates the selected layers (turns all the others off"

        from PySide import QtCore, QtGui

        onrows = []
        for index in self.dialog.tree.selectedIndexes():
            if not index.row() in onrows:
                onrows.append(index.row())
        for row in range(self.model.rowCount()):
            if not row in onrows:
                self.model.item(row,0).setCheckState(QtCore.Qt.Unchecked)


if FreeCAD.GuiUp:

    from PySide import QtCore, QtGui

    class Layers_Delegate(QtGui.QStyledItemDelegate):

        "model delegate"

        def __init__(self, parent=None, *args):

            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
            # setEditorData() is triggered several times.
            # But we want to show the color dialog only the first time
            self.first = True

        def createEditor(self,parent,option,index):

            if index.column() == 0: # Layer on/off
                editor = QtGui.QCheckBox(parent)
            if index.column() == 1: # Layer name
                editor = QtGui.QLineEdit(parent)
            elif index.column() == 2: # Line width
                editor = QtGui.QSpinBox(parent)
                editor.setMaximum(99)
            elif index.column() == 3: # Line style
                editor = QtGui.QComboBox(parent)
                editor.addItems(["Solid","Dashed","Dotted","Dashdot"])
            elif index.column() == 4: # Line color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            elif index.column() == 5: # Shape color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            elif index.column() == 6: # Transparency
                editor = QtGui.QSpinBox(parent)
                editor.setMaximum(100)
            elif index.column() == 7: # Line print color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            return editor

        def setEditorData(self, editor, index):

            if index.column() == 0: # Layer on/off
                editor.setChecked(index.data())
            elif index.column() == 1: # Layer name
                editor.setText(index.data())
            elif index.column() == 2: # Line width
                editor.setValue(index.data())
            elif index.column() == 3: # Line style
                editor.setCurrentIndex(["Solid","Dashed","Dotted","Dashdot"].index(index.data()))
            elif index.column() == 4: # Line color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(QtGui.QColor(int(c[0]*255),int(c[1]*255),int(c[2]*255)))
                    editor.setText(str(color.getRgbF()))
                    self.first = False
            elif index.column() == 5: # Shape color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(QtGui.QColor(int(c[0]*255),int(c[1]*255),int(c[2]*255)))
                    editor.setText(str(color.getRgbF()))
                    self.first = False
            elif index.column() == 6: # Transparency
                editor.setValue(index.data())
            elif index.column() == 7:  # Line print color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(QtGui.QColor(int(c[0]*255),int(c[1]*255),int(c[2]*255)))
                    editor.setText(str(color.getRgbF()))
                    self.first = False

        def setModelData(self, editor, model, index):

            if index.column() == 0: # Layer on/off
                model.setData(index,editor.isChecked())
            elif index.column() == 1: # Layer name
                model.setData(index,editor.text())
            elif index.column() == 2: # Line width
                model.setData(index,editor.value())
            elif index.column() == 3: # Line style
                model.setData(index,["Solid","Dashed","Dotted","Dashdot"][editor.currentIndex()])
            elif index.column() == 4: # Line color
                model.setData(index,eval(editor.text()),QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))
            elif index.column() == 5: # Shape color
                model.setData(index,eval(editor.text()),QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))
            elif index.column() == 6: # Transparency
                model.setData(index,editor.value())
            elif index.column() == 7: # Line prin color
                model.setData(index,eval(editor.text()),QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))




Gui.addCommand('Draft_Layer', Layer())
Gui.addCommand('Draft_LayerManager', LayerManager())

## @}
