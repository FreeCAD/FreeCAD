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

__title__ = "FreeCAD FEM mesh boundary layer document object"
__author__ = "Bernd Hahnebach, Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

## @package FemMeshBoundaryLayer
#  \ingroup FEM
#  \brief FEM mesh boundary layer object


class _FemMeshBoundaryLayer:
    "The FemMeshBoundaryLayer object"
    def __init__(self, obj):
        self.Type = "Fem::FemMeshBoundaryLayer"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to this object

        obj.addProperty("App::PropertyInteger", "NumberOfLayers", "MeshBoundaryLayerProperties", "set number of inflation layers for this boundary")

        obj.NumberOfLayers = 3

        obj.addProperty("App::PropertyLength", "MinimumThickness", "MeshBoundaryLayerProperties", "set minimum thickness,usually the first inflation layer")
        # default to zero, user must specify a proper value for this property

        obj.addProperty("App::PropertyFloat", "GrowthRate", "MeshBoundaryLayerProperties", "set growth rate of inflation layers for smooth transition")

        obj.GrowthRate = 1.5

        obj.addProperty("App::PropertyLinkSubList", "References", "MeshBoundaryLayerShapes", "List of FEM mesh region shapes")

    def execute(self, obj):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
