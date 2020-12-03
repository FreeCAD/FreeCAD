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

__title__  = "FreeCAD FEM constraint electrostatic potential document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__    = "https://www.freecadweb.org"

## @package constraint_electrostaticpotential
#  \ingroup FEM
#  \brief constraint electrostatic potential object

from . import base_fempythonobject


class ConstraintElectrostaticPotential(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintElectrostaticPotential"

    def __init__(self, obj):
        super(ConstraintElectrostaticPotential, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):
        self.add_properties(obj)

    def add_properties(self, obj):
        if not hasattr(obj, "Potential"):
            obj.addProperty(
                "App::PropertyFloat",
                "Potential",
                "Parameter",
                "Potential"
            ),
            obj.Potential = 0.0

        if not hasattr(obj, "PotentialEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "PotentialEnabled",
                "Parameter",
                "Potential Enabled"
            ),
            obj.PotentialEnabled = False

        if not hasattr(obj, "PotentialConstant"):
            obj.addProperty(
                "App::PropertyBool",
                "PotentialConstant",
                "Parameter",
                "Potential Constant"
            ),
            obj.PotentialConstant = False

        if not hasattr(obj, "ElectricInfinity"):
            obj.addProperty(
                "App::PropertyBool",
                "ElectricInfinity",
                "Parameter",
                "Electric Infinity"
            ),
            obj.ElectricInfinity = False

        if not hasattr(obj, "ElectricForcecalculation"):
            obj.addProperty(
                "App::PropertyBool",
                "ElectricForcecalculation",
                "Parameter",
                "Electric Force Calculation"
            ),
            obj.ElectricForcecalculation = False

        if not hasattr(obj, "CapacitanceBody"):
            obj.addProperty(
                "App::PropertyInteger",
                "CapacitanceBody",
                "Parameter",
                "Capacitance Body"
            ),
            obj.CapacitanceBody = 0

        if not hasattr(obj, "CapacitanceBodyEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "CapacitanceBodyEnabled",
                "Parameter",
                "Capacitance Body Enabled"
            )
            obj.CapacitanceBodyEnabled = False
