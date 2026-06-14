# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM solver Z88 writer"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import numpy as np
import time
import os

import FreeCAD

from .. import writerbase
from femmesh import meshtools

from .write_constraint_displacement import WriterDisplacement
from .write_constraint_fixed import WriterFixed
from .write_constraint_force import WriterForce
from .write_constraint_pressure import WriterPressure
from .write_sectionprint import WriterSectionPrint
from .write_element1D import WriterElement1D
from .write_element2D import WriterElement2D
from .write_material import WriterMaterial
from . import z88utils


class FemInputWriterZ88(writerbase.FemInputWriter):
    def __init__(self, analysis_obj, solver_obj, mesh_obj, member, dir_name=None):
        writerbase.FemInputWriter.__init__(
            self, analysis_obj, solver_obj, mesh_obj, member, dir_name
        )

    # ********************************************************************************************
    # write solver input
    def write_solver_input(self):
        timestart = time.process_time()

        FreeCAD.Console.PrintMessage("\nZ88 solver input writing...\n")
        FreeCAD.Console.PrintMessage(
            f"Write z88 input files to: {self.solver_obj.WorkingDirectory}\n"
        )

        self.write_z88_mesh()
        # write first materials and elements. They may be used by constraints and loads.
        self.write_z88_materials()
        self.write_z88_elements_properties()
        self.write_z88_constraints()
        self.write_z88_face_loads()
        self.write_z88_section_prints()
        self.write_z88_integration_properties()
        self.write_z88_memory_parameter()
        self.write_z88_solver_parameter()

        writing_time_string = "Writing time input file: {} seconds".format(
            round((time.process_time() - timestart), 2)
        )
        FreeCAD.Console.PrintMessage(f"{writing_time_string}\n\n")

    # ********************************************************************************************
    def write_z88_constraints(self):
        constraints_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88i2.txt")
        self.z88i2 = open(constraints_file_path, "w")
        self.z88i2_rows = []

        # displacement constraints
        w_disp = WriterDisplacement(self)
        w_disp.write_items()

        # fixed constraints
        w_fixed = WriterFixed(self)
        w_fixed.write_items()

        # force constraints
        w_force = WriterForce(self)
        w_force.write_items()

        self.z88i2.write(f"{len(self.z88i2_rows)}\n")
        self.z88i2.writelines(self.z88i2_rows)

        self.z88i2.close()

    # ********************************************************************************************
    def write_z88_face_loads(self):
        face_load_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88i5.txt")
        self.z88i5 = open(face_load_file_path, "w")
        self.z88i5_rows = []

        # pressure constraints
        w_pressure = WriterPressure(self)
        w_pressure.write_items()

        self.z88i5.write(f"{len(self.z88i5_rows)}\n")
        self.z88i5.writelines(self.z88i5_rows)

        self.z88i5.close()

    # ********************************************************************************************
    def write_z88_section_prints(self):
        section_print_file_path = os.path.join(
            self.solver_obj.WorkingDirectory, "z88section_print.npy"
        )
        # remove old section print data
        if os.path.exists(section_print_file_path):
            os.remove(section_print_file_path)

        # collection of index in nodes array per section print object
        self.z88section_print_dict = {}

        # section print
        w_section_print = WriterSectionPrint(self)
        w_section_print.write_items()
        if self.z88section_print_dict:
            np.save(section_print_file_path, self.z88section_print_dict)

    # ********************************************************************************************
    def write_z88_materials(self):
        material_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88mat.txt")
        self.z88mat = open(material_file_path, "w")
        self.z88mat_rows = []

        # material constraints
        w_material = WriterMaterial(self)
        w_material.write_items()

        self.z88mat.write(f"{len(self.z88mat_rows)}\n")
        self.z88mat.writelines(map(lambda x: "{} {} {}\n".format(*x), self.z88mat_rows))

        self.z88mat.close()

    # ********************************************************************************************
    def write_z88_elements_properties(self):
        element_properties_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88elp.txt")
        self.z88elp = open(element_properties_file_path, "w")
        self.z88elp_rows = []

        # elements parameters
        w_element1D = WriterElement1D(self)
        w_element1D.write_items()
        w_element2D = WriterElement2D(self)
        w_element2D.write_items()

        self.z88elp.write(f"{len(self.z88elp_rows)}\n")
        self.z88elp.writelines(map(lambda x: "{} {} {}\n".format(*x), self.z88elp_rows))

        self.z88elp.close()

    # ********************************************************************************************
    def write_z88_integration_properties(self):
        integration_properties_file_path = os.path.join(
            self.solver_obj.WorkingDirectory, "z88int.txt"
        )
        self.z88int = open(integration_properties_file_path, "w")
        self.z88int_rows = []
        el_types = np.unique(self.elements["type"])

        # at the moment, stress integration order is 0
        param = "0 0"
        for t in el_types:
            idx = np.arange(1, len(self.elements) + 1)[self.elements["type"] == t]
            if t in [16, 17]:
                # tetra elements
                param = f"{self.solver_obj.IntegrationOrderTetra} 0"
            elif t in [14, 15, 18, 24]:
                # tria elements
                param = f"{self.solver_obj.IntegrationOrderTria} 0"
            elif t in [7, 8, 20, 23]:
                # quad elements
                param = f"{self.solver_obj.IntegrationOrderQuad} 0"
            elif t in [1, 7, 8, 10, 20, 23]:
                # hexa elements
                param = f"{self.solver_obj.IntegrationOrderHexa} 0"

            # write integration orders as z88 ranges
            first = idx[0]
            start_end = [first, first]
            for el_index in idx[1:]:
                if el_index - start_end[1] == 1:
                    start_end[1] = el_index
                else:
                    self.z88int_rows.append(f"{start_end[0]} {start_end[1]} {param}\n")
                    start_end = [el_index, el_index]
            # add last computed range
            self.z88int_rows.append(f"{start_end[0]} {start_end[1]} {param}\n")

        self.z88int.write(f"{len(self.z88int_rows)}\n")
        self.z88int.writelines(self.z88int_rows)

        self.z88int.close()

    # ********************************************************************************************
    def write_z88_solver_parameter(self):
        el_types = np.unique(self.elements["type"])
        # check beams in structure
        ibflag = 1 if any([i in el_types for i in [2, 13, 25]]) else 0
        # check plates in structure
        ipflag = 1 if any([i in el_types for i in [18, 20]]) else 0
        # check shells in structure
        ihflag = self.solver_obj.ShellFlag if any([i in el_types for i in [23, 24]]) else 0
        maxit = self.solver_obj.IterationMaximum
        eps = self.solver_obj.ResidualLimit
        ralpha = self.solver_obj.ShiftFactor
        romega = self.solver_obj.RelaxationFactor
        kdflag = 0
        isflag = 0

        z88_man_template = z88utils.z88_man_template.format(
            param_ibflag=ibflag,
            param_ipflag=ipflag,
            param_ihflag=ihflag,
            param_maxit=maxit,
            param_eps=eps,
            param_ralpha=ralpha,
            param_romega=romega,
            param_kdflag=kdflag,
            param_isflag=isflag,
        )

        solver_parameter_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88man.txt")
        self.z88man = open(solver_parameter_file_path, "w")
        self.z88man.write(z88_man_template)
        self.z88man.close()

    # ********************************************************************************************
    def write_z88_memory_parameter(self):
        maxgs = self.solver_obj.MatrixMaximum
        maxkoi = self.solver_obj.VectorMaximum
        z88_dyn_template = z88utils.z88_dyn_template.format(
            param_maxgs=maxgs,
            param_maxkoi=maxkoi,
        )

        solver_parameter_file_path = os.path.join(self.solver_obj.WorkingDirectory, "z88.dyn")
        self.z88dyn = open(solver_parameter_file_path, "w")
        self.z88dyn.write(z88_dyn_template)
        self.z88dyn.close()

    def write_z88_mesh(self):
        mesh = self.mesh_object.FemMesh
        mesh_nodes = dict(sorted(mesh.Nodes.items()))

        # initialize some variables
        nodes_dof_map = {}
        mesh_elem = mesh_nodes.keys()
        e_count = len(mesh_elem)
        smesh_type_from_nodes = {1: 1}
        max_elem_nodes = 1

        if meshtools.is_solid_femmesh(mesh):
            e_count = mesh.VolumeCount
            mesh_elem = mesh.Volumes
            smesh_type_from_nodes = z88utils.smesh_volume_type_from_nodes
            max_elem_nodes = 20
        elif meshtools.is_face_femmesh(mesh):
            e_count = mesh.FaceCount
            smesh_type_from_nodes = z88utils.smesh_face_type_from_nodes
            mesh_elem = mesh.Faces
            max_elem_nodes = 8
        elif meshtools.is_edge_femmesh(mesh):
            e_count = mesh.EdgeCount
            smesh_type_from_nodes = z88utils.smesh_edge_type_from_nodes
            mesh_elem = mesh.Edges
            max_elem_nodes = 3

        # get nodes from used elements
        nodes_in_use = set()
        for e in mesh_elem:
            nodes_in_use.update(mesh.getElementNodes(e))
        nodes_in_use = sorted(nodes_in_use)

        # save map node key -> array order
        # use masked array for nodes.
        # start it from 0 for easy indexing from smesh node numbering
        self.node_mask = np.ma.zeros(max(mesh_nodes.keys()) + 1, dtype=int)
        self.nodes = np.zeros(
            len(nodes_in_use), dtype=[("index", "u4"), ("coords", ("f8", (3,))), ("dof", "u1")]
        )
        self.node_mask.mask = np.full(self.node_mask.size, True)
        self.node_mask.mask[nodes_in_use] = False
        self.node_mask[nodes_in_use] = np.arange(len(nodes_in_use))
        # For consistency in z88 node/element files, force nodes id start from 1
        self.nodes["index"] = np.arange(1, len(nodes_in_use) + 1)

        nodecoords = []
        for n in nodes_in_use:
            nodecoords.append(mesh_nodes[n])

        self.nodes["coords"] = nodecoords

        smesh_to_z88_type = z88utils.smesh_to_z88_type(self.solver_obj)
        dt_elements = np.dtype(
            {
                "names": ["nodes", "index", "type", "size"],
                "formats": [("u4", (max_elem_nodes,)), "u4", "u1", "u1"],
            }
        )
        self.elements = np.zeros((e_count), dtype=dt_elements)
        self.elements["index"] = mesh_elem

        for elem in self.elements:
            nodes = list(mesh.getElementNodes(elem["index"]))
            n_len = len(nodes)
            smesh_type = smesh_type_from_nodes[n_len]
            try:
                z88_type = smesh_to_z88_type[smesh_type]
            except KeyError:
                raise ValueError(
                    f"Mesh element {elem['index']}: {z88utils.smesh_type_names[smesh_type]} type not supported by Z88"
                )
            elem["type"] = z88_type
            elem["nodes"][:n_len] = self.nodes["index"][self.node_mask[nodes]][
                z88utils.smesh_to_z88_order[smesh_type]
            ]
            elem["size"] = n_len
            # set dof for each node
            self.nodes["dof"][self.node_mask[nodes]] = z88utils.z88_dof[elem["type"]]

        self.element_id_map = dict(zip(self.elements["index"], range(e_count)))

        self.z88i1 = open(os.path.join(self.solver_obj.WorkingDirectory, "z88i1.txt"), "w")

        dim = 3 if self.solver_obj.ModelSpace == "3D" else 2
        self.z88i1.writelines(
            "{0} {1} {2} {3} written by FreeCAD\n".format(
                dim, self.nodes["index"].size, self.elements.size, np.sum(self.nodes["dof"])
            )
        )

        # Z88 elements ascending order and starting from 1
        for node in self.nodes:
            self.z88i1.writelines(
                "{0} {1} {2:E} {3:E} {4:E}\n".format(node["index"], node["dof"], *node["coords"])
            )

        for i, elem in enumerate(self.elements, start=1):
            self.z88i1.writelines("{} {}\n".format(i, elem["type"]))
            self.z88i1.writelines(" ".join(map(str, elem["nodes"][: elem["size"]])) + "\n")

        self.z88i1.close()

    def node_id_map(self, idx):
        n = self.node_mask[idx]
        if n is np.ma.masked:
            raise RuntimeError(f"Node {idx} is not part of any mesh element")
        return n


##  @}
