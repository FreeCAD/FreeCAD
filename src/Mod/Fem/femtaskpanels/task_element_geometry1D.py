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
            self.parameter_widget.cb_cross_section_type,
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
        QtCore.QObject.connect(
            self.parameter_widget.qsb_elliptical_axis1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.elliptical_axis1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_elliptical_axis2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.elliptical_axis2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_height_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_width_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_t1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_t2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_t3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_box_t4,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t4_changed,
        )

        self.parameter_widget.cb_cross_section_type.addItems(
            obj.getEnumerationsOfProperty("SectionType")
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
        self.axis1_length = self.obj.Axis1Length
        self.axis2_length = self.obj.Axis2Length
        self.box_height = self.obj.BoxHeight
        self.box_width = self.obj.BoxWidth
        self.box_t1 = self.obj.BoxT1
        self.box_t2 = self.obj.BoxT2
        self.box_t3 = self.obj.BoxT3
        self.box_t4 = self.obj.BoxT4

    def set_beamsection_props(self):
        self.obj.SectionType = self.section_type
        self.obj.RectHeight = self.rect_height
        self.obj.RectWidth = self.rect_width
        self.obj.CircDiameter = self.circ_diameter
        self.obj.PipeDiameter = self.pipe_diameter
        self.obj.PipeThickness = self.pipe_thickness
        self.obj.Axis1Length = self.axis1_length
        self.obj.Axis2Length = self.axis2_length
        self.obj.BoxHeight = self.box_height
        self.obj.BoxWidth = self.box_width
        self.obj.BoxT1 = self.box_t1
        self.obj.BoxT2 = self.box_t2
        self.obj.BoxT3 = self.box_t3
        self.obj.BoxT4 = self.box_t4

    def update_parameter_widget(self):
        "fills the widgets"
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

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_elliptical_axis1).bind(
            self.obj, "Axis1Length"
        )
        self.parameter_widget.qsb_elliptical_axis1.setProperty("value", self.axis1_length)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_elliptical_axis2).bind(
            self.obj, "Axis2Length"
        )
        self.parameter_widget.qsb_elliptical_axis2.setProperty("value", self.axis2_length)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_height).bind(
            self.obj, "BoxHeight"
        )
        self.parameter_widget.qsb_box_height.setProperty("value", self.box_height)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_width).bind(self.obj, "BoxWidth")
        self.parameter_widget.qsb_box_width.setProperty("value", self.box_width)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_t1).bind(self.obj, "BoxT1")
        self.parameter_widget.qsb_box_t1.setProperty("value", self.box_t1)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_t2).bind(self.obj, "BoxT2")
        self.parameter_widget.qsb_box_t2.setProperty("value", self.box_t2)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_t3).bind(self.obj, "BoxT3")
        self.parameter_widget.qsb_box_t3.setProperty("value", self.box_t3)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_box_t4).bind(self.obj, "BoxT4")
        self.parameter_widget.qsb_box_t4.setProperty("value", self.box_t4)

        index_cross_section_type = self.parameter_widget.cb_cross_section_type.findText(
            self.section_type
        )
        self.parameter_widget.cb_cross_section_type.setCurrentIndex(index_cross_section_type)

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.parameter_widget.cb_cross_section_type.setCurrentIndex(index)
        self.section_type = self.parameter_widget.cb_cross_section_type.itemText(index)

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

    def elliptical_axis1_changed(self, base_quantity_value):
        self.axis1_length = base_quantity_value

    def elliptical_axis2_changed(self, base_quantity_value):
        self.axis2_length = base_quantity_value

    def box_height_changed(self, base_quantity_value):
        self.box_height = base_quantity_value

    def box_width_changed(self, base_quantity_value):
        self.box_width = base_quantity_value

    def box_t1_changed(self, base_quantity_value):
        self.box_t1 = base_quantity_value

    def box_t2_changed(self, base_quantity_value):
        self.box_t2 = base_quantity_value

    def box_t3_changed(self, base_quantity_value):
        self.box_t3 = base_quantity_value

    def box_t4_changed(self, base_quantity_value):
        self.box_t4 = base_quantity_value
