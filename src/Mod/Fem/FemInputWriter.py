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

__title__ = "FemInputWriter"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import FemMeshTools
import os


class FemInputWriter():
    def __init__(self,
                 analysis_obj, solver_obj,
                 mesh_obj, matlin_obj, matnonlin_obj,
                 fixed_obj, displacement_obj,
                 contact_obj, planerotation_obj, transform_obj,
                 selfweight_obj, force_obj, pressure_obj,
                 temperature_obj, heatflux_obj, initialtemperature_obj,
                 beamsection_obj, shellthickness_obj, fluidsection_obj,
                 analysis_type, dir_name
                 ):
        self.analysis = analysis_obj
        self.solver_obj = solver_obj
        self.mesh_object = mesh_obj
        self.material_objects = matlin_obj
        self.material_nonlinear_objects = matnonlin_obj
        self.fixed_objects = fixed_obj
        self.displacement_objects = displacement_obj
        self.contact_objects = contact_obj
        self.planerotation_objects = planerotation_obj
        self.transform_objects = transform_obj
        self.selfweight_objects = selfweight_obj
        self.force_objects = force_obj
        self.pressure_objects = pressure_obj
        self.temperature_objects = temperature_obj
        self.heatflux_objects = heatflux_obj
        self.initialtemperature_objects = initialtemperature_obj
        self.beamsection_objects = beamsection_obj
        self.fluidsection_objects = fluidsection_obj
        self.shellthickness_objects = shellthickness_obj
        self.analysis_type = analysis_type
        self.dir_name = dir_name
        if not dir_name:
            print('Error: FemInputWriter has no working_dir --> we are going to make a temporary one!')
            self.dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + analysis_obj.Uid[-4:]
        self.dir_name = os.path.join(self.dir_name, '')  # check dir_name has a slash at the end, if not add one
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)
        self.fc_ver = FreeCAD.Version()
        self.ccx_nall = 'Nall'
        self.ccx_eall = 'Eall'
        self.ccx_evolumes = 'Evolumes'
        self.ccx_efaces = 'Efaces'
        self.ccx_eedges = 'Eedges'
        self.ccx_elsets = []
        self.femmesh = self.mesh_object.FemMesh
        self.femnodes_mesh = {}
        self.femelement_table = {}
        self.constraint_conflict_nodes = []
        self.femnodes_ele_table = {}

    def get_constraints_fixed_nodes(self):
        # get nodes
        for femobj in self.fixed_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj['Nodes']:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_displacement_nodes(self):
        # get nodes
        for femobj in self.displacement_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)
            # add nodes to constraint_conflict_nodes, needed by constraint plane rotation
            for node in femobj['Nodes']:
                self.constraint_conflict_nodes.append(node)

    def get_constraints_planerotation_nodes(self):
        # get nodes
        for femobj in self.planerotation_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)

    def get_constraints_transform_nodes(self):
        # get nodes
        for femobj in self.transform_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)

    def get_constraints_temperature_nodes(self):
        # get nodes
        for femobj in self.temperature_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)

    def get_constraints_fluidsection_nodes(self):
        # get nodes
        for femobj in self.fluidsection_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['Nodes'] = FemMeshTools.get_femnodes_by_femobj_with_references(self.femmesh, femobj)

    def get_constraints_force_nodeloads(self):
        # check shape type of reference shape
        for femobj in self.force_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            frc_obj = femobj['Object']
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

    def get_constraints_pressure_faces(self):
        # TODO see comments in get_constraints_force_nodeloads(), it applies here too. Mhh it applies to all constraints ...

        '''
        # depreciated version
        # get the faces and face numbers
        for femobj in self.pressure_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            femobj['PressureFaces'] = FemMeshTools.get_pressure_obj_faces_depreciated(self.femmesh, femobj)
            # print(femobj['PressureFaces'])
        '''

        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        if not self.femnodes_ele_table:
            self.femnodes_ele_table = FemMeshTools.get_femnodes_ele_table(self.femnodes_mesh, self.femelement_table)

        for femobj in self.pressure_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            pressure_faces = FemMeshTools.get_pressure_obj_faces(self.femmesh, self.femelement_table, self.femnodes_ele_table, femobj)
            # print(len(pressure_faces))
            femobj['PressureFaces'] = [(femobj['Object'].Name + ': face load', pressure_faces)]
            print(femobj['PressureFaces'])

    def get_element_geometry2D_elements(self):
        # get element ids and write them into the objects
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.shellthickness_objects)

    def get_element_geometry1D_elements(self):
        # get element ids and write them into the objects
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.beamsection_objects)

    def get_element_fluid1D_elements(self):
        # get element ids and write them into the objects
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
        FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.fluidsection_objects)

    def get_material_elements(self):
        # it only works if either Volumes or Shellthicknesses or Beamsections are in the material objects
        # it means it does not work for mixed meshes and multiple materials, this is checked in check_prerequisites
        if self.femmesh.Volumes:
            # we only could do this for volumes, if a mesh contains volumes we're going to use them in the analysis
            # but a mesh could contain the element faces of the volumes as faces and the edges of the faces as edges,
            # there we have to check of some geometric objects
            all_found = False
            if self.femmesh.GroupCount:
                all_found = FemMeshTools.get_femelement_sets_from_group_data(self.femmesh, self.material_objects)
                print(all_found)
            if all_found is False:
                if not self.femelement_table:
                    self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
                # we're going to use the binary search for get_femelements_by_femnodes()
                # thus we need the parameter values self.femnodes_ele_table
                if not self.femnodes_mesh:
                    self.femnodes_mesh = self.femmesh.Nodes
                if not self.femnodes_ele_table:
                    self.femnodes_ele_table = FemMeshTools.get_femnodes_ele_table(self.femnodes_mesh, self.femelement_table)
                FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects, self.femnodes_ele_table)
        if self.shellthickness_objects:
            if not self.femelement_table:
                self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
            FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)
        if self.beamsection_objects or self.fluidsection_objects:
            if not self.femelement_table:
                self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
            FemMeshTools.get_femelement_sets(self.femmesh, self.femelement_table, self.material_objects)

##  @}
