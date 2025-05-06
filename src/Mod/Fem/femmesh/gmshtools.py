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
import re
import shutil
import subprocess
from PySide.QtCore import QProcess, QThread

import FreeCAD
from FreeCAD import Console
from FreeCAD import Units

import Fem
from . import meshtools
from femtools import femutils
from femtools import geomtools


class GmshError(Exception):
    pass


class GmshTools:

    name = "Gmsh"

    def __init__(self, gmsh_mesh_obj, analysis=None):

        # mesh obj
        self.mesh_obj = gmsh_mesh_obj

        self.process = QProcess()
        # analysis
        self.analysis = None
        if analysis:
            self.analysis = analysis
        else:
            for i in self.mesh_obj.InList:
                if i.isDerivedFrom("Fem::FemAnalysis"):
                    self.analysis = i
                    break

        self.load_properties()
        self.error = False

    def _next_field_number(self, background=True):
        # returns the next unique field number. If cakcground = True,
        # the field is used as background field
        self._field_counter += 1
        if background:
            self._background_fields.append(self._field_counter)
        return self._field_counter

    def load_properties(self):
        # part to mesh
        self.part_obj = self.mesh_obj.Shape

        # clmax, CharacteristicLengthMax: float, 0.0 = 1e+22
        self.clmax = Units.Quantity(self.mesh_obj.CharacteristicLengthMax).Value
        if self.clmax == 0.0:
            self.clmax = 1e22

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
        elif algo2D == "Quasi-structured Quad":
            self.algorithm2D = "11"
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

        # SubdivisionAlgorithm
        algoSubdiv = self.mesh_obj.SubdivisionAlgorithm
        if algoSubdiv == "All Quadrangles":
            self.SubdivisionAlgorithm = "1"
        elif algoSubdiv == "All Hexahedra":
            self.SubdivisionAlgorithm = "2"
        elif algoSubdiv == "Barycentric":
            self.SubdivisionAlgorithm = "3"
        else:
            self.SubdivisionAlgorithm = "0"

        # mesh groups
        if self.mesh_obj.GroupsOfNodes is True:
            self.group_nodes_export = True
        else:
            self.group_nodes_export = False
        self.group_elements = {}

        # mesh regions
        self.ele_length_list = []       # [ (element length, {element names}) ]
        self.region_element_set = set() # set to remove duplicated element edge or faces

        # mesh boundary layer
        self.bl_setting_list = []  # list of dict, each item map to MeshBoundaryLayer object
        self.bl_boundary_list = []  # to remove duplicated boundary edge or faces

        # mesh distance
        self.dist_setting_list = []     # list of dict, each item map to MeshBoundaryLayer object
        self.dist_element_set = set()   # set to remove duplicated element edge or faces

        # transfinite meshes
        self.transfinite_curve_settings = []      # list of dict, one entry per curve
        self.transfinite_curve_elements = set()   # set to remove duplicated element edge or faces

        # other initializations
        self.temp_file_geometry = ""
        self.temp_file_mesh = ""
        self.temp_file_geo = ""
        self.mesh_name = ""
        self.gmsh_bin = ""
        self._field_counter = 0
        self._background_fields = []

    def update_mesh_data(self):
        self.start_logs()
        self.get_dimension()
        self.get_group_data()
        self.get_region_data()
        self.get_boundary_layer_data()
        self.get_distance_data()
        self.get_transfinite_data()

    def write_gmsh_input_files(self):
        self.write_part_file()
        self.write_geo()

    def prepare(self):
        self.load_properties()
        self.update_mesh_data()
        self.get_tmp_file_paths()
        self.get_gmsh_command()
        self.write_gmsh_input_files()

    def compute(self):
        log_level = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetString(
            "LogVerbosity", "3"
        )
        command_list = ["-v", log_level, "-", self.temp_file_geo]
        self.process.start(self.gmsh_bin, command_list)
        return self.process

    def update_properties(self):
        self.mesh_obj.FemMesh = Fem.read(self.temp_file_mesh)
        self.rename_groups()

    def create_mesh(self):
        self.prepare()
        p = self.compute()
        p.waitForFinished()
        self.update_properties()

    def start_logs(self):
        Console.PrintLog("\nGmsh FEM mesh run is being started.\n")
        Console.PrintLog(
            "  Part to mesh: Name --> {},  Label --> {}, ShapeType --> {}\n".format(
                self.part_obj.Name, self.part_obj.Label, self.part_obj.Shape.ShapeType
            )
        )
        Console.PrintLog(f"  MeshSizeMax: {self.clmax}\n")
        Console.PrintLog(f"  MeshSizeMin: {self.clmin}\n")
        Console.PrintLog(f"  ElementOrder: {self.order}\n")

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
                        "Dir given as parameter '{}' doesn't exist, "
                        "but parameter to create it is set to True. "
                        "Dir will be created.\n".format(self.working_dir)
                    )
                    os.mkdir(param_working_dir)
                else:
                    Console.PrintError(
                        "Dir given as parameter '{}' doesn't exist "
                        "and create parameter is set to False.\n".format(self.working_dir)
                    )
                    self.working_dir = femutils.get_pref_working_dir(self.mesh_obj)
                    Console.PrintMessage(f"Dir '{self.working_dir}' will be used instead.\n")
        else:
            self.working_dir = femutils.get_pref_working_dir(self.mesh_obj)

        # check working_dir exist, if not use a tmp dir and inform the user
        if femutils.check_working_dir(self.working_dir) is not True:
            Console.PrintError(f"Dir '{self.working_dir}' doesn't exist or cannot be created.\n")
            self.working_dir = femutils.get_temp_dir(self.mesh_obj)
            Console.PrintMessage(f"Dir '{self.working_dir}' will be used instead.\n")

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
        self.gmsh_bin = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh"
        ).GetString("gmshBinaryPath", "")

        if self.gmsh_bin and not shutil.which(self.gmsh_bin):
            error_message = (
                f"Configured Gmsh binary '${self.gmsh_bin}' not found. Please set path to "
                "binary in FEM's Gmsh preferences page or leave blank to use default binary.\n"
            )
            Console.PrintError(error_message)
            raise GmshError(error_message)

        if not self.gmsh_bin:
            from platform import system

            gmsh_path = shutil.which("gmsh")
            if system() == "Darwin" and not gmsh_path:
                gmsh_path = shutil.which("/Applications/Gmsh.app/Contents/MacOS/gmsh")

            if not gmsh_path:
                error_message = (
                    "Gmsh binary not found. Please install Gmsh or set path to binary "
                    "in FEM's Gmsh preferences page.\n"
                )
                Console.PrintError(error_message)
                raise GmshError(error_message)
            self.gmsh_bin = gmsh_path
        Console.PrintMessage("  " + self.gmsh_bin + "\n")

    def get_group_data(self):
        # mesh group objects. Only one shape type is expected
        geom = self.mesh_obj.Shape.getPropertyOfGeometry()
        r_solids = range(1, len(geom.Solids) + 1)
        r_faces = range(1, len(geom.Faces) + 1)
        r_edges = range(1, len(geom.Edges) + 1)
        solids = [(f"Solid{i}", [f"Solid{i}"]) for i in r_solids]
        faces = [(f"Face{i}", [f"Face{i}"]) for i in r_faces]
        edges = [(f"Edge{i}", [f"Edge{i}"]) for i in r_edges]
        shapes = []
        shapes.extend(solids)
        shapes.extend(faces)
        shapes.extend(edges)

        self.group_elements = dict(shapes)
        if not self.mesh_obj.MeshGroupList:
            # print("  No mesh group objects.")
            pass
        else:
            Console.PrintMessage("  Mesh group objects, we need to get the elements.\n")
            for mg in self.mesh_obj.MeshGroupList:
                if mg.Suppressed:
                    continue
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
            new_group_elements = meshtools.get_analysis_group_elements(self.analysis, self.part_obj)
            for ge in new_group_elements:
                if ge not in self.group_elements:
                    self.group_elements[ge] = new_group_elements[ge]
                else:
                    Console.PrintError("  A group with this name exists already.\n")
        # else:
        #    Console.PrintMessage("  No Group meshing for analysis.\n")

        # if self.group_elements:
        #    Console.PrintMessage("  {}\n".format(self.group_elements))

    def rename_groups(self):
        # salomemesh adds a suffix to the names of element groups if there are also nodes
        #  in the groups in the .unv file. This method removes the suffix
        reg_exp = re.compile(r"(?P<item>(Edge|Face|Solid)\d+)_(?!Nodes)\w+$")
        fem_mesh = self.mesh_obj.FemMesh
        for i in fem_mesh.Groups:
            grp = fem_mesh.getGroupName(i)
            m = reg_exp.match(grp)
            if m:
                fem_mesh.renameGroup(i, m.group("item"))

    def version(self):
        self.get_gmsh_command()
        if shutil.which(self.gmsh_bin):
            found_message = "file found: " + self.gmsh_bin
            Console.PrintMessage(found_message + "\n")
        else:
            found_message = "file not found: " + self.gmsh_bin
            Console.PrintError(found_message + "\n")
            return found_message

        command_list = [self.gmsh_bin, "--info"]
        try:
            p = subprocess.Popen(
                command_list,
                shell=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
                startupinfo=femutils.startProgramInfo("hide"),
            )
        except Exception as e:
            Console.PrintMessage(str(e) + "\n")
            return found_message + "\n\n" + "Error: " + str(e)

        gmsh_stdout, gmsh_stderr = p.communicate()
        return gmsh_stdout

    def _get_definitions_of_type(self, type_):
        result = []
        for definition in self.mesh_obj.MeshRefinementList:
            if femutils.is_of_type(definition, type_):
                result.append(definition)

        return result

    def _get_reference_elements(self, mr_obj, duplicates_set = None):

        elements = set()
        for sub in mr_obj.References:
            # print(sub[0])  # Part the elements belongs to
            # check if the shape of the mesh refinements
            # is an element of the Part to mesh
            # if not try to find the element in the shape to mesh
            search_ele_in_shape_to_mesh = False
            if not self.part_obj.Shape.isSame(sub[0].Shape):
                Console.PrintLog(
                    "  One element of the mesh refinement {} is "
                    "not an element of the Part to mesh.\n"
                    "But we are going to try to find it in "
                    "the Shape to mesh :-)\n".format(mr_obj.Name)
                )
                search_ele_in_shape_to_mesh = True

            for element in sub[1]:
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
                        element = found_element
                    else:
                        Console.PrintError(
                            "One element of the mesh refinement {} could not be found "
                            "in the Part to mesh. It will be ignored.\n".format(
                                mr_obj.Name
                            )
                        )
                # print(elems)  # element
                elements.add(element)

        if duplicates_set:
            duplicates = duplicates_set.intersection(elements)
            if duplicates:
                Console.PrintError(
                                "The elements {} of the mesh distance {} have been added "
                                "to another mesh distance already.\n".format(
                                    duplicates, mr_obj.Name
                                )
                            )
                elements = elements - duplicates

            duplicates_set.update(elements)

        return elements


    def _element_list_to_shape_idx_dict(self, element_list):
        # takes element list and builds a dict from it mapping from
        # shapes types to all indices

        reg_exp = re.compile(r"(?:.*\.)?(?P<shape>Solid|Face|Edge|Vertex)(?P<index>\d+)$")
        result = {"Solid": [], "Face": [], "Edge": [], "Vertex": []}
        for element in element_list:
            m = reg_exp.match(element)
            if m:
                result[m.group("shape")].append(m.group("index"))

        return result


    def get_region_data(self):
        # mesh regions
        mesh_region_list = self._get_definitions_of_type("Fem::MeshRegion")
        if not mesh_region_list:
            # print("  No mesh refinements.")
            pass
        else:
            # Console.PrintMessage("  Mesh regions, we need to get the elements.\n")
            # by the use of MeshRegion object and a BooleanSplitCompound
            # there could be problems with node numbers see
            # https://forum.freecad.org/viewtopic.php?f=18&t=18780&start=40#p149467
            # https://forum.freecad.org/viewtopic.php?f=18&t=18780&p=149520#p149520
            part = self.part_obj
            if (
                mesh_region_list
                and part.Shape.ShapeType == "Compound"
                and (
                    femutils.is_of_type(part, "FeatureBooleanFragments")
                    or femutils.is_of_type(part, "FeatureSlice")
                    or femutils.is_of_type(part, "FeatureXOR")
                )
            ):
                self.outputCompoundWarning
            for mr_obj in mesh_region_list:
                if mr_obj.Suppressed:
                    continue

                if mr_obj.CharacteristicLength:
                    if mr_obj.References:

                        elements = self._get_reference_elements(mr_obj, self.region_element_set)
                        if not elements:
                            Console.PrintError( ("The mesh distance {} is not used because no unique"
                                                "elements are selected.\n").format(mr_obj.Name))
                            continue

                        value = Units.Quantity(mr_obj.CharacteristicLength).Value
                        self.ele_length_list.append((value, elements))

                    else:
                        Console.PrintError(
                            "The mesh refinement: {} is not used to create the mesh "
                            "because the reference list is empty.\n".format(mr_obj.Name)
                        )
                else:
                    Console.PrintError(
                        "The mesh refinement: {} is not used to create the "
                        "mesh because the CharacteristicLength is 0.0 mm.\n".format(mr_obj.Name)
                    )

            # Console.PrintMessage("  {}\n".format(self.ele_length_list))

    def get_boundary_layer_data(self):
        # mesh boundary layer
        # currently only one boundary layer setting object is allowed
        # but multiple boundary can be selected
        # Mesh.CharacteristicLengthMin, must be zero
        # or a value less than first inflation layer height

        mesh_boundary_list = self._get_definitions_of_type("Fem::MeshBoundaryLayer")
        if not mesh_boundary_list:
            # print("  No mesh boundary layer setting document object.")
            pass
        else:
            # Console.PrintMessage("  Mesh boundary layers, we need to get the elements.\n")
            if self.part_obj.Shape.ShapeType == "Compound":
                # see https://forum.freecad.org/viewtopic.php?f=18&t=18780&start=40#p149467 and
                # https://forum.freecad.org/viewtopic.php?f=18&t=18780&p=149520#p149520
                self.outputCompoundWarning

            boundary_layer_set = False
            for mr_obj in mesh_boundary_list:
                if mr_obj.Suppressed:
                    continue
                if boundary_layer_set:
                    Console.PrintLog("Boundary layer already set, ignoring {}".format(mr_obj.Name))
                    # continue to get one waring for each ignored object
                    continue

                if mr_obj.MinimumThickness and Units.Quantity(mr_obj.MinimumThickness).Value > 0:
                    if mr_obj.References:

                        # ensure to not have a second valid boundary layer object
                        boundary_layer_set = True

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
                                    "the Shape to mesh :-)\n".format(mr_obj.Name)
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
                                        self.part_obj.Shape, ele_shape
                                    )
                                    if found_element:  # also
                                        elems = found_element
                                    else:
                                        Console.PrintError(
                                            "One element of the mesh boundary layer {} could "
                                            "not be found in the Part to mesh. "
                                            "It will be ignored.\n".format(mr_obj.Name)
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
                                        "to another mesh boundary layer.\n".format(
                                            elems, mr_obj.Name
                                        )
                                    )

                        # Notes:
                        # 1. With gmsh version 4.7 new names for settings have been introduced.
                        #    Due to deprication of old command names we switched to the new ones,
                        #    dropping support for gmsh <4.7 (released Nov. 2020)
                        setting = {}
                        setting["Size"] = Units.Quantity(mr_obj.MinimumThickness).Value
                        setting["Ratio"] = mr_obj.GrowthRate
                        setting["Thickness"] = sum(
                            [
                                setting["Size"] * setting["Ratio"] ** i
                                for i in range(mr_obj.NumberOfLayers)
                            ]
                        )

                        # SizeFar: cell dimension outside boundary
                        # should be set later if some character length is set
                        if (
                            self.clmax > setting["Thickness"] * 0.8
                            and self.clmax < setting["Thickness"] * 1.6
                        ):
                            setting["SizeFar"] = self.clmax
                        else:
                            # set a value for safety, it may works as background mesh cell size
                            setting["SizeFar"] = setting["Thickness"]
                        # from face name -> face id is done in geo file write up
                        # TODO: fan angle setup is not implemented yet

                        setting["CurvesList"] = belem_list
                        self.bl_setting_list.append(setting)
                    else:
                        Console.PrintError(
                            "The mesh boundary layer: {} is not used to create "
                            "the mesh because the reference list is empty.\n".format(mr_obj.Name)
                        )
                else:
                    Console.PrintError(
                        "The mesh boundary layer: {} is not used to create "
                        "the mesh because the min thickness is 0.0 mm.\n".format(mr_obj.Name)
                    )
            Console.PrintMessage(f"  {self.bl_setting_list}\n")


    def get_distance_data(self):
        # mesh distance
        mesh_distance_list = self._get_definitions_of_type("Fem::MeshDistance")
        if not mesh_distance_list:
            # print("  No mesh refinements.")
            pass
        else:
            # Console.PrintMessage("  Mesh distances, we need to get the elements.\n")
            # by the use of MeshRegion object and a BooleanSplitCompound
            # there could be problems with node numbers see
            # https://forum.freecad.org/viewtopic.php?f=18&t=18780&start=40#p149467
            # https://forum.freecad.org/viewtopic.php?f=18&t=18780&p=149520#p149520
            part = self.part_obj
            if (
                mesh_distance_list
                and part.Shape.ShapeType == "Compound"
                and (
                    femutils.is_of_type(part, "FeatureBooleanFragments")
                    or femutils.is_of_type(part, "FeatureSlice")
                    or femutils.is_of_type(part, "FeatureXOR")
                )
            ):
                self.outputCompoundWarning
            for mr_obj in mesh_distance_list:
                if mr_obj.Suppressed:
                    continue
                # print(mr_obj.Name)
                # print(mr_obj.CharacteristicLength)
                # print(Units.Quantity(mr_obj.CharacteristicLength).Value)
                #if mr_obj.CharacteristicLength:
                if mr_obj.References:

                    # collect all elements!
                    elements = self._get_reference_elements(mr_obj, self.dist_element_set)
                    if not elements:
                        Console.PrintError( ("The mesh distance {} is not used because no unique"
                                             "elements are selected.\n").format(mr_obj.Name))
                        continue

                    idx_dict = self._element_list_to_shape_idx_dict(elements)

                    # get the settings!
                    settings = {"Distance":{}, "Threshold":{}}
                    settings["Threshold"]["DistMin"] = Units.Quantity(mr_obj.DistanceMinimum).Value
                    settings["Threshold"]["DistMax"] = Units.Quantity(mr_obj.DistanceMaximum).Value
                    settings["Threshold"]["SizeMin"] = Units.Quantity(mr_obj.SizeMinimum).Value
                    settings["Threshold"]["SizeMax"] = Units.Quantity(mr_obj.SizeMaximum).Value
                    settings["Threshold"]["Sigmoid"] = int(not mr_obj.LinearInterpolation)
                    settings["Threshold"]["StopAtDistMax"] = 1
                    settings["Distance"]["Sampling"] = mr_obj.Sampling
                    if idx_dict["Vertex"]:
                        ids = ", ".join(str(i) for i in idx_dict["Vertex"])
                        settings["Distance"]["PointsList"] = f"{{ {ids} }}"
                    if idx_dict["Edge"]:
                        ids = ", ".join(str(i) for i in idx_dict["Edge"])
                        settings["Distance"]["CurvesList"] = f"{{ {ids} }}"
                    if idx_dict["Face"]:
                        ids = ", ".join(str(i) for i in idx_dict["Face"])
                        settings["Distance"]["SurfacesList"] = f"{{ {ids} }}"

                    # save everything for later processing
                    self.dist_setting_list.append(settings)

                else:
                    Console.PrintError(
                        "The mesh refinement: {} is not used to create the mesh "
                        "because the reference list is empty.\n".format(mr_obj.Name)
                    )


    def get_transfinite_data(self):

        # transfinite curves
        transfinite_curve_list = self._get_definitions_of_type("Fem::MeshTransfiniteCurve")
        if not transfinite_curve_list:
            pass
        else:
            part = self.part_obj
            if (
                transfinite_curve_list
                and part.Shape.ShapeType == "Compound"
                and (
                    femutils.is_of_type(part, "FeatureBooleanFragments")
                    or femutils.is_of_type(part, "FeatureSlice")
                    or femutils.is_of_type(part, "FeatureXOR")
                )
            ):
                self.outputCompoundWarning

            for mr_obj in transfinite_curve_list:
                if mr_obj.Suppressed:
                    continue

                if mr_obj.References:

                    # collect all elements!
                    elements = self._get_reference_elements(mr_obj, self.transfinite_curve_elements)
                    if not elements:
                        Console.PrintError( ("The transfinite curve {} is not used because no unique"
                                             "elements are selected.\n").format(mr_obj.Name))
                        continue

                    idx_dict = self._element_list_to_shape_idx_dict(elements)
                    #if mr_obj.in

                    #if mr_obj.Invert
                    if idx_dict["Edge"]:
                        settings = {}
                        prefix = ""
                        coef = mr_obj.Coefficient

                        if mr_obj.Invert:
                            if mr_obj.Distribution == "Progression":
                                prefix = "-"
                            else:
                                coef = 1.0/coef

                        settings["tag"] = ",".join(str(prefix+i) for i in idx_dict["Edge"])
                        settings["numNodes"] = mr_obj.Number
                        if mr_obj.Distribution != "Constant":
                            settings["meshType"] = mr_obj.Distribution
                            settings["coef"] = coef

                        self.transfinite_curve_settings.append(settings)

                else:
                    Console.PrintError(
                        "The transfinite curve {} is not used to create the mesh "
                        "because the reference list is empty.\n".format(mr_obj.Name)
                    )


    def write_groups(self, geo):
        # find shape type and index from group elements and isolate them from possible prefix
        # for example: "PartObject.Solid2" -> shape: Solid, index: 2
        # we use the element index of FreeCAD which starts with 1 (example: "Face1"),
        # same as Gmsh. For unit test we need them to have a fixed order
        reg_exp = re.compile(r"(?:.*\.)?(?P<shape>Solid|Face|Edge|Vertex)(?P<index>\d+)$")

        if self.group_elements:
            # print("  We are going to have to find elements to make mesh groups for.")
            geo.write("// group data\n")
            for group in sorted(self.group_elements):
                gdata = self.group_elements[group]
                ele = {"Volume": [], "Surface": [], "Line": [], "Point": []}

                for i in gdata:
                    m = reg_exp.match(i)
                    if m:
                        shape = m.group("shape")
                        index = str(m.group("index"))
                        if shape == "Solid":
                            ele["Volume"].append(index)
                        elif shape == "Face":
                            ele["Surface"].append(index)
                        elif shape == "Edge":
                            ele["Line"].append(index)
                        elif shape == "Vertex":
                            ele["Point"].append(index)

                for phys in ele:
                    if ele[phys]:
                        items = "{" + ", ".join(ele[phys]) + "}"
                        geo.write('Physical {}("{}") = {};\n'.format(phys, group, items))

            geo.write("\n")

    def write_regions(self, geo):
        geo.write("// Constant size regions\n")
        if self.ele_length_list:
            # we use the index FreeCAD which starts with 0
            # we need to add 1 for the index in Gmsh
            geo.write("// Constant size field according to  Element length map\n")
            for entry in self.ele_length_list:
                field_id = self._next_field_number()
                element_dict = self._element_list_to_shape_idx_dict(entry[1])

                geo.write(f"Field[{field_id}] = Constant;\n")
                geo.write(f"Field[{field_id}].VIn = {entry[0]};\n")
                if element_dict["Vertex"]:
                    id_list = ", ".join(str(i) for i in element_dict["Vertex"])
                    geo.write(f"Field[{field_id}].PointsList = {{ {id_list} }};\n")
                if element_dict["Edge"]:
                    id_list = ", ".join(str(i) for i in element_dict["Edge"])
                    geo.write(f"Field[{field_id}].CurvesList = {{ {id_list} }};\n")
                if element_dict["Face"]:
                    id_list = ", ".join(str(i) for i in element_dict["Face"])
                    geo.write(f"Field[{field_id}].SurfacesList = {{ {id_list} }};\n")
                if element_dict["Solid"]:
                    id_list = ", ".join(str(i) for i in element_dict["Solid"])
                    geo.write(f"Field[{field_id}].VolumesList = {{ {id_list} }};\n")

            geo.write("\n")

        geo.write("// End of constant size regions\n")
        geo.write("\n")

    def write_boundary_layer(self, geo):
        # currently single body is supported
        if len(self.bl_setting_list):
            geo.write("// boundary layer setting\n")
            Console.PrintMessage("  Start to write boundary layer setup\n")
            for item in self.bl_setting_list:
                field_number = self._next_field_number()
                prefix = "Field[" + str(field_number) + "]"
                geo.write(prefix + " = BoundaryLayer;\n")
                for k in item:
                    v = item[k]
                    if k == "CurvesList":
                        # the element name of FreeCAD which starts
                        # with 1 (example: "Face1"), same as Gmsh
                        # el_id = int(el[4:])  # FIXME:  strip `face` or `edge` prefix
                        ele_nodes = ("".join((str(el[4:]) + ", ") for el in v)).rstrip(", ")
                        line = prefix + "." + str(k) + " = {" + ele_nodes + " };\n"
                        geo.write(line)
                    else:
                        line = prefix + "." + str(k) + " = " + str(v) + ";\n"
                        geo.write(line)
                    Console.PrintMessage(f"{line}\n")
                geo.write("BoundaryLayer Field = " + str(field_number) + ";\n")
                geo.write("// end of this boundary layer setup \n")

            geo.write("\n")
            geo.flush()
            Console.PrintMessage("  finished in boundary layer setup\n")
        else:
            # print("  no boundary layer setup is found for this mesh")
            geo.write("// no boundary layer settings for this mesh\n")

    def write_distances(self, geo):
        # currently single body is supported
        if len(self.dist_setting_list):
            geo.write("// distance settings\n")
            Console.PrintMessage("  Start to write distance setup\n")
            for item in self.dist_setting_list:
                # distance is not a background field
                distance_field_number = self._next_field_number(False)
                prefix = "Field[" + str(distance_field_number) + "]"
                geo.write(prefix + " = Distance;\n")
                for k in item["Distance"]:
                    line = prefix + "." + str(k) + " = " + str(item["Distance"][k]) + ";\n"
                    geo.write(line)
                    Console.PrintMessage(f"{line}\n")

                # threshold is a background field
                threshold_field_number = self._next_field_number()
                prefix = "Field[" + str(threshold_field_number) + "]"
                geo.write(prefix + " = Threshold;\n")
                for k in item["Threshold"]:
                    line = prefix + "." + str(k) + " = " + str(item["Threshold"][k]) + ";\n"
                    geo.write(line)
                    Console.PrintMessage(f"{line}\n")

                line = prefix + ".InField = " + str(distance_field_number) + ";\n"
                geo.write(line)
                Console.PrintMessage(f"{line}\n")

            geo.write("// end of distance setup \n")

            geo.write("\n")
            geo.flush()
            Console.PrintMessage("  finished in distance setup\n")
        else:
            # print("  no boundary layer setup is found for this mesh")
            geo.write("// no distance settings for this mesh\n")


    def write_transfinite(self, geo):

        geo.write("\n")
        geo.write("// Transfinite elements\n")
        for setting in self.transfinite_curve_settings:
            geo.write("Transfinite Curve {")
            geo.write(f"{setting["tag"]} }} = {setting["numNodes"]}")

            if "meshType" in setting:
                geo.write(f" Using {setting["meshType"]} {setting["coef"]}")
            geo.write(";\n")

        geo.write("\n")


    def write_part_file(self):
        global_pla = self.part_obj.getGlobalPlacement()
        geom = self.part_obj.getPropertyOfGeometry()
        # get partner shape
        geom_trans = geom.transformed(FreeCAD.Placement().Matrix)
        geom_trans.Placement = global_pla
        geom_trans.exportBrep(self.temp_file_geometry)

    def write_geo(self):
        temp_dir = os.path.dirname(self.temp_file_geo)
        geo = open(self.temp_file_geo, "w")
        geo.write("// geo file for meshing with Gmsh meshing software created by FreeCAD\n")
        geo.write("\n")

        cpu_count = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetInt(
            "NumOfThreads", QThread.idealThreadCount()
        )
        geo.write("// enable multi-core processing\n")
        geo.write(f"General.NumThreads = {cpu_count};\n")
        geo.write("\n")

        geo.write("// open brep geometry\n")
        # explicit use double quotes in geo file
        geo.write(f'Merge "{os.path.relpath(self.temp_file_geometry, temp_dir)}";\n')
        geo.write("\n")

        # groups
        self.write_groups(geo)

        # Constant size fields
        self.write_regions(geo)

        # Distance size fields
        self.write_distances(geo)

        # boundary layer generation may need special setup
        # of Gmsh properties, set them in Gmsh TaskPanel
        self.write_boundary_layer(geo)

        # transfinite elements
        self.write_transfinite(geo)

        # write the background size field, if fields have been added
        if self._background_fields:

            # background field
            field_id = self._next_field_number(False)
            geo.write(f"\nField[{field_id}] = Min;\n")
            id_list = ", ".join(str(i) for i in self._background_fields)
            geo.write(f"Field[{field_id}].FieldsList = {{ {id_list} }};\n")
            geo.write(f"Background Field = {field_id};\n\n")

            geo.write("Mesh.MeshSizeExtendFromBoundary = 0;\n")
            geo.write("\n")


        # mesh parameter
        geo.write("// min, max Characteristic Length\n")
        geo.write("Mesh.MeshSizeMax = " + str(self.clmax) + ";\n")
        if len(self.bl_setting_list):
            # if minLength must smaller than first layer of boundary_layer
            # it is safer to set it as zero (default value) to avoid error
            geo.write("Mesh.MeshSizeMin = " + str(0) + ";\n")
        else:
            geo.write("Mesh.MeshSizeMin = " + str(self.clmin) + ";\n")
        if hasattr(self.mesh_obj, "MeshSizeFromCurvature"):
            geo.write(
                "Mesh.MeshSizeFromCurvature = {}"
                "; // number of elements per 2*pi radians, 0 to deactivate\n".format(
                    self.mesh_obj.MeshSizeFromCurvature
                )
            )
        geo.write("Mesh.MeshSizeFromPoints = 0;\n")
        geo.write("\n")


        if hasattr(self.mesh_obj, "RecombineAll") and self.mesh_obj.RecombineAll is True:
            geo.write("// recombination for surfaces\n")
            geo.write("Mesh.RecombineAll = 1;\n")
        if hasattr(self.mesh_obj, "Recombine3DAll") and self.mesh_obj.Recombine3DAll is True:
            geo.write("// recombination for volumes\n")
            geo.write("Mesh.Recombine3DAll = 1;\n")
        if (hasattr(self.mesh_obj, "RecombineAll") and self.mesh_obj.RecombineAll is True) or (
            hasattr(self.mesh_obj, "Recombine3DAll") and self.mesh_obj.Recombine3DAll is True
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
            "5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad, 9=Packing Parallelograms, 11=Quasi-structured Quad)\n"
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

        geo.write("// subdivision algorithm\n")
        geo.write("Mesh.SubdivisionAlgorithm = " + self.SubdivisionAlgorithm + ";\n")
        geo.write("\n")

        geo.write("// incomplete second order elements\n")
        if (
            self.SubdivisionAlgorithm == "1"
            or self.SubdivisionAlgorithm == "2"
            or self.mesh_obj.RecombineAll
        ):
            sec_order_inc = "1"
        else:
            sec_order_inc = "0"
        geo.write("Mesh.SecondOrderIncomplete = " + sec_order_inc + ";\n")
        geo.write("\n")

        geo.write("// meshing\n")
        # remove duplicate vertices
        # see https://forum.freecad.org/viewtopic.php?f=18&t=21571&start=20#p179443
        if hasattr(self.mesh_obj, "CoherenceMesh") and self.mesh_obj.CoherenceMesh is True:
            geo.write(
                "Geometry.Tolerance = {}; // set geometrical "
                "tolerance (also used for merging nodes)\n".format(self.geotol)
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
                "for one material there is no group defined;\n"
            )
        geo.write("// Ignore Physical definitions and save all elements;\n")
        geo.write("Mesh.SaveAll = 1;\n")
        # explicit use double quotes in geo file
        geo.write(f'Save "{os.path.relpath(self.temp_file_mesh, temp_dir)}";\n')
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
                stderr=subprocess.PIPE,
                startupinfo=femutils.startProgramInfo("hide"),
            )
            output, error = p.communicate()
            error = error.decode("utf-8")
            # stdout is still cut at some point
            # but the warnings are in stderr and thus printed :-)
            # print(output)
            # print(error)
        except Exception:
            if shutil.which(self.gmsh_bin):
                error = "Error executing: {}\n".format(" ".join(command_list))
            else:
                error = f"Gmsh executable not found: '{self.gmsh_bin}'\n"
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
            "and the mesh has mesh refinements list.\n"
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
femmesh_obj.Shape = box_obj
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
    femmesh_obj.Shape = box_obj
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
