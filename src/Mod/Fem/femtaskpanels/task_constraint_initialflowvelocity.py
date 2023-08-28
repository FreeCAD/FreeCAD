# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM constraint initial flow velocity task panel for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_constraint_initialflowvelocity
#  \ingroup FEM
#  \brief task panel for constraint initial flow velocity object

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
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/InitialFlowVelocity.ui")

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References,
            ["Solid", "Face"],
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

        # connect unspecified option
        QtCore.QObject.connect(
            self._paramWidget.velocityXBox,
            QtCore.SIGNAL("toggled(bool)"),
            self._velocityXEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.velocityYBox,
            QtCore.SIGNAL("toggled(bool)"),
            self._velocityYEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.velocityZBox,
            QtCore.SIGNAL("toggled(bool)"),
            self._velocityZEnable
        )

        # connect formula option
        QtCore.QObject.connect(
            self._paramWidget.formulaXCB,
            QtCore.SIGNAL("toggled(bool)"),
            self._formulaXEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.formulaYCB,
            QtCore.SIGNAL("toggled(bool)"),
            self._formulaYEnable
        )
        QtCore.QObject.connect(
            self._paramWidget.formulaZCB,
            QtCore.SIGNAL("toggled(bool)"),
            self._formulaZEnable
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
        FreeCADGui.ActiveDocument.resetEdit()
        self._restoreVisibility()
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
        unit = "m/s"
        self._paramWidget.velocityX.setProperty('unit', unit)
        self._paramWidget.velocityY.setProperty('unit', unit)
        self._paramWidget.velocityZ.setProperty('unit', unit)

        self._paramWidget.velocityX.setProperty(
            'value', self._obj.VelocityX)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.velocityX).bind(self._obj, "VelocityX")
        self._paramWidget.velocityXBox.setChecked(
            self._obj.VelocityXUnspecified)
        self._paramWidget.formulaX.setText(self._obj.VelocityXFormula)
        self._paramWidget.formulaXCB.setChecked(
            self._obj.VelocityXHasFormula)

        self._paramWidget.velocityY.setProperty(
            'value', self._obj.VelocityY)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.velocityY).bind(self._obj, "VelocityY")
        self._paramWidget.velocityYBox.setChecked(
            self._obj.VelocityYUnspecified)
        self._paramWidget.formulaY.setText(self._obj.VelocityYFormula)
        self._paramWidget.formulaYCB.setChecked(
            self._obj.VelocityYHasFormula)

        self._paramWidget.velocityZ.setProperty(
            'value', self._obj.VelocityZ)
        FreeCADGui.ExpressionBinding(
            self._paramWidget.velocityZ).bind(self._obj, "VelocityZ")
        self._paramWidget.velocityZBox.setChecked(
            self._obj.VelocityZUnspecified)
        self._paramWidget.formulaZ.setText(self._obj.VelocityZFormula)
        self._paramWidget.formulaZCB.setChecked(
            self._obj.VelocityZHasFormula)

    def _applyVelocityChanges(self, enabledBox, velocityQSB):
        enabled = enabledBox.isChecked()
        velocity = None
        try:
            velocity = velocityQSB.property('value')
        except ValueError:
            FreeCAD.Console.PrintMessage(
                "Wrong input. Not recognised input: '{}' "
                "Velocity has not been set.\n".format(velocityQSB.text())
            )
            velocity = '0.0 m/s'
        return enabled, velocity

    def _applyWidgetChanges(self):
        # apply the velocities and their enabled state
        self._obj.VelocityXUnspecified, self._obj.VelocityX = \
            self._applyVelocityChanges(
                self._paramWidget.velocityXBox,
                self._paramWidget.velocityX
            )
        self._obj.VelocityXHasFormula = self._paramWidget.formulaXCB.isChecked()
        self._obj.VelocityXFormula = self._paramWidget.formulaX.text()

        self._obj.VelocityYUnspecified, self._obj.VelocityY = \
            self._applyVelocityChanges(
                self._paramWidget.velocityYBox,
                self._paramWidget.velocityY
            )
        self._obj.VelocityYHasFormula = self._paramWidget.formulaYCB.isChecked()
        self._obj.VelocityYFormula = self._paramWidget.formulaY.text()

        self._obj.VelocityZUnspecified, self._obj.VelocityZ = \
            self._applyVelocityChanges(
                self._paramWidget.velocityZBox,
                self._paramWidget.velocityZ
            )
        self._obj.VelocityZHasFormula = self._paramWidget.formulaZCB.isChecked()
        self._obj.VelocityZFormula = self._paramWidget.formulaZ.text()
