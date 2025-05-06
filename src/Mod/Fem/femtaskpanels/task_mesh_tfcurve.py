# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM mesh refinement task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_mesh_region
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of FemMeshRegion objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # self.parameter_widget = FreeCADGui.PySideUic.loadUi(
        #     FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshDistance.ui"
        # )
        # self.parameter_widget.setWindowTitle("Distance threshold settings")
        # self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshDistance.svg"))
        # self._init_parameter_widget()

        # geometry selection widget
        # only allow valid distance objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Edge", "Vertex"], True, False
        )
        self.selection_widget.setWindowTitle("Reference geometries")
        self.selection_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshDistance.svg"))

        # form made from param and selection widget
        self.form = [self.selection_widget]

    def _init_parameter_widget(self):

        # check which picture to use
        # Note: Does work only with system theme, not with stylesheets.
        #       It is unknown how to detect the correct color from stylesheets
        ui = self.parameter_widget
        palette = ui.palette()
        if palette.color(QtGui.QPalette.Text).lightness() > palette.color(QtGui.QPalette.Window).lightness():
            pixmap = QtGui.QPixmap(":images/FEM_MeshDistanceThresholdLight.svg")
        else:
            pixmap = QtGui.QPixmap(":images/FEM_MeshDistanceThresholdDark.svg")
        ui.Diagram.setPixmap(pixmap)

        ui.SizeMin.setProperty("value", self.obj.SizeMinimum)
        FreeCADGui.ExpressionBinding(ui.SizeMin).bind(self.obj, "SizeMinimum")
        ui.SizeMin.valueChanged.connect(self.sizeMinChanged)

        ui.SizeMax.setProperty("value", self.obj.SizeMaximum)
        FreeCADGui.ExpressionBinding(ui.SizeMax).bind(self.obj, "SizeMaximum")
        ui.SizeMax.valueChanged.connect(self.sizeMaxChanged)

        ui.DistMin.setProperty("value", self.obj.DistanceMinimum)
        FreeCADGui.ExpressionBinding(ui.DistMin).bind(self.obj, "DistanceMinimum")
        ui.DistMin.valueChanged.connect(self.distMinChanged)


        ui.DistMax.setProperty("value", self.obj.DistanceMaximum)
        FreeCADGui.ExpressionBinding(ui.DistMax).bind(self.obj, "DistanceMaximum")
        ui.DistMax.valueChanged.connect(self.distMaxChanged)

        ui.Sampling.setProperty("value", FreeCAD.Units.Quantity(self.obj.Sampling))
        FreeCADGui.ExpressionBinding(ui.Sampling).bind(self.obj, "Sampling")
        ui.Sampling.valueChanged.connect(self.samplingChanged)

        ui.Linear.setChecked(self.obj.LinearInterpolation)
        ui.Linear.toggled.connect(self.linearChanged)


    def accept(self):
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMaxChanged(self, value):
        self.obj.DistanceMaximum = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMinChanged(self, value):
        self.obj.DistanceMinimum = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMaxChanged(self, value):
        self.obj.SizeMaximum = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMinChanged(self, value):
        self.obj.SizeMinimum = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def samplingChanged(self, value):
        self.obj.Sampling = int(value)

    @QtCore.Slot(bool)
    def linearChanged(self, value):
        self.obj.LinearInterpolation = value


