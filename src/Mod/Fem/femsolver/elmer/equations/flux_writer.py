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

__title__ = "FreeCAD FEM Flux Elmer writer"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from .. import sifio
from . import flux


class Fluxwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getFluxSolver(self, equation):
        s = self.write.createLinearSolver(equation)
        # check if we need to update the equation
        self._updateFluxSolver(equation)
        # output the equation parameters
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("FluxSolver/FluxSolver")
        if equation.AverageWithinMaterials is True:
            s["Average Within Materials"] = equation.AverageWithinMaterials
        s["Calculate Flux"] = equation.CalculateFlux
        if equation.CalculateFluxAbs is True:
            s["Calculate Flux Abs"] = equation.CalculateFluxAbs
        if equation.CalculateFluxMagnitude is True:
            s["Calculate Flux Magnitude"] = equation.CalculateFluxMagnitude
        s["Calculate Grad"] = equation.CalculateGrad
        if equation.CalculateGradAbs is True:
            s["Calculate Grad Abs"] = equation.CalculateGradAbs
        if equation.CalculateGradMagnitude is True:
            s["Calculate Grad Magnitude"] = equation.CalculateGradMagnitude
        if equation.DiscontinuousGalerkin is True:
            s["Discontinuous Galerkin"] = equation.DiscontinuousGalerkin
        if equation.EnforcePositiveMagnitude is True:
            s["Enforce Positive Magnitude"] = equation.EnforcePositiveMagnitude
        s["Flux Coefficient"] = equation.FluxCoefficient
        s["Flux Variable"] = equation.FluxVariable
        s["Stabilize"] = equation.Stabilize
        return s

    def _updateFluxSolver(self, equation):
        # updates older Flux equations
        if not hasattr(equation, "AverageWithinMaterials"):
            equation.addProperty(
                "App::PropertyBool",
                "AverageWithinMaterials",
                "Flux",
                (
                    "Enforces continuity within the same material\n"
                    "in the 'Discontinuous Galerkin' discretization"
                )
            )
        if hasattr(equation, "Bubbles"):
            # Bubbles was removed because it is unused by Elmer for the flux solver
            equation.removeProperty("Bubbles")
        if not hasattr(equation, "CalculateFluxAbs"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateFluxAbs",
                "Flux",
                "Computes absolute of flux vector"
            )
        if not hasattr(equation, "CalculateFluxMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateFluxMagnitude",
                "Flux",
                "Computes magnitude of flux vector field"
            )
        if not hasattr(equation, "CalculateGradAbs"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateGradAbs",
                "Flux",
                "Computes absolute of gradient field"
            )
        if not hasattr(equation, "CalculateGradMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateGradMagnitude",
                "Flux",
                "Computes magnitude of gradient field"
            )
        if not hasattr(equation, "DiscontinuousGalerkin"):
            equation.addProperty(
                "App::PropertyBool",
                "DiscontinuousGalerkin",
                "Flux",
                (
                    "Enable if standard Galerkin approximation leads to\n"
                    "unphysical results when there are discontinuities"
                )
            )
        if not hasattr(equation, "EnforcePositiveMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "EnforcePositiveMagnitude",
                "Flux",
                (
                    "If true, negative values of computed magnitude fields\n"
                    "are a posteriori set to zero."
                )
            )
        tempFluxCoefficient = ""
        if hasattr(equation, "FluxCoefficient"):
            if equation.FluxCoefficient not in flux.COEFFICIENTS:
                # was an App::PropertyString and changed to
                # App::PropertyEnumeration
                tempFluxCoefficient = equation.FluxCoefficient
                equation.removeProperty("FluxCoefficient")
        if not hasattr(equation, "FluxCoefficient"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "FluxCoefficient",
                "Flux",
                "Name of proportionality coefficient\nto compute the flux"
            )
            equation.FluxCoefficient = flux.COEFFICIENTS
            if tempFluxCoefficient:
                equation.FluxCoefficient = tempFluxCoefficient
            else:
                equation.FluxCoefficient = "None"
        tempFluxVariable = ""
        if hasattr(equation, "FluxVariable"):
            if equation.FluxVariable not in flux.VARIABLES:
                # was an App::PropertyString and changed to
                # App::PropertyEnumeration
                tempFluxVariable = equation.FluxVariable
                equation.removeProperty("FluxVariable")
                equation.addProperty(
                    "App::PropertyEnumeration",
                    "FluxVariable",
                    "Flux",
                    "Variable name for flux calculation"
                )
                equation.FluxVariable = flux.VARIABLES
                equation.FluxVariable = tempFluxVariable

##  @}
