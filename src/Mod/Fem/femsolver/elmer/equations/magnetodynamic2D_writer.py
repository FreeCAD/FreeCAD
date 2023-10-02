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

__title__ = "FreeCAD FEM Magnetodynamics2D Elmer writer"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from FreeCAD import Console
from FreeCAD import Units

from .. import sifio
from .. import writer as general_writer


class MgDyn2Dwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getMagnetodynamic2DSolver(self, equation):
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        if not equation.IsHarmonic:
            s["Equation"] = "MgDyn2D"
            s["Procedure"] = sifio.FileAttr("MagnetoDynamics2D/MagnetoDynamics2D")
            s["Variable"] = "Potential"
        else:
            s["Equation"] = "MgDyn2DHarmonic"
            s["Procedure"] = sifio.FileAttr("MagnetoDynamics2D/MagnetoDynamics2DHarmonic")
            s["Variable"] = "Potential[Potential Re:1 Potential Im:1]"
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        return s

    def getMagnetodynamic2DSolverPost(self, equation):
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        s["Equation"] = "MgDyn2DPost"
        s["Exec Solver"] = "Always"
        s["Procedure"] = sifio.FileAttr("MagnetoDynamics/MagnetoDynamicsCalcFields")
        if equation.IsHarmonic:
            s["Angular Frequency"] = float(Units.Quantity(equation.AngularFrequency).Value)
        s["Potential Variable"] = "Potential"
        if equation.CalculateCurrentDensity is True:
            s["Calculate Current Density"] = True
        if equation.CalculateElectricField is True:
            s["Calculate Electric Field"] = True
        if equation.CalculateElementalFields is False:
            s["Calculate Elemental Fields"] = False
        if equation.CalculateHarmonicLoss is True:
            s["Calculate Harmonic Loss"] = True
        if equation.CalculateJouleHeating is True:
            s["Calculate Joule Heating"] = True
        if equation.CalculateMagneticFieldStrength is True:
            s["Calculate Magnetic Field Strength"] = True
        if equation.CalculateMaxwellStress is True:
            s["Calculate Maxwell Stress"] = True
        if equation.CalculateNodalFields is False:
            s["Calculate Nodal Fields"] = False
        if equation.CalculateNodalForces is True:
            s["Calculate Nodal Forces"] = True
        if equation.CalculateNodalHeating is True:
            s["Calculate Nodal Heating"] = True
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        return s

    def handleMagnetodynamic2DConstants(self):
        permeability = self.write.convert(
            self.write.constsdef["PermeabilityOfVacuum"],
            "M*L/(T^2*I^2)"
        )
        # we round in the following to get rid of numerical artifacts
        self.write.constant("Permeability Of Vacuum", round(permeability, 20))

        permittivity = self.write.convert(
            self.write.constsdef["PermittivityOfVacuum"],
            "T^4*I^2/(L^3*M)"
        )
        self.write.constant("Permittivity Of Vacuum", round(permittivity, 20))

    def handleMagnetodynamic2DMaterial(self, bodies):
        # check that all bodies have a set material
        for name in bodies:
            if self.write.getBodyMaterial(name) is None:
                raise general_writer.WriteError(
                    "The body {} is not referenced in any material.\n\n".format(name)
                )
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self.write.getAllBodies())
            for name in (n for n in refs if n in bodies):
                if "ElectricalConductivity" not in m:
                    Console.PrintMessage("m: {}\n".format(m))
                    raise general_writer.WriteError(
                        "The electrical conductivity must be specified for all materials.\n\n"
                    )
                if "RelativePermeability" not in m:
                    Console.PrintMessage("m: {}\n".format(m))
                    raise general_writer.WriteError(
                        "The relative permeability must be specified for all materials.\n\n"
                    )
                self.write.material(name, "Name", m["Name"])
                conductivity = self.write.convert(m["ElectricalConductivity"], "T^3*I^2/(L^3*M)")
                conductivity = round(conductivity, 10)  # to get rid of numerical artifacts
                self.write.material(name, "Electric Conductivity", conductivity)
                self.write.material(
                    name, "Relative Permeability",
                    float(m["RelativePermeability"])
                )
                # permittivity might be necessary for the post processor
                if "RelativePermittivity" in m:
                    self.write.material(
                        name, "Relative Permittivity",
                        float(m["RelativePermittivity"])
                    )

    def _outputMagnetodynamic2DBodyForce(self, obj, name, equation):
        if hasattr(obj, "CurrentDensity_re_1"):
            # output only if current density is enabled and needed
            if not obj.CurrentDensity_re_1_Disabled:
                currentDensity = float(obj.CurrentDensity_re_1.getValueAs("A/m^2"))
                self.write.bodyForce(name, "Current Density", round(currentDensity, 6))
            # imaginaries are only needed for harmonic equation
            if equation.IsHarmonic:
                if not obj.CurrentDensity_im_1_Disabled:
                    currentDensity = float(obj.CurrentDensity_im_1.getValueAs("A/m^2"))
                    self.write.bodyForce(name, "Current Density Im", round(currentDensity, 6))

        if hasattr(obj, "Magnetization_re_1"):
            # output only if magnetization is enabled and needed
            if not obj.Magnetization_re_1_Disabled:
                magnetization = float(obj.Magnetization_re_1.getValueAs("A/m"))
                self.write.material(name, "Magnetization 1", round(magnetization, 6))
            if not obj.Magnetization_re_2_Disabled:
                magnetization = float(obj.Magnetization_re_2.getValueAs("A/m"))
                self.write.material(name, "Magnetization 2", round(magnetization, 6))
            # imaginaries are only needed for harmonic equation
            if equation.IsHarmonic:
                if not obj.Magnetization_im_1_Disabled:
                    magnetization = float(obj.Magnetization_im_1.getValueAs("A/m"))
                    self.write.material(name, "Magnetization Im 1", round(magnetization, 6))
                if not obj.Magnetization_im_2_Disabled:
                    magnetization = float(obj.Magnetization_im_2.getValueAs("A/m"))
                    self.write.material(name, "Magnetization Im 2", round(magnetization, 6))

    def handleMagnetodynamic2DBodyForces(self, bodies, equation):
        currentDensities = self.write.getMember("Fem::ConstraintCurrentDensity")
        for obj in currentDensities:
            if obj.References:
                for name in obj.References[0][1]:
                    self._outputMagnetodynamic2DBodyForce(obj, name, equation)
                self.write.handled(obj)
            else:
                # if there is only one current density without a reference,
                # add it to all bodies
                if len(currentDensities) == 1:
                    for name in bodies:
                        self._outputMagnetodynamic2DBodyForce(obj, name, equation)
                else:
                    raise general_writer.WriteError(
                        "Several current density constraints found without reference to a body.\n"
                        "Please set a body for each current density constraint."
                    )
            self.write.handled(obj)

        magnetizations = self.write.getMember("Fem::ConstraintMagnetization")
        for obj in magnetizations:
            if obj.References:
                for name in obj.References[0][1]:
                    self._outputMagnetodynamic2DBodyForce(obj, name, equation)
                self.write.handled(obj)
            else:
                # if there is only one magnetization without a reference,
                # add it to all bodies
                if len(magnetizations) == 1:
                    for name in bodies:
                        self._outputMagnetodynamic2DBodyForce(obj, name, equation)
                else:
                    raise general_writer.WriteError(
                        "Several magnetization constraints found without reference to a body.\n"
                        "Please set a body for each current density constraint."
                    )
            self.write.handled(obj)

    def handleMagnetodynamic2DBndConditions(self):
        for obj in self.write.getMember("Fem::ConstraintElectrostaticPotential"):
            if obj.References:
                for name in obj.References[0][1]:
                    # output the FreeCAD label as comment
                    if obj.Label:
                        self.write.boundary(name, "! FreeCAD Name", obj.Label)
                    if obj.PotentialEnabled:
                        if hasattr(obj, "Potential"):
                            potential = float(obj.Potential.getValueAs("V"))
                            self.write.boundary(name, "Potential", round(potential, 6))
                    if obj.ElectricInfinity:
                        self.write.boundary(name, "Infinity BC", True)
                self.write.handled(obj)

    def handleMagnetodynamic2DEquation(self, bodies, equation):
        for b in bodies:
            if equation.IsHarmonic and (equation.AngularFrequency == 0):
                raise general_writer.WriteError(
                    "The angular frequency must not be zero.\n\n"
                )
            self.write.equation(b, "Name", equation.Name)
            if equation.IsHarmonic:
                frequency = float(Units.Quantity(equation.AngularFrequency).Value)
                self.write.equation(b, "Angular Frequency", round(frequency, 6))

##  @}
