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

        self.panel2 = FreeCADGui.PySideUic.loadUi(":/ui/taskWindowProperties.ui")
        self.model = QtGui.QStandardItemModel(9, 2)
        self.panel2.table.setModel(self.model)
        self.model.itemChanged.connect(self.edit_properties)
        self.panel2.table.horizontalHeader().hide()
        self.panel2.table.verticalHeader().hide()
        self.panel2.table.horizontalHeader().setStretchLastSection(True)
        self.set_titles()
        self.form = [self.panel1, self.panel2]
        

    def set_titles(self):
        """Fills the first column of the table"""

        c1 = QtGui.QStandardItem(translate("BIM", "Name"))
        c1.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(0, 0, c1)
        c2 = QtGui.QStandardItem(translate("BIM", "Parent"))
        c2.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(1, 0, c2)
        c3 = QtGui.QStandardItem(translate("BIM", "Base object"))
        c3.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(2, 0, c3)
        c4 = QtGui.QStandardItem(translate("BIM", "Wires"))
        c4.setFlags(QtCore.Qt.NoItemFlags)
        self.model.setItem(3, 0, c4)
        c5 = QtGui.QStandardItem(translate("BIM", "Type"))
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
            if parent:
                parent = parent[0][6:]
            comps.append([name, parent])
        comps.sort(key = lambda comp: bool(comp[1]))
        widgets = {}
        for name, parent in comps:
            item = QtGui.QTreeWidgetItem([name])
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
        if not name or not compindex:
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
        if not name in self.WindowParts:
            print("DEBUG: ArchWindowTaskPanel: component not found:",name)
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
        name = QtGui.QStandardItem(name)
        self.model.setItem(0, 1, name)
        parts2 = self.WindowParts[compindex+2].split(",")
        parent = [p for p in parts2 if p.startswith("Parent")]
        if parent:
            parent = parent[0][6:]
            parent = QtGui.QStandardItem(parent)
        else:
            parent = QtGui.QStandardItem("")
        self.model.setItem(1, 1, parent)
        parts1 = self.WindowParts[compindex+1].split(",")
        base = parts1[1] if len(parts1) > 1 else ""
        if base:
            base = QtGui.QStandardItem(base)
        else:
            base = QtGui.QStandardItem("")
        self.model.setItem(2, 1, base)
        wires = [p for p in parts2 if p.startswith("Wire")]
        if wires:
            wires = ",".join(wires)
            wires = QtGui.QStandardItem(wires)
        else:
            wires = QtGui.QStandardItem("")
        self.model.setItem(3, 1, wires)
        comptype = parts1[0]
        comptype = QtGui.QStandardItem(comptype)
        self.model.setItem(4, 1, comptype)
        thickness = self.WindowParts[compindex+3]
        thickness = QtGui.QStandardItem(thickness)
        self.model.setItem(5, 1, thickness)
        offset = self.WindowParts[compindex+4]
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
            mode = mode[0][4:]
            mode = QtGui.QStandardItem(mode)
        else:
            mode = QtGui.QStandardItem("")
        self.model.setItem(8, 1, mode)
        self.setting_properties = False

    def edit_properties(self, item):
        """One of the component properties has changed"""

        if self.setting_properties:
            return
        orig_name, compindex = self.get_component()
        if not orig_name or not compindex:
            return
        name = self.model.item(0, 1).text()
        parent = self.model.item(1, 1).text()
        if parent:
            parent = "Parent" + parent
        baseobj = self.model.item(2, 1).text()
        wires = self.model.item(3, 1).text()
        comptype = self.model.item(4, 1).text()
        thickness = self.model.item(5, 1).text()
        offset = self.model.item(6, 1).text()
        hinge = self.model.item(7, 1).text()
        mode = self.model.item(8, 1).text()
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

        items = self.panel1.tree.findItems(name, QtCore.Qt.MatchExactly|QtCore.Qt.MatchRecursive)
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
            if not name in self.WindowParts:
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

        for i in range(self.panel1.tree.topLevelItemCount()):
            child = self.panel1.tree.topLevelItem(i)
            set_parent(item, None)
            crawl_child(child)
        QtGui.QTreeWidget.dropEvent(self, event)

    def get_component_names_minus(name):
        """Returns all the compoonent names minus the given one"""

        cnames = []
        for cnum in range(int(len(self.WindowParts)/5)):
            cname = self.WindowParts[cnum*5]
            if cname != name:
                cnames.append(cname)
        return cnames


