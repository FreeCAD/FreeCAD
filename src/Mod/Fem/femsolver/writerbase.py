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

import FreeCAD

from femmesh import meshtools
from femtools.femutils import type_of_obj


class FemInputWriter():
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None
    ):
        # class attributes from parameter values
        self.analysis = analysis_obj
        self.solver_obj = solver_obj
        self.analysis_type = self.solver_obj.AnalysisType
        self.mesh_object = mesh_obj
        self.document = self.analysis.Document
        # materials
        self.material_objects = member.mats_linear
        self.material_nonlinear_objects = member.mats_nonlinear
        # geometries
        self.beamsection_objects = member.geos_beamsection
        self.beamrotation_objects = member.geos_beamrotation
        self.fluidsection_objects = member.geos_fluidsection
        self.shellthickness_objects = member.geos_shellthickness
        # constraints
        self.centrif_objects = member.cons_centrif
        self.contact_objects = member.cons_contact
        self.displacement_objects = member.cons_displacement
        self.fixed_objects = member.cons_fixed
        self.force_objects = member.cons_force
        self.heatflux_objects = member.cons_heatflux
        self.initialtemperature_objects = member.cons_initialtemperature
        self.planerotation_objects = member.cons_planerotation
        self.pressure_objects = member.cons_pressure
        self.sectionprint_objects = member.cons_sectionprint
        self.selfweight_objects = member.cons_selfweight
        self.temperature_objects = member.cons_temperature
        self.tie_objects = member.cons_tie
        self.transform_objects = member.cons_transform
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
        self.ccx_elsets = []
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

    # ********************************************************************************************
    # ********************************************************************************************
    # use set for node sets to be sure all nodes are unique
    # use sorted to be sure the order is the same on different runs
    # be aware a sorted set returns a list, because set are not sorted by default
    #     - done in return value of meshtools.get_femnodes_by_femobj_with_references
    # TODO FIXME might be appropriate for element sets too

    # ********************************************************************************************
    # ********************************************************************************************
    # node sets
    def get_constraints_fixed_nodes(self):
        # get nodes
        for femobj in self.fixed_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj["Nodes"]:
                self.constraint_conflict_nodes.append(node)
        # if mixed mesh with solids the node set needs to be split
        # because solid nodes do not have rotational degree of freedom
        if self.femmesh.Volumes \
                and (len(self.shellthickness_objects) > 0 or len(self.beamsection_objects) > 0):
            FreeCAD.Console.PrintMessage("We need to find the solid nodes.\n")
            if not self.femelement_volumes_table:
                self.femelement_volumes_table = meshtools.get_femelement_volumes_table(
                    self.femmesh
                )
            for femobj in self.fixed_objects:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                nds_solid = []
                nds_faceedge = []
                for n in femobj["Nodes"]:
                    solid_node = False
                    for ve in self.femelement_volumes_table:
                        if n in self.femelement_volumes_table[ve]:
                            solid_node = True
                            nds_solid.append(n)
                            break
                    if not solid_node:
                        nds_faceedge.append(n)
                femobj["NodesSolid"] = set(nds_solid)
                femobj["NodesFaceEdge"] = set(nds_faceedge)

    def get_constraints_displacement_nodes(self):
        # get nodes
        for femobj in self.displacement_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj["Nodes"]:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_planerotation_nodes(self):
        # get nodes
        for femobj in self.planerotation_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_transform_nodes(self):
        # get nodes
        for femobj in self.transform_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_temperature_nodes(self):
        # get nodes
        for femobj in self.temperature_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_fluidsection_nodes(self):
        # get nodes
        for femobj in self.fluidsection_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_force_nodeloads(self):
        # check shape type of reference shape
        for femobj in self.force_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"], log=True)
            if femobj["RefShapeType"] == "Vertex":
                FreeCAD.Console.PrintLog(
                    "    load on vertices --> The femelement_table "
                    "and femnodes_mesh are not needed for node load calculation.\n"
                )
            elif femobj["RefShapeType"] == "Face" \
                    and meshtools.is_solid_femmesh(self.femmesh) \
                    and not meshtools.has_no_face_data(self.femmesh):
                FreeCAD.Console.PrintLog(
                    "    solid_mesh with face data --> The femelement_table is not "
                    "needed but the femnodes_mesh is needed for node load calculation.\n"
                )
                if not self.femnodes_mesh:
                    self.femnodes_mesh = self.femmesh.Nodes
            else:
                FreeCAD.Console.PrintLog(
                    "    mesh without needed data --> The femelement_table "
                    "and femnodes_mesh are not needed for node load calculation.\n"
                )
                if not self.femnodes_mesh:
                    self.femnodes_mesh = self.femmesh.Nodes
                if not self.femelement_table:
                    self.femelement_table = meshtools.get_femelement_table(
                        self.femmesh
                    )
        # get node loads
        FreeCAD.Console.PrintLog(
            "    Finite element mesh nodes will be retrieved by searching "
            "the appropriate nodes in the finite element mesh.\n"
        )
        FreeCAD.Console.PrintLog(
            "    The appropriate finite element mesh node load values will "
            "be calculated according to the finite element definition.\n"
        )
        for femobj in self.force_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            frc_obj = femobj["Object"]
            print_obj_info(frc_obj)
            if frc_obj.Force == 0:
                FreeCAD.Console.PrintMessage("  Warning --> Force = 0\n")
            if femobj["RefShapeType"] == "Vertex":  # point load on vertices
                femobj["NodeLoadTable"] = meshtools.get_force_obj_vertex_nodeload_table(
                    self.femmesh,
                    frc_obj
                )
            elif femobj["RefShapeType"] == "Edge":  # line load on edges
                femobj["NodeLoadTable"] = meshtools.get_force_obj_edge_nodeload_table(
                    self.femmesh,
                    self.femelement_table,
                    self.femnodes_mesh, frc_obj
                )
            elif femobj["RefShapeType"] == "Face":  # area load on faces
                femobj["NodeLoadTable"] = meshtools.get_force_obj_face_nodeload_table(
                    self.femmesh,
                    self.femelement_table,
                    self.femnodes_mesh, frc_obj
                )

    # ********************************************************************************************
    # ********************************************************************************************
    # faces sets
    def get_constraints_pressure_faces(self):
        # TODO see comments in get_constraints_force_nodeloads()
        # it applies here too. Mhh it applies to all constraints ...

        """
        # deprecated version
        # get the faces and face numbers
        for femobj in self.pressure_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            femobj["PressureFaces"] = meshtools.get_pressure_obj_faces_depreciated(
                self.femmesh,
                femobj
            )
            # print(femobj["PressureFaces"])
        """

        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                self.femnodes_mesh,
                self.femelement_table
            )

        for femobj in self.pressure_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            pressure_faces = meshtools.get_pressure_obj_faces(
                self.femmesh,
                self.femelement_table,
                self.femnodes_ele_table, femobj
            )
            # the data model is for compatibility reason with deprecated version
            # get_pressure_obj_faces_depreciated returns the face ids in a tuple per ref_shape
            # some_string was the reference_shape_element_string in deprecated method
            # [(some_string, [ele_id, ele_face_id], [ele_id, ele_face_id], ...])]
            some_string = "{}: face load".format(femobj["Object"].Name)
            femobj["PressureFaces"] = [(some_string, pressure_faces)]
            FreeCAD.Console.PrintLog("{}\n".format(femobj["PressureFaces"]))

    def get_constraints_contact_faces(self):
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                self.femnodes_mesh,
                self.femelement_table
            )

        for femobj in self.contact_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            contact_slave_faces, contact_master_faces = meshtools.get_contact_obj_faces(
                self.femmesh,
                self.femelement_table,
                self.femnodes_ele_table, femobj
            )
            # [ele_id, ele_face_id], [ele_id, ele_face_id], ...]
            # whereas the ele_face_id might be ccx specific
            femobj["ContactSlaveFaces"] = contact_slave_faces
            femobj["ContactMasterFaces"] = contact_master_faces
            # FreeCAD.Console.PrintLog("{}\n".format(femobj["ContactSlaveFaces"]))
            # FreeCAD.Console.PrintLog("{}\n".format(femobj["ContactMasterFaces"]))

    # information in the regard of element faces constraints
    # forum post: https://forum.freecadweb.org/viewtopic.php?f=18&t=42783&p=370286#p366723
    # contact: master and slave could be the same face: rubber of a damper
    # tie: master and slave have to be separate faces AFA UR_ K
    # section print: only the element faces of solid elements
    #                from one side of the geometric face are needed

    def get_constraints_tie_faces(self):
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                self.femnodes_mesh,
                self.femelement_table
            )

        for femobj in self.tie_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            slave_faces, master_faces = meshtools.get_tie_obj_faces(
                self.femmesh,
                self.femelement_table,
                self.femnodes_ele_table, femobj
            )
            # [ele_id, ele_face_id], [ele_id, ele_face_id], ...]
            # whereas the ele_face_id might be ccx specific
            femobj["TieSlaveFaces"] = slave_faces
            femobj["TieMasterFaces"] = master_faces
            # FreeCAD.Console.PrintLog("{}\n".format(femobj["ContactSlaveFaces"]))
            # FreeCAD.Console.PrintLog("{}\n".format(femobj["ContactMasterFaces"]))

    def get_constraints_sectionprint_faces(self):
        # TODO: use meshtools to get the surfaces
        # see constraint contact or constrint tie
        for femobj in self.sectionprint_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            sectionprint_obj = femobj["Object"]
            if len(sectionprint_obj.References) > 1:
                FreeCAD.Console.PrintError(
                    "Only one reference shape allowed for a section print "
                    "but {} found: {}\n"
                    .format(len(sectionprint_obj.References), sectionprint_obj.References)
                )
            for o, elem_tup in sectionprint_obj.References:
                for elem in elem_tup:
                    # there should only be one reference for each section print object
                    # in the gui this is checked
                    ref_shape = o.Shape.getElement(elem)
                    if ref_shape.ShapeType == "Face":
                        v = self.mesh_object.FemMesh.getccxVolumesByFace(ref_shape)
                        if len(v) > 0:
                            femobj["SectionPrintFaces"] = v
                            # volume elements found
                            FreeCAD.Console.PrintLog(
                                "{}, surface {}, {} touching volume elements found\n"
                                .format(sectionprint_obj.Label, sectionprint_obj.Name, len(v))
                            )
                        else:
                            # no volume elements found, shell elements not allowed
                            FreeCAD.Console.PrintError(
                                "{}, surface {}, Error: "
                                "No volume elements found!\n"
                                .format(sectionprint_obj.Label, sectionprint_obj.Name)
                            )
                    else:
                        # in Gui only Faces can be added
                        FreeCAD.Console.PrintError(
                            "Wrong reference shapt type for {} "
                            "Only Faces are allowed, but a {} was found.\n"
                            .format(sectionprint_obj.Name, ref_shape.ShapeType)
                        )

    def get_constraints_heatflux_faces(self):
        # TODO: use meshtools to get the surfaces (or move to mesh tools)
        # see constraint contact or constrint tie and constraint force
        # heatflux_obj_face_table: see force_obj_node_load_table
        #     [
        #         ("refshape_name:elemname", face_table),
        #         ...,
        #         ("refshape_name:elemname", face_table)
        #     ]
        for femobj in self.heatflux_objects:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            heatflux_obj = femobj["Object"]
            femobj["HeatFluxFaceTable"] = []
            for o, elem_tup in heatflux_obj.References:
                for elem in elem_tup:
                    ho = o.Shape.getElement(elem)
                    if ho.ShapeType == "Face":
                        elem_info = "{}:{}".format(o.Name, elem)
                        face_table = self.mesh_object.FemMesh.getccxVolumesByFace(ho)
                        femobj["HeatFluxFaceTable"].append((elem_info, face_table))

    # ********************************************************************************************
    # ********************************************************************************************
    # element sets constraints
    def get_constraints_centrif_elements(self):
        # get element ids and write them into the femobj
        if len(self.centrif_objects) == 0:
            return
        elif len(self.centrif_objects) == 1 and not self.centrif_objects[0]["Object"].References:
            self.centrif_objects[0]["FEMElements"] = self.ccx_evolumes
        else:
            self.get_solid_element_sets(self.centrif_objects)

    # ********************************************************************************************
    # ********************************************************************************************
    # element sets material and element geometry
    def get_solid_element_sets(self, femobjs):
        # get element ids and write them into the femobj
        all_found = False
        if self.femmesh.GroupCount:
            all_found = meshtools.get_femelement_sets_from_group_data(
                self.femmesh,
                femobjs
            )
            FreeCAD.Console.PrintMessage(all_found)
            FreeCAD.Console.PrintMessage("\n")
        if all_found is False:
            if not self.femelement_table:
                self.femelement_table = meshtools.get_femelement_table(self.femmesh)
            # we're going to use the binary search for get_femelements_by_femnodes()
            # thus we need the parameter values self.femnodes_ele_table
            if not self.femnodes_mesh:
                self.femnodes_mesh = self.femmesh.Nodes
            if not self.femnodes_ele_table:
                self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                    self.femnodes_mesh,
                    self.femelement_table
                )
            control = meshtools.get_femelement_sets(
                self.femmesh,
                self.femelement_table,
                femobjs,
                self.femnodes_ele_table
            )
            # we only need to set it, if it is still True
            if (self.femelement_count_test is True) and (control is False):
                self.femelement_count_test = False

    def get_element_geometry2D_elements(self):
        # get element ids and write them into the objects
        FreeCAD.Console.PrintMessage("Shell thicknesses\n")
        if not self.femelement_faces_table:
            self.femelement_faces_table = meshtools.get_femelement_faces_table(
                self.femmesh
            )
        meshtools.get_femelement_sets(
            self.femmesh,
            self.femelement_faces_table,
            self.shellthickness_objects
        )

    def get_element_geometry1D_elements(self):
        # get element ids and write them into the objects
        FreeCAD.Console.PrintMessage("Beam sections\n")
        if not self.femelement_edges_table:
            self.femelement_edges_table = meshtools.get_femelement_edges_table(
                self.femmesh
            )
        meshtools.get_femelement_sets(
            self.femmesh,
            self.femelement_edges_table,
            self.beamsection_objects
        )

    def get_element_rotation1D_elements(self):
        # get for each geometry edge direction the element ids and rotation norma
        FreeCAD.Console.PrintMessage("Beam rotations\n")
        if not self.femelement_edges_table:
            self.femelement_edges_table = meshtools.get_femelement_edges_table(
                self.femmesh
            )
        meshtools.get_femelement_direction1D_set(
            self.femmesh,
            self.femelement_edges_table,
            self.beamrotation_objects,
            self.theshape
        )

    def get_element_fluid1D_elements(self):
        # get element ids and write them into the objects
        FreeCAD.Console.PrintMessage("Fluid sections\n")
        if not self.femelement_edges_table:
            self.femelement_edges_table = meshtools.get_femelement_edges_table(
                self.femmesh
            )
        meshtools.get_femelement_sets(
            self.femmesh,
            self.femelement_edges_table,
            self.fluidsection_objects
        )

    def get_material_elements(self):
        # it only works if either Volumes or Shellthicknesses or Beamsections
        # are in the material objects, it means it does not work
        # for mixed meshes and multiple materials, this is checked in check_prerequisites
        # the femelement_table is only calculated for
        # the highest dimension in get_femelement_table
        FreeCAD.Console.PrintMessage("Materials\n")
        if self.femmesh.Volumes:
            # we only could do this for volumes
            # if a mesh contains volumes we're going to use them in the analysis
            # but a mesh could contain
            # the element faces of the volumes as faces
            # and the edges of the faces as edges
            # there we have to check of some geometric objects
            # get element ids and write them into the femobj
            self.get_solid_element_sets(self.material_objects)
        if self.shellthickness_objects:
            if not self.femelement_faces_table:
                self.femelement_faces_table = meshtools.get_femelement_faces_table(
                    self.femmesh
                )
            meshtools.get_femelement_sets(
                self.femmesh,
                self.femelement_faces_table,
                self.material_objects
            )
        if self.beamsection_objects or self.fluidsection_objects:
            if not self.femelement_edges_table:
                self.femelement_edges_table = meshtools.get_femelement_edges_table(
                    self.femmesh
                )
            meshtools.get_femelement_sets(
                self.femmesh,
                self.femelement_edges_table,
                self.material_objects
            )

    def get_element_sets_material_and_femelement_geometry(self):
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


# ************************************************************************************************
# Helpers


# ccx elset names:
# M .. Material
# B .. Beam
# R .. BeamRotation
# D .. Direction
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


def print_obj_info(obj, log=False):
    if log is False:
        FreeCAD.Console.PrintMessage("{}:\n".format(obj.Label))
        FreeCAD.Console.PrintMessage(
            "    Type: {}, Name: {}\n".format(type_of_obj(obj), obj.Name)
        )
    else:
        FreeCAD.Console.PrintLog("{}:\n".format(obj.Label))
        FreeCAD.Console.PrintLog(
            "    Type: {}, Name: {}\n".format(type_of_obj(obj), obj.Name)
        )

##  @}
