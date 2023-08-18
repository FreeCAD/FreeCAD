# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM constraint electrostatic potential document object"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

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
                "App::PropertyElectricPotential",
                "Potential",
                "Parameter",
                "Electric Potential"
            )
            # setting  1 V assures that the unit does not switch to mV
            # and the constraint holds usually Volts
            obj.Potential = "1 V"

        if not hasattr(obj, "AV_re_1"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_re_1",
                "Vector Potential",
                "Real part of potential x-component"
            )
            obj.AV_re_1 = "0 V"
        if not hasattr(obj, "AV_re_2"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_re_2",
                "Vector Potential",
                "Real part of potential y-component"
            )
            obj.AV_re_2 = "0 V"
        if not hasattr(obj, "AV_re_3"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_re_3",
                "Vector Potential",
                "Real part of potential z-component"
            )
            obj.AV_re_3 = "0 V"
        if not hasattr(obj, "AV_im"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_im",
                "Vector Potential",
                "Imaginary part of scalar potential"
            )
            obj.AV_im = "0 V"
        if not hasattr(obj, "AV_im_1"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_im_1",
                "Vector Potential",
                "Imaginary part of potential x-component"
            )
            obj.AV_im_1 = "0 V"
        if not hasattr(obj, "AV_im_2"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_im_2",
                "Vector Potential",
                "Imaginary part of potential y-component"
            )
            obj.AV_im_2 = "0 V"
        if not hasattr(obj, "AV_im_3"):
            obj.addProperty(
                "App::PropertyElectricPotential",
                "AV_im_3",
                "Vector Potential",
                "Imaginary part of potential z-component"
            )
            obj.AV_im_3 = "0 V"

        # now the enable bools
        if not hasattr(obj, "PotentialEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "PotentialEnabled",
                "Parameter",
                "Potential Enabled"
            )
            obj.PotentialEnabled = True
        if not hasattr(obj, "AV_re_1_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_re_1_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_re_1_Disabled = True
        if not hasattr(obj, "AV_re_2_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_re_2_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_re_2_Disabled = True
        if not hasattr(obj, "AV_re_3_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_re_3_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_re_3_Disabled = True
        if not hasattr(obj, "AV_im_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_im_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_im_Disabled = True
        if not hasattr(obj, "AV_im_1_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_im_1_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_im_1_Disabled = True
        if not hasattr(obj, "AV_im_2_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_im_2_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_im_2_Disabled = True
        if not hasattr(obj, "AV_im_3_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "AV_im_3_Disabled",
                "Vector Potential",
                ""
            )
            obj.AV_im_3_Disabled = True

        if not hasattr(obj, "PotentialConstant"):
            obj.addProperty(
                "App::PropertyBool",
                "PotentialConstant",
                "Parameter",
                "Potential Constant"
            )
            obj.PotentialConstant = False

        if not hasattr(obj, "ElectricInfinity"):
            obj.addProperty(
                "App::PropertyBool",
                "ElectricInfinity",
                "Parameter",
                "Electric Infinity"
            )
            obj.ElectricInfinity = False

        if not hasattr(obj, "ElectricForcecalculation"):
            obj.addProperty(
                "App::PropertyBool",
                "ElectricForcecalculation",
                "Parameter",
                "Electric Force Calculation"
            )
            obj.ElectricForcecalculation = False

        if not hasattr(obj, "CapacitanceBody"):
            obj.addProperty(
                "App::PropertyInteger",
                "CapacitanceBody",
                "Parameter",
                "Capacitance Body"
            )
            obj.CapacitanceBody = 0

        if not hasattr(obj, "CapacitanceBodyEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "CapacitanceBodyEnabled",
                "Parameter",
                "Capacitance Body Enabled"
            )
            obj.CapacitanceBodyEnabled = False
