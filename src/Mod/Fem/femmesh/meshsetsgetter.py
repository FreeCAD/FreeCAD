# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM sets getter"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import time

import FreeCAD

from femmesh import meshtools
from femtools.femutils import type_of_obj


class MeshSetsGetter():
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
    ):
        # class attributes from parameter values
        self.analysis = analysis_obj
        self.solver_obj = solver_obj  # TODO without _obj
        self.mesh_object = mesh_obj  # TODO without _object
        self.member = member

        # more attributes
        self.analysis_type = self.solver_obj.AnalysisType
        self.document = self.analysis.Document
        self.fc_ver = FreeCAD.Version()
        self.ccx_nall = "Nall"
        self.ccx_eall = "Eall"
        self.ccx_evolumes = "Evolumes"
        self.ccx_efaces = "Efaces"
        self.ccx_eedges = "Eedges"
        self.mat_geo_sets = []
        self.theshape = None
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
                # TODO somehow this is not smart, pur mesh objects might be used often
                if (
                    self.member.geos_beamsection
                    and (
                        type_of_obj(self.solver_obj) == "Fem::SolverCcxTools"
                        or type_of_obj(self.solver_obj) == "Fem::SolverCalculix"
                    )
                ):
                    FreeCAD.Console.PrintError(
                        "The mesh does not know the geometry it is made from. "
                        "Beam rotations can not retrieved but they are needed "
                        "for writing CalculiX solver input. "
                        "There might be problems in retrieving mesh data.\n"
                    )
                    # Z88 will run but CalculiX not
            self.femmesh = self.mesh_object.FemMesh
        else:
            FreeCAD.Console.PrintWarning(
                "No finite element mesh object was given to the writer class. "
                "In rare cases this might not be an error.\n"
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
        self.mat_geo_sets = []

    # ********************************************************************************************
    # ********************************************************************************************
    # use set for node sets to be sure all nodes are unique
    # use sorted to be sure the order is the same on different runs
    # be aware a sorted set returns a list, because set are not sorted by default
    #     - done in return value of meshtools.get_femnodes_by_femobj_with_references
    # TODO FIXME might be appropriate for element sets and surfaceface sets too

    # ********************************************************************************************
    # ********************************************************************************************
    # get all known sets
    def get_mesh_sets(self):

        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage(
            "Get mesh data for constraints, materials and element geometry...\n"
        )
        FreeCAD.Console.PrintLog(
            "MeshSetsGetter: Get mesh data for "
            "node sets (groups), surface sets (groups) and element sets (groups)\n"
        )

        time_start = time.process_time()

        # materials and element geometry element sets getter
        self.get_element_sets_material_and_femelement_geometry()

        # constraints element sets getter
        self.get_constraints_centrif_elements()

        # constraints node sets getter
        self.get_constraints_fixed_nodes()
        self.get_constraints_displacement_nodes()
        self.get_constraints_rigidbody_nodes()
        self.get_constraints_planerotation_nodes()

        # constraints surface sets getter
        self.get_constraints_contact_faces()
        self.get_constraints_tie_faces()
        self.get_constraints_sectionprint_faces()
        self.get_constraints_transform_nodes()
        self.get_constraints_temperature_nodes()

        # constraints sets with constraint data
        self.get_constraints_force_nodeloads()
        self.get_constraints_pressure_faces()
        self.get_constraints_heatflux_faces()

        setstime = round((time.process_time() - time_start), 3)
        FreeCAD.Console.PrintMessage(
            "Getting mesh data time: {} seconds.\n".format(setstime)
        )

    # ********************************************************************************************
    # ********************************************************************************************
    # node sets
    def get_constraints_fixed_nodes(self):
        if not self.member.cons_fixed:
            return
        # get nodes
        for femobj in self.member.cons_fixed:
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
        if (
            self.femmesh.Volumes
            and (
                len(self.member.geos_shellthickness) > 0
                or len(self.member.geos_beamsection) > 0
            )
        ):
            FreeCAD.Console.PrintMessage("We need to find the solid nodes.\n")
            if not self.femelement_volumes_table:
                self.femelement_volumes_table = meshtools.get_femelement_volumes_table(
                    self.femmesh
                )
            for femobj in self.member.cons_fixed:
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

    def get_constraints_rigidbody_nodes(self):
        if not self.member.cons_rigidbody:
            return
        # get nodes
        for femobj in self.member.cons_rigidbody:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj["Nodes"]:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_displacement_nodes(self):
        if not self.member.cons_displacement:
            return
        # get nodes
        for femobj in self.member.cons_displacement:
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
        if not self.member.cons_planerotation:
            return
        # get nodes
        for femobj in self.member.cons_planerotation:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_transform_nodes(self):
        if not self.member.cons_transform:
            return
        # get nodes
        for femobj in self.member.cons_transform:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_temperature_nodes(self):
        if not self.member.cons_temperature:
            return
        # get nodes
        for femobj in self.member.cons_temperature:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_fluidsection_nodes(self):
        if not self.member.geos_fluidsection:
            return
        # get nodes
        for femobj in self.member.geos_fluidsection:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            print_obj_info(femobj["Object"])
            femobj["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.femmesh,
                femobj
            )

    def get_constraints_force_nodeloads(self):
        if not self.member.cons_force:
            return
        # check shape type of reference shape
        for femobj in self.member.cons_force:
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
        for femobj in self.member.cons_force:
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
        if not self.member.cons_pressure:
            return
        # TODO see comments in get_constraints_force_nodeloads()
        # it applies here too. Mhh it applies to all constraints ...

        """
        # deprecated version
        # get the faces and face numbers
        for femobj in self.member.cons_pressure:
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

        for femobj in self.member.cons_pressure:
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
        if not self.member.cons_contact:
            return
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                self.femnodes_mesh,
                self.femelement_table
            )

        for femobj in self.member.cons_contact:
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
    # forum post: https://forum.freecad.org/viewtopic.php?f=18&t=42783&p=370286#p366723
    # contact: master and slave could be the same face: rubber of a damper
    # tie: master and slave have to be separate faces AFA UR_ K
    # section print: only the element faces of solid elements
    #                from one side of the geometric face are needed

    def get_constraints_tie_faces(self):
        if not self.member.cons_tie:
            return
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = meshtools.get_femnodes_ele_table(
                self.femnodes_mesh,
                self.femelement_table
            )

        for femobj in self.member.cons_tie:
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
        if not self.member.cons_sectionprint:
            return
        # TODO: use meshtools to get the surfaces
        # see constraint contact or constraint tie
        for femobj in self.member.cons_sectionprint:
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
                            "Wrong reference shape type for {} "
                            "Only Faces are allowed, but a {} was found.\n"
                            .format(sectionprint_obj.Name, ref_shape.ShapeType)
                        )

    def get_constraints_heatflux_faces(self):
        if not self.member.cons_heatflux:
            return
        # TODO: use meshtools to get the surfaces (or move to mesh tools)
        # see constraint contact or constraint tie and constraint force
        # heatflux_obj_face_table: see force_obj_node_load_table
        #     [
        #         ("refshape_name:elemname", face_table),
        #         ...,
        #         ("refshape_name:elemname", face_table)
        #     ]
        for femobj in self.member.cons_heatflux:
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
        if not self.member.cons_centrif:
            return
        if (
            len(self.member.cons_centrif) == 1
            and not self.member.cons_centrif[0]["Object"].References
        ):
            self.member.cons_centrif[0]["FEMElements"] = self.ccx_evolumes
        else:
            self.get_solid_element_sets(self.member.cons_centrif)

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
            self.member.geos_shellthickness
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
            self.member.geos_beamsection
        )

    def get_element_rotation1D_elements(self):
        # get for each geometry edge direction the element ids and rotation norma
        FreeCAD.Console.PrintMessage("Beam rotations\n")
        if self.theshape is None:
            FreeCAD.Console.PrintError(
                "Beam rotations set can not be retrieved, "
                "because the mesh does not know the Geometry it is made from\n"
            )
            return
        if not self.femelement_edges_table:
            self.femelement_edges_table = meshtools.get_femelement_edges_table(
                self.femmesh
            )
        meshtools.get_femelement_direction1D_set(
            self.femmesh,
            self.femelement_edges_table,
            self.member.geos_beamrotation,
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
            self.member.geos_fluidsection
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
            self.get_solid_element_sets(self.member.mats_linear)
        if self.member.geos_shellthickness:
            if not self.femelement_faces_table:
                self.femelement_faces_table = meshtools.get_femelement_faces_table(
                    self.femmesh
                )
            meshtools.get_femelement_sets(
                self.femmesh,
                self.femelement_faces_table,
                self.member.mats_linear
            )
        if self.member.geos_beamsection or self.member.geos_fluidsection:
            if not self.femelement_edges_table:
                self.femelement_edges_table = meshtools.get_femelement_edges_table(
                    self.femmesh
                )
            meshtools.get_femelement_sets(
                self.femmesh,
                self.femelement_edges_table,
                self.member.mats_linear
            )

    def get_element_sets_material_and_femelement_geometry(self):
        if not self.member.mats_linear:
            return
        # in any case if we have beams, we're going to need the element ids for the rotation elsets
        if self.member.geos_beamsection:
            # we will need to split the beam even for one beamobj
            # because no beam in z-direction can be used in ccx without a special adjustment
            # thus they need an own matgeoset
            self.get_element_rotation1D_elements()

        # get the element ids for face and edge elements and write them into the objects
        if len(self.member.geos_shellthickness) > 1:
            self.get_element_geometry2D_elements()
        if len(self.member.geos_beamsection) > 1:
            self.get_element_geometry1D_elements()
        if len(self.member.geos_fluidsection) > 1:
            self.get_element_fluid1D_elements()

        # get the element ids for material objects and write them into the material object
        if len(self.member.mats_linear) > 1:
            self.get_material_elements()

        # create the mat_geo_sets
        if len(self.member.mats_linear) == 1:
            if self.femmesh.Volumes:
                # we only could do this for volumes, if a mesh contains volumes
                # we're going to use them in the analysis
                # but a mesh could contain the element faces of the volumes as faces
                # and the edges of the faces as edges
                # there we have to check for some geometric objects
                self.get_mat_geo_sets_single_mat_solid()
            if len(self.member.geos_shellthickness) == 1:
                self.get_mat_geo_sets_single_mat_single_shell()
            elif len(self.member.geos_shellthickness) > 1:
                self.get_mat_geo_sets_single_mat_multiple_shell()
            if len(self.member.geos_beamsection) == 1:
                self.get_mat_geo_sets_single_mat_single_beam()
            elif len(self.member.geos_beamsection) > 1:
                self.get_mat_geo_sets_single_mat_multiple_beam()
            if len(self.member.geos_fluidsection) == 1:
                self.get_mat_geo_sets_single_mat_single_fluid()
            elif len(self.member.geos_fluidsection) > 1:
                self.get_mat_geo_sets_single_mat_multiple_fluid()
        elif len(self.member.mats_linear) > 1:
            if self.femmesh.Volumes:
                # we only could do this for volumes, if a mseh contains volumes
                # we're going to use them in the analysis
                # but a mesh could contain the element faces of the volumes as faces
                # and the edges of the faces as edges
                # there we have to check for some geometric objects
                # volume is a bit special
                # because retrieving ids from group mesh data is implemented
                self.get_mat_geo_sets_multiple_mat_solid()
            if len(self.member.geos_shellthickness) == 1:
                self.get_mat_geo_sets_multiple_mat_single_shell()
            elif len(self.member.geos_shellthickness) > 1:
                self.get_mat_geo_sets_multiple_mat_multiple_shell()
            if len(self.member.geos_beamsection) == 1:
                self.get_mat_geo_sets_multiple_mat_single_beam()
            elif len(self.member.geos_beamsection) > 1:
                self.get_mat_geo_sets_multiple_mat_multiple_beam()
            if len(self.member.geos_fluidsection) == 1:
                self.get_mat_geo_sets_multiple_mat_single_fluid()
            elif len(self.member.geos_fluidsection) > 1:
                self.get_mat_geo_sets_multiple_mat_multiple_fluid()

    # self.mat_geo_sets = [ {
    #                        "ccx_elset" : [e1, e2, e3, ... , en] or elements set name strings
    #                        "ccx_elset_name" : "ccx_identifier_elset"
    #                        "mat_obj_name" : "mat_obj.Name"
    #                        "ccx_mat_name" : "mat_obj.Material["Name"]"   !!! not unique !!!
    #                        "beamsection_obj" : "beamsection_obj"         if exists
    #                        "fluidsection_obj" : "fluidsection_obj"       if exists
    #                        "shellthickness_obj" : shellthickness_obj"    if exists
    #                        "beam_axis_m" : main local beam axis          for beams only
    #                     },
    #                     {}, ... , {} ]

    # beam
    # TODO support multiple beamrotations
    # we do not need any more any data from the rotation document object,
    # thus we do not need to save the rotation document object name in the else
    def get_mat_geo_sets_single_mat_single_beam(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        beamsec_obj = self.member.geos_beamsection[0]["Object"]
        beamrot_data = self.member.geos_beamrotation[0]
        for i, beamdirection in enumerate(beamrot_data["FEMRotations1D"]):
            # ID's for this direction
            elset_data = beamdirection["ids"]
            names = [
                {"short": "M0"},
                {"short": "B0"},
                {"short": beamrot_data["ShortName"]},
                {"short": "D" + str(i)}
            ]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_short(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            matgeoset["beamsection_obj"] = beamsec_obj
            # beam_axis_m for this direction
            matgeoset["beam_axis_m"] = beamdirection["beam_axis_m"]
            self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_single_mat_multiple_beam(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        beamrot_data = self.member.geos_beamrotation[0]
        for beamsec_data in self.member.geos_beamsection:
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
                    matgeoset = {}
                    matgeoset["ccx_elset"] = elset_data
                    matgeoset["ccx_elset_name"] = get_elset_name_short(names)
                    matgeoset["mat_obj_name"] = mat_obj.Name
                    matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
                    matgeoset["beamsection_obj"] = beamsec_obj
                    # beam_axis_m for this direction
                    matgeoset["beam_axis_m"] = beamdirection["beam_axis_m"]
                    self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_single_beam(self):
        beamsec_obj = self.member.geos_beamsection[0]["Object"]
        beamrot_data = self.member.geos_beamrotation[0]
        for mat_data in self.member.mats_linear:
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
                    matgeoset = {}
                    matgeoset["ccx_elset"] = elset_data
                    matgeoset["ccx_elset_name"] = get_elset_name_short(names)
                    matgeoset["mat_obj_name"] = mat_obj.Name
                    matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
                    matgeoset["beamsection_obj"] = beamsec_obj
                    # beam_axis_m for this direction
                    matgeoset["beam_axis_m"] = beamdirection["beam_axis_m"]
                    self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_multiple_beam(self):
        beamrot_data = self.member.geos_beamrotation[0]
        for beamsec_data in self.member.geos_beamsection:
            beamsec_obj = beamsec_data["Object"]
            beamsec_ids = set(beamsec_data["FEMElements"])
            for mat_data in self.member.mats_linear:
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
                        matgeoset = {}
                        matgeoset["ccx_elset"] = elset_data
                        matgeoset["ccx_elset_name"] = get_elset_name_short(names)
                        matgeoset["mat_obj_name"] = mat_obj.Name
                        matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
                        matgeoset["beamsection_obj"] = beamsec_obj
                        # beam_axis_m for this direction
                        matgeoset["beam_axis_m"] = beamdirection["beam_axis_m"]
                        self.mat_geo_sets.append(matgeoset)

    # fluid
    def get_mat_geo_sets_single_mat_single_fluid(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        fluidsec_obj = self.member.geos_fluidsection[0]["Object"]
        elset_data = self.ccx_eedges
        names = [{"short": "M0"}, {"short": "F0"}]
        matgeoset = {}
        matgeoset["ccx_elset"] = elset_data
        matgeoset["ccx_elset_name"] = get_elset_name_short(names)
        matgeoset["mat_obj_name"] = mat_obj.Name
        matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
        matgeoset["fluidsection_obj"] = fluidsec_obj
        self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_single_mat_multiple_fluid(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        for fluidsec_data in self.member.geos_fluidsection:
            fluidsec_obj = fluidsec_data["Object"]
            elset_data = fluidsec_data["FEMElements"]
            names = [{"short": "M0"}, {"short": fluidsec_data["ShortName"]}]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_short(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            matgeoset["fluidsection_obj"] = fluidsec_obj
            self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_single_fluid(self):
        fluidsec_obj = self.member.geos_fluidsection[0]["Object"]
        for mat_data in self.member.mats_linear:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [{"short": mat_data["ShortName"]}, {"short": "F0"}]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_short(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            matgeoset["fluidsection_obj"] = fluidsec_obj
            self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_multiple_fluid(self):
        for fluidsec_data in self.member.geos_fluidsection:
            fluidsec_obj = fluidsec_data["Object"]
            for mat_data in self.member.mats_linear:
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
                    matgeoset = {}
                    matgeoset["ccx_elset"] = elset_data
                    matgeoset["ccx_elset_name"] = get_elset_name_short(names)
                    matgeoset["mat_obj_name"] = mat_obj.Name
                    matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
                    matgeoset["fluidsection_obj"] = fluidsec_obj
                    self.mat_geo_sets.append(matgeoset)

    # shell
    def get_mat_geo_sets_single_mat_single_shell(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        shellth_obj = self.member.geos_shellthickness[0]["Object"]
        elset_data = self.ccx_efaces
        names = [
            {"long": mat_obj.Name, "short": "M0"},
            {"long": shellth_obj.Name, "short": "S0"}
        ]
        matgeoset = {}
        matgeoset["ccx_elset"] = elset_data
        matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
        matgeoset["mat_obj_name"] = mat_obj.Name
        matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
        matgeoset["shellthickness_obj"] = shellth_obj
        self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_single_mat_multiple_shell(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        for shellth_data in self.member.geos_shellthickness:
            shellth_obj = shellth_data["Object"]
            elset_data = shellth_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": "M0"},
                {"long": shellth_obj.Name, "short": shellth_data["ShortName"]}
            ]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            matgeoset["shellthickness_obj"] = shellth_obj
            self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_single_shell(self):
        shellth_obj = self.member.geos_shellthickness[0]["Object"]
        for mat_data in self.member.mats_linear:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": mat_data["ShortName"]},
                {"long": shellth_obj.Name, "short": "S0"}
            ]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            matgeoset["shellthickness_obj"] = shellth_obj
            self.mat_geo_sets.append(matgeoset)

    def get_mat_geo_sets_multiple_mat_multiple_shell(self):
        for shellth_data in self.member.geos_shellthickness:
            shellth_obj = shellth_data["Object"]
            for mat_data in self.member.mats_linear:
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
                    matgeoset = {}
                    matgeoset["ccx_elset"] = elset_data
                    matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
                    matgeoset["mat_obj_name"] = mat_obj.Name
                    matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
                    matgeoset["shellthickness_obj"] = shellth_obj
                    self.mat_geo_sets.append(matgeoset)

    # solid
    def get_mat_geo_sets_single_mat_solid(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        elset_data = self.ccx_evolumes
        names = [
            {"long": mat_obj.Name, "short": "M0"},
            {"long": "Solid", "short": "Solid"}
        ]
        matgeoset = {}
        matgeoset["ccx_elset"] = elset_data
        matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
        matgeoset["mat_obj_name"] = mat_obj.Name
        matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
        self.mat_geo_sets.append(matgeoset)
        print(self.mat_geo_sets)

    def get_mat_geo_sets_multiple_mat_solid(self):
        for mat_data in self.member.mats_linear:
            mat_obj = mat_data["Object"]
            elset_data = mat_data["FEMElements"]
            names = [
                {"long": mat_obj.Name, "short": mat_data["ShortName"]},
                {"long": "Solid", "short": "Solid"}
            ]
            matgeoset = {}
            matgeoset["ccx_elset"] = elset_data
            matgeoset["ccx_elset_name"] = get_elset_name_standard(names)
            matgeoset["mat_obj_name"] = mat_obj.Name
            matgeoset["ccx_mat_name"] = mat_obj.Material["Name"]
            self.mat_geo_sets.append(matgeoset)


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


def get_elset_name_standard(names):
    # standard max length = 80
    elset_name = ""
    for name in names:
        elset_name += name["long"]
    if len(elset_name) < 81:
        return elset_name
    else:
        elset_name = ""
        for name in names:
            elset_name += name["short"]
        if len(elset_name) < 81:
            return elset_name
        else:
            error = (
                "FEM: Trouble in elset name, because an "
                "elset name is longer than 80 character! {}\n"
                .format(elset_name)
            )
            raise Exception(error)


def get_elset_name_short(names):
    # restricted max length = 20 (elsets)
    # in CalculiX solver input this is needed for beam elsets
    elset_name = ""
    for name in names:
        elset_name += name["short"]
    if len(elset_name) < 21:
        return elset_name
    else:
        error = (
            "FEM: Trouble in elset name, because an"
            "short elset name is longer than 20 characters! {}\n"
            .format(elset_name)
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
