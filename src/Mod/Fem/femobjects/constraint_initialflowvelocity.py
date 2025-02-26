# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
        obj.addLockedProperty(
            "App::PropertyVelocity", "VelocityX", "Parameter", "Velocity in x-direction"
        )

        obj.addLockedProperty(
            "App::PropertyString",
            "VelocityXFormula",
            "Parameter",
            "Velocity formula in x-direction",
        )

        obj.addLockedProperty(
            "App::PropertyBool", "VelocityXUnspecified", "Parameter", "Use velocity in x-direction"
        )

        obj.VelocityXUnspecified = True
        obj.addLockedProperty(
            "App::PropertyBool",
            "VelocityXHasFormula",
            "Parameter",
            "Use formula for velocity in x-direction",
        )

        obj.addLockedProperty(
            "App::PropertyVelocity", "VelocityY", "Parameter", "Velocity in y-direction"
        )

        obj.addLockedProperty(
            "App::PropertyString",
            "VelocityYFormula",
            "Parameter",
            "Velocity formula in y-direction",
        )

        obj.addLockedProperty(
            "App::PropertyBool", "VelocityYUnspecified", "Parameter", "Use velocity in y-direction"
        )

        obj.VelocityYUnspecified = True
        obj.addLockedProperty(
            "App::PropertyBool",
            "VelocityYHasFormula",
            "Parameter",
            "Use formula for velocity in y-direction",
        )

        obj.addLockedProperty(
            "App::PropertyVelocity", "VelocityZ", "Parameter", "Velocity in z-direction"
        )

        obj.addLockedProperty(
            "App::PropertyString",
            "VelocityZFormula",
            "Parameter",
            "Velocity formula in z-direction",
        )

        obj.addLockedProperty(
            "App::PropertyBool", "VelocityZUnspecified", "Parameter", "Use velocity in z-direction"
        )

        obj.VelocityZUnspecified = True
        obj.addLockedProperty(
            "App::PropertyBool",
            "VelocityZHasFormula",
            "Parameter",
            "Use formula for velocity in z-direction",
        )
