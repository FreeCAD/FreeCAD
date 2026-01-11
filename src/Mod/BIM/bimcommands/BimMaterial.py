# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui

    class MatLineEdit(QtGui.QLineEdit):
        "custom QLineEdit widget that has the power to catch up/down arrow keypress"

        up = QtCore.Signal()
        down = QtCore.Signal()

        def __init__(self, parent=None):
            QtGui.QLineEdit.__init__(self, parent)

        def keyPressEvent(self, event):
            if event.key() == QtCore.Qt.Key_Up:
                self.up.emit()
            elif event.key() == QtCore.Qt.Key_Down:
                self.down.emit()
            else:
                QtGui.QLineEdit.keyPressEvent(self, event)


class BIM_Material:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Material",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Material", "Material"),
            "Accel": "M, A",
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Material", "Sets or creates a material for selected objects"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        # only raise the dialog if it is already open
        if getattr(self, "dlg", None):
            self.dlg.raise_()
            return

        self.dlg = QtGui.QDialog()
        self.dlg.objects = [
            obj
            for obj in FreeCADGui.Selection.getSelection()
            if hasattr(obj, "Material") or hasattr(obj, "StepId")
        ]
        w = PARAMS.GetInt("BimMaterialDialogWidth", 230)
        h = PARAMS.GetInt("BimMaterialDialogHeight", 350)
        self.dlg.resize(w, h)
        self.dlg.setWindowTitle(translate("BIM", "Select Material"))
        self.dlg.setWindowIcon(QtGui.QIcon(":/icons/Arch_Material.svg"))
        mw = FreeCADGui.getMainWindow()
        self.dlg.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.dlg.rect().center())
        lay = QtGui.QVBoxLayout(self.dlg)
        matList = QtGui.QListWidget(self.dlg)
        matList.setSortingEnabled(True)
        matList.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.dlg.matList = matList
        lay.addWidget(matList)

        # populate materials list
        self.rescan(rebuild=True)

        if matList.count():
            # add search box
            searchLayout = QtGui.QHBoxLayout()
            searchLayout.setSpacing(2)
            searchBox = MatLineEdit(self.dlg)
            searchBox.setPlaceholderText(translate("BIM", "Search materials"))
            searchBox.setToolTip(translate("BIM", "Searches for materials in the list"))
            self.dlg.searchBox = searchBox
            searchLayout.addWidget(searchBox)
            searchBox.textChanged.connect(self.onSearch)
            # searchBox.up.connect(self.onUpArrow)
            QtCore.QObject.connect(searchBox, QtCore.SIGNAL("up()"), self.onUpArrow)
            # searchBox.down.connect(self.onDownArrow)
            QtCore.QObject.connect(searchBox, QtCore.SIGNAL("down()"), self.onDownArrow)

            # add clear button
            buttonClear = QtGui.QToolButton(self.dlg)
            buttonClear.setFixedSize(18, 21)
            buttonClear.setStyleSheet("QToolButton {margin-bottom:1px}")
            buttonClear.setIcon(QtGui.QIcon(":/icons/edit-cleartext.svg"))
            buttonClear.setToolTip(translate("BIM", "Clears the search field"))
            buttonClear.setAutoRaise(True)
            searchLayout.addWidget(buttonClear)
            buttonClear.clicked.connect(self.onClearSearch)
            lay.addLayout(searchLayout)

            createButtonsLayoutBox = QtGui.QGroupBox(
                translate("BIM", " Material Operations"), self.dlg
            )
            createButtonsLayoutBox.setObjectName("matOpsGrpBox")
            createButtonsLayout = QtGui.QGridLayout()

            # create
            buttonCreate = QtGui.QPushButton(translate("BIM", "New Material"), self.dlg)
            buttonCreate.setIcon(QtGui.QIcon(":/icons/Arch_Material.svg"))
            createButtonsLayout.addWidget(buttonCreate, 0, 0)
            buttonCreate.clicked.connect(self.onCreate)

            # create multi
            buttonMulti = QtGui.QPushButton(translate("BIM", "New Multi-Material"), self.dlg)
            buttonMulti.setIcon(QtGui.QIcon(":/icons/Arch_Material_Multi.svg"))
            createButtonsLayout.addWidget(buttonMulti, 0, 1)
            buttonMulti.clicked.connect(self.onMulti)

            # merge dupes
            opsLayout = QtGui.QHBoxLayout()
            buttonMergeDupes = QtGui.QPushButton(translate("BIM", "Merge Duplicates"), self.dlg)
            buttonMergeDupes.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
            createButtonsLayout.addWidget(buttonMergeDupes, 1, 0)
            self.dlg.buttonMergeDupes = buttonMergeDupes
            buttonMergeDupes.clicked.connect(self.onMergeDupes)
            if len(self.dlg.materials) < 2:
                buttonMergeDupes.setEnabled(False)

            # delete unused
            buttonDeleteUnused = QtGui.QPushButton(translate("BIM", "Delete Unused"), self.dlg)
            buttonDeleteUnused.setIcon(QtGui.QIcon(":/icons/delete.svg"))
            createButtonsLayout.addWidget(buttonDeleteUnused, 1, 1)
            buttonDeleteUnused.clicked.connect(self.onDeleteUnused)

            createButtonsLayoutBox.setLayout(createButtonsLayout)
            lay.addWidget(createButtonsLayoutBox)

            # add standard buttons
            buttonBox = QtGui.QDialogButtonBox(self.dlg)
            buttonBox.setOrientation(QtCore.Qt.Horizontal)
            buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel | QtGui.QDialogButtonBox.Ok)
            lay.addWidget(buttonBox)
            buttonBox.accepted.connect(self.onAccept)
            buttonBox.rejected.connect(self.onReject)

            # set context menu
            self.contextMenu = QtGui.QMenu()
            context1 = self.contextMenu.addAction(translate("BIM", "Rename"))
            context1.triggered.connect(self.onStartRename)
            context2 = self.contextMenu.addAction(translate("BIM", "Duplicate"))
            context2.triggered.connect(self.onDuplicate)
            context3 = self.contextMenu.addAction(translate("BIM", "Merge To…"))
            context3.triggered.connect(self.onMergeTo)
            context4 = self.contextMenu.addAction(translate("BIM", "Delete"))
            context4.triggered.connect(self.onDelete)

            # other signal/slots to connect
            self.dlg.rejected.connect(self.onReject)
            matList.customContextMenuRequested.connect(self.onRightClick)
            matList.itemDoubleClicked.connect(self.onAccept)
            matList.itemChanged.connect(self.onEndRename)

            self.dlg.show()
            self.dlg.searchBox.setFocus()

        else:
            # no material in the document
            self.onReject()
            FreeCADGui.runCommand("Arch_Material")

    def onRightClick(self, pos):
        parentPosition = self.dlg.matList.mapToGlobal(QtCore.QPoint(0, 0))
        self.contextMenu.move(parentPosition + pos)
        self.contextMenu.show()

    def onMergeDupes(self):
        if self.dlg:
            todelete = []
            first = True
            for mat in self.dlg.materials:
                orig = None
                if mat.Label[-1].isdigit() and mat.Label[-2].isdigit() and mat.Label[-3].isdigit():
                    for om in self.dlg.materials:
                        if om.Label == mat.Label[:-3].strip():
                            orig = om
                            break
                if orig:
                    for par in mat.InList:
                        for prop in par.PropertiesList:
                            if getattr(par, prop) == mat:
                                FreeCAD.Console.PrintMessage(
                                    "Changed property '"
                                    + prop
                                    + "' of object "
                                    + par.Label
                                    + " from "
                                    + mat.Label
                                    + " to "
                                    + orig.Label
                                    + "\n"
                                )
                                if first:
                                    FreeCAD.ActiveDocument.openTransaction("Merge materials")
                                    first = False
                                setattr(par, prop, orig)
                    todelete.append(mat)
            for tod in todelete:
                if not tod.InList:
                    FreeCAD.Console.PrintMessage(
                        translate("BIM", "Merging duplicate material") + " " + tod.Label + "\n"
                    )
                    if first:
                        FreeCAD.ActiveDocument.openTransaction("Merge materials")
                        first = False
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                elif (len(tod.InList) == 1) and (
                    tod.InList[0].isDerivedFrom("App::DocumentObjectGroup")
                ):
                    FreeCAD.Console.PrintMessage(
                        translate("BIM", "Merging duplicate material") + " " + tod.Label + "\n"
                    )
                    if first:
                        FreeCAD.ActiveDocument.openTransaction("Merge materials")
                        first = False
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                else:
                    FreeCAD.Console.PrintMessage(
                        translate("BIM", "Unable to delete material")
                        + " "
                        + tod.Label
                        + ": "
                        + translate("BIM", "InList not empty")
                        + "\n"
                    )
            if not first:
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                self.rescan(rebuild=True)

    def onDeleteUnused(self):
        first = True
        if self.dlg:
            for i in range(self.dlg.matList.count()):
                item = self.dlg.matList.item(i)
                if item:
                    obj = FreeCAD.ActiveDocument.getObject(item.toolTip())
                    if obj:
                        parents = [
                            parent
                            for parent in obj.InList
                            if not parent.isDerivedFrom("App::DocumentObjectGroup")
                        ]
                        if not parents:
                            name = obj.Name
                            label = obj.Label
                            if first:
                                FreeCAD.ActiveDocument.openTransaction("Delete materials")
                                first = False
                            FreeCAD.Console.PrintMessage(
                                translate("BIM", "Deleting unused material") + " " + label + "\n"
                            )
                            FreeCAD.ActiveDocument.removeObject(name)
        if not first:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            self.rescan(rebuild=True)

    def onDuplicate(self):
        if self.dlg:
            item = self.dlg.matList.currentItem()
            if item:
                oldmat = FreeCAD.ActiveDocument.getObject(item.toolTip())
                if oldmat:
                    import Arch

                    newmat = Arch.makeMaterial()
                    newmat.Label = item.text()
                    newmat.Material = oldmat.Material
                    FreeCAD.ActiveDocument.recompute()
                    i = QtGui.QListWidgetItem(
                        self.createIcon(newmat), newmat.Label, self.dlg.matList
                    )
                    i.setToolTip(newmat.Name)
                    i.setFlags(i.flags() | QtCore.Qt.ItemIsEditable)
                    self.dlg.matList.setCurrentItem(i)
                    self.rescan()

    def onStartRename(self):
        if self.dlg:
            item = self.dlg.matList.currentItem()
            if item:
                self.dlg.matList.editItem(item)

    def onEndRename(self, item):
        obj = FreeCAD.ActiveDocument.getObject(item.toolTip())
        if obj:
            if obj.Label != item.text():
                obj.Label = item.text()

    def onMergeTo(self):
        if self.dlg:
            item = self.dlg.matList.currentItem()
            if item:
                oldmat = FreeCAD.ActiveDocument.getObject(item.toolTip())
                # load dialog
                form = FreeCADGui.PySideUic.loadUi(":/ui/dialogListWidget.ui")
                # center the dialog over FreeCAD window
                mw = FreeCADGui.getMainWindow()
                form.move(mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center())
                form.setWindowTitle(translate("BIM", "Select material to merge to"))
                form.setWindowIcon(QtGui.QIcon(":/icons/Arch_Material.svg"))
                for i in range(self.dlg.matList.count()):
                    oit = self.dlg.matList.item(i)
                    if oit != item:
                        nit = QtGui.QListWidgetItem(oit.icon(), oit.text(), form.listWidget)
                        nit.setToolTip(oit.toolTip())
                result = form.exec_()
                if result:
                    mergeto = form.listWidget.currentItem()
                    if mergeto:
                        mergemat = FreeCAD.ActiveDocument.getObject(mergeto.toolTip())
                        if oldmat and mergemat:
                            parents = [
                                parent
                                for parent in oldmat.InList
                                if (hasattr(parent, "Material") and (parent.Material == oldmat))
                            ]
                            name = oldmat.Name
                            FreeCAD.ActiveDocument.openTransaction("Merge material")
                            for parent in parents:
                                parent.Material = mergemat
                            FreeCAD.ActiveDocument.removeObject(name)
                            FreeCAD.ActiveDocument.commitTransaction()
                            FreeCAD.ActiveDocument.recompute()
                            self.rescan()
                else:
                    return

    def onDelete(self):
        if self.dlg:
            item = self.dlg.matList.currentItem()
            if item:
                obj = FreeCAD.ActiveDocument.getObject(item.toolTip())
                if obj:
                    parents = [
                        parent
                        for parent in obj.InList
                        if not parent.isDerivedFrom("App::DocumentObjectGroup")
                    ]
                    if parents:
                        QtGui.QMessageBox.information(
                            None,
                            "",
                            translate("BIM", "This material is used by:")
                            + " "
                            + ",".join([p.Label for p in parents]),
                        )
                    else:
                        self.dlg.matList.takeItem(self.dlg.matList.currentRow())
                        name = obj.Name
                        FreeCAD.ActiveDocument.openTransaction("Delete material")
                        FreeCAD.ActiveDocument.removeObject(name)
                        FreeCAD.ActiveDocument.commitTransaction()
                        FreeCAD.ActiveDocument.recompute()
                        self.rescan()

    def onCreate(self):
        self.onReject()
        FreeCADGui.runCommand("Arch_Material")

    def onMulti(self):
        self.onReject()
        FreeCADGui.runCommand("Arch_MultiMaterial")

    def onAccept(self, item=None):
        if self.dlg:
            item = self.dlg.matList.currentItem()
            if item and self.dlg.objects:
                mat = FreeCAD.ActiveDocument.getObject(item.toolTip())
                if mat:
                    if self.dlg.objects:
                        FreeCAD.ActiveDocument.openTransaction("Change material")
                        for obj in self.dlg.objects:
                            if hasattr(obj, "StepId"):
                                from nativeifc import ifc_materials

                                ifc_materials.set_material(mat, obj)
                            else:
                                obj.Material = mat
                        FreeCAD.ActiveDocument.commitTransaction()
                        FreeCAD.ActiveDocument.recompute()
            p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
            p.SetInt("BimMaterialDialogWidth", self.dlg.width())
            p.SetInt("BimMaterialDialogHeight", self.dlg.height())
        from DraftGui import todo

        # delay required for matList.itemDoubleClicked action
        todo.delay(self.onReject, None)

    def onReject(self):
        if self.dlg:
            self.dlg.hide()
            self.dlg = None

    def onUpArrow(self):
        if self.dlg:
            i = self.dlg.matList.currentRow()
            if i > 0:
                self.dlg.matList.setCurrentRow(i - 1)
            else:
                self.dlg.matList.setCurrentRow(self.dlg.matList.count() - 1)

    def onDownArrow(self):
        if self.dlg:
            i = self.dlg.matList.currentRow()
            if i < self.dlg.matList.count() - 1:
                self.dlg.matList.setCurrentRow(i + 1)
            else:
                self.dlg.matList.setCurrentRow(0)

    def onClearSearch(self):
        if self.dlg:
            self.dlg.searchBox.setText("")

    def onSearch(self, text):
        from PySide import QtCore, QtGui

        self.dlg.matList.clear()
        for o in self.dlg.materials:
            if text.lower() in o.Label.lower():
                i = QtGui.QListWidgetItem(self.createIcon(o), o.Label, self.dlg.matList)
                i.setToolTip(o.Name)
                i.setFlags(i.flags() | QtCore.Qt.ItemIsEditable)
                self.dlg.matList.setCurrentItem(i)

    def rescan(self, rebuild=False):
        from PySide import QtCore, QtGui

        if self.dlg:
            self.dlg.materials = []
            for o in FreeCAD.ActiveDocument.Objects:
                if o.isDerivedFrom("App::MaterialObjectPython") or (
                    (o.TypeId == "App::FeaturePython") and hasattr(o, "Materials")
                ):
                    self.dlg.materials.append(o)
            if rebuild:
                c = self.dlg.matList.currentItem()
                name = None
                if c:
                    name = c.toolTip()
                elif len(self.dlg.objects) == 1:
                    if getattr(self.dlg.objects[0], "Material", None):
                        if hasattr(self.dlg.objects[0].Material, "Name"):
                            name = self.dlg.objects[0].Material.Name
                        else:
                            name = "None"
                self.dlg.matList.clear()
                for o in self.dlg.materials:
                    i = QtGui.QListWidgetItem(self.createIcon(o), o.Label, self.dlg.matList)
                    i.setToolTip(o.Name)
                    i.setFlags(i.flags() | QtCore.Qt.ItemIsEditable)
                    if o.Name == name:
                        self.dlg.matList.setCurrentItem(i)
            if hasattr(self.dlg, "buttonMergeDupes"):
                hasMultipleMaterials = len(self.dlg.materials) > 1
                self.dlg.buttonMergeDupes.setEnabled(hasMultipleMaterials)

    def createIcon(self, obj):
        from PySide import QtCore, QtGui

        if hasattr(obj, "Materials"):
            return QtGui.QIcon(":/icons/Arch_Material_Multi.svg")
        elif obj.ViewObject.Icon:
            return obj.ViewObject.Icon
        elif hasattr(obj, "Color"):
            c = obj.Color
            matcolor = QtGui.QColor(int(c[0] * 255), int(c[1] * 255), int(c[2] * 255))
            darkcolor = QtGui.QColor(int(c[0] * 125), int(c[1] * 125), int(c[2] * 125))
            im = QtGui.QImage(48, 48, QtGui.QImage.Format_ARGB32)
            im.fill(QtCore.Qt.transparent)
            pt = QtGui.QPainter(im)
            pt.setPen(QtGui.QPen(QtCore.Qt.black, 2, QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
            # pt.setBrush(QtGui.QBrush(matcolor, QtCore.Qt.SolidPattern))
            gradient = QtGui.QLinearGradient(0, 0, 48, 48)
            gradient.setColorAt(0, matcolor)
            gradient.setColorAt(1, darkcolor)
            pt.setBrush(QtGui.QBrush(gradient))
            pt.drawEllipse(6, 6, 36, 36)
            pt.setPen(QtGui.QPen(QtCore.Qt.white, 1, QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
            pt.setBrush(QtGui.QBrush(QtCore.Qt.white, QtCore.Qt.SolidPattern))
            pt.drawEllipse(12, 12, 12, 12)
            pt.end()
            px = QtGui.QPixmap.fromImage(im)
            return QtGui.QIcon(px)
        else:
            return QtGui.QIcon(":/icons/Arch_Material.svg")


class Arch_Material:
    "the Arch Material command definition"

    def GetResources(self):

        return {
            "Pixmap": "Arch_Material_Group",
            "MenuText": QT_TRANSLATE_NOOP("Arch_Material", "Material"),
            "Accel": "M, T",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_Material", "Creates or edits the material definition of a selected object."
            ),
        }

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create material"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.doCommand("mat = Arch.makeMaterial()")
        for obj in sel:
            if hasattr(obj, "Material") and hasattr(obj, "MoveWithHost"):  # 'isComponent' check
                FreeCADGui.doCommand(
                    'FreeCAD.ActiveDocument.getObject("' + obj.Name + '").Material = mat'
                )
        FreeCADGui.doCommandGui("mat.ViewObject.Document.setEdit(mat.ViewObject, 0)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v


class Arch_MultiMaterial:
    "the Arch MultiMaterial command definition"

    def GetResources(self):

        return {
            "Pixmap": "Arch_Material_Multi",
            "MenuText": QT_TRANSLATE_NOOP("Arch_MultiMaterial", "Multi-Material"),
            "Accel": "M, T",
            "ToolTip": QT_TRANSLATE_NOOP("Arch_MultiMaterial", "Creates or edits multi-materials"),
        }

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create multi-material"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.doCommand("mat = Arch.makeMultiMaterial()")
        for obj in sel:
            if hasattr(obj, "Material"):
                if not obj.isDerivedFrom("App::MaterialObject"):
                    FreeCADGui.doCommand("FreeCAD.ActiveDocument." + obj.Name + ".Material = mat")
        FreeCADGui.doCommandGui("mat.ViewObject.Document.setEdit(mat.ViewObject, 0)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v


class Arch_MaterialToolsCommand:

    def GetCommands(self):
        return tuple(["Arch_Material", "Arch_MultiMaterial"])

    def GetResources(self):
        return {
            "MenuText": QT_TRANSLATE_NOOP("Arch_MaterialTools", "Material Tools"),
            "ToolTip": QT_TRANSLATE_NOOP("Arch_MaterialTools", "Material tools"),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v


FreeCADGui.addCommand("BIM_Material", BIM_Material())
FreeCADGui.addCommand("Arch_Material", Arch_Material())
FreeCADGui.addCommand("Arch_MultiMaterial", Arch_MultiMaterial())
FreeCADGui.addCommand("Arch_MaterialTools", Arch_MaterialToolsCommand())
