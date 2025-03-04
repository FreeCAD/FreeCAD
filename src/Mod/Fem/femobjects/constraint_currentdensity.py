# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM constraint current density document object"
__author__ = "Uwe Stöhr, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package constraint_currentdensity
#  \ingroup FEM
#  \brief constraint current density object

from FreeCAD import Base

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class ConstraintCurrentDensity(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintCurrentDensity"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_re_1",
                group="Current Density",
                doc="Real part of current density x-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_re_2",
                group="Current Density",
                doc="Real part of current density y-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_re_3",
                group="Current Density",
                doc="Real part of current density z-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_im_1",
                group="Current Density",
                doc="Imaginary part of current density x-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_im_2",
                group="Current Density",
                doc="Imaginary part of current density y-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="CurrentDensity_im_3",
                group="Current Density",
                doc="Imaginary part of current density z-component",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableCurrentDensity_1",
                group="Current Density",
                doc="Enable currenty density x component",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableCurrentDensity_2",
                group="Current Density",
                doc="Enable currenty density y component",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableCurrentDensity_3",
                group="Current Density",
                doc="Enable currenty density z component",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="NormalCurrentDensity_re",
                group="Current Density",
                doc="Real part of current density normal to boundary",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyCurrentDensity",
                name="NormalCurrentDensity_im",
                group="Current Density",
                doc="Imaginary part of current density normal to boundary",
                value="0 A/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Mode",
                group="Current Density",
                doc="Set current boundary condition direction mode",
                value=["Custom", "Normal"],
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

        # enable current density properties from old properties
        try:
            obj.EnableCurrentDensity_1 = not obj.getPropertyByName(
                "CurrentDensity_re_1_Disabled"
            ) or not obj.getPropertyByName("CurrentDensity_im_1_Disabled")

            obj.EnableCurrentDensity_2 = not obj.getPropertyByName(
                "CurrentDensity_re_2_Disabled"
            ) or not obj.getPropertyByName("CurrentDensity_im_2_Disabled")

            obj.EnableCurrentDensity_3 = not obj.getPropertyByName(
                "CurrentDensity_re_3_Disabled"
            ) or not obj.getPropertyByName("CurrentDensity_im_3_Disabled")

            # remove old properties
            obj.setPropertyStatus("CurrentDensity_re_1_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_re_1_Disabled")
            obj.setPropertyStatus("CurrentDensity_re_2_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_re_2_Disabled")
            obj.setPropertyStatus("CurrentDensity_re_3_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_re_3_Disabled")
            obj.setPropertyStatus("CurrentDensity_im_1_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_im_1_Disabled")
            obj.setPropertyStatus("CurrentDensity_im_2_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_im_2_Disabled")
            obj.setPropertyStatus("CurrentDensity_im_3_Disabled", "-LockDynamic")
            obj.removeProperty("CurrentDensity_im_3_Disabled")

        except Base.PropertyError:
            pass
