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
__url__ = "http://www.freecad.org"

## @package task_element_geometry1D
#  \ingroup FEM
#  \brief task panel for element geometry 1D object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from femobjects import element_geometry1D


class _TaskPanel:
    """
    The TaskPanel for editing References property of ElementGeometry1D objects
    """

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry1D.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_crosssectiontype,
            QtCore.SIGNAL("activated(int)"),
            self.sectiontype_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_rec_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_height_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_rec_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_width_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_circ_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.circ_diameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_pipe_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_diameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_pipe_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_thickness_changed
        )

        self.parameterWidget.cb_crosssectiontype.addItems(
            element_geometry1D.ElementGeometry1D.known_beam_types
        )
        self.get_beamsection_props()
        self.updateParameterWidget()

        # geometry selection widget
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Edge"],
            False,
            True
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_beamsection_props()
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

    def get_beamsection_props(self):
        self.SectionType = self.obj.SectionType
        self.RectHeight = self.obj.RectHeight
        self.RectWidth = self.obj.RectWidth
        self.CircDiameter = self.obj.CircDiameter
        self.PipeDiameter = self.obj.PipeDiameter
        self.PipeThickness = self.obj.PipeThickness

    def set_beamsection_props(self):
        self.obj.SectionType = self.SectionType
        self.obj.RectHeight = self.RectHeight
        self.obj.RectWidth = self.RectWidth
        self.obj.CircDiameter = self.CircDiameter
        self.obj.PipeDiameter = self.PipeDiameter
        self.obj.PipeThickness = self.PipeThickness

    def updateParameterWidget(self):
        "fills the widgets"
        index_crosssectiontype = self.parameterWidget.cb_crosssectiontype.findText(
            self.SectionType
        )
        self.parameterWidget.cb_crosssectiontype.setCurrentIndex(index_crosssectiontype)
        self.parameterWidget.if_rec_height.setText(self.RectHeight.UserString)
        self.parameterWidget.if_rec_width.setText(self.RectWidth.UserString)
        self.parameterWidget.if_circ_diameter.setText(self.CircDiameter.UserString)
        self.parameterWidget.if_pipe_diameter.setText(self.PipeDiameter.UserString)
        self.parameterWidget.if_pipe_thickness.setText(self.PipeThickness.UserString)

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
