
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

    Type = "Fem::MeshShape"

    def __init__(self, obj):
        obj.addExtension("Fem::BoxExtensionPython")
        obj.addExtension("Fem::SphereExtensionPython")
        obj.addExtension("Fem::CylinderExtensionPython")
        super().__init__(obj)

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ShapeType",
                group="Mesh",
                doc="Mesh shape to be used",
                value = ["Box", "Sphere", "Cylinder"],
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeIn",
                group="Mesh",
                doc="Mesh size within the sphere",
                value="100mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeOut",
                group="Mesh",
                doc="Mesh size outside of the sphere",
                value="1000mm",
            ),
             _PropHelper(
                type="App::PropertyLength",
                name="Thickness",
                group="Mesh",
                doc="Thickness of transition layer between in/out mesh sizes (added outside of the sphere)",
                value="0mm",
            )
        ]

        return props

    def onChanged(self, obj, prop):

        if "Center" in prop:
            if hasattr(self, "_block_center_change") and self._block_center_change:
                return

            # make sure all 3 shape centers are equal
            self._block_center_change = True
            match prop:
                case "BoxCenter":
                    obj.SphereCenter = obj.BoxCenter
                    obj.CylinderCenter = obj.BoxCenter
                case "SphereCenter":
                    obj.BoxCenter = obj.SphereCenter
                    obj.CylinderCenter = obj.SphereCenter
                case "CylinderCenter":
                    obj.SphereCenter = obj.CylinderCenter
                    obj.BoxCenter = obj.CylinderCenter
            self._block_center_change = False

