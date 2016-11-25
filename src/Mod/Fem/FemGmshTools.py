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

__title__ = "Tools for the work with GMSH mesher"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import Fem
import Units
import subprocess
import tempfile
from platform import system


class FemGmshTools():
    def __init__(self, gmsh_mesh_obj, analysis=None):
        self.mesh_obj = gmsh_mesh_obj
        if analysis:
            self.analysis = analysis
            # group meshing turned on
        else:
            self.analysis = None
            # group meshing turned off

        # part to mesh
        self.part_obj = self.mesh_obj.Part

        # clmax, ElementSizeMax: float, 0.0 = 1e+22
        self.clmax = Units.Quantity(self.mesh_obj.ElementSizeMax).Value
        if self.clmax == 0.0:
            self.clmax = 1e+22

        # clmin, ElementSizeMin: float
        self.clmin = Units.Quantity(self.mesh_obj.ElementSizeMin).Value

        # order, ElementOrder: ['Auto', '1st', '2nd']
        self.order = self.mesh_obj.ElementOrder
        if self.order == '1st':
            self.order = '1'
        elif self.order == 'Auto' or self.order == '2nd':
            self.order = '2'
        else:
            print('Error in order')

        # dimension, ElementDimension:  ['Auto', '1D', '2D', '3D']
        self.dimension = self.mesh_obj.ElementDimension

    def create_mesh(self):
        print("\nWe gone start GMSH FEM mesh run!")
        print('  Part to mesh: Name --> ' + self.part_obj.Name + ',  Label --> ' + self.part_obj.Label + ', ShapeType --> ' + self.part_obj.Shape.ShapeType)
        print('  ElementSizeMax: ' + str(self.clmax))
        print('  ElementSizeMin: ' + str(self.clmin))
        print('  ElementOrder: ' + self.order)
        self.get_dimension()
        self.get_tmp_file_paths()
        self.get_gmsh_command()
        self.get_group_data()
        self.write_part_file()
        self.write_geo()
        error = self.run_gmsh_with_geo()
        self.read_and_set_new_mesh()
        return error

    def get_dimension(self):
        # Dimension
        # GMSH uses the hightest availabe.
        # A use case for not auto would be a surface (2D) mesh of a solid or other 3d shape
        if self.dimension == 'Auto':
            shty = self.part_obj.Shape.ShapeType
            if shty == 'Solid' or shty == 'CompSolid':
                # print('Found: ' + shty)
                self.dimension = '3'
            elif shty == 'Face' or shty == 'Shell':
                # print('Found: ' + shty)
                self.dimension = '2'
            elif shty == 'Edge' or shty == 'Wire':
                # print('Found: ' + shty)
                self.dimension = '1'
            elif shty == 'Vertex':
                # print('Found: ' + shty)
                FreeCAD.Console.PrintError("You can not mesh a Vertex.\n")
                self.dimension = '0'
            elif shty == 'Compound':
                print('Found: ' + shty)
                print('I do not know what is inside your Compound. Dimension was set to 3 anyway.')
                # TODO check contents of Compound
                # use dimension 3 on any shape works for 2D and 1d meshes as well !
                # but not in combination with sewfaces or connectfaces
                self.dimension = '3'
            else:
                self.dimension = '0'
                FreeCAD.Console.PrintError('Could not retrive Dimension from shape type. Please choose dimension.')
        elif self.dimension == '3D':
            self.dimension = '3'
        elif self.dimension == '2D':
            self.dimension = '2'
        elif self.dimension == '1D':
            self.dimension = '1'
        else:
            print('Error in dimension')
        print('  ElementDimension: ' + self.dimension)

    def get_tmp_file_paths(self):
        if system() == "Linux":
            path_sep = "/"
        elif system() == "Windows":
            path_sep = "\\"
        else:
            path_sep = "/"
        tmpdir = tempfile.gettempdir()
        # geometry file
        self.temp_file_geometry = tmpdir + path_sep + self.part_obj.Name + '_Geometry.brep'
        print('  ' + self.temp_file_geometry)
        # mesh file
        self.mesh_name = self.part_obj.Name + '_Mesh_TmpGmsh'
        self.temp_file_mesh = tmpdir + path_sep + self.mesh_name + '.unv'
        print('  ' + self.temp_file_mesh)
        # GMSH input file
        self.temp_file_geo = tmpdir + path_sep + 'shape2mesh.geo'
        print('  ' + self.temp_file_geo)

    def get_gmsh_command(self):
        self.gmsh_bin = None
        gmsh_std_location = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetBool("UseStandardGmshLocation")
        if gmsh_std_location:
            if system() == "Windows":
                gmsh_path = FreeCAD.getHomePath() + "bin/gmsh.exe"
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").SetString("gmshBinaryPath", gmsh_path)
                self.gmsh_bin = gmsh_path
            elif system() == "Linux":
                p1 = subprocess.Popen(['which', 'gmsh'], stdout=subprocess.PIPE)
                if p1.wait() == 0:
                    gmsh_path = p1.stdout.read().split('\n')[0]
                elif p1.wait() == 1:
                    error_message = "GMSH binary gmsh not found in standard system binary path. Please install gmsh or set path to binary in FEM preferences tab GMSH.\n"
                    # if FreeCAD.GuiUp:
                    #     QtGui.QMessageBox.critical(None, "No GMSH binary ccx", error_message)
                    raise Exception(error_message)
                self.gmsh_bin = gmsh_path
        else:
            if not self.gmsh_bin:
                self.gmsh_bin = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetString("gmshBinaryPath", "")
            if not self.gmsh_bin:  # in prefs not set, we will try to use something reasonable
                if system() == "Linux":
                    self.gmsh_bin = "gmsh"
                elif system() == "Windows":
                    self.gmsh_bin = FreeCAD.getHomePath() + "bin/gmsh.exe"
                else:
                    self.gmsh_bin = "gmsh"
            self.gmsh_bin = self.gmsh_bin
        print('  ' + self.gmsh_bin)

    def get_group_data(self):
        if self.analysis:
            print('  Group meshing.')
            import FemMeshTools
            self.group_elements = FemMeshTools.get_analysis_group_elements(self.analysis, self.part_obj)
            print(self.group_elements)
        else:
            print('  NO group meshing.')

    def write_part_file(self):
        self.part_obj.Shape.exportBrep(self.temp_file_geometry)

    def write_geo(self):
        geo = open(self.temp_file_geo, "w")
        geo.write('Merge "' + self.temp_file_geometry + '";\n')
        geo.write("\n")
        if self.analysis and self.group_elements:
            print('  We gone have found elements to make mesh groups for!')
            geo.write("// group data\n")
            # we use the element name of FreeCAD which starts with 1 (example: 'Face1'), same as GMSH
            for group in self.group_elements:
                gdata = self.group_elements[group]
                # print(gdata)
                # geo.write("// " + group + "\n")
                ele_nr = ''
                if gdata[0].startswith('Solid'):
                    physical_type = 'Volume'
                    for ele in gdata:
                        ele_nr += (ele.lstrip('Solid') + ', ')
                elif gdata[0].startswith('Face'):
                    physical_type = 'Surface'
                    for ele in gdata:
                        ele_nr += (ele.lstrip('Face') + ', ')
                elif gdata[0].startswith('Edge') or gdata[0].startswith('Vertex'):
                    geo.write("// " + group + " group data not written. Edges or Vertexes group data not supported.\n")
                    print('  Groups for Edges or Vertexes reference shapes not handeled yet.')
                if ele_nr:
                    ele_nr = ele_nr.rstrip(', ')
                    # print(ele_nr)
                    geo.write('Physical ' + physical_type + '("' + group + '") = {' + ele_nr + '};\n')
            geo.write("\n")
        geo.write("Mesh.CharacteristicLengthMax = " + str(self.clmax) + ";\n")
        geo.write("Mesh.CharacteristicLengthMin = " + str(self.clmin) + ";\n")
        geo.write("Mesh.ElementOrder = " + self.order + ";\n")
        geo.write("//Mesh.HighOrderOptimize = 1;\n")  # but does not really work, in GUI it does
        geo.write("Mesh.Algorithm3D = 1;\n")
        geo.write("Mesh.Algorithm = 2;\n")
        geo.write("Mesh  " + self.dimension + ";\n")
        geo.write("Mesh.Format = 2;\n")  # unv
        if self.analysis and self.group_elements:
            geo.write("// For each group save not only the elements but the nodes too.;\n")
            geo.write("Mesh.SaveGroupsOfNodes = 1;\n")
            geo.write("// Needed for Group meshing too, because for one material there is no group defined;\n")  # belongs to Mesh.SaveAll but anly needed if there are groups
        geo.write("// Ignore Physical definitions and save all elements;\n")
        geo.write("Mesh.SaveAll = 1;\n")
        geo.write("\n")
        geo.write('Save "' + self.temp_file_mesh + '";\n')
        geo.write("\n\n")
        geo.write("//////////////////////////////////////////////////////////////////////\n")
        geo.write("// GMSH documentation:\n")
        geo.write("// http://gmsh.info/doc/texinfo/gmsh.html#Mesh\n")
        geo.write("//\n")
        geo.write("// We do not check if something went wrong, like negative jacobians etc. You can run GMSH manually yourself: \n")
        geo.write("//\n")
        geo.write("// to see full GMSH log, run in bash:\n")
        geo.write("// " + self.gmsh_bin + " - " + self.temp_file_geo + "\n")
        geo.write("//\n")
        geo.write("// to run GMSH and keep file in GMSH GUI (with log), run in bash:\n")
        geo.write("// " + self.gmsh_bin + " " + self.temp_file_geo + "\n")
        geo.close

    def run_gmsh_with_geo(self):
        self.error = False
        comandlist = [self.gmsh_bin, '-', self.temp_file_geo]
        # print(comandlist)
        try:
            p = subprocess.Popen(comandlist, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            output, error = p.communicate()
            # print(output)  # stdout is still cut at some point but the warnings are in stderr and thus printed :-)
            # print(error)
        except:
            error = 'Error executing: {}\n'.format(self.gmsh_command)
            FreeCAD.Console.PrintError(error)
            self.error = True
        return error

    def read_and_set_new_mesh(self):
        if not self.error:
            fem_mesh = Fem.read(self.temp_file_mesh)
            self.mesh_obj.FemMesh = fem_mesh
            print('  The Part should have a pretty new FEM mesh!')
        else:
            print('No mesh was created.')
        del self.temp_file_geometry
        del self.temp_file_mesh

#  @}
