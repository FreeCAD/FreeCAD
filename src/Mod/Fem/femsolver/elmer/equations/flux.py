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

__title__ = "FreeCAD FEM solver Elmer equation object Flux"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from ... import equationbase
from . import linear


def create(doc, name="Flux"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(linear.Proxy, equationbase.FluxProxy):

    Type = "Fem::EquationElmerFlux"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFlux",
            "Flux",
            "Select type of solver for linear system"
        )
        obj.addProperty(
            "App::PropertyString",
            "FluxVariable",
            "Flux",
            "Insert variable name for flux calculation"
        )
        """
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFluxAbs",
            "Flux",
            "Select calculation of abs of flux"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateFluxMagnitude",
            "Flux",
            "Select calculation of magnitude of flux"
        )
        """
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGrad",
            "Flux",
            "Select calculation of gradient"
        )
        """
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGradAbs",
            "Flux",
            "Select calculation of abs of gradient"
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateGradMagnitude",
            "Flux",
            "Select calculation of magnitude of gradient"
        )
        obj.addProperty(
            "App::PropertyBool",
            "EnforcePositiveMagnitude",
            "Flux",
            "Select calculation of positive magnitude"
        )
        """
        obj.Priority = 5


class ViewProxy(linear.ViewProxy, equationbase.FluxViewProxy):
    pass

##  @}
