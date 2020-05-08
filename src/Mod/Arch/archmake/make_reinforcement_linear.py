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

__title__ = "FreeCAD arch make reinforcement linear"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FreeCAD import Vector as vec

from archobjects.reinforcement_linear import ReinforcementLinear
from draftutils.translate import translate

if FreeCAD.GuiUp:
    import archviewproviders.view_reinforcement_linear as view_linear


# TODO put this somewhere else where it makes sense
# Does it makes sense to put OffsetEnd and OffsetStart in make too?
# on ifc import they are 0, they will not be needed there
# they would be needed it direction and distance will be set with and real edge
# do not put them at the moment, set them to 0, done in init

# would be good to be able to create the object with the make only
# thus 2 of these 3 are needed, the third is calculated
# Amount, Distance, Spacing
#
# If two are given
# calculate the third
#
# If all are given
# ATM: print, not yet supported
# do some test need to be make if there is some rest
# What if there is a rest.
# rest:
# Distance is fixed, fix Amount, change Spacing
# Amount is fixed, fix Distance, change Spacing
# Spacing is fixed, fix Amount, change Distance
# in any case print a message what was changed
#
# If only one is given:
# ATM: print, not yet supported
#
# If none of them is given:
# ATM: print, not yet supported


def make_reinforcement_linear(
    base_rebar,
    amount=None,
    spacing=None,
    distance=None,
    direction=vec(0, 0, 1),
    base_placement=FreeCAD.Placement(),
    name="ReinforcementLinear"
):
    """
    make_reinforcement_linear(
        base_rebar,
        [amount],
        [spacing],
        [distance],
        [direction],
        [base_placement],
        [name]
    )
    Adds a linear reinforcement object.
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    # print("amount: {}".format(amount))
    # print("distance: {}".format(distance))
    # print("spacing: {}".format(spacing))
    if (
        (amount is None and spacing is None)
        or (amount is None and distance is None)
        or (distance is None and spacing is None)
        or (amount is None and distance is None and spacing is None)
        or (
            amount is not None
            and distance is not None
            and spacing is not None
        )
    ):
        FreeCAD.Console.PrintError(
            "This combination of parameter is not yet supported. Aborting\n"
        )
        return

    obj = FreeCAD.ActiveDocument.addObject(
        "Part::FeaturePython",
        "ReinforcementLinear"
    )
    obj.Label = translate("Arch", name)

    ReinforcementLinear(obj)
    if FreeCAD.GuiUp:
        view_linear.ViewProviderReinforcementLinear(obj.ViewObject)

    obj.BaseRebar = base_rebar
    obj.BasePlacement = base_placement
    obj.Direction = direction

    if distance is None:
        obj.FixedAttribut = "Amount"
        obj.Amount = amount
        obj.Spacing = spacing

    if amount is None:
        obj.FixedAttribut = "Distance"
        obj.Distance = distance
        obj.Spacing = spacing

    if spacing is None:
        obj.FixedAttribut = "Spacing"
        obj.Distance = distance
        obj.Amount = amount

    # mark base_rebar obj to make it collect its new child
    base_rebar.touch()
    return obj


"""
# test above code
import FreeCAD, Draft, archadd
from FreeCAD import Vector as vec
wire = Draft.makeWire([vec(0, 0, 0), vec(1000, 0, 0)])
baserebar = archadd.BaseRebar(wire, diameter=30, mark=1, name="BaseRebarTest")

from archobjects.reinforcement_linear import ReinforcementLinear
import archviewproviders.view_reinforcement_linear as view_linear
obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","RLinear")
ReinforcementLinear(obj)
view_linear.ViewProviderReinforcementLinear(obj.ViewObject)

obj.BaseRebar = baserebar
obj.BasePlacement = FreeCAD.Placement()
obj.Direction = vec(0, 0, 1)

obj.FixedAttribut = "Amount"
obj.Amount = 20
obj.Spacing = 100
obj.Document.recompute()  # do no earlier recompute()


# test combinations
import FreeCAD, Draft, archadd
from FreeCAD import Vector as vec
wire = Draft.makeWire([vec(0, 0, 0), vec(1000, 0, 0)])
baserebar = archadd.BaseRebar(wire, diameter=30, mark=1, name="BaseRebarTest")
archadd.ReinforcementLinear(baserebar, amount=20)
archadd.ReinforcementLinear(baserebar, spacing=100)
archadd.ReinforcementLinear(baserebar, distance=1000)
archadd.ReinforcementLinear(baserebar, amount=20, spacing=100, distance=1000)
archadd.ReinforcementLinear(baserebar)

"""
