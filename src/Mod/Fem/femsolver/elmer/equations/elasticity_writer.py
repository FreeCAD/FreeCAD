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

__title__ = "FreeCAD FEM Elasticity Elmer writer"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from FreeCAD import Console
from FreeCAD import Units

from .. import sifio
from .. import writer as general_writer
from femtools import femutils
from . import elasticity


class ElasticityWriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getElasticitySolver(self, equation):
        s = self.write.createLinearSolver(equation)
        # check if we need to update the equation
        self._updateElasticitySolver(equation)
        # output the equation parameters
        s["Equation"] = "Stress Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("StressSolve/StressSolver")
        if equation.CalculateStrains is True:
            s["Calculate Strains"] = equation.CalculateStrains
        if equation.CalculateStresses is True:
            s["Calculate Stresses"] = equation.CalculateStresses
        if equation.CalculatePrincipal is True:
            s["Calculate Principal"] = equation.CalculatePrincipal
        if equation.CalculatePangle is True:
            s["Calculate Pangle"] = equation.CalculatePangle
        if equation.ConstantBulkSystem is True:
            s["Constant Bulk System"] = equation.ConstantBulkSystem
        s["Displace mesh"] = equation.DisplaceMesh
        s["Eigen Analysis"] = equation.EigenAnalysis
        if equation.EigenAnalysis is True:
            s["Eigen System Convergence Tolerance"] = \
                equation.EigenSystemTolerance
            s["Eigen System Complex"] = equation.EigenSystemComplex
            if equation.EigenSystemComputeResiduals is True:
                s["Eigen System Compute Residuals"] = equation.EigenSystemComputeResiduals
            s["Eigen System Damped"] = equation.EigenSystemDamped
            s["Eigen System Max Iterations"] = equation.EigenSystemMaxIterations
            s["Eigen System Select"] = equation.EigenSystemSelect
            s["Eigen System Values"] = equation.EigenSystemValues
        if equation.FixDisplacement is True:
            s["Fix Displacement"] = equation.FixDisplacement
        s["Geometric Stiffness"] = equation.GeometricStiffness
        if equation.Incompressible is True:
            s["Incompressible"] = equation.Incompressible
        if equation.MaxwellMaterial is True:
            s["Maxwell Material"] = equation.MaxwellMaterial
        if equation.ModelLumping is True:
            s["Model Lumping"] = equation.ModelLumping
        if equation.ModelLumping is True:
            s["Model Lumping Filename"] = equation.ModelLumpingFilename
        s["Optimize Bandwidth"] = True
        if equation.StabilityAnalysis is True:
            s["Stability Analysis"] = equation.StabilityAnalysis
        s["Stabilize"] = equation.Stabilize
        if equation.UpdateTransientSystem is True:
            s["Update Transient System"] = equation.UpdateTransientSystem
        s["Variable"] = equation.Variable
        s["Variable DOFs"] = 3
        return s

    def handleElasticityEquation(self, bodies, equation):
        for b in bodies:
            # not for bodies with fluid material
            if not self.write.isBodyMaterialFluid(b):
                if equation.PlaneStress:
                    self.write.equation(b, "Plane Stress", equation.PlaneStress)

    def _updateElasticitySolver(self, equation):
        # updates older Elasticity equations
        if not hasattr(equation, "Variable"):
            equation.addProperty(
                "App::PropertyString",
                "Variable",
                "Elasticity",
                (
                    "Only change this if 'Incompressible' is set to true\n"
                    "according to the Elmer manual."
                )
            )
            equation.Variable = "Displacement"
        if hasattr(equation, "Bubbles"):
            # Bubbles was removed because it is unused by Elmer for the stress solver
            equation.removeProperty("Bubbles")
        if not hasattr(equation, "ConstantBulkSystem"):
            equation.addProperty(
                "App::PropertyBool",
                "ConstantBulkSystem",
                "Elasticity",
                "See Elmer manual for info"
            )
        if not hasattr(equation, "DisplaceMesh"):
            equation.addProperty(
                "App::PropertyBool",
                "DisplaceMesh",
                "Elasticity",
                (
                    "If mesh is deformed by displacement field.\n"
                    "Set to False for 'Eigen Analysis'."
                )
            )
            # DisplaceMesh is true except if DoFrequencyAnalysis is true
            equation.DisplaceMesh = True
            if hasattr(equation, "DoFrequencyAnalysis"):
                if equation.DoFrequencyAnalysis is True:
                    equation.DisplaceMesh = False
        if not hasattr(equation, "EigenAnalysis"):
            # DoFrequencyAnalysis was renamed to EigenAnalysis
            # to follow the Elmer manual
            equation.addProperty(
                "App::PropertyBool",
                "EigenAnalysis",
                "Eigen Values",
                "If true, modal analysis"
            )
            if hasattr(equation, "DoFrequencyAnalysis"):
                equation.EigenAnalysis = equation.DoFrequencyAnalysis
                equation.removeProperty("DoFrequencyAnalysis")
        if not hasattr(equation, "EigenSystemComplex"):
            equation.addProperty(
                "App::PropertyBool",
                "EigenSystemComplex",
                "Eigen Values",
                (
                    "Should be true if eigen system is complex\n"
                    "Must be false for a damped eigen value analysis."
                )
            )
            equation.EigenSystemComplex = True
        if not hasattr(equation, "EigenSystemComputeResiduals"):
            equation.addProperty(
                "App::PropertyBool",
                "EigenSystemComputeResiduals",
                "Eigen Values",
                "Computes residuals of eigen value system"
            )
        if not hasattr(equation, "EigenSystemDamped"):
            equation.addProperty(
                "App::PropertyBool",
                "EigenSystemDamped",
                "Eigen Values",
                (
                    "Set a damped eigen analysis. Can only be\n"
                    "used if 'Linear Solver Type' is 'Iterative'."
                )
            )
        if not hasattr(equation, "EigenSystemMaxIterations"):
            equation.addProperty(
                "App::PropertyIntegerConstraint",
                "EigenSystemMaxIterations",
                "Eigen Values",
                "Max iterations for iterative eigensystem solver"
            )
            equation.EigenSystemMaxIterations = (300, 1, int(1e8), 1)
        if not hasattr(equation, "EigenSystemSelect"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "EigenSystemSelect",
                "Eigen Values",
                "Which eigenvalues are computed"
            )
            equation.EigenSystemSelect = elasticity.EIGEN_SYSTEM_SELECT
            equation.EigenSystemSelect = "Smallest Magnitude"
        if not hasattr(equation, "EigenSystemTolerance"):
            equation.addProperty(
                "App::PropertyFloat",
                "EigenSystemTolerance",
                "Eigen Values",
                (
                    "Convergence tolerance for iterative eigensystem solve\n"
                    "Default is 100 times the 'Linear Tolerance'"
                )
            )
            equation.setExpression("EigenSystemTolerance", str(100 * equation.LinearTolerance))
        if not hasattr(equation, "EigenSystemValues"):
            # EigenmodesCount was renamed to EigenSystemValues
            # to follow the Elmer manual
            equation.addProperty(
                "App::PropertyInteger",
                "EigenSystemValues",
                "Eigen Values",
                "Number of lowest eigen modes"
            )
            if hasattr(equation, "EigenmodesCount"):
                equation.EigenSystemValues = equation.EigenmodesCount
                equation.removeProperty("EigenmodesCount")
        if not hasattr(equation, "FixDisplacement"):
            equation.addProperty(
                "App::PropertyBool",
                "FixDisplacement",
                "Elasticity",
                "If displacements or forces are set,\nthereby model lumping is used"
            )
        if not hasattr(equation, "GeometricStiffness"):
            equation.addProperty(
                "App::PropertyBool",
                "GeometricStiffness",
                "Elasticity",
                "Consider geometric stiffness"
            )
        if not hasattr(equation, "Incompressible"):
            equation.addProperty(
                "App::PropertyBool",
                "Incompressible",
                "Elasticity",
                (
                    "Computation of incompressible material in connection\n"
                    "with viscoelastic Maxwell material and a custom 'Variable'"
                )
            )
        if not hasattr(equation, "MaxwellMaterial"):
            equation.addProperty(
                "App::PropertyBool",
                "MaxwellMaterial",
                "Elasticity",
                "Compute viscoelastic material model"
            )
        if not hasattr(equation, "ModelLumping"):
            equation.addProperty(
                "App::PropertyBool",
                "ModelLumping",
                "Elasticity",
                "Use model lumping"
            )
        if not hasattr(equation, "ModelLumpingFilename"):
            equation.addProperty(
                "App::PropertyFile",
                "ModelLumpingFilename",
                "Elasticity",
                "File to save results from model lumping to"
            )
        if not hasattr(equation, "PlaneStress"):
            equation.addProperty(
                "App::PropertyBool",
                "PlaneStress",
                "Equation",
                (
                    "Computes solution according to plane\nstress situation.\n"
                    "Applies only for 2D geometry."
                )
            )
        if not hasattr(equation, "StabilityAnalysis"):
            equation.addProperty(
                "App::PropertyBool",
                "StabilityAnalysis",
                "Elasticity",
                (
                    "If true, 'Eigen Analysis' is stability analysis.\n"
                    "Otherwise modal analysis is performed."
                )
            )
        if not hasattr(equation, "UpdateTransientSystem"):
            equation.addProperty(
                "App::PropertyBool",
                "UpdateTransientSystem",
                "Elasticity",
                "See Elmer manual for info"
            )

    def handleElasticityConstants(self):
        pass

    def handleElasticityBndConditions(self):
        for obj in self.write.getMember("Fem::ConstraintPressure"):
            if obj.References:
                for name in obj.References[0][1]:
                    pressure = float(obj.Pressure.getValueAs("Pa"))
                    if not obj.Reversed:
                        pressure *= -1
                    self.write.boundary(name, "Normal Force", pressure)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintFixed"):
            if obj.References:
                for name in obj.References[0][1]:
                    self.write.boundary(name, "Displacement 1", 0.0)
                    self.write.boundary(name, "Displacement 2", 0.0)
                    self.write.boundary(name, "Displacement 3", 0.0)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintForce"):
            if obj.References:
                for name in obj.References[0][1]:
                    force = float(obj.Force.getValueAs("N"))
                    self.write.boundary(name, "Force 1", obj.DirectionVector.x * force)
                    self.write.boundary(name, "Force 2", obj.DirectionVector.y * force)
                    self.write.boundary(name, "Force 3", obj.DirectionVector.z * force)
                    self.write.boundary(name, "Force 1 Normalize by Area", True)
                    self.write.boundary(name, "Force 2 Normalize by Area", True)
                    self.write.boundary(name, "Force 3 Normalize by Area", True)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintDisplacement"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.useFlowSurfaceForce:
                        self.write.boundary(name, "FSI BC", obj.useFlowSurfaceForce)
                        # if useFlowSurfaceForce no displacements must be output
                        continue
                    if not obj.xFree:
                        if not obj.hasXFormula:
                            displacement = float(obj.xDisplacement.getValueAs("m"))
                        else:
                            displacement = obj.xDisplacementFormula
                        self.write.boundary(name, "Displacement 1", displacement)
                    if not obj.yFree:
                        if not obj.hasYFormula:
                            displacement = float(obj.yDisplacement.getValueAs("m"))
                        else:
                            displacement = obj.yDisplacementFormula
                        self.write.boundary(name, "Displacement 2", displacement)
                    if not obj.zFree:
                        if not obj.hasZFormula:
                            displacement = float(obj.zDisplacement.getValueAs("m"))
                        else:
                            displacement = obj.zDisplacementFormula
                        self.write.boundary(name, "Displacement 3", displacement)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintSpring"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ElmerStiffness == "Normal Stiffness":
                        spring = float(obj.NormalStiffness.getValueAs("N/m"))
                    else:
                        spring = float(obj.TangentialStiffness.getValueAs("N/m"))
                    self.write.boundary(name, "Spring", spring)
                self.write.handled(obj)

    def handleElasticityInitial(self, bodies):
        pass

    def handleElasticityBodyForces(self, bodies):
        obj = self.write.getSingleMember("Fem::ConstraintSelfWeight")
        if obj is not None:
            for name in bodies:
                gravity = self.write.convert(self.write.constsdef["Gravity"], "L/T^2")
                if self.write.getBodyMaterial(name) is None:
                    raise general_writer.WriteError(
                        "The body {} is not referenced in any material.\n\n".format(name)
                    )
                m = self.write.getBodyMaterial(name).Material

                densityQuantity = Units.Quantity(m["Density"])
                dimension = "M/L^3"
                if name.startswith("Edge"):
                    # not tested, bernd
                    # TODO: test
                    densityQuantity.Unit = Units.Unit(-2, 1)
                    dimension = "M/L^2"
                density = self.write.convert(densityQuantity, dimension)

                force1 = gravity * obj.Gravity_x * density
                force2 = gravity * obj.Gravity_y * density
                force3 = gravity * obj.Gravity_z * density
                self.write.bodyForce(name, "Stress Bodyforce 1", force1)
                self.write.bodyForce(name, "Stress Bodyforce 2", force2)
                self.write.bodyForce(name, "Stress Bodyforce 3", force3)
            self.write.handled(obj)

    def handleElasticityMaterial(self, bodies):
        # density
        # is needed for self weight constraints and frequency analysis
        density_needed = False
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElasticity"):
                if equation.EigenAnalysis is True:
                    density_needed = True
                    break  # there could be a second equation without frequency
        gravObj = self.write.getSingleMember("Fem::ConstraintSelfWeight")
        if gravObj is not None:
            density_needed = True
        # temperature
        tempObj = self.write.getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = float(tempObj.initialTemperature.getValueAs("K"))
            for name in bodies:
                self.write.material(name, "Reference Temperature", refTemp)
        # get the material data for all bodies
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self.write.getAllBodies()
            )
            for name in (n for n in refs if n in bodies):
                # don't evaluate fluid material
                if self.write.isBodyMaterialFluid(name):
                    break
                if "YoungsModulus" not in m:
                    Console.PrintMessage("m: {}\n".format(m))
                    # it is no fluid but also no solid
                    # -> user set no material reference at all
                    # that now material is known
                    raise general_writer.WriteError(
                        "There are two or more materials with empty references.\n\n"
                        "Set for the materials to what solid they belong to.\n"
                    )
                self.write.material(name, "Name", m["Name"])
                if density_needed is True:
                    self.write.material(
                        name, "Density",
                        self.write.getDensity(m)
                    )
                self.write.material(
                    name, "Youngs Modulus",
                    self._getYoungsModulus(m)
                )
                self.write.material(
                    name, "Poisson ratio",
                    float(m["PoissonRatio"])
                )
                if tempObj:
                    self.write.material(
                        name, "Heat expansion Coefficient",
                        self.write.convert(m["ThermalExpansionCoefficient"], "O^-1")
                    )

    def _getYoungsModulus(self, m):
        youngsModulus = self.write.convert(m["YoungsModulus"], "M/(L*T^2)")
        if self.write.getMeshDimension() == 2:
            youngsModulus *= 1e3
        return youngsModulus

##  @}
