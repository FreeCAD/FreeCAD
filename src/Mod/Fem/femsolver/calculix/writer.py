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

__title__  = "FreeCAD FEM solver CalculiX writer"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__    = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

# import io
import codecs
import os
import six
import sys
import time
from os.path import join

import FreeCAD

from .. import writerbase
from femmesh import meshtools
from femtools import geomtools


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
**  The unit system which is used at Guido Dhodts company: mm, N, s, K
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
**  Density: t/mm^2
**  Gravity: mm/s^2
**  Thermal conductivity: t*mm/K*s^3
**  Specific Heat: kJ/t/K = mm^2/s^2/K
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
        self.FluidInletoutlet_ele = []
        self.fluid_inout_nodes_file = join(
            self.dir_name,
            "{}_inout_nodes.txt".format(self.mesh_name)
        )
        from femtools import constants
        from FreeCAD import Units
        self.gravity = int(Units.Quantity(constants.gravity()).getValueAs("mm/s^2"))  # 9820 mm/s2

    # ********************************************************************************************
    # write calculix input
    def write_calculix_input_file(self):
        timestart = time.process_time()
        FreeCAD.Console.PrintMessage("Start writing CalculiX input file\n")
        FreeCAD.Console.PrintMessage("Write ccx input file to: {}\n".format(self.file_name))
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.mesh_name  -->  " + self.mesh_name + "\n"
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.dir_name  -->  " + self.dir_name + "\n"
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.include  -->  " + self.mesh_name + "\n"
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.file_name  -->  " + self.file_name + "\n"
        )

        self.write_calculix_input()

        writing_time_string = (
            "Writing time CalculiX input file: {} seconds"
            .format(round((time.process_time() - timestart), 2))
        )
        if self.femelement_count_test is True:
            FreeCAD.Console.PrintMessage(writing_time_string + " \n\n")
            return self.file_name
        else:
            FreeCAD.Console.PrintMessage(writing_time_string + " \n")
            FreeCAD.Console.PrintError(
                "Problems on writing input file, check report prints.\n\n"
            )
            return ""

    def write_calculix_input(self):
        if self.solver_obj.SplitInputWriter is True:
            self.split_inpfile = True
        else:
            self.split_inpfile = False

        # mesh
        inpfileMain = self.write_mesh(self.split_inpfile)

        # element and material sets
        self.write_element_sets_material_and_femelement_type(inpfileMain)

        # node sets and surface sets
        self.write_node_sets_constraints_fixed(inpfileMain, self.split_inpfile)
        self.write_node_sets_constraints_displacement(inpfileMain, self.split_inpfile)
        self.write_node_sets_constraints_planerotation(inpfileMain, self.split_inpfile)
        self.write_surfaces_constraints_contact(inpfileMain, self.split_inpfile)
        self.write_surfaces_constraints_tie(inpfileMain, self.split_inpfile)
        self.write_surfaces_constraints_sectionprint(inpfileMain, self.split_inpfile)
        self.write_node_sets_constraints_transform(inpfileMain, self.split_inpfile)
        self.write_node_sets_constraints_temperature(inpfileMain, self.split_inpfile)

        # materials and fem element types
        self.write_materials(inpfileMain)
        self.write_constraints_initialtemperature(inpfileMain)
        self.write_femelementsets(inpfileMain)

        # Fluid sections:
        # Inlet and Outlet requires special element definition
        # some data from the elsets are needed thus this can not be moved
        # to mesh writing TODO it would be much better if this would be
        # at mesh writing as the mesh will be changed
        if self.fluidsection_objects:
            if is_fluid_section_inlet_outlet(self.ccx_elsets) is True:
                if self.split_inpfile is True:
                    meshtools.use_correct_fluidinout_ele_def(
                        self.FluidInletoutlet_ele,
                        # use mesh file split, see write_mesh method split_mesh_file_path
                        join(self.dir_name, self.mesh_name + "_femesh.inp"),
                        self.fluid_inout_nodes_file
                    )
                else:
                    inpfileMain.close()
                    meshtools.use_correct_fluidinout_ele_def(
                        self.FluidInletoutlet_ele,
                        self.file_name,
                        self.fluid_inout_nodes_file
                    )
                    # inpfileMain = io.open(self.file_name, "a", encoding="utf-8")
                    inpfileMain = codecs.open(self.file_name, "a", encoding="utf-8")

        # constraints independent from steps
        self.write_constraints_planerotation(inpfileMain)
        self.write_constraints_contact(inpfileMain)
        self.write_constraints_tie(inpfileMain)
        self.write_constraints_transform(inpfileMain)

        # step begin
        self.write_step_begin(inpfileMain)

        # constraints dependent from steps
        self.write_constraints_fixed(inpfileMain)
        self.write_constraints_displacement(inpfileMain)
        self.write_constraints_sectionprint(inpfileMain)
        self.write_constraints_selfweight(inpfileMain)
        self.write_constraints_force(inpfileMain, self.split_inpfile)
        self.write_constraints_pressure(inpfileMain, self.split_inpfile)
        self.write_constraints_temperature(inpfileMain)
        self.write_constraints_heatflux(inpfileMain, self.split_inpfile)
        self.write_constraints_fluidsection(inpfileMain)

        # output and step end
        self.write_outputs_types(inpfileMain)
        self.write_step_end(inpfileMain)

        # footer
        self.write_footer(inpfileMain)
        inpfileMain.close()

    # ********************************************************************************************
    # mesh
    def write_mesh(self, inpfile_split=None):
        # write mesh to file
        element_param = 1  # highest element order only
        group_param = False  # do not write mesh group data
        if inpfile_split is True:
            write_name = "femesh"
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            split_mesh_file_path = join(self.dir_name, file_name_splitt)

            self.femmesh.writeABAQUS(
                split_mesh_file_path,
                element_param,
                group_param
            )

            # Check to see if fluid sections are in analysis and use D network element type
            if self.fluidsection_objects:
                meshtools.write_D_network_element_to_inputfile(split_mesh_file_path)

            # inpfile = io.open(self.file_name, "w", encoding="utf-8")
            inpfile = codecs.open(self.file_name, "w", encoding="utf-8")
            inpfile.write("***********************************************************\n")
            inpfile.write("** {}\n".format(write_name))
            inpfile.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))

        else:
            self.femmesh.writeABAQUS(
                self.file_name,
                element_param,
                group_param
            )

            # Check to see if fluid sections are in analysis and use D network element type
            if self.fluidsection_objects:
                # inpfile is closed
                meshtools.write_D_network_element_to_inputfile(self.file_name)

            # reopen file with "append" to add all the rest
            # inpfile = io.open(self.file_name, "a", encoding="utf-8")
            inpfile = codecs.open(self.file_name, "a", encoding="utf-8")
            inpfile.write("\n\n")

        return inpfile

    # ********************************************************************************************
    # constraints fixed
    def write_node_sets_constraints_fixed(self, f, inpfile_split=None):
        if not self.fixed_objects:
            return
        # write for all analysis types

        # get nodes
        self.get_constraints_fixed_nodes()

        write_name = "constraints_fixed_node_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_node_sets_nodes_constraints_fixed(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_node_sets_nodes_constraints_fixed(f)

    def write_node_sets_nodes_constraints_fixed(self, f):
        # write nodes to file
        for femobj in self.fixed_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            fix_obj = femobj["Object"]
            f.write("** " + fix_obj.Label + "\n")
            if self.femmesh.Volumes \
                    and (len(self.shellthickness_objects) > 0 or len(self.beamsection_objects) > 0):
                if len(femobj["NodesSolid"]) > 0:
                    f.write("*NSET,NSET=" + fix_obj.Name + "Solid\n")
                    for n in femobj["NodesSolid"]:
                        f.write(str(n) + ",\n")
                if len(femobj["NodesFaceEdge"]) > 0:
                    f.write("*NSET,NSET=" + fix_obj.Name + "FaceEdge\n")
                    for n in femobj["NodesFaceEdge"]:
                        f.write(str(n) + ",\n")
            else:
                f.write("*NSET,NSET=" + fix_obj.Name + "\n")
                for n in femobj["Nodes"]:
                    f.write(str(n) + ",\n")

    def write_constraints_fixed(self, f):
        if not self.fixed_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Fixed Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.fixed_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            f.write("** " + femobj["Object"].Label + "\n")
            fix_obj_name = femobj["Object"].Name
            if self.femmesh.Volumes \
                    and (len(self.shellthickness_objects) > 0 or len(self.beamsection_objects) > 0):
                if len(femobj["NodesSolid"]) > 0:
                    f.write("*BOUNDARY\n")
                    f.write(fix_obj_name + "Solid" + ",1\n")
                    f.write(fix_obj_name + "Solid" + ",2\n")
                    f.write(fix_obj_name + "Solid" + ",3\n")
                    f.write("\n")
                if len(femobj["NodesFaceEdge"]) > 0:
                    f.write("*BOUNDARY\n")
                    f.write(fix_obj_name + "FaceEdge" + ",1\n")
                    f.write(fix_obj_name + "FaceEdge" + ",2\n")
                    f.write(fix_obj_name + "FaceEdge" + ",3\n")
                    f.write(fix_obj_name + "FaceEdge" + ",4\n")
                    f.write(fix_obj_name + "FaceEdge" + ",5\n")
                    f.write(fix_obj_name + "FaceEdge" + ",6\n")
                    f.write("\n")
            else:
                f.write("*BOUNDARY\n")
                f.write(fix_obj_name + ",1\n")
                f.write(fix_obj_name + ",2\n")
                f.write(fix_obj_name + ",3\n")
                if self.beamsection_objects or self.shellthickness_objects:
                    f.write(fix_obj_name + ",4\n")
                    f.write(fix_obj_name + ",5\n")
                    f.write(fix_obj_name + ",6\n")
                f.write("\n")

    # ********************************************************************************************
    # constraints displacement
    def write_node_sets_constraints_displacement(self, f, inpfile_split=None):
        if not self.displacement_objects:
            return
        # write for all analysis types

        # get nodes
        self.get_constraints_displacement_nodes()

        write_name = "constraints_displacement_node_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_node_sets_nodes_constraints_displacement(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_node_sets_nodes_constraints_displacement(f)

    def write_node_sets_nodes_constraints_displacement(self, f):
        # write nodes to file
        for femobj in self.displacement_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            disp_obj = femobj["Object"]
            f.write("** " + disp_obj.Label + "\n")
            f.write("*NSET,NSET=" + disp_obj.Name + "\n")
            for n in femobj["Nodes"]:
                f.write(str(n) + ",\n")

    def write_constraints_displacement(self, f):
        if not self.displacement_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Displacement constraint applied\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.displacement_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            f.write("** " + femobj["Object"].Label + "\n")
            disp_obj = femobj["Object"]
            disp_obj_name = disp_obj.Name
            f.write("*BOUNDARY\n")
            if disp_obj.xFix:
                f.write(disp_obj_name + ",1\n")
            elif not disp_obj.xFree:
                f.write(disp_obj_name + ",1,1," + str(disp_obj.xDisplacement) + "\n")
            if disp_obj.yFix:
                f.write(disp_obj_name + ",2\n")
            elif not disp_obj.yFree:
                f.write(disp_obj_name + ",2,2," + str(disp_obj.yDisplacement) + "\n")
            if disp_obj.zFix:
                f.write(disp_obj_name + ",3\n")
            elif not disp_obj.zFree:
                f.write(disp_obj_name + ",3,3," + str(disp_obj.zDisplacement) + "\n")

            if self.beamsection_objects or self.shellthickness_objects:
                if disp_obj.rotxFix:
                    f.write(disp_obj_name + ",4\n")
                elif not disp_obj.rotxFree:
                    f.write(disp_obj_name + ",4,4," + str(disp_obj.xRotation) + "\n")
                if disp_obj.rotyFix:
                    f.write(disp_obj_name + ",5\n")
                elif not disp_obj.rotyFree:
                    f.write(disp_obj_name + ",5,5," + str(disp_obj.yRotation) + "\n")
                if disp_obj.rotzFix:
                    f.write(disp_obj_name + ",6\n")
                elif not disp_obj.rotzFree:
                    f.write(disp_obj_name + ",6,6," + str(disp_obj.zRotation) + "\n")
        f.write("\n")

    # ********************************************************************************************
    # constraints planerotation
    def write_node_sets_constraints_planerotation(self, f, inpfile_split=None):
        if not self.planerotation_objects:
            return
        # write for all analysis types

        # get nodes
        self.get_constraints_planerotation_nodes()

        write_name = "constraints_planerotation_node_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_node_sets_nodes_constraints_planerotation(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_node_sets_nodes_constraints_planerotation(f)

    def write_node_sets_nodes_constraints_planerotation(self, f):
        # write nodes to file
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        # info about self.constraint_conflict_nodes:
        # is used to check if MPC and constraint fixed and
        # constraint displacement share same nodes
        # because MPC"s and constraints fixed and
        # constraints displacement can't share same nodes.
        # Thus call write_node_sets_constraints_planerotation has to be
        # after constraint fixed and constraint displacement
        for femobj in self.planerotation_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            l_nodes = femobj["Nodes"]
            fric_obj = femobj["Object"]
            f.write("** " + fric_obj.Label + "\n")
            f.write("*NSET,NSET=" + fric_obj.Name + "\n")
            # Code to extract nodes and coordinates on the PlaneRotation support face
            nodes_coords = []
            for node in l_nodes:
                nodes_coords.append((
                    node,
                    self.femnodes_mesh[node].x,
                    self.femnodes_mesh[node].y,
                    self.femnodes_mesh[node].z
                ))
            node_planerotation = meshtools.get_three_non_colinear_nodes(nodes_coords)
            for i in range(len(l_nodes)):
                if l_nodes[i] not in node_planerotation:
                    node_planerotation.append(l_nodes[i])
            MPC_nodes = []
            for i in range(len(node_planerotation)):
                cnt = 0
                for j in range(len(self.constraint_conflict_nodes)):
                    if node_planerotation[i] == self.constraint_conflict_nodes[j]:
                        cnt = cnt + 1
                if cnt == 0:
                    MPC = node_planerotation[i]
                    MPC_nodes.append(MPC)
            for i in range(len(MPC_nodes)):
                f.write(str(MPC_nodes[i]) + ",\n")

    def write_constraints_planerotation(self, f):
        if not self.planerotation_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** PlaneRotation Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.planerotation_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            f.write("** " + femobj["Object"].Label + "\n")
            fric_obj_name = femobj["Object"].Name
            f.write("*MPC\n")
            f.write("PLANE," + fric_obj_name + "\n")

    # ********************************************************************************************
    # constraints contact
    def write_surfaces_constraints_contact(self, f, inpfile_split=None):
        if not self.contact_objects:
            return
        # write for all analysis types

        # get faces
        self.get_constraints_contact_faces()

        write_name = "constraints_contact_surface_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_surfacefaces_constraints_contact(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_surfacefaces_constraints_contact(f)

    def write_surfacefaces_constraints_contact(self, f):
        # write faces to file
        for femobj in self.contact_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            contact_obj = femobj["Object"]
            f.write("** " + contact_obj.Label + "\n")
            # slave DEP
            f.write("*SURFACE, NAME=DEP{}\n".format(contact_obj.Name))
            for i in femobj["ContactSlaveFaces"]:
                f.write("{},S{}\n".format(i[0], i[1]))
            # master IND
            f.write("*SURFACE, NAME=IND{}\n".format(contact_obj.Name))
            for i in femobj["ContactMasterFaces"]:
                f.write("{},S{}\n".format(i[0], i[1]))

    def write_constraints_contact(self, f):
        if not self.contact_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Contact Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.contact_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            contact_obj = femobj["Object"]
            f.write("** " + contact_obj.Label + "\n")
            f.write(
                "*CONTACT PAIR, INTERACTION=INT{},TYPE=SURFACE TO SURFACE\n"
                .format(contact_obj.Name)
            )
            ind_surf = "IND" + contact_obj.Name
            dep_surf = "DEP" + contact_obj.Name
            f.write(dep_surf + "," + ind_surf + "\n")
            f.write("*SURFACE INTERACTION, NAME=INT{}\n".format(contact_obj.Name))
            f.write("*SURFACE BEHAVIOR,PRESSURE-OVERCLOSURE=LINEAR\n")
            slope = contact_obj.Slope
            f.write(str(slope) + " \n")
            friction = contact_obj.Friction
            if friction > 0:
                f.write("*FRICTION \n")
                stick = (slope / 10.0)
                f.write(str(friction) + ", " + str(stick) + " \n")

    # ********************************************************************************************
    # constraints tie
    def write_surfaces_constraints_tie(self, f, inpfile_split=None):
        if not self.tie_objects:
            return
        # write for all analysis types

        # get faces
        self.get_constraints_tie_faces()

        write_name = "constraints_tie_surface_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_surfacefaces_constraints_tie(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_surfacefaces_constraints_tie(f)

    def write_surfacefaces_constraints_tie(self, f):
        # write faces to file
        for femobj in self.tie_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            tie_obj = femobj["Object"]
            f.write("** " + tie_obj.Label + "\n")
            # slave DEP
            f.write("*SURFACE, NAME=TIE_DEP{}\n".format(tie_obj.Name))
            for i in femobj["TieSlaveFaces"]:
                f.write("{},S{}\n".format(i[0], i[1]))
            # master IND
            f.write("*SURFACE, NAME=TIE_IND{}\n".format(tie_obj.Name))
            for i in femobj["TieMasterFaces"]:
                f.write("{},S{}\n".format(i[0], i[1]))

    def write_constraints_tie(self, f):
        if not self.tie_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Tie Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.tie_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            tie_obj = femobj["Object"]
            f.write("** {}\n".format(tie_obj.Label))
            tolerance = str(tie_obj.Tolerance.getValueAs("mm")).rstrip()
            f.write(
                "*TIE, POSITION TOLERANCE={}, ADJUST=NO, NAME=TIE{}\n"
                .format(tolerance, tie_obj.Name)
            )
            ind_surf = "TIE_IND" + tie_obj.Name
            dep_surf = "TIE_DEP" + tie_obj.Name
            f.write("{},{}\n".format(dep_surf, ind_surf))

    # ********************************************************************************************
    # constraints sectionprint
    def write_surfaces_constraints_sectionprint(self, f, inpfile_split=None):
        if not self.sectionprint_objects:
            return
        # write for all analysis types

        write_name = "constraints_sectionprint_surface_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_surfacefaces_constraints_sectionprint(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_surfacefaces_constraints_sectionprint(f)

    # TODO move code parts from this method to base writer module
    def write_surfacefaces_constraints_sectionprint(self, f):
        # get surface nodes and write them to file
        obj = 0
        for femobj in self.sectionprint_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            sectionprint_obj = femobj["Object"]
            f.write("** " + sectionprint_obj.Label + "\n")
            obj = obj + 1
            for o, elem_tup in sectionprint_obj.References:
                for elem in elem_tup:
                    ref_shape = o.Shape.getElement(elem)
                    if ref_shape.ShapeType == "Face":
                        name = "SECTIONFACE" + str(obj)
                        f.write("*SURFACE, NAME=" + name + "\n")

                        v = self.mesh_object.FemMesh.getccxVolumesByFace(ref_shape)
                        if len(v) > 0:
                            # volume elements found
                            FreeCAD.Console.PrintLog(
                                "{}, surface {}, {} touching volume elements found\n"
                                .format(sectionprint_obj.Label, name, len(v))
                            )
                            for i in v:
                                f.write("{},S{}\n".format(i[0], i[1]))
                        else:
                            # no volume elements found, shell elements not allowed
                            FreeCAD.Console.PrintError(
                                "{}, surface {}, Error: "
                                "No volume elements found!\n"
                                .format(sectionprint_obj.Label, name)
                            )
                            f.write("** Error: empty list\n")

    def write_constraints_sectionprint(self, f):
        if not self.sectionprint_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** SectionPrint Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        obj = 0
        for femobj in self.sectionprint_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            obj = obj + 1
            sectionprint_obj = femobj["Object"]
            f.write("** {}\n".format(sectionprint_obj.Label))
            f.write(
                "*SECTION PRINT, SURFACE=SECTIONFACE{}, NAME=SECTIONPRINT{}\n"
                .format(obj, obj)
            )
            f.write("SOF, SOM, SOAREA\n")

    # ********************************************************************************************
    # constraints transform
    def write_node_sets_constraints_transform(self, f, inpfile_split=None):
        if not self.transform_objects:
            return
        # write for all analysis types

        # get nodes
        self.get_constraints_transform_nodes()

        write_name = "constraints_transform_node_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_node_sets_nodes_constraints_transform(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_node_sets_nodes_constraints_transform(f)

    def write_node_sets_nodes_constraints_transform(self, f):
        # write nodes to file
        for femobj in self.transform_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            trans_obj = femobj["Object"]
            f.write("** " + trans_obj.Label + "\n")
            if trans_obj.TransformType == "Rectangular":
                f.write("*NSET,NSET=Rect" + trans_obj.Name + "\n")
            elif trans_obj.TransformType == "Cylindrical":
                f.write("*NSET,NSET=Cylin" + trans_obj.Name + "\n")
            for n in femobj["Nodes"]:
                f.write(str(n) + ",\n")

    def write_constraints_transform(self, f):
        if not self.transform_objects:
            return
        # write for all analysis types

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Transform Constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for trans_object in self.transform_objects:
            trans_obj = trans_object["Object"]
            trans_name = ""
            trans_type = ""
            if trans_obj.TransformType == "Rectangular":
                trans_name = "Rect"
                trans_type = "R"
                coords = geomtools.get_rectangular_coords(trans_obj)
            elif trans_obj.TransformType == "Cylindrical":
                trans_name = "Cylin"
                trans_type = "C"
                coords = geomtools.get_cylindrical_coords(trans_obj)
            f.write("** {}\n".format(trans_obj.Label))
            f.write("*TRANSFORM, NSET={}{}, TYPE={}\n".format(
                trans_name,
                trans_obj.Name,
                trans_type,
            ))
            f.write("{:f},{:f},{:f},{:f},{:f},{:f}\n".format(
                coords[0],
                coords[1],
                coords[2],
                coords[3],
                coords[4],
                coords[5],
            ))

    # ********************************************************************************************
    # constraints temperature
    def write_node_sets_constraints_temperature(self, f, inpfile_split=None):
        if not self.temperature_objects:
            return
        if not self.analysis_type == "thermomech":
            return

        # get nodes
        self.get_constraints_temperature_nodes()

        write_name = "constraints_temperature_node_sets"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_node_sets_nodes_constraints_temperature(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_node_sets_nodes_constraints_temperature(f)

    def write_node_sets_nodes_constraints_temperature(self, f):
        # write nodes to file
        for femobj in self.temperature_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            temp_obj = femobj["Object"]
            f.write("** " + temp_obj.Label + "\n")
            f.write("*NSET,NSET=" + temp_obj.Name + "\n")
            for n in femobj["Nodes"]:
                f.write(str(n) + ",\n")

    def write_constraints_temperature(self, f):
        if not self.temperature_objects:
            return
        if not self.analysis_type == "thermomech":
            return

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Fixed temperature constraint applied\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for ftobj in self.temperature_objects:
            fixedtemp_obj = ftobj["Object"]
            f.write("** " + fixedtemp_obj.Label + "\n")
            NumberOfNodes = len(ftobj["Nodes"])
            if fixedtemp_obj.ConstraintType == "Temperature":
                f.write("*BOUNDARY\n")
                f.write("{},11,11,{}\n".format(fixedtemp_obj.Name, fixedtemp_obj.Temperature))
                f.write("\n")
            elif fixedtemp_obj.ConstraintType == "CFlux":
                f.write("*CFLUX\n")
                f.write("{},11,{}\n".format(
                    fixedtemp_obj.Name,
                    fixedtemp_obj.CFlux * 0.001 / NumberOfNodes
                ))
                f.write("\n")

    # ********************************************************************************************
    # constraints initialtemperature
    def write_constraints_initialtemperature(self, f):
        if not self.initialtemperature_objects:
            return
        if not self.analysis_type == "thermomech":
            return

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Initial temperature constraint\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        f.write("*INITIAL CONDITIONS,TYPE=TEMPERATURE\n")
        for itobj in self.initialtemperature_objects:  # Should only be one
            inittemp_obj = itobj["Object"]
            # OvG: Initial temperature
            f.write("{0},{1}\n".format(self.ccx_nall, inittemp_obj.initialTemperature))

    # ********************************************************************************************
    # constraints selfweight
    def write_constraints_selfweight(self, f):
        if not self.selfweight_objects:
            return
        if not (self.analysis_type == "static" or self.analysis_type == "thermomech"):
            return

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** Self weight Constraint\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        for femobj in self.selfweight_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            selwei_obj = femobj["Object"]
            f.write("** " + selwei_obj.Label + "\n")
            f.write("*DLOAD\n")
            f.write(
                # elset, GRAV, magnitude, direction x, dir y ,dir z
                "{},GRAV,{},{},{},{}\n"
                .format(
                    self.ccx_eall,
                    self.gravity,  # actual magnitude of gravity vector
                    selwei_obj.Gravity_x,  # coordinate x of normalized gravity vector
                    selwei_obj.Gravity_y,  # y
                    selwei_obj.Gravity_z  # z
                )
            )
            f.write("\n")
        # grav (erdbeschleunigung) is equal for all elements
        # should be only one constraint
        # different element sets for different density
        # are written in the material element sets already

    # ********************************************************************************************
    # constraints force
    def write_constraints_force(self, f, inpfile_split=None):
        if not self.force_objects:
            return
        if not (self.analysis_type == "static" or self.analysis_type == "thermomech"):
            return

        # check shape type of reference shape and get node loads
        self.get_constraints_force_nodeloads()

        write_name = "constraints_force_node_loads"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_nodeloads_constraints_force(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_nodeloads_constraints_force(f)

    def write_nodeloads_constraints_force(self, f):
        # write node loads to file
        f.write("*CLOAD\n")
        for femobj in self.force_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            f.write("** " + femobj["Object"].Label + "\n")
            direction_vec = femobj["Object"].DirectionVector
            for ref_shape in femobj["NodeLoadTable"]:
                f.write("** " + ref_shape[0] + "\n")
                for n in sorted(ref_shape[1]):
                    node_load = ref_shape[1][n]
                    if (direction_vec.x != 0.0):
                        v1 = "{:.13E}".format(direction_vec.x * node_load)
                        f.write(str(n) + ",1," + v1 + "\n")
                    if (direction_vec.y != 0.0):
                        v2 = "{:.13E}".format(direction_vec.y * node_load)
                        f.write(str(n) + ",2," + v2 + "\n")
                    if (direction_vec.z != 0.0):
                        v3 = "{:.13E}".format(direction_vec.z * node_load)
                        f.write(str(n) + ",3," + v3 + "\n")
                f.write("\n")
            f.write("\n")

    # ********************************************************************************************
    # constraints pressure
    def write_constraints_pressure(self, f, inpfile_split=None):
        if not self.pressure_objects:
            return
        if not (self.analysis_type == "static" or self.analysis_type == "thermomech"):
            return

        # get the faces and face numbers
        self.get_constraints_pressure_faces()

        write_name = "constraints_pressure_element_face_loads"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_faceloads_constraints_pressure(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_faceloads_constraints_pressure(f)

    def write_faceloads_constraints_pressure(self, f):
        # write face loads to file
        for femobj in self.pressure_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            prs_obj = femobj["Object"]
            f.write("** " + prs_obj.Label + "\n")
            rev = -1 if prs_obj.Reversed else 1
            f.write("*DLOAD\n")
            for ref_shape in femobj["PressureFaces"]:
                # the loop is needed for compatibility reason
                # in deprecated method get_pressure_obj_faces_depreciated
                # the face ids where per ref_shape
                f.write("** " + ref_shape[0] + "\n")
                for face, fno in ref_shape[1]:
                    if fno > 0:  # solid mesh face
                        f.write("{},P{},{}\n".format(face, fno, rev * prs_obj.Pressure))
                    # on shell mesh face: fno == 0
                    # normal of element face == face normal
                    elif fno == 0:
                        f.write("{},P,{}\n".format(face, rev * prs_obj.Pressure))
                    # on shell mesh face: fno == -1
                    # normal of element face opposite direction face normal
                    elif fno == -1:
                        f.write("{},P,{}\n".format(face, -1 * rev * prs_obj.Pressure))

    # ********************************************************************************************
    # constraints heatflux
    def write_constraints_heatflux(self, f, inpfile_split=None):
        if not self.heatflux_objects:
            return
        if not self.analysis_type == "thermomech":
            return

        write_name = "constraints_heatflux_element_face_heatflux"
        f.write("\n***********************************************************\n")
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        if inpfile_split is True:
            file_name_splitt = self.mesh_name + "_" + write_name + ".inp"
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_splitt))
            inpfile_splitt = open(join(self.dir_name, file_name_splitt), "w")
            self.write_faceheatflux_constraints_heatflux(inpfile_splitt)
            inpfile_splitt.close()
        else:
            self.write_faceheatflux_constraints_heatflux(f)

    def write_faceheatflux_constraints_heatflux(self, f):
        # write heat flux faces to file
        for hfobj in self.heatflux_objects:
            heatflux_obj = hfobj["Object"]
            f.write("** " + heatflux_obj.Label + "\n")
            if heatflux_obj.ConstraintType == "Convection":
                f.write("*FILM\n")
                for o, elem_tup in heatflux_obj.References:
                    for elem in elem_tup:
                        ho = o.Shape.getElement(elem)
                        if ho.ShapeType == "Face":
                            v = self.mesh_object.FemMesh.getccxVolumesByFace(ho)
                            f.write("** Heat flux on face {}\n".format(elem))
                            for i in v:
                                # SvdW: add factor to force heatflux to units system of t/mm/s/K
                                # OvG: Only write out the VolumeIDs linked to a particular face
                                f.write("{},F{},{},{}\n".format(
                                    i[0],
                                    i[1],
                                    heatflux_obj.AmbientTemp,
                                    heatflux_obj.FilmCoef * 0.001
                                ))
            elif heatflux_obj.ConstraintType == "DFlux":
                f.write("*DFLUX\n")
                for o, elem_tup in heatflux_obj.References:
                    for elem in elem_tup:
                        ho = o.Shape.getElement(elem)
                        if ho.ShapeType == "Face":
                            v = self.mesh_object.FemMesh.getccxVolumesByFace(ho)
                            f.write("** Heat flux on face {}\n".format(elem))
                            for i in v:
                                f.write("{},S{},{}\n".format(
                                    i[0],
                                    i[1],
                                    heatflux_obj.DFlux * 0.001
                                ))

    # ********************************************************************************************
    # constraints fluidsection
    def write_constraints_fluidsection(self, f):
        if not self.fluidsection_objects:
            return
        if not self.analysis_type == "thermomech":
            return

        # write constraint to file
        f.write("\n***********************************************************\n")
        f.write("** FluidSection constraints\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        if os.path.exists(self.fluid_inout_nodes_file):
            inout_nodes_file = open(self.fluid_inout_nodes_file, "r")
            lines = inout_nodes_file.readlines()
            inout_nodes_file.close()
        else:
            FreeCAD.Console.PrintError(
                "1DFlow inout nodes file not found: {}\n"
                .format(self.fluid_inout_nodes_file)
            )
        # get nodes
        self.get_constraints_fluidsection_nodes()
        for femobj in self.fluidsection_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            fluidsection_obj = femobj["Object"]
            f.write("** " + fluidsection_obj.Label + "\n")
            if fluidsection_obj.SectionType == "Liquid":
                if fluidsection_obj.LiquidSectionType == "PIPE INLET":
                    f.write("**Fluid Section Inlet \n")
                    if fluidsection_obj.InletPressureActive is True:
                        f.write("*BOUNDARY \n")
                        for n in femobj["Nodes"]:
                            for line in lines:
                                b = line.split(",")
                                if int(b[0]) == n and b[3] == "PIPE INLET\n":
                                    # degree of freedom 2 is for defining pressure
                                    f.write("{},{},{},{}\n".format(
                                        b[0],
                                        "2",
                                        "2",
                                        fluidsection_obj.InletPressure
                                    ))
                    if fluidsection_obj.InletFlowRateActive is True:
                        f.write("*BOUNDARY,MASS FLOW \n")
                        for n in femobj["Nodes"]:
                            for line in lines:
                                b = line.split(",")
                                if int(b[0]) == n and b[3] == "PIPE INLET\n":
                                    # degree of freedom 1 is for defining flow rate
                                    # factor applied to convert unit from kg/s to t/s
                                    f.write("{},{},{},{}\n".format(
                                        b[1],
                                        "1",
                                        "1",
                                        fluidsection_obj.InletFlowRate * 0.001
                                    ))
                elif fluidsection_obj.LiquidSectionType == "PIPE OUTLET":
                    f.write("**Fluid Section Outlet \n")
                    if fluidsection_obj.OutletPressureActive is True:
                        f.write("*BOUNDARY \n")
                        for n in femobj["Nodes"]:
                            for line in lines:
                                b = line.split(",")
                                if int(b[0]) == n and b[3] == "PIPE OUTLET\n":
                                    # degree of freedom 2 is for defining pressure
                                    f.write("{},{},{},{}\n".format(
                                        b[0],
                                        "2",
                                        "2",
                                        fluidsection_obj.OutletPressure
                                    ))
                    if fluidsection_obj.OutletFlowRateActive is True:
                        f.write("*BOUNDARY,MASS FLOW \n")
                        for n in femobj["Nodes"]:
                            for line in lines:
                                b = line.split(",")
                                if int(b[0]) == n and b[3] == "PIPE OUTLET\n":
                                    # degree of freedom 1 is for defining flow rate
                                    # factor applied to convert unit from kg/s to t/s
                                    f.write("{},{},{},{}\n".format(
                                        b[1],
                                        "1",
                                        "1",
                                        fluidsection_obj.OutletFlowRate * 0.001
                                    ))

    # ********************************************************************************************
    # step begin and end
    def write_step_begin(self, f):
        f.write("\n***********************************************************\n")
        f.write("** At least one step is needed to run an CalculiX analysis of FreeCAD\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
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
                step += ", INC=" + str(self.solver_obj.IterationsThermoMechMaximum)
            elif self.analysis_type == "static" or self.analysis_type == "frequency":
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
            elif self.analysis_type == "static" or self.analysis_type == "frequency":
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
        # write analysis type line, analysis parameter line
        f.write(analysis_type + "\n")
        f.write(analysis_parameter + "\n")

    def write_step_end(self, f):
        f.write("\n***********************************************************\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        f.write("*END STEP \n")

    # ********************************************************************************************
    # output types
    def write_outputs_types(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Outputs --> frd file\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
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
        f.write("** written by {} function\n".format(
            sys._getframe().f_code.co_name
        ))
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
    # material and fem element type
    def write_element_sets_material_and_femelement_type(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Element sets for materials and FEM element type (solid, shell, beam, fluid)\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))

        # in any case if we have beams, we're going to need the element ids for the rotation elsets
        if self.beamsection_objects:
            # we will need to split the beam even for one beamobj
            # because no beam in z-direction can be used in ccx without a special adjustment
            # thus they need an own ccx_elset
            self.get_element_rotation1D_elements()

        # get the element ids for face and edge elements and write them into the objects
        if len(self.shellthickness_objects) > 1:
            self.get_element_geometry2D_elements()
        if len(self.beamsection_objects) > 1:
            self.get_element_geometry1D_elements()
        if len(self.fluidsection_objects) > 1:
            self.get_element_fluid1D_elements()

        # get the element ids for material objects and write them into the material object
        if len(self.material_objects) > 1:
            self.get_material_elements()

        # create the ccx_elsets
        if len(self.material_objects) == 1:
            if self.femmesh.Volumes:
                # we only could do this for volumes, if a mesh contains volumes
                # we're going to use them in the analysis
                # but a mesh could contain the element faces of the volumes as faces
                # and the edges of the faces as edges
                # there we have to check for some geometric objects
                self.get_ccx_elsets_single_mat_solid()
            if len(self.shellthickness_objects) == 1:
                self.get_ccx_elsets_single_mat_single_shell()
            elif len(self.shellthickness_objects) > 1:
                self.get_ccx_elsets_single_mat_multiple_shell()
            if len(self.beamsection_objects) == 1:
                self.get_ccx_elsets_single_mat_single_beam()
            elif len(self.beamsection_objects) > 1:
                self.get_ccx_elsets_single_mat_multiple_beam()
            if len(self.fluidsection_objects) == 1:
                self.get_ccx_elsets_single_mat_single_fluid()
            elif len(self.fluidsection_objects) > 1:
                self.get_ccx_elsets_single_mat_multiple_fluid()
        elif len(self.material_objects) > 1:
            if self.femmesh.Volumes:
                # we only could do this for volumes, if a mseh contains volumes
                # we're going to use them in the analysis
                # but a mesh could contain the element faces of the volumes as faces
                # and the edges of the faces as edges
                # there we have to check for some geometric objects
                # volume is a bit special
                # because retrieving ids from group mesh data is implemented
                self.get_ccx_elsets_multiple_mat_solid()
            if len(self.shellthickness_objects) == 1:
                self.get_ccx_elsets_multiple_mat_single_shell()
            elif len(self.shellthickness_objects) > 1:
                self.get_ccx_elsets_multiple_mat_multiple_shell()
            if len(self.beamsection_objects) == 1:
                self.get_ccx_elsets_multiple_mat_single_beam()
            elif len(self.beamsection_objects) > 1:
                self.get_ccx_elsets_multiple_mat_multiple_beam()
            if len(self.fluidsection_objects) == 1:
                self.get_ccx_elsets_multiple_mat_single_fluid()
            elif len(self.fluidsection_objects) > 1:
                self.get_ccx_elsets_multiple_mat_multiple_fluid()

        # TODO: some elementIDs are collected for 1D-Flow calculation,
        # this should be a def somewhere else, preferable inside the get_ccx_elsets_... methods
        for ccx_elset in self.ccx_elsets:
            # use six to be sure to be Python 2.7 and 3.x compatible
            if ccx_elset["ccx_elset"] \
                    and not isinstance(ccx_elset["ccx_elset"], six.string_types):
                if "fluidsection_obj"in ccx_elset:
                    fluidsec_obj = ccx_elset["fluidsection_obj"]
                    if fluidsec_obj.SectionType == "Liquid":
                        if (fluidsec_obj.LiquidSectionType == "PIPE INLET") \
                                or (fluidsec_obj.LiquidSectionType == "PIPE OUTLET"):
                            elsetchanged = False
                            counter = 0
                            for elid in ccx_elset["ccx_elset"]:
                                counter = counter + 1
                                if (elsetchanged is False) \
                                        and (fluidsec_obj.LiquidSectionType == "PIPE INLET"):
                                    # 3rd index is to track which line nr the element is defined
                                    self.FluidInletoutlet_ele.append(
                                        [str(elid), fluidsec_obj.LiquidSectionType, 0]
                                    )
                                    elsetchanged = True
                                elif (fluidsec_obj.LiquidSectionType == "PIPE OUTLET") \
                                        and (counter == len(ccx_elset["ccx_elset"])):
                                    # 3rd index is to track which line nr the element is defined
                                    self.FluidInletoutlet_ele.append(
                                        [str(elid), fluidsec_obj.LiquidSectionType, 0]
                                    )

        # write ccx_elsets to file
        for ccx_elset in self.ccx_elsets:
            f.write("*ELSET,ELSET=" + ccx_elset["ccx_elset_name"] + "\n")
            # use six to be sure to be Python 2.7 and 3.x compatible
            if isinstance(ccx_elset["ccx_elset"], six.string_types):
                f.write(ccx_elset["ccx_elset"] + "\n")
            else:
                for elid in ccx_elset["ccx_elset"]:
                    f.write(str(elid) + ",\n")

    # self.ccx_elsets = [ {
    #                        "ccx_elset" : [e1, e2, e3, ... , en] or elements set name strings
    #                        "ccx_elset_name" : "ccx_identifier_elset"
    #                        "mat_obj_name" : "mat_obj.Name"
    #                        "ccx_mat_name" : "mat_obj.Material["Name"]"   !!! not unique !!!
    #                        "beamsection_obj" : "beamsection_obj"         if exists
    #                        "fluidsection_obj" : "fluidsection_obj"       if exists
    #                        "shellthickness_obj" : shellthickness_obj"    if exists
    #                        "beam_normal" : normal vector                 for beams only
    #                     },
    #                     {}, ... , {} ]

    # beam
    # TODO support multiple beamrotations
    # we do not need any more any data from the rotation document object,
    # thus we do not need to save the rotation document object name in the else
    def get_ccx_elsets_single_mat_single_beam(self):
        mat_obj = self.material_objects[0]["Object"]
        beamsec_obj = self.beamsection_objects[0]["Object"]
        beamrot_data = self.beamrotation_objects[0]
        for i, beamdirection in enumerate(beamrot_data["FEMRotations1D"]):
            # ID's for this direction
            elset_data = beamdirection["ids"]
            names = [
                {"short": "M0"},
                {"short": "B0"},
                {"short": beamrot_data["ShortName"]},
                {"short": "D" + str(i)}
            ]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            ccx_elset["beamsection_obj"] = beamsec_obj
            # normal for this direction
            ccx_elset["beam_normal"] = beamdirection["normal"]
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_beam(self):
        mat_obj = self.material_objects[0]["Object"]
        beamrot_data = self.beamrotation_objects[0]
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data["Object"]
            beamsec_ids = set(beamsec_data["FEMElements"])
            for i, beamdirection in enumerate(beamrot_data["FEMRotations1D"]):
                beamdir_ids = set(beamdirection["ids"])
                # empty intersection sets possible
                elset_data = list(sorted(beamsec_ids.intersection(beamdir_ids)))
                if elset_data:
                    names = [
                        {"short": "M0"},
                        {"short": beamsec_data["ShortName"]},
                        {"short": beamrot_data["ShortName"]},
                        {"short": "D" + str(i)}
                    ]
                    ccx_elset = {}
                    ccx_elset["ccx_elset"] = elset_data
                    ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
                    ccx_elset["mat_obj_name"] = mat_obj.Name
                    ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
                    ccx_elset["beamsection_obj"] = beamsec_obj
                    # normal for this direction
                    ccx_elset["beam_normal"] = beamdirection["normal"]
                    self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_beam(self):
        beamsec_obj = self.beamsection_objects[0]["Object"]
        beamrot_data = self.beamrotation_objects[0]
        for mat_data in self.material_objects:
            mat_obj = mat_data["Object"]
            mat_ids = set(mat_data["FEMElements"])
            for i, beamdirection in enumerate(beamrot_data["FEMRotations1D"]):
                beamdir_ids = set(beamdirection["ids"])
                elset_data = list(sorted(mat_ids.intersection(beamdir_ids)))
                if elset_data:
                    names = [
                        {"short": mat_data["ShortName"]},
                        {"short": "B0"},
                        {"short": beamrot_data["ShortName"]},
                        {"short": "D" + str(i)}
                    ]
                    ccx_elset = {}
                    ccx_elset["ccx_elset"] = elset_data
                    ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
                    ccx_elset["mat_obj_name"] = mat_obj.Name
                    ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
                    ccx_elset["beamsection_obj"] = beamsec_obj
                    # normal for this direction
                    ccx_elset["beam_normal"] = beamdirection["normal"]
                    self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_beam(self):
        beamrot_data = self.beamrotation_objects[0]
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data["Object"]
            beamsec_ids = set(beamsec_data["FEMElements"])
            for mat_data in self.material_objects:
                mat_obj = mat_data["Object"]
                mat_ids = set(mat_data["FEMElements"])
                for i, beamdirection in enumerate(beamrot_data["FEMRotations1D"]):
                    beamdir_ids = set(beamdirection["ids"])
                    # empty intersection sets possible
                    elset_data = list(sorted(
                        beamsec_ids.intersection(mat_ids).intersection(beamdir_ids)
                    ))
                    if elset_data:
                        names = [
                            {"short": mat_data["ShortName"]},
                            {"short": beamsec_data["ShortName"]},
                            {"short": beamrot_data["ShortName"]},
                            {"short": "D" + str(i)}
                        ]
                        ccx_elset = {}
                        ccx_elset["ccx_elset"] = elset_data
                        ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
                        ccx_elset["mat_obj_name"] = mat_obj.Name
                        ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
                        ccx_elset["beamsection_obj"] = beamsec_obj
                        # normal for this direction
                        ccx_elset["beam_normal"] = beamdirection["normal"]
                        self.ccx_elsets.append(ccx_elset)

    # fluid
    def get_ccx_elsets_single_mat_single_fluid(self):
        mat_obj = self.material_objects[0]["Object"]
        fluidsec_obj = self.fluidsection_objects[0]["Object"]
        elset_data = self.ccx_eedges
        names = [{"short": "M0"}, {"short": "F0"}]
        ccx_elset = {}
        ccx_elset["ccx_elset"] = elset_data
        ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
        ccx_elset["mat_obj_name"] = mat_obj.Name
        ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
        ccx_elset["fluidsection_obj"] = fluidsec_obj
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_fluid(self):
        mat_obj = self.material_objects[0]["Object"]
        for fluidsec_data in self.fluidsection_objects:
            fluidsec_obj = fluidsec_data["Object"]
            elset_data = fluidsec_data["FEMElements"]
            names = [{"short": "M0"}, {"short": fluidsec_data["ShortName"]}]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            ccx_elset["fluidsection_obj"] = fluidsec_obj
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_fluid(self):
        fluidsec_obj = self.fluidsection_objects[0]["Object"]
        for mat_data in self.material_objects:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [{"short": mat_data["ShortName"]}, {"short": "F0"}]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            ccx_elset["fluidsection_obj"] = fluidsec_obj
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_fluid(self):
        for fluidsec_data in self.fluidsection_objects:
            fluidsec_obj = fluidsec_data["Object"]
            for mat_data in self.material_objects:
                mat_obj = mat_data["Object"]
                fluidsec_ids = set(fluidsec_data["FEMElements"])
                mat_ids = set(mat_data["FEMElements"])
                # empty intersection sets possible
                elset_data = list(sorted(fluidsec_ids.intersection(mat_ids)))
                if elset_data:
                    names = [
                        {"short": mat_data["ShortName"]},
                        {"short": fluidsec_data["ShortName"]}
                    ]
                    ccx_elset = {}
                    ccx_elset["ccx_elset"] = elset_data
                    ccx_elset["ccx_elset_name"] = get_ccx_elset_name_short(names)
                    ccx_elset["mat_obj_name"] = mat_obj.Name
                    ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
                    ccx_elset["fluidsection_obj"] = fluidsec_obj
                    self.ccx_elsets.append(ccx_elset)

    # shell
    def get_ccx_elsets_single_mat_single_shell(self):
        mat_obj = self.material_objects[0]["Object"]
        shellth_obj = self.shellthickness_objects[0]["Object"]
        elset_data = self.ccx_efaces
        names = [
            {"long": mat_obj.Name, "short": "M0"},
            {"long": shellth_obj.Name, "short": "S0"}
        ]
        ccx_elset = {}
        ccx_elset["ccx_elset"] = elset_data
        ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
        ccx_elset["mat_obj_name"] = mat_obj.Name
        ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
        ccx_elset["shellthickness_obj"] = shellth_obj
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_shell(self):
        mat_obj = self.material_objects[0]["Object"]
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data["Object"]
            elset_data = shellth_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": "M0"},
                {"long": shellth_obj.Name, "short": shellth_data["ShortName"]}
            ]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            ccx_elset["shellthickness_obj"] = shellth_obj
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_shell(self):
        shellth_obj = self.shellthickness_objects[0]["Object"]
        for mat_data in self.material_objects:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": mat_data["ShortName"]},
                {"long": shellth_obj.Name, "short": "S0"}
            ]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            ccx_elset["shellthickness_obj"] = shellth_obj
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_shell(self):
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data["Object"]
            for mat_data in self.material_objects:
                mat_obj = mat_data["Object"]
                shellth_ids = set(shellth_data["FEMElements"])
                mat_ids = set(mat_data["FEMElements"])
                # empty intersection sets possible
                elset_data = list(sorted(shellth_ids.intersection(mat_ids)))
                if elset_data:
                    names = [
                        {"long": mat_obj.Name, "short": mat_data["ShortName"]},
                        {"long": shellth_obj.Name, "short": shellth_data["ShortName"]}
                    ]
                    ccx_elset = {}
                    ccx_elset["ccx_elset"] = elset_data
                    ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
                    ccx_elset["mat_obj_name"] = mat_obj.Name
                    ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
                    ccx_elset["shellthickness_obj"] = shellth_obj
                    self.ccx_elsets.append(ccx_elset)

    # solid
    def get_ccx_elsets_single_mat_solid(self):
        mat_obj = self.material_objects[0]["Object"]
        elset_data = self.ccx_evolumes
        names = [
            {"long": mat_obj.Name, "short": "M0"},
            {"long": "Solid", "short": "Solid"}
        ]
        ccx_elset = {}
        ccx_elset["ccx_elset"] = elset_data
        ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
        ccx_elset["mat_obj_name"] = mat_obj.Name
        ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_solid(self):
        for mat_data in self.material_objects:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": mat_data["ShortName"]},
                {"long": "Solid", "short": "Solid"}
            ]
            ccx_elset = {}
            ccx_elset["ccx_elset"] = elset_data
            ccx_elset["ccx_elset_name"] = get_ccx_elset_name_standard(names)
            ccx_elset["mat_obj_name"] = mat_obj.Name
            ccx_elset["ccx_mat_name"] = mat_obj.Material["Name"]
            self.ccx_elsets.append(ccx_elset)

    def write_materials(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Materials\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
        f.write("** Young\'s modulus unit is MPa = N/mm2\n")
        if self.analysis_type == "frequency" \
                or self.selfweight_objects \
                or (
                    self.analysis_type == "thermomech"
                    and not self.solver_obj.ThermoMechSteadyState
                ):
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
            if self.analysis_type == "frequency" \
                    or self.selfweight_objects \
                    or (
                        self.analysis_type == "thermomech"
                        and not self.solver_obj.ThermoMechSteadyState
                    ):
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

            if self.analysis_type == "frequency" \
                    or self.selfweight_objects \
                    or (
                        self.analysis_type == "thermomech"
                        and not self.solver_obj.ThermoMechSteadyState
                    ):
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

    def write_femelementsets(self, f):
        f.write("\n***********************************************************\n")
        f.write("** Sections\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
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
# ccx elset names:
# M .. Material
# B .. Beam
# R .. BeamRotation
# D ..Direction
# F .. Fluid
# S .. Shell,
# TODO write comment into input file to elset ids and elset attributes
def get_ccx_elset_name_standard(names):
    # standard max length = 80
    ccx_elset_name = ""
    for name in names:
        ccx_elset_name += name["long"]
    if len(ccx_elset_name) < 81:
        return ccx_elset_name
    else:
        ccx_elset_name = ""
        for name in names:
            ccx_elset_name += name["short"]
        if len(ccx_elset_name) < 81:
            return ccx_elset_name
        else:
            error = (
                "FEM: Trouble in ccx input file, because an "
                "elset name is longer than 80 character! {}\n"
                .format(ccx_elset_name)
            )
            raise Exception(error)


def get_ccx_elset_name_short(names):
    # restricted max length = 20 (beam elsets)
    ccx_elset_name = ""
    for name in names:
        ccx_elset_name += name["short"]
    if len(ccx_elset_name) < 21:
        return ccx_elset_name
    else:
        error = (
            "FEM: Trouble in ccx input file, because an"
            "beam elset name is longer than 20 character! {}\n"
            .format(ccx_elset_name)
        )
        raise Exception(error)


def is_fluid_section_inlet_outlet(ccx_elsets):
    """ Fluid section: Inlet and Outlet requires special element definition
    """
    for ccx_elset in ccx_elsets:
        if ccx_elset["ccx_elset"]:
            if "fluidsection_obj" in ccx_elset:  # fluid mesh
                fluidsec_obj = ccx_elset["fluidsection_obj"]
                if fluidsec_obj.SectionType == "Liquid":
                    if (fluidsec_obj.LiquidSectionType == "PIPE INLET") \
                            or (fluidsec_obj.LiquidSectionType == "PIPE OUTLET"):
                        return True
    return False


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
            head = str(obj.PumpHeadLoss[i])
            section_geo = section_geo + flow_rate + "," + head + ","
        section_geo = section_geo + "\n"
        return section_geo
    else:
        return ""
##  @}
