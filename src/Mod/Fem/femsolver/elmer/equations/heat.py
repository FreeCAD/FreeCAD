# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver Elmer equation object Heat"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from . import nonlinear
from ... import equationbase

CONVECTION_TYPE = ["None", "Computed", "Constant"]
PHASE_CHANGE_MODEL = ["None", "Spatial 1", "Spatial 2", "Temporal"]


def create(doc, name="Heat"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(nonlinear.Proxy, equationbase.HeatProxy):

    Type = "Fem::EquationElmerHeat"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        # according to the Elmer models manual Bubbles is by default True
        # and Stabilize is False (Stabilize is added in linear.py)
        obj.addProperty(
            "App::PropertyBool",
            "Bubbles",
            "Heat",
            ""
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Convection",
            "Equation",
            "Type of convection to be used"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "PhaseChangeModel",
            "Equation",
            "Model for phase change"
        )

        obj.Bubbles = True
        obj.Stabilize = False
        obj.Convection = CONVECTION_TYPE
        obj.Convection = "None"
        obj.PhaseChangeModel = PHASE_CHANGE_MODEL
        obj.PhaseChangeModel = "None"
        obj.Priority = 20


class ViewProxy(nonlinear.ViewProxy, equationbase.HeatViewProxy):
    pass

##  @}
