# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
__author__ = "Markus Hovorka"
__url__ = "https://www.freecadweb.org"

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
from .equations import elasticity
from .equations import flow
from .equations import heat


_STARTINFO_NAME = "ELMERSOLVER_STARTINFO"
_SIF_NAME = "case.sif"
_ELMERGRID_IFORMAT = "8"
_ELMERGRID_OFORMAT = "2"
_SOLID_PREFIX = "Solid"


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
        self._handleHeat()
        self._handleElasticity()
        self._handleElectrostatic()
        self._handleFlux()
        self._handleElectricforce()
        self._handleFlow()
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
        # see forum topic: https://forum.freecadweb.org/viewtopic.php?f=18&t=70150
        #
        # TODO: adapt method and comment
        # should be only one system for all solver and not in each solver
        # https://forum.freecadweb.org/viewtopic.php?t=47895
        # https://forum.freecadweb.org/viewtopic.php?t=48451
        # https://forum.freecadweb.org/viewtopic.php?f=10&t=48642
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

    def _getFromUi(self, value, unit, outputDim):
        quantity = Units.Quantity(str(value) + str(unit))
        return self._convert(quantity, outputDim)

    def _convert(self, quantityStr, unit):
        quantity = Units.Quantity(quantityStr)
        for key, setting in self.unit_system.items():
            unit = unit.replace(key, setting)
        return float(quantity.getValueAs(unit))

    def _handleConstants(self):
        self.constsdef = {
            "Gravity": constants.gravity(),
            "StefanBoltzmann": constants.stefan_boltzmann(),
            "PermittivityOfVacuum": constants.vacuum_permittivity(),
            "BoltzmannConstant": constants.boltzmann_constant(),
        }

    def _getConstant(self, name, unit_dimension):
        # TODO without method directly use self.constsdef[name]
        return self._convert(self.constsdef[name], unit_dimension)

    def _setConstant(self, name, quantityStr):
        # TODO without method directly use self.constsdef[name]
        if name == "PermittivityOfVacuum":
            theUnit = "s^4*A^2 / (m^3*kg)"
            self.constsdef[name] = "{} {}".format(self._convert(quantityStr, theUnit), theUnit)
        return True

    def _writeMesh(self):
        mesh = self._getSingleMember("Fem::FemMeshObject")
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
            if int(num_cores) > 1:
                args = argsBasic
                args.extend(["-partdual", "-metiskway", num_cores,
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
        permittivity_objs = self._getMember("Fem::ConstantVacuumPermittivity")
        if len(permittivity_objs) == 1:
            Console.PrintLog("Constand permittivity overwriting.\n")
            self._setConstant("PermittivityOfVacuum", permittivity_objs[0].VacuumPermittivity)
        elif len(permittivity_objs) > 1:
            Console.PrintError(
                "More than one permittivity constant overwriting objects ({} objs). "
                "The permittivity constant overwriting is ignored.\n"
                .format(len(permittivity_objs))
            )

    def _handleSimulation(self):
        # check if we need to update the equation
        self._updateSimulation(self.solver)
        # output the equation parameters
        # first check what equations we have
        hasHeat = False
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerHeat"):
                hasHeat = True
        if hasHeat:
            self._simulation("BDF Order", self.solver.BDFOrder)
        self._simulation("Coordinate System", "Cartesian 3D")
        self._simulation("Coordinate Mapping", (1, 2, 3))
        # Elmer uses SI base units, but our mesh is in mm, therefore we must tell
        # the solver that we have another scale
        self._simulation("Coordinate Scaling", 0.001)
        self._simulation("Output Intervals", 1)
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
            self._simulation("Timestep Intervals", self.solver.TimestepIntervals)
            self._simulation("Timestep Sizes", self.solver.TimestepSizes)
            # Output Intervals must be equal to Timestep Intervals
            self._simulation("Output Intervals", self.solver.TimestepIntervals)
        if hasHeat:
            self._simulation("Timestepping Method", "BDF")
        self._simulation("Use Mesh Names", True)

    def _updateSimulation(self, solver):
        # updates older simulations
        if not hasattr(self.solver, "BDFOrder"):
            solver.addProperty(
                "App::PropertyIntegerConstraint",
                "BDFOrder",
                "Timestepping",
                "Order of time stepping method 'BDF'"
            )
            solver.BDFOrder = (2, 1, 5, 1)
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

    def _handleHeat(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerHeat"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getHeatSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                self._handleHeatEquation(activeIn, equation)
        if activeIn:
            self._handleHeatConstants()
            self._handleHeatBndConditions()
            self._handleHeatInitial(activeIn)
            self._handleHeatBodyForces(activeIn)
            self._handleHeatMaterial(activeIn)

    def _getHeatSolver(self, equation):
        # check if we need to update the equation
        self._updateHeatSolver(equation)
        # output the equation parameters
        s = self._createNonlinearSolver(equation)
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("HeatSolve/HeatSolver")
        s["Bubbles"] = equation.Bubbles
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        s["Variable"] = self._getUniqueVarName("Temperature")
        return s

    def _handleHeatConstants(self):
        self._constant(
            "Stefan Boltzmann",
            self._getConstant("StefanBoltzmann", "M/(O^4*T^3)"))

    def _handleHeatEquation(self, bodies, equation):
        for b in bodies:
            if equation.Convection != "None":
                self._equation(b, "Convection", equation.Convection)
            if equation.PhaseChangeModel != "None":
                self._equation(b, "Phase Change Model", equation.PhaseChangeModel)

    def _updateHeatSolver(self, equation):
        # updates older Heat equations
        if not hasattr(equation, "Convection"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "Convection",
                "Equation",
                "Type of convection to be used"
            )
            equation.Convection = heat.CONVECTION_TYPE
            equation.Convection = "None"
        if not hasattr(equation, "PhaseChangeModel"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "PhaseChangeModel",
                "Equation",
                "Model for phase change"
            )
            equation.PhaseChangeModel = heat.PHASE_CHANGE_MODEL
            equation.PhaseChangeModel = "None"

    def _handleHeatBndConditions(self):
        for obj in self._getMember("Fem::ConstraintTemperature"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Temperature":
                        temp = self._getFromUi(obj.Temperature, "K", "O")
                        self._boundary(name, "Temperature", temp)
                    elif obj.ConstraintType == "CFlux":
                        flux = self._getFromUi(obj.CFlux, "kg*mm^2*s^-3", "M*L^2*T^-3")
                        self._boundary(name, "Temperature Load", flux)
                self._handled(obj)
        for obj in self._getMember("Fem::ConstraintHeatflux"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Convection":
                        film = self._getFromUi(obj.FilmCoef, "W/(m^2*K)", "M/(T^3*O)")
                        temp = self._getFromUi(obj.AmbientTemp, "K", "O")
                        self._boundary(name, "Heat Transfer Coefficient", film)
                        self._boundary(name, "External Temperature", temp)
                    elif obj.ConstraintType == "DFlux":
                        flux = self._getFromUi(obj.DFlux, "W/m^2", "M*T^-3")
                        self._boundary(name, "Heat Flux BC", True)
                        self._boundary(name, "Heat Flux", flux)
                self._handled(obj)

    def _handleHeatInitial(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if obj is not None:
            for name in bodies:
                temp = self._getFromUi(obj.initialTemperature, "K", "O")
                self._initial(name, "Temperature", temp)
            self._handled(obj)

    def _handleHeatBodyForces(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintBodyHeatSource")
        if obj is not None:
            for name in bodies:
                heatSource = self._getFromUi(obj.HeatSource, "W/kg", "L^2*T^-3")
                # according Elmer forum W/kg is correct
                # http://www.elmerfem.org/forum/viewtopic.php?f=7&t=1765
                # 1 watt = kg * m2 / s3 ... W/kg = m2 / s3
                self._bodyForce(name, "Heat Source", heatSource)
            self._handled(obj)

    def _handleHeatMaterial(self, bodies):
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = self._getFromUi(tempObj.initialTemperature, "K", "O")
            for name in bodies:
                self._material(name, "Reference Temperature", refTemp)
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies())
            for name in (n for n in refs if n in bodies):
                if "Density" not in m:
                    raise WriteError(
                        "Used material does not specify the necessary 'Density'."
                    )
                self._material(
                    name, "Density",
                    self._getDensity(m))
                if "ThermalConductivity" not in m:
                    raise WriteError(
                        "Used material does not specify the necessary 'Thermal Conductivity'."
                    )
                self._material(
                    name, "Heat Conductivity",
                    self._convert(m["ThermalConductivity"], "M*L/(T^3*O)"))
                if "SpecificHeat" not in m:
                    raise WriteError(
                        "Used material does not specify the necessary 'Specific Heat'."
                    )
                self._material(
                    name, "Heat Capacity",
                    self._convert(m["SpecificHeat"], "L^2/(T^2*O)"))

    def _handleElectrostatic(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElectrostatic"):
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
        # check if we need to update the equation
        self._updateElectrostaticSolver(equation)
        # output the equation parameters
        s = self._createLinearSolver(equation)
        s["Equation"] = "Stat Elec Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("StatElecSolve/StatElecSolver")
        s["Variable"] = self._getUniqueVarName("Potential")
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
        if (
            equation.CalculateCapacitanceMatrix is False
            and (equation.PotentialDifference != 0.0)
        ):
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
                )
            )
            equation.CapacitanceMatrixFilename = "cmatrix.dat"
        if not hasattr(equation, "ConstantWeights"):
            equation.addProperty(
                "App::PropertyBool",
                "ConstantWeights",
                "Electrostatic",
                "Use constant weighting for results"
            )
        if not hasattr(equation, "PotentialDifference"):
            equation.addProperty(
                "App::PropertyFloat",
                "PotentialDifference",
                "Electrostatic",
                (
                    "Potential difference in Volt for which capacitance is\n"
                    "calculated if 'CalculateCapacitanceMatrix' is false"
                )
            )
            equation.PotentialDifference = 0.0

    def _handleElectrostaticConstants(self):
        self._constant(
            "Permittivity Of Vacuum",
            self._getConstant("PermittivityOfVacuum", "T^4*I^2/(L^3*M)")
        )
        # https://forum.freecadweb.org/viewtopic.php?f=18&p=400959#p400959

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
                        float(m["RelativePermittivity"])
                    )

    def _handleElectrostaticBndConditions(self):
        for obj in self._getMember("Fem::ConstraintElectrostaticPotential"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.PotentialEnabled:
                        if hasattr(obj, "Potential"):
                            potential = float(obj.Potential.getValueAs("V"))
                            self._boundary(name, "Potential", potential)
                    if obj.PotentialConstant:
                        self._boundary(name, "Potential Constant", True)
                    if obj.ElectricInfinity:
                        self._boundary(name, "Electric Infinity BC", True)
                    if obj.ElectricForcecalculation:
                        self._boundary(name, "Calculate Electric Force", True)
                    if obj.CapacitanceBodyEnabled:
                        if hasattr(obj, "CapacitanceBody"):
                            self._boundary(name, "Capacitance Body", obj.CapacitanceBody)
                self._handled(obj)

    def _handleFlux(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerFlux"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getFluxSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)

    def _getFluxSolver(self, equation):
        s = self._createLinearSolver(equation)
        # check if we need to update the equation
        self._updateFluxSolver(equation)
        # output the equation parameters
        s["Equation"] = "Flux Solver"  # equation.Name
        s["Procedure"] = sifio.FileAttr("FluxSolver/FluxSolver")
        if equation.AverageWithinMaterials is True:
            s["Average Within Materials"] = equation.AverageWithinMaterials
        s["Calculate Flux"] = equation.CalculateFlux
        if equation.CalculateFluxAbs is True:
            s["Calculate Flux Abs"] = equation.CalculateFluxAbs
        if equation.CalculateFluxMagnitude is True:
            s["Calculate Flux Magnitude"] = equation.CalculateFluxMagnitude
        s["Calculate Grad"] = equation.CalculateGrad
        if equation.CalculateGradAbs is True:
            s["Calculate Grad Abs"] = equation.CalculateGradAbs
        if equation.CalculateGradMagnitude is True:
            s["Calculate Grad Magnitude"] = equation.CalculateGradMagnitude
        if equation.DiscontinuousGalerkin is True:
            s["Discontinuous Galerkin"] = equation.DiscontinuousGalerkin
        if equation.EnforcePositiveMagnitude is True:
            s["Enforce Positive Magnitude"] = equation.EnforcePositiveMagnitude
        s["Flux Coefficient"] = equation.FluxCoefficient
        s["Flux Variable"] = equation.FluxVariable
        s["Stabilize"] = equation.Stabilize
        return s

    def _updateFluxSolver(self, equation):
        # updates older Flux equations
        if not hasattr(equation, "AverageWithinMaterials"):
            equation.addProperty(
                "App::PropertyBool",
                "AverageWithinMaterials",
                "Flux",
                (
                    "Enforces continuity within the same material\n"
                    "in the 'Discontinuous Galerkin' discretization"
                )
            )
        if hasattr(equation, "Bubbles"):
            # Bubbles was removed because it is unused by Elmer for the flux solver
            equation.removeProperty("Bubbles")
        if not hasattr(equation, "CalculateFluxAbs"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateFluxAbs",
                "Flux",
                "Computes absolute of flux vector"
            )
        if not hasattr(equation, "CalculateFluxMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateFluxMagnitude",
                "Flux",
                "Computes magnitude of flux vector field"
            )
        if not hasattr(equation, "CalculateGradAbs"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateGradAbs",
                "Flux",
                "Computes absolute of gradient field"
            )
        if not hasattr(equation, "CalculateGradMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "CalculateGradMagnitude",
                "Flux",
                "Computes magnitude of gradient field"
            )
        if not hasattr(equation, "DiscontinuousGalerkin"):
            equation.addProperty(
                "App::PropertyBool",
                "DiscontinuousGalerkin",
                "Flux",
                (
                    "Enable if standard Galerkin approximation leads to\n"
                    "unphysical results when there are discontinuities"
                )
            )
        if not hasattr(equation, "EnforcePositiveMagnitude"):
            equation.addProperty(
                "App::PropertyBool",
                "EnforcePositiveMagnitude",
                "Flux",
                (
                    "If true, negative values of computed magnitude fields\n"
                    "are a posteriori set to zero."
                )
            )
        if not hasattr(equation, "FluxCoefficient"):
            equation.addProperty(
                "App::PropertyString",
                "FluxCoefficient",
                "Flux",
                "Name of proportionality coefficient\nto compute the flux"
            )

    def _handleElectricforce(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElectricforce"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getElectricforceSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)

    def _getElectricforceSolver(self, equation):
        s = self._createEmptySolver()
        s["Equation"] = "Electric Force"  # equation.Name
        s["Procedure"] = sifio.FileAttr("ElectricForce/StatElecForce")
        s["Stabilize"] = equation.Stabilize
        return s

    def _handleElasticity(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElasticity"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getElasticitySolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                self._handleElasticityEquation(activeIn, equation)
        if activeIn:
            self._handleElasticityConstants()
            self._handleElasticityBndConditions()
            self._handleElasticityInitial(activeIn)
            self._handleElasticityBodyForces(activeIn)
            self._handleElasticityMaterial(activeIn)

    def _getElasticitySolver(self, equation):
        s = self._createLinearSolver(equation)
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

    def _handleElasticityEquation(self, bodies, equation):
        for b in bodies:
            if equation.PlaneStress:
                self._equation(b, "Plane Stress", equation.PlaneStress)

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

    def _handleElasticityConstants(self):
        pass

    def _handleElasticityBndConditions(self):
        for obj in self._getMember("Fem::ConstraintPressure"):
            if obj.References:
                for name in obj.References[0][1]:
                    pressure = self._getFromUi(obj.Pressure, "MPa", "M/(L*T^2)")
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
                    force = self._getFromUi(obj.Force, "N", "M*L*T^-2")
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
        obj = self._getSingleMember("Fem::ConstraintSelfWeight")
        if obj is not None:
            for name in bodies:
                gravity = self._getConstant("Gravity", "L/T^2")
                m = self._getBodyMaterial(name).Material

                densityQuantity = Units.Quantity(m["Density"])
                dimension = "M/L^3"
                if name.startswith("Edge"):
                    # not tested, bernd
                    # TODO: test
                    densityQuantity.Unit = Units.Unit(-2, 1)
                    dimension = "M/L^2"
                density = self._convert(densityQuantity, dimension)

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
        # density
        # is needed for self weight constraints and frequency analysis
        density_needed = False
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerElasticity"):
                if equation.EigenAnalysis is True:
                    density_needed = True
                    break  # there could be a second equation without frequency
        gravObj = self._getSingleMember("Fem::ConstraintSelfWeight")
        if gravObj is not None:
            density_needed = True
        # temperature
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = self._getFromUi(tempObj.initialTemperature, "K", "O")
            for name in bodies:
                self._material(name, "Reference Temperature", refTemp)
        # get the material data for all boddies
        for obj in self._getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self._getAllBodies()
            )
            for name in (n for n in refs if n in bodies):
                if density_needed is True:
                    self._material(
                        name, "Density",
                        self._getDensity(m)
                    )
                self._material(
                    name, "Youngs Modulus",
                    self._getYoungsModulus(m)
                )
                self._material(
                    name, "Poisson ratio",
                    float(m["PoissonRatio"])
                )
                if tempObj:
                    self._material(
                        name, "Heat expansion Coefficient",
                        self._convert(m["ThermalExpansionCoefficient"], "O^-1")
                    )

    def _getDensity(self, m):
        density = self._convert(m["Density"], "M/L^3")
        if self._getMeshDimension() == 2:
            density *= 1e3
        return density

    def _getYoungsModulus(self, m):
        youngsModulus = self._convert(m["YoungsModulus"], "M/(L*T^2)")
        if self._getMeshDimension() == 2:
            youngsModulus *= 1e3
        return youngsModulus

    def _isMaterialFlow(self, body):
        m = self._getBodyMaterial(body).Material
        return "KinematicViscosity" in m

    def _handleFlow(self):
        activeIn = []
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerFlow"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self._getAllBodies()
                solverSection = self._getFlowSolver(equation)
                for body in activeIn:
                    if self._isMaterialFlow(body):
                        self._addSolver(body, solverSection)
                self._handleFlowEquation(activeIn, equation)
        if activeIn:
            self._handleFlowConstants()
            self._handleFlowBndConditions()
            self._handleFlowInitialVelocity(activeIn)
            # self._handleFlowInitial(activeIn)
            # self._handleFlowBodyForces(activeIn)
            self._handleFlowMaterial(activeIn)

    def _getFlowSolver(self, equation):
        # check if we need to update the equation
        self._updateFlowSolver(equation)
        # output the equation parameters
        s = self._createNonlinearSolver(equation)
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

    def _handleFlowConstants(self):
        gravity = self._getConstant("Gravity", "L/T^2")
        self._constant("Gravity", (0.0, -1.0, 0.0, gravity))

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

    def _handleFlowMaterial(self, bodies):
        tempObj = self._getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = self._getFromUi(tempObj.initialTemperature, "K", "O")
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
                        self._getDensity(m)
                    )
                if "ThermalConductivity" in m:
                    self._material(
                        name, "Heat Conductivity",
                        self._convert(m["ThermalConductivity"], "M*L/(T^3*O)")
                    )
                if "KinematicViscosity" in m:
                    density = self._getDensity(m)
                    kViscosity = self._convert(m["KinematicViscosity"], "L^2/T")
                    self._material(
                        name, "Viscosity", kViscosity * density)
                if "ThermalExpansionCoefficient" in m:
                    value = self._convert(m["ThermalExpansionCoefficient"], "O^-1")
                    if value > 0:
                        self._material(
                            name, "Heat expansion Coefficient", value)
                if "ReferencePressure" in m:
                    pressure = self._convert(m["ReferencePressure"], "M/(L*T^2)")
                    self._material(name, "Reference Pressure", pressure)
                if "SpecificHeatRatio" in m:
                    self._material(
                        name, "Specific Heat Ratio",
                        float(m["SpecificHeatRatio"])
                    )
                if "CompressibilityModel" in m:
                    self._material(
                        name, "Compressibility Model",
                        m["CompressibilityModel"])

    def _handleFlowInitialVelocity(self, bodies):
        obj = self._getSingleMember("Fem::ConstraintInitialFlowVelocity")
        if obj is not None:
            for name in bodies:
                if obj.VelocityXEnabled:
                    velocity = self._getFromUi(obj.VelocityX, "m/s", "L/T")
                    self._initial(name, "Velocity 1", velocity)
                if obj.VelocityYEnabled:
                    velocity = self._getFromUi(obj.VelocityY, "m/s", "L/T")
                    self._initial(name, "Velocity 2", velocity)
                if obj.VelocityZEnabled:
                    velocity = self._getFromUi(obj.VelocityZ, "m/s", "L/T")
                    self._initial(name, "Velocity 3", velocity)
            self._handled(obj)

    def _handleFlowBndConditions(self):
        for obj in self._getMember("Fem::ConstraintFlowVelocity"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.VelocityXEnabled:
                        velocity = self._getFromUi(obj.VelocityX, "m/s", "L/T")
                        self._boundary(name, "Velocity 1", velocity)
                    if obj.VelocityYEnabled:
                        velocity = self._getFromUi(obj.VelocityY, "m/s", "L/T")
                        self._boundary(name, "Velocity 2", velocity)
                    if obj.VelocityZEnabled:
                        velocity = self._getFromUi(obj.VelocityZ, "m/s", "L/T")
                        self._boundary(name, "Velocity 3", velocity)
                    if obj.NormalToBoundary:
                        self._boundary(name, "Normal-Tangential Velocity", True)
                self._handled(obj)

    def _handleFlowEquation(self, bodies, equation):
        for b in bodies:
            if equation.Convection != "None":
                self._equation(b, "Convection", equation.Convection)
            if equation.MagneticInduction is True:
                self._equation(b, "Magnetic Induction", equation.MagneticInduction)

    def _createEmptySolver(self):
        s = sifio.createSection(sifio.SOLVER)
        return s

    def _hasExpression(self, equation):
        obj = None
        exp = None
        for (obj, exp) in equation.ExpressionEngine:
            if obj == equation:
                return exp
        return None

    def _updateLinearSolver(self, equation):
        if self._hasExpression(equation) != equation.LinearTolerance:
            equation.setExpression("LinearTolerance", str(equation.LinearTolerance))
        if self._hasExpression(equation) != equation.SteadyStateTolerance:
            equation.setExpression("SteadyStateTolerance", str(equation.SteadyStateTolerance))

    def _createLinearSolver(self, equation):
        # first check if we have to update
        self._updateLinearSolver(equation)
        # write the solver
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

    def _updateNonlinearSolver(self, equation):
        if self._hasExpression(equation) != equation.NonlinearTolerance:
            equation.setExpression("NonlinearTolerance", str(equation.NonlinearTolerance))
        if self._hasExpression(equation) != equation.NonlinearNewtonAfterTolerance:
            equation.setExpression(
                "NonlinearNewtonAfterTolerance",
                str(equation.NonlinearNewtonAfterTolerance)
            )

    def _createNonlinearSolver(self, equation):
        # first check if we have to update
        self._updateNonlinearSolver(equation)
        # write the linear solver
        s = self._createLinearSolver(equation)
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
        # Since FreeCAD meshes are in mm we let Elmer scale it
        # _handleSimulation(self).
        # To get it back in the original size we let Elmer scale it back
        s["Coordinate Scaling Revert"] = True
        s["Equation"] = "ResultOutput"
        s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr("case")
        s["Vtu Format"] = True
        if self.unit_schema == Units.Scheme.SI2:
            s["Coordinate Scaling Revert"] = True
            Console.PrintMessage(
                "'Coordinate Scaling Revert = Logical True' was "
                "inserted into the solver input file.\n"
            )
        for name in self._getAllBodies():
            self._addSolver(name, s)

    def _writeSif(self):
        sifPath = os.path.join(self.directory, _SIF_NAME)
        with open(sifPath, "w") as fstream:
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
        return membertools.get_member(self.analysis, t)

    def _getSingleMember(self, t):
        return membertools.get_single_member(self.analysis, t)


class WriteError(Exception):
    pass

##  @}
