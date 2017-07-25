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
import FemDefsElmer
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


SUPPORTED = [
        ("Fem::ConstraintFixed",),
        ("Fem::ConstraintForce",),
        ("Fem::ConstraintDisplacement",),
        ("Fem::ConstraintTemperature",),
        ("Fem::ConstraintSelfWeight",),
        ("Fem::ConstraintInitialTemperature",),
        ("Fem::FeaturePython", "FemConstraintSelfWeight",),
]


def getFromUi(value, unit, outputDim):
    quantity = Units.Quantity(str(value) + str(unit))
    return convert(quantity, outputDim)


def convert(quantityStr, unit):
    quantity = Units.Quantity(quantityStr)
    for key, setting in UNITS.iteritems():
        unit = unit.replace(key, setting)
    return float(quantity.getValueAs(unit))


class Writer(object):

    # The first parameter defines the input file format:
    #   1)  .grd      : Elmergrid file format
    #   2)  .mesh.*   : Elmer input format
    #   3)  .ep       : Elmer output format
    #   4)  .ansys    : Ansys input format
    #   5)  .inp      : Abaqus input format by Ideas
    #   6)  .fil      : Abaqus output format
    #   7)  .FDNEUT   : Gambit (Fidap) neutral file
    #   8)  .unv      : Universal mesh file format
    #   9)  .mphtxt   : Comsol Multiphysics mesh format
    #   10) .dat      : Fieldview format
    #   11) .node,.ele: Triangle 2D mesh format
    #   12) .mesh     : Medit mesh format
    #   13) .msh      : GID mesh format
    #   14) .msh      : Gmsh mesh format
    #   15) .ep.i     : Partitioned ElmerPost format
    #
    # The second parameter defines the output file format:
    #   1)  .grd      : ElmerGrid file format
    #   2)  .mesh.*   : ElmerSolver format (also partitioned .part format)
    #   3)  .ep       : ElmerPost format
    #   4)  .msh      : Gmsh mesh format
    #   5)  .vtu      : VTK ascii XML format
    #
    # The third parameter is the name of the input file.
    # If the file does not exist, an example with the same name is created.
    # The default output file name is the same with a different suffix.

    def __init__(self, analysis, solver, directory):
        self.analysis = analysis
        self.solver = solver
        self.directory = directory
        self._groups = set()
        self._bndSections = dict()

    def writeInputFiles(self, report):
        self._writeSif()
        self._writeStartinfo()
        self._writeMesh()

    def _writeStartinfo(self):
        startinfo_path = os.path.join(
                self.directory, _STARTINFO_NAME)
        with open(startinfo_path, 'w') as f:
            f.write(_SIF_NAME)

    def _writeMesh(self):
        mesh = FemMisc.getSingleMember(self.analysis, "Fem::FemMeshObject")
        unvPath = os.path.join(self.directory, "mesh.unv")
        self._exportToUnv(mesh, unvPath)
        args = [FemSettings.getBinary("ElmerGrid"),
                _ELMERGRID_IFORMAT,
                _ELMERGRID_OFORMAT,
                unvPath,
                "-autoclean",
                "-out", self.directory]
        subprocess.call(args)

    def _exportToUnv(self, mesh, meshPath):
        unvGmshFd, unvGmshPath = tempfile.mkstemp(suffix=".unv")
        brepFd, brepPath = tempfile.mkstemp(suffix=".brep")
        geoFd, geoPath = tempfile.mkstemp(suffix=".geo")
        os.close(brepFd)
        os.close(geoFd)
        os.close(unvGmshFd)

        tools = FemGmshTools.FemGmshTools(mesh)
        tools.group_elements = {g: [g] for g in self._groups}
        tools.ele_length_map = {}
        tools.temp_file_geometry = brepPath
        tools.temp_file_geo = geoPath
        tools.temp_file_mesh = unvGmshPath

        tools.get_dimension()
        tools.get_gmsh_command()
        tools.write_part_file()
        tools.write_geo()
        tools.run_gmsh_with_geo()

        ioMesh = Fem.FemMesh()
        ioMesh.read(unvGmshPath)
        ioMesh.write(meshPath)

        os.remove(brepPath)
        os.remove(geoPath)
        os.remove(unvGmshPath)

    def _getGroupName(self, subName):
        self._groups.add(subName)
        return subName

    def _writeSif(self):
        simulation = self._getSimulation()
        constants = self._getConstants()
        solvers = self._getSolvers()
        solvers.append(self._getOutputSolver())
        boundaryConditions = self._getBoundaryConditions()
        bodyForces = self._getBodyForces()
        initialConditions = self._getInitialConditions()
        equation = self._getEquation(solvers)

        bodyMaterial = dict.fromkeys(self._getSolidNames())
        materials = self._getMaterials(bodyMaterial)
        bodies = []
        for name, material in bodyMaterial.iteritems():
            bodies.append(self._getBody(
                    name, material, bodyForces, equation, initialConditions))

        sections = []
        sections.append(simulation)
        sections.append(constants)
        sections.extend(bodyForces)
        sections.extend(initialConditions)
        sections.extend(boundaryConditions)
        sections.extend(materials)
        sections.extend(solvers)
        sections.append(equation)
        sections.extend(bodies)

        sifPath = os.path.join(self.directory, _SIF_NAME)
        with open(sifPath, 'w') as fstream:
            sif = sifio.Sif(sections)
            sif.write(fstream)

    def _getSimulation(self):
        s = sifio.createSection(sifio.SIMULATION)
        s["Coordinate System"] = "Cartesian 3D"
        s["Coordinate Mapping"] = (1, 2, 3)
        s["Simulation Type"] = "Steady state"
        s["Steady State Max Iterations"] = 1
        s["Output Intervals"] = 1
        s["Timestepping Method"] = "BDF"
        s["BDF Order"] = 1
        s["Use Mesh Names"] = True
        return s

    def _getConstants(self):
        s = sifio.createSection(sifio.CONSTANTS)
        s["Gravity"] = (0.0, -1.0, 0.0, convert(
            CONSTS_DEF["Gravity"], "L/T^2"))
        s["Stefan Boltzmann"] = convert(
            CONSTS_DEF["StefanBoltzmann"], "M/(O^4*T^3)")
        s["Permittivity of Vacuum"] = convert(
            CONSTS_DEF["PermittivityOfVacuum"], "T^4*I^2/(L*M)")
        s["Boltzmann Constant"] = convert(
            CONSTS_DEF["BoltzmannConstant"], "M*L^2/(T^2*K)")
        return s

    def _getBodyForces(self):
        sections = []
        obj = FemMisc.getSingleMember(
                self.analysis, "Fem::FeaturePython", "FemConstraintSelfWeight")
        matObj = FemMisc.getSingleMember(
                self.analysis, "App::MaterialObjectPython")
        density = convert(matObj.Material["Density"], "M/L^3")
        if obj is not None:
            sections.append(self._getSelfweight(obj, density))
        if self.solver.AnalysisType == FemDefsElmer.THERMOMECH:
            obj = FemMisc.getSingleMember(
                    self.analysis, "Fem::FeaturePython",
                    "FemConstraintBodyHeatFlux")
            if obj is not None:
                sections.append(self._getBodyHeatFlux(obj))
        return sections

    def _getBoundaryConditions(self):
        for obj in FemMisc.getMember(self.analysis, "Fem::ConstraintFixed"):
            self._createFixeds(obj)
        for obj in FemMisc.getMember(self.analysis, "Fem::ConstraintForce"):
            self._createForces(obj)
        for obj in FemMisc.getMember(
                self.analysis, "Fem::ConstraintDisplacement"):
            self._createDisplacements(obj)
        if self.solver.AnalysisType == FemDefsElmer.THERMOMECH:
            for obj in FemMisc.getMember(
                    self.analysis, "Fem::ConstraintTemperature"):
                self._createTemps(obj)
        return self._bndSections.values()

    def _getInitialConditions(self):
        sections = []
        if self.solver.AnalysisType == FemDefsElmer.THERMOMECH:
            obj = FemMisc.getSingleMember(
                    self.analysis, "Fem::ConstraintInitialTemperature")
            if obj is not None:
                sections.append(self._getInitialTemp(obj))
        return sections

    def _getMaterials(self, bodyMaterials):
        sections = []
        for obj in FemMisc.getMember(
                self.analysis, "App::MaterialObjectPython"):
            s = self._getMaterialSection(obj)
            self._updateBodyMaterials(bodyMaterials, obj, s)
            sections.append(s)
        return sections

    def _getSolidNames(self):
        mesh = FemMisc.getSingleMember(
            self.analysis, "Fem::FemMeshObject")
        shape = mesh.Part.Shape
        return ["%s%d" % (_SOLID_PREFIX, i+1)
                for i in range(len(shape.Solids))]

    def _getMaterialSection(self, obj):
        m = obj.Material
        s = sifio.createSection(sifio.MATERIAL)
        s["Density"] = convert(m["Density"], "M/L^3")
        s["Youngs Modulus"] = convert(m["YoungsModulus"], "M/(L*T^2)")
        s["Poisson ratio"] = float(m["PoissonRatio"])
        s["Heat Conductivity"] = convert(
                m["ThermalConductivity"], "M*L/(T^3*O)")
        s["Heat expansion Coefficient"] = convert(
                m["ThermalExpansionCoefficient"], "O^-1")
        if self.solver.AnalysisType == FemDefsElmer.THERMOMECH:
            tempObj = FemMisc.getSingleMember(
                    self.analysis, "Fem::ConstraintInitialTemperature")
            if tempObj is not None:
                s["Reference Temperature"] = tempObj.initialTemperature
        return s

    def _updateBodyMaterials(self, bodyMaterials, obj, section):
        if len(obj.References) == 0:
            for name, material in dict(bodyMaterials).iteritems():
                bodyMaterials[name] = section
        else:
            for part, ref in obj.References:
                bodyMaterials[ref[0]] = section

    def _getSolvers(self):
        sections = []
        if self.solver.AnalysisType == FemDefsElmer.THERMOMECH:
            sections.append(self._getTermoSolver())
        sections.append(self._getElasticitySolver())
        return sections

    def _getOutputSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        s["Equation"] = "ResultOutput"
        s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr("case")
        s["Vtu Format"] = True
        return s

    def _getElasticitySolver(self):
        s = sifio.createSection(sifio.SOLVER)
        s["Equation"] = "Linear elasticity"
        s["Procedure"] = sifio.FileAttr("StressSolve/StressSolver")
        s["Displace mesh"] = False
        s["Variable"] = "Displacement"
        s["Variable DOFs"] = 3
        s["Exec Solver"] = "Always"
        s["Stabilize"] = True
        s["Bubbles"] = False
        s["Optimize Bandwidth"] = True
        s["Steady State Convergence Tolerance"] = 1.0e-5
        s["Linear System Residual Output"] = 1
        if self.solver.AnalysisType == FemDefsElmer.FREQUENCY:
            s["Linear System Solver"] = "Direct"
            s["Eigen Analysis"] = True
            s["Eigen System Values"] = self.solver.EigenmodesCount
        else:
            s["Linear System Solver"] = "Iterative"
            s["Linear System Iterative Method"] = "BiCGStab"
            s["BiCGstabl polynomial degree"] = 2
            s["Linear System Max Iterations"] = \
                self.solver.LinMaxIterations
            s["Linear System Convergence Tolerance"] = \
                self.solver.LinConvergenceTolerance
            s["Linear System Preconditioning"] = "Diagonal"
            s["Linear System Precondition Recompute"] = 1
            s["Linear System Abort Not Converged"] = False
        return s

    def _getTermoSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        s["Equation"] = "Heat Equation"
        s["Procedure"] = sifio.FileAttr("HeatSolve/HeatSolver")
        s["Variable"] = "Temperature"
        s["Exec Solver"] = "Always"
        s["Stabilize"] = True
        s["Bubbles"] = False
        s["Optimize Bandwidth"] = True
        s["Steady State Convergence Tolerance"] = 1.0e-5
        s["Nonlinear System Convergence Tolerance"] = \
            self.solver.TermoNLinConvergenceTolerance
        s["Nonlinear System Max Iterations"] = \
            self.solver.TermoNLinMaxIterations
        s["Nonlinear System Newton After Iterations"] = 3
        s["Nonlinear System Newton After Tolerance"] = 1.0e-3
        s["Nonlinear System Relaxation Factor"] = 1.0
        s["Linear System Solver"] = "Iterative"
        s["Linear System Iterative Method"] = "BiCGStab"
        s["Linear System Max Iterations"] = \
            self.solver.TermoLinMaxIterations
        s["Linear System Convergence Tolerance"] = \
            self.solver.TermoLinConvergenceTolerance
        s["Linear System Preconditioning"] = "Diagonal"
        s["Linear System Abort Not Converged"] = False
        s["Linear System Residual Output"] = 1
        s["Linear System Precondition Recompute"] = 1
        return s

    def _getEquation(self, solvers):
        s = sifio.createSection(sifio.EQUATION)
        s["Active Solvers"] = solvers
        return s

    def _getBody(self, name, material, bodyForces, equation, initial):
        s = sifio.createSection(sifio.BODY)
        s["Name"] = self._getGroupName(name)
        s["Material"] = material
        s["Equation"] = equation
        s["Body Force"] = bodyForces
        s["Initial Condition"] = initial
        return s

    def _getSelfweight(self, obj, density):
        s = sifio.createSection(sifio.BODY_FORCE)
        gravity = convert(CONSTS_DEF["Gravity"], "L/T^2")
        s["Stress Bodyforce 1"] = float(gravity * obj.Gravity_x * density)
        s["Stress Bodyforce 2"] = float(gravity * obj.Gravity_y * density)
        s["Stress Bodyforce 3"] = float(gravity * obj.Gravity_z * density)
        return s

    def _getBodyHeatFlux(self, obj):
        s = sifio.createSection(sifio.BODY_FORCE)
        s["Heat Source"] = getFromUi(obj.HeatFlux, "W/kg", "L^2*T^-3")
        return s

    def _createFixeds(self, obj):
        names = (self._getGroupName(x) for x in obj.References[0][1])
        for n in names:
            s = self._getBndSection(n)
            s["Displacement 1"] = 0.0
            s["Displacement 2"] = 0.0
            s["Displacement 3"] = 0.0

    def _createDisplacements(self, obj):
        names = [self._getGroupName(x) for x in obj.References[0][1]]
        for n in names:
            s = self._getBndSection(n)
            if not obj.xFree:
                s["Displacement 1"] = float(obj.xDisplacement) * 0.001
            elif obj.xFix:
                s["Displacement 1"] = 0.0
            if not obj.yFree:
                s["Displacement 2"] = float(obj.yDisplacement) * 0.001
            elif obj.yFix:
                s["Displacement 2"] = 0.0
            if not obj.zFree:
                s["Displacement 3"] = float(obj.zDisplacement) * 0.001
            elif obj.zFix:
                s["Displacement 3"] = 0.0

    def _createForces(self, obj):
        names = [self._getGroupName(x) for x in obj.References[0][1]]
        for n in names:
            s = self._getBndSection(n)
            force = getFromUi(obj.Force, "N", "M*L*T^-2")
            s["Force 1"] = float(obj.DirectionVector.x * force)
            s["Force 2"] = float(obj.DirectionVector.y * force)
            s["Force 3"] = float(obj.DirectionVector.z * force)
            s["Force 1 Normalize by Area"] = True
            s["Force 2 Normalize by Area"] = True
            s["Force 3 Normalize by Area"] = True

    def _createTemps(self, obj):
        names = [self._getGroupName(x) for x in obj.References[0][1]]
        for n in names:
            s = self._getBndSection(n)
            s["Temperature"] = getFromUi(obj.Temperature, "K", "O")

    def _getInitialTemp(self, obj):
        s = sifio.createSection(sifio.INITIAL_CONDITION)
        s["Temperature"] = getFromUi(obj.initialTemperature, "K", "O")
        return s

    def _getBndSection(self, name):
        if name in self._bndSections:
            return self._bndSections[name]
        s = sifio.createSection(sifio.BOUNDARY_CONDITION)
        s["Name"] = name
        self._bndSections[name] = s
        return s
