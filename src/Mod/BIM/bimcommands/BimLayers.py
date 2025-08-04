# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

"""Layers manager for FreeCAD"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


def getColorIcon(color):

    "returns a QtGui.QIcon from a color 3-float tuple"

    from PySide import QtGui

    c = QtGui.QColor(int(color[0] * 255), int(color[1] * 255), int(color[2] * 255))
    im = QtGui.QImage(48, 48, QtGui.QImage.Format_ARGB32)
    im.fill(c)
    px = QtGui.QPixmap.fromImage(im)
    return QtGui.QIcon(px)


class BIM_Layers:

    "The BIM_Layers FreeCAD command"

    def GetResources(self):

        return {
            "Pixmap": "BIM_Layers",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Layers", "Manage Layers"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Layers", "Sets/modifies the different layers of your BIM project"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        from PySide import QtGui

        # check if the dialog is running)
        if getattr(self, "dialog", None):
            return

        # store changes to be committed
        self.deleteList = []

        # store assignlist
        self.assignList = {}

        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(":/ui/dialogLayersIFC.ui")

        # store the ifc icon
        self.ifcicon = QtGui.QIcon(":/icons/IFC.svg")

        # set nice icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/Draft_Layer.svg"))
        self.dialog.buttonNew.setIcon(QtGui.QIcon(":/icons/document-new.svg"))
        self.dialog.buttonDelete.setIcon(QtGui.QIcon(":/icons/delete.svg"))
        self.dialog.buttonSelectAll.setIcon(QtGui.QIcon(":/icons/edit-select-all.svg"))
        self.dialog.buttonToggle.setIcon(QtGui.QIcon(":/icons/dagViewVisible.svg"))
        self.dialog.buttonIsolate.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
        self.dialog.buttonCancel.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
        self.dialog.buttonOK.setIcon(QtGui.QIcon(":/icons/edit_OK.svg"))
        self.dialog.buttonAssign.setIcon(QtGui.QIcon(":/icons/button_right.svg"))
        self.dialog.buttonIFC.setIcon(self.ifcicon)

        # restore window geometry from stored state
        w = PARAMS.GetInt("LayersManagerWidth", 640)
        h = PARAMS.GetInt("LayersManagerHeight", 320)
        self.dialog.resize(w, h)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.dialog.rect().center()
        )

        # connect signals/slots
        self.dialog.buttonNew.clicked.connect(self.addItem)
        self.dialog.buttonDelete.clicked.connect(self.onDelete)
        self.dialog.buttonSelectAll.clicked.connect(self.dialog.tree.selectAll)
        self.dialog.buttonToggle.clicked.connect(self.onToggle)
        self.dialog.buttonCancel.clicked.connect(self.dialog.reject)
        self.dialog.buttonIsolate.clicked.connect(self.onIsolate)
        self.dialog.buttonOK.clicked.connect(self.accept)
        self.dialog.buttonAssign.clicked.connect(self.onAssign)
        self.dialog.buttonIFC.clicked.connect(self.onIFC)
        self.dialog.rejected.connect(self.reject)

        # set the model up
        self.model = QtGui.QStandardItemModel()
        self.dialog.tree.setModel(self.model)
        self.dialog.tree.setUniformRowHeights(True)
        self.dialog.tree.setItemDelegate(BIM_Layers_Delegate())
        self.dialog.tree.setItemsExpandable(False)
        self.dialog.tree.setRootIsDecorated(False)  # removes spacing in first column
        self.dialog.tree.setSelectionMode(
            QtGui.QTreeView.ExtendedSelection
        )  # allow one to select many

        # fill the tree view
        self.update()

        # rock 'n roll!!!
        self.dialog.show()

    def accept(self):
        "when OK button is pressed"

        import Draft

        doc = FreeCAD.ActiveDocument
        changed = False

        # delete layers
        for name in self.deleteList:
            if not changed:
                doc.openTransaction("Layers change")
                changed = True
            doc.removeObject(name)

        # assign
        for target, objs in self.assignList.items():
            target_obj = doc.getObject(target)
            if target_obj:
                for obj in objs:
                    target_obj.Proxy.addObject(target_obj, obj)

        # apply changes
        for row in range(self.model.rowCount()):
            # get or create layer
            name = self.model.item(row, 1).toolTip()
            obj = None
            if name:
                obj = doc.getObject(name)
            if not obj:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                if self.model.item(row, 1).icon().isNull():
                    obj = Draft.make_layer(self.model.item(row, 1).text())
                    # By default BIM layers should not swallow their children otherwise
                    # they will disappear from the tree root
                    obj.ViewObject.addProperty("App::PropertyBool", "HideChildren", "Layer", locked=True)
                    obj.ViewObject.HideChildren = True
                else:
                    from nativeifc import ifc_tools
                    import FreeCADGui

                    active = FreeCADGui.ActiveDocument.ActiveView.getActiveObject(
                        "NativeIFC"
                    )
                    project = None
                    if active:
                        project = ifc_tools.get_project(active)
                    else:
                        projects = [
                            o
                            for o in doc.Objects
                            if hasattr(o, "Proxy") and hasattr(o.Proxy, "ifcfile")
                        ]
                        if projects:
                            project = projects[0]
                            if len(projects) > 1:
                                FreeCAD.Console.PrintWarning(
                                    translate(
                                        "BIM",
                                        "Warning: The new layer was added to the project",
                                    )
                                    + " "
                                    + project.Label
                                    + "\n"
                                )
                        else:
                            FreeCAD.Console.PrintError(
                                translate(
                                    "BIM", "There is no IFC project in this document"
                                )
                                + "\n"
                            )
                    if project:
                        obj = ifc_tools.create_layer(
                            self.model.item(row, 1).text(), project
                        )
            vobj = obj.ViewObject

            # visibility
            checked = self.model.item(row, 0).checkState() == QtCore.Qt.Checked
            if checked != vobj.Visibility:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.Visibility = checked

            # label
            label = self.model.item(row, 1).text()
            # Setting Label="" is possible in the Property editor but we avoid it here:
            if label and obj.Label != label:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                obj.Label = label

            # line width
            width = self.model.item(row, 2).data(QtCore.Qt.DisplayRole)
            # Setting LineWidth=0 is possible in the Property editor but we avoid it here:
            if width and vobj.LineWidth != width:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.LineWidth = width

            # draw style
            style = self.model.item(row, 3).text()
            if style is not None and vobj.DrawStyle != style:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.DrawStyle = style

            # line color
            color = self.model.item(row, 4).data(QtCore.Qt.UserRole)
            if color is not None and vobj.LineColor[3:] != color:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.LineColor = color

            # shape color
            color = self.model.item(row, 5).data(QtCore.Qt.UserRole)
            if color is not None and vobj.ShapeColor[3:] != color:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.ShapeColor = color

            # transparency
            transparency = self.model.item(row, 6).data(QtCore.Qt.DisplayRole)
            if transparency is not None and vobj.Transparency != transparency:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.Transparency = transparency

            # line print color
            color = self.model.item(row, 7).data(QtCore.Qt.UserRole)
            if color is not None and vobj.LinePrintColor[3:] != color:
                if not changed:
                    doc.openTransaction("Layers change")
                    changed = True
                vobj.LinePrintColor = color

        # recompute
        if changed:
            doc.commitTransaction()
            doc.recompute()

        # exit
        return self.dialog.reject()

    def reject(self):
        "when Cancel button is pressed or dialog is closed"

        # save dialog size
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        pref.SetInt("LayersManagerWidth", self.dialog.width())
        pref.SetInt("LayersManagerHeight", self.dialog.height())

        # wipe to let FreeCAD know the dialog has been closed
        del self.dialog

        return True

    def update(self):
        "rebuild the model from document contents"

        import Draft

        self.model.clear()

        # set header
        self.model.setHorizontalHeaderLabels(
            [
                translate("BIM", "On"),
                translate("BIM", "Name"),
                translate("BIM", "Line width"),
                translate("BIM", "Draw style"),
                translate("BIM", "Line color"),
                translate("BIM", "Face color"),
                translate("BIM", "Transparency"),
                translate("BIM", "Line print color"),
            ]
        )
        self.dialog.tree.header().setDefaultSectionSize(72)
        self.dialog.tree.setColumnWidth(0, 32)  # on/off column
        self.dialog.tree.setColumnWidth(1, 128)  # name column

        # populate
        objs = [
            obj
            for obj in FreeCAD.ActiveDocument.Objects
            if Draft.getType(obj) == "Layer"
        ]
        objs.sort(key=lambda o: o.Label)
        for obj in objs:
            self.addItem(obj)

    def addItem(self, obj=None):
        "adds a row to the model"

        from PySide import QtCore, QtGui

        # create row with default values
        onItem = QtGui.QStandardItem()
        onItem.setCheckable(True)
        onItem.setCheckState(QtCore.Qt.Checked)
        nameItem = QtGui.QStandardItem(translate("BIM", "New Layer"))
        widthItem = QtGui.QStandardItem()
        widthItem.setData(
            self.getPref("DefaultShapeLineWidth", 2, "Integer"), QtCore.Qt.DisplayRole
        )
        styleItem = QtGui.QStandardItem("Solid")
        lineColorItem = QtGui.QStandardItem()
        lineColorItem.setData(
            self.getPref("DefaultShapeLineColor", 421075455), QtCore.Qt.UserRole
        )
        shapeColorItem = QtGui.QStandardItem()
        shapeColorItem.setData(
            self.getPref("DefaultShapeColor", 3435973887), QtCore.Qt.UserRole
        )
        transparencyItem = QtGui.QStandardItem()
        transparencyItem.setData(0, QtCore.Qt.DisplayRole)
        linePrintColorItem = QtGui.QStandardItem()
        linePrintColorItem.setData(
            self.getPref("DefaultPrintColor", 0), QtCore.Qt.UserRole
        )
        if FreeCADGui.ActiveDocument.ActiveView.getActiveObject("NativeIFC"):
            nameItem.setIcon(self.ifcicon)

        # populate with object data
        if obj:
            if hasattr(obj, "StepId"):
                nameItem.setIcon(self.ifcicon)
            onItem.setCheckState(
                QtCore.Qt.Checked if obj.ViewObject.Visibility else QtCore.Qt.Unchecked
            )
            nameItem.setText(obj.Label)
            nameItem.setToolTip(obj.Name)
            widthItem.setData(obj.ViewObject.LineWidth, QtCore.Qt.DisplayRole)
            styleItem.setText(obj.ViewObject.DrawStyle)
            lineColorItem.setData(obj.ViewObject.LineColor[:3], QtCore.Qt.UserRole)
            shapeColorItem.setData(obj.ViewObject.ShapeColor[:3], QtCore.Qt.UserRole)
            transparencyItem.setData(obj.ViewObject.Transparency, QtCore.Qt.DisplayRole)
            if hasattr(obj.ViewObject, "LinePrintColor"):
                linePrintColorItem.setData(
                    obj.ViewObject.LinePrintColor[:3], QtCore.Qt.UserRole
                )
        lineColorItem.setIcon(getColorIcon(lineColorItem.data(QtCore.Qt.UserRole)))
        shapeColorItem.setIcon(getColorIcon(shapeColorItem.data(QtCore.Qt.UserRole)))
        linePrintColorItem.setIcon(
            getColorIcon(linePrintColorItem.data(QtCore.Qt.UserRole))
        )

        # append row
        self.model.appendRow(
            [
                onItem,
                nameItem,
                widthItem,
                styleItem,
                lineColorItem,
                shapeColorItem,
                transparencyItem,
                linePrintColorItem,
            ]
        )

    def getPref(self, value, default, valuetype="Unsigned"):
        "retrieves a view pref value"

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        if valuetype == "Unsigned":
            c = p.GetUnsigned(value, default)
            r = float((c >> 24) & 0xFF) / 255.0
            g = float((c >> 16) & 0xFF) / 255.0
            b = float((c >> 8) & 0xFF) / 255.0
            return (
                r,
                g,
                b,
            )
        elif valuetype == "Integer":
            return p.GetInt(value, default)

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

        from PySide import QtCore

        state = None
        for index in self.dialog.tree.selectedIndexes():
            if index.column() == 0:
                # get state from first selected row
                if state is None:
                    if (
                        self.model.itemFromIndex(index).checkState()
                        == QtCore.Qt.Checked
                    ):
                        state = QtCore.Qt.Unchecked
                    else:
                        state = QtCore.Qt.Checked
                self.model.itemFromIndex(index).setCheckState(state)

    def onIsolate(self):
        "isolates the selected layers (turns all the others off"

        from PySide import QtCore

        onrows = []
        for index in self.dialog.tree.selectedIndexes():
            if not index.row() in onrows:
                onrows.append(index.row())
        for row in range(self.model.rowCount()):
            if not row in onrows:
                self.model.item(row, 0).setCheckState(QtCore.Qt.Unchecked)

    def onIFC(self):
        "attributes this layer to an IFC project"

        from PySide import QtGui

        for index in self.dialog.tree.selectedIndexes():
            if index.column() == 1:
                name = self.model.itemFromIndex(index).toolTip()
                if name:
                    obj = FreeCAD.ActiveDocument.getObject(name)
                    if obj:
                        if hasattr(obj, "StepId"):
                            return
                item = self.model.itemFromIndex(index)
                if item.icon().isNull():
                    item.setIcon(self.ifcicon)
                else:
                    item.setIcon(QtGui.QIcon())

    def onAssign(self):
        "attributes selected objects to a selected layer"

        for index in self.dialog.tree.selectedIndexes():
            if index.column() == 1:
                name = self.model.itemFromIndex(index).toolTip()
                if name:
                    target = FreeCAD.ActiveDocument.getObject(name)
                    if target:
                        selected = FreeCADGui.Selection.getSelection()
                        if selected:
                            self.assignList.setdefault(target.Name, [])
                            for i in selected:
                                if i not in self.assignList[target.Name]:
                                    self.assignList[target.Name].append(i)


if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui

    class BIM_Layers_Delegate(QtGui.QStyledItemDelegate):
        "model delegate"

        def __init__(self, parent=None, *args):
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
            # setEditorData() is triggered several times.
            # But we want to show the color dialog only the first time
            self.first = True

        def createEditor(self, parent, option, index):
            if index.column() == 0:  # Layer on/off
                editor = QtGui.QCheckBox(parent)
            if index.column() == 1:  # Layer name
                editor = QtGui.QLineEdit(parent)
            elif index.column() == 2:  # Line width
                editor = QtGui.QSpinBox(parent)
                editor.setMaximum(99)
            elif index.column() == 3:  # Line style
                editor = QtGui.QComboBox(parent)
                editor.addItems(["Solid", "Dashed", "Dotted", "Dashdot"])
            elif index.column() == 4:  # Line color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            elif index.column() == 5:  # Shape color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            elif index.column() == 6:  # Transparency
                editor = QtGui.QSpinBox(parent)
                editor.setMaximum(100)
            elif index.column() == 7:  # Line print color
                editor = QtGui.QLineEdit(parent)
                self.first = True
            return editor

        def setEditorData(self, editor, index):
            if index.column() == 0:  # Layer on/off
                editor.setChecked(index.data())
            elif index.column() == 1:  # Layer name
                editor.setText(index.data())
            elif index.column() == 2:  # Line width
                editor.setValue(index.data())
            elif index.column() == 3:  # Line style
                editor.setCurrentIndex(
                    ["Solid", "Dashed", "Dotted", "Dashdot"].index(index.data())
                )
            elif index.column() == 4:  # Line color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(
                        QtGui.QColor(int(c[0] * 255), int(c[1] * 255), int(c[2] * 255))
                    )
                    editor.setText(str(color.getRgbF()))
                    self.first = False
            elif index.column() == 5:  # Shape color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(
                        QtGui.QColor(int(c[0] * 255), int(c[1] * 255), int(c[2] * 255))
                    )
                    editor.setText(str(color.getRgbF()))
                    self.first = False
            elif index.column() == 6:  # Transparency
                editor.setValue(index.data())
            elif index.column() == 7:  # Line print color
                editor.setText(str(index.data(QtCore.Qt.UserRole)))
                if self.first:
                    c = index.data(QtCore.Qt.UserRole)
                    color = QtGui.QColorDialog.getColor(
                        QtGui.QColor(int(c[0] * 255), int(c[1] * 255), int(c[2] * 255))
                    )
                    editor.setText(str(color.getRgbF()))
                    self.first = False

        def setModelData(self, editor, model, index):
            if index.column() == 0:  # Layer on/off
                model.setData(index, editor.isChecked())
            elif index.column() == 1:  # Layer name
                model.setData(index, editor.text())
            elif index.column() == 2:  # Line width
                model.setData(index, editor.value())
            elif index.column() == 3:  # Line style
                model.setData(
                    index,
                    ["Solid", "Dashed", "Dotted", "Dashdot"][editor.currentIndex()],
                )
            elif index.column() == 4:  # Line color
                model.setData(index, eval(editor.text()), QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))
            elif index.column() == 5:  # Shape color
                model.setData(index, eval(editor.text()), QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))
            elif index.column() == 6:  # Transparency
                model.setData(index, editor.value())
            elif index.column() == 7:  # Line prin color
                model.setData(index, eval(editor.text()), QtCore.Qt.UserRole)
                model.itemFromIndex(index).setIcon(getColorIcon(eval(editor.text())))


FreeCADGui.addCommand("BIM_Layers", BIM_Layers())
