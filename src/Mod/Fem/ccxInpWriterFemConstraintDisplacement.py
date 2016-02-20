import FreeCAD
import os

class ccxInpWriterFemConstraintDisplacement:
    def __init__(self):
        displacement_constraints = []
        beam_sections = []
        shell_thicknesses = []
        f_mesh = None
        dir_name=None
        file_name=None
        
    def writeFile(self):
        print('started FemConstraintDisplacementInpFile')
        import FemGui
        f_analysis = FemGui.getActiveAnalysis()
        displacement_constraints = []
        beam_sections = []
        shell_thicknesses = []
        f_mesh = None
        dir_name=None
        file_name=None
        for m in f_analysis.Member:
            if m.isDerivedFrom("Fem::ConstraintDisplacement"): 
                        displacement_constraint_dict = {}
                        displacement_constraint_dict['Object'] = m
                        displacement_constraints.append(displacement_constraint_dict)
            elif m.isDerivedFrom("Fem::FemMeshObject"):
                f_mesh=m
            elif hasattr(m, "Proxy") and m.Proxy.Type == 'FemBeamSection':
                beam_section_dict = {}
                beam_section_dict['Object'] = m
                beam_sections.append(beam_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemShellThickness":
                shell_thickness_dict = {}
                shell_thickness_dict['Object'] = m
                shell_thicknesses.append(shell_thickness_dict)
        dir_name = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemConstraints'
        if not os.path.isdir(dir_name):
            os.mkdir(dir_name)
        file_name = dir_name + '/' + 'FemConstraintDisplacement.txt'
        print(file_name)
        inpfile = open(file_name, 'w')
        inpfile.write('\n***********************************************************\n')
        inpfile.write('** Node sets for prescribed displacement constraint\n')
        #inpfile.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for fobj in displacement_constraints:
            disp_obj = fobj['Object']
            inpfile.write('*NSET,NSET='+disp_obj.Name + '\n')
            for o, elem in disp_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Face':
                    n = f_mesh.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    n = f_mesh.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    n = f_mesh.FemMesh.getNodesByVertex(fo)
                for i in n:
                    inpfile.write(str(i) + ',\n')
        inpfile.write('\n***********************************************************\n')
        inpfile.write('** Displacement constraint applied\n')
        #f.write('** written by {} function\n'.format(sys._getframe().f_code.co_name))
        for disp_obj in displacement_constraints:
            disp_obj_name = disp_obj['Object'].Name
            inpfile.write('*BOUNDARY\n')
            if disp_obj['Object'].xFix == True:
                inpfile.write(disp_obj_name + ',1\n')
            elif disp_obj['Object'].xFree == False:
                inpfile.write(disp_obj_name + ',1,1,'+str(disp_obj['Object'].xDisplacement)+'\n')
            if disp_obj['Object'].yFix == True:
                inpfile.write(disp_obj_name + ',2\n')
            elif disp_obj['Object'].yFree == False:
                inpfile.write(disp_obj_name + ',2,2,'+str(disp_obj['Object'].yDisplacement)+'\n')
            if disp_obj['Object'].zFix == True:
                inpfile.write(disp_obj_name + ',3\n')
            elif disp_obj['Object'].zFree == False:
                inpfile.write(disp_obj_name + ',3,3,'+str(disp_obj['Object'].zDisplacement)+'\n')

            if beam_sections or shell_thicknesses:
                if disp_obj['Object'].rotxFix == True:
                    inpfile.write(disp_obj_name + ',4\n')
                elif disp_obj['Object'].rotxFree == False:
                    inpfile.write(disp_obj_name + ',4,4,'+str(disp_obj['Object'].xRotation)+'\n')
                if disp_obj['Object'].rotyFix == True:
                    inpfile.write(disp_obj_name + ',5\n')
                elif disp_obj['Object'].rotyFree == False:
                    inpfile.write(disp_obj_name + ',5,5,'+str(disp_obj['Object'].yRotation)+'\n')
                if disp_obj['Object'].rotzFix == True:
                    inpfile.write(disp_obj_name + ',6\n')
                elif disp_obj['Object'].rotzFree == False:
                    inpfile.write(disp_obj_name + ',6,6,'+str(disp_obj['Object'].zRotation)+'\n')
        inpfile.close()
        print('completed FemConstraintDisplacementInpFile')
