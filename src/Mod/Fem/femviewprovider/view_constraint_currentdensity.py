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

__title__ = "FreeCAD FEM constraint current density ViewProvider for the document object"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package view_constraint_currentdensity
#  \ingroup FEM
#  \brief view provider for the constraint current density object

from pivy import coin

from femtaskpanels import task_constraint_currentdensity
from . import view_base_femconstraint


class VPConstraintCurrentDensity(view_base_femconstraint.VPBaseFemConstraint):

    def __init__(self, vobj):
        super().__init__(vobj)
        mat = vobj.ShapeAppearance[0]
        mat.DiffuseColor = (0.71, 0.40, 0.11, 0.0)
        vobj.ShapeAppearance = mat

    def setEdit(self, vobj, mode=0):
        return view_base_femconstraint.VPBaseFemConstraint.setEdit(
            self, vobj, mode, task_constraint_currentdensity._TaskPanel
        )

    def attach(self, vobj):
        super().attach(vobj)
        vobj.loadSymbol(self.resource_symbol_dir + "ConstraintCurrentDensity.iv")

    def updateData(self, obj, prop):
        if prop == "Mode":
            symb = obj.ViewObject.SymbolNode.getChild(0)
            if obj.Mode == "Normal":
                obj.ViewObject.RotateSymbol = True
                symb.whichChild.setValue(0)
            elif obj.Mode == "Custom":
                obj.ViewObject.RotateSymbol = False
                symb.whichChild.setValue(-1)
