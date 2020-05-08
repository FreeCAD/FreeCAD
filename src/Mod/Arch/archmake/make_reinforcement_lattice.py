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

__title__ = "FreeCAD arch make reinforcement lattice"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

from archobjects.reinforcement_lattice import ReinforcementLattice
from draftutils.translate import translate

# TODO guard as it is an AddOn
import lattice2BaseFeature as lattice2BF

if FreeCAD.GuiUp:
    import archviewproviders.view_reinforcement_lattice as view_lattice


def make_reinforcement_lattice(
    base_rebar,
    latice_obj,
    base_placement=FreeCAD.Placement(),
    name="ReinforcementLattice"
):
    """
    make_reinforcement_lattice(base_rebar, placements, [base_placement], [name])
    Adds a lattice reinforcement object.
    """
    if lattice2BF.isObjectLattice(latice_obj) is not True:
        FreeCAD.Console.PrintError(
            "The object provided: {} is not a Lattice2 object\n"
            .format(latice_obj.Name)
        )
        return None

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject(
        "Part::FeaturePython",
        "ReinforcementLattice"
    )
    obj.Label = translate("Arch", name)

    ReinforcementLattice(obj)
    if FreeCAD.GuiUp:
        view_lattice.ViewProviderReinforcementLattice(obj.ViewObject)

    obj.BaseRebar = base_rebar
    obj.LatticePlacement = latice_obj
    obj.BasePlacement = base_placement

    # mark base_rebar obj to make it collect its new child
    base_rebar.touch()
    return obj
