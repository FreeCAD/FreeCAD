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

__title__ = "FreeCAD FEM Flow Elmer writer"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from .. import sifio
from .. import writer as general_writer
from . import flow


class Flowwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getFlowSolver(self, equation):
        # check if we need to update the equation
        self._updateFlowSolver(equation)
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        s["Equation"] = "Navier-Stokes"
        s["Procedure"] = sifio.FileAttr("FlowSolve/FlowSolver")
        if equation.DivDiscretization is True:
            s["Div Discretization"] = equation.DivDiscretization
        s["Exec Solver"] = "Always"
        if equation.FlowModel != "Full":
            s["Flow Model"] = equation.FlowModel
        if equation.GradpDiscretization is True:
            s["Gradp Discretization"] = equation.GradpDiscretization
        s["Stabilize"] = equation.Stabilize
        s["Optimize Bandwidth"] = True
        if equation.Variable != "Flow Solution[Velocity:3 Pressure:1]":
            s["Variable"] = equation.Variable
        return s

    def handleFlowConstants(self):
        gravity = self.write.convert(self.write.constsdef["Gravity"], "L/T^2")
        self.write.constant("Gravity", (0.0, -1.0, 0.0, gravity))

    def _updateFlowSolver(self, equation):
        # updates older Flow equations
        if not hasattr(equation, "Convection"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "Convection",
                "Equation",
                "Type of convection to be used"
            )
            equation.Convection = flow.CONVECTION_TYPE
            equation.Convection = "Computed"
        if not hasattr(equation, "DivDiscretization"):
            equation.addProperty(
                "App::PropertyBool",
                "DivDiscretization",
                "Flow",
                (
                    "Set to true for incompressible flow for more stable\n"
                    "discretization when Reynolds number increases"
                )
            )
        if not hasattr(equation, "FlowModel"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "FlowModel",
                "Flow",
                "Flow model to be used"
            )
            equation.FlowModel = flow.FLOW_MODEL
            equation.FlowModel = "Full"
        if not hasattr(equation, "GradpDiscretization"):
            equation.addProperty(
                "App::PropertyBool",
                "GradpDiscretization",
                "Flow",
                (
                    "If true pressure Dirichlet boundary conditions can be used.\n"
                    "Also mass flux is available as a natural boundary condition."
                )
            )
        if not hasattr(equation, "MagneticInduction"):
            equation.addProperty(
                "App::PropertyBool",
                "MagneticInduction",
                "Equation",
                (
                    "Magnetic induction equation will be solved\n"
                    "along with the Navier-Stokes equations"
                )
            )
        if not hasattr(equation, "Variable"):
            equation.addProperty(
                "App::PropertyString",
                "Variable",
                "Flow",
                "Only for a 2D model change the '3' to '2'"
            )
            equation.Variable = "Flow Solution[Velocity:3 Pressure:1]"

    def handleFlowMaterial(self, bodies):
        tempObj = self.write.getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = float(tempObj.initialTemperature.getValueAs("K"))
            for name in bodies:
                self.write.material(name, "Reference Temperature", refTemp)
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self.write.getAllBodies())
            for name in (n for n in refs if n in bodies):
                self.write.material(name, "Name", m["Name"])
                if "Density" in m:
                    density = self.write.convert(m["Density"], "M/L^3")
                    self.write.material(name, "Density", density)
                if "ThermalConductivity" in m:
                    tConductivity = self.write.convert(m["ThermalConductivity"], "M*L/(T^3*O)")
                    self.write.material(name, "Heat Conductivity", tConductivity)
                if "DynamicViscosity" in m:
                    dViscosity = self.write.convert(m["DynamicViscosity"], "M/(L*T)")
                    self.write.material(name, "Viscosity", dViscosity)
                elif ("KinematicViscosity" in m) and ("Density" in m):
                    density = self.write.convert(m["Density"], "M/L^3")
                    kViscosity = self.write.convert(m["KinematicViscosity"], "L^2/T")
                    self.write.material(name, "Viscosity", kViscosity * density)
                if "ThermalExpansionCoefficient" in m:
                    value = self.write.convert(m["ThermalExpansionCoefficient"], "O^-1")
                    if value > 0:
                        self.write.material(name, "Heat expansion Coefficient", value)
                if "ReferencePressure" in m:
                    pressure = self.write.convert(m["ReferencePressure"], "M/(L*T^2)")
                    self.write.material(name, "Reference Pressure", pressure)
                if "SpecificHeatRatio" in m:
                    self.write.material(name, "Specific Heat Ratio", float(m["SpecificHeatRatio"]))
                if "CompressibilityModel" in m:
                    self.write.material(
                        name, "Compressibility Model",
                        m["CompressibilityModel"])

    def _outputInitialPressure(self, obj, name):
        # initial pressure only makes sense for fluid material
        if self.write.isBodyMaterialFluid(name):
            pressure = float(obj.Pressure.getValueAs("Pa"))
            self.write.initial(name, "Pressure", pressure)

    def handleFlowInitialPressure(self, bodies):
        initialPressures = self.write.getMember("Fem::ConstraintInitialPressure")
        for obj in initialPressures:
            if obj.References:
                for name in obj.References[0][1]:
                    self._outputInitialPressure(obj, name)
                self.write.handled(obj)
            else:
                # if there is only one initial pressure without a reference
                # add it to all fluid bodies
                if len(initialPressures) == 1:
                    for name in bodies:
                        self._outputInitialPressure(obj, name)
                else:
                    raise general_writer.WriteError(
                        "Several initial pressures found without reference to a body.\n"
                        "Please set a body for each initial pressure."
                    )
            self.write.handled(obj)

    def _outputInitialVelocity(self, obj, name):
        # flow only makes sense for fluid material
        if self.write.isBodyMaterialFluid(name):
            if not obj.VelocityXUnspecified:
                if not obj.VelocityXHasFormula:
                    velocity = float(obj.VelocityX.getValueAs("m/s"))
                else:
                    velocity = obj.VelocityXFormula
                self.write.initial(name, "Velocity 1", velocity)
            if not obj.VelocityYUnspecified:
                if not obj.VelocityYHasFormula:
                    velocity = float(obj.VelocityY.getValueAs("m/s"))
                else:
                    velocity = obj.VelocityYFormula
                self.write.initial(name, "Velocity 2", velocity)
            if not obj.VelocityZUnspecified:
                if not obj.VelocityZHasFormula:
                    velocity = float(obj.VelocityZ.getValueAs("m/s"))
                else:
                    velocity = obj.VelocityZFormula
                self.write.initial(name, "Velocity 3", velocity)

    def handleFlowInitialVelocity(self, bodies):
        initialVelocities = self.write.getMember("Fem::ConstraintInitialFlowVelocity")
        for obj in initialVelocities:
            if obj.References:
                for name in obj.References[0][1]:
                    self._outputInitialVelocity(obj, name)
                self.write.handled(obj)
            else:
                # if there is only one initial velocity without a reference
                # add it to all fluid bodies
                if len(initialVelocities) == 1:
                    for name in bodies:
                        self._outputInitialVelocity(obj, name)
                else:
                    raise general_writer.WriteError(
                        "Several initial velocities found without reference to a body.\n"
                        "Please set a body for each initial velocity."
                    )
            self.write.handled(obj)

    def handleFlowBndConditions(self):
        for obj in self.write.getMember("Fem::ConstraintFlowVelocity"):
            if obj.References:
                for name in obj.References[0][1]:
                    if not obj.VelocityXUnspecified:
                        if not obj.VelocityXHasFormula:
                            velocity = float(obj.VelocityX.getValueAs("m/s"))
                        else:
                            velocity = obj.VelocityXFormula
                        self.write.boundary(name, "Velocity 1", velocity)
                    if not obj.VelocityYUnspecified:
                        if not obj.VelocityYHasFormula:
                            velocity = float(obj.VelocityY.getValueAs("m/s"))
                        else:
                            velocity = obj.VelocityYFormula
                        self.write.boundary(name, "Velocity 2", velocity)
                    if not obj.VelocityZUnspecified:
                        if not obj.VelocityZHasFormula:
                            velocity = float(obj.VelocityZ.getValueAs("m/s"))
                        else:
                            velocity = obj.VelocityZFormula
                        self.write.boundary(name, "Velocity 3", velocity)
                    if obj.NormalToBoundary:
                        self.write.boundary(name, "Normal-Tangential Velocity", True)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintPressure"):
            if obj.References:
                for name in obj.References[0][1]:
                    pressure = float(obj.Pressure.getValueAs("Pa"))
                    if obj.Reversed:
                        pressure *= -1
                    self.write.boundary(name, "External Pressure", pressure)
                self.write.handled(obj)

    def handleFlowEquation(self, bodies, equation):
        for b in bodies:
            # not for bodies with solid material
            if self.write.isBodyMaterialFluid(b):
                if equation.Convection != "None":
                    self.write.equation(b, "Convection", equation.Convection)
                if equation.MagneticInduction is True:
                    self.write.equation(b, "Magnetic Induction", equation.MagneticInduction)

##  @}
