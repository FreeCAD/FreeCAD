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

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ViewProvider(object):
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

class PropertyCreate(object):

    def __init__(self, obj, grp, typ, another):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":panels/PropertyCreate.ui")

        for g in sorted(obj.CustomPropertyGroups):
            self.form.propertyGroup.addItem(g)
        if grp:
            self.form.propertyGroup.setCurrentText(grp)

        for t in sorted(PathPropertyContainer.SupportedPropertyType):
            self.form.propertyType.addItem(t)
            if PathPropertyContainer.SupportedPropertyType[t] == typ:
                typ = t
        if typ:
            self.form.propertyType.setCurrentText(typ)
        else:
            self.form.propertyType.setCurrentText('String')
        self.form.createAnother.setChecked(another)

        self.form.propertyGroup.currentTextChanged.connect(self.updateUI)
        self.form.propertyGroup.currentIndexChanged.connect(self.updateUI)
        self.form.propertyName.textChanged.connect(self.updateUI)

        self.updateUI()

    def updateUI(self):
        ok = self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok)
        if self.form.propertyName.text() and self.form.propertyGroup.currentText():
            ok.setEnabled(True)
        else:
            ok.setEnabled(False)

    def propertyName(self):
        return self.form.propertyName.text().strip()
    def propertyGroup(self):
        return self.form.propertyGroup.currentText().strip()
    def propertyType(self):
        return PathPropertyContainer.SupportedPropertyType[self.form.propertyType.currentText()].strip()
    def propertyInfo(self):
        return self.form.propertyInfo.toPlainText().strip()
    def createAnother(self):
        return self.form.createAnother.isChecked()

    def exec_(self):
        self.form.propertyName.setText('')
        self.form.propertyInfo.setText('')
        #self.form.propertyName.setFocus()
        return self.form.exec_()

class TaskPanel(object):
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
        if topLeft.column() == self.ColumnDesc:
            obj  = topLeft.data(Delegate.RoleObject)
            prop = topLeft.data(Delegate.RoleProperty)


    def _setupProperty(self, i, name):
        info  = self.obj.getDocumentationOfProperty(name)
        value = self.obj.getPropertyByName(name)

        self.model.setData(self.model.index(i, self.ColumnName), name,        QtCore.Qt.EditRole)
        self.model.setData(self.model.index(i, self.ColumnVal),  self.obj,    Delegate.RoleObject)
        self.model.setData(self.model.index(i, self.ColumnVal),  name,        Delegate.RoleProperty)
        self.model.setData(self.model.index(i, self.ColumnVal),  str(value),  QtCore.Qt.DisplayRole)
        self.model.setData(self.model.index(i, self.ColumnDesc), info,        QtCore.Qt.EditRole)

        self.model.item(i, self.ColumnName).setEditable(False)

    def setupUi(self):
        PathLog.track()

        self.delegate = Delegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props), 3, self.form)
        self.model.setHorizontalHeaderLabels(['Property', 'Value', 'Description'])

        for i,name in enumerate(self.props):
            self._setupProperty(i, name)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegateForColumn(self.ColumnVal, self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)
        self.form.table.selectionModel().selectionChanged.connect(self.propertySelected)
        self.form.add.clicked.connect(self.propertyAdd)
        self.form.remove.clicked.connect(self.propertyRemove)
        self.propertySelected([])

    def accept(self):
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
        more = False
        grp = None
        typ = None
        while True:
            dialog = PropertyCreate(self.obj, grp, typ, more)
            if dialog.exec_():
                # if we block signals the view doesn't get updated, surprise, surprise
                #self.model.blockSignals(True)
                name = dialog.propertyName()
                typ  = dialog.propertyType()
                grp  = dialog.propertyGroup()
                info = dialog.propertyInfo()
                self.obj.Proxy.addCustomProperty(typ, name, grp, info)
                index = 0
                for i in range(self.model.rowCount()):
                    index = i
                    if self.model.item(i, self.ColumnName).data(QtCore.Qt.EditRole) > dialog.propertyName():
                        break
                self.model.insertRows(index, 1)
                self._setupProperty(index, name)
                self.form.table.selectionModel().setCurrentIndex(self.model.index(index, 0), QtCore.QItemSelectionModel.Rows)
                #self.model.blockSignals(False)
                more = dialog.createAnother()
            else:
                more = False
            if not more:
                break

    def propertyRemove(self):
        PathLog.track()
        # first find all rows which need to be removed
        rows = []
        for index in self.form.table.selectionModel().selectedIndexes():
            if not index.row() in rows:
                rows.append(index.row())

        # then remove them in reverse order so the indexes of the remaining rows
        # to delete are still valid
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

class PropertyContainerCreateCommand(object):
    '''Command to create a property container object'''

    def __init__(self):
        pass

    def GetResources(self):
        return {'MenuText': translate('PathPropertyContainer', 'Property Container'),
                'ToolTip': translate('PathPropertyContainer', 'Creates an object which can be used to store reference properties.')}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        obj = Create()
        body = None
        if sel:
            if 'PartDesign::Body' == sel[0].Object.TypeId:
                body = sel[0].Object
            elif hasattr(sel[0].Object, 'getParentGeoFeatureGroup'):
                body = sel[0].Object.getParentGeoFeatureGroup()
        if body:
            obj.Label = 'Attributes'
            group = body.Group
            group.append(obj)
            body.Group = group

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_PropertyContainer', PropertyContainerCreateCommand())

FreeCAD.Console.PrintLog("Loading PathPropertyContainerGui ... done\n")
