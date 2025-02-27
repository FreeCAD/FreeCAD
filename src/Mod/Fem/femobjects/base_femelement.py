# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM base element object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package base_femelement
#  \ingroup FEM
#  \brief base object for FEM Element Features

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class BaseFemElement(base_fempythonobject.BaseFemPythonObject):

    BaseType = "Fem::BaseFemElement"

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
                group="Element",
                doc="List of element shapes",
                value=[],
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

        if not obj.hasExtension("App::SuppressibleExtensionPython"):
            obj.addExtension("App::SuppressibleExtensionPython")
