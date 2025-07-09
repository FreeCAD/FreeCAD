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

__title__ = "FreeCAD FEM postprocessing ldata view and extraction widget"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package data_extraction
#  \ingroup FEM
#  \brief A widget for data extraction. Used in the PostObject task panel.

from . import vtk_table_view

from PySide import QtCore, QtGui

from vtkmodules.vtkCommonCore import vtkVersion
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkFiltersGeneral import vtkSplitColumnComponents

if vtkVersion.GetVTKMajorVersion() > 9 and vtkVersion.GetVTKMinorVersion() > 3:
    from vtkmodules.vtkFiltersCore import vtkAttributeDataToTableFilter
else:
    from vtkmodules.vtkInfovisCore import vtkDataObjectToTable


import FreeCAD
import FreeCADGui

import femobjects.base_fempostextractors as extr
from femtaskpanels.base_fempostpanel import _BasePostTaskPanel

from . import extract_link_view

ExtractLinkView = extract_link_view.ExtractLinkView


class DataExtraction(_BasePostTaskPanel):
    # The class is not a widget itself, but provides a widget. It implements
    # all required callbacks for the widget and the task dialog.
    # Note: This object is created and used from c++! See PostTaskExtraction

    def __init__(self, vobj):

        super().__init__(vobj.Object)

        self.ViewObject = vobj
        self.Object = vobj.Object

        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/TaskPostExtraction.ui"
        )

        # connect all signals as required
        self.widget.Data.clicked.connect(self.showData)
        self.widget.Summary.clicked.connect(self.showSummary)

        # setup the data models
        self.data_model = vtk_table_view.VtkTableModel()
        self.summary_model = vtk_table_view.VtkTableSummaryModel()

        # generate the data
        self.onPostDataChanged(self.Object)

        # setup the extraction widget
        self._extraction_view = ExtractLinkView(self.Object, True, self)
        self.widget.layout().addSpacing(self.widget.Data.size().height() / 3)
        self.widget.layout().addWidget(self._extraction_view)
        self._extraction_view.repopulate()

    @QtCore.Slot()
    def showData(self):

        dialog = QtGui.QDialog(self.widget)
        dialog.setWindowTitle(f"Data of {self.Object.Label}")
        widget = vtk_table_view.VtkTableView(self.data_model)
        layout = QtGui.QVBoxLayout()
        layout.addWidget(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        dialog.setLayout(layout)
        dialog.resize(1500, 900)
        dialog.show()

    @QtCore.Slot()
    def showSummary(self):

        dialog = QtGui.QDialog(self.widget)
        dialog.setWindowTitle(f"Data Summary of {self.Object.Label}")
        widget = vtk_table_view.VtkTableView(self.summary_model)
        layout = QtGui.QVBoxLayout()
        layout.addWidget(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        dialog.setLayout(layout)
        dialog.resize(600, 900)
        dialog.show()

    def isGuiTaskOnly(self):
        # If all panels return true it omits the Apply button in the dialog
        return True

    def onPostDataChanged(self, obj):

        algo = obj.getOutputAlgorithm()
        if not algo:
            self.data_model.setTable(vtkTable())

        if vtkVersion.GetVTKMajorVersion() > 9 and vtkVersion.GetVTKMinorVersion() > 3:
            filter = vtkAttributeDataToTableFilter()
        else:
            filter = vtkDataObjectToTable()
            filter.SetFieldType(vtkDataObjectToTable.POINT_DATA)

        filter.SetInputConnection(0, algo.GetOutputPort(0))
        filter.Update()
        table = filter.GetOutputDataObject(0)

        # add the points
        points = algo.GetOutputDataObject(0).GetPoints().GetData()
        table.AddColumn(points)

        # split the components
        splitter = vtkSplitColumnComponents()
        splitter.SetNamingModeToNamesWithParens()
        splitter.SetInputData(0, table)

        splitter.Update()
        table = splitter.GetOutputDataObject(0)

        self.data_model.setTable(table)
        self.summary_model.setTable(table)

    def apply(self):
        pass

    def initiallyCollapsed(self):
        # if we do not have any extractions to show we hide initially to remove clutter

        for obj in self.Object.InList:
            if extr.is_extractor_object(obj):
                return False

        return True
