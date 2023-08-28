# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver Elmer equation object Flow"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from . import nonlinear
from ... import equationbase

CONVECTION_TYPE = ["None", "Computed", "Constant"]
FLOW_MODEL = ["Full", "No convection", "Stokes"]


def create(doc, name="Flow"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(nonlinear.Proxy, equationbase.FlowProxy):

    Type = "Fem::EquationElmerFlow"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "DivDiscretization",
            "Flow",
            (
                "Set to true for incompressible flow for more stable\n"
                "discretization when Reynolds number increases"
            )
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "FlowModel",
            "Flow",
            "Flow model to be used"
        )
        obj.addProperty(
            "App::PropertyBool",
            "GradpDiscretization",
            "Flow",
            (
                "If true pressure Dirichlet boundary conditions can be used.\n"
                "Also mass flux is available as a natural boundary condition."
            )
        )
        obj.addProperty(
            "App::PropertyString",
            "Variable",
            "Flow",
            "Only for a 2D model change the '3' to '2'"
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "Convection",
            "Equation",
            "Type of convection to be used"
        )
        obj.addProperty(
            "App::PropertyBool",
            "MagneticInduction",
            "Equation",
            (
                "Magnetic induction equation will be solved\n"
                "along with the Navier-Stokes equations"
            )
        )
        obj.FlowModel = FLOW_MODEL
        obj.FlowModel = "Full"
        obj.Variable = "Flow Solution[Velocity:3 Pressure:1]"
        obj.Convection = CONVECTION_TYPE
        obj.Convection = "Computed"
        obj.Priority = 10


class ViewProxy(nonlinear.ViewProxy, equationbase.FlowViewProxy):
    pass

##  @}
