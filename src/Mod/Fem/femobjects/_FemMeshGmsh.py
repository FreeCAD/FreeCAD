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

__title__ = "FreeCAD FEM mesh gmsh document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemMeshGmsh
#  \ingroup FEM
#  \brief FreeCAD FEM _FemMeshGmsh


class _FemMeshGmsh():
    """A Fem::FemMeshObject python type, add Gmsh specific properties
    """

    # they will be used from the task panel too, thus they need to be outside of the __init__
    known_element_dimensions = ['From Shape', '1D', '2D', '3D']
    known_element_orders = ['1st', '2nd']
    known_mesh_algorithm_2D = [
        'Automatic',
        'MeshAdapt',
        'Delaunay',
        'Frontal',
        'BAMG',
        'DelQuad'
    ]
    known_mesh_algorithm_3D = [
        'Automatic',
        'Delaunay',
        'New Delaunay',
        'Frontal',
        'Frontal Delaunay',
        'Frontal Hex',
        'MMG3D',
        'R-tree'
    ]

    def __init__(self, obj):
        self.Type = "Fem::FemMeshGmsh"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

        obj.addProperty(
            "App::PropertyLinkList",
            "MeshBoundaryLayerList",
            "Base",
            "Mesh boundaries need inflation layers"
        )
        obj.MeshBoundaryLayerList = []

        obj.addProperty(
            "App::PropertyLinkList",
            "MeshRegionList",
            "Base",
            "Mesh regions of the mesh"
        )
        obj.MeshRegionList = []

        obj.addProperty(
            "App::PropertyLinkList",
            "MeshGroupList",
            "Base",
            "Mesh groups of the mesh"
        )
        obj.MeshGroupList = []

        obj.addProperty(
            "App::PropertyLink",
            "Part",
            "FEM Mesh",
            "Geometry object, the mesh is made from. The geometry object has to have a Shape."
        )
        obj.Part = None

        obj.addProperty(
            "App::PropertyLength",
            "CharacteristicLengthMax",
            "FEM Gmsh Mesh Params",
            "Max mesh element size (0.0 = infinity)"
        )
        obj.CharacteristicLengthMax = 0.0  # will be 1e+22

        obj.addProperty(
            "App::PropertyLength",
            "CharacteristicLengthMin",
            "FEM Gmsh Mesh Params",
            "Min mesh element size"
        )
        obj.CharacteristicLengthMin = 0.0

        obj.addProperty(
            "App::PropertyEnumeration",
            "ElementDimension",
            "FEM Gmsh Mesh Params",
            "Dimension of mesh elements (Auto = according ShapeType of part to mesh)"
        )
        obj.ElementDimension = _FemMeshGmsh.known_element_dimensions
        obj.ElementDimension = 'From Shape'  # according ShapeType of Part to mesh

        obj.addProperty(
            "App::PropertyEnumeration",
            "ElementOrder",
            "FEM Gmsh Mesh Params",
            "Order of mesh elements"
        )
        obj.ElementOrder = _FemMeshGmsh.known_element_orders
        obj.ElementOrder = '2nd'

        obj.addProperty(
            "App::PropertyBool",
            "OptimizeStd",
            "FEM Gmsh Mesh Params",
            "Optimize tetra elements"
        )
        obj.OptimizeStd = True

        obj.addProperty(
            "App::PropertyBool",
            "OptimizeNetgen",
            "FEM Gmsh Mesh Params",
            "Optimize tetra elements by use of Netgen"
        )
        obj.OptimizeNetgen = False

        obj.addProperty(
            "App::PropertyBool",
            "HighOrderOptimize",
            "FEM Gmsh Mesh Params",
            "Optimize high order meshes"
        )
        obj.HighOrderOptimize = False

        obj.addProperty(
            "App::PropertyBool",
            "RecombineAll",
            "FEM Gmsh Mesh Params",
            "Apply recombination algorithm to all surfaces"
        )
        obj.RecombineAll = False

        obj.addProperty(
            "App::PropertyBool",
            "CoherenceMesh",
            "FEM Gmsh Mesh Params",
            "Removes all duplicate mesh vertices"
        )
        obj.CoherenceMesh = True

        obj.addProperty(
            "App::PropertyFloat",
            "GeometryTolerance",
            "FEM Gmsh Mesh Params",
            "Geometrical Tolerance (0.0 = GMSH std = 1e-08)"
        )
        obj.GeometryTolerance = 1e-06

        obj.addProperty(
            "App::PropertyEnumeration",
            "Algorithm2D",
            "FEM Gmsh Mesh Params",
            "mesh algorithm 2D"
        )
        obj.Algorithm2D = _FemMeshGmsh.known_mesh_algorithm_2D
        obj.Algorithm2D = 'Automatic'  # ?

        obj.addProperty(
            "App::PropertyEnumeration",
            "Algorithm3D",
            "FEM Gmsh Mesh Params",
            "mesh algorithm 3D"
        )
        obj.Algorithm3D = _FemMeshGmsh.known_mesh_algorithm_3D
        obj.Algorithm3D = 'Automatic'  # ?

        obj.addProperty(
            "App::PropertyBool",
            "GroupsOfNodes",
            "FEM Gmsh Mesh Params",
            "For each group create not only the elements but the nodes too."
        )
        obj.GroupsOfNodes = False

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
