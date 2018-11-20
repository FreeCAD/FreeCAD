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

__title__ = "FreeCAD FEM material mechanical nonlinear document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemMaterialMechanicalNonLinear
#  \ingroup FEM
#  \brief FEM nonlinear mechanical material object


class _FemMaterialMechanicalNonlinear:
    "The FemMaterialMechanicalNonlinear object"
    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "Fem::MaterialMechanicalNonlinear"

        obj.addProperty("App::PropertyLink", "LinearBaseMaterial", "Base", "Set the linear material the nonlinear builds upon.")

        choices_nonlinear_material_models = ["simple hardening"]
        obj.addProperty("App::PropertyEnumeration", "MaterialModelNonlinearity", "Fem", "Set the type on nonlinear material model")
        obj.MaterialModelNonlinearity = choices_nonlinear_material_models
        obj.MaterialModelNonlinearity = choices_nonlinear_material_models[0]

        obj.addProperty("App::PropertyString", "YieldPoint1", "Fem", "Set stress and strain for yield point one, separated by a comma.")
        obj.YieldPoint1 = "235.0, 0.0"

        obj.addProperty("App::PropertyString", "YieldPoint2", "Fem", "Set stress and strain for yield point two, separated by a comma.")
        obj.YieldPoint2 = "241.0, 0.025"

        obj.addProperty("App::PropertyString", "YieldPoint3", "Fem", "Set stress and strain for yield point three, separated by a comma.")
        obj.YieldPoint3 = ""

    def execute(self, obj):
        return
