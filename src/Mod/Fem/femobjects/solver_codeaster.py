# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver object Code Aster"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## @package SolverCodeAster
#  \ingroup FEM
#  \brief solver Code Aster object

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class SolverCodeAster(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::SolverCodeAster"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="AnalysisType",
                group="Solver",
                doc="Type of the analysis",
                value=["static", "test"],
            )
        )

        prop.append(
            _PropHelper(
                type="App::PropertyPrecision",
                name="SolverPrecision",
                group="Solver",
                doc="Precision of solver (a smaller number will quit with an error if this accuracy can't be met)",
                value=1e-6,
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            # Migrate group of properties for old projects
            if obj.getGroupOfProperty(prop.name) != prop.group:
                obj.setGroupOfProperty(prop.name, prop.group)
