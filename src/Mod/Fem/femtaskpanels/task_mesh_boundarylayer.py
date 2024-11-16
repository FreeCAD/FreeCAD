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

__title__ = "FreeCAD FEM mesh boundary layer task panel for the document object"
__author__ = "Bernd Hahnebach, Qingfeng Xia"
__url__ = "https://www.freecad.org"

## @package task_mesh_boundarylayer
#  \ingroup FEM
#  \brief task panel for mesh boundary object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of MeshBoundaryLayer objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # parameter widget
        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshBoundaryLayer.ui"
        )
        QtCore.QObject.connect(
            self.parameter_widget.isb_number_of_layers,
            QtCore.SIGNAL("valueChanged(int)"),
            self.number_of_layers_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_min_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.min_thickness_changed,
        )
        # be careful of signal signature for QDoubleSpinbox
        QtCore.QObject.connect(
            self.parameter_widget.dsb_growth_rate,
            QtCore.SIGNAL("valueChanged(double)"),
            self.growth_rate_changed,
        )
        self.init_parameter_widget()

        # geometry selection widget
        # start with Solid in list!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge", "Vertex"], True, False
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def accept(self):
        self.set_mesh_boundarylayer_props()
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    def init_parameter_widget(self):
        self.min_thickness = self.obj.MinimumThickness
        self.number_of_layers = self.obj.NumberOfLayers
        self.growth_rate = self.obj.GrowthRate
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_min_thickness).bind(
            self.obj, "MinimumThickness"
        )
        self.parameter_widget.qsb_min_thickness.setProperty("value", self.min_thickness)

        FreeCADGui.ExpressionBinding(self.parameter_widget.dsb_growth_rate).bind(
            self.obj, "GrowthRate"
        )
        self.parameter_widget.dsb_growth_rate.setProperty("value", self.growth_rate)

        FreeCADGui.ExpressionBinding(self.parameter_widget.isb_number_of_layers).bind(
            self.obj, "NumberOfLayers"
        )
        self.parameter_widget.isb_number_of_layers.setProperty("value", self.number_of_layers)

    def set_mesh_boundarylayer_props(self):
        self.obj.MinimumThickness = self.min_thickness
        self.obj.NumberOfLayers = self.number_of_layers
        self.obj.GrowthRate = self.growth_rate

    def min_thickness_changed(self, base_quantity_value):
        self.min_thickness = base_quantity_value

    def number_of_layers_changed(self, value):
        self.number_of_layers = value

    def growth_rate_changed(self, value):
        self.growth_rate = value
