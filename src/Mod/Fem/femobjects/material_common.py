# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

__title__ = "FreeCAD FEM material document object"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package material_common
#  \ingroup FEM
#  \brief material common object

from . import base_fempythonobject


class MaterialCommon(base_fempythonobject.BaseFemPythonObject):
    """
    The MaterialCommon object
    """

    Type = "Fem::MaterialCommon"

    def __init__(self, obj):
        super(MaterialCommon, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):
        self.add_properties(obj)

    def add_properties(self, obj):
        # References
        if not hasattr(obj, "References"):
            obj.addProperty(
                "App::PropertyLinkSubList",
                "References",
                "Material",
                "List of material shapes"
            )
        # Category
        # attribute Category was added in commit 61fb3d429a
        if not hasattr(obj, "Category"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Category",
                "Material",
                "Material type: fluid or solid"
            )
            obj.Category = ["Solid", "Fluid"]  # used in TaskPanel
            obj.Category = "Solid"
        """
        Some remarks to the category. Not finished, thus to be continued.

        Following question need to be answered:
        Why use a attribute to split the object? If a new fem object is needed,
        a new fem object should be created. A new object will have an own Type
        and will be collected for the writer by this type.

        The category should not be used in writer! This would be the border.
        If an fem object has to be distinguished in writer it should be an own
        fem object.

        The category is just some helper to make it easier for the user to
        distinguish between different material categories.
        It can have own command, own icon, own make method. In material TaskPanel
        it can be distinguished which materials will be shown.

        ATM in calculix writer the Category is used. See comments in CalculiX Solver.
        """
