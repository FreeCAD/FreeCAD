# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
from PySide import QtGui, QtCore
from ..models.component import MachineComponent


class MachineComponentTreeItem:
    """A node in the MachineComponentTreeView's model."""

    def __init__(self, component: MachineComponent, parent=None):
        self.component = component
        self.parent_item = parent
        self.child_items = []

    def append_child(self, item):
        self.child_items.append(item)

    def child(self, row: int):
        return self.child_items[row]

    def child_count(self):
        return len(self.child_items)

    def column_count(self):
        return 1  # We only display the label

    def data(self, column: int):
        if column == 0:
            return self.component.label
        return None

    def parent(self):
        return self.parent_item

    def row(self):
        if self.parent_item:
            return self.parent_item.child_items.index(self)
        return 0


class MachineComponentTreeModel(QtCore.QAbstractItemModel):
    """
    A model for displaying MachineComponent instances in a QTreeView.
    """

    def __init__(self, root_component: MachineComponent, parent=None):
        super().__init__(parent)
        self._root_component = root_component
        self._root_item = MachineComponentTreeItem(root_component)
        self._setup_model_data(self._root_component, self._root_item)

    def _setup_model_data(self, component: MachineComponent, parent_item: MachineComponentTreeItem):
        """Recursively builds the tree model from MachineComponent hierarchy."""
        for child_component in component.children:
            child_item = MachineComponentTreeItem(child_component, parent_item)
            parent_item.append_child(child_item)
            self._setup_model_data(child_component, child_item)

    def columnCount(self, parent: QtCore.QModelIndex):
        return 1

    def data(self, index: QtCore.QModelIndex, role: int):
        if not index.isValid():
            return None

        if role != QtCore.Qt.DisplayRole:
            return None

        item = index.internalPointer()
        return item.data(index.column())

    def index(self, row: int, column: int, parent: QtCore.QModelIndex):
        if not self.hasIndex(row, column, parent):
            return QtCore.QModelIndex()

        if not parent.isValid():
            parent_item = self._root_item
        else:
            parent_item = parent.internalPointer()

        child_item = parent_item.child(row)
        if child_item:
            return self.createIndex(row, column, child_item)
        return QtCore.QModelIndex()

    def parent(self, index: QtCore.QModelIndex):
        if not index.isValid():
            return QtCore.QModelIndex()

        child_item = index.internalPointer()
        parent_item = child_item.parent()

        if parent_item == self._root_item:
            return QtCore.QModelIndex()

        return self.createIndex(parent_item.row(), 0, parent_item)

    def rowCount(self, parent: QtCore.QModelIndex):
        if parent.column() > 0:
            return 0

        if not parent.isValid():
            parent_item = self._root_item
        else:
            parent_item = parent.internalPointer()

        return parent_item.child_count()


class MachineComponentTreeView(QtGui.QTreeView):
    """
    A QTreeView specifically for displaying MachineComponent hierarchy.
    """

    componentSelected = QtCore.Signal(object)  # Emits the selected MachineComponent

    def __init__(self, root_component: MachineComponent, parent=None):
        super().__init__(parent)
        self.setModel(MachineComponentTreeModel(root_component))
        self.setHeaderHidden(True)  # Hide header as we only have one column
        self.expandAll()
        self.clicked.connect(self._on_item_clicked)
        self._selected_index = QtCore.QModelIndex()

    def _on_item_clicked(self, index: QtCore.QModelIndex):
        if index.isValid():
            self._selected_index = index
            item = index.internalPointer()
            self.componentSelected.emit(item.component)

    def update_selected_component_label(self, new_label: str):
        if self._selected_index.isValid():
            item = self._selected_index.internalPointer()
            item.component.label = new_label
            self.model().dataChanged.emit(self._selected_index, self._selected_index)
