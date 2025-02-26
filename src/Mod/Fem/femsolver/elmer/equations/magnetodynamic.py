# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM solver Elmer equation object Magnetodynamic"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"
#
## \addtogroup FEM
#  @{

from femtools import femutils
from . import nonlinear
from ... import equationbase


def create(doc, name="Magnetodynamic"):
    return femutils.createObject(doc, name, Proxy, ViewProxy)


class Proxy(nonlinear.Proxy, equationbase.MagnetodynamicProxy):

    Type = "Fem::EquationElmerMagnetodynamic"

    def __init__(self, obj):
        super().__init__(obj)

        obj.addLockedProperty(
            "App::PropertyBool",
            "IsHarmonic",
            "Magnetodynamic",
            "If the magnetic source is harmonically driven",
        )
        obj.addLockedProperty(
            "App::PropertyFrequency",
            "AngularFrequency",
            "Magnetodynamic",
            "Frequency of the driving current",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "UsePiolaTransform",
            "Magnetodynamic",
            "Must be True if basis functions for edge element interpolation\n"
            "are selected to be members of optimal edge element family\n"
            "or if second-order approximation is used.",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "QuadraticApproximation",
            "Magnetodynamic",
            "Enables second-order approximation of driving current",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "StaticConductivity",
            "Magnetodynamic",
            "See Elmer models manual for info",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "FixInputCurrentDensity",
            "Magnetodynamic",
            "Ensures divergence-freeness of current density",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "AutomatedSourceProjectionBCs",
            "Magnetodynamic",
            "See Elmer models manual for info",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "UseLagrangeGauge",
            "Magnetodynamic",
            "See Elmer models manual for info",
        )
        obj.addLockedProperty(
            "App::PropertyFloat",
            "LagrangeGaugePenalizationCoefficient",
            "Magnetodynamic",
            "See Elmer models manual for info",
        )
        obj.addLockedProperty(
            "App::PropertyBool",
            "UseTreeGauge",
            "Magnetodynamic",
            "See Elmer models manual for info\nWill be ignored if 'UsePiolaTransform' is True",
        )
        obj.addLockedProperty("App::PropertyBool", "LinearSystemRefactorize", "Linear System", "")

        obj.IsHarmonic = False
        obj.AngularFrequency = 0
        obj.Priority = 10

        # the post processor options
        obj.addLockedProperty("App::PropertyBool", "CalculateCurrentDensity", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateElectricField", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateElementalFields", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateHarmonicLoss", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateJouleHeating", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateMagneticFieldStrength", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateMaxwellStress", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateNodalFields", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateNodalForces", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "CalculateNodalHeating", "Results", "")
        obj.addLockedProperty("App::PropertyBool", "DiscontinuousBodies", "Results", "")
        obj.CalculateCurrentDensity = False
        obj.CalculateElectricField = False
        # FIXME: at the moment FreeCAD's post processor cannot display elementary field
        # results, therefore disable despite this is by default on in Elmer
        obj.CalculateElementalFields = False
        obj.CalculateHarmonicLoss = False
        obj.CalculateJouleHeating = False
        obj.CalculateMagneticFieldStrength = False
        obj.CalculateMaxwellStress = False
        obj.CalculateNodalFields = True
        obj.CalculateNodalForces = False
        obj.CalculateNodalHeating = False
        obj.DiscontinuousBodies = False


class ViewProxy(nonlinear.ViewProxy, equationbase.MagnetodynamicViewProxy):
    pass


##  @}
