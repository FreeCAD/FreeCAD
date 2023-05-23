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


class _TaskPanel:
    """
    The TaskPanel for editing References property of MeshBoundaryLayer objects
    """

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshBoundaryLayer.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.bl_number_of_layers,
            QtCore.SIGNAL("valueChanged(int)"),
            self.bl_number_of_layers_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.bl_min_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.bl_min_thickness_changed
        )
        # be careful of signal signature for QDoubleSpinbox
        QtCore.QObject.connect(
            self.parameterWidget.bl_growth_rate,
            QtCore.SIGNAL("valueChanged(double)"),
            self.bl_growth_rate_changed
        )
        self.init_parameter_widget()

        # geometry selection widget
        # start with Solid in list!
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Solid", "Face", "Edge", "Vertex"],
            True,
            False
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_mesh_boundarylayer_props()
        self.obj.References = self.selectionWidget.references
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        doc.resetEdit()

    def init_parameter_widget(self):
        self.bl_min_thickness = self.obj.MinimumThickness
        self.bl_number_of_layers = self.obj.NumberOfLayers
        self.bl_growth_rate = self.obj.GrowthRate
        self.parameterWidget.bl_min_thickness.setText(self.bl_min_thickness.UserString)
        self.parameterWidget.bl_number_of_layers.setValue(self.bl_number_of_layers)
        self.parameterWidget.bl_growth_rate.setValue(self.bl_growth_rate)

    def set_mesh_boundarylayer_props(self):
        self.obj.MinimumThickness = self.bl_min_thickness
        self.obj.NumberOfLayers = self.bl_number_of_layers
        self.obj.GrowthRate = self.bl_growth_rate

    def bl_min_thickness_changed(self, base_quantity_value):
        self.bl_min_thickness = base_quantity_value

    def bl_number_of_layers_changed(self, value):
        self.bl_number_of_layers = value

    def bl_growth_rate_changed(self, value):
        self.bl_growth_rate = value
