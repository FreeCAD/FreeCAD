
# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net               *
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

__title__ = "FreeCAD FEM mesh shape based refinments"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_shape
#  \ingroup FEM
#  \brief  object defining the mesh size within / outside of shapes

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshShape(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshShape object
    """
    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLength",
                name="SizeIn",
                group="MeshSize",
                doc="Mesh size within the sphere",
                value="100mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeOut",
                group="MeshSize",
                doc="Mesh size outside of the sphere",
                value="1000mm",
            ),
        ]

        return super()._get_properties() + props


class MeshSphere(MeshShape):

    Type = "Fem::MeshSphere"

    def __init__(self, obj):
        obj.addExtension("Fem::SphereExtensionPython")
        super().__init__(obj)

    def _get_properties(self):
        props = super()._get_properties()
        props.append(
            _PropHelper(
                type="App::PropertyLength",
                name="Thickness",
                group="MeshSize",
                doc="Thickness of transition layer between in/out mesh sizes (added outside of the sphere)",
                value="0mm",
            ))
        return props


class MeshBox(MeshShape):

    Type = "Fem::MeshBox"

    def __init__(self, obj):
        obj.addExtension("Fem::BoxExtensionPython")
        super().__init__(obj)

    def _get_properties(self):
        props = super()._get_properties()
        props.append(
            _PropHelper(
                type="App::PropertyLength",
                name="Thickness",
                group="MeshSize",
                doc="Thickness of transition layer between in/out mesh sizes (added outside of the sphere)",
                value="0mm",
            ))
        return props


class MeshCylinder(MeshShape):

    Type = "Fem::MeshCylinder"

    def __init__(self, obj):
        obj.addExtension("Fem::CylinderExtensionPython")
        super().__init__(obj)
