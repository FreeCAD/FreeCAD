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

        obj.addProperty(
            "App::PropertyBool",
            "IsHarmonic",
            "Magnetodynamic",
            "If the magnetic source is harmonically driven",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyFrequency",
            "AngularFrequency",
            "Magnetodynamic",
            "Frequency of the driving current",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "UsePiolaTransform",
            "Magnetodynamic",
            "Must be True if basis functions for edge element interpolation\n"
            "are selected to be members of optimal edge element family\n"
            "or if second-order approximation is used.",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "QuadraticApproximation",
            "Magnetodynamic",
            "Enables second-order approximation of driving current",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "StaticConductivity",
            "Magnetodynamic",
            "See Elmer models manual for info",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "FixInputCurrentDensity",
            "Magnetodynamic",
            "Ensures divergence-freeness of current density",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "AutomatedSourceProjectionBCs",
            "Magnetodynamic",
            "See Elmer models manual for info",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseLagrangeGauge",
            "Magnetodynamic",
            "See Elmer models manual for info",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyFloat",
            "LagrangeGaugePenalizationCoefficient",
            "Magnetodynamic",
            "See Elmer models manual for info",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseTreeGauge",
            "Magnetodynamic",
            "See Elmer models manual for info\nWill be ignored if 'UsePiolaTransform' is True",
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool", "LinearSystemRefactorize", "Linear System", "", locked=True
        )

        obj.IsHarmonic = False
        obj.AngularFrequency = 0
        obj.Priority = 10

        # the post processor options
        obj.addProperty("App::PropertyBool", "CalculateCurrentDensity", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateElectricField", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateElementalFields", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateHarmonicLoss", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateJouleHeating", "Results", "", locked=True)
        obj.addProperty(
            "App::PropertyBool", "CalculateMagneticFieldStrength", "Results", "", locked=True
        )
        obj.addProperty("App::PropertyBool", "CalculateMaxwellStress", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateNodalFields", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateNodalForces", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "CalculateNodalHeating", "Results", "", locked=True)
        obj.addProperty("App::PropertyBool", "DiscontinuousBodies", "Results", "", locked=True)
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
