# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM reinforced material"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package material_reinforced
#  \ingroup FEM
#  \brief reinforced object

from . import material_common
from .base_fempythonobject import _PropHelper


class MaterialReinforced(material_common.MaterialCommon):
    """
    The MaterialReinforced object
    """

    Type = "Fem::MaterialReinforced"

    def __init__(self, obj):
        super().__init__(obj)

        # overwrite Category enumeration
        obj.Category = ["Solid"]

    def _get_properties(self):
        prop = super()._get_properties()

        prop.append(
            _PropHelper(
                type="App::PropertyMap",
                name="Reinforcement",
                group="Composites",
                doc="Reinforcement material properties",
                value={},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="ReinforcementUUID",
                group="Composites",
                doc="Reinforcement material UUID",
                hidden=True,
                value="",
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)

        # try update Reinforcement UUID from Reinforcement
        if not obj.ReinforcementUUID:
            obj.ReinforcementUUID = self._get_material_uuid(obj.Reinforcement)
