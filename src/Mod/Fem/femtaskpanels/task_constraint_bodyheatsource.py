# ***************************************************************************
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM constraint body heat source task panel for the document object"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package task_constraint_bodyheatsource
#  \ingroup FEM
#  \brief task panel for constraint bodyheatsource object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets

from femtools import membertools
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):

    def __init__(self, obj):
        super().__init__(obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/BodyHeatSource.ui"
        )

        self.init_parameter_widget()

        QtCore.QObject.connect(
            self.parameter_widget.qsb_dissipation_rate,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.dissipation_rate_changed,
        )

        QtCore.QObject.connect(
            self.parameter_widget.qsb_total_power,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.total_power_changed,
        )

        QtCore.QObject.connect(
            self.parameter_widget.cb_mode,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.mode_changed,
        )

        # geometry selection widget
        # start with Solid in list!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face"], True, False
        )

        # form made from param and selection widget
        self.form = [self.selection_widget, self.parameter_widget]

        analysis = obj.getParentGroup()
        self._mesh = None
        self._part = None
        if analysis is not None:
            self._mesh = membertools.get_single_member(analysis, "Fem::FemMeshObject")
        if self._mesh is not None:
            self._part = self._mesh.Shape
        self._partVisible = None
        self._meshVisible = None

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self.restore_visibility()
        self.selection_widget.finish_selection()
        return super().reject()

    def accept(self):
        self.obj.References = self.selection_widget.references
        self.obj.DissipationRate = self.dissipation_rate
        self.obj.TotalPower = self.total_power
        self.obj.Mode = self.mode

        self.selection_widget.finish_selection()
        self.restore_visibility()
        return super().accept()

    def restore_visibility(self):
        if self._mesh is not None and self._part is not None:
            if self._meshVisible:
                self._mesh.ViewObject.show()
            else:
                self._mesh.ViewObject.hide()
            if self._partVisible:
                self._part.ViewObject.show()
            else:
                self._part.ViewObject.hide()

    def init_parameter_widget(self):
        self.dissipation_rate = self.obj.DissipationRate
        self.total_power = self.obj.TotalPower
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_dissipation_rate).bind(
            self.obj, "DissipationRate"
        )
        self.parameter_widget.qsb_dissipation_rate.setProperty("value", self.dissipation_rate)

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_total_power).bind(
            self.obj, "TotalPower"
        )
        self.parameter_widget.qsb_total_power.setProperty("value", self.total_power)

        self.mode = self.obj.Mode
        self.mode_enum = self.obj.getEnumerationsOfProperty("Mode")
        self.parameter_widget.cb_mode.addItems(self.mode_enum)
        index = self.mode_enum.index(self.mode)
        self.parameter_widget.cb_mode.setCurrentIndex(index)
        self.mode_changed(index)

    def dissipation_rate_changed(self, base_quantity_value):
        self.dissipation_rate = base_quantity_value

    def total_power_changed(self, base_quantity_value):
        self.total_power = base_quantity_value

    def mode_changed(self, index):
        self.mode = self.mode_enum[index]
        if self.mode == "Dissipation Rate":
            self.parameter_widget.qsb_dissipation_rate.setEnabled(True)
            self.parameter_widget.qsb_total_power.setEnabled(False)
        elif self.mode == "Total Power":
            self.parameter_widget.qsb_dissipation_rate.setEnabled(False)
            self.parameter_widget.qsb_total_power.setEnabled(True)
