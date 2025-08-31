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

__title__ = "FreeCAD FEM Magnetodynamics Elmer writer"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from FreeCAD import Console
from FreeCAD import Units

from femtools import femutils
from .. import sifio
from .. import writer as general_writer


class MgDynwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getMagnetodynamicSolver(self, equation):
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        if not equation.IsHarmonic:
            s["Equation"] = "MgDyn"
            s["Procedure"] = sifio.FileAttr("MagnetoDynamics/WhitneyAVSolver")
            s["Variable"] = "av"
        else:
            s["Equation"] = "MgDynHarmonic"
            s["Procedure"] = sifio.FileAttr("MagnetoDynamics/WhitneyAVHarmonicSolver")
            s["Variable"] = "av[av re:1 av im:1]"
            frequency = equation.AngularFrequency.getValueAs("Hz")
            s["Angular Frequency"] = frequency
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        if equation.LinearSystemRefactorize is True:
            s["Linear System Refactorize"] = True
        if equation.UsePiolaTransform is True:
            s["Use Piola Transform"] = True
        if equation.QuadraticApproximation is True:
            s["Quadratic Approximation"] = True
        if equation.StaticConductivity is True:
            s["Static Conductivity"] = True
        if equation.FixInputCurrentDensity is True:
            s["Fix Input Current Density"] = True
        if equation.AutomatedSourceProjectionBCs is True:
            s["Automated Source Projection BCs"] = True
        if equation.UseLagrangeGauge is True:
            s["Use Lagrange Gauge"] = True
        if equation.LagrangeGaugePenalizationCoefficient != 0.0:
            s["Lagrange Gauge Penalization Coefficient"] = (
                equation.LagrangeGaugePenalizationCoefficient
            )
        if equation.UseTreeGauge is True:
            s["Use Tree Gauge"] = True
        return s

    def getMagnetodynamicSolverPost(self, equation):
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        s["Equation"] = "MgDynPost"
        s["Exec Solver"] = "Before Saving"
        s["Procedure"] = sifio.FileAttr("MagnetoDynamics/MagnetoDynamicsCalcFields")
        if equation.IsHarmonic:
            frequency = equation.AngularFrequency.getValueAs("Hz")
            s["Angular Frequency"] = frequency
        s["Potential Variable"] = "av"
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
        if equation.DiscontinuousBodies is True:
            s["Discontinuous Bodies"] = True
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        return s

    def handleMagnetodynamicConstants(self):
        permeability = Units.Quantity(self.write.constsdef["PermeabilityOfVacuum"]).getValueAs(
            "H/m"
        )
        self.write.constant("Permeability Of Vacuum", permeability)

        permittivity = Units.Quantity(self.write.constsdef["PermittivityOfVacuum"]).getValueAs(
            "F/m"
        )
        self.write.constant("Permittivity Of Vacuum", permittivity)

    def handleMagnetodynamicMaterial(self, bodies):
        # check that all bodies have a set material
        for name in bodies:
            if self.write.getBodyMaterial(name) is None:
                raise general_writer.WriteError(
                    f"The body {name} is not referenced in any material.\n\n"
                )
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = obj.References[0][1] if obj.References else self.write.getAllBodies()
            for name in (n for n in refs if n in bodies):
                if "ElectricalConductivity" not in m:
                    Console.PrintMessage(f"m: {m}\n")
                    raise general_writer.WriteError(
                        "The electrical conductivity must be specified for all materials.\n\n"
                    )
                if "RelativePermeability" not in m:
                    Console.PrintMessage(f"m: {m}\n")
                    raise general_writer.WriteError(
                        "The relative permeability must be specified for all materials.\n\n"
                    )
                self.write.material(name, "Name", m["Name"])
                conductivity = Units.Quantity(m["ElectricalConductivity"]).getValueAs("S/m")
                self.write.material(name, "Electric Conductivity", conductivity)
                self.write.material(name, "Relative Permeability", float(m["RelativePermeability"]))
                # permittivity might be necessary for the post processor
                if "RelativePermittivity" in m:
                    self.write.material(
                        name, "Relative Permittivity", float(m["RelativePermittivity"])
                    )

    def _outputMagnetodynamicBodyForce(self, obj, name, equation):
        if femutils.is_derived_from(obj, "Fem::ConstraintCurrentDensity") and obj.Mode == "Custom":
            # output only if current density is enabled and needed
            if obj.EnableCurrentDensity_re_1:
                current_density = obj.CurrentDensity_re_1.getValueAs("A/m^2")
                self.write.bodyForce(name, "Current Density 1", current_density)
            if obj.EnableCurrentDensity_re_2:
                current_density = obj.CurrentDensity_re_2.getValueAs("A/m^2")
                self.write.bodyForce(name, "Current Density 2", current_density)
            if obj.EnableCurrentDensity_re_3:
                current_density = obj.CurrentDensity_re_3.getValueAs("A/m^2")
                self.write.bodyForce(name, "Current Density 3", current_density)
            # imaginaries are only needed for harmonic equation
            if equation.IsHarmonic:
                if obj.EnableCurrentDensity_im_1:
                    current_density = obj.CurrentDensity_im_1.getValueAs("A/m^2")
                    self.write.bodyForce(name, "Current Density Im 1", current_density)
                if obj.EnableCurrentDensity_im_2:
                    current_density = obj.CurrentDensity_im_2.getValueAs("A/m^2")
                    self.write.bodyForce(name, "Current Density Im 2", current_density)
                if obj.EnableCurrentDensity_im_3:
                    current_density = obj.CurrentDensity_im_3.getValueAs("A/m^2")
                    self.write.bodyForce(name, "Current Density Im 3", current_density)

        if femutils.is_derived_from(obj, "Fem::ConstraintMagnetization"):
            # output only if magnetization is enabled and needed
            if obj.EnableMagnetization_1:
                magnetization = obj.Magnetization_re_1.getValueAs("A/m")
                self.write.bodyForce(name, "Magnetization 1", magnetization)
            if obj.EnableMagnetization_2:
                magnetization = obj.Magnetization_re_2.getValueAs("A/m")
                self.write.bodyForce(name, "Magnetization 2", magnetization)
            if obj.EnableMagnetization_3:
                magnetization = obj.Magnetization_re_3.getValueAs("A/m")
                self.write.bodyForce(name, "Magnetization 3", magnetization)
            # imaginaries are only needed for harmonic equation
            if equation.IsHarmonic:
                if obj.EnableMagnetization_1:
                    magnetization = obj.Magnetization_im_1.getValueAs("A/m")
                    self.write.bodyForce(name, "Magnetization Im 1", magnetization)
                if obj.EnableMagnetization_2:
                    magnetization = obj.Magnetization_im_2.getValueAs("A/m")
                    self.write.bodyForce(name, "Magnetization Im 2", magnetization)
                if obj.EnableMagnetization_3:
                    magnetization = obj.Magnetization_im_3.getValueAs("A/m")
                    self.write.bodyForce(name, "Magnetization Im 3", magnetization)

        if femutils.is_derived_from(obj, "Fem::ConstraintElectrostaticPotential"):
            if obj.PotentialEnabled:
                # output only if potential is enabled and needed
                potential = obj.Potential.getValueAs("V")
                self.write.bodyForce(name, "Electric Potential", potential)
                # imaginary is only needed for harmonic equation
                if equation.IsHarmonic:
                    potential = obj.AV_im.getValueAs("V")
                    self.write.bodyForce(name, "Electric Potential Im", potential)

    def handleMagnetodynamicBodyForces(self, bodies, equation):
        # the current density can either be a body force or a boundary constraint
        # therefore only output here if a solid is referenced
        currentDensities = self.write.getMember("Fem::ConstraintCurrentDensity")
        for obj in currentDensities:
            if obj.References:
                firstName = obj.References[0][1][0]
                firstName = firstName.rstrip("0123456789")
                if firstName == "Solid":
                    for name in obj.References[0][1]:
                        self._outputMagnetodynamicBodyForce(obj, name, equation)
                    self.write.handled(obj)
            else:
                # if there is only one current density without a reference,
                # add it to all bodies
                if len(currentDensities) == 1:
                    for name in bodies:
                        self._outputMagnetodynamicBodyForce(obj, name, equation)
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
                    self._outputMagnetodynamicBodyForce(obj, name, equation)
                self.write.handled(obj)
            else:
                # if there is only one magnetization without a reference,
                # add it to all bodies
                if len(magnetizations) == 1:
                    for name in bodies:
                        self._outputMagnetodynamicBodyForce(obj, name, equation)
                else:
                    raise general_writer.WriteError(
                        "Several magnetization constraints found without reference to a body.\n"
                        "Please set a body for each current density constraint."
                    )
            self.write.handled(obj)

        # the potential can either be a body force or a boundary constraint
        # therefore only output here if a solid is referenced
        potentials = self.write.getMember("Fem::ConstraintElectrostaticPotential")
        for obj in potentials:
            if obj.References:
                firstName = obj.References[0][1][0]
                firstName = firstName.rstrip("0123456789")
                if firstName == "Solid":
                    for name in obj.References[0][1]:
                        # output only if potentiual is enabled and needed
                        self._outputMagnetodynamicBodyForce(obj, name, equation)
                    self.write.handled(obj)

    def _outputMagnetodynamicBndConditions(self, obj, name, equation):
        if femutils.is_derived_from(obj, "Fem::ConstraintCurrentDensity") and obj.Mode == "Normal":
            current_density = obj.NormalCurrentDensity_re.getValueAs("A/m^2")
            self.write.boundary(name, "Electric Current Density", current_density)
            # imaginaries are only needed for harmonic equation
            if equation.IsHarmonic:
                current_density = obj.NormalCurrentDensity_im.getValueAs("A/m^2")
                self.write.boundary(name, "Electric Current Density Im", current_density)

        if femutils.is_derived_from(obj, "Fem::ConstraintElectrostaticPotential"):
            if obj.EnableAV:
                potential = obj.AV_re.getValueAs("V")
                if equation.IsHarmonic:
                    self.write.boundary(name, "AV re", potential)
                    potential = obj.AV_im.getValueAs("V")
                    self.write.boundary(name, "AV im", potential)
                else:
                    self.write.boundary(name, "AV", potential)
            if obj.EnableAV_1:
                potential = obj.AV_re_1.getValueAs("Wb/m")
                if equation.IsHarmonic:
                    self.write.boundary(name, "AV re {e} 1", potential)
                    potential = obj.AV_im_1.getValueAs("Wb/m")
                    self.write.boundary(name, "AV im {e} 1", potential)
                else:
                    self.write.boundary(name, "AV {e} 1", potential)
            if obj.EnableAV_2:
                potential = obj.AV_re_2.getValueAs("Wb/m")
                if equation.IsHarmonic:
                    self.write.boundary(name, "AV re {e} 2", potential)
                    potential = obj.AV_im_2.getValueAs("Wb/m")
                    self.write.boundary(name, "AV im {e} 2", potential)
                else:
                    self.write.boundary(name, "AV {e} 2", potential)
            if obj.EnableAV_3:
                potential = obj.AV_re_3.getValueAs("Wb/m")
                if equation.IsHarmonic:
                    self.write.boundary(name, "AV re {e} 3", potential)
                    potential = obj.AV_im_3.getValueAs("Wb/m")
                    self.write.boundary(name, "AV im {e} 3", potential)
                else:
                    self.write.boundary(name, "AV {e} 3", potential)

    def handleMagnetodynamicBndConditions(self, equation):
        # the current density can either be a body force or a boundary constraint
        # therefore only output here if a face is referenced
        currentDensities = self.write.getMember("Fem::ConstraintCurrentDensity")
        for obj in currentDensities:
            if obj.References:
                firstName = obj.References[0][1][0]
                firstName = firstName.rstrip("0123456789")
                if firstName == "Face":
                    for name in obj.References[0][1]:
                        self._outputMagnetodynamicBndConditions(obj, name, equation)
                    self.write.handled(obj)

        # the potential can either be a body force or a boundary constraint
        # therefore only output here if a face is referenced
        potentials = self.write.getMember("Fem::ConstraintElectrostaticPotential")
        for obj in potentials:
            if obj.References:
                firstName = obj.References[0][1][0]
                firstName = firstName.rstrip("0123456789")
                if firstName == "Face":
                    for name in obj.References[0][1]:
                        # output the FreeCAD label as comment
                        if obj.Label:
                            self.write.boundary(name, "! FreeCAD Name", obj.Label)
                        # output only if potential is enabled and needed
                        self._outputMagnetodynamicBndConditions(obj, name, equation)
                    self.write.handled(obj)


##  @}
