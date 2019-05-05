# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver Elmer writer"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess
import tempfile

from FreeCAD import Units
import Fem
import femtools.femutils as femutils
import femmesh.gmshtools as gmshtools
from .. import settings
from . import sifio


_STARTINFO_NAME = "ELMERSOLVER_STARTINFO"
_SIF_NAME = "case.sif"
_ELMERGRID_IFORMAT = "8"
_ELMERGRID_OFORMAT = "2"
_SOLID_PREFIX = "Solid"


UNITS = {
    "L": "mm",
    "M": "kg",
    "T": "s",
    "I": "A",
    "O": "K",
    "N": "mol",
    "J": "cd",
}


CONSTS_DEF = {
    "Gravity": "9.82 m/s^2",
    "StefanBoltzmann": "5.67e-8 W/(m^2*K^4)",
    "PermittivityOfVacuum": "8.8542e-12 s^4*A^2/(m*kg)",
    "BoltzmannConstant": "1.3807e-23 J/K",
}


def getFromUi(value, unit, outputDim):
    quantity = Units.Quantity(str(value) + str(unit))
    return convert(quantity, outputDim)


def convert(quantityStr, unit):
    quantity = Units.Quantity(quantityStr)
    for key, setting in UNITS.items():
        unit = unit.replace(key, setting)
    return float(quantity.getValueAs(unit))


def _getAllSubObjects(obj):
    s = ["Solid" + str(i + 1) for i in range(len(obj.Shape.Solids))]
    s.extend(("Face" + str(i + 1) for i in range(len(obj.Shape.Faces))))
    s.extend(("Edge" + str(i + 1) for i in range(len(obj.Shape.Edges))))
    s.extend(("Vertex" + str(i + 1) for i in range(len(obj.Shape.Vertexes))))
    return s


def getConstant(name, dimension):
    return convert(CONSTS_DEF[name], dimension)


class Writer(object):

    def __init__(self, solver, directory, testmode=False):
        self.analysis = femutils.findAnalysisOfMember(solver)
        self.solver = solver
        self.directory = directory
        self.testmode = testmode
        self._usedVarNames = set()
        self._builder = sifio.Builder()
        self._handledObjects = set()

    def getHandledConstraints(self):
        return self._handledObjects

    def write(self):
        self._handleSimulation()
        self._handleHeat()
        self._handleElasticity()
        self._handleElectrostatic()
        self._handleFluxsolver()
        self._handleFlow()
        self._addOutputSolver()

        self._writeSif()
        self._writeMesh()
        self._writeStartinfo()

    def _writeMesh(self):
        mesh = self._getSingleMember("Fem::FemMeshObject")
        unvPath = os.path.join(self.directory, "mesh.unv")
        groups = []
        groups.extend(self._builder.getBodyNames())
        groups.extend(self._builder.getBoundaryNames())
        self._exportToUnv(groups, mesh, unvPath)
        if self.testmode:
            print("We are in testmode ElmerGrid may not be installed!")
        else:
            binary = settings.get_binary("ElmerGrid")
            if binary is None:
                raise WriteError("Couldn't find ElmerGrid binary.")
            args = [binary,
                    _ELMERGRID_IFORMAT,
                    _ELMERGRID_OFORMAT,
                    unvPath,
                    "-out", self.directory]
            subprocess.call(args)

    def _writeStartinfo(self):
        path = os.path.join(self.directory, _STARTINFO_NAME)
        with open(path, 'w') as f:
            f.write(_SIF_NAME)

    def _exportToUnv(self, groups, mesh, meshPath):
        unvGmshFd, unvGmshPath = tempfile.mkstemp(suffix=".unv")
        brepFd, brepPath = tempfile.mkstemp(suffix=".brep")
        geoFd, geoPath = tempfile.mkstemp(suffix=".geo")
        os.close(brepFd)
        os.close(geoFd)
        os.close(unvGmshFd)

        tools = gmshtools.GmshTools(mesh)
        tools.group_elements = {g: [g] for g in groups}
        tools.group_nodes_export = False
        tools.ele_length_map = {}
        tools.temp_file_geometry = brepPath
        tools.temp_file_geo = geoPath
        tools.temp_file_mesh = unvGmshPath

        tools.get_dimension()
        tools.get_gmsh_command()
        tools.get_region_data()
        tools.get_boundary_layer_data()
        tools.write_part_file()
        tools.write_geo()
        if self.testmode:
            print("We are in testmode, GMSH may not be installed!")
            import shutil
            shutil.copyfile(geoPath, os.path.join(self.directory, "group_mesh.geo"))
        else:
            tools.run_gmsh_with_geo()

            ioMesh = Fem.FemMesh()
            ioMesh.read(unvGmshPath)
            ioMesh.write(meshPath)

        os.remove(brepPath)
        os.remove(geoPath)
        os.remove(unvGmshPath)

    def _handleSimulation(self):
        self._simulation("Coordinate System", "Cartesian 3D")
        self._simulation("Coordinate Mapping", (1, 2, 3))
        self._simulation("Simulation Type", "Steady state")
        self._simulation("Steady State Max Iterations", 1)
        self._simulation("Output Intervals", 1)
        self._simulation("Timestepping Method", "BDF")
        self._simulation("BDF Order", 1)
        self._simulation("Use Mesh Names", True)
        self._simulation(
            "Steady State Max Iterations",
            self.solver.SteadyStateMaxIterations)
        self._simulation(
            "Steady State Min Iterations",
            self.solver.SteadyStateMinIterations)

    def _handleHeat(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::FemEquationElmerHeat"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getHeatSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
        if activeIn:
            self._handleHeatConstants()
            self._handleHeatBndConditions()
            self._handleHeatInitial(activeIn)
            self._handleHeatBodyForces(activeIn)
            self._handleHeatMaterial(activeIn)

    def _getHeatSolver(self, equation):
        s = self._createNonlinearSolver(equation)
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("HeatSolve/HeatSolver")
        s["Variable"] = self._getUniqueVarName("Temperature")
        s["Exec Solver"] = "Always"
        s["Stabilize"] = equation.Stabilize
        s["Bubbles"] = equation.Bubbles
        s["Optimize Bandwidth"] = True
        return s

    def _handleHeatConstants(self):
        self._constant(
            "Stefan Boltzmann",
            getConstant("StefanBoltzmann", "M/(O^4*T^3)"))

    def _handleHeatBndConditions(self):
        for obj in self._getMember("Fem::ConstraintTemperature"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Temperature":
                        temp = getFromUi(obj.Temperature, "K", "O")
                        self._boundary(name, "Temperature", temp)
                    elif obj.ConstraintType == "CFlux":
                        flux = getFromUi(obj.CFlux, "kg*mm^2*s^-3", "M*L^2*T^-3")
                        self._boundary(name, "Temperature Load", flux)
                self._handled(obj)
        for obj in self._getMember("Fem::ConstraintHeatflux"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Convection":
                        film = getFromUi(obj.FilmCoef, "W/(m^2*K)", "M/(T^3*O)")
                        temp = getFromUi(obj.AmbientTemp, "K", "O")
                        self._boundary(name, "Heat Transfer Coefficient", film)
                        self._boundary(name, "External Temperature", temp)
                    elif obj.ConstraintType == "DFlux":
                        flux = getFromUi(obj.DFlux, "W/m^2", "M*T^-3")
                        self._boundary(name, "Heat Flux BC", True)
                        self._boundary(name, "Heat Flux", flux)
                self._handled(obj)

    def _handleHeatInitial(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if obj is not None:
            for name in bodies:
                temp = getFromUi(obj.initialTemperature, "K", "O")
                self._initial(name, "Temperature", temp)
            self._handled(obj)

    def _handleHeatBodyForces(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintBodyHeatSource")
        if obj is not None:
            for name in bodies:
                heatSource = getFromUi(obj.HeatSource, "W/kg", "L^2*T^-3")
                # according Elmer forum W/kg is correct, http://www.elmerfem.org/forum/viewtopic.php?f=7&t=1765
                # 1 watt = kg * m2 / s3 ... W/kg = m2 / s3
                self._bodyForce(name, "Heat Source", heatSource)
            self._handled(obj)

    def _handleHeatMaterial(self, bodies):
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = getFromUi(tempObj.initialTemperature, "K", "O")
            for name in bodies:
                self._material(name, "Reference Temperature", refTemp)
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies())
            for name in (n for n in refs if n in bodies):
                self._material(
                    name, "Density",
                    self._getDensity(m))
                self._material(
                    name, "Heat Conductivity",
                    convert(m["ThermalConductivity"], "M*L/(T^3*O)"))
                self._material(
                    name, "Heat Capacity",
                    convert(m["SpecificHeat"], "L^2/(T^2*O)"))

    def _handleElectrostatic(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::FemEquationElmerElectrostatic"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getElectrostaticSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
        if activeIn:
            self._handleElectrostaticConstants()
            self._handleElectrostaticBndConditions()
            # self._handleElectrostaticInitial(activeIn)
            # self._handleElectrostaticBodyForces(activeIn)
            self._handleElectrostaticMaterial(activeIn)

    def _getElectrostaticSolver(self, equation):
        s = self._createLinearSolver(equation)
        s["Equation"] = "Stat Elec Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("StatElecSolve/StatElecSolver")
        s["Variable"] = self._getUniqueVarName("Potential")
        s["Variable DOFs"] = 1
        s["Calculate Electric Field"] = equation.CalculateElectricField
        # s["Calculate Electric Flux"] = equation.CalculateElectricFlux
        s["Calculate Electric Energy"] = equation.CalculateElectricEnergy
        s["Calculate Surface Charge"] = equation.CalculateSurfaceCharge
        s["Displace mesh"] = False
        s["Exec Solver"] = "Always"
        s["Stabilize"] = equation.Stabilize
        s["Bubbles"] = equation.Bubbles
        s["Optimize Bandwidth"] = True
        return s

    def _handleElectrostaticConstants(self):
        self._constant(
            "Permittivity Of Vacuum",
            getConstant("PermittivityOfVacuum", "T^4*I^2/(L*M)"))

    def _handleElectrostaticMaterial(self, bodies):
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies())
            for name in (n for n in refs if n in bodies):
                if "RelativePermittivity" in m:
                    self._material(
                        name, "Relative Permittivity",
                        float(m["RelativePermittivity"]))

    def _handleElectrostaticBndConditions(self):
        for obj in self._getMember("Fem::ConstraintElectrostaticPotential"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.Potential:
                        potential = getFromUi(obj.Potential, "V", "M*L^2/(T^3 * I)")
                        self._boundary(name, "Potential", potential)
                    if obj.PotentialConstant:
                        self._boundary(name, "Potential Constant", True)
                self._handled(obj)

    def _handleFluxsolver(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::FemEquationElmerFluxsolver"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getFluxsolverSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)

    def _getFluxsolverSolver(self, equation):
        s = self._createLinearSolver(equation)
        s["Equation"] = "Flux Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("FluxSolver/FluxSolver")
        s["Flux Variable"] = equation.FluxVariable
        s["Calculate Flux"] = equation.CalculateFlux
        s["Calculate Grad"] = equation.CalculateGrad
        return s

    def _handleElasticity(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::FemEquationElmerElasticity"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getElasticitySolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
        if activeIn:
            self._handleElasticityConstants()
            self._handleElasticityBndConditions()
            self._handleElasticityInitial(activeIn)
            self._handleElasticityBodyForces(activeIn)
            self._handleElasticityMaterial(activeIn)

    def _getElasticitySolver(self, equation):
        s = self._createLinearSolver(equation)
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("StressSolve/StressSolver")
        s["Variable"] = self._getUniqueVarName("Displacement")
        s["Variable DOFs"] = 3
        s["Eigen Analysis"] = equation.DoFrequencyAnalysis
        s["Eigen System Values"] = equation.EigenmodesCount
        s["Calculate Strains"] = equation.CalculateStrains
        s["Calculate Stresses"] = equation.CalculateStresses
        s["Calculate Principal"] = equation.CalculatePrincipal
        s["Calculate Pangle"] = equation.CalculatePangle
        s["Displace mesh"] = False
        s["Exec Solver"] = "Always"
        s["Stabilize"] = equation.Stabilize
        s["Bubbles"] = equation.Bubbles
        s["Optimize Bandwidth"] = True
        return s

    def _handleElasticityConstants(self):
        pass

    def _handleElasticityBndConditions(self):
        for obj in self._getMember("Fem::ConstraintPressure"):
            if obj.References:
                for name in obj.References[0][1]:
                    pressure = getFromUi(obj.Pressure, "MPa", "M/(L*T^2)")
                    if not obj.Reversed:
                        pressure *= -1
                    self._boundary(name, "Normal Force", pressure)
                self._handled(obj)
        for obj in self._getMember("Fem::ConstraintFixed"):
            if obj.References:
                for name in obj.References[0][1]:
                    self._boundary(name, "Displacement 1", 0.0)
                    self._boundary(name, "Displacement 2", 0.0)
                    self._boundary(name, "Displacement 3", 0.0)
                self._handled(obj)
        for obj in self._getMember("Fem::ConstraintForce"):
            if obj.References:
                for name in obj.References[0][1]:
                    force = getFromUi(obj.Force, "N", "M*L*T^-2")
                    self._boundary(name, "Force 1", obj.DirectionVector.x * force)
                    self._boundary(name, "Force 2", obj.DirectionVector.y * force)
                    self._boundary(name, "Force 3", obj.DirectionVector.z * force)
                    self._boundary(name, "Force 1 Normalize by Area", True)
                    self._boundary(name, "Force 2 Normalize by Area", True)
                    self._boundary(name, "Force 3 Normalize by Area", True)
                self._handled(obj)
        for obj in self._getMember("Fem::ConstraintDisplacement"):
            if obj.References:
                for name in obj.References[0][1]:
                    if not obj.xFree:
                        self._boundary(
                            name, "Displacement 1", obj.xDisplacement * 0.001)
                    elif obj.xFix:
                        self._boundary(name, "Displacement 1", 0.0)
                    if not obj.yFree:
                        self._boundary(
                            name, "Displacement 2", obj.yDisplacement * 0.001)
                    elif obj.yFix:
                        self._boundary(name, "Displacement 2", 0.0)
                    if not obj.zFree:
                        self._boundary(
                            name, "Displacement 3", obj.zDisplacement * 0.001)
                    elif obj.zFix:
                        self._boundary(name, "Displacement 3", 0.0)
                self._handled(obj)

    def _handleElasticityInitial(self, bodies):
        pass

    def _handleElasticityBodyForces(self, bodies):
        obj = self._getSingleMember("FemConstraintSelfWeight")
        if obj is not None:
            for name in bodies:
                gravity = getConstant("Gravity", "L/T^2")
                m = self._getBodyMaterial(name).Material

                densityQuantity = Units.Quantity(m["Density"])
                dimension = "M/L^3"
                if name.startswith("Edge"):
                    density = None  # not tested, but it seems needed because denisty does not exist (IMHO, bernd)
                    if density:
                        density.Unit = Units.Unit(-2, 1)
                    dimension = "M/L^2"
                density = convert(densityQuantity, dimension)

                force1 = gravity * obj.Gravity_x * density
                force2 = gravity * obj.Gravity_y * density
                force3 = gravity * obj.Gravity_z * density
                self._bodyForce(name, "Stress Bodyforce 1", force1)
                self._bodyForce(name, "Stress Bodyforce 2", force2)
                self._bodyForce(name, "Stress Bodyforce 3", force3)
            self._handled(obj)

    def _getBodyMaterial(self, name):
        for obj in self._getMember("App::MaterialObject"):
            if not obj.References or name in obj.References[0][1]:
                return obj
        return None

    def _handleElasticityMaterial(self, bodies):
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = getFromUi(tempObj.initialTemperature, "K", "O")
            for name in bodies:
                self._material(name, "Reference Temperature", refTemp)
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies())
            for name in (n for n in refs if n in bodies):
                self._material(
                    name, "Density",
                    self._getDensity(m))
                self._material(
                    name, "Youngs Modulus",
                    self._getYoungsModulus(m))
                self._material(
                    name, "Poisson ratio",
                    float(m["PoissonRatio"]))
                self._material(
                    name, "Heat expansion Coefficient",
                    convert(m["ThermalExpansionCoefficient"], "O^-1"))

    def _getDensity(self, m):
        density = convert(m["Density"], "M/L^3")
        if self._getMeshDimension() == 2:
            density *= 1e3
        return density

    def _getYoungsModulus(self, m):
        youngsModulus = convert(m["YoungsModulus"], "M/(L*T^2)")
        if self._getMeshDimension() == 2:
            youngsModulus *= 1e3
        return youngsModulus

    def _handleFlow(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::FemEquationElmerFlow"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getFlowSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
        if activeIn:
            self._handleFlowConstants()
            self._handleFlowBndConditions()
            self._handleFlowInitialVelocity(activeIn)
            # self._handleFlowInitial(activeIn)
            # self._handleFlowBodyForces(activeIn)
            self._handleFlowMaterial(activeIn)
            self._handleFlowEquation(activeIn)

    def _getFlowSolver(self, equation):
        s = self._createNonlinearSolver(equation)
        s["Equation"] = "Navier-Stokes"
        # s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("FlowSolve/FlowSolver")
        s["Exec Solver"] = "Always"
        s["Stabilize"] = equation.Stabilize
        s["Bubbles"] = equation.Bubbles
        s["Optimize Bandwidth"] = True
        return s

    def _handleFlowConstants(self):
        gravity = getConstant("Gravity", "L/T^2")
        self._constant("Gravity", (0.0, -1.0, 0.0, gravity))

    def _handleFlowMaterial(self, bodies):
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = getFromUi(tempObj.initialTemperature, "K", "O")
            for name in bodies:
                self._material(name, "Reference Temperature", refTemp)
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies())
            for name in (n for n in refs if n in bodies):
                if "Density" in m:
                    self._material(
                        name, "Density",
                        self._getDensity(m))
                if "ThermalConductivity" in m:
                    self._material(
                        name, "Heat Conductivity",
                        convert(m["ThermalConductivity"], "M*L/(T^3*O)"))
                if "KinematicViscosity" in m:
                    density = self._getDensity(m)
                    kViscosity = convert(m["KinematicViscosity"], "L^2/T")
                    self._material(
                        name, "Viscosity", kViscosity * density)
                if "ThermalExpansionCoefficient" in m:
                    value = convert(m["ThermalExpansionCoefficient"], "O^-1")
                    if value > 0:
                        self._material(
                            name, "Heat expansion Coefficient", value)
                if "ReferencePressure" in m:
                    pressure = convert(m["ReferencePressure"], "M/(L*T^2)")
                    self._material(name, "Reference Pressure", pressure)
                if "SpecificHeatRatio" in m:
                    self._material(
                        name, "Specific Heat Ratio",
                        float(m["SpecificHeatRatio"]))
                if "CompressibilityModel" in m:
                    self._material(
                        name, "Compressibility Model",
                        m["CompressibilityModel"])

    def _handleFlowInitialVelocity(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintInitialFlowVelocity")
        if obj is not None:
            for name in bodies:
                if obj.VelocityXEnabled:
                    velocity = getFromUi(obj.VelocityX, "m/s", "L/T")
                    self._initial(name, "Velocity 1", velocity)
                if obj.VelocityYEnabled:
                    velocity = getFromUi(obj.VelocityY, "m/s", "L/T")
                    self._initial(name, "Velocity 2", velocity)
                if obj.VelocityZEnabled:
                    velocity = getFromUi(obj.VelocityZ, "m/s", "L/T")
                    self._initial(name, "Velocity 3", velocity)
            self._handled(obj)

    def _handleFlowBndConditions(self):
        for obj in self._getMember("Fem::ConstraintFlowVelocity"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.VelocityXEnabled:
                        velocity = getFromUi(obj.VelocityX, "m/s", "L/T")
                        self._boundary(name, "Velocity 1", velocity)
                    if obj.VelocityYEnabled:
                        velocity = getFromUi(obj.VelocityY, "m/s", "L/T")
                        self._boundary(name, "Velocity 2", velocity)
                    if obj.VelocityZEnabled:
                        velocity = getFromUi(obj.VelocityZ, "m/s", "L/T")
                        self._boundary(name, "Velocity 3", velocity)
                    if obj.NormalToBoundary:
                        self._boundary(name, "Normal-Tangential Velocity", True)
                self._handled(obj)

    def _handleFlowEquation(self, bodies):
        for b in bodies:
            self._equation(b, "Convection", "Computed")

    def _createLinearSolver(self, equation):
        s = sifio.createSection(sifio.SOLVER)
        s.priority = equation.Priority
        s["Linear System Solver"] = equation.LinearSolverType
        if equation.LinearSolverType == "Direct":
            s["Linear System Direct Method"] = \
                equation.LinearDirectMethod
        elif equation.LinearSolverType == "Iterative":
            s["Linear System Iterative Method"] = \
                equation.LinearIterativeMethod
            if equation.LinearIterativeMethod == "BiCGStabl":
                s["BiCGstabl polynomial degree"] = \
                    equation.BiCGstablDegree
            s["Linear System Max Iterations"] = \
                equation.LinearIterations
            s["Linear System Convergence Tolerance"] = \
                equation.LinearTolerance
            s["Linear System Preconditioning"] = \
                equation.LinearPreconditioning
        s["Steady State Convergence Tolerance"] = \
            equation.SteadyStateTolerance
        s["Linear System Abort Not Converged"] = False
        s["Linear System Residual Output"] = 1
        s["Linear System Precondition Recompute"] = 1
        return s

    def _createNonlinearSolver(self, equation):
        s = self._createLinearSolver(equation)
        s["Nonlinear System Max Iterations"] = \
            equation.NonlinearIterations
        s["Nonlinear System Convergence Tolerance"] = \
            equation.NonlinearTolerance
        s["Nonlinear System Relaxation Factor"] = \
            equation.RelaxationFactor
        s["Nonlinear System Newton After Iterations"] = \
            equation.NonlinearNewtonAfterIterations
        s["Nonlinear System Newton After Tolerance"] = \
            equation.NonlinearNewtonAfterTolerance
        return s

    def _getUniqueVarName(self, varName):
        postfix = 1
        if varName in self._usedVarNames:
            varName += "%2d" % postfix
        while varName in self._usedVarNames:
            postfix += 1
            varName = varName[:-2] + "%2d" % postfix
        self._usedVarNames.add(varName)
        return varName

    def _getAllBodies(self):
        obj = self._getSingleMember("Fem::FemMeshObject")
        bodyCount = 0
        prefix = ""
        if obj.Part.Shape.Solids:
            prefix = "Solid"
            bodyCount = len(obj.Part.Shape.Solids)
        elif obj.Part.Shape.Faces:
            prefix = "Face"
            bodyCount = len(obj.Part.Shape.Faces)
        elif obj.Part.Shape.Edges:
            prefix = "Edge"
            bodyCount = len(obj.Part.Shape.Edges)
        return [prefix + str(i + 1) for i in range(bodyCount)]

    def _getMeshDimension(self):
        obj = self._getSingleMember("Fem::FemMeshObject")
        if obj.Part.Shape.Solids:
            return 3
        elif obj.Part.Shape.Faces:
            return 2
        elif obj.Part.Shape.Edges:
            return 1
        return None

    def _addOutputSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        s["Equation"] = "ResultOutput"
        s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr("case")
        s["Vtu Format"] = True
        for name in self._getAllBodies():
            self._addSolver(name, s)

    def _writeSif(self):
        sifPath = os.path.join(self.directory, _SIF_NAME)
        with open(sifPath, 'w') as fstream:
            sif = sifio.Sif(self._builder)
            sif.write(fstream)

    def _handled(self, obj):
        self._handledObjects.add(obj)

    def _simulation(self, key, attr):
        self._builder.simulation(key, attr)

    def _constant(self, key, attr):
        self._builder.constant(key, attr)

    def _initial(self, body, key, attr):
        self._builder.initial(body, key, attr)

    def _material(self, body, key, attr):
        self._builder.material(body, key, attr)

    def _equation(self, body, key, attr):
        self._builder.equation(body, key, attr)

    def _bodyForce(self, body, key, attr):
        self._builder.bodyForce(body, key, attr)

    def _addSolver(self, body, solverSection):
        self._builder.addSolver(body, solverSection)

    def _boundary(self, boundary, key, attr):
        self._builder.boundary(boundary, key, attr)

    def _addSection(self, section):
        self._builder.addSection(section)

    def _getMember(self, t):
        return femutils.get_member(self.analysis, t)

    def _getSingleMember(self, t):
        return femutils.get_single_member(self.analysis, t)


class WriteError(Exception):
    pass

##  @}
