# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Stefan Tröger <stefantroeger@gmx.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

__title__ = "FreeCAD FEM mesh shape ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_mesh_shape
#  \ingroup FEM
#  \brief view provider for mesh shape objects

from femtaskpanels import task_mesh_shape
from . import view_base_femmeshelement


class VPMeshShape(view_base_femmeshelement.VPBaseFemMeshElement):
    """
    A View Provider for the FemMeshSphere object
    """

    def __init__(self, vobj):
        vobj.addExtension("FemGui::ViewProviderBoxExtensionPython")
        vobj.addExtension("FemGui::ViewProviderSphereExtensionPython")
        vobj.addExtension("FemGui::ViewProviderCylinderExtensionPython")
        super().__init__(vobj)

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_mesh_shape._TaskPanelShape)

    def getDisplayModes(self, obj):
        """Return a list of display modes."""
        modes = ["Box", "Sphere", "Cylinder"]
        return modes

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in getDisplayModes."""
        return "Box"

    def updateData(self, obj, prop):

        if prop == "ShapeType":
            self.ViewObject.DisplayMode = obj.ShapeType
            self.ViewObject.signalChangeIcon()

    def getIcon(self):
        return f":icons/FEM_Mesh{self.Object.ShapeType}.svg"
