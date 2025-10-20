# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "FreeCAD FEM mesh shape ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_mesh_shape
#  \ingroup FEM
#  \brief view provider for mesh shape objects

from femtaskpanels import task_mesh_shape
from . import view_base_femmeshelement


class VPMeshSphere(view_base_femmeshelement.VPBaseFemMeshElement):
    """
    A View Provider for the FemMeshSphere object
    """

    def __init__(self, vobj):
        vobj.addExtension("FemGui::ViewProviderSphereExtensionPython")
        super().__init__(vobj)

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_mesh_shape._TaskPanelSphere)

    def getDisplayModes(self,obj):
        '''Return a list of display modes.'''
        modes=[]
        modes.append("Default")
        return modes

    def getDefaultDisplayMode(self):
        '''Return the name of the default display mode. It must be defined in getDisplayModes.'''
        return "Default"

class VPMeshBox(view_base_femmeshelement.VPBaseFemMeshElement):
    """
    A View Provider for the FemMeshBox object
    """

    def __init__(self, vobj):
        vobj.addExtension("FemGui::ViewProviderBoxExtensionPython")
        super().__init__(vobj)

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_mesh_shape._TaskPanelBox)

    def getDisplayModes(self,obj):
        '''Return a list of display modes.'''
        modes=[]
        modes.append("Default")
        return modes

    def getDefaultDisplayMode(self):
        '''Return the name of the default display mode. It must be defined in getDisplayModes.'''
        return "Default"

class VPMeshCylinder(view_base_femmeshelement.VPBaseFemMeshElement):
    """
    A View Provider for the FemMeshCylinder object
    """

    def __init__(self, vobj):
        vobj.addExtension("FemGui::ViewProviderCylinderExtensionPython")
        super().__init__(vobj)

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_mesh_shape._TaskPanelCylinder)

    def getDisplayModes(self,obj):
        '''Return a list of display modes.'''
        modes=[]
        modes.append("Default")
        return modes

    def getDefaultDisplayMode(self):
        '''Return the name of the default display mode. It must be defined in getDisplayModes.'''
        return "Default"
