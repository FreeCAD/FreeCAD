# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package constraint_electrostaticpotential
#  \ingroup FEM
#  \brief constraint electrostatic potential object

from FreeCAD import Base

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class ConstraintElectrostaticPotential(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintElectrostaticPotential"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyElectricPotential",
                name="Potential",
                group="Parameter",
                doc="Electric Potential",
                value="1 V",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="PotentialEnabled",
                group="Parameter",
                doc="Enable electric potential",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectricPotential",
                name="AV_re",
                group="Electromagnetic Potential",
                doc="Real part of scalar potential",
                value="0 V",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectricPotential",
                name="AV_im",
                group="Electromagnetic Potential",
                doc="Imaginary part of scalar potential",
                value="0 V",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_re_1",
                group="Electromagnetic Potential",
                doc="Real part of vector potential x-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_re_2",
                group="Electromagnetic Potential",
                doc="Real part of vector potential y-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_re_3",
                group="Electromagnetic Potential",
                doc="Real part of vector potential z-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_im_1",
                group="Electromagnetic Potential",
                doc="Imaginary part of vector potential x-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_im_2",
                group="Electromagnetic Potential",
                doc="Imaginary part of vector potential y-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectromagneticPotential",
                name="AV_im_3",
                group="Electromagnetic Potential",
                doc="Imaginary part of vector potential z-component",
                value="0 Wb/m",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertySurfaceChargeDensity",
                name="ElectricFluxDensity",
                group="Parameter",
                doc="Electric displacement field D.\n"
                + "For interfaces, it represents the difference\n"
                + "between the normal component in the two media",
                value="0 C/m^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="BoundaryCondition",
                group="Parameter",
                doc="Set boundary condition type",
                value=["Dirichlet", "Neumann"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableAV",
                group="Electromagnetic Potential",
                doc="Enable scalar potential boundary condition",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableAV_1",
                group="Electromagnetic Potential",
                doc="Enable vector potential x-component boundary condition",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableAV_2",
                group="Electromagnetic Potential",
                doc="Enable vector potential y-component boundary condition",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="EnableAV_3",
                group="Electromagnetic Potential",
                doc="Enable vector potential z-component boundary condition",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="PotentialConstant",
                group="Parameter",
                doc="",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ElectricInfinity",
                group="Parameter",
                doc="Electric Infinity",
                value=False,
            )
        )

        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ElectricForcecalculation",
                group="Parameter",
                doc="Electric force calculation",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="CapacitanceBody",
                group="Parameter",
                doc="Capacitance body",
                value=0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CapacitanceBodyEnabled",
                group="Parameter",
                doc="Capacitance body enabled",
                value=False,
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

            # convert old potential float to Volt
            if prop.name == "Potential":
                prop.handle_change_type(
                    obj,
                    old_type="App::PropertyFloat",
                    convert_old_value=lambda x: "{} V".format(1e6 * x),
                )
            # fix unit for magnetic vector potential properties
            if prop.name in ("AV_re_1", "AV_re_2", "AV_re_3", "AV_im_1", "AV_im_2", "AV_im_3"):
                prop.handle_change_type(
                    obj,
                    old_type="App::PropertyElectricPotential",
                    convert_old_value=lambda x: "{} Wb/m".format(x.getValueAs("V").Value),
                )

        # enable electromagnetic properties from old properties
        try:
            obj.EnableAV_1 = not obj.getPropertyByName(
                "AV_re_1_Disabled"
            ) or not obj.getPropertyByName("AV_im_1_Disabled")
            obj.EnableAV_2 = not obj.getPropertyByName(
                "AV_re_2_Disabled"
            ) or not obj.getPropertyByName("AV_im_2_Disabled")
            obj.EnableAV_3 = not obj.getPropertyByName(
                "AV_re_3_Disabled"
            ) or not obj.getPropertyByName("AV_im_3_Disabled")
            obj.EnableAV = not obj.getPropertyByName("AV_im_Disabled")

            # remove old properties
            obj.setPropertyStatus("AV_re_1_Disabled", "-LockDynamic")
            obj.removeProperty("AV_re_1_Disabled")
            obj.setPropertyStatus("AV_re_2_Disabled", "-LockDynamic")
            obj.removeProperty("AV_re_2_Disabled")
            obj.setPropertyStatus("AV_re_3_Disabled", "-LockDynamic")
            obj.removeProperty("AV_re_3_Disabled")
            obj.setPropertyStatus("AV_im_1_Disabled", "-LockDynamic")
            obj.removeProperty("AV_im_1_Disabled")
            obj.setPropertyStatus("AV_im_2_Disabled", "-LockDynamic")
            obj.removeProperty("AV_im_2_Disabled")
            obj.setPropertyStatus("AV_im_3_Disabled", "-LockDynamic")
            obj.removeProperty("AV_im_3_Disabled")
            obj.setPropertyStatus("AV_im_Disabled", "-LockDynamic")
            obj.removeProperty("AV_im_Disabled")

        except Base.PropertyError:
            pass

        # set electric flux density from old surface charge density
        try:
            obj.ElectricFluxDensity = obj.getPropertyByName("SurfaceChargeDensity")
            obj.setPropertyStatus("SurfaceChargeDensity", "-LockDynamic")
            obj.removeProperty("SurfaceChargeDensity")

        except Base.PropertyError:
            pass
