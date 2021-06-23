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

# import io
import codecs
import math
import os
import six
import sys
import time
from os.path import join

import FreeCAD
from FreeCAD import Units

from .. import writerbase
from femmesh import meshtools
from femtools import constants
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

    def write_file(self):
        if self.solver_obj.SplitInputWriter is True:
            self.split_inpfile = True
        else:
            self.split_inpfile = False

        # mesh
        inpfile_main = self.write_mesh()

        # element sets for materials and element geometry
        # self.write_element_sets_material_and_femelement_geometry(inpfile_main)
        self.write_element_sets_material_and_femelement_type(inpfile_main)

        if self.fluidsection_objects:
            # some fluidsection objs need special treatment, ccx_elsets are needed for this
            inpfile_main = self.handle_fluidsection_liquid_inlet_outlet(inpfile_main)

        # element sets constraints
        self.write_element_sets_constraints_centrif(inpfile_main)

        # node sets and surface sets
        self.write_node_sets_constraints_fixed(inpfile_main)
        self.write_node_sets_constraints_displacement(inpfile_main)
        self.write_node_sets_constraints_planerotation(inpfile_main)
        self.write_surfaces_constraints_contact(inpfile_main)
        self.write_surfaces_constraints_tie(inpfile_main)
        self.write_surfaces_constraints_sectionprint(inpfile_main)
        self.write_node_sets_constraints_transform(inpfile_main)
        self.write_node_sets_constraints_temperature(inpfile_main)

        # materials and fem element types
        self.write_materials(inpfile_main)
        self.write_constraints_initialtemperature(inpfile_main)
        # self.write_femelement_geometry(inpfile_main)
        self.write_femelementsets(inpfile_main)

        # constraints independent from steps
        self.write_constraints_planerotation(inpfile_main)
        self.write_constraints_contact(inpfile_main)
        self.write_constraints_tie(inpfile_main)
        self.write_constraints_transform(inpfile_main)

        # step begin
        self.write_step_begin(inpfile_main)

        # constraints dependent from steps
        self.write_constraints_fixed(inpfile_main)
        self.write_constraints_displacement(inpfile_main)
        self.write_constraints_sectionprint(inpfile_main)
        self.write_constraints_selfweight(inpfile_main)
        self.write_constraints_centrif(inpfile_main)
        self.write_constraints_force(inpfile_main)
        self.write_constraints_pressure(inpfile_main)
        self.write_constraints_temperature(inpfile_main)
        self.write_constraints_heatflux(inpfile_main)
        self.write_constraints_fluidsection(inpfile_main)

        # output and step end
        self.write_outputs_types(inpfile_main)
        self.write_step_end(inpfile_main)

        # footer
        self.write_footer(inpfile_main)
        inpfile_main.close()

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
        analysis_types,
        sets_getter_method,
        write_name,
        sets_writer_method,
        caller_method_name="",
        write_before="",
        write_after="",
    ):
        def constraint_sets_loop_writing(the_file, femobjs, write_before, write_after):
            if write_before != "":
                f.write(write_before)
            for femobj in femobjs:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                the_obj = femobj["Object"]
                f.write("** {}\n".format(the_obj.Label))
                sets_writer_method(the_file, femobj, the_obj)
            if write_after != "":
                f.write(write_after)

        if not femobjs:
            return

        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        # get the sets
        sets_getter_method()

        # write sets to file
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(write_name.replace("_", " ")))
        f.write("** written by {} function\n".format(caller_method_name))

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
        analysis_types,
        constraint_title_name,
        constraint_writer_method,
        caller_method_name="",
        write_before="",
        write_after="",
    ):
        if not femobjs:
            return

        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        # write constraint to file
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(constraint_title_name))
        f.write("** written by {} function\n".format(caller_method_name))
        if write_before != "":
            f.write(write_before)
        for femobj in femobjs:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            the_obj = femobj["Object"]
            f.write("** {}\n".format(the_obj.Label))
            constraint_writer_method(f, femobj, the_obj)
        if write_after != "":
            f.write(write_after)

    # ********************************************************************************************
    # constraints fixed
    def write_node_sets_constraints_fixed(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.fixed_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_fixed_nodes,
            write_name="constraints_fixed_node_sets",
            sets_writer_method=self.write_node_sets_nodes_constraints_fixed,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_fixed(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.fixed_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Fixed Constraints",
            constraint_writer_method=self.constraint_fixed_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_node_sets_nodes_constraints_fixed(self, f, femobj, fix_obj):
        if self.femmesh.Volumes \
                and (len(self.shellthickness_objects) > 0 or len(self.beamsection_objects) > 0):
            if len(femobj["NodesSolid"]) > 0:
                f.write("*NSET,NSET={}Solid\n".format(fix_obj.Name))
                for n in femobj["NodesSolid"]:
                    f.write("{},\n".format(n))
            if len(femobj["NodesFaceEdge"]) > 0:
                f.write("*NSET,NSET={}FaceEdge\n".format(fix_obj.Name))
                for n in femobj["NodesFaceEdge"]:
                    f.write("{},\n".format(n))
        else:
            f.write("*NSET,NSET=" + fix_obj.Name + "\n")
            for n in femobj["Nodes"]:
                f.write("{},\n".format(n))

    def constraint_fixed_writer(self, f, femobj, fix_obj):
        if self.femmesh.Volumes \
                and (len(self.shellthickness_objects) > 0 or len(self.beamsection_objects) > 0):
            if len(femobj["NodesSolid"]) > 0:
                f.write("*BOUNDARY\n")
                f.write(fix_obj.Name + "Solid" + ",1\n")
                f.write(fix_obj.Name + "Solid" + ",2\n")
                f.write(fix_obj.Name + "Solid" + ",3\n")
                f.write("\n")
            if len(femobj["NodesFaceEdge"]) > 0:
                f.write("*BOUNDARY\n")
                f.write(fix_obj.Name + "FaceEdge" + ",1\n")
                f.write(fix_obj.Name + "FaceEdge" + ",2\n")
                f.write(fix_obj.Name + "FaceEdge" + ",3\n")
                f.write(fix_obj.Name + "FaceEdge" + ",4\n")
                f.write(fix_obj.Name + "FaceEdge" + ",5\n")
                f.write(fix_obj.Name + "FaceEdge" + ",6\n")
                f.write("\n")
        else:
            f.write("*BOUNDARY\n")
            f.write(fix_obj.Name + ",1\n")
            f.write(fix_obj.Name + ",2\n")
            f.write(fix_obj.Name + ",3\n")
            if self.beamsection_objects or self.shellthickness_objects:
                f.write(fix_obj.Name + ",4\n")
                f.write(fix_obj.Name + ",5\n")
                f.write(fix_obj.Name + ",6\n")
            f.write("\n")

    # ********************************************************************************************
    # constraints displacement
    def write_node_sets_constraints_displacement(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.displacement_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_displacement_nodes,
            write_name="constraints_displacement_node_sets",
            sets_writer_method=self.write_node_sets_nodes_constraints_displacement,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_displacement(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.displacement_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Displacement constraint applied",
            constraint_writer_method=self.constraint_displacement_writer,
            caller_method_name=sys._getframe().f_code.co_name,
            write_after="\n",
        )

    def write_node_sets_nodes_constraints_displacement(self, f, femobj, disp_obj):
        f.write("*NSET,NSET={}\n".format(disp_obj.Name))
        for n in femobj["Nodes"]:
            f.write("{},\n".format(n))

    def constraint_displacement_writer(self, f, femobj, disp_obj):
        f.write("*BOUNDARY\n")
        if disp_obj.xFix:
            f.write("{},1\n".format(disp_obj.Name))
        elif not disp_obj.xFree:
            f.write("{},1,1,{}\n".format(disp_obj.Name, disp_obj.xDisplacement))
        if disp_obj.yFix:
            f.write("{},2\n".format(disp_obj.Name))
        elif not disp_obj.yFree:
            f.write("{},2,2,{}\n".format(disp_obj.Name, disp_obj.yDisplacement))
        if disp_obj.zFix:
            f.write("{},3\n".format(disp_obj.Name))
        elif not disp_obj.zFree:
            f.write("{},3,3,{}\n".format(disp_obj.Name, disp_obj.zDisplacement))

        if self.beamsection_objects or self.shellthickness_objects:
            if disp_obj.rotxFix:
                f.write("{},4\n".format(disp_obj.Name))
            elif not disp_obj.rotxFree:
                f.write("{},4,4,{}\n".format(disp_obj.Name, disp_obj.xRotation))
            if disp_obj.rotyFix:
                f.write("{},5\n".format(disp_obj.Name))
            elif not disp_obj.rotyFree:
                f.write("{},5,5,{}\n".format(disp_obj.Name, disp_obj.yRotation))
            if disp_obj.rotzFix:
                f.write("{},6\n".format(disp_obj.Name))
            elif not disp_obj.rotzFree:
                f.write("{},6,6,{}\n".format(disp_obj.Name, disp_obj.zRotation))

    # ********************************************************************************************
    # constraints planerotation
    def write_node_sets_constraints_planerotation(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.planerotation_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_planerotation_nodes,
            write_name="constraints_planerotation_node_sets",
            sets_writer_method=self.write_node_sets_nodes_constraints_planerotation,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_planerotation(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.planerotation_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="PlaneRotation Constraints",
            constraint_writer_method=self.constraint_planerotation_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_node_sets_nodes_constraints_planerotation(self, f, femobj, fric_obj):
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
        l_nodes = femobj["Nodes"]
        f.write("*NSET,NSET={}\n".format(fric_obj.Name))
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
            f.write("{},\n".format(MPC_nodes[i]))

    def constraint_planerotation_writer(self, f, femobj, fric_obj):
        f.write("*MPC\n")
        f.write("PLANE,{}\n".format(fric_obj.Name))

    # ********************************************************************************************
    # constraints contact
    def write_surfaces_constraints_contact(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.contact_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_contact_faces,
            write_name="constraints_contact_surface_sets",
            sets_writer_method=self.write_surfacefaces_constraints_contact,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_contact(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.contact_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Contact Constraints",
            constraint_writer_method=self.constraint_contact_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_surfacefaces_constraints_contact(self, f, femobj, contact_obj):
        # slave DEP
        f.write("*SURFACE, NAME=DEP{}\n".format(contact_obj.Name))
        for i in femobj["ContactSlaveFaces"]:
            f.write("{},S{}\n".format(i[0], i[1]))
        # master IND
        f.write("*SURFACE, NAME=IND{}\n".format(contact_obj.Name))
        for i in femobj["ContactMasterFaces"]:
            f.write("{},S{}\n".format(i[0], i[1]))

    def constraint_contact_writer(self, f, femobj, contact_obj):
        f.write(
            "*CONTACT PAIR, INTERACTION=INT{},TYPE=SURFACE TO SURFACE\n"
            .format(contact_obj.Name)
        )
        ind_surf = "IND" + contact_obj.Name
        dep_surf = "DEP" + contact_obj.Name
        f.write("{},{}\n".format(dep_surf, ind_surf))
        f.write("*SURFACE INTERACTION, NAME=INT{}\n".format(contact_obj.Name))
        f.write("*SURFACE BEHAVIOR,PRESSURE-OVERCLOSURE=LINEAR\n")
        slope = contact_obj.Slope
        f.write("{} \n".format(slope))
        friction = contact_obj.Friction
        if friction > 0:
            f.write("*FRICTION \n")
            stick = (slope / 10.0)
            f.write("{}, {} \n".format(friction, stick))

    # ********************************************************************************************
    # constraints tie
    def write_surfaces_constraints_tie(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.tie_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_tie_faces,
            write_name="constraints_tie_surface_sets",
            sets_writer_method=self.write_surfacefaces_constraints_tie,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_tie(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.tie_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Tie Constraints",
            constraint_writer_method=self.constraint_tie_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_surfacefaces_constraints_tie(self, f, femobj, tie_obj):
        # slave DEP
        f.write("*SURFACE, NAME=TIE_DEP{}\n".format(tie_obj.Name))
        for i in femobj["TieSlaveFaces"]:
            f.write("{},S{}\n".format(i[0], i[1]))
        # master IND
        f.write("*SURFACE, NAME=TIE_IND{}\n".format(tie_obj.Name))
        for i in femobj["TieMasterFaces"]:
            f.write("{},S{}\n".format(i[0], i[1]))

    def constraint_tie_writer(self, f, femobj, tie_obj):
        tolerance = str(tie_obj.Tolerance.getValueAs("mm")).rstrip()
        f.write(
            "*TIE, POSITION TOLERANCE={}, ADJUST=NO, NAME=TIE{}\n"
            .format(tolerance, tie_obj.Name)
        )
        ind_surf = "TIE_IND{}".format(tie_obj.Name)
        dep_surf = "TIE_DEP{}".format(tie_obj.Name)
        f.write("{},{}\n".format(dep_surf, ind_surf))

    # ********************************************************************************************
    # constraints sectionprint
    def write_surfaces_constraints_sectionprint(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.sectionprint_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_sectionprint_faces,
            write_name="constraints_sectionprint_surface_sets",
            sets_writer_method=self.write_surfacefaces_constraints_sectionprint,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_sectionprint(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.sectionprint_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="SectionPrint Constraints",
            constraint_writer_method=self.constraint_sectionprint_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_surfacefaces_constraints_sectionprint(self, f, femobj, sectionprint_obj):
        f.write("*SURFACE, NAME=SECTIONFACE{}\n".format(sectionprint_obj.Name))
        for i in femobj["SectionPrintFaces"]:
            f.write("{},S{}\n".format(i[0], i[1]))

    def constraint_sectionprint_writer(self, f, femobj, sectionprint_obj):
        f.write(
            "*SECTION PRINT, SURFACE=SECTIONFACE{}, NAME=SECTIONPRINT{}\n"
            .format(sectionprint_obj.Name, sectionprint_obj.Name)
        )
        f.write("SOF, SOM, SOAREA\n")

    # ********************************************************************************************
    # constraints transform
    def write_node_sets_constraints_transform(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.transform_objects,
            analysis_types="all",  # write for all analysis types
            sets_getter_method=self.get_constraints_transform_nodes,
            write_name="constraints_transform_node_sets",
            sets_writer_method=self.write_node_sets_nodes_constraints_transform,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_transform(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.transform_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Transform Constraints",
            constraint_writer_method=self.constraint_transform_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_node_sets_nodes_constraints_transform(self, f, femobj, trans_obj):
        if trans_obj.TransformType == "Rectangular":
            f.write("*NSET,NSET=Rect{}\n".format(trans_obj.Name))
        elif trans_obj.TransformType == "Cylindrical":
            f.write("*NSET,NSET=Cylin{}\n".format(trans_obj.Name))
        for n in femobj["Nodes"]:
            f.write("{},\n".format(n))

    def constraint_transform_writer(self, f, femobj, trans_obj):
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
    def write_node_sets_constraints_temperature(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.temperature_objects,
            analysis_types=["thermomech"],
            sets_getter_method=self.get_constraints_temperature_nodes,
            write_name="constraints_temperature_node_sets",
            sets_writer_method=self.write_node_sets_nodes_constraints_temperature,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_temperature(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.temperature_objects,
            analysis_types=["thermomech"],
            constraint_title_name="Fixed temperature constraint applied",
            constraint_writer_method=self.constraint_temperature_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_node_sets_nodes_constraints_temperature(self, f, femobj, temp_obj):
        f.write("*NSET,NSET={}\n".format(temp_obj.Name))
        for n in femobj["Nodes"]:
            f.write("{},\n".format(n))

    def constraint_temperature_writer(self, f, femobj, temp_obj):
        NumberOfNodes = len(femobj["Nodes"])
        if temp_obj.ConstraintType == "Temperature":
            f.write("*BOUNDARY\n")
            f.write("{},11,11,{}\n".format(temp_obj.Name, temp_obj.Temperature))
            f.write("\n")
        elif temp_obj.ConstraintType == "CFlux":
            f.write("*CFLUX\n")
            f.write("{},11,{}\n".format(
                temp_obj.Name,
                temp_obj.CFlux * 0.001 / NumberOfNodes
            ))
            f.write("\n")

    # ********************************************************************************************
    # constraints initialtemperature
    def write_constraints_initialtemperature(self, f):
        if not self.initialtemperature_objects:
            return
        if self.analysis_type not in ["thermomech"]:
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
        if self.analysis_type not in ["buckling", "static", "thermomech"]:
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
    # constraints centrif
    def write_element_sets_constraints_centrif(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.centrif_objects,
            analysis_types=["buckling", "static", "thermomech"],
            sets_getter_method=self.get_constraints_centrif_elements,
            write_name="constraints_centrif_element_sets",
            sets_writer_method=self.write_element_sets_elements_constraints_centrif,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_constraints_centrif(self, f):
        self.write_constraints_data(
            f,
            femobjs=self.centrif_objects,
            analysis_types="all",  # write for all analysis types
            constraint_title_name="Centrif Constraints",
            constraint_writer_method=self.constraint_centrif_writer,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_element_sets_elements_constraints_centrif(self, f, femobj, centrif_obj):
        f.write("*ELSET,ELSET={}\n".format(centrif_obj.Name))
        # use six to be sure to be Python 2.7 and 3.x compatible
        if isinstance(femobj["FEMElements"], six.string_types):
            f.write("{}\n".format(femobj["FEMElements"]))
        else:
            for e in femobj["FEMElements"]:
                f.write("{},\n".format(e))

    def constraint_centrif_writer(self, f, femobj, centrif_obj):

        # get some data from the centrif_obj
        refobj = centrif_obj.RotationAxis[0][0]
        subobj = centrif_obj.RotationAxis[0][1][0]
        axis = refobj.Shape.getElement(subobj)

        if axis.Curve.TypeId == "Part::GeomLine":
            axiscopy = axis.copy()  # apply global placement to copy
            axiscopy.Placement = refobj.getGlobalPlacement()
            direction = axiscopy.Curve.Direction
            location = axiscopy.Curve.Location
        else:  # no line found, set default
            # TODO: No test at all in the writer
            # they should all be before in prechecks
            location = FreeCAD.Vector(0., 0., 0.)
            direction = FreeCAD.Vector(0., 0., 1.)

        # write to file
        f.write("*DLOAD\n")
        # Why {:.13G} ...
        # ccx uses F20.0 FORTRAN input fields, see in dload.f in ccx's source
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=22759&#p176578
        # example "{:.13G}".format(math.sqrt(2.)*-1e100) and count chars
        f.write(
            "{},CENTRIF,{:.13G},{:.13G},{:.13G},{:.13G},{:.13G},{:.13G},{:.13G}\n"
            .format(
                centrif_obj.Name,
                (2. * math.pi * float(centrif_obj.RotationFrequency.getValueAs("1/s"))) ** 2,
                location.x,
                location.y,
                location.z,
                direction.x,
                direction.y,
                direction.z
            )
        )
        f.write("\n")

    # ********************************************************************************************
    # constraints force
    def write_constraints_force(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.force_objects,
            analysis_types=["buckling", "static", "thermomech"],
            sets_getter_method=self.get_constraints_force_nodeloads,
            write_name="constraints_force_node_loads",
            sets_writer_method=self.write_nodeloads_constraints_force,
            caller_method_name=sys._getframe().f_code.co_name,
            write_before="*CLOAD\n"
        )

    def write_nodeloads_constraints_force(self, f, femobj, force_obj):
        direction_vec = femobj["Object"].DirectionVector
        for ref_shape in femobj["NodeLoadTable"]:
            f.write("** " + ref_shape[0] + "\n")
            for n in sorted(ref_shape[1]):
                node_load = ref_shape[1][n]
                if (direction_vec.x != 0.0):
                    v1 = "{:.13E}".format(direction_vec.x * node_load)
                    f.write("{},1,{}\n".format(n, v1))
                if (direction_vec.y != 0.0):
                    v2 = "{:.13E}".format(direction_vec.y * node_load)
                    f.write("{},2,{}\n".format(n, v2))
                if (direction_vec.z != 0.0):
                    v3 = "{:.13E}".format(direction_vec.z * node_load)
                    f.write("{},3,{}\n".format(n, v3))
            f.write("\n")
        f.write("\n")

    # ********************************************************************************************
    # constraints pressure
    def write_constraints_pressure(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.pressure_objects,
            analysis_types=["buckling", "static", "thermomech"],
            sets_getter_method=self.get_constraints_pressure_faces,
            write_name="constraints_pressure_element_face_loads",
            sets_writer_method=self.write_faceloads_constraints_pressure,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_faceloads_constraints_pressure(self, f, femobj, prs_obj):
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
    def write_constraints_heatflux(self, f):
        self.write_constraints_sets(
            f,
            femobjs=self.heatflux_objects,
            analysis_types=["thermomech"],
            sets_getter_method=self.get_constraints_heatflux_faces,
            write_name="constraints_heatflux_element_face_heatflux",
            sets_writer_method=self.write_faceheatflux_constraints_heatflux,
            caller_method_name=sys._getframe().f_code.co_name,
        )

    def write_faceheatflux_constraints_heatflux(self, f, femobj, heatflux_obj):
        if heatflux_obj.ConstraintType == "Convection":
            heatflux_key_word = "FILM"
            heatflux_facetype = "F"
            # SvdW: add factor to force heatflux to units system of t/mm/s/K
            heatflux_values = "{},{}".format(
                heatflux_obj.AmbientTemp,
                heatflux_obj.FilmCoef * 0.001
            )
        elif heatflux_obj.ConstraintType == "DFlux":
            heatflux_key_word = "DFLUX"
            heatflux_facetype = "S"
            heatflux_values = "{}".format(heatflux_obj.DFlux * 0.001)

        f.write("*{}\n".format(heatflux_key_word))
        for ref_shape in femobj["HeatFluxFaceTable"]:
            elem_string = ref_shape[0]
            face_table = ref_shape[1]
            f.write("** Heat flux on face {}\n".format(elem_string))
            for i in face_table:
                # OvG: Only write out the VolumeIDs linked to a particular face
                f.write("{},{}{},{}\n".format(
                    i[0],
                    heatflux_facetype,
                    i[1],
                    heatflux_values
                ))

    # ********************************************************************************************
    # handle elements for constraints fluidsection with Liquid Inlet or Outlet
    # belongs to write_constraints_fluidsection, should be next method
    # leave the constraints fluidsection code as the last constraint method in this module
    # as it is none standard constraint method compared to all other constraints
    def handle_fluidsection_liquid_inlet_outlet(self, inpfile_main):

        # Fluid sections:
        # fluidsection Liquid inlet outlet objs  requires special element definition
        # to fill self.FluidInletoutlet_ele list the ccx_elset are needed
        # thus this has to be after the creation of ccx_elsets
        # different pipe cross sections will generate ccx_elsets

        self.FluidInletoutlet_ele = []
        self.fluid_inout_nodes_file = join(
            self.dir_name,
            "{}_inout_nodes.txt".format(self.mesh_name)
        )

        def get_fluidsection_inoutlet_obj_if_setdata(ccx_elset):
            if (
                ccx_elset["ccx_elset"]
                # use six to be sure to be Python 2.7 and 3.x compatible
                and not isinstance(ccx_elset["ccx_elset"], six.string_types)
                and "fluidsection_obj" in ccx_elset  # fluid mesh
            ):
                fluidsec_obj = ccx_elset["fluidsection_obj"]
                if (
                    fluidsec_obj.SectionType == "Liquid"
                    and (
                        fluidsec_obj.LiquidSectionType == "PIPE INLET"
                        or fluidsec_obj.LiquidSectionType == "PIPE OUTLET"
                    )
                ):
                    return fluidsec_obj
            return None

        def is_fluidsection_inoutlet_setnames_possible(ccx_elsets):
            for ccx_elset in ccx_elsets:
                if (
                    ccx_elset["ccx_elset"]
                    and "fluidsection_obj" in ccx_elset  # fluid mesh
                ):
                    fluidsec_obj = ccx_elset["fluidsection_obj"]
                    if (
                        fluidsec_obj.SectionType == "Liquid"
                        and (
                            fluidsec_obj.LiquidSectionType == "PIPE INLET"
                            or fluidsec_obj.LiquidSectionType == "PIPE OUTLET"
                        )
                    ):
                        return True
            return False

        # collect elementIDs for fluidsection Liquid inlet outlet objs
        # if they have element data (happens if not "eall")
        for ccx_elset in self.ccx_elsets:
            fluidsec_obj = get_fluidsection_inoutlet_obj_if_setdata(ccx_elset)
            if fluidsec_obj is None:
                continue
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

        # create the correct element definition for fluidsection Liquid inlet outlet objs
        # at least one "fluidsection_obj" needs to be in ccx_elsets and has the attributes
        # TODO: what if there are other objs in elsets?
        if is_fluidsection_inoutlet_setnames_possible(self.ccx_elsets) is not None:
            # it is not distinguished if split input file
            # for split input file the main file is just closed and reopend even if not needed
            inpfile_main.close()
            meshtools.use_correct_fluidinout_ele_def(
                self.FluidInletoutlet_ele,
                self.femmesh_file,
                self.fluid_inout_nodes_file
            )
            inpfile_main = codecs.open(self.file_name, "a", encoding="utf-8")

        return inpfile_main

    # ********************************************************************************************
    # constraints fluidsection
    # TODO:
    # split method into separate methods and move some part into base writer
    # see also method handle_fluidsection_liquid_inlet_outlet
    def write_constraints_fluidsection(self, f):
        if not self.fluidsection_objects:
            return
        if self.analysis_type not in ["thermomech"]:
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
    # material and fem element geometry
    # def write_element_sets_material_and_femelement_geometry(self, f):
    def write_element_sets_material_and_femelement_type(self, f):

        self.get_element_sets_material_and_femelement_geometry()

        # write ccx_elsets to file
        f.write("\n***********************************************************\n")
        f.write("** Element sets for materials and FEM element type (solid, shell, beam, fluid)\n")
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
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
        f.write("** written by {} function\n".format(sys._getframe().f_code.co_name))
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
