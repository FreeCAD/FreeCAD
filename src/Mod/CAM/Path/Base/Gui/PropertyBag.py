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

from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.IconViewProvider as PathIconViewProvider
import Path.Base.Gui.PropertyEditor as PathPropertyEditor
import Path.Base.PropertyBag as PathPropertyBag
import Path.Base.Util as PathUtil
import re


__title__ = "Property Bag Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Task panel editor for a PropertyBag"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ViewProvider(object):
    """ViewProvider for a PropertyBag.
    It's sole job is to provide an icon and invoke the TaskPanel on edit."""

    def __init__(self, vobj, name):
        Path.Log.track(name)
        vobj.Proxy = self
        self.icon = name
        # mode = 2
        self.obj = None
        self.vobj = None

    def attach(self, vobj):
        Path.Log.track()
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        return ":/icons/Path-SetupSheet.svg"

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getDisplayMode(self, mode):
        return "Default"

    def setEdit(self, vobj, mode=0):
        Path.Log.track()
        taskPanel = TaskPanel(vobj)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(taskPanel)
        taskPanel.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def claimChildren(self):
        return []

    def doubleClicked(self, vobj):
        self.setEdit(vobj)


class Delegate(QtGui.QStyledItemDelegate):
    RoleObject = QtCore.Qt.UserRole + 1
    RoleProperty = QtCore.Qt.UserRole + 2
    RoleEditor = QtCore.Qt.UserRole + 3

    # def paint(self, painter, option, index):
    #    #Path.Log.track(index.column(), type(option))

    def createEditor(self, parent, option, index):
        editor = PathPropertyEditor.Editor(
            index.data(self.RoleObject), index.data(self.RoleProperty)
        )
        index.model().setData(index, editor, self.RoleEditor)
        return editor.widget(parent)

    def setEditorData(self, widget, index):
        Path.Log.track(index.row(), index.column())
        index.data(self.RoleEditor).setEditorData(widget)

    def setModelData(self, widget, model, index):
        Path.Log.track(index.row(), index.column())
        editor = index.data(self.RoleEditor)
        editor.setModelData(widget)
        index.model().setData(index, editor.displayString(), QtCore.Qt.DisplayRole)

    def updateEditorGeometry(self, widget, option, index):
        widget.setGeometry(option.rect)


class PropertyCreate(object):
    def __init__(self, obj, grp, typ, another):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":panels/PropertyCreate.ui")

        obj.Proxy.refreshCustomPropertyGroups()
        for g in sorted(obj.CustomPropertyGroups):
            self.form.propertyGroup.addItem(g)
        if grp:
            self.form.propertyGroup.setCurrentText(grp)

        for t in sorted(PathPropertyBag.SupportedPropertyType):
            self.form.propertyType.addItem(t)
            if PathPropertyBag.SupportedPropertyType[t] == typ:
                typ = t
        if typ:
            self.form.propertyType.setCurrentText(typ)
        else:
            self.form.propertyType.setCurrentText("String")
        self.form.createAnother.setChecked(another)

        self.form.propertyGroup.currentTextChanged.connect(self.updateUI)
        self.form.propertyGroup.currentIndexChanged.connect(self.updateUI)
        self.form.propertyName.textChanged.connect(self.updateUI)
        self.form.propertyType.currentIndexChanged.connect(self.updateUI)
        self.form.propertyEnum.textChanged.connect(self.updateUI)

    def updateUI(self):

        typeSet = True
        if self.propertyIsEnumeration():
            self.form.labelEnum.setEnabled(True)
            self.form.propertyEnum.setEnabled(True)
            typeSet = self.form.propertyEnum.text().strip() != ""
        else:
            self.form.labelEnum.setEnabled(False)
            self.form.propertyEnum.setEnabled(False)
            if self.form.propertyEnum.text().strip():
                self.form.propertyEnum.setText("")

        ok = self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok)

        if not re.match("^[A-Za-z0-9_]*$", self.form.propertyName.text()):
            typeSet = False

        if typeSet and self.propertyGroup():
            ok.setEnabled(True)
        else:
            ok.setEnabled(False)

    def propertyName(self):
        return self.form.propertyName.text().strip()

    def propertyGroup(self):
        return self.form.propertyGroup.currentText().strip()

    def propertyType(self):
        return PathPropertyBag.SupportedPropertyType[
            self.form.propertyType.currentText()
        ].strip()

    def propertyInfo(self):
        return self.form.propertyInfo.toPlainText().strip()

    def createAnother(self):
        return self.form.createAnother.isChecked()

    def propertyEnumerations(self):
        return [s.strip() for s in self.form.propertyEnum.text().strip().split(",")]

    def propertyIsEnumeration(self):
        return self.propertyType() == "App::PropertyEnumeration"

    def exec_(self, name):
        if name:
            # property exists - this is an edit operation
            self.form.propertyName.setText(name)
            if self.propertyIsEnumeration():
                self.form.propertyEnum.setText(
                    ",".join(self.obj.getEnumerationsOfProperty(name))
                )
            self.form.propertyInfo.setText(self.obj.getDocumentationOfProperty(name))

            self.form.labelName.setEnabled(False)
            self.form.propertyName.setEnabled(False)
            self.form.labelType.setEnabled(False)
            self.form.propertyType.setEnabled(False)
            self.form.createAnother.setEnabled(False)

        else:
            self.form.propertyName.setText("")
            self.form.propertyInfo.setText("")
            self.form.propertyEnum.setText("")
            # self.form.propertyName.setFocus()

        self.updateUI()

        return self.form.exec_()


class TaskPanel(object):
    ColumnName = 0
    # ColumnType = 1
    ColumnVal = 1
    # TableHeaders = ['Property', 'Type', 'Value']
    TableHeaders = ["Property", "Value"]

    def __init__(self, vobj):
        self.obj = vobj.Object
        self.props = sorted(self.obj.Proxy.getCustomProperties())
        self.form = FreeCADGui.PySideUic.loadUi(":panels/PropertyBag.ui")

        # initialized later
        self.model = None
        self.delegate = None
        FreeCAD.ActiveDocument.openTransaction("Edit PropertyBag")

    def updateData(self, topLeft, bottomRight):
        pass

    def _setupProperty(self, i, name):
        typ = PathPropertyBag.getPropertyTypeName(self.obj.getTypeIdOfProperty(name))
        val = PathUtil.getPropertyValueString(self.obj, name)
        info = self.obj.getDocumentationOfProperty(name)

        self.model.setData(
            self.model.index(i, self.ColumnName), name, QtCore.Qt.EditRole
        )
        # self.model.setData(self.model.index(i, self.ColumnType), typ,       QtCore.Qt.EditRole)
        self.model.setData(
            self.model.index(i, self.ColumnVal), self.obj, Delegate.RoleObject
        )
        self.model.setData(
            self.model.index(i, self.ColumnVal), name, Delegate.RoleProperty
        )
        self.model.setData(
            self.model.index(i, self.ColumnVal), val, QtCore.Qt.DisplayRole
        )

        self.model.setData(
            self.model.index(i, self.ColumnName), typ, QtCore.Qt.ToolTipRole
        )
        # self.model.setData(self.model.index(i, self.ColumnType), info,      QtCore.Qt.ToolTipRole)
        self.model.setData(
            self.model.index(i, self.ColumnVal), info, QtCore.Qt.ToolTipRole
        )

        self.model.item(i, self.ColumnName).setEditable(False)
        # self.model.item(i, self.ColumnType).setEditable(False)

    def setupUi(self):
        Path.Log.track()

        self.delegate = Delegate(self.form)
        self.model = QtGui.QStandardItemModel(
            len(self.props), len(self.TableHeaders), self.form
        )
        self.model.setHorizontalHeaderLabels(self.TableHeaders)

        for i, name in enumerate(self.props):
            self._setupProperty(i, name)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegateForColumn(self.ColumnVal, self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)
        self.form.table.selectionModel().selectionChanged.connect(self.propertySelected)
        self.form.add.clicked.connect(self.propertyAdd)
        self.form.remove.clicked.connect(self.propertyRemove)
        self.form.modify.clicked.connect(self.propertyModify)
        self.form.table.doubleClicked.connect(self.propertyModifyIndex)
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
        Path.Log.track()
        if selection:
            self.form.modify.setEnabled(True)
            self.form.remove.setEnabled(True)
        else:
            self.form.modify.setEnabled(False)
            self.form.remove.setEnabled(False)

    def addCustomProperty(self, obj, dialog):
        name = dialog.propertyName()
        typ = dialog.propertyType()
        grp = dialog.propertyGroup()
        info = dialog.propertyInfo()
        if name:
            propname = self.obj.Proxy.addCustomProperty(typ, name, grp, info)
            if dialog.propertyIsEnumeration():
                setattr(self.obj, name, dialog.propertyEnumerations())
            return (propname, info)
        return (None, None)

    def propertyAdd(self):
        Path.Log.track()
        more = False
        grp = None
        typ = None
        while True:
            dialog = PropertyCreate(self.obj, grp, typ, more)
            if dialog.exec_(None):
                # if we block signals the view doesn't get updated, surprise, surprise
                # self.model.blockSignals(True)
                name, info = self.addCustomProperty(self.obj, dialog)
                if name:
                    index = 0
                    for i in range(self.model.rowCount()):
                        index = i
                        if (
                            self.model.item(i, self.ColumnName).data(QtCore.Qt.EditRole)
                            > dialog.propertyName()
                        ):
                            break
                    self.model.insertRows(index, 1)
                    self._setupProperty(index, name)
                    self.form.table.selectionModel().setCurrentIndex(
                        self.model.index(index, 0), QtCore.QItemSelectionModel.Rows
                    )
                    # self.model.blockSignals(False)
                    more = dialog.createAnother()
                else:
                    more = False
            else:
                more = False
            if not more:
                break

    def propertyModifyIndex(self, index):
        Path.Log.track(index.row(), index.column())
        row = index.row()

        obj = self.model.item(row, self.ColumnVal).data(Delegate.RoleObject)
        prop = self.model.item(row, self.ColumnVal).data(Delegate.RoleProperty)
        grp = obj.getGroupOfProperty(prop)
        typ = obj.getTypeIdOfProperty(prop)

        dialog = PropertyCreate(self.obj, grp, typ, False)
        if dialog.exec_(prop):
            val = getattr(obj, prop)
            obj.removeProperty(prop)
            name, info = self.addCustomProperty(self.obj, dialog)
            try:
                setattr(obj, prop, val)
            except Exception:
                # this can happen if the old enumeration value doesn't exist anymore
                pass
            newVal = PathUtil.getPropertyValueString(obj, prop)
            self.model.setData(
                self.model.index(row, self.ColumnVal), newVal, QtCore.Qt.DisplayRole
            )

            # self.model.setData(self.model.index(row, self.ColumnType), info, QtCore.Qt.ToolTipRole)
            self.model.setData(
                self.model.index(row, self.ColumnVal), info, QtCore.Qt.ToolTipRole
            )

    def propertyModify(self):
        Path.Log.track()
        rows = []
        for index in self.form.table.selectionModel().selectedIndexes():
            row = index.row()
            if row in rows:
                continue
            rows.append(row)

            self.propertyModifyIndex(index)

    def propertyRemove(self):
        Path.Log.track()
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


def Create(name="PropertyBag"):
    """Create(name = 'PropertyBag') ... creates a new setup sheet"""
    FreeCAD.ActiveDocument.openTransaction("Create PropertyBag")
    pcont = PathPropertyBag.Create(name)
    PathIconViewProvider.Attach(pcont.ViewObject, name)
    return pcont


PathIconViewProvider.RegisterViewProvider("PropertyBag", ViewProvider)


class PropertyBagCreateCommand(object):
    """Command to create a property container object"""

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "MenuText": translate("CAM_PropertyBag", "PropertyBag"),
            "ToolTip": translate(
                "CAM_PropertyBag",
                "Creates an object which can be used to store reference properties.",
            ),
        }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        obj = Create()
        body = None
        if sel:
            if "PartDesign::Body" == sel[0].Object.TypeId:
                body = sel[0].Object
            elif hasattr(sel[0].Object, "getParentGeoFeatureGroup"):
                body = sel[0].Object.getParentGeoFeatureGroup()
        if body:
            obj.Label = "Attributes"
            group = body.Group
            group.append(obj)
            body.Group = group


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_PropertyBag", PropertyBagCreateCommand())

FreeCAD.Console.PrintLog("Loading PathPropertyBagGui ... done\n")
