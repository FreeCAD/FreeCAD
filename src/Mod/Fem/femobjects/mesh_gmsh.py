# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM mesh gmsh document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package mesh_gmsh
#  \ingroup FEM
#  \brief mesh gmsh object

from . import base_fempythonobject


class MeshGmsh(base_fempythonobject.BaseFemPythonObject):
    """
    A Fem::FemMeshObject python type, add Gmsh specific properties
    """

    Type = "Fem::FemMeshGmsh"

    # they will be used from the task panel too, thus they need to be outside of the __init__
    known_element_dimensions = ["From Shape", "1D", "2D", "3D"]
    known_element_orders = ["1st", "2nd"]
    known_mesh_algorithm_2D = [
        "Automatic",
        "MeshAdapt",
        "Delaunay",
        "Frontal",
        "BAMG",
        "DelQuad",
        "Packing Parallelograms"
    ]
    known_mesh_algorithm_3D = [
        "Automatic",
        "Delaunay",
        "New Delaunay",
        "Frontal",
        "MMG3D",
        "R-tree",
        "HXT"
    ]
    known_mesh_RecombinationAlgorithms = [
        "Simple",
        "Blossom",
        "Simple full-quad",
        "Blossom full-quad"
    ]
    known_mesh_HighOrderOptimizers = [
        "None",
        "Optimization",
        "Elastic+Optimization",
        "Elastic",
        "Fast curving"
    ]

    def __init__(self, obj):
        super(MeshGmsh, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):

        # HighOrderOptimize
        # was once App::PropertyBool, so check this
        high_order_optimizer = ""
        if obj.HighOrderOptimize is True:
            high_order_optimizer = "Optimization"
        elif obj.HighOrderOptimize is False:
            high_order_optimizer = "None"
        obj.removeProperty("HighOrderOptimize")
        # add new HighOrderOptimize property
        self.add_properties(obj)
        # write the stored high_order_optimizer
        if high_order_optimizer:
            obj.HighOrderOptimize = high_order_optimizer

        # Algorithm3D
        # refresh the list of known 3D algorithms for existing meshes
        # since some algos are meanwhile deprecated and new algos are available
        obj.Algorithm3D = MeshGmsh.known_mesh_algorithm_3D

    def add_properties(self, obj):

        # this method is called from onDocumentRestored
        # thus only add and or set a attribute
        # if the attribute does not exist

        if not hasattr(obj, "MeshBoundaryLayerList"):
            obj.addProperty(
                "App::PropertyLinkList",
                "MeshBoundaryLayerList",
                "Base",
                "Mesh boundaries need inflation layers"
            )
            obj.MeshBoundaryLayerList = []

        if not hasattr(obj, "MeshRegionList"):
            obj.addProperty(
                "App::PropertyLinkList",
                "MeshRegionList",
                "Base",
                "Mesh regions of the mesh"
            )
            obj.MeshRegionList = []

        if not hasattr(obj, "MeshGroupList"):
            obj.addProperty(
                "App::PropertyLinkList",
                "MeshGroupList",
                "Base",
                "Mesh groups of the mesh"
            )
            obj.MeshGroupList = []

        if not hasattr(obj, "Part"):
            obj.addProperty(
                "App::PropertyLink",
                "Part",
                "FEM Mesh",
                "Geometry object, the mesh is made from. The geometry object has to have a Shape."
            )
            obj.Part = None

        if not hasattr(obj, "CharacteristicLengthMax"):
            obj.addProperty(
                "App::PropertyLength",
                "CharacteristicLengthMax",
                "FEM Gmsh Mesh Params",
                "Max mesh element size (0.0 = infinity)"
            )
            obj.CharacteristicLengthMax = 0.0  # will be 1e+22

        if not hasattr(obj, "CharacteristicLengthMin"):
            obj.addProperty(
                "App::PropertyLength",
                "CharacteristicLengthMin",
                "FEM Gmsh Mesh Params",
                "Min mesh element size"
            )
            obj.CharacteristicLengthMin = 0.0

        if not hasattr(obj, "ElementDimension"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ElementDimension",
                "FEM Gmsh Mesh Params",
                "Dimension of mesh elements (Auto = according ShapeType of part to mesh)"
            )
            obj.ElementDimension = MeshGmsh.known_element_dimensions
            obj.ElementDimension = "From Shape"  # according ShapeType of Part to mesh

        if not hasattr(obj, "ElementOrder"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ElementOrder",
                "FEM Gmsh Mesh Params",
                "Order of mesh elements"
            )
            obj.ElementOrder = MeshGmsh.known_element_orders
            obj.ElementOrder = "2nd"

        if not hasattr(obj, "OptimizeStd"):
            obj.addProperty(
                "App::PropertyBool",
                "OptimizeStd",
                "FEM Gmsh Mesh Params",
                "Optimize tetrahedral elements"
            )
            obj.OptimizeStd = True

        if not hasattr(obj, "OptimizeNetgen"):
            obj.addProperty(
                "App::PropertyBool",
                "OptimizeNetgen",
                "FEM Gmsh Mesh Params",
                "Optimize tetra elements by use of Netgen"
            )
            obj.OptimizeNetgen = False

        if not hasattr(obj, "HighOrderOptimize"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "HighOrderOptimize",
                "FEM Gmsh Mesh Params",
                "Optimization of high order meshes"
            )
            obj.HighOrderOptimize = MeshGmsh.known_mesh_HighOrderOptimizers
            obj.HighOrderOptimize = "None"

        if not hasattr(obj, "RecombineAll"):
            obj.addProperty(
                "App::PropertyBool",
                "RecombineAll",
                "FEM Gmsh Mesh Params",
                "Apply recombination algorithm to all surfaces"
            )
            obj.RecombineAll = False

        if not hasattr(obj, "Recombine3DAll"):
            obj.addProperty(
                "App::PropertyBool",
                "Recombine3DAll",
                "FEM Gmsh Mesh Params",
                "Apply recombination algorithm to all volumes"
            )
            obj.Recombine3DAll = False

        if not hasattr(obj, "RecombinationAlgorithm"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "RecombinationAlgorithm",
                "FEM Gmsh Mesh Params",
                "Recombination algorithm"
            )
            obj.RecombinationAlgorithm = MeshGmsh.known_mesh_RecombinationAlgorithms
            obj.RecombinationAlgorithm = "Simple"

        if not hasattr(obj, "CoherenceMesh"):
            obj.addProperty(
                "App::PropertyBool",
                "CoherenceMesh",
                "FEM Gmsh Mesh Params",
                "Removes all duplicate mesh vertices"
            )
            obj.CoherenceMesh = True

        if not hasattr(obj, "GeometryTolerance"):
            obj.addProperty(
                "App::PropertyFloat",
                "GeometryTolerance",
                "FEM Gmsh Mesh Params",
                "Geometrical Tolerance (0.0 = GMSH std = 1e-08)"
            )
            obj.GeometryTolerance = 1e-06

        if not hasattr(obj, "SecondOrderLinear"):
            obj.addProperty(
                "App::PropertyBool",
                "SecondOrderLinear",
                "FEM Gmsh Mesh Params",
                "Second order nodes are created by linear interpolation"
            )
            obj.SecondOrderLinear = False
            # gives much better meshes in the regard of nonpositive jacobians
            # but
            # on curved faces the constraint nodes will no longer found
            # thus standard will be False
            # https://forum.freecad.org/viewtopic.php?t=41738
            # https://forum.freecad.org/viewtopic.php?f=18&t=45260&start=20#p389494

        if not hasattr(obj, "MeshSizeFromCurvature"):
            obj.addProperty(
                "App::PropertyIntegerConstraint",
                "MeshSizeFromCurvature",
                "FEM Gmsh Mesh Params",
                "number of elements per 2*pi radians, 0 to deactivate"
            )
            obj.MeshSizeFromCurvature = (12, 0, 10000, 1)

        if not hasattr(obj, "Algorithm2D"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Algorithm2D",
                "FEM Gmsh Mesh Params",
                "mesh algorithm 2D"
            )
            obj.Algorithm2D = MeshGmsh.known_mesh_algorithm_2D
            obj.Algorithm2D = "Automatic"

        if not hasattr(obj, "Algorithm3D"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Algorithm3D",
                "FEM Gmsh Mesh Params",
                "mesh algorithm 3D"
            )
            obj.Algorithm3D = MeshGmsh.known_mesh_algorithm_3D
            obj.Algorithm3D = "Automatic"

        if not hasattr(obj, "GroupsOfNodes"):
            obj.addProperty(
                "App::PropertyBool",
                "GroupsOfNodes",
                "FEM Gmsh Mesh Params",
                "For each group create not only the elements but the nodes too."
            )
            obj.GroupsOfNodes = False
