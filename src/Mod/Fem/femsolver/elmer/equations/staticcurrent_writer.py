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

__title__ = "FreeCAD FEM StaticCurrent Elmer writer"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from FreeCAD import Units
from .. import sifio


class SCwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getStaticCurrentSolver(self, equation):
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        s["Equation"] = "Stat Current Solver"
        s["Procedure"] = sifio.FileAttr("StatCurrentSolve/StatCurrentSolver")
        s["Variable"] = self.write.getUniqueVarName("Potential")
        s["Variable DOFs"] = 1
        s["Calculate Volume Current"] = equation.CalculateVolumeCurrent
        s["Calculate Joule Heating"] = equation.CalculateJouleHeating
        s["Constant Weights"] = equation.ConstantWeights
        s["Calculate Nodal Heating"] = equation.CalculateNodalHeating
        if equation.PowerControl:
            s["Power Control"] = equation.Power.getValueAs("W").Value
        if equation.CurrentControl:
            s["Current Control"] = equation.Current.getValueAs("A").Value
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize

        return s

    def handleStaticCurrentConstants(self):
        pass

    def handleStaticCurrentMaterial(self, bodies):
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = obj.References[0][1] if obj.References else self.write.getAllBodies()
            for name in (n for n in refs if n in bodies):
                self.write.material(name, "Name", m["Name"])
                if "ElectricalConductivity" in m:
                    self.write.material(
                        name,
                        "Electric Conductivity",
                        Units.Quantity(m["ElectricalConductivity"]).getValueAs("S/m").Value,
                    )

    def handleStaticCurrentBndConditions(self):
        for obj in self.write.getMember("Fem::ConstraintElectrostaticPotential"):
            if obj.References:
                for name in obj.References[0][1]:
                    # output the FreeCAD label as comment
                    if obj.Label:
                        self.write.boundary(name, "! FreeCAD Name", obj.Label)
                    if obj.BoundaryCondition == "Dirichlet":
                        if obj.PotentialEnabled:
                            self.write.boundary(name, "Current Density BC", False)
                            self.write.boundary(
                                name, "Potential", obj.Potential.getValueAs("V").Value
                            )
                self.write.handled(obj)

        for obj in self.write.getMember("Fem::ConstraintCurrentDensity"):
            if obj.References:
                for name in obj.References[0][1]:
                    # output the FreeCAD label as comment
                    if obj.Label:
                        self.write.boundary(name, "! FreeCAD Name", obj.Label)
                    if obj.Mode == "Normal":
                        self.write.boundary(name, "Current Density BC", True)
                        self.write.boundary(
                            name,
                            "Current Density",
                            obj.NormalCurrentDensity_re.getValueAs("A/m^2").Value,
                        )

                self.write.handled(obj)

    def handleStaticCurrentBodyForces(self, bodies, equation):
        for name in bodies:
            self.write.bodyForce(name, "Joule Heat", equation.HeatSource)


##  @}
