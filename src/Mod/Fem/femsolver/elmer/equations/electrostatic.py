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

__title__ = "FreeCAD FEM solver Elmer equation object Electrostatic"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from femtools import femutils
from ... import equationbase
from . import linear


def create(doc, name="Electrostatic"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(linear.Proxy, equationbase.ElectrostaticProxy):

    Type = "Fem::EquationElmerElectrostatic"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyBool",
            "CalculateCapacitanceMatrix",
            "Electrostatic",
            ""
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateElectricEnergy",
            "Electrostatic",
            ""
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateElectricField",
            "Electrostatic",
            ""
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateElectricFlux",
            "Electrostatic",
            ""
        )
        obj.addProperty(
            "App::PropertyBool",
            "CalculateSurfaceCharge",
            "Electrostatic",
            ""
        )
        """
        obj.addProperty(
            "App::PropertyInteger",
            "CapacitanceBodies",
            "Electrostatic",
            ""
        )
        """
        obj.addProperty(
            "App::PropertyFile",
            "CapacitanceMatrixFilename",
            "Electrostatic",
            (
                "File where capacitance matrix is being saved\n"
                "Only used if 'CalculateCapacitanceMatrix' is true"
            )
        )
        obj.addProperty(
            "App::PropertyBool",
            "ConstantWeights",
            "Electrostatic",
            "Use constant weighting for results"
        )
        obj.addProperty(
            "App::PropertyFloat",
            "PotentialDifference",
            "Electrostatic",
            (
                "Potential difference in Volt for which capacitance is\n"
                "calculated if 'CalculateCapacitanceMatrix' is false"
            )
        )

        obj.CapacitanceMatrixFilename = "cmatrix.dat"
        obj.PotentialDifference = 0.0
        obj.Priority = 10


class ViewProxy(linear.ViewProxy, equationbase.ElectrostaticViewProxy):
    pass

##  @}
