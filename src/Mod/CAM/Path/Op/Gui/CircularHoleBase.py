# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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
import Path
import Path.Op.Gui.Base as PathOpGui
import PathGui

from Path.Base.Gui.Util import NaturalSortTableWidgetItem
from PySide import QtCore, QtGui

__title__ = "Base for Circular Hole based operations' UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Implementation of circular hole specific base geometry page controller."

LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.NOTICE, Path.Log.thisModule())


# Column indices for baseList table
COL_ORDER = 0
COL_FEATURE = 1
COL_DIAMETER = 2


class RowMoveTableWidget(QtGui.QTableWidget):
    rowsReordered = QtCore.Signal()

    def __init__(self, parent=None, controller=None):
        super(RowMoveTableWidget, self).__init__(parent)
        self.controller = controller

    def dropEvent(self, event):
        try:
            self.cellChanged.disconnect()
        except TypeError:
            pass  # Already disconnected

        selected_rows = sorted(set(idx.row() for idx in self.selectedIndexes()))
        drop_index = self.indexAt(event.pos())
        # If dropping below the last row, insert at the end
        if not drop_index.isValid() or self.rowCount() - 1 == drop_index.row():
            drop_row = self.rowCount()
        else:
            drop_row = drop_index.row()  # Insert below the target row

        # Save row data
        rows_data = []
        for row in selected_rows:
            row_items = []
            for col in range(self.columnCount()):
                item = self.item(row, col)
                row_items.append(item.clone() if item else None)
            rows_data.append(row_items)

        # Remove original rows (from bottom up)
        for row in reversed(selected_rows):
            self.removeRow(row)
            if row < drop_row:
                drop_row -= 1

        # Insert at the drop position
        for i, row_items in enumerate(rows_data):
            self.insertRow(drop_row + i)
            for col, item in enumerate(row_items):
                if item:
                    self.setItem(drop_row + i, col, item)

        # Select the moved rows
        self.clearSelection()
        for i in range(len(rows_data)):
            for col in range(self.columnCount()):
                self.item(drop_row + i, col).setSelected(True)

        event.accept()

        # Remove any blank rows that may have been created (Qt bug workaround)
        for row in reversed(range(self.rowCount())):
            is_blank = True
            for col in range(self.columnCount()):
                item = self.item(row, col)
                if item and item.text().strip():
                    is_blank = False
                    break
            if is_blank:
                self.removeRow(row)

        self.rowsReordered.emit()

        # Reconnect cellChanged after drag-and-drop
        if self.controller:
            self.cellChanged.connect(self.controller.cellManuallyChanged)


class TaskPanelHoleGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    """Controller class to be used for the BaseGeomtery page.
    Circular holes don't just display the feature, they also add a column
    displaying the radius the feature describes. This page provides that
    UI and functionality for all circular hole based operations."""

    DataFeatureName = QtCore.Qt.ItemDataRole.UserRole
    DataObject = QtCore.Qt.ItemDataRole.UserRole + 1
    DataObjectSub = QtCore.Qt.ItemDataRole.UserRole + 2

    InitBase = False

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseHoleGeometryEdit.ui")
        old_table = form.baseList
        parent = old_table.parent()
        layout = parent.layout()
        new_table = RowMoveTableWidget(parent, controller=self)
        new_table.setObjectName("baseList")
        new_table.setColumnCount(old_table.columnCount())
        headers = []
        for col in range(old_table.columnCount()):
            header_item = old_table.horizontalHeaderItem(col)
            headers.append(header_item.text() if header_item else "")
        new_table.setHorizontalHeaderLabels(headers)
        new_table.setGeometry(old_table.geometry())
        new_table.setMinimumSize(old_table.minimumSize())
        new_table.setMaximumSize(old_table.maximumSize())
        new_table.setSizePolicy(old_table.sizePolicy())
        new_table.setCornerButtonEnabled(old_table.isCornerButtonEnabled())
        new_table.setSelectionBehavior(old_table.selectionBehavior())
        new_table.setSelectionMode(old_table.selectionMode())
        new_table.setDragDropMode(old_table.dragDropMode())
        new_table.setDragDropOverwriteMode(old_table.dragDropOverwriteMode())
        # new_table.setSortingEnabled(old_table.isSortingEnabled())
        new_table.setSortingEnabled(False)
        new_table.verticalHeader().setVisible(old_table.verticalHeader().isVisible())
        new_table.horizontalHeader().setStretchLastSection(
            old_table.horizontalHeader().stretchLastSection()
        )

        for i in range(layout.count()):
            if layout.itemAt(i).widget() == old_table:
                layout.removeWidget(old_table)
                old_table.setParent(None)
                layout.insertWidget(i, new_table)
                break
        form.baseList = new_table

        # Set sortMode QComboBox to current obj.SortingMode
        if hasattr(self, "obj") and hasattr(self.obj, "SortingMode"):
            idx = form.sortMode.findText(self.obj.SortingMode)
            if idx >= 0:
                form.sortMode.setCurrentIndex(idx)

        self.lineEditFilter = LineEditEventFilter()
        form.lineEdit.installEventFilter(self.lineEditFilter)

        return form

    def onSortModeChanged(self, text):
        if hasattr(self, "obj") and hasattr(self.obj, "SortingMode"):
            self.obj.SortingMode = text
            # Enable/disable sorting and drag-drop based on SortingMode
            auto_mode = text == "Automatic"

            if auto_mode:
                self.form.baseList.setSortingEnabled(True)
                self.form.baseList.setDragDropMode(QtGui.QAbstractItemView.NoDragDrop)
            else:  # Manual
                self.form.baseList.setSortingEnabled(False)
                self.form.baseList.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
                self.updateOrderNumbers()
            self.form.baseList.blockSignals(True)
            for row in range(self.form.baseList.rowCount()):
                item = self.form.baseList.item(row, COL_ORDER)
                if item:
                    if auto_mode:
                        item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                    else:
                        item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
            self.form.baseList.blockSignals(False)
            FreeCAD.ActiveDocument.recompute()

    def initPage(self, obj):
        self.updating = False

    def setFields(self, obj):
        """setFields(obj) ... fill form with values from obj"""
        Path.Log.track()
        self.form.baseList.blockSignals(True)
        self.form.baseList.clearContents()
        self.form.baseList.setRowCount(0)
        auto_mode = obj.SortingMode == "Automatic"

        for base, subs in obj.Base:
            for sub in subs:
                self.form.baseList.insertRow(self.form.baseList.rowCount())

                order_number = self.form.baseList.rowCount()
                order_item = QtGui.QTableWidgetItem()
                order_item.setData(QtCore.Qt.DisplayRole, order_number)
                order_item.setTextAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
                order_item.setFlags(order_item.flags() & ~QtCore.Qt.ItemIsDropEnabled)
                self.form.baseList.setItem(self.form.baseList.rowCount() - 1, COL_ORDER, order_item)

                item = NaturalSortTableWidgetItem("%s.%s" % (base.Label, sub))
                item.setFlags(
                    (item.flags() | QtCore.Qt.ItemIsUserCheckable) & ~QtCore.Qt.ItemIsEditable
                )
                if obj.Proxy.isHoleEnabled(obj, base, sub):
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
                name = "%s.%s" % (base.Name, sub)
                item.setData(self.DataFeatureName, name)
                item.setData(self.DataObject, base)
                item.setData(self.DataObjectSub, sub)

                if auto_mode:
                    order_item.setFlags(order_item.flags() & ~QtCore.Qt.ItemIsEditable)
                else:
                    order_item.setFlags(order_item.flags() | QtCore.Qt.ItemIsEditable)

                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsDropEnabled)
                self.form.baseList.setItem(self.form.baseList.rowCount() - 1, COL_FEATURE, item)

                dia = obj.Proxy.holeDiameter(obj, base, sub)
                diameterQty = FreeCAD.Units.Quantity(dia, FreeCAD.Units.Length)
                displayString = diameterQty.getUserPreferred("Length")[0]
                item = NaturalSortTableWidgetItem(displayString)
                item.setTextAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsDropEnabled)
                self.form.baseList.setItem(self.form.baseList.rowCount() - 1, COL_DIAMETER, item)

        # Ensure the combo box reflects the current SortingMode
        idx = self.form.sortMode.findText(obj.SortingMode)
        if idx >= 0:
            self.form.sortMode.setCurrentIndex(idx)

        # Enable/disable sorting and drag-drop based on SortingMode
        if auto_mode:
            self.form.baseList.setSortingEnabled(True)
            self.form.baseList.setDragDropMode(QtGui.QAbstractItemView.NoDragDrop)
        else:  # Manual
            self.form.baseList.setSortingEnabled(False)
            self.form.baseList.setDragDropMode(QtGui.QAbstractItemView.InternalMove)

        header = self.form.baseList.horizontalHeader()
        header.setSectionResizeMode(COL_ORDER, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_FEATURE, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_DIAMETER, QtGui.QHeaderView.ResizeToContents)

        self.form.baseList.resizeColumnToContents(0)
        self.form.baseList.blockSignals(False)
        self.updateSelectAllCheckbox()
        self.itemActivated()

        # Set selection behavior and drag-and-drop modes
        self.form.baseList.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.form.baseList.setDragDropOverwriteMode(False)

    def itemActivated(self):
        """itemActivated() ... callback when item in table is selected"""
        Path.Log.track()
        selected_rows = set(item.row() for item in self.form.baseList.selectedItems())
        if selected_rows:
            self.form.deleteBase.setEnabled(True)
            FreeCADGui.Selection.clearSelection()
            for row in selected_rows:
                item = self.form.baseList.item(row, COL_FEATURE)
                obj = item.data(self.DataObject)
                sub = str(item.data(self.DataObjectSub))
                if obj is not None:
                    Path.Log.debug("itemActivated() -> %s.%s" % (obj.Label, sub))
                    if sub:
                        FreeCADGui.Selection.addSelection(obj, sub)
                    else:
                        FreeCADGui.Selection.addSelection(obj)
        else:
            self.form.deleteBase.setEnabled(False)

    def deleteBase(self):
        """deleteBase() ... callback for Remove button"""
        Path.Log.track()
        selected = [self.form.baseList.row(item) for item in self.form.baseList.selectedItems()]
        self.form.baseList.blockSignals(True)
        for row in sorted(list(set(selected)), key=lambda row: -row):
            self.form.baseList.removeRow(row)
        self.updateBase()
        self.form.baseList.resizeColumnToContents(0)
        self.form.baseList.blockSignals(False)
        # self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()
        self.setFields(self.obj)

    def updateBase(self):
        """updateBase() ... helper function to transfer current table to obj"""
        Path.Log.track()
        newlist = []
        for i in range(self.form.baseList.rowCount()):
            item = self.form.baseList.item(i, COL_FEATURE)
            obj = item.data(self.DataObject)
            sub = str(item.data(self.DataObjectSub))
            base = (obj, sub)
            Path.Log.debug("keeping (%s.%s)" % (obj.Label, sub))
            newlist.append(base)
        Path.Log.debug("obj.Base=%s newlist=%s" % (self.obj.Base, newlist))
        self.updating = True
        self.obj.Base = newlist
        self.updating = False

    def checkedChanged(self):
        """checkeChanged() ... callback when checked status of a base feature changed"""
        Path.Log.track()
        disabled = []
        for i in range(0, self.form.baseList.rowCount()):
            item = self.form.baseList.item(i, COL_FEATURE)
            if item.checkState() != QtCore.Qt.Checked:
                disabled.append(item.data(self.DataFeatureName))
        self.obj.Disabled = disabled
        FreeCAD.ActiveDocument.recompute()

    def updateOrderNumbers(self):
        """Update the order numbers in the first column after row reordering or sorting."""
        table = self.form.baseList
        for row in range(table.rowCount()):
            item = table.item(row, COL_ORDER)
            if item:
                item.setData(QtCore.Qt.DisplayRole, row + 1)
        self.updateBase()
        self.filterBaseList(self.form.lineEdit.text())  # Reapply filter after
        FreeCAD.ActiveDocument.recompute()

    def updateSelectAllCheckbox(self):
        """Set the Select All checkbox state based on visible rows."""
        all_checked = True
        any_visible = False
        for row in range(self.form.baseList.rowCount()):
            if not self.form.baseList.isRowHidden(row):
                any_visible = True
                item = self.form.baseList.item(row, COL_FEATURE)
                if item.checkState() != QtCore.Qt.Checked:
                    all_checked = False
                    break
        if not any_visible:
            self.form.checkBox.setCheckState(QtCore.Qt.Unchecked)
        else:
            self.form.checkBox.setCheckState(
                QtCore.Qt.Checked if all_checked else QtCore.Qt.Unchecked
            )

    def registerSignalHandlers(self, obj):
        """registerSignalHandlers(obj) ... setup signal handlers"""
        self.form.baseList.rowsReordered.connect(self.updateOrderNumbers)
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.resetBase.clicked.connect(self.resetBase)
        self.form.clearBase.clicked.connect(self.clearBase)
        self.form.baseList.itemChanged.connect(self.itemChanged)
        self.form.lineEdit.textChanged.connect(self.filterBaseList)
        self.form.checkBox.stateChanged.connect(self.selectVisibleRows)
        self.form.baseList.cellChanged.connect(self.cellManuallyChanged)
        self.form.sortMode.currentTextChanged.connect(self.onSortModeChanged)
        self.form.baseList.horizontalHeader().sortIndicatorChanged.connect(
            self.onSortIndicatorChanged
        )

    def onSortIndicatorChanged(self, column, order):
        """Handle when the user clicks a column header to sort"""
        # Force a complete UI refresh
        self.form.baseList.viewport().update()
        # If in Automatic mode, make sure sorting is actually applied
        if getattr(self.obj, "SortingMode", None) == "Automatic":
            # Force immediate sort
            self.form.baseList.sortItems(column, order)

    def selectVisibleRows(self, state):
        """Check/uncheck all visible checkboxes in the table based on Select All checkbox."""
        check_state = (
            QtCore.Qt.Checked
            if QtCore.Qt.CheckState(state) == QtCore.Qt.Checked
            else QtCore.Qt.Unchecked
        )
        for row in range(self.form.baseList.rowCount()):
            if not self.form.baseList.isRowHidden(row):
                item = self.form.baseList.item(row, COL_FEATURE)
                if item and item.flags() & QtCore.Qt.ItemIsUserCheckable:
                    item.setCheckState(check_state)

    def filterBaseList(self, text):
        """Filter baseList rows based on the filter text in any column."""
        text = text.lower()

        if not text:
            self.form.checkBox.blockSignals(True)
            self.form.checkBox.setCheckState(QtCore.Qt.Unchecked)
            self.form.checkBox.blockSignals(False)

        for row in range(self.form.baseList.rowCount()):
            item0 = self.form.baseList.item(row, COL_FEATURE)
            item1 = self.form.baseList.item(row, COL_DIAMETER)
            col0 = item0.text().lower() if item0 else ""
            col1 = item1.text().lower() if item1 else ""
            if text in col0 or text in col1:
                self.form.baseList.setRowHidden(row, False)
            else:
                self.form.baseList.setRowHidden(row, True)

    def resetBase(self):
        """resetBase() ... Auto-select button callback"""
        self.obj.Base = []
        self.obj.Disabled = []
        self.form.baseList.horizontalHeader().setSortIndicator(-1, QtCore.Qt.AscendingOrder)
        self.obj.Proxy.findAllHoles(self.obj)
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def clearBase(self):
        """clearBase() ... Clear All button callback"""
        self.obj.Base = []
        self.obj.Disabled = []
        self.form.baseList.horizontalHeader().setSortIndicator(-1, QtCore.Qt.AscendingOrder)
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def updateData(self, obj, prop):
        """updateData(obj, prop) ... callback whenever a property of the model changed"""
        if not self.updating and prop in ["Base", "Disabled"]:
            self.setFields(obj)

    def cellManuallyChanged(self, row, column):
        if column == COL_ORDER:
            item = self.form.baseList.item(row, column)
            try:
                # Subtract 1 from the current order number so that it will be pushed to the top of the stack
                self.form.baseList.blockSignals(True)
                item.setText(str(int(item.text()) - 1))
                self.form.baseList.blockSignals(False)
            except (ValueError, AttributeError):
                # If the input is invalid, reset to the original order number
                self.form.baseList.blockSignals(True)
                item.setText(str(row + 1))
                self.form.baseList.blockSignals(False)
                return  # Ignore invalid input

            # Resort rows based on the new order numbers
            rows = []
            for row in range(self.form.baseList.rowCount()):
                order_item = self.form.baseList.item(row, COL_ORDER)
                feature_item = self.form.baseList.item(row, COL_FEATURE)
                diameter_item = self.form.baseList.item(row, COL_DIAMETER)
                try:
                    order = int(order_item.text())
                except (ValueError, AttributeError):
                    order = row + 1
                rows.append(
                    (
                        order,
                        [
                            order_item.clone() if order_item else None,
                            feature_item.clone() if feature_item else None,
                            diameter_item.clone() if diameter_item else None,
                        ],
                    )
                )
            # Sort rows by the order number
            rows.sort(key=lambda x: x[0])
            # Rebuild the table
            self.form.baseList.blockSignals(True)
            self.form.baseList.setRowCount(0)
            for idx, (order, items) in enumerate(rows):
                self.form.baseList.insertRow(idx)
                # Set consecutive order numbers
                items[0].setData(QtCore.Qt.DisplayRole, idx + 1)
                for col, item in enumerate(items):
                    if item:
                        self.form.baseList.setItem(idx, col, item)
            self.form.baseList.blockSignals(False)
            self.updateOrderNumbers()
            self.updateBase()
            self.filterBaseList(self.form.lineEdit.text())  # Reapply filter after reordering

    def itemChanged(self, item):
        """itemChanged(item) ... callback when any item in the table changes"""
        if item.column() == COL_FEATURE:
            self.checkedChanged()


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Base class for circular hole based operation's page controller."""

    def taskPanelBaseGeometryPage(self, obj, features):
        """taskPanelBaseGeometryPage(obj, features) ... Return circular hole specific page controller for Base Geometry."""
        return TaskPanelHoleGeometryPage(obj, features)


class LineEditEventFilter(QtCore.QObject):
    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.KeyPress and event.key() in (
            QtCore.Qt.Key_Return,
            QtCore.Qt.Key_Enter,
        ):
            # Prevent Enter key from propagating to the main form
            return True
        return super(LineEditEventFilter, self).eventFilter(obj, event)
