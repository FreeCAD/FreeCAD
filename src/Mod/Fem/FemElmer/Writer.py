# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "FemWriterElmer"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import os
import os.path
import subprocess
import tempfile

import Units
import Fem
import FemMisc
import FemSettings
import FemGmshTools
import sifio


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
    for key, setting in UNITS.iteritems():
        unit = unit.replace(key, setting)
    return float(quantity.getValueAs(unit))


def _getAllSubObjects(obj):
    s = ["Solid" + str(i+1) for i in range(len(obj.Shape.Solids))]
    s.extend(("Face" + str(i+1) for i in range(len(obj.Shape.Faces))))
    s.extend(("Edge" + str(i+1) for i in range(len(obj.Shape.Edges))))
    s.extend(("Vertex" + str(i+1) for i in range(len(obj.Shape.Vertexes))))
    return s


def getConstant(name, dimension):
    return convert(CONSTS_DEF[name], dimension)


class Writer(object):

    def __init__(self, solver, directory):
        self.analysis = FemMisc.findAnalysisOfMember(solver)
        self.solver = solver
        self.directory = directory
        self._usedVarNames = set()
        self._builder = sifio.Builder()
        self._handledObjects = set()

    def write(self):
        self._handleSimulation()
        self._handleHeat()
        self._handleElasticity()
        self._handleFlow()
        self._addOutputSolver()

        self._writeSif()
        self._writeMesh()
        self._writeStartinfo()

    def _writeMesh(self):
        mesh = FemMisc.getSingleMember(self.analysis, "Fem::FemMeshObject")
        unvPath = os.path.join(self.directory, "mesh.unv")
        groups = []
        groups.extend(self._builder.getBodyNames())
        groups.extend(self._builder.getBoundaryNames())
        self._exportToUnv(groups, mesh, unvPath)
        args = [FemSettings.getBinary("ElmerGrid"),
                _ELMERGRID_IFORMAT,
                _ELMERGRID_OFORMAT,
                unvPath,
                "-autoclean",
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

        tools = FemGmshTools.FemGmshTools(mesh)
        tools.group_elements = {g: [g] for g in groups}
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
            if FemMisc.isOfType(equation, "Fem::FemEquationElmerHeat"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllSolids()
                for body in activeIn:
                    solverSection = self._getHeatSolver(equation)
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
        s["Stabilize"] = True
        s["Bubbles"] = False
        s["Optimize Bandwidth"] = True
        return s


    def _handleHeatConstants(self):
        self._constant(
            "Stefan Boltzmann",
            getConstant("StefanBoltzmann", "M/(O^4*T^3)"))

    def _handleHeatBndConditions(self):
        for obj in self._getMember("Fem::ConstraintTemperature"):
            for name in obj.References[0][1]:
                temp = getFromUi(obj.Temperature, "K", "O")
                self._boundary(name, "Temperature", temp)
            self._handled(obj)

    def _handleHeatInitial(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if obj is not None:
            for name in bodies:
                temp = getFromUi(obj.initialTemperature, "K", "O")
                self._initial(name, "Temperature", temp)
            self._handled(obj)

    def _handleHeatBodyForces(self, bodies):
        obj = self._getSingleMember("Fem::FemConstraintBodyHeatFlux")
        if obj is not None:
            for name in bodies:
                heatSource = getFromUi(obj.HeatFlux, "W/kg", "L^2*T^-3")
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
                else self._getAllSolids())
            for name in (n for n in refs if n in bodies):
                self._material(
                    name, "Density",
                    convert(m["Density"], "M/L^3"))
                self._material(
                    name, "Heat Conductivity",
                    convert(m["ThermalConductivity"], "M*L/(T^3*O)"))

    def _handleElasticity(self):
        activeIn = []
        for equation in self.solver.Group:
            if FemMisc.isOfType(equation, "Fem::FemEquationElmerElasticity"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllSolids()
                for body in activeIn:
                    solverSection = self._getElasticitySolver(equation)
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
        s["Calculate Pricipal"] = equation.CalculatePricipal
        s["Calculate Pangle"] = equation.CalculatePangle
        s["Displace mesh"] = False
        s["Exec Solver"] = "Always"
        s["Stabilize"] = True
        s["Bubbles"] = False
        s["Optimize Bandwidth"] = True
        return s

    def _handleElasticityConstants(self):
        pass

    def _handleElasticityBndConditions(self):
        for obj in self._getMember("Fem::ConstraintFixed"):
            for name in obj.References[0][1]:
                self._boundary(name, "Displacement 1", 0.0)
                self._boundary(name, "Displacement 2", 0.0)
                self._boundary(name, "Displacement 3", 0.0)
            self._handled(obj)
        for obj in self._getMember("Fem::ConstraintForce"):
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
                mat = self._getBodyMaterial(name).Material
                density = convert(mat["Density"], "M/L^3")
                force1 = gravity * obj.Gravity_x * density
                force2 = gravity * obj.Gravity_y * density
                force3 = gravity * obj.Gravity_z * density
                self._bodyForce(name, "Stress Bodyforce 1", force1)
                self._bodyForce(name, "Stress Bodyforce 2", force2)
                self._bodyForce(name, "Stress Bodyforce 3", force3)
            self._handled(obj)

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
                else self._getAllSolids())
            for name in (n for n in refs if n in bodies):
                self._material(
                    name, "Density",
                    convert(m["Density"], "M/L^3"))
                self._material(
                    name, "Youngs Modulus",
                    convert(m["YoungsModulus"], "M/(L*T^2)"))
                self._material(
                    name, "Poisson ratio",
                    float(m["PoissonRatio"]))
                self._material(
                    name, "Heat expansion Coefficient",
                    convert(m["ThermalExpansionCoefficient"], "O^-1"))


    def _handleFlow(self):
        activeIn = []
        for equation in self.solver.Group:
            if FemMisc.isOfType(equation, "Fem::FemEquationElmerFlow"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllSolids()
                for body in activeIn:
                    solverSection = self._getFlowSolver(equation)
                    self._addSolver(body, solverSection)
        if activeIn:
            self._handleFlowConstants()
            #self._handleFlowBndConditions()
            #self._handleFlowInitial(activeIn)
            #self._handleFlowBodyForces(activeIn)
            self._handleFlowMaterial(activeIn)

    def _getFlowSolver(self, equation):
        s = self._createNonlinearSolver(equation)
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("FlowSolve/FlowSolver")
        s["Exec Solver"] = "Always"
        s["Stabilize"] = True
        s["Bubbles"] = False
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
                else self._getAllSolids())
            for name in (n for n in refs if n in bodies):
                if "Density" in m:
                    self._material(
                        name, "Density",
                        convert(m["Density"], "M/L^3"))
                if "ThermalConductivity" in m:
                    self._material(
                        name, "Heat Conductivity",
                        convert(m["ThermalConductivity"], "M*L/(T^3*O)"))
                if "KinematicViscosity" in m:
                    self._material(
                        name, "Viscosity",
                        convert(m["KinematicViscosity"], "M/(L*T)"))
                if "ThermalExpansionCoefficient" in m:
                    self._material(
                        name, "Heat expansion Coefficient",
                        convert(m["ThermalExpansionCoefficient"], "O^-1"))

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

    def _getAllSolids(self):
        obj = FemMisc.getSingleMember(self.analysis, "Fem::FemMeshObject")
        return ["Solid" + str(i+1) for i in range(len(obj.Part.Shape.Solids))]

    def _addOutputSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        s["Equation"] = "ResultOutput"
        s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr("case")
        s["Vtu Format"] = True
        for name in self._getAllSolids():
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

    def _material(self, body, key , attr):
        self._builder.material(body, key, attr)

    def _equation(self, body, key , attr):
        self._builder.equation(body, key, attr)

    def _bodyForce(self, body, key , attr):
        self._builder.bodyForce(body, key, attr)

    def _addSolver(self, body, solverSection):
        self._builder.addSolver(body, solverSection)
        
    def _boundary(self, boundary, key, attr):
        self._builder.boundary(boundary, key, attr)

    def _addSection(self, section):
        self._builder.addSection(section)

    def _getMember(self, t):
        return FemMisc.getMember(self.analysis, t)

    def _getSingleMember(self, t):
        return FemMisc.getSingleMember(self.analysis, t)
