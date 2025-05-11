# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.PropertyEditor as PathPropertyEditor
import Path.Base.Gui.Util as PathGuiUtil
import Path.Base.Util as PathUtil
import re


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class _Delegate(QtGui.QStyledItemDelegate):
    """Handles the creation of an appropriate editing widget for a given property."""

    ObjectRole = QtCore.Qt.UserRole + 1
    PropertyRole = QtCore.Qt.UserRole + 2
    EditorRole = QtCore.Qt.UserRole + 3

    def createEditor(self, parent, option, index):
        editor = index.data(self.EditorRole)
        if editor is None:
            obj = index.data(self.ObjectRole)
            prp = index.data(self.PropertyRole)
            editor = PathPropertyEditor.Editor(obj, prp)
            index.model().setData(index, editor, self.EditorRole)
        return editor.widget(parent)

    def setEditorData(self, widget, index):
        # called to update the widget with the current data
        index.data(self.EditorRole).setEditorData(widget)

    def setModelData(self, widget, model, index):
        # called to update the model with the data from the widget
        editor = index.data(self.EditorRole)
        editor.setModelData(widget)
        index.model().setData(
            index,
            PathUtil.getPropertyValueString(editor.obj, editor.prop),
            QtCore.Qt.DisplayRole,
        )


class ToolBitEditor(object):
    """UI and controller for editing a ToolBit.
    The controller embeds the UI to the parentWidget which has to have a
    layout attached to it.
    """

    def __init__(self, tool, parentWidget=None, loadBitBody=True):
        Path.Log.track()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitEditor.ui")

        if parentWidget:
            self.form.setParent(parentWidget)
            parentWidget.layout().addWidget(self.form)

        self.tool = tool
        tool.attach_to_doc(FreeCAD.ActiveDocument)
        self.loadbitbody = loadBitBody

        if self.loadbitbody:
            self.tool.Proxy.loadBitBody(self.tool)

        # remove example widgets
        layout = self.form.bitParams.layout()
        for i in range(layout.rowCount() - 1, -1, -1):
            layout.removeRow(i)
        # used to track property widgets and editors
        self.widgets = []

        self.setupTool(self.tool)
        self.setupAttributes(self.tool)

    def add_property_widget(self, ui, obj, prop_name: str):
        """Create an appropriate widget based on the property type."""
        prop_value = obj.getPropertyByName(prop_name)
        prop_type = obj.getTypeIdOfProperty(prop_name)
        
        # Check for Quantity type (float with units)
        if isinstance(prop_value, FreeCAD.Units.Quantity):
            spinbox = ui.createWidget("Gui::QuantitySpinBox")
            return spinbox, PathGuiUtil.QuantitySpinBox(spinbox, obj, prop_name)
        
        # Check for Boolean type
        elif isinstance(prop_value, bool):
            combobox = QtGui.QComboBox()
            return combobox, PathGuiUtil.BooleanComboBox(combobox, obj, prop_name)
        
        elif isinstance(prop_value, int):
            spinbox = QtGui.QSpinBox()
            return spinbox, PathGuiUtil.IntegerSpinBox(spinbox, obj, prop_name)
        
        # Check for Enumeration type
        if prop_type == "App::PropertyEnumeration":
            combobox = QtGui.QComboBox()
            return combobox, PathGuiUtil.EnumerationComboBox(combobox, obj, prop_name)

        # Default case
        else:
            label = QtGui.QLabel(str(prop_value))
            return label, PathGuiUtil.PropertyLabel(label, obj, prop_name)

    def setupTool(self, toolbit):
        Path.Log.track()
        # Can't delete and add fields to the form because of dangling references in case of
        # a focus change. see https://forum.freecad.org/viewtopic.php?f=10&t=52246#p458583
        # Instead we keep widgets once created and use them for new properties, and hide all
        # which aren't being needed anymore.

        def labelText(name):
            return re.sub(r"([A-Z][a-z]+)", r" \1", re.sub(r"([A-Z]+)", r" \1", name))

        layout = self.form.bitParams.layout()
        ui = FreeCADGui.UiLoader()

        # for all properties either assign them to existing labels and editors
        # or create additional ones for them if not enough have already been
        # created.
        usedRows = 0
        for nr, name in enumerate(toolbit._get_props(("Shape", "Attributes"))):
            if nr < len(self.widgets):
                Path.Log.debug("reuse row: {} [{}]".format(nr, name))
                label, widget, editor = self.widgets[nr]
                label.setText(labelText(name))
                editor.attachTo(toolbit.obj, name)
                label.show()
                widget.show()
            else:
                widget, editor = self.add_property_widget(ui, toolbit.obj, name)
                label = QtGui.QLabel(labelText(name))
                self.widgets.append((label, widget, editor))
                Path.Log.debug("create row: {} [{}]  {}".format(nr, name, type(widget)))
                if hasattr(widget, "editingFinished"):
                    widget.editingFinished.connect(self.updateTool)

            if nr >= layout.rowCount():
                layout.addRow(label, widget)
            usedRows = usedRows + 1

        # hide all rows which aren't being used
        Path.Log.track(usedRows, len(self.widgets))
        for i in range(usedRows, len(self.widgets)):
            label, qsb, editor = self.widgets[i]
            label.hide()
            qsb.hide()
            editor.attachTo(None)
            Path.Log.debug("  hide row: {}".format(i))

        icon = toolbit.get_icon()
        size = QtCore.QSize(150, 150)
        pixmap = icon.get_qpixmap(size) if icon else QtGui.QPixmap()
        self.form.image.setPixmap(pixmap)

    def setupAttributes(self, toolbit):
        """Populates the attributes shown in the Attribute tab"""
        Path.Log.track()

        setup = True
        if not hasattr(self, "delegate"):
            self.delegate = _Delegate(self.form.attrTree)
            self.model = QtGui.QStandardItemModel(self.form.attrTree)
            self.model.setHorizontalHeaderLabels(["Property", "Value"])
        else:
            self.model.removeRows(0, self.model.rowCount())
            setup = False

        attributes = toolbit.toolGroupsAndProperties(False)
        for name in attributes:
            group = QtGui.QStandardItem()
            group.setData(name, QtCore.Qt.EditRole)
            group.setEditable(False)
            for prop in attributes[name]:
                label = QtGui.QStandardItem()
                label.setData(prop, QtCore.Qt.EditRole)
                label.setEditable(False)

                value = PathUtil.getPropertyValueString(toolbit.obj, prop)
                value_item = QtGui.QStandardItem()
                value_item.setData(value, QtCore.Qt.DisplayRole)
                value_item.setData(toolbit.obj, _Delegate.ObjectRole)
                value_item.setData(prop, _Delegate.PropertyRole)

                group.appendRow([label, value_item])
            self.model.appendRow(group)

        if setup:
            self.form.attrTree.setModel(self.model)
            self.form.attrTree.setItemDelegateForColumn(1, self.delegate)
        self.form.attrTree.expandAll()
        self.form.attrTree.resizeColumnToContents(0)
        self.form.attrTree.resizeColumnToContents(1)
        # self.form.attrTree.collapseAll()

    def accept(self):
        Path.Log.track()
        self.refresh()
        self.tool.unloadBitBody()

    def reject(self):
        Path.Log.track()
        self.tool.unloadBitBody()

    def updateUI(self):
        Path.Log.track()
        self.form.toolName.setText(self.tool.obj.Label)
        self.form.shapeID.setText(self.tool.get_id())

        for lbl, qsb, editor in self.widgets:
            editor.updateWidget()

    def updateTool(self):
        Path.Log.track()

        label = str(self.form.toolName.text())
        if self.tool.obj.Label != label:
            self.tool.obj.Label = label

        for lbl, qsb, editor in self.widgets:
            editor.updateProperty()

    def refresh(self):
        Path.Log.track()
        self.form.blockSignals(True)
        self.updateTool()
        self.updateUI()
        self.form.blockSignals(False)

    def setupUI(self):
        Path.Log.track()
        self.updateUI()

        self.form.toolName.editingFinished.connect(self.refresh)
