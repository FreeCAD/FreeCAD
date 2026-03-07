# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM constraint initial flow velocity document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package constraint_initialflowvelocity
#  \ingroup FEM
#  \brief constraint initial flow velocity object

from . import base_fempythonobject


class ConstraintInitialFlowVelocity(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintInitialFlowVelocity"

    def __init__(self, obj):
        super().__init__(obj)
        obj.addProperty(
            "App::PropertyVelocity", "VelocityX", "Parameter", "Velocity in X-direction"
        )
        obj.setPropertyStatus("VelocityX", "LockDynamic")
        obj.addProperty(
            "App::PropertyString",
            "VelocityXFormula",
            "Parameter",
            "Velocity formula in X-direction",
        )
        obj.setPropertyStatus("VelocityXFormula", "LockDynamic")
        obj.addProperty(
            "App::PropertyBool", "VelocityXUnspecified", "Parameter", "Use velocity in X-direction"
        )
        obj.setPropertyStatus("VelocityXUnspecified", "LockDynamic")
        obj.VelocityXUnspecified = True
        obj.addProperty(
            "App::PropertyBool",
            "VelocityXHasFormula",
            "Parameter",
            "Use formula for velocity in X-direction",
        )
        obj.setPropertyStatus("VelocityXHasFormula", "LockDynamic")

        obj.addProperty(
            "App::PropertyVelocity", "VelocityY", "Parameter", "Velocity in Y-direction"
        )
        obj.setPropertyStatus("VelocityY", "LockDynamic")
        obj.addProperty(
            "App::PropertyString",
            "VelocityYFormula",
            "Parameter",
            "Velocity formula in Y-direction",
        )
        obj.setPropertyStatus("VelocityYFormula", "LockDynamic")
        obj.addProperty(
            "App::PropertyBool", "VelocityYUnspecified", "Parameter", "Use velocity in Y-direction"
        )
        obj.setPropertyStatus("VelocityYUnspecified", "LockDynamic")
        obj.VelocityYUnspecified = True
        obj.addProperty(
            "App::PropertyBool",
            "VelocityYHasFormula",
            "Parameter",
            "Use formula for velocity in Y-direction",
        )
        obj.setPropertyStatus("VelocityYHasFormula", "LockDynamic")

        obj.addProperty(
            "App::PropertyVelocity", "VelocityZ", "Parameter", "Velocity in Z-direction"
        )
        obj.setPropertyStatus("VelocityZ", "LockDynamic")
        obj.addProperty(
            "App::PropertyString",
            "VelocityZFormula",
            "Parameter",
            "Velocity formula in Z-direction",
        )
        obj.setPropertyStatus("VelocityZFormula", "LockDynamic")
        obj.addProperty(
            "App::PropertyBool", "VelocityZUnspecified", "Parameter", "Use velocity in Z-direction"
        )
        obj.setPropertyStatus("VelocityZUnspecified", "LockDynamic")
        obj.VelocityZUnspecified = True
        obj.addProperty(
            "App::PropertyBool",
            "VelocityZHasFormula",
            "Parameter",
            "Use formula for velocity in Z-direction",
        )
        obj.setPropertyStatus("VelocityZHasFormula", "LockDynamic")
