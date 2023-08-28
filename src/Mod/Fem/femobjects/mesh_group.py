# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM mesh group document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package mesh_group
#  \ingroup FEM
#  \brief mesh group object

from . import base_fempythonobject


class MeshGroup(base_fempythonobject.BaseFemPythonObject):
    """
    The MeshGroup object
    """

    Type = "Fem::MeshGroup"

    def __init__(self, obj):
        super(MeshGroup, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "UseLabel",
            "MeshGroupProperties",
            "The identifier used for export (True: Label, False: Name)"
        )

        obj.addProperty(
            "App::PropertyLinkSubList",
            "References",
            "MeshGroupShapes",
            "List of FEM mesh group shapes"
        )
