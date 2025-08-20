# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This module contains FreeCAD commands for the BIM workbench"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_IfcElements:

    def GetResources(self):
        return {
            "Pixmap": "BIM_IfcElements",
            "MenuText": QT_TRANSLATE_NOOP("BIM_IfcElements", "Manage IFC Elements"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_IfcElements",
                "Manages how the different elements of the BIM project will be exported to IFC",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Draft
        from PySide import QtGui

        # build objects list
        self.objectslist = {}
        try:
            import ArchIFC

            self.ifctypes = ArchIFC.IfcTypes
        except (ImportError, AttributeError):
            import ArchComponent

            self.ifctypes = ArchComponent.IfcRoles
        for obj in FreeCAD.ActiveDocument.Objects:
            mat = ""
            role = self.getRole(obj)
            if role:
                try:
                    mat = obj.Material.Name
                except AttributeError:
                    mat = ""
                self.objectslist[obj.Name] = [role, mat]

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogIfcElements.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_IfcElements.svg"))
        self.model = QtGui.QStandardItemModel()
        self.form.tree.setModel(self.model)
        self.form.tree.setUniformRowHeights(True)
        self.form.tree.setItemDelegate(IfcElementsDelegate(dialog=self))
        self.form.globalMode.addItems([" "] + self.ifctypes)
        self.form.groupMode.setItemIcon(2, QtGui.QIcon(":/icons/Arch_Material.svg"))
        self.form.groupMode.setItemIcon(3, QtGui.QIcon(":/icons/Document.svg"))
        self.form.globalMaterial.addItem(" ")
        self.form.globalMaterial.addItem(translate("BIM", "Create new material"))
        self.form.globalMaterial.addItem(translate("BIM", "Create new multi-material"))
        self.materials = []
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("App::MaterialObject") or (
                Draft.getType(o) == "MultiMaterial"
            ):
                self.materials.append(o.Name)
                self.form.globalMaterial.addItem(
                    o.Label, QtGui.QIcon(":/icons/Arch_Material.svg")
                )
        self.form.groupMode.currentIndexChanged.connect(self.update)
        self.form.tree.clicked.connect(self.onClickTree)
        if hasattr(self.form.onlyVisible, "checkStateChanged"): # Qt version >= 6.7.0
            self.form.onlyVisible.checkStateChanged.connect(self.update)
        else: # Qt version < 6.7.0
            self.form.onlyVisible.stateChanged.connect(self.update)
        self.form.buttonBox.accepted.connect(self.accept)
        self.form.globalMode.currentIndexChanged.connect(self.onObjectTypeChanged)
        self.form.globalMaterial.currentIndexChanged.connect(self.onMaterialChanged)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        self.update()
        self.form.show()

    def update(self, index=None):
        "updates the tree widgets in all tabs"

        # store current state of tree into self.objectslist before redrawing
        for row in range(self.model.rowCount()):
            name = self.model.item(row, 0).toolTip()
            mat = self.model.item(row, 2).toolTip()
            if name:
                self.objectslist[name] = [self.model.item(row, 1).text(), mat]
            if self.model.item(row, 0).hasChildren():
                for childrow in range(self.model.item(row, 0).rowCount()):
                    name = self.model.item(row, 0).child(childrow, 0).toolTip()
                    mat = self.model.item(row, 0).child(childrow, 2).toolTip()
                    if name:
                        self.objectslist[name] = [
                            self.model.item(row, 0).child(childrow, 1).text(),
                            mat,
                        ]
        self.model.clear()
        self.model.setHorizontalHeaderLabels(
            [
                translate("BIM", "Label"),
                translate("BIM", "IFC type"),
                translate("BIM", "Material"),
            ]
        )
        # self.form.tree.header().setResizeMode(QtGui.QHeaderView.Stretch)
        # self.form.tree.resizeColumnsToContents()

        if self.form.groupMode.currentIndex() == 1:
            # group by type
            self.updateByType()
        elif self.form.groupMode.currentIndex() == 2:
            # group by material
            self.updateByMaterial()
        elif self.form.groupMode.currentIndex() == 3:
            # group by model structure
            self.updateByTree()
        else:
            # group alphabetically
            self.updateDefault()
        self.model.sort(0)

    def updateByType(self):
        groups = {}
        for name, rolemat in self.objectslist.items():
            role = rolemat[0]
            mat = rolemat[1]
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    not self.form.onlyVisible.isChecked()
                ) or obj.ViewObject.isVisible():
                    groups.setdefault(role, []).append([name, mat])
        for group in groups.keys():
            s1 = group + " (" + str(len(groups[group])) + ")"
            top = QtGui.QStandardItem(s1)
            self.model.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
            for name, mat in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    it1 = QtGui.QStandardItem(obj.Label)
                    it1.setIcon(getIcon(obj))
                    it1.setToolTip(obj.Name)
                    it2 = QtGui.QStandardItem(group)
                    if group != self.getRole(obj):
                        it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    matlabel = ""
                    if mat:
                        matobj = FreeCAD.ActiveDocument.getObject(mat)
                        if matobj:
                            matlabel = matobj.Label
                    it3 = QtGui.QStandardItem(matlabel)
                    it3.setToolTip(mat)
                    omat = ""
                    if hasattr(obj, "Material") and obj.Material:
                        omat = obj.Material.Name
                        if omat != mat:
                            it3.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    top.appendRow([it1, it2, it3])
            top.sortChildren(0)
        self.form.tree.expandAll()
        self.spanTopLevels()

    def updateByMaterial(self):
        groups = {}
        for name, rolemat in self.objectslist.items():
            role = rolemat[0]
            mat = rolemat[1]
            if not mat:
                mat = "Undefined"
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    not self.form.onlyVisible.isChecked()
                ) or obj.ViewObject.isVisible():
                    groups.setdefault(mat, []).append([name, role])

        for group in groups.keys():
            grlabel = "Undefined"
            if group != "Undefined":
                matobj = FreeCAD.ActiveDocument.getObject(group)
                if matobj:
                    grlabel = matobj.Label
            s1 = grlabel + " (" + str(len(groups[group])) + ")"
            top = QtGui.QStandardItem(s1)
            self.model.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
            for name, role in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    it1 = QtGui.QStandardItem(obj.Label)
                    it1.setIcon(getIcon(obj))
                    it1.setToolTip(obj.Name)
                    it2 = QtGui.QStandardItem(role)
                    if role != self.getRole(obj):
                        it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    mat = ""
                    matlabel = ""
                    if group != "Undefined":
                        matobj = FreeCAD.ActiveDocument.getObject(group)
                        if matobj:
                            matlabel = matobj.Label
                            mat = matobj.Name
                    it3 = QtGui.QStandardItem(matlabel)
                    it3.setToolTip(mat)
                    omat = ""
                    if hasattr(obj, "Material") and obj.Material:
                        omat = obj.Material.Name
                        if omat != mat:
                            it3.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    top.appendRow([it1, it2, it3])
            top.sortChildren(0)
        self.form.tree.expandAll()
        self.spanTopLevels()

    def updateByTree(self):
        # order by hierarchy
        def istop(obj):
            for parent in obj.InListRecursive:
                if parent.Name in self.objectslist.keys():
                    return False
            return True

        rel = []
        deps = []
        for name in self.objectslist.keys():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if istop(obj):
                    rel.append(obj)
                else:
                    deps.append(obj)
        pa = 1
        while deps:
            for obj in rel:
                for child in obj.OutList:
                    if child in deps:
                        rel.append(child)
                        deps.remove(child)
            pa += 1
            if pa == 10:  # max 10 hierarchy levels, okay? Let's keep civilised
                rel.extend(deps)
                break

        # print "rel:",[o.Label for o in rel]

        done = {}
        for obj in rel:
            rolemat = self.objectslist[obj.Name]
            role = rolemat[0]
            mat = rolemat[1]

            if (not self.form.onlyVisible.isChecked()) or obj.ViewObject.isVisible():
                it1 = QtGui.QStandardItem(obj.Label)
                it1.setIcon(getIcon(obj))
                it1.setToolTip(obj.Name)
                it2 = QtGui.QStandardItem(role)
                if role != self.getRole(obj):
                    it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                matlabel = ""
                if mat:
                    matobj = FreeCAD.ActiveDocument.getObject(mat)
                    if matobj:
                        matlabel = matobj.Label
                else:
                    mat = ""
                it3 = QtGui.QStandardItem(matlabel)
                it3.setToolTip(mat)
                omat = ""
                if hasattr(obj, "Material") and obj.Material:
                    omat = obj.Material.Name
                    if omat != mat:
                        it3.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                ok = False
                for par in obj.InListRecursive:
                    if par.Name in done:
                        if (not hasattr(par, "Hosts")) or (obj not in par.Hosts):
                            done[par.Name].appendRow([it1, it2, it3])
                            done[obj.Name] = it1
                            ok = True
                            break
                if not ok:
                    self.model.appendRow([it1, it2, it3])
                    done[obj.Name] = it1
        self.form.tree.expandAll()

    def updateDefault(self):
        for name, rolemat in self.objectslist.items():
            role = rolemat[0]
            mat = rolemat[1]
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    not self.form.onlyVisible.isChecked()
                ) or obj.ViewObject.isVisible():
                    it1 = QtGui.QStandardItem(obj.Label)
                    it1.setIcon(getIcon(obj))
                    it1.setToolTip(obj.Name)
                    it2 = QtGui.QStandardItem(role)
                    if role != self.getRole(obj):
                        it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    matlabel = ""
                    if mat:
                        matobj = FreeCAD.ActiveDocument.getObject(mat)
                        if matobj:
                            matlabel = matobj.Label
                    else:
                        mat = ""
                    it3 = QtGui.QStandardItem(matlabel)
                    it3.setToolTip(mat)
                    omat = ""
                    if hasattr(obj, "Material") and obj.Material:
                        omat = obj.Material.Name
                        if omat != mat:
                            it3.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    self.model.appendRow([it1, it2, it3])

    def spanTopLevels(self):
        if self.form.groupMode.currentIndex() in [1, 2]:
            idx = self.model.invisibleRootItem().index()
            for i in range(self.model.rowCount()):
                if self.model.item(i, 0).hasChildren():
                    self.form.tree.setFirstColumnSpanned(i, idx, True)

    def getRole(self, obj):
        if hasattr(obj, "IfcType"):
            return obj.IfcType
        elif hasattr(obj, "IfcRole"):
            return obj.IfcRole
        else:
            return None

    def onClickTree(self, index=None):
        FreeCADGui.Selection.clearSelection()
        sel = self.form.tree.selectedIndexes()
        mode = None
        mat = None
        for index in sel:
            if index.column() == 0:
                obj = FreeCAD.ActiveDocument.getObject(
                    self.model.itemFromIndex(index).toolTip()
                )
                if obj:
                    FreeCADGui.Selection.addSelection(obj)

        for index in sel:
            if index.column() == 1:
                if index.data() in self.ifctypes:
                    if mode:
                        if index.data() != mode:
                            mode = None
                            break
                    else:
                        mode = index.data()
        for index in sel:
            if index.column() == 2:
                item = self.model.itemFromIndex(index)
                m = FreeCAD.ActiveDocument.getObject(item.toolTip())
                if mat:
                    if m != mat:
                        mat = None
                        break
                else:
                    mat = m
        if mode:
            self.form.globalMode.setCurrentIndex(self.ifctypes.index(mode) + 1)
        else:
            self.form.globalMode.setCurrentIndex(0)
        if mat:
            self.form.globalMaterial.setCurrentIndex(self.materials.index(mat.Name) + 3)
        else:
            self.form.globalMaterial.setCurrentIndex(0)

    def onObjectTypeChanged(self, index=-1):
        changed = False
        if index >= 1:
            role = self.ifctypes[index - 1]
            sel = self.form.tree.selectedIndexes()
            for index in sel:
                if index.column() == 1:
                    if role:
                        if index.data() != role:
                            self.model.setData(index, role)
                            changed = True
        if changed:
            self.update()

    def onMaterialChanged(self, index=-1):
        changed = False
        if index == 1:
            FreeCADGui.runCommand("Arch_Material")
            QtCore.QTimer.singleShot(1000, self.checkMatChanged)
        elif index == 2:
            FreeCADGui.runCommand("Arch_MultiMaterial")
            QtCore.QTimer.singleShot(1000, self.checkMatChanged)
        elif index >= 3:
            mat = self.materials[index - 3]
            sel = self.form.tree.selectedIndexes()
            for index in sel:
                if index.column() == 2:
                    if mat:
                        mobj = FreeCAD.ActiveDocument.getObject(mat)
                        if mobj:
                            item = self.model.itemFromIndex(index)
                            if item.toolTip() != mat:
                                item.setText(mobj.Label)
                                item.setToolTip(mat)
                                changed = True
        if changed:
            self.update()

    def checkMatChanged(self):
        import Draft
        from PySide import QtCore, QtGui

        if FreeCADGui.Control.activeDialog():
            QtCore.QTimer.singleShot(500, self.checkMatChanged)
        else:
            mats = [
                o.Name
                for o in FreeCAD.ActiveDocument.Objects
                if (
                    o.isDerivedFrom("App::MaterialObject")
                    or (Draft.getType(o) == "MultiMaterial")
                )
            ]
            if len(mats) != len(self.materials):
                newmats = [m for m in mats if not m in self.materials]
                self.materials = mats
                self.form.globalMaterial.clear()
                self.form.globalMaterial.addItem(" ")
                self.form.globalMaterial.addItem(
                    translate("BIM", "Create new material")
                )
                self.form.globalMaterial.addItem(
                    translate("BIM", "Create new multi-material")
                )
                for m in self.materials:
                    o = FreeCAD.ActiveDocument.getObject(m)
                    if o:
                        self.form.globalMaterial.addItem(
                            o.Label, QtGui.QIcon(":/icons/Arch_Material.svg")
                        )
                changed = False
                sel = self.form.tree.selectedIndexes()
                for index in sel:
                    if index.column() == 2:
                        for mat in newmats:
                            mobj = FreeCAD.ActiveDocument.getObject(mat)
                            if mobj:
                                item = self.model.itemFromIndex(index)
                                if item.toolTip() != mat:
                                    item.setText(mobj.Label)
                                    item.setToolTip(mat)
                                    changed = True
                if changed:
                    self.update()

    def accept(self):
        # get current state of tree

        self.form.hide()
        for row in range(self.model.rowCount()):
            name = self.model.item(row, 0).toolTip()
            mat = self.model.item(row, 2).toolTip()
            if name:
                self.objectslist[name] = [self.model.item(row, 1).text(), mat]
            if self.model.item(row, 0).hasChildren():
                for childrow in range(self.model.item(row, 0).rowCount()):
                    name = self.model.item(row, 0).child(childrow, 0).toolTip()
                    mat = self.model.item(row, 0).child(childrow, 2).toolTip()
                    if name:
                        self.objectslist[name] = [
                            self.model.item(row, 0).child(childrow, 1).text(),
                            mat,
                        ]
        changed = False
        for name, rolemat in self.objectslist.items():
            role = rolemat[0]
            mat = rolemat[1]
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if hasattr(obj, "IfcRole") and (obj.IfcRole != role):
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Change IFC role")
                        changed = True
                    obj.IfcRole = role
                elif hasattr(obj, "IfcType") and (obj.IfcType != role):
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Change IFC type")
                        changed = True
                    obj.IfcType = role
                if mat and hasattr(obj, "Material"):
                    mobj = FreeCAD.ActiveDocument.getObject(mat)
                    if mobj:
                        if obj.Material:
                            if obj.Material.Name != mat:
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction(
                                        "Change material"
                                    )
                                    changed = True
                                obj.Material = mobj
                        else:
                            if not changed:
                                FreeCAD.ActiveDocument.openTransaction(
                                    "Change material"
                                )
                                changed = True
                            obj.Material = mobj

        if changed:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui

    class IfcElementsDelegate(QtGui.QStyledItemDelegate):
        def __init__(self, parent=None, dialog=None, *args):
            import Arch_rc

            try:
                import ArchIFC

                self.roles = ArchIFC.IfcTypes
            except (ImportError, AttributeError):
                import ArchComponent

                self.roles = ArchComponent.IfcRoles
            self.mats = []
            self.matlabels = []
            for o in FreeCAD.ActiveDocument.Objects:
                if o.isDerivedFrom("App::MaterialObject"):
                    self.mats.append(o.Name)
                    self.matlabels.append(o.Label)
            self.dialog = dialog
            self.btn = QtGui.QPushButton()
            self.btn.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
            self.btn.setText("")
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)

        def paint(self, painter, option, index):
            # not used - ugly and fake!
            if index.column() == 3:
                self.btn.setGeometry(option.rect)
                if option.state == QtGui.QStyle.State_Selected:
                    painter.fillRect(option.rect, option.palette.highlight())
                p = QtGui.QPixmap.grabWidget(self.btn)
                painter.drawPixmap(option.rect.x(), option.rect.y(), p)
            else:
                QtGui.QStyledItemDelegate.paint(self, painter, option, index)

        def createEditor(self, parent, option, index):
            if index.column() > 0:
                editor = QtGui.QComboBox(parent)
            else:
                editor = QtGui.QLineEdit(parent)
            return editor

        def setEditorData(self, editor, index):
            if index.column() == 1:
                idx = -1
                editor.addItems(self.roles)
                if index.data() in self.roles:
                    idx = self.roles.index(index.data())
                editor.setCurrentIndex(idx)
            elif index.column() == 2:
                idx = -1
                editor.addItems(self.matlabels)
                item = index.model().itemFromIndex(index)
                if item.toolTip() in self.mats:
                    idx = self.mats.index(item.toolTip())
                editor.setCurrentIndex(idx)
            else:
                editor.setText(index.data())

        def setModelData(self, editor, model, index):
            if index.column() == 1:
                if editor.currentIndex() == -1:
                    model.setData(index, "")
                else:
                    model.setData(index, self.roles[editor.currentIndex()])
            elif index.column() == 2:
                if editor.currentIndex() > -1:
                    idx = editor.currentIndex()
                    item = model.itemFromIndex(index)
                    item.setText(self.matlabels[idx])
                    item.setToolTip(self.mats[idx])
            else:
                model.setData(index, editor.text())
                item = model.itemFromIndex(index)
                obj = FreeCAD.ActiveDocument.getObject(item.toolTip())
                if obj:
                    obj.Label = editor.text()
            self.dialog.update()


def getIcon(obj):
    """returns a QIcon for an object"""

    from PySide import QtGui
    import Arch_rc

    if hasattr(obj.ViewObject, "Icon"):
        return obj.ViewObject.Icon
    elif hasattr(obj.ViewObject, "Proxy") and hasattr(obj.ViewObject.Proxy, "getIcon"):
        icon = obj.ViewObject.Proxy.getIcon()
        if icon.startswith("/*"):
            return QtGui.QIcon(QtGui.QPixmap(icon))
        else:
            return QtGui.QIcon(icon)
    else:
        return QtGui.QIcon(":/icons/Arch_Component.svg")


FreeCADGui.addCommand("BIM_IfcElements", BIM_IfcElements())
