# ***************************************************************************
# *   Copyright (c) 2015 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver CalculiX writer"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import codecs
import os
import six
import time
from os.path import join

import FreeCAD
from FreeCAD import Units

from . import con_centrif
from . import con_contact
from . import con_displacement
from . import con_fixed
from . import con_fluidsection
from . import con_force
from . import con_heatflux
from . import con_initialtemperature as con_initialtemp
from . import con_planerotation
from . import con_pressure
from . import con_sectionprint
from . import con_selfweight
from . import con_temperature
from . import con_tie
from . import con_transform
from .. import writerbase
from femmesh import meshtools
from femtools import constants


# Interesting forum topic: https://forum.freecadweb.org/viewtopic.php?&t=48451
# TODO somehow set units at beginning and every time a value is retrieved use this identifier
# this would lead to support of unit system, force might be retrieved in base writer!


# the following text will be at the end of the main calculix input file
units_information = """***********************************************************
**  About units:
**  See ccx manual, ccx does not know about any unit.
**  Golden rule: The user must make sure that the numbers he provides have consistent units.
**  The user is the FreeCAD calculix writer module ;-)
**
**  The unit system which is used at Guido Dhondt's company: mm, N, s, K
**  Since Length and Mass are connected by Force, if Length is mm the Mass is in t to get N
**  The following units are used to write to inp file:
**
**  Length: mm (this includes the mesh geometry)
**  Mass: t
**  TimeSpan: s
**  Temperature: K
**
**  This leads to:
**  Force: N
**  Pressure: N/mm^2
**  Density: t/mm^3
**  Gravity: mm/s^2
**  Thermal conductivity: t*mm/K/s^3 (same as W/m/K)
**  Specific Heat: mm^2/s^2/K (same as J/kg/K)
"""


class FemInputWriterCcx(writerbase.FemInputWriter):
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None
    ):
        writerbase.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            member,
            dir_name
        )
        self.mesh_name = self.mesh_object.Name
        self.include = join(self.dir_name, self.mesh_name)
        self.file_name = self.include + ".inp"
        self.femmesh_file = ""  # the file the femmesh is in, no matter if one or split input file
        self.gravity = int(Units.Quantity(constants.gravity()).getValueAs("mm/s^2"))  # 9820 mm/s2

    # ********************************************************************************************
    # write calculix input
    def write_calculix_input_file(self):
        timestart = time.process_time()
        FreeCAD.Console.PrintMessage("Start writing CalculiX input file\n")
        FreeCAD.Console.PrintMessage("Write ccx input file to: {}\n".format(self.file_name))
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.mesh_name  -->  {}\n".format(self.mesh_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.dir_name  -->  {}\n".format(self.dir_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.include  -->  {}\n".format(self.mesh_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.file_name  -->  {}\n".format(self.file_name)
        )

        self.get_mesh_data()
        self.write_file()

        FreeCAD.Console.PrintMessage(
            "Writing time CalculiX input file: {} seconds \n\n"
            .format(round((time.process_time() - timestart), 2))
        )
        if self.femelement_count_test is True:
            return self.file_name
        else:
            FreeCAD.Console.PrintError(
                "Problems on writing input file, check report prints.\n\n"
            )
            return ""

    def get_mesh_data(self):
        FreeCAD.Console.PrintMessage(
            "Get mesh data for "
            "node sets (groups), surface sets (groups) and element sets (groups)\n"
        )

        # materials and element geometry element sets getter
        self.get_element_sets_material_and_femelement_geometry()

        # constraints element sets getter
        self.get_constraints_centrif_elements()

        # constraints node sets getter
        self.get_constraints_fixed_nodes()
        self.get_constraints_displacement_nodes()
        self.get_constraints_planerotation_nodes()

        # constraints suface sets getter
        self.get_constraints_contact_faces()
        self.get_constraints_tie_faces()
        self.get_constraints_sectionprint_faces()
        self.get_constraints_transform_nodes()
        self.get_constraints_temperature_nodes()

        # constraints sets with constraint data
        self.get_constraints_force_nodeloads()
        self.get_constraints_pressure_faces()
        self.get_constraints_heatflux_faces()

    def write_file(self):
        FreeCAD.Console.PrintMessage("Start writing input file\n")
        if self.solver_obj.SplitInputWriter is True:
            FreeCAD.Console.PrintMessage("Splitted input file.\n")
            self.split_inpfile = True
        else:
            FreeCAD.Console.PrintMessage("One monster input file.\n")
            self.split_inpfile = False

        # mesh
        inpfile = self.write_mesh()

        # element sets for materials and element geometry
        # self.write_element_sets_material_and_femelement_geometry(inpfile)
        self.write_element_sets_material_and_femelement_type(inpfile)

        if self.fluidsection_objects:
            # some fluidsection objs need special treatment, ccx_elsets are needed for this
            inpfile = con_fluidsection.handle_fluidsection_liquid_inlet_outlet(inpfile, self)

        # element sets constraints
        self.write_constraints_sets(inpfile, self.centrif_objects, con_centrif)

        # node sets
        self.write_constraints_sets(inpfile, self.fixed_objects, con_fixed)
        self.write_constraints_sets(inpfile, self.displacement_objects, con_displacement)
        self.write_constraints_sets(inpfile, self.planerotation_objects, con_planerotation)
        self.write_constraints_sets(inpfile, self.transform_objects, con_transform)
        self.write_constraints_sets(inpfile, self.temperature_objects, con_temperature)

        # surface sets
        self.write_constraints_sets(inpfile, self.contact_objects, con_contact)
        self.write_constraints_sets(inpfile, self.tie_objects, con_tie)
        self.write_constraints_sets(inpfile, self.sectionprint_objects, con_sectionprint)

        # materials and fem element types
        self.write_materials(inpfile)
        self.write_constraints_data(inpfile, self.initialtemperature_objects, con_initialtemp)
        # self.write_femelement_geometry(inpfile)
        self.write_femelementsets(inpfile)

        # constraints independent from steps
        self.write_constraints_data(inpfile, self.planerotation_objects, con_planerotation)
        self.write_constraints_data(inpfile, self.contact_objects, con_contact)
        self.write_constraints_data(inpfile, self.tie_objects, con_tie)
        self.write_constraints_data(inpfile, self.transform_objects, con_transform)

        # step begin
        self.write_step_begin(inpfile)

        # constraints dependent from steps
        self.write_constraints_data(inpfile, self.fixed_objects, con_fixed)
        self.write_constraints_data(inpfile, self.displacement_objects, con_displacement)
        self.write_constraints_data(inpfile, self.sectionprint_objects, con_sectionprint)
        self.write_constraints_data(inpfile, self.selfweight_objects, con_selfweight)
        self.write_constraints_data(inpfile, self.centrif_objects, con_centrif)
        self.write_constraints_sets(inpfile, self.force_objects, con_force)
        self.write_constraints_sets(inpfile, self.pressure_objects, con_pressure)
        self.write_constraints_data(inpfile, self.temperature_objects, con_temperature)
        self.write_constraints_sets(inpfile, self.heatflux_objects, con_heatflux)
        con_fluidsection.write_constraints_fluidsection(inpfile, self)

        # output and step end
        self.write_outputs_types(inpfile)
        self.write_step_end(inpfile)

        # footer
        self.write_footer(inpfile)
        inpfile.close()

    # ********************************************************************************************
    # mesh
    def write_mesh(self):
        # write mesh to file
        element_param = 1  # highest element order only
        group_param = False  # do not write mesh group data
        if self.split_inpfile is True:
            write_name = "femesh"
            file_name_split = self.mesh_name + "_" + write_name + ".inp"
            self.femmesh_file = join(self.dir_name, file_name_split)

            self.femmesh.writeABAQUS(
                self.femmesh_file,
                element_param,
                group_param
            )

            # Check to see if fluid sections are in analysis and use D network element type
            if self.fluidsection_objects:
                meshtools.write_D_network_element_to_inputfile(self.femmesh_file)

            inpfile = codecs.open(self.file_name, "w", encoding="utf-8")
            inpfile.write("***********************************************************\n")
            inpfile.write("** {}\n".format(write_name))
            inpfile.write("*INCLUDE,INPUT={}\n".format(file_name_split))

        else:
            self.femmesh_file = self.file_name
            self.femmesh.writeABAQUS(
                self.femmesh_file,
                element_param,
                group_param
            )

            # Check to see if fluid sections are in analysis and use D network element type
            if self.fluidsection_objects:
                # inpfile is closed
                meshtools.write_D_network_element_to_inputfile(self.femmesh_file)

            # reopen file with "append" to add all the rest
            inpfile = codecs.open(self.femmesh_file, "a", encoding="utf-8")
            inpfile.write("\n\n")

        return inpfile

    # ********************************************************************************************
    # write constraint node sets, constraint face sets, constraint element sets
    def write_constraints_sets(
        self,
        f,
        femobjs,
        con_module
    ):
        if not femobjs:
            return

        analysis_types = con_module.get_analysis_types()
        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        def constraint_sets_loop_writing(the_file, femobjs, write_before, write_after):
            if write_before != "":
                f.write(write_before)
            for femobj in femobjs:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                the_obj = femobj["Object"]
                f.write("** {}\n".format(the_obj.Label))
                con_module.write_meshdata_constraint(the_file, femobj, the_obj, self)
            if write_after != "":
                f.write(write_after)

        write_before = con_module.get_before_write_meshdata_constraint()
        write_after = con_module.get_after_write_meshdata_constraint()

        # write sets to file
        write_name = con_module.get_sets_name()
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(write_name.replace("_", " ")))

        if self.split_inpfile is True:
            file_name_split = "{}_{}.inp".format(self.mesh_name, write_name)
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_split))
            inpfile_split = open(join(self.dir_name, file_name_split), "w")
            constraint_sets_loop_writing(inpfile_split, femobjs, write_before, write_after)
            inpfile_split.close()
        else:
            constraint_sets_loop_writing(f, femobjs, write_before, write_after)

    # ********************************************************************************************
    # write constraint data
    def write_constraints_data(
        self,
        f,
        femobjs,
        con_module
    ):

        if not femobjs:
            return

        analysis_types = con_module.get_analysis_types()
        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        write_before = con_module.get_before_write_constraint()
        write_after = con_module.get_after_write_constraint()

        # write constraint to file
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(con_module.get_constraint_title()))
        if write_before != "":
            f.write(write_before)
        for femobj in femobjs:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            the_obj = femobj["Object"]
            f.write("** {}\n".format(the_obj.Label))
            con_module.write_constraint(f, femobj, the_obj, self)
        if write_after != "":
            f.write(write_after)

    # ********************************************************************************************
    # step begin and end
    def write_step_begin(self, f):
        f.write("\n***********************************************************\n")
        f.write("** At least one step is needed to run an CalculiX analysis of FreeCAD\n")
        # STEP line
        step = "*STEP"
        if self.solver_obj.GeometricalNonlinearity == "nonlinear":
            if self.analysis_type == "static" or self.analysis_type == "thermomech":
                # https://www.comsol.com/blogs/what-is-geometric-nonlinearity
                step += ", NLGEOM"
            elif self.analysis_type == "frequency":
                FreeCAD.Console.PrintMessage(
                    "Analysis type frequency and geometrical nonlinear "
                    "analysis are not allowed together, linear is used instead!\n"
                )
        if self.solver_obj.IterationsThermoMechMaximum:
            if self.analysis_type == "thermomech":
                step += ", INC={}".format(self.solver_obj.IterationsThermoMechMaximum)
            elif (
                self.analysis_type == "static"
                or self.analysis_type == "frequency"
                or self.analysis_type == "buckling"
            ):
                # parameter is for thermomechanical analysis only, see ccx manual *STEP
                pass
        # write step line
        f.write(step + "\n")
        # CONTROLS line
        # all analysis types, ... really in frequency too?!?
        if self.solver_obj.IterationsControlParameterTimeUse:
            f.write("*CONTROLS, PARAMETERS=TIME INCREMENTATION\n")
            f.write(self.solver_obj.IterationsControlParameterIter + "\n")
            f.write(self.solver_obj.IterationsControlParameterCutb + "\n")
        # ANALYSIS type line
        # analysis line --> analysis type
        if self.analysis_type == "static":
            analysis_type = "*STATIC"
        elif self.analysis_type == "frequency":
            analysis_type = "*FREQUENCY"
        elif self.analysis_type == "thermomech":
            analysis_type = "*COUPLED TEMPERATURE-DISPLACEMENT"
        elif self.analysis_type == "check":
            analysis_type = "*NO ANALYSIS"
        elif self.analysis_type == "buckling":
            analysis_type = "*BUCKLE"
        # analysis line --> solver type
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=43178
        if self.solver_obj.MatrixSolverType == "default":
            pass
        elif self.solver_obj.MatrixSolverType == "spooles":
            analysis_type += ", SOLVER=SPOOLES"
        elif self.solver_obj.MatrixSolverType == "iterativescaling":
            analysis_type += ", SOLVER=ITERATIVE SCALING"
        elif self.solver_obj.MatrixSolverType == "iterativecholesky":
            analysis_type += ", SOLVER=ITERATIVE CHOLESKY"
        # analysis line --> user defined incrementations --> parameter DIRECT
        # --> completely switch off ccx automatic incrementation
        if self.solver_obj.IterationsUserDefinedIncrementations:
            if self.analysis_type == "static":
                analysis_type += ", DIRECT"
            elif self.analysis_type == "thermomech":
                analysis_type += ", DIRECT"
            elif self.analysis_type == "frequency":
                FreeCAD.Console.PrintMessage(
                    "Analysis type frequency and IterationsUserDefinedIncrementations "
                    "are not allowed together, it is ignored\n"
                )
        # analysis line --> steadystate --> thermomech only
        if self.solver_obj.ThermoMechSteadyState:
            # bernd: I do not know if STEADY STATE is allowed with DIRECT
            # but since time steps are 1.0 it makes no sense IMHO
            if self.analysis_type == "thermomech":
                analysis_type += ", STEADY STATE"
                # Set time to 1 and ignore user inputs for steady state
                self.solver_obj.TimeInitialStep = 1.0
                self.solver_obj.TimeEnd = 1.0
            elif (
                self.analysis_type == "static"
                or self.analysis_type == "frequency"
                or self.analysis_type == "buckling"
            ):
                pass  # not supported for static and frequency!
        # ANALYSIS parameter line
        analysis_parameter = ""
        if self.analysis_type == "static" or self.analysis_type == "check":
            if self.solver_obj.IterationsUserDefinedIncrementations is True \
                    or self.solver_obj.IterationsUserDefinedTimeStepLength is True:
                analysis_parameter = "{},{}".format(
                    self.solver_obj.TimeInitialStep,
                    self.solver_obj.TimeEnd
                )
        elif self.analysis_type == "frequency":
            if self.solver_obj.EigenmodeLowLimit == 0.0 \
                    and self.solver_obj.EigenmodeHighLimit == 0.0:
                analysis_parameter = "{}\n".format(self.solver_obj.EigenmodesCount)
            else:
                analysis_parameter = "{},{},{}\n".format(
                    self.solver_obj.EigenmodesCount,
                    self.solver_obj.EigenmodeLowLimit,
                    self.solver_obj.EigenmodeHighLimit
                )
        elif self.analysis_type == "thermomech":
            # OvG: 1.0 increment, total time 1 for steady state will cut back automatically
            analysis_parameter = "{},{}".format(
                self.solver_obj.TimeInitialStep,
                self.solver_obj.TimeEnd
            )
        elif self.analysis_type == "buckling":
            analysis_parameter = "{}\n".format(self.solver_obj.BucklingFactors)

        # write analysis type line, analysis parameter line
        f.write(analysis_type + "\n")
        f.write(analysis_parameter + "\n")

    def write_step_end(self, f):
        f.write("\n***********************************************************\n")
        f.write("*END STEP \n")

    # ********************************************************************************************
    # output types
    def write_outputs_types(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Outputs --> frd file\n")
        if self.beamsection_objects or self.shellthickness_objects or self.fluidsection_objects:
            if self.solver_obj.BeamShellResultOutput3D is False:
                f.write("*NODE FILE, OUTPUT=2d\n")
            else:
                f.write("*NODE FILE, OUTPUT=3d\n")
        else:
            f.write("*NODE FILE\n")
        # MPH write out nodal temperatures if thermomechanical
        if self.analysis_type == "thermomech":
            if not self.fluidsection_objects:
                f.write("U, NT\n")
            else:
                f.write("MF, PS\n")
        else:
            f.write("U\n")
        if not self.fluidsection_objects:
            f.write("*EL FILE\n")
            if self.solver_obj.MaterialNonlinearity == "nonlinear":
                f.write("S, E, PEEQ\n")
            else:
                f.write("S, E\n")

            # dat file
            # reaction forces: freecadweb.org/tracker/view.php?id=2934
            if self.fixed_objects:
                f.write("** outputs --> dat file\n")
                # reaction forces for all Constraint fixed
                f.write("** reaction forces for Constraint fixed\n")
                for femobj in self.fixed_objects:
                    # femobj --> dict, FreeCAD document object is femobj["Object"]
                    fix_obj_name = femobj["Object"].Name
                    f.write("*NODE PRINT, NSET={}, TOTALS=ONLY\n".format(fix_obj_name))
                    f.write("RF\n")
                # TODO: add Constraint Displacement if nodes are restrained
                f.write("\n")

            # there is no need to write all integration point results
            # as long as there is no reader for them
            # see https://forum.freecadweb.org/viewtopic.php?f=18&t=29060
            # f.write("*NODE PRINT , NSET=" + self.ccx_nall + "\n")
            # f.write("U \n")
            # f.write("*EL PRINT , ELSET=" + self.ccx_eall + "\n")
            # f.write("S \n")

    # ********************************************************************************************
    # footer
    def write_footer(self, f):
        f.write("\n***********************************************************\n")
        f.write("** CalculiX Input file\n")
        f.write("**   written by    --> FreeCAD {}.{}.{}\n".format(
            self.fc_ver[0],
            self.fc_ver[1],
            self.fc_ver[2]
        ))
        f.write("**   written on    --> {}\n".format(
            time.ctime()
        ))
        f.write("**   file name     --> {}\n".format(
            os.path.basename(self.document.FileName)
        ))
        f.write("**   analysis name --> {}\n".format(
            self.analysis.Name
        ))
        f.write("**\n")
        f.write("**\n")
        f.write(units_information)
        f.write("**\n")

    # ********************************************************************************************
    # material and fem element geometry
    # def write_element_sets_material_and_femelement_geometry(self, f):
    def write_element_sets_material_and_femelement_type(self, f):

        # write ccx_elsets to file
        f.write("\n***********************************************************\n")
        f.write("** Element sets for materials and FEM element type (solid, shell, beam, fluid)\n")
        for ccx_elset in self.ccx_elsets:
            f.write("*ELSET,ELSET=" + ccx_elset["ccx_elset_name"] + "\n")
            # use six to be sure to be Python 2.7 and 3.x compatible
            if isinstance(ccx_elset["ccx_elset"], six.string_types):
                f.write(ccx_elset["ccx_elset"] + "\n")
            else:
                for elid in ccx_elset["ccx_elset"]:
                    f.write(str(elid) + ",\n")

    def is_density_needed(self):
        if self.analysis_type == "frequency":
            return True
        if self.analysis_type == "thermomech" and not self.solver_obj.ThermoMechSteadyState:
            return True
        if self.centrif_objects:
            return True
        if self.selfweight_objects:
            return True
        return False

    def write_materials(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Materials\n")
        f.write("** Young\'s modulus unit is MPa = N/mm2\n")
        if self.is_density_needed() is True:
            f.write("** Density\'s unit is t/mm^3\n")
        if self.analysis_type == "thermomech":
            f.write("** Thermal conductivity unit is kW/mm/K = t*mm/K*s^3\n")
            f.write("** Specific Heat unit is kJ/t/K = mm^2/s^2/K\n")
        for femobj in self.material_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            mat_obj = femobj["Object"]
            mat_info_name = mat_obj.Material["Name"]
            mat_name = mat_obj.Name
            mat_label = mat_obj.Label
            # get material properties of solid material, Currently in SI units: M/kg/s/Kelvin
            if mat_obj.Category == "Solid":
                YM = FreeCAD.Units.Quantity(mat_obj.Material["YoungsModulus"])
                YM_in_MPa = float(YM.getValueAs("MPa"))
                PR = float(mat_obj.Material["PoissonRatio"])
            if self.is_density_needed() is True:
                density = FreeCAD.Units.Quantity(mat_obj.Material["Density"])
                density_in_tonne_per_mm3 = float(density.getValueAs("t/mm^3"))
            if self.analysis_type == "thermomech":
                TC = FreeCAD.Units.Quantity(mat_obj.Material["ThermalConductivity"])
                # SvdW: Add factor to force units to results base units
                # of t/mm/s/K - W/m/K results in no factor needed
                TC_in_WmK = float(TC.getValueAs("W/m/K"))
                SH = FreeCAD.Units.Quantity(mat_obj.Material["SpecificHeat"])
                # SvdW: Add factor to force units to results base units of t/mm/s/K
                SH_in_JkgK = float(SH.getValueAs("J/kg/K")) * 1e+06
                if mat_obj.Category == "Solid":
                    TEC = FreeCAD.Units.Quantity(mat_obj.Material["ThermalExpansionCoefficient"])
                    TEC_in_mmK = float(TEC.getValueAs("mm/mm/K"))
                elif mat_obj.Category == "Fluid":
                    DV = FreeCAD.Units.Quantity(mat_obj.Material["DynamicViscosity"])
                    DV_in_tmms = float(DV.getValueAs("t/mm/s"))
            # write material properties
            f.write("** FreeCAD material name: " + mat_info_name + "\n")
            f.write("** " + mat_label + "\n")
            f.write("*MATERIAL, NAME=" + mat_name + "\n")
            if mat_obj.Category == "Solid":
                f.write("*ELASTIC\n")
                f.write("{0:.0f}, {1:.3f}\n".format(YM_in_MPa, PR))

            if self.is_density_needed() is True:
                f.write("*DENSITY\n")
                f.write("{0:.3e}\n".format(density_in_tonne_per_mm3))
            if self.analysis_type == "thermomech":
                if mat_obj.Category == "Solid":
                    f.write("*CONDUCTIVITY\n")
                    f.write("{0:.3f}\n".format(TC_in_WmK))
                    f.write("*EXPANSION\n")
                    f.write("{0:.3e}\n".format(TEC_in_mmK))
                    f.write("*SPECIFIC HEAT\n")
                    f.write("{0:.3e}\n".format(SH_in_JkgK))
                elif mat_obj.Category == "Fluid":
                    f.write("*FLUID CONSTANTS\n")
                    f.write("{0:.3e}, {1:.3e}\n".format(SH_in_JkgK, DV_in_tmms))

            # nonlinear material properties
            if self.solver_obj.MaterialNonlinearity == "nonlinear":

                for nlfemobj in self.material_nonlinear_objects:
                    # femobj --> dict, FreeCAD document object is nlfemobj["Object"]
                    nl_mat_obj = nlfemobj["Object"]
                    if nl_mat_obj.LinearBaseMaterial == mat_obj:
                        if nl_mat_obj.MaterialModelNonlinearity == "simple hardening":
                            f.write("*PLASTIC\n")
                            if nl_mat_obj.YieldPoint1:
                                f.write(nl_mat_obj.YieldPoint1 + "\n")
                            if nl_mat_obj.YieldPoint2:
                                f.write(nl_mat_obj.YieldPoint2 + "\n")
                            if nl_mat_obj.YieldPoint3:
                                f.write(nl_mat_obj.YieldPoint3 + "\n")
                    f.write("\n")

    # def write_femelement_geometry(self, f):
    def write_femelementsets(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Sections\n")
        for ccx_elset in self.ccx_elsets:
            if ccx_elset["ccx_elset"]:
                if "beamsection_obj"in ccx_elset:  # beam mesh
                    beamsec_obj = ccx_elset["beamsection_obj"]
                    elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                    material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                    normal = ccx_elset["beam_normal"]
                    if beamsec_obj.SectionType == "Rectangular":
                        height = beamsec_obj.RectHeight.getValueAs("mm")
                        width = beamsec_obj.RectWidth.getValueAs("mm")
                        section_type = ", SECTION=RECT"
                        section_geo = str(height) + ", " + str(width) + "\n"
                        section_def = "*BEAM SECTION, {}{}{}\n".format(
                            elsetdef,
                            material,
                            section_type
                        )
                    elif beamsec_obj.SectionType == "Circular":
                        radius = 0.5 * beamsec_obj.CircDiameter.getValueAs("mm")
                        section_type = ", SECTION=CIRC"
                        section_geo = str(radius) + "\n"
                        section_def = "*BEAM SECTION, {}{}{}\n".format(
                            elsetdef,
                            material,
                            section_type
                        )
                    elif beamsec_obj.SectionType == "Pipe":
                        radius = 0.5 * beamsec_obj.PipeDiameter.getValueAs("mm")
                        thickness = beamsec_obj.PipeThickness.getValueAs("mm")
                        section_type = ", SECTION=PIPE"
                        section_geo = str(radius) + ", " + str(thickness) + "\n"
                        section_def = "*BEAM GENERAL SECTION, {}{}{}\n".format(
                            elsetdef,
                            material,
                            section_type
                        )
                    # see forum topic for output formatting of rotation
                    # https://forum.freecadweb.org/viewtopic.php?f=18&t=46133&p=405142#p405142
                    section_nor = "{:f}, {:f}, {:f}\n".format(
                        normal[0],
                        normal[1],
                        normal[2]
                    )
                    f.write(section_def)
                    f.write(section_geo)
                    f.write(section_nor)
                elif "fluidsection_obj"in ccx_elset:  # fluid mesh
                    fluidsec_obj = ccx_elset["fluidsection_obj"]
                    elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                    material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                    if fluidsec_obj.SectionType == "Liquid":
                        section_type = fluidsec_obj.LiquidSectionType
                        if (section_type == "PIPE INLET") or (section_type == "PIPE OUTLET"):
                            section_type = "PIPE INOUT"
                        section_def = "*FLUID SECTION, {}TYPE={}, {}\n".format(
                            elsetdef,
                            section_type,
                            material
                        )
                        section_geo = liquid_section_def(fluidsec_obj, section_type)
                    """
                    # deactivate as it would result in section_def and section_geo not defined
                    # deactivated in the App and Gui object and thus in the task panel as well
                    elif fluidsec_obj.SectionType == "Gas":
                        section_type = fluidsec_obj.GasSectionType
                    elif fluidsec_obj.SectionType == "Open Channel":
                        section_type = fluidsec_obj.ChannelSectionType
                    """
                    f.write(section_def)
                    f.write(section_geo)
                elif "shellthickness_obj"in ccx_elset:  # shell mesh
                    shellth_obj = ccx_elset["shellthickness_obj"]
                    elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                    material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                    section_def = "*SHELL SECTION, " + elsetdef + material + "\n"
                    section_geo = str(shellth_obj.Thickness.getValueAs("mm")) + "\n"
                    f.write(section_def)
                    f.write(section_geo)
                else:  # solid mesh
                    elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                    material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                    section_def = "*SOLID SECTION, " + elsetdef + material + "\n"
                    f.write(section_def)


# ************************************************************************************************
# Helpers
def liquid_section_def(obj, section_type):
    if section_type == "PIPE MANNING":
        manning_area = str(obj.ManningArea.getValueAs("mm^2").Value)
        manning_radius = str(obj.ManningRadius.getValueAs("mm"))
        manning_coefficient = str(obj.ManningCoefficient)
        section_geo = manning_area + "," + manning_radius + "," + manning_coefficient + "\n"
        return section_geo
    elif section_type == "PIPE ENLARGEMENT":
        enlarge_area1 = str(obj.EnlargeArea1.getValueAs("mm^2").Value)
        enlarge_area2 = str(obj.EnlargeArea2.getValueAs("mm^2").Value)
        section_geo = enlarge_area1 + "," + enlarge_area2 + "\n"
        return section_geo
    elif section_type == "PIPE CONTRACTION":
        contract_area1 = str(obj.ContractArea1.getValueAs("mm^2").Value)
        contract_area2 = str(obj.ContractArea2.getValueAs("mm^2").Value)
        section_geo = contract_area1 + "," + contract_area2 + "\n"
        return section_geo
    elif section_type == "PIPE ENTRANCE":
        entrance_pipe_area = str(obj.EntrancePipeArea.getValueAs("mm^2").Value)
        entrance_area = str(obj.EntranceArea.getValueAs("mm^2").Value)
        section_geo = entrance_pipe_area + "," + entrance_area + "\n"
        return section_geo
    elif section_type == "PIPE DIAPHRAGM":
        diaphragm_pipe_area = str(obj.DiaphragmPipeArea.getValueAs("mm^2").Value)
        diaphragm_area = str(obj.DiaphragmArea.getValueAs("mm^2").Value)
        section_geo = diaphragm_pipe_area + "," + diaphragm_area + "\n"
        return section_geo
    elif section_type == "PIPE BEND":
        bend_pipe_area = str(obj.BendPipeArea.getValueAs("mm^2").Value)
        bend_radius_diameter = str(obj.BendRadiusDiameter)
        bend_angle = str(obj.BendAngle)
        bend_loss_coefficient = str(obj.BendLossCoefficient)
        section_geo = ("{},{},{},{}\n".format(
            bend_pipe_area,
            bend_radius_diameter,
            bend_angle,
            bend_loss_coefficient
        ))
        return section_geo
    elif section_type == "PIPE GATE VALVE":
        gatevalve_pipe_area = str(obj.GateValvePipeArea.getValueAs("mm^2").Value)
        gatevalve_closing_coeff = str(obj.GateValveClosingCoeff)
        section_geo = gatevalve_pipe_area + "," + gatevalve_closing_coeff + "\n"
        return section_geo
    elif section_type == "PIPE WHITE-COLEBROOK":
        colebrooke_area = str(obj.ColebrookeArea.getValueAs("mm^2").Value)
        colebrooke_diameter = str(2 * obj.ColebrookeRadius.getValueAs("mm"))
        colebrooke_grain_diameter = str(obj.ColebrookeGrainDiameter.getValueAs("mm"))
        colebrooke_form_factor = str(obj.ColebrookeFormFactor)
        section_geo = ("{},{},{},{},{}\n".format(
            colebrooke_area,
            colebrooke_diameter,
            "-1",
            colebrooke_grain_diameter,
            colebrooke_form_factor
        ))
        return section_geo
    elif section_type == "LIQUID PUMP":
        section_geo = ""
        for i in range(len(obj.PumpFlowRate)):
            flow_rate = str(obj.PumpFlowRate[i])
            top = str(obj.PumpHeadLoss[i])
            section_geo = section_geo + flow_rate + "," + top + ","
        section_geo = section_geo + "\n"
        return section_geo
    else:
        return ""
##  @}
