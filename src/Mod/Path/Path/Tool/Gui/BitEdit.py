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
import FreeCADGui
import Path
import Path.Base.Gui.PropertyEditor as PathPropertyEditor
import Path.Base.Gui.Util as PathGuiUtil
import Path.Base.Util as PathUtil
import os
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
        self.loadbitbody = loadBitBody
        if not tool.BitShape:
            self.tool.BitShape = "endmill.fcstd"

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

    def setupTool(self, tool):
        Path.Log.track()
        # Can't delete and add fields to the form because of dangling references in case of
        # a focus change. see https://forum.freecad.org/viewtopic.php?f=10&t=52246#p458583
        # Instead we keep widgets once created and use them for new properties, and hide all
        # which aren't being needed anymore.

        def labelText(name):
            return re.sub("([A-Z][a-z]+)", r" \1", re.sub("([A-Z]+)", r" \1", name))

        layout = self.form.bitParams.layout()
        ui = FreeCADGui.UiLoader()

        # for all properties either assign them to existing labels and editors
        # or create additional ones for them if not enough have already been
        # created.
        usedRows = 0
        for nr, name in enumerate(tool.Proxy.toolShapeProperties(tool)):
            if nr < len(self.widgets):
                Path.Log.debug("re-use row: {} [{}]".format(nr, name))
                label, qsb, editor = self.widgets[nr]
                label.setText(labelText(name))
                editor.attachTo(tool, name)
                label.show()
                qsb.show()
            else:
                qsb = ui.createWidget("Gui::QuantitySpinBox")
                editor = PathGuiUtil.QuantitySpinBox(qsb, tool, name)
                label = QtGui.QLabel(labelText(name))
                self.widgets.append((label, qsb, editor))
                Path.Log.debug("create row: {} [{}]  {}".format(nr, name, type(qsb)))
                if hasattr(qsb, "editingFinished"):
                    qsb.editingFinished.connect(self.updateTool)

            if nr >= layout.rowCount():
                layout.addRow(label, qsb)
            usedRows = usedRows + 1

        # hide all rows which aren't being used
        Path.Log.track(usedRows, len(self.widgets))
        for i in range(usedRows, len(self.widgets)):
            label, qsb, editor = self.widgets[i]
            label.hide()
            qsb.hide()
            editor.attachTo(None)
            Path.Log.debug("  hide row: {}".format(i))

        img = tool.Proxy.getBitThumbnail(tool)
        if img:
            self.form.image.setPixmap(QtGui.QPixmap(QtGui.QImage.fromData(img)))
        else:
            self.form.image.setPixmap(QtGui.QPixmap())

    def setupAttributes(self, tool):
        Path.Log.track()

        setup = True
        if not hasattr(self, "delegate"):
            self.delegate = _Delegate(self.form.attrTree)
            self.model = QtGui.QStandardItemModel(self.form.attrTree)
            self.model.setHorizontalHeaderLabels(["Property", "Value"])
        else:
            self.model.removeRows(0, self.model.rowCount())
            setup = False

        attributes = tool.Proxy.toolGroupsAndProperties(tool, False)
        for name in attributes:
            group = QtGui.QStandardItem()
            group.setData(name, QtCore.Qt.EditRole)
            group.setEditable(False)
            for prop in attributes[name]:
                label = QtGui.QStandardItem()
                label.setData(prop, QtCore.Qt.EditRole)
                label.setEditable(False)

                value = QtGui.QStandardItem()
                value.setData(
                    PathUtil.getPropertyValueString(tool, prop), QtCore.Qt.DisplayRole
                )
                value.setData(tool, _Delegate.ObjectRole)
                value.setData(prop, _Delegate.PropertyRole)

                group.appendRow([label, value])
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
        self.tool.Proxy.unloadBitBody(self.tool)

    def reject(self):
        Path.Log.track()
        self.tool.Proxy.unloadBitBody(self.tool)

    def updateUI(self):
        Path.Log.track()
        self.form.toolName.setText(self.tool.Label)
        self.form.shapePath.setText(self.tool.BitShape)

        for lbl, qsb, editor in self.widgets:
            editor.updateSpinBox()

    def _updateBitShape(self, shapePath):
        # Only need to go through this exercise if the shape actually changed.
        if self.tool.BitShape != shapePath:
            # Before setting a new  bitshape we need to make sure that none of
            # editors fires an event and tries to access its old property, which
            # might not exist anymore.
            for lbl, qsb, editor in self.widgets:
                editor.attachTo(self.tool, "File")
            self.tool.BitShape = shapePath
            self.setupTool(self.tool)
            self.form.toolName.setText(self.tool.Label)
            if self.tool.BitBody and self.tool.BitBody.ViewObject:
                if not self.tool.BitBody.ViewObject.Visibility:
                    self.tool.BitBody.ViewObject.Visibility = True
            self.setupAttributes(self.tool)
            return True
        return False

    def updateShape(self):
        Path.Log.track()
        shapePath = str(self.form.shapePath.text())
        # Only need to go through this exercise if the shape actually changed.
        if self._updateBitShape(shapePath):
            for lbl, qsb, editor in self.widgets:
                editor.updateSpinBox()

    def updateTool(self):
        Path.Log.track()

        label = str(self.form.toolName.text())
        shape = str(self.form.shapePath.text())
        if self.tool.Label != label:
            self.tool.Label = label
        self._updateBitShape(shape)

        for lbl, qsb, editor in self.widgets:
            editor.updateProperty()

        self.tool.Proxy._updateBitShape(self.tool)

    def refresh(self):
        Path.Log.track()
        self.form.blockSignals(True)
        self.updateTool()
        self.updateUI()
        self.form.blockSignals(False)

    def selectShape(self):
        Path.Log.track()
        path = self.tool.BitShape
        if not path:
            path = Path.Preferences.lastPathToolShape()
        foo = QtGui.QFileDialog.getOpenFileName(
            self.form, "Path - Tool Shape", path, "*.fcstd"
        )
        if foo and foo[0]:
            Path.Preferences.setLastPathToolShape(os.path.dirname(foo[0]))
            self.form.shapePath.setText(foo[0])
            self.updateShape()

    def setupUI(self):
        Path.Log.track()
        self.updateUI()

        self.form.toolName.editingFinished.connect(self.refresh)
        self.form.shapePath.editingFinished.connect(self.updateShape)
        self.form.shapeSet.clicked.connect(self.selectShape)
