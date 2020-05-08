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

__title__ = "FreeCAD arch make base rebar"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

from draftutils.translate import translate


def make_base_rebar(
    base,
    diameter=None,
    mark=None,
    name="BaseRebar"
):
    """
    make_base_rebar(base, [diameter, mark, name]):
    Adds a Reinforcement Bar object using the given base
    (sketch or wire) as sweep path.
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")

    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Rebar")
    obj.Label = translate("Arch", name)
    # may be set the label to the mark number
    # or even have an attribute which does it on any obj recompute

    from archobjects.base_rebar import BaseRebar
    BaseRebar(obj)
    if FreeCAD.GuiUp:
        from archviewproviders.view_base_rebar import ViewProviderBaseRebar
        ViewProviderBaseRebar(obj.ViewObject)

    obj.Base = base
    if FreeCAD.GuiUp:
        base.ViewObject.hide()
    if diameter:
        obj.Diameter = diameter
    else:
        obj.Diameter = p.GetFloat("RebarDiameter", 6)
    if mark:
        obj.MarkNumber = mark
    else:
        obj.MarkNumber = 1

    return obj
