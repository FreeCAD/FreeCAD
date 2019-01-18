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

__title__ = "FreeCAD FEM mesh group document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemMeshGroup
#  \ingroup FEM
#  \brief FreeCAD FEM _FemMeshGroup


class _FemMeshGroup:
    "The FemMeshGroup object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyBool", "UseLabel", "MeshGroupProperties", "The identifier used for export (True: Label, False: Name)")
        obj.addProperty("App::PropertyLinkSubList", "References", "MeshGroupShapes", "List of FEM mesh group shapes")
        obj.Proxy = self
        self.Type = "Fem::FemMeshGroup"

    def execute(self, obj):
        return
