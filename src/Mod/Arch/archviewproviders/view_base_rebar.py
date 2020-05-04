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

__title__ = "FreeCAD base rebar view object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

import ArchComponent
import Draft

from .view_rebar_generic import ViewProviderRebarCommon


class ViewProviderBaseRebar(ViewProviderRebarCommon):

    def getIcon(
        self
    ):
        import Arch_rc
        False if Arch_rc.__name__ else True  # dummy usage
        return ":/icons/Arch_RebarBase.svg"

    def onDelete(self, feature, subelements):
        try:
            for o in self.claimChildren():
                o.ViewObject.show()
        except Exception:
            FreeCAD.Console.PrintError("Error in onDelete: ")
        return True

    def claimChildren(
        self
    ):
        # collect the children for TreeView

        # children from Arch.Component
        # since we overwrite the method we need to explicit call it
        children = ArchComponent.ViewProviderComponent.claimChildren(self)

        # special rebar shape children
        if hasattr(self, "Object"):

            # claim reinforcements for this rebar
            for o in self.Object.Document.Objects:
                # print(Draft.getType(o))
                if (
                    Draft.getType(o) == "ReinforcementGeneric"
                    or Draft.getType(o) == "ReinforcementLinear"
                    or Draft.getType(o) == "ReinforcementLattice"
                    or Draft.getType(o) == "ReinforcementIndividual"
                    or Draft.getType(o) == "ReinforcementCustom"
                ):
                    if o.BaseRebar == self.Object:
                        children.append(o)

            # print(children)
            return children
        return children

    # Drag and Drop for the children
    """
    # https://forum.freecadweb.org/viewtopic.php?f=10&t=28760
    # https://forum.freecadweb.org/viewtopic.php?f=10&t=37632
    # drag ... in German: ausschneiden, ziehen
    # drop ... in German: loslassen, einfuegen, ablegen
    """
    # TODO !!!!!!!!!!!!!!!!!!!!!!
    # it is possible to copy a reinforcement into another base rebar
    # this should not be possible
    # a reinforcement can only have one base rebar, see class reinforcement
    def canDragObjects(self):
        return True

    def canDropObjects(self):
        return True

    def canDragObject(self, dragged_object):
        if (
            Draft.getType(dragged_object) == "ReinforcementGeneric"
            or Draft.getType(dragged_object) == "ReinforcementLinear"
            or Draft.getType(dragged_object) == "ReinforcementLattice"
            or Draft.getType(dragged_object) == "ReinforcementIndividual"
            or Draft.getType(dragged_object) == "ReinforcementCustom"
        ):
            return True
        else:
            return False

    def canDropObject(self, incoming_object):
        return True

    def dragObject(self, selfvp, dragged_object):
        if (
            Draft.getType(dragged_object) == "ReinforcementGeneric"
            or Draft.getType(dragged_object) == "ReinforcementLinear"
            or Draft.getType(dragged_object) == "ReinforcementLattice"
            or Draft.getType(dragged_object) == "ReinforcementIndividual"
            or Draft.getType(dragged_object) == "ReinforcementCustom"
        ):
            dragged_object.BaseRebar = None
            # mark the object we move out to recompute
            selfvp.Object.touch()
        FreeCAD.ActiveDocument.recompute()

    def dropObject(self, selfvp, incoming_object):
        if (
            Draft.getType(incoming_object) == "ReinforcementGeneric"
            or Draft.getType(incoming_object) == "ReinforcementLinear"
            or Draft.getType(incoming_object) == "ReinforcementLattice"
            or Draft.getType(incoming_object) == "ReinforcementIndividual"
            or Draft.getType(incoming_object) == "ReinforcementCustom"
        ):
            incoming_object.BaseRebar = selfvp.Object
            # mark the object we move in to recompute
            selfvp.Object.touch()
        FreeCAD.ActiveDocument.recompute()
