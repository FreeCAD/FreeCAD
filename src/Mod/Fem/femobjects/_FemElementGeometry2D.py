# ***************************************************************************
# *                                                                         *
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
__url__ = "https://www.freecadweb.org"

## @package FemElementGeometry2D
#  \ingroup FEM
#  \brief FreeCAD FEM element geometry 2D object


class _FemElementGeometry2D:
    "The FemElementGeometry2D object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength", "Thickness", "ShellThickness", "set thickness of the shell elements")
        obj.addProperty("App::PropertyLinkSubList", "References", "ShellThickness", "List of shell thickness shapes")
        obj.Proxy = self
        self.Type = "Fem::FemElementGeometry2D"

    def execute(self, obj):
        return
