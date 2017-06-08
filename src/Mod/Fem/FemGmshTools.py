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
import FemMeshTools
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

        # clmax, CharacteristicLengthMax: float, 0.0 = 1e+22
        self.clmax = Units.Quantity(self.mesh_obj.CharacteristicLengthMax).Value
        if self.clmax == 0.0:
            self.clmax = 1e+22

        # clmin, CharacteristicLengthMin: float
        self.clmin = Units.Quantity(self.mesh_obj.CharacteristicLengthMin).Value

        # order
        # known_element_orders = ['1st', '2nd']
        self.order = self.mesh_obj.ElementOrder
        if self.order == '1st':
            self.order = '1'
        elif self.order == '2nd':
            self.order = '2'
        else:
            print('Error in order')

        # dimension
        self.dimension = self.mesh_obj.ElementDimension

        # Algorithm2D
        # known_mesh_algorithm_2D = ['Automatic', 'MeshAdapt', 'Delaunay', 'Frontal', 'BAMG', 'DelQuad']
        algo2D = self.mesh_obj.Algorithm2D
        if algo2D == 'Automatic':
            self.algorithm2D = '2'
        elif algo2D == 'MeshAdapt':
            self.algorithm2D = '1'
        elif algo2D == 'Delaunay':
            self.algorithm2D = '5'
        elif algo2D == 'Frontal':
            self.algorithm2D = '6'
        elif algo2D == 'BAMG':
            self.algorithm2D = '7'
        elif algo2D == 'DelQuad':
            self.algorithm2D = '8'
        else:
            self.algorithm2D = '2'

        # Algorithm3D
        # known_mesh_algorithm_3D = ['Automatic', 'Delaunay', 'New Delaunay', 'Frontal', 'Frontal Delaunay', 'Frontal Hex', 'MMG3D', 'R-tree']
        algo3D = self.mesh_obj.Algorithm2D
        if algo3D == 'Automatic':
            self.algorithm3D = '1'
        elif algo3D == 'Delaunay':
            self.algorithm3D = '1'
        elif algo3D == 'New Delaunay':
            self.algorithm3D = '2'
        elif algo3D == 'Frontal':
            self.algorithm3D = '4'
        elif algo3D == 'Frontal Delaunay':
            self.algorithm3D = '5'
        elif algo3D == 'Frontal Hex':
            self.algorithm3D = '6'
        elif algo3D == 'MMG3D':
            self.algorithm3D = '7'
        elif algo3D == 'R-tree':
            self.algorithm3D = '9'
        else:
            self.algorithm3D = '1'

    def create_mesh(self):
        print("\nWe gone start GMSH FEM mesh run!")
        print('  Part to mesh: Name --> ' + self.part_obj.Name + ',  Label --> ' + self.part_obj.Label + ', ShapeType --> ' + self.part_obj.Shape.ShapeType)
        print('  CharacteristicLengthMax: ' + str(self.clmax))
        print('  CharacteristicLengthMin: ' + str(self.clmin))
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
        # known_element_dimensions = ['From Shape', '1D', '2D', '3D']
        # if not given, GMSH uses the hightest available.
        # A use case for not "From Shape" would be a surface (2D) mesh of a solid
        if self.dimension == 'From Shape':
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
                # print('  Found a ' + shty)
                FreeCAD.Console.PrintLog("  Found a Compound. Since it could contain any kind of shape dimension 3 is used.\n")
                self.dimension = '3'  # dimension 3 works for 2D and 1d shapes as well
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
                    FreeCAD.Console.PrintError(error_message)
                    raise Exception(error_message)
                self.gmsh_bin = gmsh_path
            else:
                error_message = "No standard location implemented for your operating system. Set GMHS binary path in FEM preferences.\n"
                FreeCAD.Console.PrintError(error_message)
                raise Exception(error_message)
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
        print('  ' + self.gmsh_bin)

    def get_group_data(self):
        self.group_elements = {}
        # TODO solid, face, edge seam not work together, some print or make it work together
        # TODO handle groups for Edges and Vertexes

        # mesh groups and groups of analysis member
        if not self.mesh_obj.MeshGroupList:
            print ('  No mesh group objects.')
        else:
            print ('  Mesh group objects, we need to get the elements.')
            for mg in self.mesh_obj.MeshGroupList:
                new_group_elements = FemMeshTools.get_mesh_group_elements(mg, self.part_obj)
                for ge in new_group_elements:
                    if ge not in self.group_elements:
                        self.group_elements[ge] = new_group_elements[ge]
                    else:
                        FreeCAD.Console.PrintError("  A group with this name exists already.\n")
        if self.analysis:
            print('  Group meshing.')
            new_group_elements = FemMeshTools.get_analysis_group_elements(self.analysis, self.part_obj)
            for ge in new_group_elements:
                if ge not in self.group_elements:
                    self.group_elements[ge] = new_group_elements[ge]
                else:
                    FreeCAD.Console.PrintError("  A group with this name exists already.\n")
        else:
            print('  No anlysis members for group meshing.')
        print('  {}'.format(self.group_elements))

        # mesh regions
        self.ele_length_map = {}  # { 'ElementString' : element length }
        self.ele_node_map = {}  # { 'ElementString' : [element nodes] }
        if not self.mesh_obj.MeshRegionList:
            print ('  No mesh regions.')
        else:
            print ('  Mesh regions, we need to get the elements.')
            # by the use of MeshRegion object and a BooleanSplitCompound there could be problems with node numbers see
            # http://forum.freecadweb.org/viewtopic.php?f=18&t=18780&start=40#p149467
            # http://forum.freecadweb.org/viewtopic.php?f=18&t=18780&p=149520#p149520
            part = self.part_obj
            if self.mesh_obj.MeshRegionList:
                if part.Shape.ShapeType == "Compound" and hasattr(part, "Proxy"):  # other part obj might not have a Proxy, thus an exception would be raised
                    if (part.Proxy.Type == "FeatureBooleanFragments" or part.Proxy.Type == "FeatureSlice" or part.Proxy.Type == "FeatureXOR"):
                        error_message = "  The mesh to shape is a boolean split tools Compound and the mesh has mesh region list. GMSH could return unexpected meshes in such circumstances. It is strongly recommended to extract the shape to mesh from the Compound and use this one."
                        FreeCAD.Console.PrintError(error_message + "\n")
                        # TODO no gui popup because FreeCAD will be in a endless prind loop as long as the pop up is on --> my be find a better solution for either of both --> thus the pop up is in task panel
            for mr_obj in self.mesh_obj.MeshRegionList:
                # print(mr_obj.Name)
                # print(mr_obj.CharacteristicLength)
                # print(Units.Quantity(mr_obj.CharacteristicLength).Value)
                if mr_obj.CharacteristicLength:
                    if mr_obj.References:
                        for sub in mr_obj.References:
                            # print(sub[0])  # Part the elements belongs to
                            # check if the shape of the mesh region is an element of the Part to mesh, if not try to find the element in the shape to mesh
                            search_ele_in_shape_to_mesh = False
                            if not self.part_obj.Shape.isSame(sub[0].Shape):
                                # print("  One element of the meshregion " + mr_obj.Name + " is not an element of the Part to mesh.")
                                # print("  But we gone try to find it in the Shape to mesh :-)")
                                search_ele_in_shape_to_mesh = True
                            for elems in sub[1]:
                                # print(elems)  # elems --> element
                                if search_ele_in_shape_to_mesh:
                                    # we gone try to find the element it in the Shape to mesh and use the found element as elems
                                    ele_shape = FemMeshTools.get_element(sub[0], elems)  # the method getElement(element) does not return Solid elements
                                    found_element = FemMeshTools.find_element_in_shape(self.part_obj.Shape, ele_shape)
                                    if found_element:
                                        elems = found_element
                                    else:
                                        FreeCAD.Console.PrintError("One element of the meshregion " + mr_obj.Name + " could not be found in the Part to mesh. It will be ignored.\n")
                                # print(elems)  # element
                                if elems not in self.ele_length_map:
                                    self.ele_length_map[elems] = Units.Quantity(mr_obj.CharacteristicLength).Value
                                else:
                                    FreeCAD.Console.PrintError("The element " + elems + " of the meshregion " + mr_obj.Name + " has been added to another mesh region.\n")
                    else:
                        FreeCAD.Console.PrintError("The meshregion: " + mr_obj.Name + " is not used to create the mesh because the reference list is empty.\n")
                else:
                    FreeCAD.Console.PrintError("The meshregion: " + mr_obj.Name + " is not used to create the mesh because the CharacteristicLength is 0.0 mm.\n")
            for eleml in self.ele_length_map:
                ele_shape = FemMeshTools.get_element(self.part_obj, eleml)  # the method getElement(element) does not return Solid elements
                ele_vertexes = FemMeshTools.get_vertexes_by_element(self.part_obj.Shape, ele_shape)
                self.ele_node_map[eleml] = ele_vertexes
        print('  {}'.format(self.ele_length_map))
        print('  {}'.format(self.ele_node_map))

    def write_part_file(self):
        self.part_obj.Shape.exportBrep(self.temp_file_geometry)

    def write_geo(self):
        geo = open(self.temp_file_geo, "w")
        geo.write("// geo file for meshing with GMSH meshing software created by FreeCAD\n")
        geo.write("\n")
        geo.write("// open brep geometry\n")
        geo.write('Merge "' + self.temp_file_geometry + '";\n')
        geo.write("\n")
        if self.group_elements:
            # print('  We gone have found elements to make mesh groups for.')
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
        geo.write("// Characteristic Length\n")
        if self.ele_length_map:
            # we use the index FreeCAD which starts with 0, we need to add 1 for the index in GMSH
            geo.write("// Characteristic Length according CharacteristicLengthMap\n")
            for e in self.ele_length_map:
                ele_nodes = (''.join((str(n + 1) + ', ') for n in self.ele_node_map[e])).rstrip(', ')
                geo.write("// " + e + "\n")
                geo.write("Characteristic Length { " + ele_nodes + " } = " + str(self.ele_length_map[e]) + ";\n")
            geo.write("\n")
        geo.write("// min, max Characteristic Length\n")
        geo.write("Mesh.CharacteristicLengthMax = " + str(self.clmax) + ";\n")
        geo.write("Mesh.CharacteristicLengthMin = " + str(self.clmin) + ";\n")
        geo.write("\n")
        if hasattr(self.mesh_obj, 'RecombineAll') and self.mesh_obj.RecombineAll is True:
            geo.write("// other mesh options\n")
            geo.write("Mesh.RecombineAll = 1;\n")
            geo.write("\n")
        geo.write("// optimize the mesh\n")
        # GMSH tetra optimizer
        if hasattr(self.mesh_obj, 'OptimizeStd') and self.mesh_obj.OptimizeStd is True:
            geo.write("Mesh.Optimize = 1;\n")
        else:
            geo.write("Mesh.Optimize = 0;\n")
        # Netgen optimizer in GMSH
        if hasattr(self.mesh_obj, 'OptimizeNetgen') and self.mesh_obj.OptimizeNetgen is True:
            geo.write("Mesh.OptimizeNetgen = 1;\n")
        else:
            geo.write("Mesh.OptimizeNetgen = 0;\n")
        # hight order mesh optimizing
        if hasattr(self.mesh_obj, 'OptimizeNetgen') and self.mesh_obj.OptimizeNetgen is True:
            geo.write("Mesh.HighOrderOptimize = 1;  //probably needs more lines off adjustment in geo file\n")
        else:
            geo.write("Mesh.HighOrderOptimize = 0;  //probably needs more lines off adjustment in geo file\n")
        geo.write("\n")
        geo.write("// mesh order\n")
        geo.write("Mesh.ElementOrder = " + self.order + ";\n")
        geo.write("\n")
        geo.write("// mesh algorithm\n")
        geo.write("// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad)\n")
        geo.write("Mesh.Algorithm = " + self.algorithm2D + ";\n")
        geo.write("// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree)\n")
        geo.write("Mesh.Algorithm3D = " + self.algorithm3D + ";\n")
        geo.write("\n")
        geo.write("// meshing\n")
        geo.write("Mesh  " + self.dimension + ";\n")
        geo.write("\n")
        geo.write("// save\n")
        geo.write("Mesh.Format = 2;\n")  # unv
        if self.analysis and self.group_elements:
            geo.write("// For each group save not only the elements but the nodes too.;\n")
            geo.write("Mesh.SaveGroupsOfNodes = 1;\n")
            geo.write("// Needed for Group meshing too, because for one material there is no group defined;\n")  # belongs to Mesh.SaveAll but anly needed if there are groups
        geo.write("// Ignore Physical definitions and save all elements;\n")
        geo.write("Mesh.SaveAll = 1;\n")
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

##  @}
