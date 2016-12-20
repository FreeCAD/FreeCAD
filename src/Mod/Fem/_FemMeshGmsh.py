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

__title__ = "_FemMeshGmsh"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemMeshGmsh
#  \ingroup FEM


class _FemMeshGmsh():
    """The Fem::FemMeshObject's Proxy python type, add GMSH specific properties
    """

    # they will be used from the task panel too, thus they need to be outside of the __init__
    known_element_dimensions = ['Automatic', '1D', '2D', '3D']
    known_element_orders = ['Automatic', '1st', '2nd']
    known_mesh_algorithm_2D = ['Automatic', 'MeshAdapt', 'Delaunay', 'Frontal', 'BAMG', 'DelQuad']
    known_mesh_algorithm_3D = ['Automatic', 'Delaunay', 'New Delaunay', 'Frontal', 'Frontal Delaunay', 'Frontal Hex', 'MMG3D', 'R-tree']

    def __init__(self, obj):
        self.Type = "FemMeshGmsh"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        obj.addProperty("App::PropertyLink", "Part", "FEM Mesh", "Part object to mesh")
        obj.Part = None

        obj.addProperty("App::PropertyLength", "CharacteristicLengthMax", "FEM GMSH Mesh Params", "Max mesh element size (0.0 = infinity)")
        obj.CharacteristicLengthMax = 0.0  # will be 1e+22

        obj.addProperty("App::PropertyLength", "CharacteristicLengthMin", "FEM GMSH Mesh Params", "Min mesh element size")
        obj.CharacteristicLengthMin = 0.0

        obj.addProperty("App::PropertyEnumeration", "ElementDimension", "FEM GMSH Mesh Params", "Dimension of mesh elements (Auto = according ShapeType of part to mesh)")
        obj.ElementDimension = _FemMeshGmsh.known_element_dimensions
        obj.ElementDimension = 'Automatic'  # according ShapeType of Part to mesh

        obj.addProperty("App::PropertyEnumeration", "ElementOrder", "FEM GMSH Mesh Params", "Order of mesh elements (Auto will be 2nd)")
        obj.ElementOrder = _FemMeshGmsh.known_element_orders
        obj.ElementOrder = 'Automatic'  # = 2nd

        obj.addProperty("App::PropertyBool", "OptimizeStd", "FEM GMSH Mesh Params", "Optimize tetra elements")
        obj.OptimizeStd = True

        obj.addProperty("App::PropertyBool", "OptimizeNetgen", "FEM GMSH Mesh Params", "Optimize tetra elements by use of Netgen")
        obj.OptimizeNetgen = False

        obj.addProperty("App::PropertyBool", "HighOrderOptimize", "FEM GMSH Mesh Params", "Optimize hight order meshes")
        obj.HighOrderOptimize = False

        obj.addProperty("App::PropertyBool", "RecombineAll", "FEM GMSH Mesh Params", "Apply recombination algorithm to all surfaces")
        obj.RecombineAll = False

        obj.addProperty("App::PropertyEnumeration", "Algorithm2D", "FEM GMSH Mesh Params", "mesh algorithm 2D")
        obj.Algorithm2D = _FemMeshGmsh.known_mesh_algorithm_2D
        obj.Algorithm2D = 'Automatic'  # ?

        obj.addProperty("App::PropertyEnumeration", "Algorithm3D", "FEM GMSH Mesh Params", "mesh algorithm 3D")
        obj.Algorithm3D = _FemMeshGmsh.known_mesh_algorithm_3D
        obj.Algorithm3D = 'Automatic'  # ?

        obj.addProperty("App::PropertyMap", "CharacteristicLengthMap", "FEM GMSH Mesh Params", "Map of CharacteristicLength of Shape elements")

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
