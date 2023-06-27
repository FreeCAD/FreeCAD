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

__title__ = "FreeCAD FEM constraint magnetization document object"
__author__ = "Uwe Stöhr"
__url__ = "https://www.freecad.org"

## @package constraint_magnetization
#  \ingroup FEM
#  \brief constraint magnetization source object

from . import base_fempythonobject


class ConstraintMagnetization(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintMagnetization"

    def __init__(self, obj):
        super(ConstraintMagnetization, self).__init__(obj)
        self.add_properties(obj)

    def onDocumentRestored(self, obj):
        self.add_properties(obj)

    def add_properties(self, obj):
        if not hasattr(obj, "Magnetization_re_1"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_re_1",
                "Vector Potential",
                "Real part of magnetization x-component"
            )
            obj.Magnetization_re_1 = "0 A/m"
        if not hasattr(obj, "Magnetization_re_2"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_re_2",
                "Vector Potential",
                "Real part of magnetization y-component"
            )
            obj.Magnetization_re_2 = "0 A/m"
        if not hasattr(obj, "Magnetization_re_3"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_re_3",
                "Vector Potential",
                "Real part of magnetization z-component"
            )
            obj.Magnetization_re_3 = "0 A/m"
        if not hasattr(obj, "Magnetization_im_1"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_im_1",
                "Vector Potential",
                "Imaginary part of magnetization x-component"
            )
            obj.Magnetization_im_1 = "0 A/m"
        if not hasattr(obj, "Magnetization_im_2"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_im_2",
                "Vector Potential",
                "Imaginary part of magnetization y-component"
            )
            obj.Magnetization_im_2 = "0 A/m"
        if not hasattr(obj, "Magnetization_im_3"):
            obj.addProperty(
                "App::PropertyMagnetization",
                "Magnetization_im_3",
                "Vector Potential",
                "Imaginary part of magnetization z-component"
            )
            obj.Magnetization_im_3 = "0 A/m"

        # now the enable bools
        if not hasattr(obj, "Magnetization_re_1_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_re_1_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_re_1_Disabled = True
        if not hasattr(obj, "Magnetization_re_2_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_re_2_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_re_2_Disabled = True
        if not hasattr(obj, "Magnetization_re_3_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_re_3_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_re_3_Disabled = True
        if not hasattr(obj, "Magnetization_im_1_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_im_1_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_im_1_Disabled = True
        if not hasattr(obj, "Magnetization_im_2_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_im_2_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_im_2_Disabled = True
        if not hasattr(obj, "Magnetization_im_3_Disabled"):
            obj.addProperty(
                "App::PropertyBool",
                "Magnetization_im_3_Disabled",
                "Vector Potential",
                ""
            )
            obj.Magnetization_im_3_Disabled = True
