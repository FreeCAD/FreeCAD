# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM material document object"
__author__ = "Juergen Riegel, Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package material_common
#  \ingroup FEM
#  \brief material common object

from FreeCAD import Base, Units
import Materials

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MaterialCommon(base_fempythonobject.BaseFemPythonObject):
    """
    The MaterialCommon object
    """

    Type = "Fem::MaterialCommon"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

        obj.addExtension("App::SuppressibleExtensionPython")

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyLinkSubListGlobal",
                name="References",
                group="Material",
                doc="List of material shapes",
                value=[],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Category",
                group="Material",
                doc="Material type: fluid or solid",
                value=["Solid", "Fluid"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="UUID",
                group="Material",
                doc="Material UUID",
                hidden=True,
                value="",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLink",
                name="Nonlinear",
                group="Material",
                doc="Material nonlinear behavior",
                value=None,
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

            if prop.name == "References":
                # change References to App::PropertyLinkSubListGlobal
                prop.handle_change_type(obj, old_type="App::PropertyLinkSubList")

        # try update UUID from Material
        if not obj.UUID:
            obj.UUID = self._get_material_uuid(obj.Material)

        if not obj.hasExtension("App::SuppressibleExtensionPython"):
            obj.addExtension("App::SuppressibleExtensionPython")

    def _get_material_uuid(self, material):
        if not material:
            return ""

        material_manager = Materials.MaterialManager()

        for a_mat in material_manager.Materials:
            unmatched_item = True
            a_mat_prop = material_manager.getMaterial(a_mat).Properties
            for it in material:
                if it in a_mat_prop:
                    # first try to compare quantities
                    try:
                        unmatched_item = Units.Quantity(material[it]) != Units.Quantity(
                            a_mat_prop[it]
                        )
                    except ValueError:
                        # if there is no quantity, compare values directly
                        unmatched_item = material[it] != a_mat_prop[it]

                if unmatched_item:
                    break

            if not unmatched_item:
                # all material items are found in a_mat
                return a_mat

        return ""

        """
        Some remarks to the category. Not finished, thus to be continued.

        Following question need to be answered:
        Why use a attribute to split the object? If a new fem object is needed,
        a new fem object should be created. A new object will have an own Type
        and will be collected for the writer by this type.

        The category should not be used in writer! This would be the border.
        If an fem object has to be distinguished in writer it should be an own
        fem object.

        The category is just some helper to make it easier for the user to
        distinguish between different material categories.
        It can have own command, own icon, own make method. In material TaskPanel
        it can be distinguished which materials will be shown.

        ATM in calculix writer the Category is used. See comments in CalculiX Solver.
        """
