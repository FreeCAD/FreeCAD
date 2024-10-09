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
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry1D.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_crosssectiontype,
            QtCore.SIGNAL("activated(int)"),
            self.sectiontype_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_rec_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_height_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_rec_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_width_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_circ_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.circ_diameter_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_pipe_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_diameter_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_pipe_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_thickness_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_elliptical_axis1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.elliptical_axis1_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_elliptical_axis2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.elliptical_axis2_changed,
        )

        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_height_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_width_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_t1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t1_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_t2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t2_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_t3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t3_changed,
        )
        QtCore.QObject.connect(
            self.parameterWidget.qsb_box_t4,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.box_t4_changed,
        )

        self.parameterWidget.cb_crosssectiontype.addItems(
            obj.getEnumerationsOfProperty("SectionType")
        )
        self.get_beamsection_props()
        self.updateParameterWidget()

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Edge"], False, True
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_beamsection_props()
        self.obj.References = self.selectionWidget.references
        self.selectionWidget.finish_selection()
        return super().accept()

    def reject(self):
        self.selectionWidget.finish_selection()
        return super().reject()

    def get_beamsection_props(self):
        self.SectionType = self.obj.SectionType
        self.RectHeight = self.obj.RectHeight
        self.RectWidth = self.obj.RectWidth
        self.CircDiameter = self.obj.CircDiameter
        self.PipeDiameter = self.obj.PipeDiameter
        self.PipeThickness = self.obj.PipeThickness
        self.Axis1Length = self.obj.Axis1Length
        self.Axis2Length = self.obj.Axis2Length
        self.BoxHeight = self.obj.BoxHeight
        self.BoxWidth = self.obj.BoxWidth
        self.BoxT1 = self.obj.BoxT1
        self.BoxT2 = self.obj.BoxT2
        self.BoxT3 = self.obj.BoxT3
        self.BoxT4 = self.obj.BoxT4

    def set_beamsection_props(self):
        self.obj.SectionType = self.SectionType
        self.obj.RectHeight = self.RectHeight
        self.obj.RectWidth = self.RectWidth
        self.obj.CircDiameter = self.CircDiameter
        self.obj.PipeDiameter = self.PipeDiameter
        self.obj.PipeThickness = self.PipeThickness
        self.obj.Axis1Length = self.Axis1Length
        self.obj.Axis2Length = self.Axis2Length
        self.obj.BoxHeight = self.BoxHeight
        self.obj.BoxWidth = self.BoxWidth
        self.obj.BoxT1 = self.BoxT1
        self.obj.BoxT2 = self.BoxT2
        self.obj.BoxT3 = self.BoxT3
        self.obj.BoxT4 = self.BoxT4

    def updateParameterWidget(self):
        "fills the widgets"
        index_crosssectiontype = self.parameterWidget.cb_crosssectiontype.findText(self.SectionType)
        self.parameterWidget.cb_crosssectiontype.setCurrentIndex(index_crosssectiontype)
        self.parameterWidget.qsb_rec_height.setProperty("value", self.RectHeight)
        self.parameterWidget.qsb_rec_width.setProperty("value", self.RectWidth)
        self.parameterWidget.qsb_circ_diameter.setProperty("value", self.CircDiameter)
        self.parameterWidget.qsb_pipe_diameter.setProperty("value", self.PipeDiameter)
        self.parameterWidget.qsb_pipe_thickness.setProperty("value", self.PipeThickness)
        self.parameterWidget.qsb_elliptical_axis1.setProperty("value", self.Axis1Length)
        self.parameterWidget.qsb_elliptical_axis2.setProperty("value", self.Axis2Length)

        self.parameterWidget.qsb_box_height.setProperty("value", self.BoxHeight)
        self.parameterWidget.qsb_box_width.setProperty("value", self.BoxWidth)
        self.parameterWidget.qsb_box_t1.setProperty("value", self.BoxT1)
        self.parameterWidget.qsb_box_t2.setProperty("value", self.BoxT2)
        self.parameterWidget.qsb_box_t3.setProperty("value", self.BoxT3)
        self.parameterWidget.qsb_box_t4.setProperty("value", self.BoxT4)

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.parameterWidget.cb_crosssectiontype.setCurrentIndex(index)
        # parameterWidget returns unicode
        self.SectionType = str(self.parameterWidget.cb_crosssectiontype.itemText(index))

    def rec_height_changed(self, base_quantity_value):
        self.RectHeight = base_quantity_value

    def rec_width_changed(self, base_quantity_value):
        self.RectWidth = base_quantity_value

    def circ_diameter_changed(self, base_quantity_value):
        self.CircDiameter = base_quantity_value

    def pipe_diameter_changed(self, base_quantity_value):
        self.PipeDiameter = base_quantity_value

    def pipe_thickness_changed(self, base_quantity_value):
        self.PipeThickness = base_quantity_value

    def elliptical_axis1_changed(self, base_quantity_value):
        self.Axis1Length = base_quantity_value

    def elliptical_axis2_changed(self, base_quantity_value):
        self.Axis2Length = base_quantity_value

    def box_height_changed(self, base_quantity_value):
        self.BoxHeight = base_quantity_value

    def box_width_changed(self, base_quantity_value):
        self.BoxWidth = base_quantity_value

    def box_t1_changed(self, base_quantity_value):
        self.BoxT1 = base_quantity_value

    def box_t2_changed(self, base_quantity_value):
        self.BoxT2 = base_quantity_value

    def box_t3_changed(self, base_quantity_value):
        self.BoxT3 = base_quantity_value

    def box_t4_changed(self, base_quantity_value):
        self.BoxT4 = base_quantity_value
