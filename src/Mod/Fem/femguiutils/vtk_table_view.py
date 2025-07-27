# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD table view widget to visualize vtkTable"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package vtk_table_view
#  \ingroup FEM
#  \brief A Qt widget to show a vtkTable

from PySide import QtGui
from PySide import QtCore

import FreeCAD
import FreeCADGui

from vtkmodules.vtkIOCore import vtkDelimitedTextWriter

translate = FreeCAD.Qt.translate


class VtkTableModel(QtCore.QAbstractTableModel):
    # Simple table model. Only supports single component columns
    # One can supply a header_names dict to replace the table column names
    # in the header. It is a dict "column_idx (int)" to "new name"" or
    # "orig_name (str)" to "new name"

    def __init__(self, header_names=None):
        super().__init__()
        self._table = None
        if header_names:
            self._header = header_names
        else:
            self._header = {}

    def setTable(self, table, header_names=None):
        self.beginResetModel()
        self._table = table
        if header_names:
            self._header = header_names
        self.endResetModel()

    def rowCount(self, index):

        if not self._table:
            return 0

        return self._table.GetNumberOfRows()

    def columnCount(self, index):

        if not self._table:
            return 0

        return self._table.GetNumberOfColumns()

    def data(self, index, role):

        if not self._table:
            return None

        if role == QtCore.Qt.DisplayRole:
            col = self._table.GetColumn(index.column())
            return col.GetTuple(index.row())[0]

        return None

    def headerData(self, section, orientation, role):

        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            if section in self._header:
                return self._header[section]

            name = self._table.GetColumnName(section)
            if name in self._header:
                return self._header[name]

            return name

        if orientation == QtCore.Qt.Vertical and role == QtCore.Qt.DisplayRole:
            return section

        return None

    def getTable(self):
        return self._table


class VtkTableSummaryModel(QtCore.QAbstractTableModel):
    # Simple model showing a summary of the table.
    # Only supports single component columns

    def __init__(self):
        super().__init__()
        self._table = None

    def setTable(self, table):
        self.beginResetModel()
        self._table = table
        self.endResetModel()

    def rowCount(self, index):

        if not self._table:
            return 0

        return self._table.GetNumberOfColumns()

    def columnCount(self, index):
        return 2  # min, max

    def data(self, index, role):

        if not self._table:
            return None

        if role == QtCore.Qt.DisplayRole:
            col = self._table.GetColumn(index.row())
            range = col.GetRange()
            return range[index.column()]

        return None

    def headerData(self, section, orientation, role):

        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return ["Min", "Max"][section]

        if orientation == QtCore.Qt.Vertical and role == QtCore.Qt.DisplayRole:
            return self._table.GetColumnName(section)

        return None

    def getTable(self):
        return self._table


class VtkTableView(QtGui.QWidget):

    def __init__(self, model):
        super().__init__()

        self.model = model

        layout = QtGui.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # start with the toolbar
        self.toolbar = QtGui.QToolBar()
        csv_action = QtGui.QAction(self)
        csv_action.triggered.connect(self.exportCsv)
        csv_action.setIcon(FreeCADGui.getIcon("Std_Export"))
        csv_action.setToolTip(translate("FEM", "Export to CSV"))
        self.toolbar.addAction(csv_action)

        copy_action = QtGui.QAction(self)
        copy_action.triggered.connect(self.copyToClipboard)
        copy_action.setIcon(FreeCADGui.getIcon("edit-copy"))
        shortcut = QtGui.QKeySequence(QtGui.QKeySequence.Copy)
        copy_action.setToolTip(
            translate("FEM", "Copy selection to clipboard ({})".format(shortcut.toString()))
        )
        copy_action.setShortcut(shortcut)
        self.toolbar.addAction(copy_action)

        layout.addWidget(self.toolbar)

        # now the table view
        self.table_view = QtGui.QTableView()
        self.table_view.setModel(model)
        self.model.modelReset.connect(self.modelReset)

        # fast initial resize and manual resizing still allowed!
        header = self.table_view.horizontalHeader()
        header.setResizeContentsPrecision(10)
        self.table_view.resizeColumnsToContents()

        layout.addWidget(self.table_view)
        self.setLayout(layout)

    @QtCore.Slot()
    def modelReset(self):
        # The model is reset, make sure the header visibility is working
        # This is needed in case new data was added
        self.table_view.resizeColumnsToContents()

    @QtCore.Slot(bool)
    def exportCsv(self, state):

        file_path, filter = QtGui.QFileDialog.getSaveFileName(
            None, translate("FEM", "Save as csv file"), "", "CSV (*.csv)"
        )
        if not file_path:
            FreeCAD.Console.PrintMessage(
                translate("FEM", "CSV file export aborted: no filename selected")
            )
            return

        writer = vtkDelimitedTextWriter()
        writer.SetFileName(file_path)
        writer.SetInputData(self.model.getTable())
        writer.Write()

    @QtCore.Slot()
    def copyToClipboard(self):

        sel_model = self.table_view.selectionModel()
        selection = sel_model.selectedIndexes()

        if len(selection) < 1:
            return

        copy_table = ""
        previous = selection.pop(0)
        for current in selection:

            data = self.model.data(previous, QtCore.Qt.DisplayRole)
            copy_table += str(data)

            if current.row() != previous.row():
                copy_table += "\n"
            else:
                copy_table += "\t"

            previous = current

        copy_table += str(self.model.data(selection[-1], QtCore.Qt.DisplayRole))
        copy_table += "\n"

        clipboard = QtGui.QApplication.instance().clipboard()
        clipboard.setText(copy_table)
