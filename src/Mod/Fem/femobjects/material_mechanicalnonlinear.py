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

__title__ = "FreeCAD FEM material mechanical nonlinear document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package material_mechanicalnonlinear
#  \ingroup FEM
#  \brief nonlinear mechanical material object

from . import base_fempythonobject


class MaterialMechanicalNonlinear(base_fempythonobject.BaseFemPythonObject):
    """
    The MaterialMechanicalNonlinear object
    """

    Type = "Fem::MaterialMechanicalNonlinear"

    def __init__(self, obj):
        super(MaterialMechanicalNonlinear, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):

        # YieldPoints was (until 0.19) stored as 3 separate variables. Consolidate them if present.
        yield_points = []
        if hasattr(obj, "YieldPoint1"):
            if obj.YieldPoint1:
                yield_points.append(obj.YieldPoint1)
            obj.removeProperty("YieldPoint1")
            if hasattr(obj, "YieldPoint2"):
                if obj.YieldPoint2:
                    yield_points.append(obj.YieldPoint2)
                obj.removeProperty("YieldPoint2")
            if hasattr(obj, "YieldPoint3"):
                if obj.YieldPoint3:
                    yield_points.append(obj.YieldPoint3)
                obj.removeProperty("YieldPoint3")

        self.add_properties(obj)
        if yield_points:
            obj.YieldPoints = yield_points

        # TODO: If in the future more nonlinear options are added, update choices here.

    def add_properties(self, obj):

        # this method is called from onDocumentRestored
        # thus only add and or set a attribute
        # if the attribute does not exist

        if not hasattr(obj, "LinearBaseMaterial"):
            obj.addProperty(
                "App::PropertyLink",
                "LinearBaseMaterial",
                "Base",
                "Set the linear material the nonlinear builds upon."
            )

        if not hasattr(obj, "MaterialModelNonlinearity"):
            choices_nonlinear_material_models = ["simple hardening"]
            obj.addProperty(
                "App::PropertyEnumeration",
                "MaterialModelNonlinearity",
                "Fem",
                "Set the type on nonlinear material model"
            )
            obj.MaterialModelNonlinearity = choices_nonlinear_material_models
            obj.MaterialModelNonlinearity = choices_nonlinear_material_models[0]

        if not hasattr(obj, "YieldPoints"):
            obj.addProperty(
                "App::PropertyStringList",
                "YieldPoints",
                "Fem",
                "Set stress and strain for yield points as a list of strings, "
                "each point \"stress, plastic strain\""
            )
            obj.YieldPoints = []
