# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "Elasticity"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import femtools.femutils as FemUtils
from ... import equationbase
from . import linear


def create(doc, name="Elasticity"):
    return FemUtils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(linear.Proxy, equationbase.ElasticityProxy):

    Type = "Fem::FemEquationElmerElasticity"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.addProperty(
            "App::PropertyBool", "DoFrequencyAnalysis",
            "Elasticity", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyInteger", "EigenmodesCount",
            "Elasticity", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyBool", "CalculateStrains",
            "Elasticity", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyBool", "CalculateStresses",
            "Elasticity", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyBool", "CalculatePrincipal",
            "Elasticity", "Select type of solver for linear system")
        obj.addProperty(
            "App::PropertyBool", "CalculatePangle",
            "Elasticity", "Select type of solver for linear system")
        obj.EigenmodesCount = 5
        obj.Priority = 10


class ViewProxy(linear.ViewProxy, equationbase.ElasticityViewProxy):
    pass
