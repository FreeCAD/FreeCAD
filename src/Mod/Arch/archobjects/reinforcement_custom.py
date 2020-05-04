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

__title__ = "FreeCAD custom reinforcement object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

import ArchRebar
import DraftVecUtils
from .reinforcement_generic import ReinforcementGeneric

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP


class ReinforcementCustom(ReinforcementGeneric):

    """
    A custom reinforcement object based on a rebar object

    """

    def __init__(
        self,
        obj
    ):
        super(ReinforcementCustom, self).__init__(obj)
        self.Type = "ReinforcementCustom"

        pl = obj.PropertiesList

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
        obj.setEditorMode("Amount", 1)

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

        # custom spacing
        if "CustomSpacing" not in pl:
            obj.addProperty(
                "App::PropertyString",
                "CustomSpacing",
                "ArrayOfRebars",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The custom spacing of rebars."
                )
            )

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

        from ArchRebar import CustomSpacingPlacement
        from ArchRebar import strprocessOfCustomSpacing

        if obj.CustomSpacing:
            spacinglist = strprocessOfCustomSpacing(obj.CustomSpacing)
            borderInfluenceArea = spacinglist[0] / 2 + spacinglist[-1] / 2
            influenceArea = sum(spacinglist) - borderInfluenceArea

        if not DraftVecUtils.isNull(obj.Direction):
            axis = FreeCAD.Vector(obj.Direction)
            axis.normalize()

        if obj.Distance.Value:
            size = obj.Distance.Value

        placementlist = []
        if spacinglist:
            placementlist[:] = []
            con_cover = obj.OffsetStart.Value + obj.OffsetEnd.Value
            reqInfluenceArea = size - con_cover
            """
            if obj.ViewObject.RebarShape == "Stirrup":
                reqInfluenceArea = size - con_cover - obj.Diameter.Value
            else:
                reqInfluenceArea = size - con_cover
            """
            # Avoid unnecessary checks to pass like.
            # For eg.: when we have values
            # like influenceArea is 100.00001 and reqInflueneArea is 100
            if round(influenceArea) > round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning(
                    "Influence area of rebars is greater than {}.\n"
                    .format(reqInfluenceArea)
                )
            elif round(influenceArea) < round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning(
                    "Last span is greater than end offset.\n"
                )
            for i in range(len(spacinglist)):
                if i == 0:
                    barplacement = CustomSpacingPlacement(
                        spacinglist,
                        1,
                        axis,
                        obj.BaseRebar.Placement.Rotation,
                        obj.OffsetStart.Value,
                        obj.OffsetEnd.Value
                    )
                    placementlist.append(barplacement)
                else:
                    barplacement = ArchRebar.CustomSpacingPlacement(
                        spacinglist,
                        i+1,
                        axis,
                        obj.BaseRebar.Placement.Rotation,
                        obj.OffsetStart.Value,
                        obj.OffsetEnd.Value
                    )
                    placementlist.append(barplacement)

        obj.RebarPlacements = placementlist

        self.build_shape(obj)
        obj.Amount = len(obj.RebarPlacements)
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
