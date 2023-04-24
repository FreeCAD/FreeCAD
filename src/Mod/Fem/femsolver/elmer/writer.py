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

__title__ = "FreeCAD FEM solver Elmer writer"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess
import tempfile
from platform import system

from FreeCAD import Console
from FreeCAD import Units
from FreeCAD import ParamGet

import Fem
from . import sifio
from . import solver as solverClass
from .. import settings
from femmesh import gmshtools
from femtools import constants
from femtools import femutils
from femtools import membertools
from .equations import deformation_writer as DEF_writer
from .equations import elasticity_writer as EL_writer
from .equations import electricforce_writer as EF_writer
from .equations import electrostatic_writer as ES_writer
from .equations import flow_writer
from .equations import flux_writer
from .equations import heat_writer
from .equations import magnetodynamic_writer as MgDyn_writer
from .equations import magnetodynamic2D_writer as MgDyn2D_writer


_STARTINFO_NAME = "ELMERSOLVER_STARTINFO"
_SIF_NAME = "case.sif"
_ELMERGRID_IFORMAT = "8"
_ELMERGRID_OFORMAT = "2"
_COORDS_NON_MAGNETO_2D = ["Polar 2D", "Polar 3D", "Cartesian 3D",
                          "Cylindric", "Cylindric Symmetric"]


def _getAllSubObjects(obj):
    s = ["Solid" + str(i + 1) for i in range(len(obj.Shape.Solids))]
    s.extend(("Face" + str(i + 1) for i in range(len(obj.Shape.Faces))))
    s.extend(("Edge" + str(i + 1) for i in range(len(obj.Shape.Edges))))
    s.extend(("Vertex" + str(i + 1) for i in range(len(obj.Shape.Vertexes))))
    return s


class Writer(object):

    def __init__(self, solver, directory, testmode=False):
        self.analysis = solver.getParentGroup()
        self.solver = solver
        self.directory = directory
        Console.PrintMessage("Write elmer input files to: {}\n".format(self.directory))
        self.testmode = testmode
        self._usedVarNames = set()
        self._builder = sifio.Builder()
        self._handledObjects = set()
        self._handleUnits()
        self._handleConstants()

    def getHandledConstraints(self):
        return self._handledObjects

    def write_solver_input(self):
        self._handleRedifinedConstants()
        self._handleSimulation()
        self._handleDeformation()
        self._handleElasticity()
        self._handleElectricforce()
        self._handleElectrostatic()
        self._handleHeat()
        self._handleFlow()
        self._handleFlux()
        self._handleMagnetodynamic()
        self._handleMagnetodynamic2D()
        self._addOutputSolver()

        self._writeSif()
        self._writeMesh()
        self._writeStartinfo()

    def _handleUnits(self):
        # Elmer solver writer no longer uses FreeCAD unit system
        # to retrieve units for writing the sif file
        #
        # ATM Elmer writer uses SI units only
        #
        # see forum topic: https://forum.freecad.org/viewtopic.php?f=18&t=70150
        #
        # TODO: adapt method and comment
        # should be only one system for all solver and not in each solver
        # https://forum.freecad.org/viewtopic.php?t=47895
        # https://forum.freecad.org/viewtopic.php?t=48451
        # https://forum.freecad.org/viewtopic.php?f=10&t=48642
        # The FreeCAD unit schema is only used to determine the schema number
        # all definition are done here ATM
        # keep in mind a unit schema might not be consistent:
        # Length could be mm and Area could be m2 and Volume could be cm3
        # as long as only the seven base units are retrieved from a unit schema
        # the units are consistent
        # TODO retrieve the seven base units from FreeCAD unit schema
        # instead of hard coding them here for a second once
        self.unit_schema = Units.Scheme.SI1
        self.unit_system = {  # standard FreeCAD Base units = unit schema 0
            "L": "m",
            "M": "kg",
            "T": "s",
            "I": "A",
            "O": "K",
            "N": "mol",
            "J": "cd",
        }
        param = ParamGet("User parameter:BaseApp/Preferences/Units")
        self.unit_schema = param.GetInt("UserSchema", Units.Scheme.SI1)
        if self.unit_schema == Units.Scheme.SI1:
            Console.PrintMessage(
                "The FreeCAD standard unit schema mm/kg/s is used. "
                "Elmer sif-file writing is however done in SI units.\n"
            )
        elif self.unit_schema == Units.Scheme.SI2:
            Console.PrintMessage(
                "The SI unit schema m/kg/s is used. "
                "Elmer sif-file writing is done in SI-units.\n"
            )
            self.unit_system = {
                "L": "m",
                "M": "kg",
                "T": "s",
                "I": "A",
                "O": "K",
                "N": "mol",
                "J": "cd",
            }
        elif self.unit_schema == Units.Scheme.FemMilliMeterNewton:
            # see also unit comment in calculix writer
            Console.PrintMessage(
                "The FEM unit schema mm/N/s is used. "
                "Elmer sif-file writing is however done in SI units.\n"
            )
            self.unit_system = {
                "L": "m",
                "M": "kg",
                "T": "s",
                "I": "A",
                "O": "K",
                "N": "mol",
                "J": "cd",
            }
        elif (
            self.unit_schema > Units.Scheme.SI2
            and self.unit_schema != Units.Scheme.FemMilliMeterNewton
        ):
            Console.PrintMessage(
                "Unit schema: {} not supported by Elmer writer. "
                "The FreeCAD standard unit schema mm/kg/s is used. "
                "Elmer sif-file writing is done in Standard FreeCAD units.\n"
                .format(Units.listSchemas(self.unit_schema))
            )

    def getFromUi(self, value, unit, outputDim):
        quantity = Units.Quantity(str(value) + str(unit))
        return self.convert(quantity, outputDim)

    def convert(self, quantityStr, unit):
        quantity = Units.Quantity(quantityStr)
        for key, setting in self.unit_system.items():
            unit = unit.replace(key, setting)
        return float(quantity.getValueAs(unit))

    def _handleConstants(self):
        self.constsdef = {
            "Gravity": constants.gravity(),
            "StefanBoltzmann": constants.stefan_boltzmann(),
            "PermeabilityOfVacuum": constants.vacuum_permeability(),
            "PermittivityOfVacuum": constants.vacuum_permittivity(),
            "BoltzmannConstant": constants.boltzmann_constant(),
        }

    def _writeMesh(self):
        mesh = self.getSingleMember("Fem::FemMeshObject")
        unvPath = os.path.join(self.directory, "mesh.unv")
        groups = []
        groups.extend(self._builder.getBodyNames())
        groups.extend(self._builder.getBoundaryNames())
        self._exportToUnv(groups, mesh, unvPath)
        if self.testmode:
            Console.PrintMessage(
                "Solver Elmer testmode, ElmerGrid will not be used. "
                "It might not be installed.\n"
            )
        else:
            binary = settings.get_binary("ElmerGrid")
            num_cores = settings.get_cores("ElmerGrid")
            if binary is None:
                raise WriteError("Could not find ElmerGrid binary.")
            # for multithreading we first need a normal mesh creation run
            # then a second to split the mesh into the number of used cores
            argsBasic = [binary,
                         _ELMERGRID_IFORMAT,
                         _ELMERGRID_OFORMAT,
                         unvPath]
            args = argsBasic
            args.extend(["-out", self.directory])
            if system() == "Windows":
                subprocess.call(
                    args,
                    stdout=subprocess.DEVNULL,
                    startupinfo=femutils.startProgramInfo("hide")
                )
            else:
                subprocess.call(args, stdout=subprocess.DEVNULL)
            if num_cores > 1:
                args = argsBasic
                args.extend(["-partdual", "-metiskway", str(num_cores),
                             "-out", self.directory])
                if system() == "Windows":
                    subprocess.call(
                        args,
                        stdout=subprocess.DEVNULL,
                        startupinfo=femutils.startProgramInfo("hide")
                    )
                else:
                    subprocess.call(args, stdout=subprocess.DEVNULL)

    def _writeStartinfo(self):
        path = os.path.join(self.directory, _STARTINFO_NAME)
        with open(path, "w") as f:
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
        tools.get_region_data()
        tools.get_boundary_layer_data()
        tools.write_part_file()
        tools.write_geo()
        if self.testmode:
            Console.PrintMessage(
                "Solver Elmer testmode, Gmsh will not be used. "
                "It might not be installed.\n"
            )
            import shutil
            shutil.copyfile(geoPath, os.path.join(self.directory, "group_mesh.geo"))
        else:
            tools.get_gmsh_command()
            tools.run_gmsh_with_geo()

            ioMesh = Fem.FemMesh()
            ioMesh.read(unvGmshPath)
            ioMesh.write(meshPath)

        os.remove(brepPath)
        os.remove(geoPath)
        os.remove(unvGmshPath)

    def _handleRedifinedConstants(self):
        """
        redefine constants in self.constsdef according constant redefine objects
        """
        objs = self.getMember("Fem::ConstantVacuumPermittivity")
        if len(objs) == 1:
            permittivity = float(objs[0].VacuumPermittivity.getValueAs("F/m"))
            # since the base unit of FC is in mm, we must scale it to get plain SI
            permittivity = permittivity * 1e-9
            Console.PrintLog("Overwriting vacuum permittivity with: {}\n".format(permittivity))
            self.constsdef["PermittivityOfVacuum"] = "{} {}".format(permittivity, "F/m")
            self.handled(objs[0])
        elif len(objs) > 1:
            Console.PrintError(
                "More than one permittivity constant overwriting objects ({} objs). "
                "The permittivity constant overwriting is ignored.\n"
                .format(len(objs))
            )

    def _handleSimulation(self):
        # check if we need to update the equation
        self._updateSimulation(self.solver)
        # output the equation parameters
        # first check what equations we have

        # hasHeat ist not used, thus commented ATM
        # hasHeat = False
        # for equation in self.solver.Group:
        #    if femutils.is_of_type(equation, "Fem::EquationElmerHeat"):
        #        hasHeat = True

        self._simulation("Coordinate System", self.solver.CoordinateSystem)
        self._simulation("Coordinate Mapping", (1, 2, 3))
        # Elmer uses SI base units, but our mesh is in mm, therefore we must tell
        # the solver that we have another scale
        self._simulation("Coordinate Scaling", 0.001)
        self._simulation("Simulation Type", self.solver.SimulationType)
        if self.solver.SimulationType == "Steady State":
            self._simulation(
                "Steady State Max Iterations",
                self.solver.SteadyStateMaxIterations
            )
            self._simulation(
                "Steady State Min Iterations",
                self.solver.SteadyStateMinIterations
            )
        if (
            self.solver.SimulationType == "Scanning"
            or self.solver.SimulationType == "Transient"
        ):
            self._simulation("BDF Order", self.solver.BDFOrder)
            self._simulation("Output Intervals", self.solver.OutputIntervals)
            self._simulation("Timestep Intervals", self.solver.TimestepIntervals)
            self._simulation("Timestep Sizes", self.solver.TimestepSizes)
            self._simulation("Timestepping Method", "BDF")
        self._simulation("Use Mesh Names", True)

    def _updateSimulation(self, solver):
        # updates older simulations
        if not hasattr(self.solver, "CoordinateSystem"):
            solver.addProperty(
                "App::PropertyEnumeration",
                "CoordinateSystem",
                "Coordinate System",
                ""
            )
            solver.CoordinateSystem = solverClass.COORDINATE_SYSTEM
            solver.CoordinateSystem = "Cartesian"
        if not hasattr(self.solver, "BDFOrder"):
            solver.addProperty(
                "App::PropertyIntegerConstraint",
                "BDFOrder",
                "Timestepping",
                "Order of time stepping method 'BDF'"
            )
            solver.BDFOrder = (2, 1, 5, 1)
        if not hasattr(self.solver, "ElmerTimeResults"):
            solver.addProperty(
                "App::PropertyLinkList",
                "ElmerTimeResults",
                "Base",
                "",
                4 | 8
            )
        if not hasattr(self.solver, "OutputIntervals"):
            solver.addProperty(
                "App::PropertyIntegerList",
                "OutputIntervals",
                "Timestepping",
                "After how many time steps a result file is output"
            )
            solver.OutputIntervals = [1]
        if not hasattr(self.solver, "SimulationType"):
            solver.addProperty(
                "App::PropertyEnumeration",
                "SimulationType",
                "Type",
                ""
            )
            solver.SimulationType = solverClass.SIMULATION_TYPE
            solver.SimulationType = "Steady State"
        if not hasattr(self.solver, "TimestepIntervals"):
            solver.addProperty(
                "App::PropertyIntegerList",
                "TimestepIntervals",
                "Timestepping",
                (
                    "List of maximum optimization rounds if 'Simulation Type'\n"
                    "is either 'Scanning' or 'Transient'"
                )
            )
            solver.TimestepIntervals = [100]
        if not hasattr(self.solver, "TimestepSizes"):
            solver.addProperty(
                "App::PropertyFloatList",
                "TimestepSizes",
                "Timestepping",
                (
                    "List of time steps of optimization if 'Simulation Type'\n"
                    "is either 'Scanning' or 'Transient'"
                )
            )
            solver.TimestepSizes = [0.1]

    # -------------------------------------------------------------------------------------------
    # Deformation

    def _handleDeformation(self):
        DEFW = DEF_writer.DeformationWriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerDeformation"):
                if not self._haveMaterialSolid():
                    raise WriteError(
                        "The Deformation equation requires at least one body with a solid material!"
                    )
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = DEFW.getDeformationSolver(equation)
                for body in activeIn:
                    if not self.isBodyMaterialFluid(body):
                        self._addSolver(body, solverSection)
                        DEFW.handleDeformationEquation(activeIn, equation)
        if activeIn:
            DEFW.handleDeformationConstants()
            DEFW.handleDeformationBndConditions()
            DEFW.handleDeformationInitial(activeIn)
            DEFW.handleDeformationBodyForces(activeIn)
            DEFW.handleDeformationMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Elasticity

    def _handleElasticity(self):
        ELW = EL_writer.ElasticityWriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElasticity"):
                if not self._haveMaterialSolid():
                    raise WriteError(
                        "The Elasticity equation requires at least one body with a solid material!"
                    )
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = ELW.getElasticitySolver(equation)
                for body in activeIn:
                    if not self.isBodyMaterialFluid(body):
                        self._addSolver(body, solverSection)
                        ELW.handleElasticityEquation(activeIn, equation)
        if activeIn:
            ELW.handleElasticityConstants()
            ELW.handleElasticityBndConditions()
            ELW.handleElasticityInitial(activeIn)
            ELW.handleElasticityBodyForces(activeIn)
            ELW.handleElasticityMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Electrostatic

    def _handleElectrostatic(self):
        ESW = ES_writer.ESwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElectrostatic"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = ESW.getElectrostaticSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
        if activeIn:
            ESW.handleElectrostaticConstants()
            ESW.handleElectrostaticBndConditions()
            ESW.handleElectrostaticMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Electricforce

    def _handleElectricforce(self):
        EFW = EF_writer.EFwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElectricforce"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = EFW.getElectricforceSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)

    # -------------------------------------------------------------------------------------------
    # Flow

    def _handleFlow(self):
        FlowW = flow_writer.Flowwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerFlow"):
                if not self._haveMaterialFluid():
                    raise WriteError(
                        "The Flow equation requires at least one body with a fluid material!"
                    )
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = FlowW.getFlowSolver(equation)
                for body in activeIn:
                    if self.isBodyMaterialFluid(body):
                        self._addSolver(body, solverSection)
                        FlowW.handleFlowEquation(activeIn, equation)
        if activeIn:
            FlowW.handleFlowConstants()
            FlowW.handleFlowBndConditions()
            FlowW.handleFlowInitialPressure(activeIn)
            FlowW.handleFlowInitialVelocity(activeIn)
            FlowW.handleFlowMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Flux

    def _handleFlux(self):
        FluxW = flux_writer.Fluxwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerFlux"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = FluxW.getFluxSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)

    # -------------------------------------------------------------------------------------------
    # Heat

    def _handleHeat(self):
        HeatW = heat_writer.Heatwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerHeat"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = HeatW.getHeatSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                HeatW.handleHeatEquation(activeIn, equation)
        if activeIn:
            HeatW.handleHeatConstants()
            HeatW.handleHeatBndConditions()
            HeatW.handleHeatInitial(activeIn)
            HeatW.handleHeatBodyForces(activeIn)
            HeatW.handleHeatMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Magnetodynamic

    def _handleMagnetodynamic(self):
        MgDyn = MgDyn_writer.MgDynwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerMagnetodynamic"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()

                solverSection = MgDyn.getMagnetodynamicSolver(equation)
                solverPostSection = MgDyn.getMagnetodynamicSolverPost(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                    self._addSolver(body, solverPostSection)
        if activeIn:
            MgDyn.handleMagnetodynamicConstants()
            MgDyn.handleMagnetodynamicBndConditions(equation)
            MgDyn.handleMagnetodynamicBodyForces(activeIn, equation)
            MgDyn.handleMagnetodynamicMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Magnetodynamic2D

    def _handleMagnetodynamic2D(self):
        MgDyn2D = MgDyn2D_writer.MgDyn2Dwriter(self, self.solver)
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerMagnetodynamic2D"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                # Magnetodynamic2D cannot handle all coordinate sysytems
                if self.solver.CoordinateSystem in _COORDS_NON_MAGNETO_2D:
                    raise WriteError(
                        "The coordinate setting '{}'\n is not "
                        "supported by the equation 'Magnetodynamic2D'.\n\n"
                        "Possible is:\n'Cartesian 2D' or 'Axi Symmetric'"
                        .format(self.solver.CoordinateSystem)
                    )

                solverSection = MgDyn2D.getMagnetodynamic2DSolver(equation)
                solverPostSection = MgDyn2D.getMagnetodynamic2DSolverPost(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                    self._addSolver(body, solverPostSection)
                    MgDyn2D.handleMagnetodynamic2DEquation(activeIn, equation)
        if activeIn:
            MgDyn2D.handleMagnetodynamic2DConstants()
            MgDyn2D.handleMagnetodynamic2DBndConditions()
            MgDyn2D.handleMagnetodynamic2DBodyForces(activeIn, equation)
            MgDyn2D.handleMagnetodynamic2DMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Solver handling

    def createEmptySolver(self):
        s = sifio.createSection(sifio.SOLVER)
        return s

    def _updateLinearSolver(self, equation):
        if self._hasExpression(equation) != equation.LinearTolerance:
            equation.setExpression("LinearTolerance", str(equation.LinearTolerance))
        if self._hasExpression(equation) != equation.SteadyStateTolerance:
            equation.setExpression("SteadyStateTolerance", str(equation.SteadyStateTolerance))
        if equation.BiCGstablDegree == 0:
            equation.BiCGstablDegree = 2
        if not hasattr(equation, "LinearSystemSolverDisabled"):
            equation.addProperty(
                "App::PropertyBool",
                "LinearSystemSolverDisabled",
                "Linear System",
                (
                    "Disable the linear system.\n"
                    "Only use for special cases\n"
                    "and consult the Elmer docs."
                )
            )
        if not hasattr(equation, "IdrsParameter"):
            equation.addProperty(
                "App::PropertyIntegerConstraint",
                "IdrsParameter",
                "Linear System",
                "Parameter for iterative method 'Idrs'"
            )
            equation.IdrsParameter = (2, 1, 10, 1)

    def createLinearSolver(self, equation):
        # first check if we have to update
        self._updateLinearSolver(equation)
        # write the solver
        s = sifio.createSection(sifio.SOLVER)
        s.priority = equation.Priority
        s["Linear System Solver"] = equation.LinearSolverType
        if equation.LinearSystemSolverDisabled is True:
            s["Linear System Solver Disabled"] = equation.LinearSystemSolverDisabled
        if equation.LinearSolverType == "Direct":
            s["Linear System Direct Method"] = \
                equation.LinearDirectMethod
        elif equation.LinearSolverType == "Iterative":
            s["Linear System Iterative Method"] = \
                equation.LinearIterativeMethod
            if equation.LinearIterativeMethod == "BiCGStabl":
                s["BiCGstabl polynomial degree"] = \
                    equation.BiCGstablDegree
            if equation.LinearIterativeMethod == "Idrs":
                s["Idrs Parameter"] = \
                    equation.IdrsParameter
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

    def _updateNonlinearSolver(self, equation):
        if self._hasExpression(equation) != equation.NonlinearTolerance:
            equation.setExpression("NonlinearTolerance", str(equation.NonlinearTolerance))
        if self._hasExpression(equation) != equation.NonlinearNewtonAfterTolerance:
            equation.setExpression(
                "NonlinearNewtonAfterTolerance",
                str(equation.NonlinearNewtonAfterTolerance)
            )

    def createNonlinearSolver(self, equation):
        # first check if we have to update
        self._updateNonlinearSolver(equation)
        # write the linear solver
        s = self.createLinearSolver(equation)
        # write the nonlinear solver
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

    # -------------------------------------------------------------------------------------------
    # Helper functions

    def _haveMaterialSolid(self):
        for obj in self.getMember("App::MaterialObject"):
            m = obj.Material
            # fluid material always has KinematicViscosity defined
            if "KinematicViscosity" not in m:
                return True
        return False

    def _haveMaterialFluid(self):
        for obj in self.getMember("App::MaterialObject"):
            m = obj.Material
            # fluid material always has a viscosity defined
            if ("DynamicViscosity" in m) or ("KinematicViscosity" in m):
                return True
        return False

    def isBodyMaterialFluid(self, body):
        # we can have the case that a body has no assigned material
        # then assume it is a solid
        if self.getBodyMaterial(body) is not None:
            m = self.getBodyMaterial(body).Material
            return ("DynamicViscosity" in m) or ("KinematicViscosity" in m)
        return False

    def getBodyMaterial(self, name):
        for obj in self.getMember("App::MaterialObject"):
            # we can have e.g. the case there are 2 bodies and 2 materials
            # body 2 has material 2 as reference while material 1 has no reference
            # therefore we must not return a material when it is not referenced
            if obj.References and (name in obj.References[0][1]):
                return obj
        # 'name' was not in the reference of any material
        return None

    def getDensity(self, m):
        density = self.convert(m["Density"], "M/L^3")
        if self.getMeshDimension() == 2:
            density *= 1e3
        return density

    def _hasExpression(self, equation):
        for (obj, exp) in equation.ExpressionEngine:
            if obj == equation:
                return exp
        return None

    def getUniqueVarName(self, varName):
        postfix = 1
        if varName in self._usedVarNames:
            varName += "%2d" % postfix
        while varName in self._usedVarNames:
            postfix += 1
            varName = varName[:-2] + "%2d" % postfix
        self._usedVarNames.add(varName)
        return varName

    def getAllBodies(self):
        obj = self.getSingleMember("Fem::FemMeshObject")
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

    def getMeshDimension(self):
        obj = self.getSingleMember("Fem::FemMeshObject")
        if obj.Part.Shape.Solids:
            return 3
        if obj.Part.Shape.Faces:
            return 2
        if obj.Part.Shape.Edges:
            return 1
        return None

    def _addOutputSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        # Since FreeCAD meshes are in mm we let Elmer scale it
        # _handleSimulation(self).
        # To get it back in the original size we let Elmer scale it back
        s["Coordinate Scaling Revert"] = True
        s["Equation"] = "ResultOutput"
        if (
            self.solver.SimulationType == "Scanning"
            or self.solver.SimulationType == "Transient"
        ):
            # we must execute the post solver every time we output a result
            # therefore we must use the same as self.solver.OutputIntervals
            s["Exec Intervals"] = self.solver.OutputIntervals
        else:
            s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr("FreeCAD")
        s["Vtu Format"] = True
        s["Vtu Time Collection"] = True
        if self.unit_schema == Units.Scheme.SI2:
            s["Coordinate Scaling Revert"] = True
            Console.PrintMessage(
                "'Coordinate Scaling Revert = Logical True' was "
                "inserted into the solver input file.\n"
            )
        for name in self.getAllBodies():
            self._addSolver(name, s)

    def _writeSif(self):
        sifPath = os.path.join(self.directory, _SIF_NAME)
        with open(sifPath, "w") as fstream:
            sif = sifio.Sif(self._builder)
            sif.write(fstream)

    def handled(self, obj):
        self._handledObjects.add(obj)

    def _simulation(self, key, attr):
        self._builder.simulation(key, attr)

    def constant(self, key, attr):
        self._builder.constant(key, attr)

    def initial(self, body, key, attr):
        self._builder.initial(body, key, attr)

    def material(self, body, key, attr):
        self._builder.material(body, key, attr)

    def equation(self, body, key, attr):
        self._builder.equation(body, key, attr)

    def bodyForce(self, body, key, attr):
        self._builder.bodyForce(body, key, attr)

    def _addSolver(self, body, solverSection):
        self._builder.addSolver(body, solverSection)

    def boundary(self, boundary, key, attr):
        self._builder.boundary(boundary, key, attr)

    def _addSection(self, section):
        self._builder.addSection(section)

    def getMember(self, t):
        return membertools.get_member(self.analysis, t)

    def getSingleMember(self, t):
        return membertools.get_single_member(self.analysis, t)


class WriteError(Exception):
    pass

##  @}
