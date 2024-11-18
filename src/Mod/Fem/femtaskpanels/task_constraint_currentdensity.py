# ***************************************************************************
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

__title__ = "FreeCAD FEM constraint current density task panel for the document object"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package task_constraint_currentdensity
#  \ingroup FEM
#  \brief task panel for constraint current density object

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets

from femtools import membertools
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):

    def __init__(self, obj):
        super().__init__(obj)

        self._paramWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/CurrentDensity.ui"
        )
        self._initParamWidget()

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face"], True, False
        )

        # form made from param and selection widget
        self.form = [self._paramWidget, self._selectionWidget]

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
        self._restoreVisibility()
        self._selectionWidget.finish_selection()
        return super().reject()

    def accept(self):
        if self.obj.References != self._selectionWidget.references:
            self.obj.References = self._selectionWidget.references
        self._applyWidgetChanges()
        self._selectionWidget.finish_selection()
        self._restoreVisibility()
        return super().accept()

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
        self._paramWidget.realXQSB.setProperty("value", self.obj.CurrentDensity_re_1)
        FreeCADGui.ExpressionBinding(self._paramWidget.realXQSB).bind(
            self.obj, "CurrentDensity_re_1"
        )
        self._paramWidget.realYQSB.setProperty("value", self.obj.CurrentDensity_re_2)
        FreeCADGui.ExpressionBinding(self._paramWidget.realYQSB).bind(
            self.obj, "CurrentDensity_re_2"
        )
        self._paramWidget.realZQSB.setProperty("value", self.obj.CurrentDensity_re_3)
        FreeCADGui.ExpressionBinding(self._paramWidget.realZQSB).bind(
            self.obj, "CurrentDensity_re_3"
        )
        self._paramWidget.imagXQSB.setProperty("value", self.obj.CurrentDensity_im_1)
        FreeCADGui.ExpressionBinding(self._paramWidget.imagXQSB).bind(
            self.obj, "CurrentDensity_im_1"
        )
        self._paramWidget.imagYQSB.setProperty("value", self.obj.CurrentDensity_im_2)
        FreeCADGui.ExpressionBinding(self._paramWidget.imagYQSB).bind(
            self.obj, "CurrentDensity_im_2"
        )
        self._paramWidget.imagZQSB.setProperty("value", self.obj.CurrentDensity_im_3)
        FreeCADGui.ExpressionBinding(self._paramWidget.imagZQSB).bind(
            self.obj, "CurrentDensity_im_3"
        )

        self._paramWidget.reXunspecBox.setChecked(self.obj.CurrentDensity_re_1_Disabled)
        self._paramWidget.reYunspecBox.setChecked(self.obj.CurrentDensity_re_2_Disabled)
        self._paramWidget.reZunspecBox.setChecked(self.obj.CurrentDensity_re_3_Disabled)
        self._paramWidget.imXunspecBox.setChecked(self.obj.CurrentDensity_im_1_Disabled)
        self._paramWidget.imYunspecBox.setChecked(self.obj.CurrentDensity_im_2_Disabled)
        self._paramWidget.imZunspecBox.setChecked(self.obj.CurrentDensity_im_3_Disabled)

    def _applyCurrentDensityChanges(self, enabledBox, currentDensityQSB):
        enabled = enabledBox.isChecked()
        currentdensity = None
        try:
            currentdensity = currentDensityQSB.property("value")
        except ValueError:
            FreeCAD.Console.PrintMessage(
                "Wrong input. Not recognised input: '{}' "
                "Current density has not been set.\n".format(currentDensityQSB.text())
            )
            currentdensity = "0.0 A/m^2"
        return enabled, currentdensity

    def _applyWidgetChanges(self):
        # apply the current densities and their enabled state
        self.obj.CurrentDensity_re_1_Disabled, self.obj.CurrentDensity_re_1 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.reXunspecBox, self._paramWidget.realXQSB
            )
        )
        self.obj.CurrentDensity_re_2_Disabled, self.obj.CurrentDensity_re_2 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.reYunspecBox, self._paramWidget.realYQSB
            )
        )
        self.obj.CurrentDensity_re_3_Disabled, self.obj.CurrentDensity_re_3 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.reZunspecBox, self._paramWidget.realZQSB
            )
        )
        self.obj.CurrentDensity_im_1_Disabled, self.obj.CurrentDensity_im_1 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.imXunspecBox, self._paramWidget.imagXQSB
            )
        )
        self.obj.CurrentDensity_im_2_Disabled, self.obj.CurrentDensity_im_2 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.imYunspecBox, self._paramWidget.imagYQSB
            )
        )
        self.obj.CurrentDensity_im_3_Disabled, self.obj.CurrentDensity_im_3 = (
            self._applyCurrentDensityChanges(
                self._paramWidget.imZunspecBox, self._paramWidget.imagZQSB
            )
        )
