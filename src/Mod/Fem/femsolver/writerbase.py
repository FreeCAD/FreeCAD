# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver writer base object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
from os.path import join

import FreeCAD

from femmesh import meshsetsgetter


class FemInputWriter():
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None,
        mat_geo_sets=None
    ):
        # class attributes from parameter values
        self.analysis = analysis_obj
        self.solver_obj = solver_obj
        self.mesh_object = mesh_obj
        self.member = member
        # more attributes
        self.analysis_type = self.solver_obj.AnalysisType
        self.document = self.analysis.Document
        # working dir
        self.dir_name = dir_name
        # if dir_name was not given or if it exists but isn't empty: create a temporary dir
        # Purpose: makes sure the analysis can be run even on wired situation
        if not dir_name:
            FreeCAD.Console.PrintWarning(
                "Error: FemInputWriter has no working_dir --> "
                "we are going to make a temporary one!\n"
            )
            self.dir_name = self.document.TransientDir.replace(
                "\\", "/"
            ) + "/FemAnl_" + analysis_obj.Uid[-4:]
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)

        # new class attributes
        self.fc_ver = FreeCAD.Version()
        self.ccx_nall = "Nall"
        self.ccx_eall = "Eall"
        self.ccx_evolumes = "Evolumes"
        self.ccx_efaces = "Efaces"
        self.ccx_eedges = "Eedges"
        self.mat_geo_sets = mat_geo_sets
        if self.mesh_object:
            if hasattr(self.mesh_object, "Shape"):
                self.theshape = self.mesh_object.Shape
            elif hasattr(self.mesh_object, "Part"):
                self.theshape = self.mesh_object.Part
            else:
                FreeCAD.Console.PrintWarning(
                    "A finite mesh without a link to a Shape was given. "
                    "Happen on pure mesh objects. "
                    "Not all methods do work without this link.\n"
                )
                # ATM only used in meshtools.get_femelement_direction1D_set
                # TODO somehow this is not smart, rare meshes might be used often
            self.femmesh = self.mesh_object.FemMesh
        else:
            FreeCAD.Console.PrintWarning(
                "No finite element mesh object was given to the writer class. "
                "In rare cases this might not be an error. "
            )

        # *************************************************
        # deprecated, leave for compatibility reasons
        # if these are calculated here they are calculated twice :-(
        self.femnodes_mesh = {}
        self.femelement_table = {}
        self.constraint_conflict_nodes = []
        self.femnodes_ele_table = {}
        self.femelements_edges_only = []
        self.femelements_faces_only = []
        self.femelement_volumes_table = {}
        self.femelement_faces_table = {}
        self.femelement_edges_table = {}
        self.femelement_count_test = True

        # deprecated, leave for compatibility reasons
        # do not add new objects
        # only the ones which exists on 0.19 release are kept
        # materials
        self.material_objects = member.mats_linear
        self.material_nonlinear_objects = member.mats_nonlinear
        # geometries
        self.beamsection_objects = member.geos_beamsection
        self.beamrotation_objects = member.geos_beamrotation
        self.fluidsection_objects = member.geos_fluidsection
        self.shellthickness_objects = member.geos_shellthickness
        # constraints
        self.contact_objects = member.cons_contact
        self.displacement_objects = member.cons_displacement
        self.fixed_objects = member.cons_fixed
        self.force_objects = member.cons_force
        self.heatflux_objects = member.cons_heatflux
        self.initialtemperature_objects = member.cons_initialtemperature
        self.planerotation_objects = member.cons_planerotation
        self.pressure_objects = member.cons_pressure
        self.selfweight_objects = member.cons_selfweight
        self.temperature_objects = member.cons_temperature
        self.tie_objects = member.cons_tie
        self.transform_objects = member.cons_transform

        # meshdatagetter, for compatibility, same with all getter methods
        self.meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.solver_obj,
            self.mesh_object,
            self.member,
        )

    # ********************************************************************************************
    # ********************************************************************************************
    # generic writer for constraints mesh sets and constraints property data
    # write constraint node sets, constraint face sets, constraint element sets
    def write_constraints_meshsets(
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

    # write constraint property data
    def write_constraints_propdata(
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
    # deprecated, do not add new constraints
    # only the ones which exists on 0.19 release are kept
    def get_constraints_fixed_nodes(self):
        self.meshdatagetter.get_constraints_fixed_nodes()

    def get_constraints_displacement_nodes(self):
        self.meshdatagetter.get_constraints_displacement_nodes()

    def get_constraints_planerotation_nodes(self):
        self.meshdatagetter.get_constraints_planerotation_nodes()

    def get_constraints_transform_nodes(self):
        self.meshdatagetter.get_constraints_transform_nodes()

    def get_constraints_temperature_nodes(self):
        self.meshdatagetter.get_constraints_temperature_nodes()

    def get_constraints_fluidsection_nodes(self):
        self.meshdatagetter.get_constraints_fluidsection_nodes()

    def get_constraints_force_nodeloads(self):
        self.meshdatagetter.get_constraints_force_nodeloads()

    def get_constraints_pressure_faces(self):
        self.meshdatagetter.get_constraints_pressure_faces()

    def get_constraints_contact_faces(self):
        self.meshdatagetter.get_constraints_contact_faces()

    def get_constraints_tie_faces(self):
        self.meshdatagetter.get_constraints_tie_faces()

    def get_constraints_heatflux_faces(self):
        self.meshdatagetter.get_constraints_heatflux_faces()


##  @}
