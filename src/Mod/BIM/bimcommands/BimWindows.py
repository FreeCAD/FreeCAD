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

"""The BIM Windows Manager command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Windows:
    def GetResources(self):
        return {
            "Pixmap": "BIM_Windows",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Windows", "Manage Doors and Windows"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Windows",
                "Manages the different doors and windows of the BIM project",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        FreeCADGui.Control.showDialog(BIM_Windows_TaskPanel())


class BIM_Windows_TaskPanel:

    def __init__(self):
        from PySide import QtGui

        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogWindows.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_Windows.svg"))
        self.form.groupMode.currentIndexChanged.connect(self.update)
        self.form.windows.itemClicked.connect(self.editWindow)
        self.form.windows.itemDoubleClicked.connect(self.showWindow)
        self.form.windowLabel.returnPressed.connect(self.setLabel)
        self.form.windowDescription.returnPressed.connect(self.setDescription)
        self.form.windowTag.returnPressed.connect(self.setTag)
        self.form.windowHeight.returnPressed.connect(self.setHeight)
        self.form.windowWidth.returnPressed.connect(self.setWidth)
        self.form.windowMaterial.clicked.connect(self.setMaterial)
        self.form.windows.header().setResizeMode(0, QtGui.QHeaderView.Stretch)
        self.update()

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Close

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def update(self, index=None):
        import Draft
        import Arch_rc
        from PySide import QtGui

        self.form.windows.clear()
        windows = [
            o for o in FreeCAD.ActiveDocument.Objects if Draft.getType(o) == "Window"
        ]
        if self.form.groupMode.currentIndex() == 0:
            for window in windows:
                s1 = window.Label
                s2 = window.Tag
                it = QtGui.QTreeWidgetItem([s1, s2])
                it.setIcon(0, QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
                it.setToolTip(0, window.Name)
                self.form.windows.addTopLevelItem(it)
        else:
            groups = {}
            for window in windows:
                group = None
                if self.form.groupMode.currentIndex() == 1:
                    group = window.Width.UserString + " x " + window.Height.UserString
                elif self.form.groupMode.currentIndex() == 2:
                    if window.CloneOf:
                        group = window.CloneOf.Label
                    else:
                        group = window.Name
                elif self.form.groupMode.currentIndex() == 3:
                    group = window.Tag
                else:
                    if window.Material:
                        group = window.Material.Label
                if not group:
                    group = "None"
                if group in groups:
                    groups[group].append(window)
                else:
                    groups[group] = [window]
            for group in groups.keys():
                s1 = group
                top = QtGui.QTreeWidgetItem([s1, ""])
                top.setExpanded(True)
                self.form.windows.addTopLevelItem(top)
                for window in groups[group]:
                    s1 = window.Label
                    s2 = window.Tag
                    it = QtGui.QTreeWidgetItem([s1, s2])
                    it.setIcon(0, QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
                    it.setToolTip(0, window.Name)
                    top.addChild(it)
            self.form.windows.expandAll()
        wc = 0
        dc = 0
        for w in windows:
            if hasattr(w, "IfcType"):
                r = w.IfcType
            elif hasattr(w, "IfcRole"):
                r = w.IfcRole
            else:
                r = w.Role
            if "Window" in r:
                wc += 1
            elif "Door" in r:
                dc += 1
        self.form.windowsCount.setText(str(wc))
        self.form.doorsCount.setText(str(dc))

    def editWindow(self, item, column):

        if len(self.form.windows.selectedItems()) == 1:
            # don't change the contents if we have more than one floor selected
            window = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
            if window:
                self.form.windowLabel.setText(window.Label)
                self.form.windowDescription.setText(window.Description)
                self.form.windowTag.setText(window.Tag)
                self.form.windowWidth.setText(window.Width.UserString)
                self.form.windowHeight.setText(window.Height.UserString)
                if window.Material:
                    self.form.windowMaterial.setText(window.Material.Label)
        # select objects
        FreeCADGui.Selection.clearSelection()
        for item in self.form.windows.selectedItems():
            o = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
            if o:
                FreeCADGui.Selection.addSelection(o)

    def showWindow(self, item, column):

        window = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
        if window:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(window)
            FreeCADGui.SendMsgToActiveView("ViewSelection")

    def setWidth(self):
        val = FreeCAD.Units.Quantity(self.form.windowWidth.text()).Value
        if val:
            for it in self.form.windows.selectedItems():
                window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
                if window:
                    window.Width = val
            self.update()

    def setHeight(self):
        val = FreeCAD.Units.Quantity(self.form.windowHeight.text()).Value
        if val:
            for it in self.form.windows.selectedItems():
                window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
                if window:
                    window.Height = val
            self.update()

    def setLabel(self):
        val = self.form.windowLabel.text()
        if val:
            for it in self.form.windows.selectedItems():
                window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
                if window:
                    window.Label = val
            self.update()

    def setTag(self):
        for it in self.form.windows.selectedItems():
            window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
            if window:
                window.Tag = self.form.windowTag.text()
        self.update()

    def setDescription(self):
        for it in self.form.windows.selectedItems():
            window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
            if window:
                window.Description = self.form.windowDescription.text()
        self.update()

    def setMaterial(self):
        import Draft
        import Arch_rc
        from PySide import QtGui

        form = FreeCADGui.PySideUic.loadUi(":/ui/dialogMaterialChooser.ui")
        mw = FreeCADGui.getMainWindow()
        form.move(
            mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center()
        )
        materials = [
            o for o in FreeCAD.ActiveDocument.Objects if Draft.getType(o) == "Material"
        ]
        it = QtGui.QListWidgetItem(translate("BIM", "None"))
        it.setIcon(QtGui.QIcon(":/icons/button_invalid.svg"))
        it.setToolTip("__None__")
        form.list.addItem(it)
        for mat in materials:
            it = QtGui.QListWidgetItem(mat.Label)
            it.setIcon(QtGui.QIcon(":/icons/Arch_Material.svg"))
            it.setToolTip(mat.Name)
            form.list.addItem(it)
        result = form.exec_()
        if result:
            mat = None
            sel = form.list.selectedItems()
            if sel:
                sel = sel[0]
                if sel.toolTip() != "__None__":
                    mat = FreeCAD.ActiveDocument.getObject(str(sel.toolTip()))
                for it in self.form.windows.selectedItems():
                    window = FreeCAD.ActiveDocument.getObject(it.toolTip(0))
                    if window:
                        if mat:
                            window.Material = mat
                        else:
                            window.Material = None
                if mat:
                    self.form.windowMaterial.setText(mat.Label)
                self.update()


FreeCADGui.addCommand("BIM_Windows", BIM_Windows())
