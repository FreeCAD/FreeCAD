# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver Elmer equation object Flux"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from ... import equationbase
from . import linear

COEFFICIENTS = ["Heat Conductivity", "None"]
VARIABLES = ["Potential", "Temperature"]


def create(doc, name="Flux"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(linear.Proxy, equationbase.FluxProxy):

    Type = "Fem::EquationElmerFlux"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "AverageWithinMaterials",
            "Flux",
            (
                "Enforces continuity within the same material\n"
                "in the 'Discontinuous Galerkin' discretization"
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFlux",
            "Flux",
            "Computes flux vector"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFluxAbs",
            "Flux",
            "Computes absolute of flux vector"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFluxMagnitude",
            "Flux",
            "Computes magnitude of flux vector field"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGrad",
            "Flux",
            "Select calculation of gradient"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGradAbs",
            "Flux",
            "Computes absolute of gradient field"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGradMagnitude",
            "Flux",
            "Computes magnitude of gradient field"
        )
        obj.addProperty(
            "App::PropertyBool",
            "DiscontinuousGalerkin",
            "Flux",
            (
                "Enable if standard Galerkin approximation leads to\n"
                "unphysical results when there are discontinuities"
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "EnforcePositiveMagnitude",
            "Flux",
            (
                "If true, negative values of computed magnitude fields\n"
                "are a posteriori set to zero."
            )
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "FluxCoefficient",
            "Flux",
            "Proportionality coefficient\nto compute the flux"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "FluxVariable",
            "Flux",
            "Variable name for flux calculation"
        )

        obj.CalculateFlux = True
        # set defaults according to the Elmer manual
        obj.FluxCoefficient = COEFFICIENTS
        obj.FluxCoefficient = "Heat Conductivity"
        obj.FluxVariable = VARIABLES
        obj.FluxVariable = "Temperature"
        # Electrostatic has priority 10, Heat has 20 and Flux needs
        # to be solved before these equations
        # therefore set priority to 25
        obj.Priority = 25


class ViewProxy(linear.ViewProxy, equationbase.FluxViewProxy):
    pass

##  @}
