# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import FreeCADGui
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSetupSheetGui as PathSetupSheetGui
import PathScripts.PathToolBit as PathToolBit
import os
import re

from PySide import QtCore, QtGui

# PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ToolBitEditor(object):
    '''UI and controller for editing a ToolBit.
    The controller embeds the UI to the parentWidget which has to have a layout attached to it.
    '''

    def __init__(self, tool, parentWidget=None):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitEditor.ui")

        if parentWidget:
            self.form.setParent(parentWidget)
            parentWidget.layout().addWidget(self.form)

        self.tool = tool
        if not tool.BitShape:
            self.tool.BitShape = 'endmill.fcstd'
        self.tool.Proxy.loadBitBody(self.tool)
        self.setupTool(self.tool)
        self.setupAttributes(self.tool)

    def setupTool(self, tool):
        layout = self.form.bitParams.layout()
        for i in range(layout.rowCount() - 1, -1, -1):
            layout.removeRow(i)
        editor = {}
        ui = FreeCADGui.UiLoader()
        for name in tool.PropertiesList:
            if tool.getGroupOfProperty(name) == PathToolBit.PropertyGroupBit:
                qsb = ui.createWidget('Gui::QuantitySpinBox')
                editor[name] = PathGui.QuantitySpinBox(qsb, tool, name)
                label = QtGui.QLabel(re.sub('([A-Z][a-z]+)', r' \1', re.sub('([A-Z]+)', r' \1', name)))
                layout.addRow(label, qsb)
        self.bitEditor = editor
        img = tool.Proxy.getBitThumbnail(tool)
        if img:
            self.form.image.setPixmap(QtGui.QPixmap(QtGui.QImage.fromData(img)))
        else:
            self.form.image.setPixmap(QtGui.QPixmap())

    def setupAttributes(self, tool):
        self.proto = PathToolBit.AttributePrototype()
        self.props = sorted(self.proto.properties)
        self.delegate = PathSetupSheetGui.Delegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props)-1, 3, self.form)
        self.model.setHorizontalHeaderLabels(['Set', 'Property', 'Value'])

        for i, name in enumerate(self.props):
            print("propname: %s " % name)

            prop = self.proto.getProperty(name)
            isset = hasattr(tool, name)

            if isset:
                prop.setValue(getattr(tool, name))

            if name == "UserAttributes":
                continue

            else:

                self.model.setData(self.model.index(i, 0), isset, QtCore.Qt.EditRole)
                self.model.setData(self.model.index(i, 1), name,  QtCore.Qt.EditRole)
                self.model.setData(self.model.index(i, 2), prop,  PathSetupSheetGui.Delegate.PropertyRole)
                self.model.setData(self.model.index(i, 2), prop.displayString(),  QtCore.Qt.DisplayRole)

                self.model.item(i, 0).setCheckable(True)
                self.model.item(i, 0).setText('')
                self.model.item(i, 1).setEditable(False)
                self.model.item(i, 1).setToolTip(prop.info)
                self.model.item(i, 2).setToolTip(prop.info)

                if isset:
                    self.model.item(i, 0).setCheckState(QtCore.Qt.Checked)
                else:
                    self.model.item(i, 0).setCheckState(QtCore.Qt.Unchecked)
                    self.model.item(i, 1).setEnabled(False)
                    self.model.item(i, 2).setEnabled(False)

        if hasattr(tool, "UserAttributes"):
            for key, value in tool.UserAttributes.items():
                print(key, value)
                c1 = QtGui.QStandardItem()
                c1.setCheckable(False)
                c1.setEditable(False)
                c1.setCheckState(QtCore.Qt.CheckState.Checked)

                c1.setText('')
                c2 = QtGui.QStandardItem(key)
                c2.setEditable(False)
                c3 = QtGui.QStandardItem(value)
                c3.setEditable(False)

                self.model.appendRow([c1, c2, c3])

        self.form.attrTable.setModel(self.model)
        self.form.attrTable.setItemDelegateForColumn(2, self.delegate)
        self.form.attrTable.resizeColumnsToContents()
        self.form.attrTable.verticalHeader().hide()

        self.model.dataChanged.connect(self.updateData)

    def updateData(self, topLeft, bottomRight):
        # pylint: disable=unused-argument
        if 0 == topLeft.column():
            isset = self.model.item(topLeft.row(), 0).checkState() == QtCore.Qt.Checked
            self.model.item(topLeft.row(), 1).setEnabled(isset)
            self.model.item(topLeft.row(), 2).setEnabled(isset)

    def accept(self):
        self.refresh()
        self.tool.Proxy.unloadBitBody(self.tool)

        # get the attributes
        for i, name in enumerate(self.props):
            prop = self.proto.getProperty(name)
            enabled = self.model.item(i, 0).checkState() == QtCore.Qt.Checked
            if enabled and not prop.getValue() is None:
                prop.setupProperty(self.tool, name, PathToolBit.PropertyGroupAttribute, prop.getValue())
            elif hasattr(self.tool, name):
                self.tool.removeProperty(name)

    def reject(self):
        self.tool.Proxy.unloadBitBody(self.tool)

    def updateUI(self):
        PathLog.track()
        self.form.toolName.setText(self.tool.Label)
        self.form.shapePath.setText(self.tool.BitShape)

        for editor in self.bitEditor:
            self.bitEditor[editor].updateSpinBox()

    def updateShape(self):
        self.tool.BitShape = str(self.form.shapePath.text())
        self.setupTool(self.tool)
        self.form.toolName.setText(self.tool.Label)

        for editor in self.bitEditor:
            self.bitEditor[editor].updateSpinBox()

    def updateTool(self):
        # pylint: disable=protected-access
        PathLog.track()
        self.tool.Label = str(self.form.toolName.text())
        self.tool.BitShape = str(self.form.shapePath.text())

        for editor in self.bitEditor:
            self.bitEditor[editor].updateProperty()

        self.tool.Proxy._updateBitShape(self.tool)

    def refresh(self):
        PathLog.track()
        self.form.blockSignals(True)
        self.updateTool()
        self.updateUI()
        self.form.blockSignals(False)

    def selectShape(self):
        path = self.tool.BitShape
        if not path:
            path = PathPreferences.lastPathToolShape()
        foo = QtGui.QFileDialog.getOpenFileName(self.form,
                "Path - Tool Shape",
                path,
                "*.fcstd")
        if foo and foo[0]:
            PathPreferences.setLastPathToolShape(os.path.dirname(foo[0]))
            self.form.shapePath.setText(foo[0])
            self.updateShape()

    def setupUI(self):
        PathLog.track()
        self.updateUI()

        self.form.toolName.editingFinished.connect(self.refresh)
        self.form.shapePath.editingFinished.connect(self.updateShape)
        self.form.shapeSet.clicked.connect(self.selectShape)
