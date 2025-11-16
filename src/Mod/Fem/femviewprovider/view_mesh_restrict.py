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

__title__ = "FreeCAD FEM mesh restrict ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_mesh_restrict
#  \ingroup FEM
#  \brief view provider for mesh restrict object

import FreeCAD

from femtaskpanels import task_mesh_restrict
from . import view_base_femmeshelement

from femtools import femutils as utils


def find_parent_gmsh(obj):
    # searches uptream for the parent gmsh mesh object
    for parent in obj.InList:
        if utils.is_of_type(parent, "Fem::FemMeshGmsh"):
            return parent

        gmsh = find_parent_gmsh(parent)
        if gmsh:
            return gmsh

    return None


class VPMeshRestrict(view_base_femmeshelement.VPBaseFemMeshElement):
    """
    A View Provider for the FemMeshRestrict object
    """

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_mesh_restrict._TaskPanel)

    def canDropObjects(self):
        return True

    def canDragObjects(self):
        return True

    def canDropObject(self, incoming_object):
        # check if object is a gmsh field based refinement
        return True

    def canDragObject(self, outgoing_object):
        return True

    def dropObject(self, vp, obj):

        if self.Object.Refinement:

            # wee need to pass the old refinement to the gmsh object we are in
            gmsh = find_parent_gmsh(self.Object)
            if not gmsh:
                raise FreeCAD.FreeCADError("Restrict object not within GMSH mesh object")

            refinements = gmsh.MeshRefinementList
            refinements.append(self.Object.Refinement)
            gmsh.MeshRefinementList = refinements

        self.Object.Refinement = obj

    def dragObject(self, vp, obj):
        if obj == self.Object.Refinement:
            self.Object.Refinement = None

    def claimChildren(self):
        return [self.Object.Refinement]
