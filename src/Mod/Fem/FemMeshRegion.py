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

__title__ = "FemMeshRegion"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import _FemMeshRegion


def makeFemMeshRegion(base_mesh, element_length=0.0, name="FEMMeshRegion"):
    '''makeFemMeshRegion([length], [name]): creates a  FEM mesh region object to define properties for a regon of a FEM mesh'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
    _FemMeshRegion._FemMeshRegion(obj)
    obj.CharacteristicLength = element_length
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh region obj. to the list
    tmplist = base_mesh.MeshRegionList
    tmplist.append(obj)
    base_mesh.MeshRegionList = tmplist
    if FreeCAD.GuiUp:
        import _ViewProviderFemMeshRegion
        _ViewProviderFemMeshRegion._ViewProviderFemMeshRegion(obj.ViewObject)
    return obj

#  @}
