# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import FreeCADGui
import PathScripts.PathGui as PathGui
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathPropertyContainer as PathPropertyContainer
import PathScripts.PathPropertyEditor as PathPropertyEditor
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

__title__ = "Property Container Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Task panel editor for a PropertyContainer"

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider:
    '''ViewProvider for a PropertyContainer.
    It's sole job is to provide an icon and invoke the TaskPanel on edit.'''

    def __init__(self, vobj, name):
        PathLog.track(name)
        vobj.Proxy = self
        self.icon = name
        # mode = 2
        self.obj = None
        self.vobj = None

    def attach(self, vobj):
        PathLog.track()
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        return ":/icons/Path-SetupSheet.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        # pylint: disable=unused-argument
        return None

    def getDisplayMode(self, mode):
        # pylint: disable=unused-argument
        return 'Default'

    def setEdit(self, vobj, mode=0):
        # pylint: disable=unused-argument
        PathLog.track()
        taskPanel = TaskPanel(vobj)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(taskPanel)
        taskPanel.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        # pylint: disable=unused-argument
        FreeCADGui.Control.closeDialog()
        return

    def claimChildren(self):
        return []

    def doubleClicked(self, vobj):
        self.setEdit(vobj)

class Delegate(QtGui.QStyledItemDelegate):
    RoleObject   = QtCore.Qt.UserRole + 1
    RoleProperty = QtCore.Qt.UserRole + 2
    RoleEditor   = QtCore.Qt.UserRole + 3


    #def paint(self, painter, option, index):
    #    #PathLog.track(index.column(), type(option))

    def createEditor(self, parent, option, index):
        # pylint: disable=unused-argument
        editor = PathPropertyEditor.Editor(index.data(self.RoleObject), index.data(self.RoleProperty))
        index.model().setData(index, editor, self.RoleEditor)
        return editor.widget(parent)

    def setEditorData(self, widget, index):
        PathLog.track(index.row(), index.column())
        index.data(self.RoleEditor).setEditorData(widget)

    def setModelData(self, widget, model, index):
        # pylint: disable=unused-argument
        PathLog.track(index.row(), index.column())
        editor = index.data(self.RoleEditor)
        editor.setModelData(widget)
        index.model().setData(index, editor.displayString(), QtCore.Qt.DisplayRole)

    def updateEditorGeometry(self, widget, option, index):
        # pylint: disable=unused-argument
        widget.setGeometry(option.rect)

class TaskPanel:
    ColumnName = 0
    ColumnVal  = 1
    ColumnDesc = 2

    def __init__(self, vobj):
        self.obj = vobj.Object
        self.props = sorted(self.obj.Proxy.getCustomProperties())
        self.form = FreeCADGui.PySideUic.loadUi(":panels/PropertyContainer.ui")

        # initialized later
        self.delegate = None
        self.model = None
        FreeCAD.ActiveDocument.openTransaction(translate("PathPropertyContainer", "Edit PropertyContainer"))

    def updateData(self, topLeft, bottomRight):
        # pylint: disable=unused-argument
        #if 0 == topLeft.column():
        #    isset = self.model.item(topLeft.row(), 0).checkState() == QtCore.Qt.Checked
        #    self.model.item(topLeft.row(), 1).setEnabled(isset)
        #    self.model.item(topLeft.row(), 2).setEnabled(isset)
        print("index = ({}, {}) - ({}, {})".format(topLeft.row(), topLeft.column(), bottomRight.row(), bottomRight.column()))
        if topLeft.column() == self.ColumnDesc:
            obj  = topLeft.data(Delegate.RoleObject)
            prop = topLeft.data(Delegate.RoleProperty)

    def setupUi(self):
        PathLog.track()

        self.delegate = Delegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props), 3, self.form)
        self.model.setHorizontalHeaderLabels(['Property', 'Value', 'Description'])

        for i,name in enumerate(self.props):
            info  = self.obj.getDocumentationOfProperty(name)
            value = self.obj.getPropertyByName(name)
            self.model.setData(self.model.index(i, self.ColumnName), name,        QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, self.ColumnVal),  self.obj,    Delegate.RoleObject)
            self.model.setData(self.model.index(i, self.ColumnVal),  name,        Delegate.RoleProperty)
            self.model.setData(self.model.index(i, self.ColumnVal),  str(value),  QtCore.Qt.DisplayRole)
            self.model.setData(self.model.index(i, self.ColumnDesc), info,        QtCore.Qt.EditRole)

            self.model.item(i, self.ColumnName).setEditable(False)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegateForColumn(self.ColumnVal, self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)
        self.form.table.selectionModel().selectionChanged.connect(self.propertySelected)
        self.form.add.clicked.connect(self.propertyAdd)
        self.form.remove.clicked.connect(self.propertyRemove)
        self.propertySelected([])

    def accept(self):
        #propertiesCreatedRemoved = False
        #for i,name in enumerate(self.props):
        #    prop = self.prototype.getProperty(name)
        #    propName = self.propertyName(name)
        #    enabled = self.model.item(i, 0).checkState() == QtCore.Qt.Checked
        #    if enabled and not prop.getValue() is None:
        #        if prop.setupProperty(self.obj, propName, self.propertyGroup(), prop.getValue()):
        #            propertiesCreatedRemoved = True
        #    else:
        #        if hasattr(self.obj, propName):
        #            self.obj.removeProperty(propName)
        #            propertiesCreatedRemoved = True

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def propertySelected(self, selection):
        PathLog.track()
        if selection:
            self.form.remove.setEnabled(True)
        else:
            self.form.remove.setEnabled(False)

    def propertyAdd(self):
        PathLog.track()

    def propertyRemove(self):
        PathLog.track()
        rows = []
        for index in self.form.table.selectionModel().selectedIndexes():
            if not index.row() in rows:
                rows.append(index.row())
        for row in reversed(sorted(rows)):
            self.obj.removeProperty(self.model.item(row).data(QtCore.Qt.EditRole))
            self.model.removeRow(row)


def Create(name = 'PropertyContainer'):
    '''Create(name = 'PropertyContainer') ... creates a new setup sheet'''
    FreeCAD.ActiveDocument.openTransaction(translate("PathPropertyContainer", "Create PropertyContainer"))
    pcont = PathPropertyContainer.Create(name)
    PathIconViewProvider.Attach(pcont.ViewObject, name)
    return pcont

PathIconViewProvider.RegisterViewProvider('PropertyContainer', ViewProvider)

