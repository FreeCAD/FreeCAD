# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


import FreeCAD
import os
import sys
import time


__title__ = "ccxInpWriter"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


class inp_writer:
    def __init__(self, analysis_obj, mesh_obj, mat_obj,
                 fixed_obj,
                 force_obj, pressure_obj,
                 displacement_obj,
                 beamsection_obj, shellthickness_obj,
                 analysis_type=None, eigenmode_parameters=None,
                 dir_name=None):
        self.dir_name = dir_name
        self.analysis = analysis_obj
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.force_objects = force_obj
        self.pressure_objects = pressure_obj
        self.displacement_objects = displacement_obj
        if eigenmode_parameters:
            self.no_of_eigenfrequencies = eigenmode_parameters[0]
            self.eigenfrequeny_range_low = eigenmode_parameters[1]
            self.eigenfrequeny_range_high = eigenmode_parameters[2]
        self.analysis_type = analysis_type
        self.beamsection_objects = beamsection_obj
        self.shellthickness_objects = shellthickness_obj
        if not dir_name:
            self.dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + analysis_obj.Uid[-4:]
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)
        self.file_name = self.dir_name + '/' + self.mesh_object.Name + '.inp'
        self.fc_ver = FreeCAD.Version()
        self.ccx_eall = 'Eall'
        self.ccx_elsets = []
        self.fem_mesh_nodes = {}

    def write_calculix_input_file(self):
        self.mesh_object.FemMesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')
        self.write_element_sets_material_and_femelement_type(inpfile)
        self.write_node_sets_constraints_fixed(inpfile)
        self.write_node_sets_constraints_displacement(inpfile)
        if self.analysis_type is None or self.analysis_type == "static":
            self.write_node_sets_constraints_force(inpfile)
        self.write_materials(inpfile)
        self.write_femelementsets(inpfile)
        self.write_step_begin(inpfile)
        self.write_constraints_fixed(inpfile)
        self.write_constraints_displacement(inpfile)
        if self.analysis_type is None or self.analysis_type == "static":
            self.write_constraints_force(inpfile)
            self.write_constraints_pressure(inpfile)
        elif self.analysis_type == "frequency":
            self.write_frequency(inpfile)
        self.write_outputs_types(inpfile)
        self.write_step_end(inpfile)
        self.write_footer(inpfile)
        inpfile.close()
        return self.file_name

    def write_element_sets_material_and_femelement_type(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Element sets for materials and FEM element type (solid, shell, beam)\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        if len(self.material_objects) == 1:
            if self.beamsection_objects and len(self.beamsection_objects) == 1:          # single mat, single beam
                self.get_ccx_elsets_single_mat_single_beam()
            elif self.beamsection_objects and len(self.beamsection_objects) > 1:         # single mat, multiple beams
                self.get_ccx_elsets_single_mat_multiple_beam()
            elif self.shellthickness_objects and len(self.shellthickness_objects) == 1:  # single mat, single shell
                self.get_ccx_elsets_single_mat_single_shell()
            elif self.shellthickness_objects and len(self.shellthickness_objects) > 1:   # single mat, multiple shells
                self.get_ccx_elsets_single_mat_multiple_shell()
            else:                                                                        # single mat, solid
                self.get_ccx_elsets_single_mat_solid()
        else:
            if self.beamsection_objects and len(self.beamsection_objects) == 1:         # multiple mats, single beam
                self.get_ccx_elsets_multiple_mat_single_beam()
            elif self.beamsection_objects and len(self.beamsection_objects) > 1:        # multiple mats, multiple beams
                self.get_ccx_elsets_multiple_mat_multiple_beam()
            elif self.shellthickness_objects and len(self.shellthickness_objects) == 1:   # multiple mats, single shell
                self.get_ccx_elsets_multiple_mat_single_shell()
            elif self.shellthickness_objects and len(self.shellthickness_objects) > 1:  # multiple mats, multiple shells
                self.get_ccx_elsets_multiple_mat_multiple_shell()
            else:                                                                       # multiple mats, solid
                self.get_ccx_elsets_multiple_mat_solid()
        for ccx_elset in self.ccx_elsets:
            f.write('*ELSET,ELSET=' + ccx_elset['ccx_elset_name'] + '\n')
            if ccx_elset['ccx_elset']:
                if ccx_elset['ccx_elset'] == self.ccx_eall:
                    f.write(self.ccx_eall + '\n')
                else:
                    for elid in ccx_elset['ccx_elset']:
                        f.write(str(elid) + ',\n')
            else:
                f.write('**No elements found for these objects\n')

    def write_node_sets_constraints_fixed(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node set for fixed constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.fixed_objects:
            fix_obj = fobj['Object']
            f.write('*NSET,NSET=' + fix_obj.Name + '\n')
            for o, elem in fix_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Face':
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')

    def write_node_sets_constraints_displacement(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node sets for prescribed displacement constraint\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.displacement_objects:
            disp_obj = fobj['Object']
            f.write('*NSET,NSET=' + disp_obj.Name + '\n')
            for o, elem in disp_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Face':
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')

    def write_node_sets_constraints_force(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node sets for loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            # in GUI defined frc_obj all ref_shape have the same shape type
            # TODO in FemTools: check if all RefShapes really have the same type an write type to dictionary
            fobj['RefShapeType'] = ''
            if frc_obj.References:
                first_ref_obj = frc_obj.References[0]
                first_ref_shape = first_ref_obj[0].Shape.getElement(first_ref_obj[1])
                fobj['RefShapeType'] = first_ref_shape.ShapeType
            else:
                # frc_obj.References could be empty ! # TODO in FemTools: check
                FreeCAD.Console.PrintError('At least one Force Object has empty References!\n')
            if fobj['RefShapeType'] == 'Vertex':
                pass  # point load on vertices --> the FemElementTable is not needed for node load calculation
            elif fobj['RefShapeType'] == 'Face' and is_solid_mesh(self.mesh_object.FemMesh) and not has_no_face_data(self.mesh_object.FemMesh):
                pass  # solid_mesh with face data --> the FemElementTable is not needed for node load calculation
            else:
                if not hasattr(self, 'fem_element_table'):
                    self.fem_element_table = getFemElementTable(self.mesh_object.FemMesh)
        for fobj in self.force_objects:
            if fobj['RefShapeType'] == 'Vertex':
                frc_obj = fobj['Object']
                f.write('*NSET,NSET=' + frc_obj.Name + '\n')
                NbrForceNodes = 0
                for o, elem in frc_obj.References:
                    fo = o.Shape.getElement(elem)
                    n = []
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                    for i in n:
                        f.write(str(i) + ',\n')
                        NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from force_object
                # calculate node load
                if NbrForceNodes != 0:
                    fobj['NodeLoad'] = (frc_obj.Force) / NbrForceNodes
                    f.write('** concentrated load [N] distributed on all mesh nodes of the given vertieces\n')
                    f.write('** ' + str(frc_obj.Force) + ' N / ' + str(NbrForceNodes) + ' Nodes = ' + str(fobj['NodeLoad']) + ' N on each node\n')
            else:
                f.write('** no point load on vertices --> no set for node loads\n')

    def write_materials(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Materials\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('** Young\'s modulus unit is MPa = N/mm2\n')
        for m in self.material_objects:
            mat_obj = m['Object']
            # get material properties
            YM = FreeCAD.Units.Quantity(mat_obj.Material['YoungsModulus'])
            YM_in_MPa = YM.getValueAs('MPa')
            PR = float(mat_obj.Material['PoissonRatio'])
            mat_info_name = mat_obj.Material['Name']
            mat_name = mat_obj.Name
            # write material properties
            f.write('**FreeCAD material name: ' + mat_info_name + '\n')
            f.write('*MATERIAL, NAME=' + mat_name + '\n')
            f.write('*ELASTIC \n')
            f.write('{},  '.format(YM_in_MPa))
            f.write('{0:.3f}\n'.format(PR))
            density = FreeCAD.Units.Quantity(mat_obj.Material['Density'])
            density_in_tone_per_mm3 = float(density.getValueAs('t/mm^3'))
            f.write('*DENSITY \n')
            f.write('{0:.3e}, \n'.format(density_in_tone_per_mm3))

    def write_femelementsets(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Sections\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for ccx_elset in self.ccx_elsets:
            if ccx_elset['ccx_elset']:
                if 'beamsection_obj'in ccx_elset:  # beam mesh
                    beamsec_obj = ccx_elset['beamsection_obj']
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    setion_def = '*BEAM SECTION, ' + elsetdef + material + ', SECTION=RECT\n'
                    setion_geo = str(beamsec_obj.Height.getValueAs('mm')) + ', ' + str(beamsec_obj.Width.getValueAs('mm')) + '\n'
                    f.write(setion_def)
                    f.write(setion_geo)
                elif 'shellthickness_obj'in ccx_elset:  # shell mesh
                    shellth_obj = ccx_elset['shellthickness_obj']
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    setion_def = '*SHELL SECTION, ' + elsetdef + material + '\n'
                    setion_geo = str(shellth_obj.Thickness.getValueAs('mm')) + '\n'
                    f.write(setion_def)
                    f.write(setion_geo)
                else:  # solid mesh
                    elsetdef = 'ELSET=' + ccx_elset['ccx_elset_name'] + ', '
                    material = 'MATERIAL=' + ccx_elset['mat_obj_name']
                    setion_def = '*SOLID SECTION, ' + elsetdef + material + '\n'
                    f.write(setion_def)

    def write_step_begin(self, f):
        f.write('\n***********************************************************\n')
        f.write('** One step is needed to calculate the mechanical analysis of FreeCAD\n')
        f.write('** loads are applied quasi-static, means without involving the time dimension\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*STEP\n')
        f.write('*STATIC\n')

    def write_constraints_fixed(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Constaints\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fixed_object in self.fixed_objects:
            fix_obj_name = fixed_object['Object'].Name
            f.write('*BOUNDARY\n')
            f.write(fix_obj_name + ',1\n')
            f.write(fix_obj_name + ',2\n')
            f.write(fix_obj_name + ',3\n')
            if self.beamsection_objects or self.shellthickness_objects:
                f.write(fix_obj_name + ',4\n')
                f.write(fix_obj_name + ',5\n')
                f.write(fix_obj_name + ',6\n')
            f.write('\n')

    def write_constraints_displacement(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Displacement constraint applied\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for disp_obj in self.displacement_objects:
            disp_obj_name = disp_obj['Object'].Name
            f.write('*BOUNDARY\n')
            if disp_obj['Object'].xFix:
                f.write(disp_obj_name + ',1\n')
            elif not disp_obj['Object'].xFree:
                f.write(disp_obj_name + ',1,1,' + str(disp_obj['Object'].xDisplacement) + '\n')
            if disp_obj['Object'].yFix:
                f.write(disp_obj_name + ',2\n')
            elif not disp_obj['Object'].yFree:
                f.write(disp_obj_name + ',2,2,' + str(disp_obj['Object'].yDisplacement) + '\n')
            if disp_obj['Object'].zFix:
                f.write(disp_obj_name + ',3\n')
            elif not disp_obj['Object'].zFree:
                f.write(disp_obj_name + ',3,3,' + str(disp_obj['Object'].zDisplacement) + '\n')

            if self.beamsection_objects or self.shellthickness_objects:
                if disp_obj['Object'].rotxFix:
                    f.write(disp_obj_name + ',4\n')
                elif not disp_obj['Object'].rotxFree:
                    f.write(disp_obj_name + ',4,4,' + str(disp_obj['Object'].xRotation) + '\n')
                if disp_obj['Object'].rotyFix:
                    f.write(disp_obj_name + ',5\n')
                elif not disp_obj['Object'].rotyFree:
                    f.write(disp_obj_name + ',5,5,' + str(disp_obj['Object'].yRotation) + '\n')
                if disp_obj['Object'].rotzFix:
                    f.write(disp_obj_name + ',6\n')
                elif not disp_obj['Object'].rotzFree:
                    f.write(disp_obj_name + ',6,6,' + str(disp_obj['Object'].zRotation) + '\n')
        f.write('\n')

    def write_constraints_force(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*CLOAD\n')
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            f.write('** ' + frc_obj.Name + '\n')
            direction_vec = frc_obj.DirectionVector
            if frc_obj.Force == 0:
                print('  Warning --> Force = 0')

            if fobj['RefShapeType'] == 'Vertex':  # point load on vertieces
                node_load = fobj['NodeLoad']
                frc_obj_name = frc_obj.Name
                f.write('** force: ' + str(node_load) + ' N,  direction: ' + str(direction_vec) + '\n')
                v1 = "{:.13E}".format(direction_vec.x * node_load)
                v2 = "{:.13E}".format(direction_vec.y * node_load)
                v3 = "{:.13E}".format(direction_vec.z * node_load)
                f.write(frc_obj_name + ',1,' + v1 + '\n')
                f.write(frc_obj_name + ',2,' + v2 + '\n')
                f.write(frc_obj_name + ',3,' + v3 + '\n\n')

            elif fobj['RefShapeType'] == 'Edge':  # line load on edges
                sum_ref_edge_length = 0
                sum_ref_edge_node_length = 0  # for debugging
                sum_node_load = 0  # for debugging
                for o, elem in frc_obj.References:
                    elem_o = o.Shape.getElement(elem)
                    sum_ref_edge_length += elem_o.Length
                if sum_ref_edge_length != 0:
                    force_per_sum_ref_edge_length = frc_obj.Force / sum_ref_edge_length
                for o, elem in frc_obj.References:
                    elem_o = o.Shape.getElement(elem)
                    ref_edge = elem_o

                    # edge_table = { meshedgeID : ( nodeID, ... , nodeID ) }
                    edge_table = self.get_refedge_node_table(ref_edge)

                    # node_length_table = [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry
                    node_length_table = self.get_refedge_node_lengths(edge_table)

                    # node_sum_length_table = { nodeID : Length, ... , nodeID : Length }  LengthSum for each node, one entry for each node
                    node_sum_length_table = self.get_ref_shape_node_sum_geom_table(node_length_table)

                    # node_load_table = { nodeID : NodeLoad, ... , nodeID : NodeLoad }  NodeLoad for each node, one entry for each node
                    node_load_table = {}
                    sum_node_lengths = 0  # for debugging
                    for node in node_sum_length_table:
                        sum_node_lengths += node_sum_length_table[node]  # for debugging
                        node_load_table[node] = node_sum_length_table[node] * force_per_sum_ref_edge_length
                    ratio_refedge_lengths = sum_node_lengths / elem_o.Length
                    if ratio_refedge_lengths < 0.99 or ratio_refedge_lengths > 1.01:
                        FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
                        print('  sum_node_lengths:', sum_node_lengths)
                        print('  refedge_length:  ', elem_o.Length)
                        bad_refedge = elem_o
                    sum_ref_edge_node_length += sum_node_lengths

                    f.write('** node loads on element ' + fobj['RefShapeType'] + ': ' + o.Name + ':' + elem + '\n')
                    for n in sorted(node_load_table):
                        node_load = node_load_table[n]
                        sum_node_load += node_load  # for debugging
                        if (direction_vec.x != 0.0):
                            v1 = "{:.13E}".format(direction_vec.x * node_load)
                            f.write(str(n) + ',1,' + v1 + '\n')
                        if (direction_vec.y != 0.0):
                            v2 = "{:.13E}".format(direction_vec.y * node_load)
                            f.write(str(n) + ',2,' + v2 + '\n')
                        if (direction_vec.z != 0.0):
                            v3 = "{:.13E}".format(direction_vec.z * node_load)
                            f.write(str(n) + ',3,' + v3 + '\n')
                    f.write('\n')
                f.write('\n')
                ratio = sum_node_load / frc_obj.Force
                if ratio < 0.99 or ratio > 1.01:
                    print('Deviation  sum_node_load to frc_obj.Force is more than 1% :  ', ratio)
                    print('  sum_ref_edge_node_length: ', sum_ref_edge_node_length)
                    print('  sum_ref_edge_length:      ', sum_ref_edge_length)
                    print('  sum_node_load:          ', sum_node_load)
                    print('  frc_obj.Force:          ', frc_obj.Force)
                    print('  the reason could be simply a circle length --> see method get_ref_edge_node_lengths')
                    print('  the reason could also be an problem in retrieving the ref_edge_node_length')

                    # try debugging of the last bad refedge
                    print('DEBUGGING')
                    print(bad_refedge)

                    print('bad_refedge_nodes')
                    bad_refedge_nodes = self.mesh_object.FemMesh.getNodesByEdge(bad_refedge)
                    print(len(bad_refedge_nodes))
                    print(bad_refedge_nodes)
                    # import FreeCADGui
                    # FreeCADGui.ActiveDocument.Compound_Mesh.HighlightedNodes = bad_refedge_nodes

                    print('bad_edge_table')
                    # bad_edge_table = { meshedgeID : ( nodeID, ... , nodeID ) }
                    bad_edge_table = self.get_refedge_node_table(bad_refedge)
                    print(len(bad_edge_table))
                    bad_edge_table_nodes = []
                    for elem in bad_edge_table:
                        print(elem, ' --> ', bad_edge_table[elem])
                        for node in bad_edge_table[elem]:
                            if node not in bad_edge_table_nodes:
                                bad_edge_table_nodes.append(node)
                    print('sorted(bad_edge_table_nodes)')
                    print(sorted(bad_edge_table_nodes))   # should be == bad_refedge_nodes
                    # import FreeCADGui
                    # FreeCADGui.ActiveDocument.Compound_Mesh.HighlightedNodes = bad_edge_table_nodes
                    # bad_node_length_table = [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry

                    print('good_edge_table')
                    good_edge_table = delete_duplicate_mesh_elements(bad_edge_table)
                    for elem in good_edge_table:
                        print(elem, ' --> ', bad_edge_table[elem])

                    print('bad_node_length_table')
                    bad_node_length_table = self.get_refedge_node_lengths(bad_edge_table)
                    for n, l in bad_node_length_table:
                        print(n, ' --> ', l)

            elif fobj['RefShapeType'] == 'Face':  # area load on faces
                sum_ref_face_area = 0
                sum_ref_face_node_area = 0  # for debugging
                sum_node_load = 0  # for debugging
                for o, elem in frc_obj.References:
                    elem_o = o.Shape.getElement(elem)
                    sum_ref_face_area += elem_o.Area
                if sum_ref_face_area != 0:
                    force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area
                for o, elem in frc_obj.References:
                    elem_o = o.Shape.getElement(elem)
                    ref_face = elem_o

                    # face_table = { meshfaceID : ( nodeID, ... , nodeID ) }
                    face_table = self.get_ref_face_node_table(ref_face)

                    # node_area_table = [ (nodeID, Area), ... , (nodeID, Area) ]  some nodes will have more than one entry
                    node_area_table = self.get_ref_face_node_areas(face_table)

                    # node_sum_area_table = { nodeID : Area, ... , nodeID : Area }  AreaSum for each node, one entry for each node
                    node_sum_area_table = self.get_ref_shape_node_sum_geom_table(node_area_table)

                    # node_load_table = { nodeID : NodeLoad, ... , nodeID : NodeLoad }  NodeLoad for each node, one entry for each node
                    node_load_table = {}
                    sum_node_areas = 0  # for debugging
                    for node in node_sum_area_table:
                        sum_node_areas += node_sum_area_table[node]  # for debugging
                        node_load_table[node] = node_sum_area_table[node] * force_per_sum_ref_face_area
                    ratio_refface_areas = sum_node_areas / elem_o.Area
                    if ratio_refface_areas < 0.99 or ratio_refface_areas > 1.01:
                        FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
                        print('  sum_node_lengths:', sum_node_areas)
                        print('  refedge_length:  ', elem_o.Area)
                    sum_ref_face_node_area += sum_node_areas

                    f.write('** node loads on element ' + fobj['RefShapeType'] + ': ' + o.Name + ':' + elem + '\n')
                    for n in sorted(node_load_table):
                        node_load = node_load_table[n]
                        sum_node_load += node_load  # for debugging
                        if (direction_vec.x != 0.0):
                            v1 = "{:.13E}".format(direction_vec.x * node_load)
                            f.write(str(n) + ',1,' + v1 + '\n')
                        if (direction_vec.y != 0.0):
                            v2 = "{:.13E}".format(direction_vec.y * node_load)
                            f.write(str(n) + ',2,' + v2 + '\n')
                        if (direction_vec.z != 0.0):
                            v3 = "{:.13E}".format(direction_vec.z * node_load)
                            f.write(str(n) + ',3,' + v3 + '\n')
                    f.write('\n')
                f.write('\n')
                ratio = sum_node_load / frc_obj.Force
                if ratio < 0.99 or ratio > 1.01:
                    print('Deviation  sum_node_load to frc_obj.Force is more than 1% :  ', ratio)
                    print('  sum_ref_face_node_area: ', sum_ref_face_node_area)
                    print('  sum_ref_face_area:      ', sum_ref_face_area)
                    print('  sum_node_load:          ', sum_node_load)
                    print('  frc_obj.Force:          ', frc_obj.Force)
                    print('  the reason could be simply a circle area --> see method get_ref_face_node_areas')
                    print('  the reason could also be an problem in retrieving the ref_face_node_area')

    def write_constraints_pressure(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Element + CalculiX face + load in [MPa]\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.pressure_objects:
            prs_obj = fobj['Object']
            f.write('*DLOAD\n')
            for o, e in prs_obj.References:
                rev = -1 if prs_obj.Reversed else 1
                elem = o.Shape.getElement(e)
                if elem.ShapeType == 'Face':
                    v = self.mesh_object.FemMesh.getccxVolumesByFace(elem)
                    f.write("** Load on face {}\n".format(e))
                    for i in v:
                        f.write("{},P{},{}\n".format(i[0], i[1], rev * prs_obj.Pressure))

    def write_frequency(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Frequency analysis\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*FREQUENCY\n')
        f.write('{},{},{}\n'.format(self.no_of_eigenfrequencies, self.eigenfrequeny_range_low, self.eigenfrequeny_range_high))

    def write_outputs_types(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Outputs --> frd file\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        if self.beamsection_objects or self.shellthickness_objects:
            f.write('*NODE FILE, OUTPUT=2d\n')
        else:
            f.write('*NODE FILE\n')
        f.write('U\n')
        f.write('*EL FILE\n')
        f.write('S, E\n')
        f.write('** outputs --> dat file\n')
        f.write('*NODE PRINT , NSET=Nall \n')
        f.write('U \n')
        f.write('*EL PRINT , ELSET=Eall \n')
        f.write('S \n')

    def write_step_end(self, f):
        f.write('\n***********************************************************\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('*END STEP \n')

    def write_footer(self, f):
        f.write('\n***********************************************************\n')
        f.write('** CalculiX Input file\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        f.write('**   written by    --> FreeCAD ' + self.fc_ver[0] + '.' + self.fc_ver[1] + '.' + self.fc_ver[2] + '\n')
        f.write('**   written on    --> ' + time.ctime() + '\n')
        f.write('**   file name     --> ' + os.path.basename(FreeCAD.ActiveDocument.FileName) + '\n')
        f.write('**   analysis name --> ' + self.analysis.Name + '\n')
        f.write('**\n')
        f.write('**\n')
        f.write('**\n')
        f.write('**   Units\n')
        f.write('**\n')
        f.write('**   Geometry (mesh data)        --> mm\n')
        f.write("**   Materials (Young's modulus) --> N/mm2 = MPa\n")
        f.write('**   Loads (nodal loads)         --> N\n')
        f.write('**\n')

    # self.ccx_elsets = [ {
    #                        'beamsection_obj' : 'beamsection_obj'       if exists
    #                        'shellthickness_obj' : shellthickness_obj'  if exists
    #                        'ccx_elset' : [e1, e2, e3, ... , en] or string self.ccx_eall
    #                        'ccx_elset_name' : 'ccx_identifier_elset'
    #                        'mat_obj_name' : 'mat_obj.Name'
    #                        'ccx_mat_name' : 'mat_obj.Material['Name']'   !!! not unique !!!
    #                     },
    #                     {}, ... , {} ]
    def get_ccx_elsets_single_mat_single_beam(self):
        mat_obj = self.material_objects[0]['Object']
        beamsec_obj = self.beamsection_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['beamsection_obj'] = beamsec_obj
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_single_shell(self):
        mat_obj = self.material_objects[0]['Object']
        shellth_obj = self.shellthickness_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['shellthickness_obj'] = shellth_obj
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_solid(self):
        mat_obj = self.material_objects[0]['Object']
        ccx_elset = {}
        ccx_elset['ccx_elset'] = self.ccx_eall
        ccx_elset['ccx_elset_name'] = get_ccx_elset_solid_name(mat_obj.Name)
        ccx_elset['mat_obj_name'] = mat_obj.Name
        ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
        self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_beam(self):
        mat_obj = self.material_objects[0]['Object']
        self.get_femelement_sets(self.beamsection_objects)
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data['Object']
            ccx_elset = {}
            ccx_elset['beamsection_obj'] = beamsec_obj
            ccx_elset['ccx_elset'] = beamsec_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, None, beamsec_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_single_mat_multiple_shell(self):
        mat_obj = self.material_objects[0]['Object']
        self.get_femelement_sets(self.shellthickness_objects)
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data['Object']
            ccx_elset = {}
            ccx_elset['shellthickness_obj'] = shellth_obj
            ccx_elset['ccx_elset'] = shellth_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, None, shellth_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_beam(self):
        beamsec_obj = self.beamsection_objects[0]['Object']
        self.get_femelement_sets(self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['beamsection_obj'] = beamsec_obj
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_single_shell(self):
        shellth_obj = self.shellthickness_objects[0]['Object']
        self.get_femelement_sets(self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['shellthickness_obj'] = shellth_obj
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_solid(self):
        self.get_femelement_sets(self.material_objects)
        for mat_data in self.material_objects:
            mat_obj = mat_data['Object']
            ccx_elset = {}
            ccx_elset['ccx_elset'] = mat_data['FEMElements']
            ccx_elset['ccx_elset_name'] = get_ccx_elset_solid_name(mat_obj.Name, None, mat_data['ShortName'])
            ccx_elset['mat_obj_name'] = mat_obj.Name
            ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
            self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_beam(self):
        self.get_femelement_sets(self.beamsection_objects)
        self.get_femelement_sets(self.material_objects)
        for beamsec_data in self.beamsection_objects:
            beamsec_obj = beamsec_data['Object']
            for mat_data in self.material_objects:
                mat_obj = mat_data['Object']
                ccx_elset = {}
                ccx_elset['beamsection_obj'] = beamsec_obj
                elemids = []
                for elemid in beamsec_data['FEMElements']:
                    if elemid in mat_data['FEMElements']:
                        elemids.append(elemid)
                ccx_elset['ccx_elset'] = elemids
                ccx_elset['ccx_elset_name'] = get_ccx_elset_beam_name(mat_obj.Name, beamsec_obj.Name, mat_data['ShortName'], beamsec_data['ShortName'])
                ccx_elset['mat_obj_name'] = mat_obj.Name
                ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
                self.ccx_elsets.append(ccx_elset)

    def get_ccx_elsets_multiple_mat_multiple_shell(self):
        self.get_femelement_sets(self.shellthickness_objects)
        self.get_femelement_sets(self.material_objects)
        for shellth_data in self.shellthickness_objects:
            shellth_obj = shellth_data['Object']
            for mat_data in self.material_objects:
                mat_obj = mat_data['Object']
                ccx_elset = {}
                ccx_elset['shellthickness_obj'] = shellth_obj
                elemids = []
                for elemid in shellth_data['FEMElements']:
                    if elemid in mat_data['FEMElements']:
                        elemids.append(elemid)
                ccx_elset['ccx_elset'] = elemids
                ccx_elset['ccx_elset_name'] = get_ccx_elset_shell_name(mat_obj.Name, shellth_obj.Name, mat_data['ShortName'], shellth_data['ShortName'])
                ccx_elset['mat_obj_name'] = mat_obj.Name
                ccx_elset['ccx_mat_name'] = mat_obj.Material['Name']
                self.ccx_elsets.append(ccx_elset)

    def get_femelement_sets(self, fem_objects):
        # get femelements for reference shapes of each obj.References
        if not hasattr(self, 'fem_element_table'):
            self.fem_element_table = getFemElementTable(self.mesh_object.FemMesh)
        count_femelements = 0
        referenced_femelements = []
        has_remaining_femelements = None
        for fem_object_i, fem_object in enumerate(fem_objects):
            obj = fem_object['Object']
            fem_object['ShortName'] = get_ccx_elset_short_name(obj, fem_object_i)  # unique short ccx_identifier
            if obj.References:
                ref_shape_femelements = []
                for ref in obj.References:
                    femnodes = []
                    femelements = []
                    if ref[1]:
                        r = ref[0].Shape.getElement(ref[1])
                    else:
                        r = ref[0].Shape
                    # print('  ReferenceShape : ', r.ShapeType, ', ', ref[0].Name, ', ', ref[0].Label, ' --> ', ref[1])
                    if r.ShapeType == 'Edge':
                        femnodes = self.mesh_object.FemMesh.getNodesByEdge(r)
                    elif r.ShapeType == 'Face':
                        femnodes = self.mesh_object.FemMesh.getNodesByFace(r)
                    elif r.ShapeType == 'Solid':
                        femnodes = self.mesh_object.FemMesh.getNodesBySolid(r)
                    else:
                        print('  No Edge, Face or Solid as reference shapes!')
                    femelements = getFemElementsByNodes(self.fem_element_table, femnodes)
                    ref_shape_femelements += femelements
                    referenced_femelements += femelements
                    count_femelements += len(femelements)
                fem_object['FEMElements'] = ref_shape_femelements
            else:
                has_remaining_femelements = obj.Name
        # get remaining femelements for the fem_objects
        if has_remaining_femelements:
            remaining_femelements = []
            for elemid in self.fem_element_table:
                if elemid not in referenced_femelements:
                    remaining_femelements.append(elemid)
            count_femelements += len(remaining_femelements)
            for fem_object in fem_objects:
                obj = fem_object['Object']
                if obj.Name == has_remaining_femelements:
                    fem_object['FEMElements'] = sorted(remaining_femelements)
        # check if all worked out well
        if not femelements_count_ok(self.fem_element_table, count_femelements):
            FreeCAD.Console.PrintError('Error in get_femelement_sets -- > femelements_count_ok failed!\n')

    def get_refedge_node_table(self, refedge):
        edge_table = {}  # { meshedgeID : ( nodeID, ... , nodeID ) }
        refedge_nodes = self.mesh_object.FemMesh.getNodesByEdge(refedge)
        if is_solid_mesh(self.mesh_object.FemMesh):
            refedge_fem_volumeelements = []
            # if at least two nodes of a femvolumeelement are in refedge_nodes the volume is added to refedge_fem_volumeelements
            for elem in self.fem_element_table:
                nodecount = 0
                for node in self.fem_element_table[elem]:
                    if node in refedge_nodes:
                        nodecount += 1
                if nodecount > 1:
                    refedge_fem_volumeelements.append(elem)
            # for every refedge_fem_volumeelement look which of his nodes is in refedge_nodes --> add all these nodes to edge_table
            for elem in refedge_fem_volumeelements:
                fe_refedge_nodes = []
                for node in self.fem_element_table[elem]:
                    if node in refedge_nodes:
                        fe_refedge_nodes.append(node)
                    edge_table[elem] = fe_refedge_nodes  # { volumeID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
            #  FIXME duplicate_mesh_elements: as soon as contact ans springs are supported the user should decide on which edge the load is applied
            edge_table = delete_duplicate_mesh_elements(edge_table)
        elif is_shell_mesh(self.mesh_object.FemMesh):
            refedge_fem_faceelements = []
            # if at least two nodes of a femfaceelement are in refedge_nodes the volume is added to refedge_fem_volumeelements
            for elem in self.fem_element_table:
                nodecount = 0
                for node in self.fem_element_table[elem]:
                    if node in refedge_nodes:
                        nodecount += 1
                if nodecount > 1:
                    refedge_fem_faceelements.append(elem)
            # for every refedge_fem_faceelement look which of his nodes is in refedge_nodes --> add all these nodes to edge_table
            for elem in refedge_fem_faceelements:
                fe_refedge_nodes = []
                for node in self.fem_element_table[elem]:
                    if node in refedge_nodes:
                        fe_refedge_nodes.append(node)
                    edge_table[elem] = fe_refedge_nodes  # { faceID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
            #  FIXME duplicate_mesh_elements: as soon as contact ans springs are supported the user should decide on which edge the load is applied
            edge_table = delete_duplicate_mesh_elements(edge_table)
        elif is_beam_mesh(self.mesh_object.FemMesh):
            refedge_fem_edgeelements = getFemElementsByNodes(self.fem_element_table, refedge_nodes)
            for elem in refedge_fem_edgeelements:
                edge_table[elem] = self.fem_element_table[elem]  # { edgeID : ( nodeID, ... , nodeID  )} # all nodes off this femedgeelement
        return edge_table

    def get_ref_face_node_table(self, ref_face):
        face_table = {}  # { meshfaceID : ( nodeID, ... , nodeID ) }
        if is_solid_mesh(self.mesh_object.FemMesh):
            if has_no_face_data(self.mesh_object.FemMesh):
                # there is no face data, the volumeID is used as key { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
                ref_face_volume_elements = self.mesh_object.FemMesh.getccxVolumesByFace(ref_face)  # list of tupels (mv, ccx_face_nr)
                ref_face_nodes = self.mesh_object.FemMesh.getNodesByFace(ref_face)
                for ve in ref_face_volume_elements:
                    veID = ve[0]
                    ve_ref_face_nodes = []
                    for nodeID in self.fem_element_table[veID]:
                        if nodeID in ref_face_nodes:
                            ve_ref_face_nodes.append(nodeID)
                    face_table[veID] = ve_ref_face_nodes  # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
            else:  # the femmesh has face_data
                volume_faces = self.mesh_object.FemMesh.getVolumesByFace(ref_face)   # (mv, mf)
                for mv, mf in volume_faces:
                    face_table[mf] = self.mesh_object.FemMesh.getElementNodes(mf)
        elif is_shell_mesh(self.mesh_object.FemMesh):
            ref_face_nodes = self.mesh_object.FemMesh.getNodesByFace(ref_face)
            ref_face_elements = getFemElementsByNodes(self.fem_element_table, ref_face_nodes)
            for mf in ref_face_elements:
                face_table[mf] = self.fem_element_table[mf]
        return face_table

    def get_refedge_node_lengths(self, edge_table):
        # calulate the appropriate node_length for every node of every mesh edge (me)
        # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff

        #  [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry
        node_length_table = []
        mesh_edge_length = 0
        if not self.fem_mesh_nodes:
            self.fem_mesh_nodes = self.mesh_object.FemMesh.Nodes
        # print(len(edge_table))
        for me in edge_table:
            if len(edge_table[me]) == 2:  # 2 node mesh edge
                # end_node_length = mesh_edge_length / 2
                #    ______
                #  P1      P2
                P1 = self.fem_mesh_nodes[edge_table[me][0]]
                P2 = self.fem_mesh_nodes[edge_table[me][1]]
                edge_vec = P2 - P1
                mesh_edge_length = edge_vec.Length
                # print(mesh_edge_length)
                end_node_length = mesh_edge_length / 2.0
                node_length_table.append((edge_table[me][0], end_node_length))
                node_length_table.append((edge_table[me][1], end_node_length))

            elif len(edge_table[me]) == 3:  # 3 node mesh edge
                # end_node_length = mesh_edge_length / 6
                # middle_node_length = mesh_face_area * 2 / 3
                #   _______ _______
                # P1       P3      P2
                P1 = self.fem_mesh_nodes[edge_table[me][0]]
                P2 = self.fem_mesh_nodes[edge_table[me][1]]
                P3 = self.fem_mesh_nodes[edge_table[me][2]]
                edge_vec1 = P3 - P1
                edge_vec2 = P2 - P3
                mesh_edge_length = edge_vec1.Length + edge_vec2.Length
                # print(me, ' --> ', mesh_edge_length)
                end_node_length = mesh_edge_length / 6.0
                middle_node_length = mesh_edge_length * 2.0 / 3.0
                node_length_table.append((edge_table[me][0], end_node_length))
                node_length_table.append((edge_table[me][1], end_node_length))
                node_length_table.append((edge_table[me][2], middle_node_length))
        return node_length_table

    def get_ref_face_node_areas(self, face_table):
        # calulate the appropriate node_areas for every node of every mesh face (mf)
        # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff
        # FIXME only gives exact results in case of a real triangle. If for S6 or C3D10 elements
        # the midnodes are not on the line between the end nodes the area will not be a triangle
        # see http://forum.freecadweb.org/viewtopic.php?f=18&t=10939&start=40#p91355  and ff

        #  [ (nodeID,Area), ... , (nodeID,Area) ]  some nodes will have more than one entry
        node_area_table = []
        mesh_face_area = 0
        if not self.fem_mesh_nodes:
            self.fem_mesh_nodes = self.mesh_object.FemMesh.Nodes
        for mf in face_table:
            if len(face_table[mf]) == 3:  # 3 node mesh face triangle
                # corner_node_area = mesh_face_area / 3.0
                #      P3
                #      /\
                #     /  \
                #    /____\
                #  P1      P2
                P1 = self.fem_mesh_nodes[face_table[mf][0]]
                P2 = self.fem_mesh_nodes[face_table[mf][1]]
                P3 = self.fem_mesh_nodes[face_table[mf][2]]

                mesh_face_area = getTriangleArea(P1, P2, P3)
                corner_node_area = mesh_face_area / 3.0

                node_area_table.append((face_table[mf][0], corner_node_area))
                node_area_table.append((face_table[mf][1], corner_node_area))
                node_area_table.append((face_table[mf][2], corner_node_area))

            elif len(face_table[mf]) == 4:  # 4 node mesh face quad
                FreeCAD.Console.PrintError('Face load on 4 node quad faces are not supported\n')

            elif len(face_table[mf]) == 6:  # 6 node mesh face triangle
                # corner_node_area = 0
                # middle_node_area = mesh_face_area / 3.0
                #         P3
                #         /\
                #        /t3\
                #       /    \
                #     P6------P5
                #     / \ t4 / \
                #    /t1 \  /t2 \
                #   /_____\/_____\
                # P1      P4      P2
                P1 = self.fem_mesh_nodes[face_table[mf][0]]
                P2 = self.fem_mesh_nodes[face_table[mf][1]]
                P3 = self.fem_mesh_nodes[face_table[mf][2]]
                P4 = self.fem_mesh_nodes[face_table[mf][3]]
                P5 = self.fem_mesh_nodes[face_table[mf][4]]
                P6 = self.fem_mesh_nodes[face_table[mf][5]]

                mesh_face_t1_area = getTriangleArea(P1, P4, P6)
                mesh_face_t2_area = getTriangleArea(P2, P5, P4)
                mesh_face_t3_area = getTriangleArea(P3, P6, P5)
                mesh_face_t4_area = getTriangleArea(P4, P5, P6)
                mesh_face_area = mesh_face_t1_area + mesh_face_t2_area + mesh_face_t3_area + mesh_face_t4_area
                middle_node_area = mesh_face_area / 3.0

                node_area_table.append((face_table[mf][0], 0))
                node_area_table.append((face_table[mf][1], 0))
                node_area_table.append((face_table[mf][2], 0))
                node_area_table.append((face_table[mf][3], middle_node_area))
                node_area_table.append((face_table[mf][4], middle_node_area))
                node_area_table.append((face_table[mf][5], middle_node_area))

            elif len(face_table[mf]) == 8:  # 8 node mesh face quad
                FreeCAD.Console.PrintError('Face load on 8 node quad faces are not supported\n')
        return node_area_table

    def get_ref_shape_node_sum_geom_table(self, node_geom_table):
        # shape could be Edge or Face, geom could be lenght or area
        # summ of legth or area for each node of the ref_shape
        node_sum_geom_table = {}
        for n, A in node_geom_table:
            # print(n, ' --> ', A)
            if n in node_sum_geom_table:
                node_sum_geom_table[n] = node_sum_geom_table[n] + A
            else:
                node_sum_geom_table[n] = A
        return node_sum_geom_table


# Helpers
def getTriangleArea(P1, P2, P3):
    vec1 = P2 - P1
    vec2 = P3 - P1
    vec3 = vec1.cross(vec2)
    return 0.5 * vec3.Length


def getFemElementTable(fem_mesh):
    """ getFemElementTable(fem_mesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    fem_element_table = {}
    if is_solid_mesh(fem_mesh):
        for i in fem_mesh.Volumes:
            fem_element_table[i] = fem_mesh.getElementNodes(i)
    elif is_shell_mesh(fem_mesh):
        for i in fem_mesh.Faces:
            fem_element_table[i] = fem_mesh.getElementNodes(i)
    elif is_beam_mesh(fem_mesh):
        for i in fem_mesh.Edges:
            fem_element_table[i] = fem_mesh.getElementNodes(i)
    else:
        FreeCAD.Console.PrintError('Neither solid nor shell nor beam mesh!\n')
    return fem_element_table


def getFemElementsByNodes(fem_element_table, node_list):
    '''for every fem_element of fem_element_table
    if all nodes of the fem_element are in node_list,
    the fem_element is added to the list which is returned
    e: elementlist
    nodes: nodelist '''
    e = []  # elementlist
    for elementID in sorted(fem_element_table):
        nodecount = 0
        for nodeID in fem_element_table[elementID]:
            if nodeID in node_list:
                nodecount = nodecount + 1
        if nodecount == len(fem_element_table[elementID]):   # all nodes of the element are in the node_list!
            e.append(elementID)
    return e


def is_solid_mesh(fem_mesh):
    if fem_mesh.VolumeCount > 0:  # solid mesh
        return True


def has_no_face_data(fem_mesh):
    if fem_mesh.FaceCount == 0:   # mesh has no face data, could be a beam mesh or a solid mesh without face data
        return True


def is_shell_mesh(fem_mesh):
    if fem_mesh.VolumeCount == 0 and fem_mesh.FaceCount > 0:  # shell mesh
        return True


def is_beam_mesh(fem_mesh):
    if fem_mesh.VolumeCount == 0 and fem_mesh.FaceCount == 0 and fem_mesh.EdgeCount > 0:  # beam mesh
        return True


def femelements_count_ok(fem_element_table, count_femelements):
    if count_femelements == len(fem_element_table):
        # print('Count Elements written to CalculiX file: ', count_femelements)
        # print('Count Elements of the FreeCAD FEM Mesh:  ', len(fem_element_table))
        return True
    else:
        print('ERROR: self.fem_element_table != count_femelements')
        print('Count Elements written to CalculiX file: ', count_femelements)
        print('Count Elements of the FreeCAD FEM Mesh:  ', len(fem_element_table))
        return False


def delete_duplicate_mesh_elements(refelement_table):
    new_refelement_table = {}  # duplicates deleted
    for elem, nodes in refelement_table.items():
        if sorted(nodes) not in sortlistoflistvalues(new_refelement_table.values()):
            new_refelement_table[elem] = nodes
    return new_refelement_table


def sortlistoflistvalues(listoflists):
    new_list = []
    for l in listoflists:
        new_list.append(sorted(l))
    return new_list


def get_ccx_elset_beam_name(mat_name, beamsec_name, mat_short_name=None, beamsec_short_name=None):
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if not beamsec_short_name:
        beamsec_short_name = 'Beam0'
    if len(mat_name + beamsec_name) > 20:   # max identifier lenght in CalculiX for beam elsets
        return mat_short_name + beamsec_short_name
    else:
        return mat_name + beamsec_name


def get_ccx_elset_shell_name(mat_name, shellth_name, mat_short_name=None, shellth_short_name=None):
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if not shellth_short_name:
        shellth_short_name = 'Shell0'
    if len(mat_name + shellth_name) > 80:   # standard max identifier lenght in CalculiX
        return mat_short_name + shellth_short_name
    else:
        return mat_name + shellth_name


def get_ccx_elset_solid_name(mat_name, solid_name=None, mat_short_name=None):
    if not solid_name:
        solid_name = 'Solid'
    if not mat_short_name:
        mat_short_name = 'Mat0'
    if len(mat_name + solid_name) > 80:   # standard max identifier lenght in CalculiX
        return mat_short_name + solid_name
    else:
        return mat_name + solid_name


def get_ccx_elset_short_name(obj, i):
    if hasattr(obj, "Proxy") and obj.Proxy.Type == 'MechanicalMaterial':
        return 'Mat' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'FemBeamSection':
        return 'Beam' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'FemShellThickness':
        return 'Shell' + str(i)
    else:
        print('Error: ', obj.Name, ' --> ', obj.Proxy.Type)
