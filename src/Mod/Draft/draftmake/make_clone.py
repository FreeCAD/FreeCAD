# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to create Clone objects."""
## @package make_clone
# \ingroup draftmake
# \brief Provides functions to create Clone objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.clone import Clone

if App.GuiUp:
    from draftutils.todo import ToDo
    from draftviewproviders.view_clone import ViewProviderClone


def make_clone(obj, delta=None, forcedraft=False):
    """clone(obj,[delta,forcedraft])

    Makes a clone of the given object(s).
    The clone is an exact, linked copy of the given object. If the original
    object changes, the final object changes too.

    Parameters
    ----------
    obj :

    delta : Base.Vector
        Delta Vector to move the clone from the original position.

    forcedraft : bool
        If forcedraft is True, the resulting object is a Draft clone
        even if the input object is an Arch object.

    """

    prefix = utils.get_param("ClonePrefix","")

    cl = None

    if prefix:
        prefix = prefix.strip() + " "

    if not isinstance(obj,list):
        obj = [obj]

    if (len(obj) == 1) and obj[0].isDerivedFrom("Part::Part2DObject"):
        cl = App.ActiveDocument.addObject("Part::Part2DObjectPython","Clone2D")
        cl.Label = prefix + obj[0].Label + " (2D)"
    elif (len(obj) == 1) and (hasattr(obj[0],"CloneOf") or (utils.get_type(obj[0]) == "BuildingPart")) and (not forcedraft):
        # arch objects can be clones
        import Arch
        if utils.get_type(obj[0]) == "BuildingPart":
            cl = Arch.makeComponent()
        else:
            try: # new-style make function
                cl = getattr(Arch, "make_" + obj[0].Proxy.Type.lower())()
            except Exception:
                try: # old-style make function
                    cl = getattr(Arch, "make" + obj[0].Proxy.Type)()
                except Exception:
                    pass # not a standard Arch object... Fall back to Draft mode
        if cl:
            base = utils.get_clone_base(obj[0])
            cl.Label = prefix + base.Label
            cl.CloneOf = base
            if hasattr(cl,"Material") and hasattr(obj[0],"Material"):
                cl.Material = obj[0].Material
            if utils.get_type(obj[0]) != "BuildingPart":
                cl.Placement = obj[0].Placement
            try:
                cl.Role = base.Role
                cl.Description = base.Description
                cl.Tag = base.Tag
            except Exception:
                pass
            if App.GuiUp:
                gui_utils.format_object(cl, base)
                # Workaround to trigger update of DiffuseColor:
                ToDo.delay(reapply_diffuse_color, cl.ViewObject)
                gui_utils.select(cl)
            return cl

    # fall back to Draft clone mode
    if not cl:
        cl = App.ActiveDocument.addObject("Part::FeaturePython","Clone")
        cl.addExtension("Part::AttachExtensionPython")
        cl.Label = prefix + obj[0].Label
    Clone(cl)
    cl.Objects = obj
    if delta:
        cl.Placement.move(delta)
    elif (len(obj) == 1) and hasattr(obj[0],"Placement"):
        cl.Placement = obj[0].Placement
    if hasattr(cl,"LongName") and hasattr(obj[0],"LongName"):
        cl.LongName = obj[0].LongName
    if App.GuiUp:
        ViewProviderClone(cl.ViewObject)
        gui_utils.format_object(cl, obj[0])
        # Workaround to trigger update of DiffuseColor:
        ToDo.delay(reapply_diffuse_color, cl.ViewObject)
        gui_utils.select(cl)
    return cl


def reapply_diffuse_color(vobj):
    try:
        vobj.DiffuseColor = vobj.DiffuseColor
    except:
        pass


clone = make_clone

## @}
