import FreeCAD
import os
import sys
import time


class inp_writer:
    def __init__(self, analysis_obj, mesh_obj, mat_obj, fixed_obj, force_obj,
                 pressure_obj, analysis_type=None, eigenmode_parameters=None, dir_name=None):
        self.dir_name = dir_name
        self.analysis = analysis_obj
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.force_objects = force_obj
        self.pressure_objects = pressure_obj
        if eigenmode_parameters:
            self.no_of_eigenfrequencies = eigenmode_parameters[0]
            self.eigenfrequeny_range_low = eigenmode_parameters[1]
            self.eigenfrequeny_range_high = eigenmode_parameters[2]
        self.analysis_type = analysis_type
        if not dir_name:
            self.dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + analysis_obj.Uid[-4:]
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)
        self.base_name = self.mesh_object.Name
        self.file_name = self.dir_name + '/' + self.base_name + '.inp'
        self.fc_ver = FreeCAD.Version()

    def write_calculix_input_file(self):
        self.mesh_object.FemMesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')
        self.write_element_sets_material_and_femelement_type(inpfile)
        self.write_node_sets_constraints_fixed(inpfile)
        self.write_node_sets_constraints_force(inpfile)
        self.write_materials(inpfile)
        self.write_step_begin(inpfile)
        self.write_constraints_fixed(inpfile)
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
        for m in self.material_objects:
            if len(self.material_objects) == 1:
                f.write('*ELSET,ELSET=MaterialSolidElements\n')
                f.write('Eall\n')
            else:
                print 'material object count: ', len(self.material_objects)
                FreeCAD.Console.PrintError('Multiple materials are not yet supported!\n')

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

    def write_node_sets_constraints_force(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node sets for loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            f.write('*NSET,NSET=' + frc_obj.Name + '\n')
            NbrForceNodes = 0
            for o, elem in frc_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Edge':
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')
                    NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from force_object
            # calculate node load
            if NbrForceNodes != 0:
                fobj['NodeLoad'] = (frc_obj.Force) / NbrForceNodes
                #  FIXME for loads on edges the node count is used to distribute the load on the edges.
                #  In case of a not uniform fem mesh this could result in wrong force distribution
                #  and thus in wrong analysis results. see  def write_constraints_force()
                f.write('** concentrated load [N] distributed on all mesh nodes of the given shapes\n')
                f.write('** ' + str(frc_obj.Force) + ' N / ' + str(NbrForceNodes) + ' Nodes = ' + str(fobj['NodeLoad']) + ' N on each node\n')
            if frc_obj.Force == 0:
                print '  Warning --> Force = 0'

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
            mat_obj_name = mat_obj.Name
            mat_name = mat_obj.Material['Name'][:80]
            # write material properties
            f.write('*MATERIAL, NAME=' + mat_name + '\n')
            f.write('*ELASTIC \n')
            f.write('{}, \n'.format(YM_in_MPa))
            f.write('{0:.3f}\n'.format(PR))
            density = FreeCAD.Units.Quantity(mat_obj.Material['Density'])
            density_in_tone_per_mm3 = float(density.getValueAs('t/mm^3'))
            f.write('*DENSITY \n')
            f.write('{0:.3e}, \n'.format(density_in_tone_per_mm3))
            # write element properties
            if len(self.material_objects) == 1:
                f.write('*SOLID SECTION, ELSET=MaterialSolidElements, MATERIAL=' + mat_name + '\n')
            else:
                if mat_obj_name == 'MechanicalMaterial':
                    f.write('*SOLID SECTION, ELSET=MaterialSolidElements, MATERIAL=' + mat_name + '\n')

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
            f.write(fix_obj_name + ',3\n\n')

    def write_constraints_force(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        if is_shell_mesh(self.mesh_object.FemMesh) or (is_solid_mesh(self.mesh_object.FemMesh) and has_no_face_data(self.mesh_object.FemMesh)):
            if not hasattr(self, 'fem_element_table'):
                self.fem_element_table = getFemElementTable(self.mesh_object.FemMesh)
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            if 'NodeLoad' in fobj:   # load on edges or vertieces
                node_load = fobj['NodeLoad']
                frc_obj_name = frc_obj.Name
                vec = frc_obj.DirectionVector
                f.write('*CLOAD\n')
                f.write('** force: ' + str(node_load) + ' N,  direction: ' + str(vec) + '\n')
                v1 = "{:.13E}".format(vec.x * node_load)
                v2 = "{:.13E}".format(vec.y * node_load)
                v3 = "{:.13E}".format(vec.z * node_load)
                f.write(frc_obj_name + ',1,' + v1 + '\n')
                f.write(frc_obj_name + ',2,' + v2 + '\n')
                f.write(frc_obj_name + ',3,' + v3 + '\n\n')

            # area load on faces
            sum_ref_face_area = 0
            sum_ref_face_node_area = 0
            sum_node_load = 0
            for o, elem in frc_obj.References:
                elem_o = o.Shape.getElement(elem)
                if elem_o.ShapeType == 'Face':
                    sum_ref_face_area += elem_o.Area
            if sum_ref_face_area != 0:
                force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area

            for o, elem in frc_obj.References:
                elem_o = o.Shape.getElement(elem)
                if elem_o.ShapeType == 'Face':
                    ref_face = elem_o
                    f.write('** ' + frc_obj.Name + '\n')
                    f.write('*CLOAD\n')
                    f.write('** node loads on element face: ' + o.Name + '.' + elem + '\n')

                    face_table = {}  # { meshfaceID : ( nodeID, ... , nodeID ) }
                    if is_solid_mesh(self.mesh_object.FemMesh):
                        if has_no_face_data(self.mesh_object.FemMesh):
                            ref_face_volume_elements = self.mesh_object.FemMesh.getccxVolumesByFace(ref_face)  # list of tupels
                            ref_face_nodes = self.mesh_object.FemMesh.getNodesByFace(ref_face)
                            for ve in ref_face_volume_elements:
                                veID = ve[0]
                                ve_ref_face_nodes = []
                                for nodeID in self.fem_element_table[veID]:
                                    if nodeID in ref_face_nodes:
                                        ve_ref_face_nodes.append(nodeID)
                                face_table[veID] = ve_ref_face_nodes  # { volumeID : ( facenodeID, ... , facenodeID ) }
                        else:
                            volume_faces = self.mesh_object.FemMesh.getVolumesByFace(ref_face)   # (mv, mf)
                            for mv, mf in volume_faces:
                                face_table[mf] = self.mesh_object.FemMesh.getElementNodes(mf)
                    elif is_shell_mesh(self.mesh_object.FemMesh):
                        ref_face_nodes = self.mesh_object.FemMesh.getNodesByFace(ref_face)
                        ref_face_elements = getFemElementsByNodes(self.fem_element_table, ref_face_nodes)
                        for mf in ref_face_elements:
                            face_table[mf] = self.fem_element_table[mf]

                    # calulate the appropriate node_areas for every node of every mesh face (mf)
                    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff
                    # FIXME only gives exact results in case of a real triangle. If for S6 or C3D10 elements
                    # the midnodes are not on the line between the end nodes the area will not be a triangle
                    # see http://forum.freecadweb.org/viewtopic.php?f=18&t=10939&start=40#p91355  and ff

                    #  [ (nodeID,Area), ... , (nodeID,Area) ]  some nodes will have more than one entry
                    node_area_table = []
                    #  { nodeID : Area, ... , nodeID:Area }  AreaSum for each node, one entry for each node
                    node_sumarea_table = {}
                    mesh_face_area = 0
                    for mf in face_table:
                        if len(face_table[mf]) == 3:  # 3 node mesh face triangle
                            # corner_node_area = mesh_face_area / 3.0
                            #      P3
                            #      /\
                            #     /  \
                            #    /____\
                            #  P1      P2
                            P1 = self.mesh_object.FemMesh.Nodes[face_table[mf][0]]
                            P2 = self.mesh_object.FemMesh.Nodes[face_table[mf][1]]
                            P3 = self.mesh_object.FemMesh.Nodes[face_table[mf][2]]

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
                            P1 = self.mesh_object.FemMesh.Nodes[face_table[mf][0]]
                            P2 = self.mesh_object.FemMesh.Nodes[face_table[mf][1]]
                            P3 = self.mesh_object.FemMesh.Nodes[face_table[mf][2]]
                            P4 = self.mesh_object.FemMesh.Nodes[face_table[mf][3]]
                            P5 = self.mesh_object.FemMesh.Nodes[face_table[mf][4]]
                            P6 = self.mesh_object.FemMesh.Nodes[face_table[mf][5]]

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

                    # node_sumarea_table
                    for n, A in node_area_table:
                        # print n, ' --> ', A
                        if n in node_sumarea_table:
                            node_sumarea_table[n] = node_sumarea_table[n] + A
                        else:
                            node_sumarea_table[n] = A

                    sum_node_areas = 0
                    for n in node_sumarea_table:
                        sum_node_areas = sum_node_areas + node_sumarea_table[n]
                    sum_ref_face_node_area += sum_node_areas

                    # write CLOAD lines to CalculiX file
                    vec = frc_obj.DirectionVector
                    for n in sorted(node_sumarea_table):
                        node_load = node_sumarea_table[n] * force_per_sum_ref_face_area
                        sum_node_load += node_load
                        if (vec.x != 0.0):
                            v1 = "{:.13E}".format(vec.x * node_load)
                            f.write(str(n) + ',1,' + v1 + '\n')
                        if (vec.y != 0.0):
                            v2 = "{:.13E}".format(vec.y * node_load)
                            f.write(str(n) + ',2,' + v2 + '\n')
                        if (vec.z != 0.0):
                            v3 = "{:.13E}".format(vec.z * node_load)
                            f.write(str(n) + ',3,' + v3 + '\n')
                f.write('\n')
            f.write('\n')

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
    '''if all nodes of an fem_element are in node_list,
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
