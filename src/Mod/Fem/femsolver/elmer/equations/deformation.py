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

__title__ = "FreeCAD FEM solver Elmer equation object Deformation"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from . import nonlinear
from ... import equationbase


def create(doc, name="Deformation"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(nonlinear.Proxy, equationbase.DeformationProxy):

    Type = "Fem::EquationElmerDeformation"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "CalculatePangle",
            "Deformation",
            "Compute principal stress angles"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculatePrincipal",
            "Deformation",
            "Compute principal stress components"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateStrains",
            "Deformation",
            "Compute the strain tensor"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateStresses",
            "Deformation",
            "Compute stress tensor and vanMises"
        )
        obj.addProperty(
            "App::PropertyBool",
            "InitializeStateVariables",
            "Deformation",
            "See Elmer manual for info"
        )
        obj.addProperty(
            "App::PropertyBool",
            "MixedFormulation",
            "Deformation",
            "See Elmer manual for info"
        )
        obj.addProperty(
            "App::PropertyBool",
            "NeoHookeanMaterial",
            "Deformation",
            (
                "Uses the neo-Hookean material model"
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "PlaneStress",
            "Equation",
            (
                "Computes solution according to plane\nstress situation.\n"
                "Applies only for 2D geometry."
            )
        )
        obj.addProperty(
            "App::PropertyString",
            "Variable",
            "Deformation",
            "Only for a 2D model change the '3' to '2'"
        )

        obj.Priority = 10
        obj.CalculatePrincipal = True
        obj.CalculateStresses = True
        obj.Variable = "-dofs 3 Displacement"


class ViewProxy(nonlinear.ViewProxy, equationbase.DeformationViewProxy):
    pass

##  @}
