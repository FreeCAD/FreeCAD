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

__title__ = "FreeCAD FEM constraint electrostatic potential ViewProvider for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemConstraintElctrostaticPotential
#  \ingroup FEM
#  \brief FreeCAD FEM view provider for constraint electrostatic potential object

import FreeCAD
import FreeCADGui
from . import ViewProviderFemConstraint

# for the panel
import femtools.femutils as FemUtils
from FreeCAD import Units
from . import FemSelectionWidgets


class ViewProxy(ViewProviderFemConstraint.ViewProxy):

    def getIcon(self):
        return ":/icons/fem-constraint-electrostatic-potential.svg"

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
        self._refWidget = FemSelectionWidgets.BoundarySelector()
        self._refWidget.setReferences(obj.References)
        self._paramWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElectrostaticPotential.ui")
        self._initParamWidget()
        self.form = [self._refWidget, self._paramWidget]
        analysis = FemUtils.findAnalysisOfMember(obj)
        self._mesh = FemUtils.getSingleMember(analysis, "Fem::FemMeshObject")
        self._part = self._mesh.Part if self._mesh is not None else None
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
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def accept(self):
        if self._obj.References != self._refWidget.references():
            self._obj.References = self._refWidget.references()
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
        unit = "V"
        q = Units.Quantity("{} {}".format(self._obj.Potential, unit))

        self._paramWidget.potentialTxt.setText(
            q.UserString)
        self._paramWidget.potentialBox.setChecked(
            not self._obj.PotentialEnabled)
        self._paramWidget.potentialConstantBox.setChecked(
            self._obj.PotentialConstant)

    def _applyWidgetChanges(self):
        unit = "V"
        self._obj.PotentialEnabled = \
            not self._paramWidget.potentialBox.isChecked()
        if self._obj.PotentialEnabled:
            quantity = Units.Quantity(self._paramWidget.potentialTxt.text())
            self._obj.Potential = float(quantity.getValueAs(unit))
        self._obj.PotentialConstant = self._paramWidget.potentialConstantBox.isChecked()
