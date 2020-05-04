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

__title__ = "FreeCAD arch make reinforcement custom"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FreeCAD import Vector as vec

from DraftTools import translate

# see linear


def makeReinforcementCustom(
    base_rebar,
    custom_spacing,
    direction=vec(0, 0, 1),
    base_placement=FreeCAD.Placement(),
    name="ReinforcementCustom"
):
    """
    makeReinforcementCustom(
        base_rebar,
        customspacing,
        [base_placement],
        [name]
    )
    Adds a custom reinforcement object.
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    obj = FreeCAD.ActiveDocument.addObject(
        "Part::FeaturePython",
        "ReinforcementCustom"
    )
    obj.Label = translate("Arch", name)

    from archobjects.reinforcement_custom import ReinforcementCustom
    ReinforcementCustom(obj)
    if FreeCAD.GuiUp:
        import archviewproviders.view_reinforcement_custom as view_custom
        view_custom.ViewProviderReinforcementCustom(obj.ViewObject)

    obj.BaseRebar = base_rebar
    obj.CustomSpacing = custom_spacing
    obj.BasePlacement = base_placement
    obj.Direction = direction
    obj.Document.recompute()

    # mark base_rebar obj for recompute to make it collect its new child
    base_rebar.touch()
    obj.Document.recompute()
    return obj
