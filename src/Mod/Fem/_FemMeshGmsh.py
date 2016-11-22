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
    known_element_dimensions = ['Auto', '1D', '2D', '3D']
    known_element_orders = ['Auto', '1st', '2nd']

    def __init__(self, obj):
        self.Type = "FemMeshGmsh"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        obj.addProperty("App::PropertyLink", "Part", "FEM Mesh", "Part object to mesh")
        obj.Part = None

        obj.addProperty("App::PropertyFloat", "ElementSizeMax", "FEM Mesh Params", "Max mesh element size (0.0 = infinity)")
        obj.ElementSizeMax = 0.0  # will be 1e+22

        obj.addProperty("App::PropertyFloat", "ElementSizeMin", "FEM Mesh Params", "Min mesh element size")
        obj.ElementSizeMin = 0.0

        obj.addProperty("App::PropertyEnumeration", "ElementDimension", "FEM Mesh Params", "Dimension of mesh elements (Auto = according ShapeType of part to mesh)")
        obj.ElementDimension = _FemMeshGmsh.known_element_dimensions
        obj.ElementDimension = 'Auto'  # according ShapeType of Part to mesh

        obj.addProperty("App::PropertyEnumeration", "ElementOrder", "FEM Mesh Params", "Order of mesh elements (Auto will be 2nd)")
        obj.ElementOrder = _FemMeshGmsh.known_element_orders
        obj.ElementOrder = 'Auto'  # = 2nd

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
