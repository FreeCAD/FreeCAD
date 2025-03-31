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

class VtkTableModel(QtCore.QAbstractTableModel):
    # Simple table model. Only supports single component columns

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

    def headerData(self, section, orientation, role):

        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self._table.GetColumnName(section)

        if orientation == QtCore.Qt.Vertical and role == QtCore.Qt.DisplayRole:
            return section

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
        return 2 # min, max

    def data(self, index, role):

        if not self._table:
            return None

        if role == QtCore.Qt.DisplayRole:
            col = self._table.GetColumn(index.row())
            range = col.GetRange()
            return range[index.column()]

    def headerData(self, section, orientation, role):

        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return ["Min","Max"][section]

        if orientation == QtCore.Qt.Vertical and role == QtCore.Qt.DisplayRole:
            return self._table.GetColumnName(section)


class VtkTableView(QtGui.QWidget):

    def __init__(self, model):
        super().__init__()

        self.model = model
        self.table_view = QtGui.QTableView()
        self.table_view.setModel(model)

        # fast initial resize and manual resizing still allowed!
        header = self.table_view.horizontalHeader()
        header.setResizeContentsPrecision(10)
        self.table_view.resizeColumnsToContents()

        layout = QtGui.QVBoxLayout()
        layout.setContentsMargins(0,0,0,0)
        layout.addWidget(self.table_view)
        self.setLayout(layout)

