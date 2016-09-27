# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


'''
- next step would be save the constraints node and element data in the in the FreeCAD FEM Mesh Object
  and link them to the appropriate constraint object
- if the informations are used by the FEM Mesh file exporter FreeCAD would support writing FEM Mesh Groups
- which is a most needed feature of FEM module
- smesh supports mesh groups, how about pythonbinding in FreeCAD. Is there somethin implemented allready?
'''


__title__ = "FemInputWriter"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
import FemMeshTools
import os


class FemInputWriter():
    def __init__(self,
                 analysis_obj, solver_obj,
                 mesh_obj, mat_obj,
                 fixed_obj, displacement_obj,
                 contact_obj, planerotation_obj,
                 selfweight_obj, force_obj, pressure_obj,
                 temperature_obj, heatflux_obj, initialtemperature_obj,
                 beamsection_obj, shellthickness_obj,
                 analysis_type, dir_name
                 ):
        self.analysis = analysis_obj
        self.solver_obj = solver_obj
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.displacement_objects = displacement_obj
        self.contact_objects = contact_obj
        self.planerotation_objects = planerotation_obj
        self.selfweight_objects = selfweight_obj
        self.force_objects = force_obj
        self.pressure_objects = pressure_obj
        self.temperature_objects = temperature_obj
        self.heatflux_objects = heatflux_obj
        self.initialtemperature_objects = initialtemperature_obj
        self.beamsection_objects = beamsection_obj
        self.shellthickness_objects = shellthickness_obj
        self.analysis_type = analysis_type
        self.dir_name = dir_name
        if not dir_name:
            print('Error: FemInputWriter has no working_dir --> we gone make a temporary one!')
            self.dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + analysis_obj.Uid[-4:]
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)
        self.fc_ver = FreeCAD.Version()
        self.ccx_eall = 'Eall'
        self.ccx_elsets = []
        self.femmesh = self.mesh_object.FemMesh
        self.femnodes_mesh = {}
        self.femelement_table = {}
        self.constraint_conflict_nodes = []

    def get_constraints_fixed_nodes(self):
        # get nodes
        for femobj in self.fixed_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_references(self.femmesh, femobj['Object'].References)
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj['Nodes']:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_displacement_nodes(self):
        # get nodes
        for femobj in self.displacement_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_references(self.femmesh, femobj['Object'].References)
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj['Nodes']:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_planerotation_nodes(self):
        # get nodes
        for femobj in self.planerotation_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_references(self.femmesh, femobj['Object'].References)

    def get_constraints_temperature_nodes(self):
        # get nodes
        for femobj in self.temperature_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_references(self.femmesh, femobj['Object'].References)

    def get_constraints_force_nodeloads(self):
        # check shape type of reference shape
        for femobj in self.force_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            frc_obj = femobj['Object']
            # in GUI defined frc_obj all ref_shape have the same shape type
            # TODO in FemTools: check if all RefShapes really have the same type an write type to dictionary
            femobj['RefShapeType'] = ''
            if frc_obj.References:
                first_ref_obj = frc_obj.References[0]
                first_ref_shape = first_ref_obj[0].Shape.getElement(first_ref_obj[1][0])
                femobj['RefShapeType'] = first_ref_shape.ShapeType
            else:
                # frc_obj.References could be empty ! # TODO in FemTools: check
                FreeCAD.Console.PrintError('At least one Force Object has empty References!\n')
            if femobj['RefShapeType'] == 'Vertex':
                # print("load on vertices --> we do not need the femelement_table and femnodes_mesh for node load calculation")
                pass
            elif femobj['RefShapeType'] == 'Face' and FemMeshTools.is_solid_femmesh(self.femmesh) and not FemMeshTools.has_no_face_data(self.femmesh):
                # print("solid_mesh with face data --> we do not need the femelement_table but we need the femnodes_mesh for node load calculation")
                if not self.femnodes_mesh:
                    self.femnodes_mesh = self.femmesh.Nodes
            else:
                # print("mesh without needed data --> we need the femelement_table and femnodes_mesh for node load calculation")
                if not self.femnodes_mesh:
                    self.femnodes_mesh = self.femmesh.Nodes
                if not self.femelement_table:
                    self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        # get node loads
        for femobj in self.force_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            frc_obj = femobj['Object']
            if frc_obj.Force == 0:
                print('  Warning --> Force = 0')
            if femobj['RefShapeType'] == 'Vertex':  # point load on vertieces
                femobj['NodeLoadTable'] = FemMeshTools.get_force_obj_vertex_nodeload_table(self.femmesh, frc_obj)
            elif femobj['RefShapeType'] == 'Edge':  # line load on edges
                femobj['NodeLoadTable'] = FemMeshTools.get_force_obj_edge_nodeload_table(self.femmesh, self.femelement_table, self.femnodes_mesh, frc_obj)
            elif femobj['RefShapeType'] == 'Face':  # area load on faces
                femobj['NodeLoadTable'] = FemMeshTools.get_force_obj_face_nodeload_table(self.femmesh, self.femelement_table, self.femnodes_mesh, frc_obj)
