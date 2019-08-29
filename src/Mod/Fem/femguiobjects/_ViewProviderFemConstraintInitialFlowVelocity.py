# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM constraint initial flow velocity ViewProvider for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemConstraintInitialFlowVelocity
#  \ingroup FEM
#  \brief FreeCAD FEM view provider for constraint initial flow velocity object

import FreeCAD
import FreeCADGui
from . import ViewProviderFemConstraint

# for the panel
import femtools.femutils as femutils
from FreeCAD import Units


class ViewProxy(ViewProviderFemConstraint.ViewProxy):

    def getIcon(self):
        return ":/icons/fem-constraint-initial-flow-velocity.svg"

    def setEdit(self, vobj, mode=0):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show task panel
        task = _TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(task)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True


class _TaskPanel(object):

    def __init__(self, obj):
        self._obj = obj
        self._paramWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/InitialFlowVelocity.ui")
        self._initParamWidget()
        self.form = [self._paramWidget]
        analysis = femutils.findAnalysisOfMember(obj)
        self._mesh = femutils.get_single_member(analysis, "Fem::FemMeshObject")
        self._part = None
        if self._mesh is not None:
            self._part = femutils.get_part_to_mesh(self._mesh)
        self._partVisible = None
        self._meshVisible = None

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
        self._paramWidget.velocityXTxt.setText(
            str(self._obj.VelocityX) + unit)
        self._paramWidget.velocityYTxt.setText(
            str(self._obj.VelocityY) + unit)
        self._paramWidget.velocityZTxt.setText(
            str(self._obj.VelocityZ) + unit)
        self._paramWidget.velocityXBox.setChecked(
            not self._obj.VelocityXEnabled)
        self._paramWidget.velocityYBox.setChecked(
            not self._obj.VelocityYEnabled)
        self._paramWidget.velocityZBox.setChecked(
            not self._obj.VelocityZEnabled)

    def _applyWidgetChanges(self):
        unit = "m/s"
        self._obj.VelocityXEnabled = \
            not self._paramWidget.velocityXBox.isChecked()
        if self._obj.VelocityXEnabled:
            quantity = Units.Quantity(self._paramWidget.velocityXTxt.text())
            self._obj.VelocityX = float(quantity.getValueAs(unit))
        self._obj.VelocityYEnabled = \
            not self._paramWidget.velocityYBox.isChecked()
        if self._obj.VelocityYEnabled:
            quantity = Units.Quantity(self._paramWidget.velocityYTxt.text())
            self._obj.VelocityY = float(quantity.getValueAs(unit))
        self._obj.VelocityZEnabled = \
            not self._paramWidget.velocityZBox.isChecked()
        if self._obj.VelocityZEnabled:
            quantity = Units.Quantity(self._paramWidget.velocityZTxt.text())
            self._obj.VelocityZ = float(quantity.getValueAs(unit))
