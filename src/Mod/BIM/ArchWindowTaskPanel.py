# ***************************************************************************
# *   Copyright (c) 2025 Yorik van Havre <yorik@uncreated.net>              *
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

"""The task panel system of Arch windows"""

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui
import ArchWindow

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class window_task_panel:

    def __init__(self, obj):
        """Creates the panels. The WindowParts list is kept in self.WindowParts
        and the object itself is updated only if the task is closed with OK."""

        self.setting_properties = False  # to not trigger edit_properties when setting
        self.obj = obj
        self.WindowParts = self.obj.WindowParts
        self.update_parents()

        self.panel1 = FreeCADGui.PySideUic.loadUi(":/ui/taskWindowComponents.ui")
        self.update_components_list()
        self.panel1.tree.setDropIndicatorShown(True)
        self.panel1.tree.currentItemChanged.connect(self.update_properties)
        self.panel1.buttonAdd.clicked.connect(self.add_component)
        self.panel1.buttonDelete.clicked.connect(self.remove_component)
        # TODO below does not work - self.dropEvent is never triggered
        self.panel1.tree.dropEvent = self.dropEvent

        self.panel2 = FreeCADGui.PySideUic.loadUi(":/ui/taskWindowProperties.ui")
        self.model = QtGui.QStandardItemModel(9, 2)
        self.panel2.table.setModel(self.model)
        self.panel2.table.setItemDelegate(window_component_delegate(self, self.panel2.table))
        self.model.itemChanged.connect(self.edit_properties)
        self.panel2.table.horizontalHeader().hide()
        self.panel2.table.verticalHeader().hide()
        self.panel2.table.horizontalHeader().setStretchLastSection(True)

        self.panel3 = FreeCADGui.PySideUic.loadUi(":/ui/taskWindowTools.ui")
        self.panel3.buttonInvertOpening.clicked.connect(self.invert_opening)
        self.panel3.buttonInvertHinge.clicked.connect(self.invert_hinge)

        self.set_titles()
        self.form = [self.panel3, self.panel1, self.panel2]
        FreeCADGui.Selection.clearSelection()

    def set_titles(self):
        """Fills the first column of the table"""

        c1 = QtGui.QStandardItem(translate("BIM", "Name"))
        c1.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(0, 0, c1)
        c2 = QtGui.QStandardItem(translate("BIM", "Type"))
        c2.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(1, 0, c2)
        c3 = QtGui.QStandardItem(translate("BIM", "Parent"))
        c3.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(2, 0, c3)
        c4 = QtGui.QStandardItem(translate("BIM", "Base object"))
        c4.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(3, 0, c4)
        c5 = QtGui.QStandardItem(translate("BIM", "Wires"))
        c5.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(4, 0, c5)
        c6 = QtGui.QStandardItem(translate("BIM", "Thickness"))
        c6.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(5, 0, c6)
        c7 = QtGui.QStandardItem(translate("BIM", "Offset"))
        c7.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(6, 0, c7)
        c8 = QtGui.QStandardItem(translate("BIM", "Hinge"))
        c8.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(7, 0, c8)
        c9 = QtGui.QStandardItem(translate("BIM", "Opening mode"))
        c9.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(8, 0, c9)

    def update_parents(self):
        """Updates the parents in the components list.
        Older files did not specify parents, they were implicit"""

        parent = None
        for cnum in range(int(len(self.WindowParts)/5)):
            name = self.WindowParts[cnum*5]
            parts = self.WindowParts[cnum*5+2].split(",")
            if parent:
                if any([p.startswith('Parent') for p in parts]):
                    pass  # expicit parents override the old way
                else:
                    parts.append("Parent"+parent)
                    self.WindowParts[cnum*5+2] = ",".join(parts)
            if any([p.startswith("Edge") for p in parts]):
                parent = name

    def update_components_list(self):
        """Updates the components list"""

        self.panel1.tree.clear()
        comps = []
        for cnum in range(int(len(self.WindowParts)/5)):
            name = self.WindowParts[cnum*5]
            parts = self.WindowParts[cnum*5+2].split(",")
            parent = [p for p in parts if p.startswith('Parent')]
            comptype = self.WindowParts[cnum*5+1].split(",")[0]
            if parent:
                parent = parent[0][6:]
            comps.append([name, parent, comptype])
        comps.sort(key = lambda comp: bool(comp[1]))
        widgets = {}
        for name, parent, comptype in comps:
            item = QtGui.QTreeWidgetItem([name])
            item.setIcon(0, QtGui.QIcon(":/icons/window_components/"+comptype+".svg"))
            widgets[name] = item
            if parent and (parent in widgets):
                widgets[parent].addChild(item)
            else:
                self.panel1.tree.addTopLevelItem(item)
        self.panel1.tree.expandAll()

    def add_component(self):
        """Adds a new component"""

        name = translate("BIM", "New component")
        self.WindowParts.extend([name, "", "", "", ""])
        self.update_components_list()

    def remove_component(self):
        """Removes a component"""

        name, compindex = self.get_component()
        if not name:
            return
        self.WindowParts = self.WindowParts[:compindex] + self.WindowParts[compindex+5:]
        self.update_components_list()

    def get_component(self):
        """Returns a name and index from the selected component"""

        comp = self.panel1.tree.currentItem()
        if not comp:
            return None, None
        name = comp.text(0)
        if not name:
            return None, None
        if name not in self.WindowParts:
            print("DEBUG: ArchWindowTaskPanel: component not found:", name)
            return None, None
        compindex = self.WindowParts.index(name)
        return name, compindex

    def update_properties(self):
        """Updates the properties panel"""

        self.setting_properties = True
        self.model.clear()
        self.set_titles()
        name, compindex = self.get_component()
        if not name:
            return
        self.current_name = name
        name = QtGui.QStandardItem(name)
        parts1 = self.WindowParts[compindex+1].split(",")  # type, [sketch]
        parts2 = self.WindowParts[compindex+2].split(",")  # wires/def, [hinge], [mode], [parent]
        self.model.setItem(0, 1, name)
        comptype = parts1[0]
        comptype = QtGui.QStandardItem(comptype)
        self.model.setItem(1, 1, comptype)
        parent = [p for p in parts2 if p.startswith("Parent")]
        if parent:
            parent = parent[0][6:]
            parent = QtGui.QStandardItem(parent)
        else:
            parent = QtGui.QStandardItem("")
        self.model.setItem(2, 1, parent)
        base = parts1[1] if len(parts1) > 1 else ""
        if base:
            base = QtGui.QStandardItem(base)
        else:
            base = QtGui.QStandardItem("")
        self.model.setItem(3, 1, base)
        wires = [p for p in parts2 if p.startswith("Wire")]
        if wires:
            wires = ",".join(wires)
            wires = QtGui.QStandardItem(wires)
        else:
            compdef = [p for p in parts2 if (p and p[0].isdigit())]
            if compdef:
                wires = ",".join(compdef)
                wires = QtGui.QStandardItem(wires)
            else:
                wires = QtGui.QStandardItem("")
        self.model.setItem(4, 1, wires)
        thickness = self.WindowParts[compindex+3]
        if not thickness:
            thickness = "0"
        thickness = QtGui.QStandardItem(thickness)
        self.model.setItem(5, 1, thickness)
        offset = self.WindowParts[compindex+4]
        if not offset:
            offset = "0"
        offset = QtGui.QStandardItem(offset)
        self.model.setItem(6, 1, offset)
        hinge = [p for p in parts2 if p.startswith("Edge")]
        if hinge:
            hinge = hinge[0]
            hinge = QtGui.QStandardItem(hinge)
        else:
            hinge = QtGui.QStandardItem("")
        self.model.setItem(7, 1, hinge)
        mode = [p for p in parts2 if p.startswith("Mode")]
        if mode:
            mode = ArchWindow.WindowOpeningModes[int(mode[0][4:])]
            mode = QtGui.QStandardItem(mode)
        else:
            mode = QtGui.QStandardItem("")
        self.model.setItem(8, 1, mode)
        self.setting_properties = False

    def edit_properties(self, item=None):
        """One of the component properties has changed"""

        if self.setting_properties:
            return
        orig_name, compindex = self.get_component()
        if not orig_name or compindex is None:
            return
        name = self.model.item(0, 1).text()
        comptype = self.model.item(1, 1).text()
        parent = self.model.item(2, 1).text()
        if parent:
            parent = "Parent" + parent
        baseobj = self.model.item(3, 1).text()
        wires = self.model.item(4, 1).text()
        thickness = self.model.item(5, 1).text()
        offset = self.model.item(6, 1).text()
        hinge = self.model.item(7, 1).text()
        mode = self.model.item(8, 1).text()
        if mode:
            mode = "Mode" + str(ArchWindow.WindowOpeningModes.index(mode))
        self.WindowParts[compindex] = name
        self.WindowParts[compindex+1] = ",".join([comptype, baseobj])
        self.WindowParts[compindex+2] = ",".join([wires, hinge, mode, parent])
        self.WindowParts[compindex+3] = thickness
        self.WindowParts[compindex+4] = offset
        self.change_parents(orig_name, name)
        self.update_components_list()
        self.select(name)

    def change_parents(self, old_name, new_name):
        """Updates any objects that uses old_name as parent to new_name"""

        for i, pstr in enumerate(list(self.WindowParts)):
            if "Parent"+old_name in pstr:
                self.WindowParts[i] = pstr.replace("Parent"+old_name, "Parent"+new_name)

    def select(self, name):
        """Selects the component with the given name"""

        items = self.panel1.tree.findItems(name, QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive)
        if items:
            self.panel1.tree.setCurrentItem(items[0])

    def accept(self):
        """OK was clicked"""

        if self.WindowParts != self.obj.WindowParts:
            doc = self.obj.Document
            doc.openTransaction(translate("BIM", "Edit window components"))
            self.obj.WindowParts = self.WindowParts
            doc.commitTransaction()
            doc.recompute()
        return self.reject()

    def reject(self):
        """Cancel was clicked"""

        self.obj.ViewObject.Proxy.unsetEdit(self.obj.ViewObject)
        return True

    def dropEvent(self, event):
        """Reimplmented QTreeWidfget.dropEvent.
        if a component is dropped on any other,
        update the parent slot of the component"""

        def crawl_child(item):
            for i in range(item.childCount()):
                child = item.child(i)
                set_parent(child, item)
                crawl_child(child)

        def set_parent(item, parent=None):
            name = item.text(0)
            if name not in self.WindowParts:
                return
            compindex = self.WindowParts.index(name)
            parts = self.WindowParts[compindex+2].split(",")
            for i, p in enumerate(parts):
                if p.startswith("Parent"):
                    if parent:
                        parts[i] = "Parent"+parent.text(0)
                    else:
                        parts[i] = ""
                    break
            else:
                if parent:
                    parts.append("Parent"+parent.text(0))

        print("dropEvent")

        for i in range(self.panel1.tree.topLevelItemCount()):
            print("top level", i)
            child = self.panel1.tree.topLevelItem(i)
            set_parent(child, None)
            crawl_child(child)
        QtGui.QTreeWidget.dropEvent(self, event)

    def invert_opening(self):
        """Inverts the opening direction of this window"""

        if self.obj:
            self.obj.ViewObject.Proxy.invertOpening()
            self.reject()

    def invert_hinge(self):
        """Inverts the hinge direction of this window"""

        if self.obj:
            self.obj.ViewObject.Proxy.invertHinge()
            self.reject()


class window_size_editor(QtGui.QWidget):
    """an editor widget for thickness and offset, that
    has a quantity editor and a checkbox"""

    def __init__(self, propname, *args):
        super().__init__(*args)
        self.uiloader = FreeCADGui.UiLoader()
        self.layout = QtGui.QHBoxLayout()
        self.layout.setContentsMargins(QtCore. QMargins(0, 0, 0, 0))
        self.setLayout(self.layout)
        self.tt = translate("BIM", "If checked, the value of the window's"
                                   "%1 property is added to this value")
        self.tt = self.tt.replace("%1", propname)
        self.val = None
        self.check = False

    def text(self):
        try:
            return self.widget1.text()
        except (RuntimeError, AttributeError):
            return self.val

    def setText(self, text):
        self.val = text
        try:
            self.widget1.setText(text)
        except (RuntimeError, AttributeError):  # this widget might have been destroyed
            self.widget1 = self.uiloader.createWidget("Gui::InputField")
            self.widget1.setParent(self)
            self.layout.addWidget(self.widget1)
            self.widget1.setText(text)
            self.widget1.keyPressEvent = self.keyPressEvent

    def checkState(self):
        try:
            return self.widget2.checkState()
        except (RuntimeError, AttributeError):
            return self.check

    def setCheckState(self, state):
        self.check = state
        try:
            self.widget2.setCheckState(state)
        except (RuntimeError, AttributeError):  # this widget might have been destroyed
            self.widget2 = QtGui.QCheckBox(self)
            self.widget2.setToolTip(self.tt)
            self.layout.addWidget(self.widget2)
            self.widget2.setCheckState(state)

    def keyPressEvent(self, event):
        # do not propagate Enter to parent widget otherwise it closes the editor
        if event.key() == QtCore.Qt.Key_Enter or event.key() == QtCore.Qt.Key_Return:
            event.accept()


class line_edit(QtGui.QLineEdit):
    """reimplemented so it does not spread Enter to parents"""

    def __init__(self, *args):
        super().__init__(*args)

    def keyPressEvent(self, event):
        super().keyPressEvent(event)
        if event.key() == QtCore.Qt.Key_Enter or event.key() == QtCore.Qt.Key_Return:
            event.accept()


class window_component_delegate(QtGui.QStyledItemDelegate):
    """model delegate for window component properties"""

    def __init__(self, task, *args):
        self.task = task
        super().__init__(*args)

    def paint(self, painter, option, index):
        if index.column() == 1:
            self.parent().openPersistentEditor(index)
        super().paint(painter, option, index)

    def createEditor(self, parent, option, index):
        """Creates an editor widget"""

        row = index.row()
        if row == 0:  # name
            editor = line_edit(parent)
        elif row == 1:  # type
            editor = QtGui.QComboBox(parent)
            editor.addItems(ArchWindow.WindowPartTypes)
        elif row == 2:  # parent
            editor = QtGui.QComboBox(parent)
            name = getattr(self.task, "current_name", None)
            items = self.get_component_names_minus(self.task.WindowParts, name)
            editor.addItems(items)
        elif row == 3:  # base object
            editor = QtGui.QPushButton(parent)
            editor.clicked.connect(self.on_click_baseobject)
            self.editor_baseobj = editor
            editor.setEnabled(False)
        elif row == 4:  # wires
            editor = QtGui.QPushButton(parent)
            editor.clicked.connect(self.on_click_wires)
            self.editor_wires = editor
        elif row == 5:  # thickness
            editor = window_size_editor("Thickness", parent)
        elif row == 6:  # offset
            editor = window_size_editor("Offset", parent)
        elif row == 7:  # hinge
            editor = QtGui.QPushButton(parent)
            editor.setToolTip(translate("BIM", "Select an edge and press this button"))
            editor.clicked.connect(self.on_click_hinge)
            self.editor_hinge = editor
        elif row == 8:  # mode
            editor = QtGui.QComboBox(parent)
            editor.addItems(ArchWindow.WindowOpeningModes)
        return editor

    def setEditorData(self, editor, index):
        """Fills the editor widget with current data"""

        row = index.row()
        if row in [0, 4, 7]:
            editor.setText(index.data())
        elif row == 1:
            if index.data():
                editor.setCurrentIndex(ArchWindow.WindowPartTypes.index(index.data()))
        elif row == 2:
            idx = editor.findText(index.data(), QtCore.Qt.MatchExactly)
            if idx >= 0:
                editor.setCurrentText(index.data())
        elif row == 3:
            editor.setText(getattr(self.task.obj.Base, "Label", ""))
        elif row in [5, 6]:
            if index.data():
                editor.setText(index.data().replace("+V", ""))
                if "+V" in index.data():
                    editor.setCheckState(QtCore.Qt.Checked)
                else:
                    editor.setCheckState(QtCore.Qt.Unchecked)
            else:
                editor.setCheckState(QtCore.Qt.Unchecked)
        elif row == 8:
            if index.data():
                editor.setCurrentIndex(ArchWindow.WindowOpeningModes.index(index.data()))

    def setModelData(self, editor, model, index):
        """Saves the changed data"""

        row = index.row()
        if row == 0:
            model.setData(index, editor.text())
        elif row in [4, 7]:
            # this is set directly by the appropriate tools
            model.setData(index, editor.text())
        elif row == 1:
            model.setData(index, ArchWindow.WindowPartTypes[editor.currentIndex()])
        elif row == 2:
            model.setData(index, editor.currentText())
        elif row == 3:
            model.setData(index, "")
        elif row in [5, 6]:
            val = editor.text()
            if editor.checkState():
                val += "+V"
            model.setData(index, val)
        elif row == 8:
            model.setData(index, ArchWindow.WindowOpeningModes[editor.currentIndex()])
        self.task.edit_properties()

    def get_component_names_minus(self, windowparts, name):
        """Returns all the compoonent names minus the given one"""

        cnames = ["",]
        for cnum in range(int(len(windowparts)/5)):
            cname = windowparts[cnum*5]
            if cname != name:
                cnames.append(cname)
        return cnames

    def on_click_baseobject(self):
        """Select a base object"""

        for sel in FreeCADGui.Selection.getSelectionEx():
            for obn in sel.ObjectNames:
                if obn != self.task.obj.Name:
                    self.editor_baseobj.setText(obn)
                    return
        self.editor_baseobj.setText("")

    def on_click_wires(self):
        """Select wires, or opens the componenbt definition dialog"""

        if not self.task.obj.Base:
            self.on_define()
            return
        menu = QtGui.QMenu()
        if self.task.obj.Base:
            wires = ["Wire"+str(i) for i in list(range(len(self.task.obj.Base.Shape.Wires)))]
            for w in wires:
                act = menu.addAction(w)
                act.triggered.connect(self.on_select_wire)
                act.hovered.connect(self.on_hover_wire)
        act_define = menu.addAction(translate("BIM", "Define..."))
        act_define.triggered.connect(self.on_define)
        pos = FreeCADGui.getMainWindow().cursor().pos()
        menu.exec_(pos)

    def on_hover_wire(self):
        """A wire is hovered with the mouse"""

        wire = int(self.sender().text()[4:])
        if self.task.obj.Base:
            if len(self.task.obj.Base.Shape.Wires) <= wire:
                return
            wire = self.task.obj.Base.Shape.Wires[wire]
            FreeCADGui.Selection.clearSelection()
            for edge in wire.Edges:
                for i, bedge in enumerate(self.task.obj.Base.Shape.Edges):
                    if bedge.hashCode() == edge.hashCode():
                        FreeCADGui.Selection.addSelection(self.task.obj.Base, "Edge"+str(i+1))
                        break

    def on_select_wire(self):
        """A wire has been selected in the menu"""

        FreeCADGui.Selection.clearSelection()
        wire = self.sender().text()
        wires = self.editor_wires.text().split(",")
        if wire in wires:
            wires.pop(wires.index(wire))
        else:
            wires += [wire]
        self.editor_wires.setText(",".join(wires))
        self.task.model.setData(self.task.model.index(4, 1), ",".join(wires))

    def on_click_hinge(self):
        """Selects an hinge"""

        for sel in FreeCADGui.Selection.getSelectionEx():
            for sub in sel.SubElementNames:
                if "Edge" in sub:
                    self.editor_hinge.setText(sub)
                    self.task.model.setData(self.task.model.index(7, 1), sub)
                    return
        self.editor_hinge.setText("")
        self.task.model.setData(self.task.model.index(7, 1), "")

    def on_define(self):
        """Define one's own wire"""

        self.panel3 = FreeCADGui.PySideUic.loadUi(":/ui/dialogWindowComponentEditor.ui")
        self.expand_thickness(False)
        if hasattr(self.panel3.checkThickness, "checkStateChanged"):
            self.panel3.checkThickness.checkStateChanged.connect(self.expand_thickness)
        else:  # Qt5 and older Qt6
            self.panel3.checkThickness.stateChanged.connect(self.expand_thickness)
        name, compindex = self.task.get_component()
        if not name:
            return
        parts = []
        if self.task.WindowParts[compindex+2]:
            parts = self.task.WindowParts[compindex+2].split(",")  # wires, hinge, mode, parent
        compdef = []
        for p in parts:
            try:
                v = FreeCAD.Units.Quantity(p).UserString
                compdef.append(v)
            except ValueError:
                pass
        if len(compdef) > 0:
            self.panel3.fieldX.setText(compdef[0])
        if len(compdef) > 1:
            self.panel3.fieldY.setText(compdef[1])
        if len(compdef) > 2:
            self.panel3.fieldHeight.setText(compdef[2])
        if len(compdef) > 3:
            self.panel3.fieldWidth.setText(compdef[3])
        if len(compdef) >= 8:
            try:
                self.panel3.checkThickness.setCheckState(QtCore.Qt.Checked)
            except:
                self.panel3.checkThickness.setChecked(True)
            if len(compdef) > 5:
                self.panel3.fieldTopThickness.setText(compdef[5])
            if len(compdef) > 6:
                self.panel3.fieldRightThickness.setText(compdef[6])
            if len(compdef) > 7:
                self.panel3.fieldBottomThickness.setText(compdef[7])
            if len(compdef) > 8:
                self.panel3.fieldLeftThickness.setText(compdef[8])
        elif len(compdef) > 4:
            self.panel3.fieldThickness.setText(compdef[4])
        result = self.panel3.exec()
        if result:
            x = FreeCAD.Units.Quantity(self.panel3.fieldX.text()).Value
            y = FreeCAD.Units.Quantity(self.panel3.fieldY.text()).Value
            h = FreeCAD.Units.Quantity(self.panel3.fieldHeight.text()).Value
            w = FreeCAD.Units.Quantity(self.panel3.fieldWidth.text()).Value
            if self.panel3.checkThickness.checkState() in [False, QtCore.Qt.Unchecked]:
                t = FreeCAD.Units.Quantity(self.panel3.fieldThickness.text()).Value
            else:
                t1 = FreeCAD.Units.Quantity(self.panel3.fieldTopThickness.text()).Value
                t2 = FreeCAD.Units.Quantity(self.panel3.fieldRightThickness.text()).Value
                t3 = FreeCAD.Units.Quantity(self.panel3.fieldBottomThickness.text()).Value
                t4 = FreeCAD.Units.Quantity(self.panel3.fieldLeftThickness.text()).Value
                t = ",".join([str(i) for i in [t1, t2, t3, t4]])
            txt = ",".join([str(i) for i in [x, y, h, w, t]])
            self.editor_wires.setText(txt)
            self.task.model.setData(self.task.model.index(4, 1), txt)

    def expand_thickness(self, state):
        """Shows/hides extra thicknesses"""

        if state in [True, QtCore.Qt.Checked]:
            self.panel3.labelThickness.hide()
            self.panel3.fieldThickness.hide()
            self.panel3.labelTopThickness.show()
            self.panel3.fieldTopThickness.show()
            self.panel3.labelRightThickness.show()
            self.panel3.fieldRightThickness.show()
            self.panel3.labelBottomThickness.show()
            self.panel3.fieldBottomThickness.show()
            self.panel3.labelLeftThickness.show()
            self.panel3.fieldLeftThickness.show()
        else:
            self.panel3.labelThickness.show()
            self.panel3.fieldThickness.show()
            self.panel3.labelTopThickness.hide()
            self.panel3.fieldTopThickness.hide()
            self.panel3.labelRightThickness.hide()
            self.panel3.fieldRightThickness.hide()
            self.panel3.labelBottomThickness.hide()
            self.panel3.fieldBottomThickness.hide()
            self.panel3.labelLeftThickness.hide()
            self.panel3.fieldLeftThickness.hide()
