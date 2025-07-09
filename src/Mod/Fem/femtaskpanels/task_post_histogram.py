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

__title__ = "FreeCAD FEM histogram plot task panel"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_post_histogram
#  \ingroup FEM
#  \brief task panel for post histogram plot

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from . import base_fempostpanel
from femguiutils import extract_link_view as elv
from femguiutils import vtk_table_view

translate = FreeCAD.Qt.translate


class _TaskPanel(base_fempostpanel._BasePostTaskPanel):
    """
    The TaskPanel for editing properties of glyph filter
    """

    def __init__(self, vobj):
        super().__init__(vobj.Object)

        # data widget
        self.data_widget = QtGui.QWidget()
        hbox = QtGui.QHBoxLayout()
        self.data_widget.show_plot = QtGui.QPushButton()
        self.data_widget.show_plot.setText(translate("FEM", "Show Plot"))
        hbox.addWidget(self.data_widget.show_plot)
        self.data_widget.show_table = QtGui.QPushButton()
        self.data_widget.show_table.setText(translate("FEM", "Show Data"))
        hbox.addWidget(self.data_widget.show_table)
        vbox = QtGui.QVBoxLayout()
        vbox.addItem(hbox)
        vbox.addSpacing(10)

        extracts = elv.ExtractLinkView(self.obj, False, self)
        vbox.addWidget(extracts)

        self.data_widget.setLayout(vbox)
        self.data_widget.setWindowTitle(translate("FEM", "Histogram Data"))
        self.data_widget.setWindowIcon(FreeCADGui.getIcon(":/icons/FEM_PostHistogram.svg"))

        # histogram parameter widget
        self.view_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/TaskPostHistogram.ui"
        )
        self.view_widget.setWindowTitle(translate("FEM", "Histogram View Settings"))
        self.view_widget.setWindowIcon(FreeCADGui.getIcon(":/icons/FEM_PostHistogram.svg"))

        self.__init_widgets()

        # form made from param and selection widget
        self.form = [self.data_widget, self.view_widget]

    # Setup functions
    # ###############

    def __init_widgets(self):

        # connect data widget
        self.data_widget.show_plot.clicked.connect(self.showPlot)
        self.data_widget.show_table.clicked.connect(self.showTable)

        # set current values to view widget
        viewObj = self.obj.ViewObject

        self.view_widget.Bins.setValue(viewObj.Bins)
        self._enumPropertyToCombobox(viewObj, "Type", self.view_widget.Type)
        self.view_widget.Cumulative.setChecked(viewObj.Cumulative)

        self.view_widget.Title.setText(viewObj.Title)
        self.view_widget.XLabel.setText(viewObj.XLabel)
        self.view_widget.YLabel.setText(viewObj.YLabel)

        self.view_widget.LegendShow.setChecked(viewObj.Legend)
        self._enumPropertyToCombobox(viewObj, "LegendLocation", self.view_widget.LegendPos)
        self.view_widget.BarWidth.setValue(viewObj.BarWidth)
        self.view_widget.HatchWidth.setValue(viewObj.HatchLineWidth)

        # connect callbacks
        self.view_widget.Bins.valueChanged.connect(self.binsChanged)
        self.view_widget.Type.activated.connect(self.typeChanged)
        self.view_widget.Cumulative.toggled.connect(self.comulativeChanged)

        self.view_widget.Title.editingFinished.connect(self.titleChanged)
        self.view_widget.XLabel.editingFinished.connect(self.xLabelChanged)
        self.view_widget.YLabel.editingFinished.connect(self.yLabelChanged)

        self.view_widget.LegendShow.toggled.connect(self.legendShowChanged)
        self.view_widget.LegendPos.activated.connect(self.legendPosChanged)
        self.view_widget.BarWidth.valueChanged.connect(self.barWidthChanged)
        self.view_widget.HatchWidth.valueChanged.connect(self.hatchWidthChanged)

    QtCore.Slot()

    def showPlot(self):
        self.obj.ViewObject.Proxy.show_visualization()

    QtCore.Slot()

    def showTable(self):

        # TODO: make data model update when object is recomputed
        data_model = vtk_table_view.VtkTableModel()
        data_model.setTable(self.obj.Table)

        dialog = QtGui.QDialog(self.data_widget)
        widget = vtk_table_view.VtkTableView(data_model)
        layout = QtGui.QVBoxLayout()
        layout.addWidget(widget)
        layout.setContentsMargins(0, 0, 0, 0)

        dialog.setLayout(layout)
        dialog.resize(1500, 900)
        dialog.show()

    QtCore.Slot(int)

    def binsChanged(self, bins):
        self.obj.ViewObject.Bins = bins

    QtCore.Slot(int)

    def typeChanged(self, idx):
        self.obj.ViewObject.Type = idx

    QtCore.Slot(bool)

    def comulativeChanged(self, state):
        self.obj.ViewObject.Cumulative = state

    QtCore.Slot()

    def titleChanged(self):
        self.obj.ViewObject.Title = self.view_widget.Title.text()

    QtCore.Slot()

    def xLabelChanged(self):
        self.obj.ViewObject.XLabel = self.view_widget.XLabel.text()

    QtCore.Slot()

    def yLabelChanged(self):
        self.obj.ViewObject.YLabel = self.view_widget.YLabel.text()

    QtCore.Slot(int)

    def legendPosChanged(self, idx):
        self.obj.ViewObject.LegendLocation = idx

    QtCore.Slot(bool)

    def legendShowChanged(self, state):
        self.obj.ViewObject.Legend = state

    QtCore.Slot(float)

    def barWidthChanged(self, value):
        self.obj.ViewObject.BarWidth = value

    QtCore.Slot(float)

    def hatchWidthChanged(self, value):
        self.obj.ViewObject.HatchLineWidth = value
