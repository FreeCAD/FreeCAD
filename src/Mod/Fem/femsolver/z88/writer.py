# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import time
import femmesh.meshtools as FemMeshTools
import feminout.importZ88Mesh as importZ88Mesh
from .. import writerbase as FemInputWriter


class FemInputWriterZ88(FemInputWriter.FemInputWriter):
    def __init__(self,
                 analysis_obj,
                 solver_obj,
                 mesh_obj,
                 matlin_obj,
                 matnonlin_obj,
                 fixed_obj,
                 displacement_obj,
                 contact_obj,
                 planerotation_obj,
                 transform_obj,
                 selfweight_obj,
                 force_obj,
                 pressure_obj,
                 temperature_obj,
                 heatflux_obj,
                 initialtemperature_obj,
                 beamsection_obj,
                 beamrotation_obj,
                 shellthickness_obj,
                 fluidsection_obj,
                 dir_name=None
                 ):

        FemInputWriter.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            matlin_obj,
            matnonlin_obj,
            fixed_obj,
            displacement_obj,
            contact_obj,
            planerotation_obj,
            transform_obj,
            selfweight_obj,
            force_obj,
            pressure_obj,
            temperature_obj,
            heatflux_obj,
            initialtemperature_obj,
            beamsection_obj,
            beamrotation_obj,
            shellthickness_obj,
            fluidsection_obj,
            dir_name)
        from os.path import join
        self.file_name = join(self.dir_name, 'z88')
        FreeCAD.Console.PrintLog('FemInputWriterZ88 --> self.dir_name  -->  ' + self.dir_name + '\n')
        FreeCAD.Console.PrintMessage('FemInputWriterZ88 --> self.file_name  -->  ' + self.file_name + '\n')

    def write_z88_input(self):
        timestart = time.clock()
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
            self.element_count = len(self.femelement_table)
        self.set_z88_elparam()
        self.write_z88_mesh()
        self.write_z88_contraints()
        self.write_z88_face_loads()
        self.write_z88_materials()
        self.write_z88_elements_properties()
        self.write_z88_integration_properties()
        self.write_z88_memory_parameter()
        self.write_z88_solver_parameter()
        FreeCAD.Console.PrintMessage("Writing time input file: " + str(time.clock() - timestart) + ' \n\n')
        return self.dir_name

    def set_z88_elparam(self):
        # TODO: param should be moved to the solver object like the known analysis
        z8804 = {'INTORD': '0', 'INTOS': '0', 'IHFLAG': '0', 'ISFLAG': '1'}  # seg2 --> stab4
        z8824 = {'INTORD': '7', 'INTOS': '7', 'IHFLAG': '1', 'ISFLAG': '1'}  # tria6 --> schale24
        z8823 = {'INTORD': '3', 'INTOS': '0', 'IHFLAG': '1', 'ISFLAG': '0'}  # quad8 --> schale23
        z8817 = {'INTORD': '4', 'INTOS': '0', 'IHFLAG': '0', 'ISFLAG': '0'}  # tetra4 --> volume17
        z8816 = {'INTORD': '4', 'INTOS': '0', 'IHFLAG': '0', 'ISFLAG': '0'}  # tetra10 --> volume16
        z8801 = {'INTORD': '2', 'INTOS': '2', 'IHFLAG': '0', 'ISFLAG': '1'}  # hexa8 --> volume1
        z8810 = {'INTORD': '3', 'INTOS': '0', 'IHFLAG': '0', 'ISFLAG': '0'}  # hexa20 --> volume10
        param = {4: z8804, 24: z8824, 23: z8823, 17: z8817, 16: z8816, 1: z8801, 10: z8810}
        # elemente 17, 16, 10, INTORD etc ... testen !!!
        self.z88_element_type = importZ88Mesh.get_z88_element_type(self.femmesh, self.femelement_table)
        if self.z88_element_type in param:
            self.z88_elparam = param[self.z88_element_type]
        else:
            raise Exception('Element type not supported by Z88.')
        FreeCAD.Console.PrintMessage(self.z88_elparam)
        FreeCAD.Console.PrintMessage('\n')

    def write_z88_mesh(self):
        mesh_file_path = self.file_name + 'i1.txt'
        f = open(mesh_file_path, 'w')
        importZ88Mesh.write_z88_mesh_to_file(self.femnodes_mesh, self.femelement_table, self.z88_element_type, f)
        f.close()

    def write_z88_contraints(self):
        constraints_data = []  # will be a list of tuple for better sorting

        # fixed constraints
        # get nodes
        self.get_constraints_fixed_nodes()
        # write nodes to constraints_data (different from writing to file in ccxInpWriter)
        for femobj in self.fixed_objects:
            for n in femobj['Nodes']:
                constraints_data.append((n, str(n) + '  1  2  0\n'))
                constraints_data.append((n, str(n) + '  2  2  0\n'))
                constraints_data.append((n, str(n) + '  3  2  0\n'))

        # forces constraints
        # check shape type of reference shape and get node loads
        self.get_constraints_force_nodeloads()
        # write node loads to constraints_data (a bit different from writing to file for ccxInpWriter)
        for femobj in self.force_objects:  # femobj --> dict, FreeCAD document object is femobj['Object']
            direction_vec = femobj['Object'].DirectionVector
            for ref_shape in femobj['NodeLoadTable']:
                for n in sorted(ref_shape[1]):
                    node_load = ref_shape[1][n]
                    if (direction_vec.x != 0.0):
                        v1 = direction_vec.x * node_load
                        constraints_data.append((n, str(n) + '  1  1  ' + str(v1) + '\n'))
                    if (direction_vec.y != 0.0):
                        v2 = direction_vec.y * node_load
                        constraints_data.append((n, str(n) + '  2  1  ' + str(v2) + '\n'))
                    if (direction_vec.z != 0.0):
                        v3 = direction_vec.z * node_load
                        constraints_data.append((n, str(n) + '  3  1  ' + str(v3) + '\n'))

        # write constraints_data to file
        contraints_file_path = self.file_name + 'i2.txt'
        f = open(contraints_file_path, 'w')
        f.write(str(len(constraints_data)) + '\n')
        for c in sorted(constraints_data):
            f.write(c[1])
        f.close()

    def write_z88_face_loads(self):
        # not yet supported
        face_load_file_path = self.file_name + 'i5.txt'
        f = open(face_load_file_path, 'w')
        f.write(' 0')
        f.write('\n')
        f.close()

    def write_z88_materials(self):
        if len(self.material_objects) == 1:
            material_data_file_name = '51.txt'
            materials_file_path = self.file_name + 'mat.txt'
            fms = open(materials_file_path, 'w')
            fms.write('1\n')
            fms.write('1 ' + str(self.element_count) + ' ' + material_data_file_name)
            fms.write('\n')
            fms.close()
            material_data_file_path = self.dir_name + '/' + material_data_file_name
            fmd = open(material_data_file_path, 'w')
            mat_obj = self.material_objects[0]['Object']
            YM = FreeCAD.Units.Quantity(mat_obj.Material['YoungsModulus'])
            YM_in_MPa = YM.getValueAs('MPa')
            PR = float(mat_obj.Material['PoissonRatio'])
            fmd.write('{0} {1:.3f}'.format(YM_in_MPa, PR))
            fmd.write('\n')
            fmd.close()
        else:
            FreeCAD.Console.PrintError("Multiple Materials for Z88 not yet supported!\n")

    def write_z88_elements_properties(self):
        element_properties_file_path = self.file_name + 'elp.txt'
        elements_data = []
        if FemMeshTools.is_edge_femmesh(self.femmesh):
            if len(self.beamsection_objects) == 1:
                beam_obj = self.beamsection_objects[0]['Object']
                width = beam_obj.RectWidth.getValueAs('mm')
                height = beam_obj.RectHeight.getValueAs('mm')
                area = str(width * height)
                elements_data.append('1 ' + str(self.element_count) + ' ' + area + ' 0 0 0 0 0 0 ')
                FreeCAD.Console.PrintMessage("Be aware, only trusses are supported for edge meshes!\n")
            else:
                FreeCAD.Console.PrintError("Multiple beamsections for Z88 not yet supported!\n")
        elif FemMeshTools.is_face_femmesh(self.femmesh):
            if len(self.shellthickness_objects) == 1:
                thick_obj = self.shellthickness_objects[0]['Object']
                thickness = str(thick_obj.Thickness.getValueAs('mm'))
                elements_data.append('1 ' + str(self.element_count) + ' ' + thickness + ' 0 0 0 0 0 0 ')
            else:
                FreeCAD.Console.PrintError("Multiple thicknesses for Z88 not yet supported!\n")
        elif FemMeshTools.is_solid_femmesh(self.femmesh):
            elements_data.append('1 ' + str(self.element_count) + ' 0 0 0 0 0 0 0')
        else:
            FreeCAD.Console.PrintError("Error!\n")
        f = open(element_properties_file_path, 'w')
        f.write(str(len(elements_data)) + '\n')
        for e in elements_data:
            f.write(e)
        f.write('\n')
        f.close()

    def write_z88_integration_properties(self):
        integration_data = []
        integration_data.append('1 ' + str(self.element_count) + ' ' + self.z88_elparam['INTORD'] + ' ' + self.z88_elparam['INTOS'])
        integration_properties_file_path = self.file_name + 'int.txt'
        f = open(integration_properties_file_path, 'w')
        f.write(str(len(integration_data)) + '\n')
        for i in integration_data:
            f.write(i)
        f.write('\n')
        f.close()

    def write_z88_solver_parameter(self):
        global z88_man_template
        z88_man_template = z88_man_template.replace("$z88_param_ihflag", str(self.z88_elparam['IHFLAG']))
        z88_man_template = z88_man_template.replace("$z88_param_isflag", str(self.z88_elparam['ISFLAG']))
        solver_parameter_file_path = self.file_name + 'man.txt'
        f = open(solver_parameter_file_path, 'w')
        f.write(z88_man_template)
        f.close()

    def write_z88_memory_parameter(self):
        # self.z88_param_maxgs = 6000000
        self.z88_param_maxgs = 50000000  # vierkantrohr
        global z88_dyn_template
        z88_dyn_template = z88_dyn_template.replace("$z88_param_maxgs", str(self.z88_param_maxgs))
        solver_parameter_file_path = self.file_name + '.dyn'
        f = open(solver_parameter_file_path, 'w')
        f.write(z88_dyn_template)
        f.close()


# for solver parameter file Z88man.txt
z88_man_template = '''DYNAMIC START
---------------------------------------------------------------------------
Z88V14OS
---------------------------------------------------------------------------

---------------------------------------------------------------------------
GLOBAL
---------------------------------------------------------------------------

GLOBAL START
   IBFLAG          0
   IPFLAG          0
   IHFLAG          $z88_param_ihflag
GLOBAL END

---------------------------------------------------------------------------
LINEAR SOLVER
---------------------------------------------------------------------------

SOLVER START
   MAXIT           10000
   EPS             1e-007
   RALPHA          0.0001
   ROMEGA          1.1
SOLVER END

---------------------------------------------------------------------------
STRESS
---------------------------------------------------------------------------

STRESS START
   KDFLAG        0
   ISFLAG        $z88_param_isflag
STRESS END

DYNAMIC END
'''

# for memory parameter file z88.dyn
z88_dyn_template = '''DYNAMIC START
---------------------------------------------------------------------------
Z88 new version 14OS                   Z88 neue Version 14OS
---------------------------------------------------------------------------

---------------------------------------------------------------------------
LANGUAGE                   SPRACHE
---------------------------------------------------------------------------
GERMAN

---------------------------------------------------------------------------
Entries for mesh generator Z88N        Daten fuer Netzgenerator
---------------------------------------------------------------------------
  NET START
    MAXSE  40000
    MAXESS   800
    MAXKSS  4000
    MAXAN     15
  NET END

---------------------------------------------------------------------------
Common entries for all modules         gemeinsame Daten fuer alle Module
---------------------------------------------------------------------------

  COMMON START
    MAXGS    $z88_param_maxgs
    MAXKOI   1200000
    MAXK       60000
    MAXE      300000
    MAXNFG    200000
    MAXMAT        32
    MAXPEL        32
    MAXJNT        32
    MAXPR      10000
    MAXRBD     15000
    MAXIEZ   6000000
    MAXGP    2000000
  COMMON END

---------------------------------------------------------------------------
Entries for Cuthill-McKee Z88H         Daten fuer Cuthill- McKee Programm
---------------------------------------------------------------------------
  CUTKEE START
    MAXGRA  200
    MAXNDL 1000
  CUTKEE END


DYNAMIC END
'''

##  @}
