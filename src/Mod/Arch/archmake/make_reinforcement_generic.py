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

__title__ = "FreeCAD arch make generic reinforcement"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

from archobjects.reinforcement_generic import ReinforcementGeneric
from draftutils.translate import translate

if FreeCAD.GuiUp:
    import archviewproviders.view_reinforcement_generic as view_generic


def make_reinforcement_generic(
    base_rebar,
    placements=[],
    base_placement=FreeCAD.Placement(),
    name="ReinforcementGeneric"
):
    """
    make_reinforcement_generic(
        base_rebar,
        placements,
        [base_placement],
        [name]
    )
    Adds a generic reinforcement object.
    """
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject(
        "Part::FeaturePython",
        "ReinforcementGeneric"
    )
    obj.Label = translate("Arch", name)

    ReinforcementGeneric(obj)
    if FreeCAD.GuiUp:
        view_generic.ViewProviderReinforcementGeneric(obj.ViewObject)

    obj.BaseRebar = base_rebar
    obj.RebarPlacements = placements
    obj.BasePlacement = base_placement
    obj.Amount = len(placements)
    obj.TotalLength = obj.Amount * base_rebar.Length

    # mark base_rebar obj to make it collect its new child
    # TODO is touche really needed
    base_rebar.touch()
    return obj
