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

__title__ = "FreeCAD FEM Electrostatics Elmer writer"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from FreeCAD import Units

from .. import sifio


class ESwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getElectrostaticSolver(self, equation):
        # check if we need to update the equation
        self._updateElectrostaticSolver(equation)
        # output the equation parameters
        s = self.write.createLinearSolver(equation)
        s["Equation"] = "Stat Elec Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("StatElecSolve/StatElecSolver")
        s["Variable"] = self.write.getUniqueVarName("Potential")
        s["Variable DOFs"] = 1
        if equation.CalculateCapacitanceMatrix is True:
            s["Calculate Capacitance Matrix"] = equation.CalculateCapacitanceMatrix
            s["Capacitance Matrix Filename"] = equation.CapacitanceMatrixFilename
        if equation.CalculateElectricEnergy is True:
            s["Calculate Electric Energy"] = equation.CalculateElectricEnergy
        if equation.CalculateElectricField is True:
            s["Calculate Electric Field"] = equation.CalculateElectricField
        if equation.CalculateElectricFlux is True:
            s["Calculate Electric Flux"] = equation.CalculateElectricFlux
        if equation.CalculateSurfaceCharge is True:
            s["Calculate Surface Charge"] = equation.CalculateSurfaceCharge
        if equation.ConstantWeights is True:
            s["Constant Weights"] = equation.ConstantWeights
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        if equation.CalculateCapacitanceMatrix is False and (equation.PotentialDifference != 0.0):
            s["Potential Difference"] = equation.PotentialDifference
        s["Stabilize"] = equation.Stabilize
        return s

    def _updateElectrostaticSolver(self, equation):
        # updates older Electrostatic equations
        if not hasattr(equation, "CapacitanceMatrixFilename"):
            equation.addProperty(
                "App::PropertyFile",
                "CapacitanceMatrixFilename",
                "Electrostatic",
                (
                    "File where capacitance matrix is being saved\n"
                    "Only used if 'CalculateCapacitanceMatrix' is true"
                ),
                locked=True,
            )
            equation.CapacitanceMatrixFilename = "cmatrix.dat"
        if not hasattr(equation, "ConstantWeights"):
            equation.addProperty(
                "App::PropertyBool",
                "ConstantWeights",
                "Electrostatic",
                "Use constant weighting for results",
                locked=True,
            )
        if not hasattr(equation, "PotentialDifference"):
            equation.addProperty(
                "App::PropertyFloat",
                "PotentialDifference",
                "Electrostatic",
                (
                    "Potential difference in Volt for which capacitance is\n"
                    "calculated if 'CalculateCapacitanceMatrix' is false"
                ),
                locked=True,
            )
            equation.PotentialDifference = 0.0

    def handleElectrostaticConstants(self):
        permittivity = Units.Quantity(self.write.constsdef["PermittivityOfVacuum"]).getValueAs(
            "F/m"
        )
        self.write.constant("Permittivity Of Vacuum", permittivity)

    def handleElectrostaticMaterial(self, bodies):
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = obj.References[0][1] if obj.References else self.write.getAllBodies()
            for name in (n for n in refs if n in bodies):
                self.write.material(name, "Name", m["Name"])
                if "RelativePermittivity" in m:
                    self.write.material(
                        name, "Relative Permittivity", float(m["RelativePermittivity"])
                    )

    def handleElectrostaticBndConditions(self):
        for obj in self.write.getMember("Fem::ConstraintElectrostaticPotential"):
            if obj.References:
                for name in obj.References[0][1]:
                    # output the FreeCAD label as comment
                    if obj.Label:
                        self.write.boundary(name, "! FreeCAD Name", obj.Label)
                    if obj.BoundaryCondition == "Dirichlet":
                        if obj.PotentialEnabled:
                            potential = obj.Potential.getValueAs("V")
                            self.write.boundary(name, "Potential", potential)
                    elif obj.BoundaryCondition == "Neumann":
                        flux_density = obj.ElectricFluxDensity.getValueAs("C/m^2")
                        self.write.boundary(name, "Electric Flux", flux_density)
                    if obj.PotentialConstant:
                        self.write.boundary(name, "Potential Constant", True)
                    if obj.ElectricInfinity:
                        self.write.boundary(name, "Electric Infinity BC", True)
                    if obj.ElectricForcecalculation:
                        self.write.boundary(name, "Calculate Electric Force", True)
                    if obj.CapacitanceBodyEnabled:
                        self.write.boundary(name, "Capacitance Body", obj.CapacitanceBody)
                self.write.handled(obj)

        for obj in self.write.getMember("Fem::ConstraintElectricChargeDensity"):
            match obj.Mode:
                case "Interface":
                    density = obj.InterfaceChargeDensity
                case "Total Interface":
                    density = obj.Proxy.get_total_interface_density(obj)
                case _:
                    continue

            for feat, sub_elem in obj.References:
                for name in sub_elem:
                    self.write.boundary(name, "! FreeCAD Name", obj.Label)
                    self.write.boundary(
                        name,
                        "Surface Charge Density",
                        density.getValueAs("C/m^2"),
                    )
                    self.write.handled(obj)

    def handleElectrostaticBodyForces(self):
        for obj in self.write.getMember("Fem::ConstraintElectricChargeDensity"):
            match obj.Mode:
                case "Source":
                    density = obj.SourceChargeDensity
                case "Total Source":
                    density = obj.Proxy.get_total_source_density(obj)
                case _:
                    continue

            for feat, sub_elem in obj.References:
                for name in sub_elem:
                    self.write.bodyForce(name, "! FreeCAD Name", obj.Label)
                    self.write.bodyForce(name, "Charge Density", density.getValueAs("C/m^3"))
                    self.write.handled(obj)


##  @}
