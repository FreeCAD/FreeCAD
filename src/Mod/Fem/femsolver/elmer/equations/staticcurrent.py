# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM solver Elmer equation object StaticCurrent"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from ... import equationbase
from . import nonlinear


def create(doc, name="StaticCurrent"):
    return femutils.createObject(doc, name, Proxy, ViewProxy)


class Proxy(nonlinear.Proxy, equationbase.StaticCurrentProxy):

    Type = "Fem::EquationElmerStaticCurrent"

    def __init__(self, obj):
        super().__init__(obj)

        obj.addProperty(
            "App::PropertyBool", "CalculateVolumeCurrent", "StaticCurrent", "", locked=True
        )
        obj.CalculateVolumeCurrent = True
        obj.addProperty(
            "App::PropertyBool", "CalculateJouleHeating", "StaticCurrent", "", locked=True
        )
        obj.addProperty(
            "App::PropertyBool",
            "ConstantWeights",
            "StaticCurrent",
            "Used to turn constant weighting on for the results",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateNodalHeating",
            "StaticCurrent",
            "Calculate nodal heating that may be used to couple the heat equation optimally when using conforming finite element meshes",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "HeatSource",
            "StaticCurrent",
            "Use Joule heating as a heat source in combination with heat equation",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "PowerControl",
            "StaticCurrent",
            "Apply power control with the desired heating power",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "CurrentControl",
            "StaticCurrent",
            "Apply current control with the desired current",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyElectricCurrent",
            "Current",
            "StaticCurrent",
            "Current control value",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyPower", "Power", "StaticCurrent", "Power control value", locked=True
        )


class ViewProxy(nonlinear.ViewProxy, equationbase.StaticCurrentViewProxy):
    pass


##  @}
