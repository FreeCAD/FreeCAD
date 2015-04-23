import FemGui
import FreeCAD
import os
import time


class inp_writer:
    def __init__(self, dir_name, mesh_obj, mat_obj, fixed_obj, force_obj):
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.force_objects = force_obj
        self.base_name = dir_name + '/' + self.mesh_object.Name
        self.file_name = self.base_name + '.inp'
        print 'CalculiX .inp file will be written to: ', self.file_name

    def write_calculix_input_file(self):
        print 'write_calculix_input_file'
        self.mesh_object.FemMesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')

        # write material element sets
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** element sets for materials\n')
        for material_object in self.material_objects:
            print material_object['Object'].Name, ':  ', material_object['Object'].Material['Name']
            inpfile.write('*ELSET,ELSET=' + material_object['Object'].Name + '\n')
            if len(self.material_objects) == 1:
                inpfile.write('Eall\n')
            else:
                if material_object['Object'].Name == 'MechanicalMaterial':
                    inpfile.write('Eall\n')
            inpfile.write('\n\n')

        # write fixed node sets
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** node set for fixed constraint\n')
        for fixed_object in self.fixed_objects:
            print fixed_object['Object'].Name
            inpfile.write('*NSET,NSET=' + fixed_object['Object'].Name + '\n')
            for o, f in fixed_object['Object'].References:
                fo = o.Shape.getElement(f)
                n = []
                if fo.ShapeType == 'Face':
                    print '  Face Support (fixed face) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Support (fixed edge) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Support (fixed vertex) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    inpfile.write(str(i) + ',\n')
            inpfile.write('\n\n')

        # write load node sets and calculate node loads
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** node sets for loads\n')
        for force_object in self.force_objects:
            print force_object['Object'].Name
            inpfile.write('*NSET,NSET=' + force_object['Object'].Name + '\n')
            NbrForceNodes = 0
            for o, f in force_object['Object'].References:
                fo = o.Shape.getElement(f)
                n = []
                if fo.ShapeType == 'Face':
                    print '  AreaLoad (face load) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Load (edge load) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Load (vertex load) on: ', f
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    inpfile.write(str(i) + ',\n')
                    NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from force_object
            # calculate node load
            if NbrForceNodes == 0:
                print '  Warning --> no FEM-Mesh-node to apply the load to was found?'
            else:
                force_object['NodeLoad'] = (force_object['Object'].Force) / NbrForceNodes
                inpfile.write('** concentrated load [N] distributed on all mesh nodes of the given shapes\n')
                inpfile.write('** ' + str(force_object['Object'].Force) + ' N / ' + str(NbrForceNodes) + ' Nodes = ' + str(force_object['NodeLoad']) + ' N on each node\n')
            if force_object['Object'].Force == 0:
                print '  Warning --> Force = 0'
            inpfile.write('\n\n')

        # write materials
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** materials\n')
        inpfile.write('** youngs modulus unit is MPa = N/mm2\n')
        for material_object in self.material_objects:
            # get material properties
            YM = FreeCAD.Units.Quantity(material_object['Object'].Material['YoungsModulus'])
            if YM.Unit.Type == '':
                print 'Material "YoungsModulus" has no Unit, asuming kPa!'
                YM = FreeCAD.Units.Quantity(YM.Value, FreeCAD.Units.Unit('Pa'))
            else:
                print 'YM unit: ', YM.Unit.Type
            print 'YM = ', YM
            PR = float(material_object['Object'].Material['PoissonRatio'])
            print 'PR = ', PR
            material_name = material_object['Object'].Material['Name'][:80]
            # write material properties
            inpfile.write('*MATERIAL, NAME=' + material_name + '\n')
            inpfile.write('*ELASTIC \n')
            inpfile.write('{0:.3f}, '.format(YM.Value * 1E-3))
            inpfile.write('{0:.3f}\n'.format(PR))
            # write element properties
            if len(self.material_objects) == 1:
                inpfile.write('*SOLID SECTION, ELSET=' + material_object['Object'].Name + ', MATERIAL=' + material_name + '\n\n')
            else:
                if material_object['Object'].Name == 'MechanicalMaterial':
                    inpfile.write('*SOLID SECTION, ELSET=' + material_object['Object'].Name + ', MATERIAL=' + material_name + '\n\n')

        # write step beginn
        inpfile.write('\n\n\n\n***********************************************************\n')
        inpfile.write('** one step is needed to calculate the mechanical analysis of FreeCAD\n')
        inpfile.write('** loads are applied quasi-static, means without involving the time dimension\n')
        inpfile.write('*STEP\n')
        inpfile.write('*STATIC\n\n')

        # write constaints
        inpfile.write('\n** constaints\n')
        for fixed_object in self.fixed_objects:
            inpfile.write('*BOUNDARY\n')
            inpfile.write(fixed_object['Object'].Name + ',1\n')
            inpfile.write(fixed_object['Object'].Name + ',2\n')
            inpfile.write(fixed_object['Object'].Name + ',3\n\n')

        # write loads
        #inpfile.write('*DLOAD\n')
        #inpfile.write('Eall,NEWTON\n')
        inpfile.write('\n** loads\n')
        inpfile.write('** node loads, see load node sets for how the value is calculated!\n')
        for force_object in self.force_objects:
            if 'NodeLoad' in force_object:
                vec = force_object['Object'].DirectionVector
                inpfile.write('*CLOAD\n')
                inpfile.write('** force: ' + str(force_object['NodeLoad']) + ' N,  direction: ' + str(vec) + '\n')
                v1 = "{:.15}".format(repr(vec.x * force_object['NodeLoad']))
                v2 = "{:.15}".format(repr(vec.y * force_object['NodeLoad']))
                v3 = "{:.15}".format(repr(vec.z * force_object['NodeLoad']))
                inpfile.write(force_object['Object'].Name + ',1,' + v1 + '\n')
                inpfile.write(force_object['Object'].Name + ',2,' + v2 + '\n')
                inpfile.write(force_object['Object'].Name + ',3,' + v3 + '\n\n')

        # write outputs, both are needed by FreeCAD
        inpfile.write('\n** outputs --> frd file\n')
        inpfile.write('*NODE FILE\n')
        inpfile.write('U\n')
        inpfile.write('*EL FILE\n')
        inpfile.write('S, E\n')
        inpfile.write('** outputs --> dat file\n')
        inpfile.write('*NODE PRINT , NSET=Nall \n')
        inpfile.write('U \n')
        inpfile.write('*EL PRINT , ELSET=Eall \n')
        inpfile.write('S \n')
        inpfile.write('\n\n')

        # write step end
        inpfile.write('*END STEP \n')

        # write some informations
        FcVersionInfo = FreeCAD.Version()
        inpfile.write('\n\n\n\n***********************************************************\n')
        inpfile.write('**\n')
        inpfile.write('**   CalculiX Inputfile\n')
        inpfile.write('**\n')
        inpfile.write('**   written by    --> FreeCAD ' + FcVersionInfo[0] + '.' + FcVersionInfo[1] + '.' + FcVersionInfo[2] + '\n')
        inpfile.write('**   written on    --> ' + time.ctime() + '\n')
        inpfile.write('**   file name     --> ' + os.path.basename(FreeCAD.ActiveDocument.FileName) + '\n')
        inpfile.write('**   analysis name --> ' + FemGui.getActiveAnalysis().Name + '\n')
        inpfile.write('**\n')
        inpfile.write('**\n')
        inpfile.write('**   Units\n')
        inpfile.write('**\n')
        inpfile.write('**   Geometry (mesh data)        --> mm\n')
        inpfile.write("**   Materials (young's modulus) --> N/mm2 = MPa\n")
        inpfile.write('**   Loads (nodal loads)         --> N\n')
        inpfile.write('**\n')
        inpfile.write('**\n')

        inpfile.close()
        return self.base_name
