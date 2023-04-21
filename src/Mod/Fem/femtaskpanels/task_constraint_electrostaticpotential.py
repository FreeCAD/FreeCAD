# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM constraint electrostatic potential task panel for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package task_constraint_electrostaticpotential
#  \ingroup FEM
#  \brief task panel for constraint electrostatic potential object

from PySide import QtCore

import FreeCAD
import FreeCADGui
from femguiutils import selection_widgets

from femtools import femutils
from femtools import membertools


class _TaskPanel(object):

    def __init__(self, obj):
        self._obj = obj

        self._paramWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElectrostaticPotential.ui")
        self._initParamWidget()

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Solid", "Face", "Edge", "Vertex"],
            True,
            False
        )

        # form made from param and selection widget
        self.form = [self._paramWidget, self._selectionWidget]

        analysis = obj.getParentGroup()
        self._mesh = None
        self._part = None
        if analysis is not None:
            self._mesh = membertools.get_single_member(analysis, "Fem::FemMeshObject")
        if self._mesh is not None:
            self._part = femutils.get_part_to_mesh(self._mesh)
        self._partVisible = None
        self._meshVisible = None

        # start with vector inputs hidden if no vector is set
        if (
            self._obj.AV_re_1_Disabled
            and self._obj.AV_re_2_Disabled
            and self._obj.AV_re_3_Disabled
            and self._obj.AV_im_Disabled
            and self._obj.AV_im_1_Disabled
            and self._obj.AV_im_2_Disabled
            and self._obj.AV_im_3_Disabled
        ):
            self._vectorField_visibility(False)
            self._paramWidget.vectorFieldBox.setChecked(False)
        QtCore.QObject.connect(
            self._paramWidget.vectorFieldBox,
            QtCore.SIGNAL("toggled(bool)"),
            self._vectorField_visibility
        )

    def _vectorField_visibility(self, visible):
        self._paramWidget.vectorFieldGB.setVisible(visible)

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self._restoreVisibility()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def accept(self):
        if self._obj.References != self._selectionWidget.references:
            self._obj.References = self._selectionWidget.references
        self._applyWidgetChanges()
        self._obj.Document.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        self._restoreVisibility()
        return True

    def _restoreVisibility(self):
        if self._mesh is not None and self._part is not None:
            if self._meshVisible:
                self._mesh.ViewObject.show()
            else:
                self._mesh.ViewObject.hide()
            if self._partVisible:
                self._part.ViewObject.show()
            else:
                self._part.ViewObject.hide()

    def _initParamWidget(self):
        self._paramWidget.potentialQSB.setProperty(
            'value', self._obj.Potential)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.potentialQSB).bind(self._obj, "Potential")
        self._paramWidget.potentialBox.setChecked(
            not self._obj.PotentialEnabled)

        # the vector potentials
        # realScalarQSB always the same value as potentialQSB
        self._paramWidget.realScalarQSB.setProperty(
            'value', self._obj.Potential)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.realScalarQSB).bind(self._obj, "Potential")
        self._paramWidget.realXQSB.setProperty(
            'value', self._obj.AV_re_1)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.realXQSB).bind(self._obj, "AV_re_1")
        self._paramWidget.realYQSB.setProperty(
            'value', self._obj.AV_re_2)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.realYQSB).bind(self._obj, "AV_re_2")
        self._paramWidget.realZQSB.setProperty(
            'value', self._obj.AV_re_3)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.realZQSB).bind(self._obj, "AV_re_3")
        self._paramWidget.imagScalarQSB.setProperty(
            'value', self._obj.AV_im)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.imagScalarQSB).bind(self._obj, "AV_im")
        self._paramWidget.imagXQSB.setProperty(
            'value', self._obj.AV_im_1)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.imagXQSB).bind(self._obj, "AV_im_1")
        self._paramWidget.imagYQSB.setProperty(
            'value', self._obj.AV_im_2)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.imagYQSB).bind(self._obj, "AV_im_2")
        self._paramWidget.imagZQSB.setProperty(
            'value', self._obj.AV_im_3)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.imagZQSB).bind(self._obj, "AV_im_3")

        self._paramWidget.reXunspecBox.setChecked(
            self._obj.AV_re_1_Disabled)
        self._paramWidget.reYunspecBox.setChecked(
            self._obj.AV_re_2_Disabled)
        self._paramWidget.reZunspecBox.setChecked(
            self._obj.AV_re_3_Disabled)
        self._paramWidget.imScalarunspecBox.setChecked(
            self._obj.AV_im_Disabled)
        self._paramWidget.imXunspecBox.setChecked(
            self._obj.AV_im_1_Disabled)
        self._paramWidget.imYunspecBox.setChecked(
            self._obj.AV_im_2_Disabled)
        self._paramWidget.imZunspecBox.setChecked(
            self._obj.AV_im_3_Disabled)

        self._paramWidget.potentialConstantBox.setChecked(
            self._obj.PotentialConstant)

        self._paramWidget.electricInfinityBox.setChecked(
            self._obj.ElectricInfinity)

        self._paramWidget.electricForcecalculationBox.setChecked(
            self._obj.ElectricForcecalculation)

        self._paramWidget.capacitanceBodyBox.setChecked(
            not self._obj.CapacitanceBodyEnabled)
        self._paramWidget.capacitanceBody_spinBox.setValue(
            self._obj.CapacitanceBody)
        self._paramWidget.capacitanceBody_spinBox.setEnabled(
            not self._paramWidget.capacitanceBodyBox.isChecked())

    def _applyPotentialChanges(self, enabledBox, potentialQSB):
        enabled = enabledBox.isChecked()
        potential = None
        try:
            potential = potentialQSB.property('value')
        except ValueError:
            FreeCAD.Console.PrintMessage(
                "Wrong input. Not recognised input: '{}' "
                "Potential has not been set.\n".format(potentialQSB.text())
            )
            potential = '0.0 mm^2*kg/(s^3*A)'
        return enabled, potential

    def _applyWidgetChanges(self):
        # apply the voltages and their enabled state
        self._obj.PotentialEnabled, self._obj.Potential = \
            self._applyPotentialChanges(
                self._paramWidget.potentialBox,
                self._paramWidget.potentialQSB
            )
        self._obj.AV_re_1_Disabled, self._obj.AV_re_1 = \
            self._applyPotentialChanges(
                self._paramWidget.reXunspecBox,
                self._paramWidget.realXQSB
            )
        self._obj.AV_re_2_Disabled, self._obj.AV_re_2 = \
            self._applyPotentialChanges(
                self._paramWidget.reYunspecBox,
                self._paramWidget.realYQSB
            )
        self._obj.AV_re_3_Disabled, self._obj.AV_re_3 = \
            self._applyPotentialChanges(
                self._paramWidget.reZunspecBox,
                self._paramWidget.realZQSB
            )
        self._obj.AV_im_Disabled, self._obj.AV_im = \
            self._applyPotentialChanges(
                self._paramWidget.imScalarunspecBox,
                self._paramWidget.imagScalarQSB
            )
        self._obj.AV_im_1_Disabled, self._obj.AV_im_1 = \
            self._applyPotentialChanges(
                self._paramWidget.imXunspecBox,
                self._paramWidget.imagXQSB
            )
        self._obj.AV_im_2_Disabled, self._obj.AV_im_2 = \
            self._applyPotentialChanges(
                self._paramWidget.imYunspecBox,
                self._paramWidget.imagYQSB
            )
        self._obj.AV_im_3_Disabled, self._obj.AV_im_3 = \
            self._applyPotentialChanges(
                self._paramWidget.imZunspecBox,
                self._paramWidget.imagZQSB
            )
        # because this is an enable the others are disabled, reverse
        self._obj.PotentialEnabled = not self._obj.PotentialEnabled

        self._obj.PotentialConstant = self._paramWidget.potentialConstantBox.isChecked()

        self._obj.ElectricInfinity = self._paramWidget.electricInfinityBox.isChecked()

        calc_is_checked = self._paramWidget.electricForcecalculationBox.isChecked()
        self._obj.ElectricForcecalculation = calc_is_checked  # two lines because max line length

        self._obj.CapacitanceBodyEnabled = \
            not self._paramWidget.capacitanceBodyBox.isChecked()
        if self._obj.CapacitanceBodyEnabled:
            self._paramWidget.capacitanceBody_spinBox.setEnabled(True)
            self._obj.CapacitanceBody = self._paramWidget.capacitanceBody_spinBox.value()
