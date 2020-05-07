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

__title__ = "FreeCAD linear reinforcement object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD

import ArchComponent
import DraftVecUtils
from .reinforcement_generic import ReinforcementGeneric


class ReinforcementLinear(ReinforcementGeneric):

    """
    A linear reinforcement object based on a rebar object

    """

    def __init__(
        self,
        obj
    ):
        super(ReinforcementLinear, self).__init__(obj)
        self.Type = "ReinforcementLinear"

        pl = obj.PropertiesList

        # user needs be able to change Amount in PropertyEditor
        obj.setEditorMode("Amount", 0)
        obj.Amount = 3

        # New properties

        # direction
        # is not only direction but startpoint too
        # TODO might be later given by a straight edge
        if "Direction" not in pl:
            obj.addProperty(
                "App::PropertyVector",
                "Direction",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "The direction to use to spread the bars. "
                        "Keep (0,0,0) for automatic direction."
                    )
                )
            )
        obj.Direction = FreeCAD.Vector(0, 0, 1)

        # length of the rebar distrbution
        # later if we use a edge too, 0 indicates the use of the edge length
        if "Distance" not in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Distance",
                "Reinforcement",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "The total distance to span the rebars over. "
                        # "Keep 0 to automatically use the host shape size."
                    )
                )
            )
        obj.Distance = 400

        # concrete cover start
        if "OffsetStart" not in pl:
            obj.addProperty(
                "App::PropertyLength",
                "OffsetStart",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "The distance between the start of the "
                        "reinforcement and the first bar (concrete cover)."
                    )
                ),
            )
        obj.OffsetStart = 0

        # concrete cover end
        if "OffsetEnd" not in pl:
            obj.addProperty(
                "App::PropertyLength",
                "OffsetEnd",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "The distance between the end of the "
                        "reinforcement and the last bar (concrete cover)."
                    )
                ),
            )
        obj.OffsetEnd = 0

        # space between rebars
        if "Spacing" not in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Spacing",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The spacing between the bars"
                )
            )
        obj.Spacing = 200

        # fixed value
        # this value will fixed if one of the others is changed
        # the third will be calculated thus:
        # one value will be changed by the user
        # one value is fixed by this attribute
        # one value will be recalculated onChanged()
        if "FixedAttribut" not in pl:
            obj.addProperty(
                "App::PropertyEnumeration",
                "FixedAttribut",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    (
                        "The value which is fixed whereas one of the others "
                        "is changed and the third will be recalculated."
                    )
                )
            )
        obj.FixedAttribut = [
            "Amount",
            "Distance",
            "Spacing"
        ]
        obj.FixedAttribut = "Distance"

        """
        # AddRestTo
        # if ChangeableValue is Amount than there could be some rest
        # it is to choose what should happen with rest
        obj.addProperty(
            "App::PropertyEnumeration",
            "AddRestTo",
            "ArrayOfRebars",
            "Where should the rest be added if ChangeableValue is Amount."
        )
        obj.AddRestTo = [
            "OffsetStart_and_OffsetEnd",
            "OffsetStart",
            "OffsetEnd",
            "Adapt_Distance"
        ]
        obj.AddRestTo = "OffsetEnd"
        """

    def onChanged(
        self,
        obj,
        prop
    ):

        # since we overwrite the method we need to explicit call it
        ArchComponent.Component.onChanged(self, obj, prop)

        # before an attribute will be set check the value
        # if it will be set onChanged will be called attribute will be set ...
        # bang, crash
        # onChanged is called on any change even on
        # init of the obj after any prop
        # check for prop existence of all needed props

        # https://forum.freecadweb.org/viewtopic.php?f=22&t=37157
        # https://forum.freecadweb.org/viewtopic.php?f=22&t=39106

        """
                    # some circle, bang, crash
                    #if obj.AddRestTo == "OffsetStart_and_OffsetEnd":
                    #    new_offsetstart = obj.OffsetStart.Value + 0.5 * rest
                    #    new_offsetend = obj.OffsetEnd.Value + 0.5 * rest
                    #    if obj.OffsetStart.Value != new_offsetstart:
                    #        obj.OffsetStart.Value = new_offsetstart
                    #    if obj.OffsetEnd.Value != new_offsetend:
                    #        obj.OffsetEnd.Value = new_offsetend
                    #elif obj.AddRestTo == "OffsetStart":
                    #    new_offsetstart = obj.OffsetStart.Value + rest
                    #    if obj.OffsetStart.Value != new_offsetstart:
                    #        obj.OffsetStart.Value = new_offsetstart
                    #elif obj.AddRestTo == "OffsetEnd":
                    #    new_offsetend = obj.OffsetEnd.Value + rest
                    #    if obj.OffsetEnd.Value != new_offsetend:
                    #        obj.OffsetEnd.Value = new_offsetend
                    #elif obj.AddRestTo == "Adapt_Distance":
                    #    new_distance = obj.Distance.Value - con_cover - rest
                    #    if obj.Distance.Value != new_distance:
                    #        obj.Distance.Value = new_distance
        """

        # fixed: Distance, changed: Spacing, calculated: Amount, rest: printed
        # fixed: Spacing, changed: Distance, calculated: Amount, rest: printed

        # fixed: Amount, changed: Distance, calculated: Spacing, no rest
        # fixed: Distance, changed: Amount, calculated: Spacing, no rest

        # fixed: Amount, changed: Spacing, calculated: Distance, no rest
        # fixed: Spacing, changed: Amount, calculated: Distance, no rest

        if (
            (
                prop == "Amount"
                or prop == "Distance"
                or prop == "OffsetEnd"
                or prop == "OffsetStart"
                or prop == "Spacing"
            )
            and hasattr(obj, "Amount")
            and hasattr(obj, "Distance")
            and hasattr(obj, "OffsetEnd")
            and hasattr(obj, "OffsetStart")
            and hasattr(obj, "Spacing")
            and hasattr(obj, "FixedAttribut")
        ):

            # TODO: concrete cover changed
            # if there was some rest this will be deleted
            # TODO treat the rest first, there should never be some
            # con cover changed Amount is fixed, fix Distance, change Spacing
            # con cover changed Distance is fixed, fix Amount, change Spacing
            # con cover changed Spacing is fixed, fix Amount, change Distance
            # work around, print message
            if prop == "OffsetStart" or prop == "OffsetEnd":
                print("onChanged() not yet implemented")

            con_cover = obj.OffsetEnd.Value + obj.OffsetStart.Value

            if (
                obj.FixedAttribut == "Amount" and prop == "Distance"
                or obj.FixedAttribut == "Distance" and prop == "Amount"
            ):
                # Attribute to recalculate is Spacing
                dist_rebars = obj.Distance.Value - con_cover
                if obj.Amount > 1:
                    spaceing = dist_rebars / (obj.Amount - 1)
                    if obj.Spacing.Value != spaceing:
                        obj.Spacing = spaceing
                else:
                    print("Use more than 1 as Amount, 1 is on TODO")
            elif (
                obj.FixedAttribut == "Distance" and prop == "Spacing"
                or obj.FixedAttribut == "Spacing" and prop == "Distance"
            ):
                # Attribute to recalculate is Amount
                dist_rebars = obj.Distance.Value - con_cover
                amount = int(dist_rebars // obj.Spacing.Value) + 1
                if obj.Amount != amount:
                    obj.Amount = amount
                new_dist_rebars = (obj.Amount - 1) * obj.Spacing.Value
                rest = dist_rebars - new_dist_rebars
                if rest == 0:
                    pass
                elif rest < 0:
                    print("Rest negative, should not happen.")
                elif rest >= obj.Spacing.Value:
                    print("Rest >= Spacing, should not happen.")
                else:
                    print("Rest {}".format(rest))
            elif (
                obj.FixedAttribut == "Amount" and prop == "Spacing"
                or obj.FixedAttribut == "Spacing" and prop == "Amount"
            ):
                # Attribute to recalculate is Distance
                dist_rebars = (obj.Amount - 1) * obj.Spacing.Value
                distance = dist_rebars + con_cover
                if obj.Distance.Value != distance:
                    obj.Distance.Value = distance

    def execute(
        self,
        obj  # why obj? self is the obj?
    ):

        # what should be used to access an attribute
        # self.Attribute
        # obj.Attribute
        # same in onChanged()

        if self.clone(obj):
            return
        if not obj.BaseRebar:
            return
        # should we check for more Attributes?

        # TODO What happens if
        # Amount, Distance, OffsetEnd, OffsetStart, Spacing
        # do not fit together (means there is some rest)

        pl_list = []
        rot = FreeCAD.Rotation()
        move = obj.OffsetStart.Value
        for i in range(obj.Amount):
            barlocation = DraftVecUtils.scaleTo(obj.Direction, move)
            pl_list.append(FreeCAD.Placement(barlocation, rot))
            move += obj.Spacing.Value  # the first should not be moved
        obj.RebarPlacements = pl_list

        self.build_shape(obj)
        obj.TotalLength = obj.Amount * obj.BaseRebar.Length

        # set Visibility of BaseRebar
        # this should be done in the Gui Command,
        # but this dos not yet exist TODO
        # set view of base rebar to off
        # if reinforcement shape is not a null shape
        # TODO may be use another color for base rebar
        if FreeCAD.GuiUp:
            if obj.Shape.isNull() is not True:
                obj.BaseRebar.ViewObject.Visibility = False
