# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element geometry 2D document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package element_geometry2D
#  \ingroup FEM
#  \brief element geometry 2D object

from . import base_femelement
from .base_fempythonobject import _PropHelper


class ElementGeometry2D(base_femelement.BaseFemElement):
    """
    The ElementGeometry2D object
    """

    Type = "Fem::ElementGeometry2D"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = super()._get_properties()

        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="Thickness",
                group="ShellThickness",
                doc="Set thickness of the shell elements",
                value="0 mm",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="Offset",
                group="ShellThickness",
                doc="Set thickness offset of the shell elements",
                value=0.0,
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        super().onDocumentRestored(obj)
