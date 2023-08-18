# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Tools for the work with Gmsh mesher"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import os
import subprocess

import FreeCAD
from FreeCAD import Console
from FreeCAD import Units

import Fem
from . import meshtools
from femtools import femutils
from femtools import geomtools


class GmshError(Exception):
    pass


class GmshTools():
    def __init__(self, gmsh_mesh_obj, analysis=None):

        # mesh obj
        self.mesh_obj = gmsh_mesh_obj

        # analysis
        if analysis:
            self.analysis = analysis
        else:
            self.analysis = None

        # part to mesh
        self.part_obj = self.mesh_obj.Part

        # clmax, CharacteristicLengthMax: float, 0.0 = 1e+22
        self.clmax = Units.Quantity(self.mesh_obj.CharacteristicLengthMax).Value
        if self.clmax == 0.0:
            self.clmax = 1e+22

        # clmin, CharacteristicLengthMin: float
        self.clmin = Units.Quantity(self.mesh_obj.CharacteristicLengthMin).Value

        # geotol, GeometryTolerance: float, 0.0 = 1e-08
        self.geotol = self.mesh_obj.GeometryTolerance
        if self.geotol == 0.0:
            self.geotol = 1e-08

        # order
        # known_element_orders = ["1st", "2nd"]
        self.order = self.mesh_obj.ElementOrder
        if self.order == "1st":
            self.order = "1"
        elif self.order == "2nd":
            self.order = "2"
        else:
            Console.PrintError("Error in order\n")

        # dimension
        self.dimension = self.mesh_obj.ElementDimension

        # Algorithm2D
        algo2D = self.mesh_obj.Algorithm2D
        if algo2D == "Automatic":
            self.algorithm2D = "2"
        elif algo2D == "MeshAdapt":
            self.algorithm2D = "1"
        elif algo2D == "Delaunay":
            self.algorithm2D = "5"
        elif algo2D == "Frontal":
            self.algorithm2D = "6"
        elif algo2D == "BAMG":
            self.algorithm2D = "7"
        elif algo2D == "DelQuad":
            self.algorithm2D = "8"
        elif algo2D == "Packing Parallelograms":
            self.algorithm2D = "9"
        else:
            self.algorithm2D = "2"

        # Algorithm3D
        algo3D = self.mesh_obj.Algorithm3D
        if algo3D == "Automatic":
            self.algorithm3D = "1"
        elif algo3D == "Delaunay":
            self.algorithm3D = "1"
        elif algo3D == "New Delaunay":
            self.algorithm3D = "2"
        elif algo3D == "Frontal":
            self.algorithm3D = "4"
        elif algo3D == "MMG3D":
            self.algorithm3D = "7"
        elif algo3D == "R-tree":
            self.algorithm3D = "9"
        elif algo3D == "HXT":
            self.algorithm3D = "10"
        else:
            self.algorithm3D = "1"

        # RecombinationAlgorithm
        algoRecombo = self.mesh_obj.RecombinationAlgorithm
        if algoRecombo == "Simple":
            self.RecombinationAlgorithm = "0"
        elif algoRecombo == "Blossom":
            self.RecombinationAlgorithm = "1"
        elif algoRecombo == "Simple full-quad":
            self.RecombinationAlgorithm = "2"
        elif algoRecombo == "Blossom full-quad":
            self.RecombinationAlgorithm = "3"
        else:
            self.algoRecombo = "0"

        # HighOrderOptimize
        optimizers = self.mesh_obj.HighOrderOptimize
        if optimizers == "None":
            self.HighOrderOptimize = "0"
        elif optimizers == "Optimization":
            self.HighOrderOptimize = "1"
        elif optimizers == "Elastic+Optimization":
            self.HighOrderOptimize = "2"
        elif optimizers == "Elastic":
            self.HighOrderOptimize = "3"
        elif optimizers == "Fast Curving":
            self.HighOrderOptimize = "4"
        else:
            self.HighOrderOptimize = "0"

        # mesh groups
        if self.mesh_obj.GroupsOfNodes is True:
            self.group_nodes_export = True
        else:
            self.group_nodes_export = False
        self.group_elements = {}

        # mesh regions
        self.ele_length_map = {}  # { "ElementString" : element length }
        self.ele_node_map = {}  # { "ElementString" : [element nodes] }

        # mesh boundary layer
        self.bl_setting_list = []  # list of dict, each item map to MeshBoundaryLayer object
        self.bl_boundary_list = []  # to remove duplicated boundary edge or faces

        # other initializations
        self.temp_file_geometry = ""
        self.temp_file_mesh = ""
        self.temp_file_geo = ""
        self.mesh_name = ""
        self.gmsh_bin = ""
        self.error = False

    def update_mesh_data(self):
        self.start_logs()
        self.get_dimension()
        self.get_group_data()
        self.get_region_data()
        self.get_boundary_layer_data()

    def write_gmsh_input_files(self):
        self.write_part_file()
        self.write_geo()

    def create_mesh(self):
        try:
            self.update_mesh_data()
            self.get_tmp_file_paths()
            self.get_gmsh_command()
            self.write_gmsh_input_files()
            error = self.run_gmsh_with_geo()
            self.read_and_set_new_mesh()
        except GmshError as e:
            error = str(e)
        return error

    def start_logs(self):
        Console.PrintLog("\nGmsh FEM mesh run is being started.\n")
        Console.PrintLog("  Part to mesh: Name --> {},  Label --> {}, ShapeType --> {}\n".format(
            self.part_obj.Name,
            self.part_obj.Label,
            self.part_obj.Shape.ShapeType
        ))
        Console.PrintLog("  CharacteristicLengthMax: {}\n".format(self.clmax))
        Console.PrintLog("  CharacteristicLengthMin: {}\n".format(self.clmin))
        Console.PrintLog("  ElementOrder: {}\n".format(self.order))

    def get_dimension(self):
        # Dimension
        # known_element_dimensions = ["From Shape", "1D", "2D", "3D"]
        # if not given, Gmsh uses the highest available.
        # A use case for not "From Shape" would be a surface (2D) mesh of a solid
        if self.dimension == "From Shape":
            shty = self.part_obj.Shape.ShapeType
            if shty == "Solid" or shty == "CompSolid":
                # print("Found: " + shty)
                self.dimension = "3"
            elif shty == "Face" or shty == "Shell":
                # print("Found: " + shty)
                self.dimension = "2"
            elif shty == "Edge" or shty == "Wire":
                # print("Found: " + shty)
                self.dimension = "1"
            elif shty == "Vertex":
                # print("Found: " + shty)
                Console.PrintError("You can not mesh a Vertex.\n")
                self.dimension = "0"
            elif shty == "Compound":
                # print("  Found a " + shty)
                Console.PrintLog(
                    "  Found a Compound. Since it could contain"
                    "any kind of shape dimension 3 is used.\n"
                )
                self.dimension = "3"  # dimension 3 works for 2D and 1d shapes as well
            else:
                self.dimension = "0"
                Console.PrintError(
                    "Could not retrieve Dimension from shape type. Please choose dimension.\n"
                )
        elif self.dimension == "3D":
            self.dimension = "3"
        elif self.dimension == "2D":
            self.dimension = "2"
        elif self.dimension == "1D":
            self.dimension = "1"
        else:
            Console.PrintError("Error in dimension\n")
        Console.PrintMessage("  ElementDimension: " + self.dimension + "\n")

    def get_tmp_file_paths(self, param_working_dir=None, create=False):
        self.working_dir = ""
        # try to use given working dir
        if param_working_dir is not None:
            self.working_dir = param_working_dir
            if femutils.check_working_dir(self.working_dir) is not True:
                if create is True:
                    Console.PrintMessage(
                        "Dir given as parameter \'{}\' doesn't exist, "
                        "but parameter to create it is set to True. "
                        "Dir will be created.\n".format(self.working_dir)
                    )
                    os.mkdir(param_working_dir)
                else:
                    Console.PrintError(
                        "Dir given as parameter \'{}\' doesn't exist "
                        "and create parameter is set to False.\n"
                        .format(self.working_dir)
                    )
                    self.working_dir = femutils.get_pref_working_dir(self.mesh_obj)
                    Console.PrintMessage(
                        "Dir \'{}\' will be used instead.\n"
                        .format(self.working_dir)
                    )
        else:
            self.working_dir = femutils.get_pref_working_dir(self.mesh_obj)

        # check working_dir exist, if not use a tmp dir and inform the user
        if femutils.check_working_dir(self.working_dir) is not True:
            Console.PrintError(
                "Dir \'{}\' doesn't exist or cannot be created.\n"
                .format(self.working_dir)
            )
            self.working_dir = femutils.get_temp_dir(self.mesh_obj)
            Console.PrintMessage(
                "Dir \'{}\' will be used instead.\n"
                .format(self.working_dir)
            )

        # file paths
        _geometry_name = self.part_obj.Name + "_Geometry"
        self.mesh_name = self.part_obj.Name + "_Mesh"
        # geometry file
        self.temp_file_geometry = os.path.join(self.working_dir, _geometry_name + ".brep")
        # mesh file
        self.temp_file_mesh = os.path.join(self.working_dir, self.mesh_name + ".unv")
        # Gmsh input file
        self.temp_file_geo = os.path.join(self.working_dir, "shape2mesh.geo")
        Console.PrintMessage("  " + self.temp_file_geometry + "\n")
        Console.PrintMessage("  " + self.temp_file_mesh + "\n")
        Console.PrintMessage("  " + self.temp_file_geo + "\n")

    def get_gmsh_command(self):
        from platform import system
        gmsh_std_location = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh"
        ).GetBool("UseStandardGmshLocation")
        if gmsh_std_location:
            if system() == "Windows":
                gmsh_path = FreeCAD.getHomePath() + "bin/gmsh.exe"
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh"
                ).SetString("gmshBinaryPath", gmsh_path)
                self.gmsh_bin = gmsh_path
            elif system() == "Linux":
                p1 = subprocess.Popen(["which", "gmsh"], stdout=subprocess.PIPE)
                if p1.wait() == 0:
                    output = p1.stdout.read()
                    output = output.decode("utf-8")
                    gmsh_path = output.split("\n")[0]
                elif p1.wait() == 1:
                    error_message = (
                        "Gmsh binary gmsh not found in standard system binary path. "
                        "Please install Gmsh or set path to binary "
                        "in FEM preferences tab Gmsh.\n"
                    )
                    Console.PrintError(error_message)
                    raise GmshError(error_message)
                self.gmsh_bin = gmsh_path
            elif system() == "Darwin":
                # https://forum.freecad.org/viewtopic.php?f=13&t=73041&p=642026#p642022
                gmsh_path = "/Applications/Gmsh.app/Contents/MacOS/gmsh"
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh"
                ).SetString("gmshBinaryPath", gmsh_path)
                self.gmsh_bin = gmsh_path
            else:
                error_message = (
                    "No standard location implemented for your operating system. "
                    "Set GMHS binary path in FEM preferences.\n"
                )
                Console.PrintError(error_message)
                raise GmshError(error_message)
        else:
            if not self.gmsh_bin:
                self.gmsh_bin = FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh"
                ).GetString("gmshBinaryPath", "")
            if not self.gmsh_bin:  # in prefs not set, we will try to use something reasonable
                if system() == "Linux":
                    self.gmsh_bin = "gmsh"
                elif system() == "Windows":
                    self.gmsh_bin = FreeCAD.getHomePath() + "bin/gmsh.exe"
                else:
                    self.gmsh_bin = "gmsh"
        Console.PrintMessage("  " + self.gmsh_bin + "\n")

    def get_group_data(self):
        # TODO: solids, faces, edges and vertexes don't seem to work together in one group,
        #       some output message or make them work together

        # mesh group objects
        if not self.mesh_obj.MeshGroupList:
            # print("  No mesh group objects.")
            pass
        else:
            Console.PrintMessage("  Mesh group objects, we need to get the elements.\n")
            for mg in self.mesh_obj.MeshGroupList:
                new_group_elements = meshtools.get_mesh_group_elements(mg, self.part_obj)
                for ge in new_group_elements:
                    if ge not in self.group_elements:
                        self.group_elements[ge] = new_group_elements[ge]
                    else:
                        Console.PrintError("  A group with this name exists already.\n")

        # group meshing for analysis
        analysis_group_meshing = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General"
        ).GetBool("AnalysisGroupMeshing", False)
        if self.analysis and analysis_group_meshing:
            Console.PrintWarning(
                "  Group meshing for analysis is set to true in FEM General Preferences. "
                "Are you really sure about this? You could run into trouble!\n"
            )
            self.group_nodes_export = True
            new_group_elements = meshtools.get_analysis_group_elements(
                self.analysis,
                self.part_obj
            )
            for ge in new_group_elements:
                if ge not in self.group_elements:
                    self.group_elements[ge] = new_group_elements[ge]
                else:
                    Console.PrintError("  A group with this name exists already.\n")
        # else:
        #    Console.PrintMessage("  No Group meshing for analysis.\n")

        # if self.group_elements:
        #    Console.PrintMessage("  {}\n".format(self.group_elements))

    def get_gmsh_version(self):
        self.get_gmsh_command()
        if os.path.exists(self.gmsh_bin):
            found_message = "file found: " + self.gmsh_bin
            Console.PrintMessage(found_message + "\n")
        else:
            found_message = "file not found: " + self.gmsh_bin
            Console.PrintError(found_message + "\n")
            return (None, None, None), found_message

        command_list = [self.gmsh_bin, "--info"]
        try:
            p = subprocess.Popen(
                command_list,
                shell=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True
            )
        except Exception as e:
            Console.PrintMessage(str(e) + "\n")
            return (None, None, None), found_message + "\n\n" + "Error: " + str(e)

        gmsh_stdout, gmsh_stderr = p.communicate()
        Console.PrintMessage("Gmsh: StdOut:\n" + gmsh_stdout + "\n")
        if gmsh_stderr:
            Console.PrintError("Gmsh: StdErr:\n" + gmsh_stderr + "\n")

        from re import search
        # use raw string mode to get pep8 quiet
        # https://stackoverflow.com/q/61497292
        # https://github.com/MathSci/fecon236/issues/6
        match = search(r"^Version\s*:\s*(\d+)\.(\d+)\.(\d+)", gmsh_stdout)
        # return (major, minor, patch), fullmessage
        if match:
            mess = found_message + "\n\n" + gmsh_stdout
            return match.group(1, 2, 3), mess
        else:
            mess = found_message + "\n\n" + "Warning: Output not recognized\n\n" + gmsh_stdout
            return (None, None, None), mess

    def get_region_data(self):
        # mesh regions
        if not self.mesh_obj.MeshRegionList:
            # print("  No mesh regions.")
            pass
        else:
            # Console.PrintMessage("  Mesh regions, we need to get the elements.\n")
            # by the use of MeshRegion object and a BooleanSplitCompound
            # there could be problems with node numbers see
            # http://forum.freecad.org/viewtopic.php?f=18&t=18780&start=40#p149467
            # http://forum.freecad.org/viewtopic.php?f=18&t=18780&p=149520#p149520
            part = self.part_obj
            if (
                self.mesh_obj.MeshRegionList and part.Shape.ShapeType == "Compound"
                and (
                    femutils.is_of_type(part, "FeatureBooleanFragments")
                    or femutils.is_of_type(part, "FeatureSlice")
                    or femutils.is_of_type(part, "FeatureXOR")
                )
            ):
                self.outputCompoundWarning
            for mr_obj in self.mesh_obj.MeshRegionList:
                # print(mr_obj.Name)
                # print(mr_obj.CharacteristicLength)
                # print(Units.Quantity(mr_obj.CharacteristicLength).Value)
                if mr_obj.CharacteristicLength:
                    if mr_obj.References:
                        for sub in mr_obj.References:
                            # print(sub[0])  # Part the elements belongs to
                            # check if the shape of the mesh region
                            # is an element of the Part to mesh
                            # if not try to find the element in the shape to mesh
                            search_ele_in_shape_to_mesh = False
                            if not self.part_obj.Shape.isSame(sub[0].Shape):
                                Console.PrintLog(
                                    "  One element of the meshregion {} is "
                                    "not an element of the Part to mesh.\n"
                                    "But we are going to try to find it in "
                                    "the Shape to mesh :-)\n"
                                    .format(mr_obj.Name)
                                )
                                search_ele_in_shape_to_mesh = True
                            for elems in sub[1]:
                                # print(elems)  # elems --> element
                                if search_ele_in_shape_to_mesh:
                                    # we're going to try to find the element in the
                                    # Shape to mesh and use the found element as elems
                                    # the method getElement(element)
                                    # does not return Solid elements
                                    ele_shape = geomtools.get_element(sub[0], elems)
                                    found_element = geomtools.find_element_in_shape(
                                        self.part_obj.Shape, ele_shape
                                    )
                                    if found_element:
                                        elems = found_element
                                    else:
                                        Console.PrintError(
                                            "One element of the meshregion {} could not be found "
                                            "in the Part to mesh. It will be ignored.\n"
                                            .format(mr_obj.Name)
                                        )
                                # print(elems)  # element
                                if elems not in self.ele_length_map:
                                    self.ele_length_map[elems] = Units.Quantity(
                                        mr_obj.CharacteristicLength
                                    ).Value
                                else:
                                    Console.PrintError(
                                        "The element {} of the meshregion {} has "
                                        "been added to another mesh region.\n"
                                        .format(elems, mr_obj.Name)
                                    )
                    else:
                        Console.PrintError(
                            "The meshregion: {} is not used to create the mesh "
                            "because the reference list is empty.\n"
                            .format(mr_obj.Name)
                        )
                else:
                    Console.PrintError(
                        "The meshregion: {} is not used to create the "
                        "mesh because the CharacteristicLength is 0.0 mm.\n"
                        .format(mr_obj.Name)
                    )
            for eleml in self.ele_length_map:
                # the method getElement(element) does not return Solid elements
                ele_shape = geomtools.get_element(self.part_obj, eleml)
                ele_vertexes = geomtools.get_vertexes_by_element(self.part_obj.Shape, ele_shape)
                self.ele_node_map[eleml] = ele_vertexes
            # Console.PrintMessage("  {}\n".format(self.ele_length_map))
            # Console.PrintMessage("  {}\n".format(self.ele_node_map))

    def get_boundary_layer_data(self):
        # mesh boundary layer
        # currently only one boundary layer setting object is allowed
        # but multiple boundary can be selected
        # Mesh.CharacteristicLengthMin, must be zero
        # or a value less than first inflation layer height
        if not self.mesh_obj.MeshBoundaryLayerList:
            # print("  No mesh boundary layer setting document object.")
            pass
        else:
            # Console.PrintMessage("  Mesh boundary layers, we need to get the elements.\n")
            if self.part_obj.Shape.ShapeType == "Compound":
                # see http://forum.freecad.org/viewtopic.php?f=18&t=18780&start=40#p149467 and
                # http://forum.freecad.org/viewtopic.php?f=18&t=18780&p=149520#p149520
                self.outputCompoundWarning
            for mr_obj in self.mesh_obj.MeshBoundaryLayerList:
                if mr_obj.MinimumThickness and Units.Quantity(mr_obj.MinimumThickness).Value > 0:
                    if mr_obj.References:
                        belem_list = []
                        for sub in mr_obj.References:
                            # print(sub[0])  # Part the elements belongs to
                            # check if the shape of the mesh boundary_layer is an
                            # element of the Part to mesh
                            # if not try to find the element in the shape to mesh
                            search_ele_in_shape_to_mesh = False
                            if not self.part_obj.Shape.isSame(sub[0].Shape):
                                Console.PrintLog(
                                    "  One element of the mesh boundary layer {} is "
                                    "not an element of the Part to mesh.\n"
                                    "But we are going to try to find it in "
                                    "the Shape to mesh :-)\n"
                                    .format(mr_obj.Name)
                                )
                                search_ele_in_shape_to_mesh = True
                            for elems in sub[1]:
                                # print(elems)  # elems --> element
                                if search_ele_in_shape_to_mesh:
                                    # we try to find the element it in the Shape to mesh
                                    # and use the found element as elems
                                    # the method getElement(element) does not return Solid elements
                                    ele_shape = geomtools.get_element(sub[0], elems)
                                    found_element = geomtools.find_element_in_shape(
                                        self.part_obj.Shape,
                                        ele_shape
                                    )
                                    if found_element:  # also
                                        elems = found_element
                                    else:
                                        Console.PrintError(
                                            "One element of the mesh boundary layer {} could "
                                            "not be found in the Part to mesh. "
                                            "It will be ignored.\n"
                                            .format(mr_obj.Name)
                                        )
                                # print(elems)  # element
                                if elems not in self.bl_boundary_list:
                                    # fetch settings in DocumentObject
                                    # fan setting is not implemented
                                    belem_list.append(elems)
                                    self.bl_boundary_list.append(elems)
                                else:
                                    Console.PrintError(
                                        "The element {} of the mesh boundary "
                                        "layer {} has been added "
                                        "to another mesh boundary layer.\n"
                                        .format(elems, mr_obj.Name)
                                    )
                        setting = {}
                        setting["hwall_n"] = Units.Quantity(mr_obj.MinimumThickness).Value
                        setting["ratio"] = mr_obj.GrowthRate
                        setting["thickness"] = sum([
                            setting["hwall_n"] * setting["ratio"] ** i for i in range(
                                mr_obj.NumberOfLayers
                            )
                        ])
                        # setting["hwall_n"] * 5 # tangential cell dimension
                        setting["hwall_t"] = setting["thickness"]

                        # hfar: cell dimension outside boundary
                        # should be set later if some character length is set
                        if self.clmax > setting["thickness"] * 0.8 \
                                and self.clmax < setting["thickness"] * 1.6:
                            setting["hfar"] = self.clmax
                        else:
                            # set a value for safety, it may works as background mesh cell size
                            setting["hfar"] = setting["thickness"]
                        # from face name -> face id is done in geo file write up
                        # TODO: fan angle setup is not implemented yet
                        if self.dimension == "2":
                            setting["EdgesList"] = belem_list
                        elif self.dimension == "3":
                            setting["FacesList"] = belem_list
                        else:
                            Console.PrintError(
                                "boundary layer is only supported for 2D and 3D mesh.\n"
                            )
                        self.bl_setting_list.append(setting)
                    else:
                        Console.PrintError(
                            "The mesh boundary layer: {} is not used to create "
                            "the mesh because the reference list is empty.\n"
                            .format(mr_obj.Name)
                        )
                else:
                    Console.PrintError(
                        "The mesh boundary layer: {} is not used to create "
                        "the mesh because the min thickness is 0.0 mm.\n"
                        .format(mr_obj.Name)
                    )
            Console.PrintMessage("  {}\n".format(self.bl_setting_list))

    def write_groups(self, geo):
        if self.group_elements:
            # print("  We are going to have to find elements to make mesh groups for.")
            geo.write("// group data\n")
            # we use the element name of FreeCAD which starts
            # with 1 (example: "Face1"), same as Gmsh
            # for unit test we need them to have a fixed order
            for group in sorted(self.group_elements):
                gdata = self.group_elements[group]
                # print(gdata)
                # geo.write("// " + group + "\n")
                ele_nr = ""
                if gdata[0].startswith("Solid"):
                    physical_type = "Volume"
                    for ele in gdata:
                        ele_nr += (ele.lstrip("Solid") + ", ")
                elif gdata[0].startswith("Face"):
                    physical_type = "Surface"
                    for ele in gdata:
                        ele_nr += (ele.lstrip("Face") + ", ")
                elif gdata[0].startswith("Edge"):
                    physical_type = "Line"
                    for ele in gdata:
                        ele_nr += (ele.lstrip("Edge") + ", ")
                elif gdata[0].startswith("Vertex"):
                    physical_type = "Point"
                    for ele in gdata:
                        ele_nr += (ele.lstrip("Vertex") + ", ")
                if ele_nr:
                    ele_nr = ele_nr.rstrip(", ")
                    # print(ele_nr)
                    curly_br_s = "{"
                    curly_br_e = "}"
                    # explicit use double quotes in geo file
                    geo.write(
                        'Physical {}("{}") = {}{}{};\n'
                        .format(physical_type, group, curly_br_s, ele_nr, curly_br_e)
                    )
            geo.write("\n")

    def write_boundary_layer(self, geo):
        # currently single body is supported
        if len(self.bl_setting_list):
            geo.write("// boundary layer setting\n")
            Console.PrintMessage("  Start to write boundary layer setup\n")
            field_number = 1
            for item in self.bl_setting_list:
                prefix = "Field[" + str(field_number) + "]"
                geo.write(prefix + " = BoundaryLayer;\n")
                for k in item:
                    v = item[k]
                    if k in set(["EdgesList", "FacesList"]):
                        # the element name of FreeCAD which starts
                        # with 1 (example: "Face1"), same as Gmsh
                        # el_id = int(el[4:])  # FIXME:  strip `face` or `edge` prefix
                        ele_nodes = ("".join((str(el[4:]) + ", ") for el in v)).rstrip(", ")
                        line = prefix + "." + str(k) + " = {" + ele_nodes + " };\n"
                        geo.write(line)
                    else:
                        line = prefix + "." + str(k) + " = " + str(v) + ";\n"
                        geo.write(line)
                    Console.PrintMessage("{}\n".format(line))
                geo.write("BoundaryLayer Field = " + str(field_number) + ";\n")
                geo.write("// end of this boundary layer setup \n")
                field_number += 1
            geo.write("\n")
            geo.flush()
            Console.PrintMessage("  finished in boundary layer setup\n")
        else:
            # print("  no boundary layer setup is found for this mesh")
            geo.write("// no boundary layer settings for this mesh\n")

    def write_part_file(self):
        self.part_obj.Shape.exportBrep(self.temp_file_geometry)

    def write_geo(self):
        temp_dir = os.path.dirname(self.temp_file_geo)
        geo = open(self.temp_file_geo, "w")
        geo.write("// geo file for meshing with Gmsh meshing software created by FreeCAD\n")
        geo.write("\n")

        cpu_count = os.cpu_count()
        if cpu_count is not None and cpu_count > 1:
            geo.write("// enable multi-core processing\n")
            geo.write(f"General.NumThreads = {cpu_count};\n")
            geo.write("\n")

        geo.write("// open brep geometry\n")
        # explicit use double quotes in geo file
        geo.write('Merge "{}";\n'.format(os.path.relpath(self.temp_file_geometry, temp_dir)))
        geo.write("\n")

        # groups
        self.write_groups(geo)

        # Characteristic Length of the elements
        geo.write("// Characteristic Length\n")
        if self.ele_length_map:
            # we use the index FreeCAD which starts with 0
            # we need to add 1 for the index in Gmsh
            geo.write("// Characteristic Length according CharacteristicLengthMap\n")
            for e in self.ele_length_map:
                ele_nodes = (
                    "".join((str(n + 1) + ", ") for n in self.ele_node_map[e])
                ).rstrip(", ")
                geo.write("// " + e + "\n")
                elestr1 = "{"
                elestr2 = "}"
                geo.write(
                    "Characteristic Length {} {} {} = {};\n"
                    .format(
                        elestr1,
                        ele_nodes,
                        elestr2,
                        self.ele_length_map[e]
                    )
                )
            geo.write("\n")

        # boundary layer generation may need special setup
        # of Gmsh properties, set them in Gmsh TaskPanel
        self.write_boundary_layer(geo)

        # mesh parameter
        geo.write("// min, max Characteristic Length\n")
        geo.write("Mesh.CharacteristicLengthMax = " + str(self.clmax) + ";\n")
        if len(self.bl_setting_list):
            # if minLength must smaller than first layer of boundary_layer
            # it is safer to set it as zero (default value) to avoid error
            geo.write("Mesh.CharacteristicLengthMin = " + str(0) + ";\n")
        else:
            geo.write("Mesh.CharacteristicLengthMin = " + str(self.clmin) + ";\n")
        if hasattr(self.mesh_obj, "MeshSizeFromCurvature"):
            geo.write(
                "Mesh.MeshSizeFromCurvature = {}"
                "; // number of elements per 2*pi radians, 0 to deactivate\n"
                .format(self.mesh_obj.MeshSizeFromCurvature)
            )
        geo.write("\n")
        if hasattr(self.mesh_obj, "RecombineAll") and self.mesh_obj.RecombineAll is True:
            geo.write("// recombination for surfaces\n")
            geo.write("Mesh.RecombineAll = 1;\n")
        if hasattr(self.mesh_obj, "Recombine3DAll") and self.mesh_obj.Recombine3DAll is True:
            geo.write("// recombination for volumes\n")
            geo.write("Mesh.Recombine3DAll = 1;\n")
        if (
            (hasattr(self.mesh_obj, "RecombineAll") and self.mesh_obj.RecombineAll is True)
            or (hasattr(self.mesh_obj, "Recombine3DAll") and self.mesh_obj.Recombine3DAll is True)
        ):
            geo.write("// recombination algorithm\n")
            geo.write("Mesh.RecombinationAlgorithm = " + self.RecombinationAlgorithm + ";\n")
            geo.write("\n")

        geo.write("// optimize the mesh\n")
        # Gmsh tetra optimizer
        if hasattr(self.mesh_obj, "OptimizeStd") and self.mesh_obj.OptimizeStd is True:
            geo.write("Mesh.Optimize = 1;\n")
        else:
            geo.write("Mesh.Optimize = 0;\n")
        # Netgen optimizer in Gmsh
        if hasattr(self.mesh_obj, "OptimizeNetgen") and self.mesh_obj.OptimizeNetgen is True:
            geo.write("Mesh.OptimizeNetgen = 1;\n")
        else:
            geo.write("Mesh.OptimizeNetgen = 0;\n")
        # higher order mesh optimizing
        geo.write(
            "// High-order meshes optimization (0=none, 1=optimization, 2=elastic+optimization, "
            "3=elastic, 4=fast curving)\n"
        )
        geo.write("Mesh.HighOrderOptimize = " + self.HighOrderOptimize + ";\n")
        geo.write("\n")

        geo.write("// mesh order\n")
        geo.write("Mesh.ElementOrder = " + self.order + ";\n")
        if self.order == "2":
            if (
                hasattr(self.mesh_obj, "SecondOrderLinear")
                and self.mesh_obj.SecondOrderLinear is True
            ):
                geo.write(
                    "Mesh.SecondOrderLinear = 1; // Second order nodes are created "
                    "by linear interpolation instead by curvilinear\n"
                )
            else:
                geo.write(
                    "Mesh.SecondOrderLinear = 0; // Second order nodes are created "
                    "by linear interpolation instead by curvilinear\n"
                )
        geo.write("\n")

        geo.write(
            "// mesh algorithm, only a few algorithms are "
            "usable with 3D boundary layer generation\n"
        )
        geo.write(
            "// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, "
            "5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad, 9=Packing Parallelograms)\n"
        )
        if len(self.bl_setting_list) and self.dimension == 3:
            geo.write("Mesh.Algorithm = " + "DelQuad" + ";\n")  # Frontal/DelQuad are tested
        else:
            geo.write("Mesh.Algorithm = " + self.algorithm2D + ";\n")
        geo.write(
            "// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, "
            "7=MMG3D, 9=R-tree, 10=HTX)\n"
        )
        geo.write("Mesh.Algorithm3D = " + self.algorithm3D + ";\n")
        geo.write("\n")

        geo.write("// meshing\n")
        # remove duplicate vertices
        # see https://forum.freecad.org/viewtopic.php?f=18&t=21571&start=20#p179443
        if hasattr(self.mesh_obj, "CoherenceMesh") and self.mesh_obj.CoherenceMesh is True:
            geo.write(
                "Geometry.Tolerance = {}; // set geometrical "
                "tolerance (also used for merging nodes)\n"
                .format(self.geotol)
            )
            geo.write("Mesh  " + self.dimension + ";\n")
            geo.write("Coherence Mesh; // Remove duplicate vertices\n")
        else:
            geo.write("Mesh  " + self.dimension + ";\n")
        geo.write("\n")

        # save mesh
        geo.write("// save\n")
        if self.group_elements and self.group_nodes_export:
            geo.write("// For each group save not only the elements but the nodes too.;\n")
            geo.write("Mesh.SaveGroupsOfNodes = 1;\n")
            # belongs to Mesh.SaveAll but only needed if there are groups
            geo.write(
                "// Needed for Group meshing too, because "
                "for one material there is no group defined;\n")
        geo.write("// Ignore Physical definitions and save all elements;\n")
        geo.write("Mesh.SaveAll = 1;\n")
        # explicit use double quotes in geo file
        geo.write('Save "{}";\n'.format(os.path.relpath(self.temp_file_mesh, temp_dir)))
        geo.write("\n\n")

        # some useful information
        geo.write("// " + "*" * 70 + "\n")
        geo.write("// Gmsh documentation:\n")
        geo.write("// https://gmsh.info/doc/texinfo/gmsh.html#Mesh\n")
        geo.write("//\n")
        geo.write(
            "// We do not check if something went wrong, like negative "
            "jacobians etc. You can run Gmsh manually yourself: \n"
        )
        geo.write("//\n")
        geo.write("// to see full Gmsh log, run in bash:\n")
        geo.write("// " + self.gmsh_bin + " - " + self.temp_file_geo + "\n")
        geo.write("//\n")
        geo.write("// to run Gmsh and keep file in Gmsh GUI (with log), run in bash:\n")
        geo.write("// " + self.gmsh_bin + " " + self.temp_file_geo + "\n")
        geo.close()

    def run_gmsh_with_geo(self):
        command_list = [self.gmsh_bin, "-", self.temp_file_geo]
        # print(command_list)
        try:
            p = subprocess.Popen(
                command_list,
                shell=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            output, error = p.communicate()
            error = error.decode("utf-8")
            # stdout is still cut at some point
            # but the warnings are in stderr and thus printed :-)
            # print(output)
            # print(error)
        except Exception:
            if os.path.exists(self.gmsh_bin):
                error = "Error executing: {}\n".format(" ".join(command_list))
            else:
                error = "Gmsh executable not found: {}\n".format(self.gmsh_bin)
            Console.PrintError(error)
            self.error = True

        # workaround
        # filter useless gmsh warning in the regard of unknown element MSH type 15
        # https://forum.freecad.org/viewtopic.php?f=18&t=33946
        useless_warning = (
            "Warning : Unknown element type for UNV export "
            "(MSH type 15) - output file might be invalid"
        )
        new_err = error.replace(useless_warning, "")
        # remove empty lines, https://stackoverflow.com/a/1140967
        new_err = "".join([s for s in new_err.splitlines(True) if s.strip("\r\n")])

        return new_err

    def read_and_set_new_mesh(self):
        if not self.error:
            fem_mesh = Fem.read(self.temp_file_mesh)
            self.mesh_obj.FemMesh = fem_mesh
            Console.PrintMessage("  New mesh was added to the mesh object.\n")
        else:
            Console.PrintError("No mesh was created.\n")

    def outputCompoundWarning(self):
        error_message = (
            "The mesh to shape is a Boolean Split Tools compound "
            "and the mesh has mesh region list.\n"
            "Gmsh could return unexpected meshes in such circumstances.\n"
            "If this is the case, use the part workbench and "
            "apply a Compound Filter on the compound.\n"
            "Use the Compound Filter as input for the mesh."
        )
        Console.PrintWarning(error_message + "\n")

##  @}


"""
# simple example how to use the class GmshTools
import Part, ObjectsFem
doc = App.ActiveDocument

box_obj = doc.addObject("Part::Box", "Box")
doc.recompute()
box_obj.ViewObject.Visibility = False

femmesh_obj = ObjectsFem.makeMeshGmsh(doc, box_obj.Name + "_Mesh")
femmesh_obj.Part = box_obj
doc.recompute()

from femmesh.gmshtools import GmshTools as gt
gmsh_mesh = gt(femmesh_obj)
error = gmsh_mesh.create_mesh()
print(error)
doc.recompute()

"""


"""
# more sophisticated example which changes the mesh size
import Part, ObjectsFem
doc = App.ActiveDocument

box_obj = doc.addObject("Part::Box", "Box")
doc.recompute()
box_obj.ViewObject.Visibility = False

from femmesh.gmshtools import GmshTools
max_mesh_sizes = [0.5, 1, 2, 3, 5, 10]
for len in max_mesh_sizes:
    quantity_len = "{}".format(len)
    print("\n\n Start length = {}".format(quantity_len))
    femmesh_obj = ObjectsFem.makeMeshGmsh(doc, box_obj.Name + "_Mesh")
    femmesh_obj.Part = box_obj
    femmesh_obj.CharacteristicLengthMax = "{}".format(quantity_len)
    femmesh_obj.CharacteristicLengthMin = "{}".format(quantity_len)
    doc.recompute()
    gm = GmshTools(femmesh_obj)
    gm.update_mesh_data()
    # set the tmp file path to some user path including the length
    gm.get_tmp_file_paths("/tmp/fcgm_" + str(len), True)
    gm.get_gmsh_command()
    gm.write_gmsh_input_files()
    error = gm.run_gmsh_with_geo()
    print(error)
    gm.read_and_set_new_mesh()
    doc.recompute()
    print("Done length = {}".format(quantity_len))

"""

"""
TODO
class GmshTools should be splittet in two classes
one class should only collect the mesh parameter from mesh object and its childs
a second class only uses the collected parameter,
writes the input file runs gmsh reads back the unv and returns a FemMesh
gmsh binary will be collected in the second class
with this we could mesh without document objects
create a shape and run meshinging class, get the FemMesh :-)
"""
