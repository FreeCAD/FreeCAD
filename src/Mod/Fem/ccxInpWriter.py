import FreeCAD
import os
import sys


class inp_writer:
    def __init__(self, analysis_obj, mesh_obj, mat_obj, fixed_obj, force_obj, pressure_obj, dir_name=None):
        self.dir_name = dir_name
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.force_objects = force_obj
        self.pressure_objects = pressure_obj
        if not dir_name:
            self.dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + analysis_obj.Uid[-4:]
        if not os.path.isdir(self.dir_name):
            os.mkdir(self.dir_name)
        self.base_name = self.dir_name + '/' + self.mesh_object.Name
        self.file_name = self.base_name + '.inp'

    def write_calculix_input_file(self):
        self.mesh_object.FemMesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')
        self.write_material_element_sets(inpfile)
        self.write_fixed_node_sets(inpfile)
        self.write_load_node_sets(inpfile)
        self.write_materials(inpfile)
        self.write_step_begin(inpfile)
        self.write_constraints_fixed(inpfile)
        self.write_constraints_force(inpfile)
        self.write_face_load(inpfile)
        self.write_outputs_types(inpfile)
        self.write_step_end(inpfile)
        self.write_footer(inpfile)
        inpfile.close()
        return self.base_name

    def write_material_element_sets(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Element sets for materials\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for m in self.material_objects:
            mat_obj = m['Object']
            mat_obj_name = mat_obj.Name[:80]

            f.write('*ELSET,ELSET=' + mat_obj_name + '\n')
            if len(self.material_objects) == 1:
                f.write('Eall\n')
            else:
                if mat_obj_name == 'MechanicalMaterial':
                    f.write('Eall\n')

    def write_fixed_node_sets(self, f):
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

    def write_load_node_sets(self, f):
        f.write('\n***********************************************************\n')
        f.write('** Node sets for loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            print frc_obj.Name
            f.write('*NSET,NSET=' + frc_obj.Name + '\n')
            NbrForceNodes = 0
            for o, elem in frc_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Edge':
                    print '  Line Load (edge load) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Load (vertex load) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')
                    NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from force_object
            # calculate node load
            if NbrForceNodes == 0:
                print 'No Line Loads or Point Loads in the model'
            else:
                fobj['NodeLoad'] = (frc_obj.Force) / NbrForceNodes
                #  FIXME this method is incorrect, but we don't have anything else right now
                #  Please refer to thread "CLOAD and DLOAD for the detailed description
                #  http://forum.freecadweb.org/viewtopic.php?f=18&t=10692
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
            f.write('{}, '.format(YM_in_MPa))
            f.write('{0:.3f}\n'.format(PR))
            # write element properties
            if len(self.material_objects) == 1:
                f.write('*SOLID SECTION, ELSET=' + mat_obj_name + ', MATERIAL=' + mat_name + '\n')
            else:
                if mat_obj_name == 'MechanicalMaterial':
                    f.write('*SOLID SECTION, ELSET=' + mat_obj_name + ', MATERIAL=' + mat_name + '\n')

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

        def getTriangleArea(P1, P2, P3):
            vec1 = P2 - P1
            vec2 = P3 - P1
            vec3 = vec1.cross(vec2)
            return 0.5 * vec3.Length

        f.write('\n***********************************************************\n')
        f.write('** Node loads\n')
        f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            if 'NodeLoad' in fobj:
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

            # area load on faces of volume elements --> CLOAD is used
            sum_ref_face_area = 0
            sum_ref_face_node_area = 0
            sum_node_load = 0
            for o, elem in frc_obj.References:
                elem_o = o.Shape.getElement(elem)
                if elem_o.ShapeType == 'Face':
                    sum_ref_face_area += elem_o.Area
            if sum_ref_face_area != 0:
                print frc_obj.Name, ', AreaLoad on faces, CLOAD is used'
                force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area
                print '  force_per_sum_ref_face_area: ', force_per_sum_ref_face_area

            for o, elem in frc_obj.References:
                elem_o = o.Shape.getElement(elem)
                if elem_o.ShapeType == 'Face':
                    ref_face = elem_o
                    print '  ', o.Name, '.', elem,
                    f.write('** ' + frc_obj.Name + '\n')
                    f.write('*CLOAD\n')
                    f.write('** node loads on element face: ' + o.Name + '.' + elem + '\n')

                    volume_faces = self.mesh_object.FemMesh.getVolumesByFace(ref_face)
                    face_table = {}  # { meshfaceID : ( nodeID, ... , nodeID ) }
                    for mv, mf in volume_faces:
                        face_table[mf] = self.mesh_object.FemMesh.getElementNodes(mf)

                    # calulate the appropriate node_areas for every node of every mesh face (mf)
                    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff

                    #  [ (nodeID,Area), ... , (nodeID,Area) ]  some nodes will have more than one entry
                    node_area_table = []
                    #  { nodeID : Area, ... , nodeID:Area }  AreaSum for each node, one entry for each node
                    node_sumarea_table = {}
                    mesh_face_area = 0
                    for mf in face_table:
                        # print '    ', mf, ' --> ', face_table[mf]
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

                        if len(face_table[mf]) == 6:  # 6 node mesh face triangle
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

                    # node_sumarea_table
                    for n, A in node_area_table:
                        # print n, ' --> ', A
                        if n in node_sumarea_table:
                            node_sumarea_table[n] = node_sumarea_table[n] + A
                        else:
                            node_sumarea_table[n] = A

                    sum_node_areas = 0
                    for n in node_sumarea_table:
                        # print n, ' --> ', node_sumarea_table[n]
                        sum_node_areas = sum_node_areas + node_sumarea_table[n]
                    print '    sum_node_areas ', sum_node_areas, ' ref_face.Area: ', ref_face.Area
                    sum_ref_face_node_area += sum_node_areas

                    # write CLOAD lines to CalculiX file
                    vec = frc_obj.DirectionVector
                    for n in sorted(node_sumarea_table):
                        node_load = node_sumarea_table[n] * force_per_sum_ref_face_area
                        sum_node_load += node_load
                        #print '    nodeID: ', n, '  nodeload: ', node_load
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

            # print '  sum_ref_face_node_area: ', sum_ref_face_node_area
            # print '  sum_ref_face_area     : ', sum_ref_face_area
            # print '  sum_ref_face_node_area * force_per_sum_ref_face_area: ', sum_ref_face_node_area * force_per_sum_ref_face_area
            # print '  sum_node_load:                                        ', sum_node_load
            # print '  frc_obj.Force:                                        ', frc_obj.Force
            f.write('\n')

    def write_face_load(self, f):
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
        f.write('**\n')
        f.write('**   Units\n')
        f.write('**\n')
        f.write('**   Geometry (mesh data)        --> mm\n')
        f.write("**   Materials (Young's modulus) --> N/mm2 = MPa\n")
        f.write('**   Loads (nodal loads)         --> N\n')
        f.write('**\n')
