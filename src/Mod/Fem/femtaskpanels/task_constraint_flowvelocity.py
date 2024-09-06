# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM constraint flow velocity task panel for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package task_constraint_flowvelocity
#  \ingroup FEM
#  \brief task panel for constraint flow velocity object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from femtools import membertools
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):

    def __init__(self, obj):
        super().__init__(obj)

        self._paramWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/FlowVelocity.ui"
        )

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge", "Vertex"], True, False
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

        # connect unspecified option
        QtCore.QObject.connect(
            self._paramWidget.velocityXBox, QtCore.SIGNAL("toggled(bool)"), self._velocityXEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.velocityYBox, QtCore.SIGNAL("toggled(bool)"), self._velocityYEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.velocityZBox, QtCore.SIGNAL("toggled(bool)"), self._velocityZEnable
        )

        # connect formula option
        QtCore.QObject.connect(
            self._paramWidget.formulaXCB, QtCore.SIGNAL("toggled(bool)"), self._formulaXEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.formulaYCB, QtCore.SIGNAL("toggled(bool)"), self._formulaYEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.formulaZCB, QtCore.SIGNAL("toggled(bool)"), self._formulaZEnable
        )

        self._initParamWidget()

    def _velocityXEnable(self, toggled):
        if toggled:
            self._paramWidget.formulaX.setDisabled(toggled)
            self._paramWidget.velocityX.setDisabled(toggled)
        else:
            if self._paramWidget.formulaXCB.isChecked():
                self._paramWidget.formulaX.setDisabled(toggled)
            else:
                self._paramWidget.velocityX.setDisabled(toggled)

    def _velocityYEnable(self, toggled):
        if toggled:
            self._paramWidget.formulaY.setDisabled(toggled)
            self._paramWidget.velocityY.setDisabled(toggled)
        else:
            if self._paramWidget.formulaYCB.isChecked():
                self._paramWidget.formulaY.setDisabled(toggled)
            else:
                self._paramWidget.velocityY.setDisabled(toggled)

    def _velocityZEnable(self, toggled):
        if toggled:
            self._paramWidget.formulaZ.setDisabled(toggled)
            self._paramWidget.velocityZ.setDisabled(toggled)
        else:
            if self._paramWidget.formulaZCB.isChecked():
                self._paramWidget.formulaZ.setDisabled(toggled)
            else:
                self._paramWidget.velocityZ.setDisabled(toggled)

    def _formulaXEnable(self, toggled):
        FreeCAD.Console.PrintMessage("_formulaXEnable\n")
        if self._paramWidget.velocityXBox.isChecked():
            FreeCAD.Console.PrintMessage("velocityXBox isChecked\n")
            return
        else:
            FreeCAD.Console.PrintMessage("velocityXBox not checked\n")
            self._paramWidget.formulaX.setEnabled(toggled)
            self._paramWidget.velocityX.setDisabled(toggled)

    def _formulaYEnable(self, toggled):
        if self._paramWidget.velocityYBox.isChecked():
            return
        else:
            self._paramWidget.formulaY.setEnabled(toggled)
            self._paramWidget.velocityY.setDisabled(toggled)

    def _formulaZEnable(self, toggled):
        if self._paramWidget.velocitZXBox.isChecked():
            return
        else:
            self._paramWidget.formulaZ.setEnabled(toggled)
            self._paramWidget.velocityZ.setDisabled(toggled)

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self._selectionWidget.finish_selection()
        self._restoreVisibility()
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
        unit = "m/s"
        self._paramWidget.velocityX.setProperty("unit", unit)
        self._paramWidget.velocityY.setProperty("unit", unit)
        self._paramWidget.velocityZ.setProperty("unit", unit)

        self._paramWidget.velocityX.setProperty("value", self.obj.VelocityX)
        FreeCADGui.ExpressionBinding(self._paramWidget.velocityX).bind(self.obj, "VelocityX")
        self._paramWidget.velocityXBox.setChecked(self.obj.VelocityXUnspecified)
        self._paramWidget.formulaX.setText(self.obj.VelocityXFormula)
        self._paramWidget.formulaXCB.setChecked(self.obj.VelocityXHasFormula)

        self._paramWidget.velocityY.setProperty("value", self.obj.VelocityY)
        FreeCADGui.ExpressionBinding(self._paramWidget.velocityY).bind(self.obj, "VelocityY")
        self._paramWidget.velocityYBox.setChecked(self.obj.VelocityYUnspecified)
        self._paramWidget.formulaY.setText(self.obj.VelocityYFormula)
        self._paramWidget.formulaYCB.setChecked(self.obj.VelocityYHasFormula)

        self._paramWidget.velocityZ.setProperty("value", self.obj.VelocityZ)
        FreeCADGui.ExpressionBinding(self._paramWidget.velocityZ).bind(self.obj, "VelocityZ")
        self._paramWidget.velocityZBox.setChecked(self.obj.VelocityZUnspecified)
        self._paramWidget.formulaZ.setText(self.obj.VelocityZFormula)
        self._paramWidget.formulaZCB.setChecked(self.obj.VelocityZHasFormula)

        self._paramWidget.normalBox.setChecked(self.obj.NormalToBoundary)

    def _applyVelocityChanges(self, enabledBox, velocityQSB):
        enabled = enabledBox.isChecked()
        velocity = None
        try:
            velocity = velocityQSB.property("value")
        except ValueError:
            FreeCAD.Console.PrintMessage(
                "Wrong input. Not recognised input: '{}' "
                "Velocity has not been set.\n".format(velocityQSB.text())
            )
            velocity = "0.0 m/s"
        return enabled, velocity

    def _applyWidgetChanges(self):
        # apply the velocities and their enabled state
        self.obj.VelocityXUnspecified, self.obj.VelocityX = self._applyVelocityChanges(
            self._paramWidget.velocityXBox, self._paramWidget.velocityX
        )
        self.obj.VelocityXHasFormula = self._paramWidget.formulaXCB.isChecked()
        self.obj.VelocityXFormula = self._paramWidget.formulaX.text()

        self.obj.VelocityYUnspecified, self.obj.VelocityY = self._applyVelocityChanges(
            self._paramWidget.velocityYBox, self._paramWidget.velocityY
        )
        self.obj.VelocityYHasFormula = self._paramWidget.formulaYCB.isChecked()
        self.obj.VelocityYFormula = self._paramWidget.formulaY.text()

        self.obj.VelocityZUnspecified, self.obj.VelocityZ = self._applyVelocityChanges(
            self._paramWidget.velocityZBox, self._paramWidget.velocityZ
        )
        self.obj.VelocityZHasFormula = self._paramWidget.formulaZCB.isChecked()
        self.obj.VelocityZFormula = self._paramWidget.formulaZ.text()

        self.obj.NormalToBoundary = self._paramWidget.normalBox.isChecked()
