# ***************************************************************************
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM constraint initial pressure document object"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package constraint_initialpressure
#  \ingroup FEM
#  \brief constraint initial pressure object

from . import base_fempythonobject


class ConstraintInitialPressure(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintInitialPressure"

    def __init__(self, obj):
        super(ConstraintInitialPressure, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):
        self.add_properties(obj)

    def add_properties(self, obj):
        if not hasattr(obj, "Pressure"):
            obj.addProperty(
                "App::PropertyPressure",
                "Pressure",
                "Parameter",
                "Initial Pressure"
            )
            # we initialize 1 bar
            obj.Pressure = "100 kPa"
