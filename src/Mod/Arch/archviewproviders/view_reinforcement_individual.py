# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD individual reinforcement view object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import Arch_rc
import ArchComponent

from .view_reinforcement_generic import ViewProviderReinforcementGeneric


False if Arch_rc.__name__ else True  # dummy usage


class ViewProviderReinforcementIndividual(ViewProviderReinforcementGeneric):

    def getIcon(
        self
    ):
        return ":/icons/Arch_ReinforcementIndividual.svg"

    def claimChildren(
        self
    ):
        # collect the children for TreeView

        # children from Arch.Component
        # since we overwrite the method we need to explicit call it
        children = ArchComponent.ViewProviderComponent.claimChildren(self)

        # reinforcement individual children
        if hasattr(self, "Object"):
            return children + self.Object.Vertieces
        else:
            return children

    # Drag and Drop for the children
    # TODO: implement Drag and Drop
