# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element geometry 1D task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_element_geometry1D
#  \ingroup FEM
#  \brief task panel for element geometry 1D object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from femobjects import element_geometry1D
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of ElementGeometry1D objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # parameter widget
        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry1D.ui"
        )
        QtCore.QObject.connect(
            self.parameter_widget.cb_crosssectiontype,
            QtCore.SIGNAL("activated(int)"),
            self.sectiontype_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_rec_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_height_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_rec_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_width_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_circ_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.circ_diameter_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_pipe_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_diameter_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_pipe_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_thickness_changed,
        )

        self.parameter_widget.cb_crosssectiontype.addItems(
            element_geometry1D.ElementGeometry1D.known_beam_types
        )
        self.get_beamsection_props()
        self.update_parameter_widget()

        # geometry selection widget
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Edge"], False, True
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def accept(self):
        self.set_beamsection_props()
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    def get_beamsection_props(self):
        self.section_type = self.obj.SectionType
        self.rect_height = self.obj.RectHeight
        self.rect_width = self.obj.RectWidth
        self.circ_diameter = self.obj.CircDiameter
        self.pipe_diameter = self.obj.PipeDiameter
        self.pipe_thickness = self.obj.PipeThickness

    def set_beamsection_props(self):
        self.obj.SectionType = self.section_type
        self.obj.RectHeight = self.rect_height
        self.obj.RectWidth = self.rect_width
        self.obj.CircDiameter = self.circ_diameter
        self.obj.PipeDiameter = self.pipe_diameter
        self.obj.PipeThickness = self.pipe_thickness

    def update_parameter_widget(self):
        "fills the widgets"
        self.rect_height = self.obj.RectHeight
        self.rect_width = self.obj.RectWidth
        self.circ_diameter = self.obj.CircDiameter
        self.pipe_diameter = self.obj.PipeDiameter
        self.pipe_thickness = self.obj.PipeThickness

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_rec_height).bind(
            self.obj, "RectHeight"
        )
        self.parameter_widget.qsb_rec_height.setProperty("value", self.rect_height)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_rec_width).bind(
            self.obj, "RectWidth"
        )
        self.parameter_widget.qsb_rec_width.setProperty("value", self.rect_width)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_circ_diameter).bind(
            self.obj, "CircDiameter"
        )
        self.parameter_widget.qsb_circ_diameter.setProperty("value", self.circ_diameter)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_pipe_diameter).bind(
            self.obj, "PipeDiameter"
        )
        self.parameter_widget.qsb_pipe_diameter.setProperty("value", self.pipe_diameter)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_pipe_thickness).bind(
            self.obj, "PipeThickness"
        )
        self.parameter_widget.qsb_pipe_thickness.setProperty("value", self.pipe_thickness)

        index_crosssectiontype = self.parameter_widget.cb_crosssectiontype.findText(
            self.section_type
        )
        self.parameter_widget.cb_crosssectiontype.setCurrentIndex(index_crosssectiontype)

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.parameter_widget.cb_crosssectiontype.setCurrentIndex(index)
        self.section_type = self.parameter_widget.cb_crosssectiontype.itemText(index)

    def rec_height_changed(self, base_quantity_value):
        self.rect_height = base_quantity_value

    def rec_width_changed(self, base_quantity_value):
        self.rect_width = base_quantity_value

    def circ_diameter_changed(self, base_quantity_value):
        self.circ_diameter = base_quantity_value

    def pipe_diameter_changed(self, base_quantity_value):
        self.pipe_diameter = base_quantity_value

    def pipe_thickness_changed(self, base_quantity_value):
        self.pipe_thickness = base_quantity_value
