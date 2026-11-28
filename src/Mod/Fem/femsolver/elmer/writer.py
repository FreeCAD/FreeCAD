# SPDX-License-Identifier: LGPL-2.1-or-later

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

import math
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
from .. import settings
from femtools import constants
from femtools import femutils
from femtools import materialtables
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
from .equations import staticcurrent_writer as SC_writer

_STARTINFO_NAME = "ELMERSOLVER_STARTINFO"
_SIF_NAME = "case.sif"
_ELMERGRID_IFORMAT = "8"
_ELMERGRID_OFORMAT = "2"
_COORDS_NON_MAGNETO_2D = [
    "Polar 2D",
    "Polar 3D",
    "Cartesian 3D",
    "Cylindric",
    "Cylindric Symmetric",
]
RESULT_FILENAME = "FreeCAD"
RESULT_DIRECTORY = "Result"
SCALARS_DIRECTORY = "Scalars"
FRAMES_INFO = [None, Units.Unit(""), ""]


def _getAllSubObjects(obj):
    s = ["Solid" + str(i + 1) for i in range(len(obj.Shape.Solids))]
    s.extend("Face" + str(i + 1) for i in range(len(obj.Shape.Faces)))
    s.extend("Edge" + str(i + 1) for i in range(len(obj.Shape.Edges)))
    s.extend("Vertex" + str(i + 1) for i in range(len(obj.Shape.Vertexes)))
    return s


class Writer:

    def __init__(self, solver, directory, testmode=False):
        self.analysis = solver.getParentGroup()
        self.solver = solver
        self.directory = directory
        Console.PrintMessage(f"Write elmer input files to: {self.directory}\n")
        self.testmode = testmode
        self._usedVarNames = set()
        self._builder = sifio.Builder()
        self._handledObjects = set()
        self._solverIds = None
        self._unwrittenTables = {}
        self._handleUnits()
        self._handleConstants()
        self.scanning_file = "scanning.dat"
        # to set eigen analysis in result output block
        self.eigen_analysis = False
        # frames info: [mode, unit, description]. Set in solver writers
        self.frames_info = FRAMES_INFO
        self.frames_values_file = ""

    def getHandledConstraints(self):
        return self._handledObjects

    def _writeBlocks(self):
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
        self._handleStaticCurrent()
        self._handleMultiframeMode()
        self._addOutputSolver()

    def _writtenTables(self):
        """(body, keyword, table) of every temperature table the sif holds"""
        # A later material can put a scalar over a table, only what reached the sif counts
        tables = []
        for body in self._builder.getBodyNames():
            material = self._builder.getBodySection(body, sifio.MATERIAL)
            for key in material.keys() if material is not None else []:
                if isinstance(material[key], TemperatureTable):
                    tables.append((body, key, material[key]))
        return tables

    def solverOrderNotes(self):
        """Solvers reading an electrical conductivity table before the Heat solver of the body"""
        # Elmer runs its solvers by their number, the one the written sif gave them
        steady = self.solver.SimulationType == "Steady State"
        if self._solverIds is None or (steady and self.solver.SteadyStateMaxIterations > 1):
            return []
        if steady:
            effect = (
                "with one steady state iteration it reads the electrical conductivity table at "
                "the starting temperature. Give the Heat equation a higher Priority, or raise "
                "Steady State Max Iterations."
            )
        else:
            effect = (
                "at each time step it reads the electrical conductivity table at the temperature "
                "of the previous time step. Give the Heat equation a higher Priority."
            )
        number = self._solverIds.getId
        notes = []
        electric = {b for b, key, _table in self._writtenTables() if key == "Electric Conductivity"}
        for body in sorted(electric):
            equation = self._builder.getBodySection(body, sifio.EQUATION)
            if equation is None or "Active Solvers" not in equation:
                continue
            # Only Exec Solver = Always, the default, runs in the coupled loop (MainUtils.F90)
            active = [
                s
                for s in equation["Active Solvers"]
                if "Exec Solver" not in s or str(s["Exec Solver"]).strip().lower() == "always"
            ]
            solvers = [(s, s["Procedure"] if "Procedure" in s else "") for s in active]
            heats = sorted(
                (s for s, procedure in solvers if procedure == "HeatSolve/HeatSolver"), key=number
            )
            for solver, procedure in solvers:
                # One Heat run earlier is enough, the reader sees the temperature it computed
                if procedure not in CONDUCTIVITY_READERS or not heats:
                    continue
                if number(heats[0]) < number(solver):
                    continue
                note = f"{solver['Equation']} runs before {heats[0]['Equation']}: {effect}"
                if note not in notes:
                    notes.append(note)
        return notes

    def write_solver_input(self):
        self._writeBlocks()
        # Judged on what reaches the sif, a later material can put a scalar over a table
        written = self._writtenTables()
        self.checkElectricalTables(written)
        self._writeSif()
        self._writeStartinfo()
        # Told once the input is written, a refused or failed write announces nothing
        notes = []
        for _body, _key, table in written:
            if table.note not in notes:
                notes.append(table.note)
        for label, skipped in self._unwrittenTables.values():
            verb = "isn't" if len(skipped) == 1 else "aren't"
            note = (
                f"{label}: {' and '.join(skipped)} {verb} written for Elmer yet, it uses the "
                "single values of the card where there are some."
            )
            if skipped and note not in notes:
                notes.append(note)
        for note in notes + self.solverOrderNotes():
            Console.PrintWarning(f"{note}\n")

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
        self.unit_schema = Units.Scheme.Internal
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
        self.unit_schema = param.GetInt("UserSchema", Units.Scheme.Internal)
        if self.unit_schema == Units.Scheme.Internal:
            Console.PrintMessage(
                "The FreeCAD standard unit schema mm/kg/s is used. "
                "Elmer sif-file writing is however done in SI units.\n"
            )
        elif self.unit_schema == Units.Scheme.MKS:
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
        elif self.unit_schema == Units.Scheme.FEM:
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
        elif self.unit_schema > Units.Scheme.MKS and self.unit_schema != Units.Scheme.FEM:
            Console.PrintMessage(
                "Unit schema: {} not supported by Elmer writer. "
                "The FreeCAD standard unit schema mm/kg/s is used. "
                "Elmer sif-file writing is done in Standard FreeCAD units.\n".format(
                    Units.listSchemas(self.unit_schema)
                )
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

    def _handleRedifinedConstants(self):
        """
        redefine constants in self.constsdef according constant redefine objects
        """
        objs = self.getMember("Fem::ConstantVacuumPermittivity")
        if len(objs) == 1:
            permittivity = float(objs[0].VacuumPermittivity.getValueAs("F/m"))
            # since the base unit of FC is in mm, we must scale it to get plain SI
            permittivity = permittivity * 1e-9
            Console.PrintLog(f"Overwriting vacuum permittivity with: {permittivity}\n")
            self.constsdef["PermittivityOfVacuum"] = "{} {}".format(permittivity, "F/m")
            self.handled(objs[0])
        elif len(objs) > 1:
            Console.PrintError(
                "More than one permittivity constant overwriting objects ({} objs). "
                "The permittivity constant overwriting is ignored.\n".format(len(objs))
            )

    def _handleSimulation(self):
        self._simulation("Coordinate System", self.solver.CoordinateSystem)
        self._simulation("Coordinate Mapping", (1, 2, 3))
        # Elmer uses SI base units, but our mesh is in mm, therefore we must tell
        # the solver that we have another scale
        self._simulation("Coordinate Scaling", 0.001)
        self._simulation("Simulation Type", self.solver.SimulationType)
        param = ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Elmer")
        out_level = 10 if self.testmode else param.GetInt("MaxOutputLevel", 10)
        self._simulation("Max Output Level", out_level)
        if self.solver.SimulationType == "Steady State":
            self._simulation("Steady State Max Iterations", self.solver.SteadyStateMaxIterations)
            self._simulation("Steady State Min Iterations", self.solver.SteadyStateMinIterations)
        if self.solver.SimulationType == "Scanning" or self.solver.SimulationType == "Transient":
            self._simulation("BDF Order", self.solver.BDFOrder)
            self._simulation("Output Intervals", self.solver.OutputIntervals)
            self._simulation("Timestep Intervals", self.solver.TimestepIntervals)
            self._simulation("Timestep Sizes", self.solver.TimestepSizes)
            self._simulation("Timestepping Method", "BDF")
        self._simulation("Use Mesh Names", True)

    def _writeStartinfo(self):
        path = os.path.join(self.directory, _STARTINFO_NAME)
        with open(path, "w") as f:
            f.write(f"{_SIF_NAME}\n")

    # -------------------------------------------------------------------------------------------
    # Deformation

    def _handleDeformation(self):
        DEFW = DEF_writer.DeformationWriter(self, self.solver)
        for equation in self.solver.Group:
            activeIn = []
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
        for equation in self.solver.Group:
            activeIn = []
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
                solverEigenSection = ELW.getEigenSolver(equation)
                for body in activeIn:
                    if not self.isBodyMaterialFluid(body):
                        self._addSolver(body, solverSection)
                        ELW.handleElasticityEquation(activeIn, equation)
                        if solverEigenSection is not None:
                            self._addSolver(body, solverEigenSection)
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
        for equation in self.solver.Group:
            activeIn = []
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
                    ESW.handleElectrostaticBodyForces()
                    ESW.handleElectrostaticMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # Electricforce

    def _handleElectricforce(self):
        EFW = EF_writer.EFwriter(self, self.solver)
        for equation in self.solver.Group:
            activeIn = []
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
        for equation in self.solver.Group:
            activeIn = []
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
        for equation in self.solver.Group:
            activeIn = []
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

    def heatBodies(self):
        # The bodies a Temperature field exists on, as _handleHeat assigns them
        bodies = set()
        for equation in self.solver.Group:
            if femutils.is_of_type(equation, "Fem::EquationElmerHeat"):
                if equation.References:
                    bodies.update(equation.References[0][1])
                else:
                    bodies.update(self.getAllBodies())
        return bodies

    def _handleHeat(self):
        HeatW = heat_writer.Heatwriter(self, self.solver)
        for equation in self.solver.Group:
            activeIn = []
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
        for equation in self.solver.Group:
            activeIn = []
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
        for equation in self.solver.Group:
            activeIn = []
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
                        "Possible is:\n'Cartesian 2D' or 'Axi Symmetric'".format(
                            self.solver.CoordinateSystem
                        )
                    )

                solverSection = MgDyn2D.getMagnetodynamic2DSolver(equation)
                solverPostSection = MgDyn2D.getMagnetodynamic2DSolverPost(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                    self._addSolver(body, solverPostSection)
                    MgDyn2D.handleMagnetodynamic2DEquation(activeIn, equation)
                if activeIn:
                    MgDyn2D.handleMagnetodynamic2DConstants()
                    MgDyn2D.handleMagnetodynamic2DBndConditions(equation)
                    MgDyn2D.handleMagnetodynamic2DBodyForces(activeIn, equation)
                    MgDyn2D.handleMagnetodynamic2DMaterial(activeIn)

    # -------------------------------------------------------------------------------------------
    # StaticCurrent

    def _handleStaticCurrent(self):
        SCW = SC_writer.SCwriter(self, self.solver)
        for equation in self.solver.Group:
            activeIn = []
            if femutils.is_of_type(equation, "Fem::EquationElmerStaticCurrent"):
                if equation.References:
                    activeIn = equation.References[0][1]
                else:
                    activeIn = self.getAllBodies()
                solverSection = SCW.getStaticCurrentSolver(equation)
                for body in activeIn:
                    self._addSolver(body, solverSection)
                SCW.handleStaticCurrentBodyForces(activeIn, equation)

                if activeIn:
                    SCW.handleStaticCurrentConstants()
                    SCW.handleStaticCurrentBndConditions()
                    SCW.handleStaticCurrentMaterial(activeIn)

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
                ),
                locked=True,
            )
        if not hasattr(equation, "IdrsParameter"):
            equation.addProperty(
                "App::PropertyIntegerConstraint",
                "IdrsParameter",
                "Linear System",
                "Parameter for iterative method 'Idrs'",
                locked=True,
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
            s["Linear System Direct Method"] = equation.LinearDirectMethod
        elif equation.LinearSolverType == "Iterative":
            s["Linear System Iterative Method"] = equation.LinearIterativeMethod
            if equation.LinearIterativeMethod == "BiCGStabl":
                s["BiCGstabl polynomial degree"] = equation.BiCGstablDegree
            if equation.LinearIterativeMethod == "Idrs":
                s["Idrs Parameter"] = equation.IdrsParameter
            s["Linear System Max Iterations"] = equation.LinearIterations
            s["Linear System Convergence Tolerance"] = equation.LinearTolerance
            s["Linear System Preconditioning"] = equation.LinearPreconditioning
        s["Steady State Convergence Tolerance"] = equation.SteadyStateTolerance
        s["Linear System Abort Not Converged"] = False
        s["Linear System Residual Output"] = 1
        s["Linear System Precondition Recompute"] = 1
        return s

    def _updateNonlinearSolver(self, equation):
        if self._hasExpression(equation) != equation.NonlinearTolerance:
            equation.setExpression("NonlinearTolerance", str(equation.NonlinearTolerance))
        if self._hasExpression(equation) != equation.NonlinearNewtonAfterTolerance:
            equation.setExpression(
                "NonlinearNewtonAfterTolerance", str(equation.NonlinearNewtonAfterTolerance)
            )

    def createNonlinearSolver(self, equation):
        # first check if we have to update
        self._updateNonlinearSolver(equation)
        # write the linear solver
        s = self.createLinearSolver(equation)
        # write the nonlinear solver
        s["Nonlinear System Max Iterations"] = equation.NonlinearIterations
        s["Nonlinear System Convergence Tolerance"] = equation.NonlinearTolerance
        s["Nonlinear System Relaxation Factor"] = equation.RelaxationFactor
        s["Nonlinear System Newton After Iterations"] = equation.NonlinearNewtonAfterIterations
        s["Nonlinear System Newton After Tolerance"] = equation.NonlinearNewtonAfterTolerance
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
        return density

    def convertTable(self, obj, table, unit, quantity):
        """A FEM material table as an Elmer value, None when the table is empty"""
        rows = materialtables.sorted_rows(getattr(obj, table, []), table, obj.Label)
        points = [(temperature, self.convert(cells[0], unit)) for temperature, cells in rows]
        return temperature_table(points, f"{obj.Label} {quantity}")

    def electricalConductivityTable(self, obj):
        """The electrical conductivity table of a material, None without one"""
        return self.convertTable(
            obj, "ElectricalConductivityTable", "S/m", "electrical conductivity"
        )

    def elasticTables(self, obj, name):
        # E, nu and alpha against temperature are a later step, a later material on the body wins
        skipped = [table for table in ("ElasticTable", "ExpansionTable") if getattr(obj, table, [])]
        self._unwrittenTables[name] = (obj.Label, skipped)

    def checkElectricalTables(self, written):
        # Without a Temperature field Elmer stops, or reads zero where it's missing
        for body, key, table in written:
            if key == "Electric Conductivity" and body not in self.heatBodies():
                raise WriteError(
                    f"{table.label}: the table on {body} needs a Heat equation active on "
                    f"{body}, Elmer has no temperature to read it at. Add one with a higher "
                    "Priority than the equation reading the table, or clear the table to use "
                    "the single conductivity value."
                )

    def _hasExpression(self, equation):
        for obj, exp in equation.ExpressionEngine:
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
        obj = self.getMesh()
        if not obj.FemMesh.Groups:
            raise RuntimeError(f"Mesh object '{obj.Label}' has no groups, please remesh\n")
        bodyCount = 0
        prefix = ""
        if obj.Shape.Shape.Solids:
            prefix = "Solid"
            bodyCount = len(obj.Shape.Shape.Solids)
        elif obj.Shape.Shape.Faces:
            prefix = "Face"
            bodyCount = len(obj.Shape.Shape.Faces)
        elif obj.Shape.Shape.Edges:
            prefix = "Edge"
            bodyCount = len(obj.Shape.Shape.Edges)
        return [prefix + str(i + 1) for i in range(bodyCount)]

    def getMeshDimension(self):
        obj = self.getMesh()
        if obj.Shape.Shape.Solids:
            return 3
        if obj.Shape.Shape.Faces:
            return 2
        if obj.Shape.Shape.Edges:
            return 1
        return None

    def getMesh(self):
        return membertools.get_mesh_to_solve(self.analysis)

    def getCoordSystemDimension(self):
        dim = 0
        match self.solver.CoordinateSystem:
            case "Cartesian":
                # defined by the mesh
                dim = self.getMeshDimension()
            case "Cartesian 1D":
                dim = 1
            case "Cartesian 2D" | "Polar 2D" | "Cylindric Symmetric" | "Axi Symmetric":
                dim = 2
            case "Cartesian 3D" | "Polar 3D" | "Cylindric":
                dim = 3
        return dim

    def _addOutputSolver(self):
        s = sifio.createSection(sifio.SOLVER)
        # Since FreeCAD meshes are in mm we let Elmer scale it
        # _handleSimulation(self).
        # To get it back in the original size we let Elmer scale it back
        s["Coordinate Scaling Revert"] = True
        s["Equation"] = "ResultOutput"
        if self.solver.SimulationType in ["Transient", "Scanning"]:
            # we must execute the post solver every time we output a result
            # therefore we must use the same as self.solver.OutputIntervals
            s["Exec Intervals"] = self.solver.OutputIntervals
        else:
            s["Exec Solver"] = "After simulation"
        s["Procedure"] = sifio.FileAttr("ResultOutputSolve/ResultOutputSolver")
        s["Output File Name"] = sifio.FileAttr(RESULT_FILENAME)
        s["Output Directory"] = sifio.FileAttr(RESULT_DIRECTORY)
        if self.eigen_analysis:
            s["Eigen Analysis"] = True
        s["Vtu Format"] = True
        s["Binary Output"] = self.solver.BinaryOutput
        s["Save Geometry Ids"] = self.solver.SaveGeometryIndex
        s["Vtu Time Collection"] = True

        for name in self.getAllBodies():
            self._addSolver(name, s)

    def _handleMultiframeMode(self):
        # in steady state, the frames are generated by the solvers
        # transient and scanning take precedence over other multiframe modes
        if self.solver.SimulationType == "Scanning":
            s = sifio.createSection(sifio.SOLVER)
            s["Procedure"] = sifio.FileAttr("SaveData/SaveScalars")
            s["Output Directory"] = sifio.FileAttr(SCALARS_DIRECTORY)
            s["Parallel Reduce"] = True
            s["Filename"] = sifio.FileAttr(self.scanning_file)
            s["Variable 1"] = "Time"
            # ignore scalars from solvers
            s["Scalars Prefix"] = ""
            self.frames_info = ["scanning", Units.Unit(""), "Scanning step"]
            self.frames_values_file = self.scanning_file
            self.eigen_analysis = False
            for name in self.getAllBodies():
                self._addSolver(name, s)
        elif self.solver.SimulationType == "Transient":
            # special case. No need set SaveScalars to collect multiframes
            self.frames_info = [None, Units.Unit("s"), "Timestep"]
            self.frames_values_file = ""
            self.eigen_analysis = False

    def _writeSif(self):
        sifPath = os.path.join(self.directory, _SIF_NAME)
        with open(sifPath, "w") as fstream:
            sif = sifio.Sif(self._builder)
            sif.write(fstream)
        self._solverIds = sif.ids

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


class TemperatureTable(str):
    """An Elmer Variable Temperature value that keeps the label and range note of its table

    A str, since sifio writes as it is any string that starts with Variable, and the
    builder stores it like any other value.
    """

    def __new__(cls, value, label, note):
        table = super().__new__(cls, value)
        table.label = label
        table.note = note
        return table


# Elmer extrapolates linearly outside a table (SearchInterval in fem/src/GeneralUtils.F90)
# CalculiX keeps the edge value. A flat point on each side gives Elmer the same behaviour
TABLE_FLOOR_K = 0.0
TABLE_CEILING_K = 1.0e6

# The Elmer procedures that read Electric Conductivity, CalcFields for the Joule heat
# solverOrderNotes checks them against the Heat solver of each body
CONDUCTIVITY_READERS = (
    "StatCurrentSolve/StatCurrentSolver",
    "MagnetoDynamics/WhitneyAVSolver",
    "MagnetoDynamics/WhitneyAVHarmonicSolver",
    "MagnetoDynamics/MagnetoDynamicsCalcFields",
    "MagnetoDynamics2D/MagnetoDynamics2D",
    "MagnetoDynamics2D/MagnetoDynamics2DHarmonic",
)


def temperature_table(points, label):
    """Elmer value of (temperature in K, value) points sorted by temperature

    A single point is written as a scalar: Elmer reads a one row table as value * T
    (InterpolateCurve in fem/src/GeneralUtils.F90).
    """
    # No table, the caller writes the single value of the card
    if not points:
        return None
    check_finite(points, label)
    if len(points) == 1:
        return points[0][1]
    # Compared as written, 2 temperatures printed alike make Elmer refuse the table
    for (first, _), (second, _) in zip(points, points[1:]):
        if float(f"{first:.13g}") == float(f"{second:.13g}"):
            raise WriteError(
                f"{label}: the temperature table gives two values at {first:g} K, keep one."
            )
    # The ends as written, 999999.99999999 prints as the ceiling itself
    first, last = (float(f"{t:.13g}") for t in (points[0][0], points[-1][0]))
    low, high = flat_point(first, -1), flat_point(last, 1)
    # A margin past 1e308 K overflows, the card itself is finite
    if not (math.isfinite(low) and math.isfinite(high)):
        raise WriteError(
            f"{label}: the temperature table reaches {max(abs(first), abs(last)):g} K, "
            "too large to add the flat points that keep its end values."
        )
    note = table_range_note(points, label)
    points = [(low, points[0][1])] + points + [(high, points[-1][1])]
    rows = "\n".join(f"    {temperature:.13g} {value:.13g}" for temperature, value in points)
    return TemperatureTable(f"Variable Temperature\n  Real\n{rows}\n  End", label, note)


def flat_point(end, side):
    """Temperature of the flat point past an end of a table, side -1 below it and 1 above

    Any flat segment past an end gives Elmer a zero slope to extrapolate with. The floor
    or the ceiling when the end lies inside them, else a margin that grows with the end,
    past 1e13 K one kelvin vanishes in the 13 written digits.
    """
    bound = TABLE_FLOOR_K if side < 0 else TABLE_CEILING_K
    if side * (bound - end) > 0:
        return bound
    return end + side * (1.0 + abs(end))


def check_finite(points, label):
    # A quantity like 1e309 K parses to inf and would reach the sif as is
    for temperature, value in points:
        if not (math.isfinite(temperature) and math.isfinite(value)):
            raise WriteError(
                f"{label}: the temperature table holds a value Elmer can't read "
                f"({temperature:g} K, {value:g}), check the card."
            )


def table_range_note(points, label):
    # The user can't see from the sif alone that the result left the data
    low, high = points[0][0], points[-1][0]
    return (
        f"{label}: the table covers {low:g} K to {high:g} K, "
        f"outside it the value at the nearest end is kept."
    )


##  @}
