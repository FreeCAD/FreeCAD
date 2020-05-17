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
"""This module provides the code for Draft make_path_array function.
"""
## @package make_patharray
# \ingroup DRAFT
# \brief This module provides the code for Draft make_path_array function.

import FreeCAD as App
import FreeCADGui as Gui

import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.translate import _tr, translate
from draftobjects.patharray import PathArray

from draftviewproviders.view_draftlink import ViewProviderDraftLink
if App.GuiUp:
    from draftviewproviders.view_array import ViewProviderDraftArray


def make_path_array(baseobject,pathobject,count,xlate=None,align=False,pathobjsubs=[],use_link=False):
    """make_path_array(docobj, path, count, xlate, align, pathobjsubs, use_link)
    
    Make a Draft PathArray object.
    
    Distribute count copies of a document baseobject along a pathobject 
    or subobjects of a pathobject. 

    
    Parameters
    ----------
    docobj : 
        Object to array

    path : 
        Path object

    pathobjsubs : 
        TODO: Complete documentation

    align : 
        Optionally aligns baseobject to tangent/normal/binormal of path. TODO: verify

    count : 
        TODO: Complete documentation

    xlate : Base.Vector
        Optionally translates each copy by FreeCAD.Vector xlate direction
        and distance to adjust for difference in shape centre vs shape reference point.
        
    use_link :
        TODO: Complete documentation
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if use_link:
        obj = App.ActiveDocument.addObject("Part::FeaturePython","PathArray", PathArray(None), None, True)
    else:
        obj = App.ActiveDocument.addObject("Part::FeaturePython","PathArray")
        PathArray(obj)

    obj.Base = baseobject
    obj.PathObj = pathobject

    if pathobjsubs:
        sl = []
        for sub in pathobjsubs:
            sl.append((obj.PathObj,sub))
        obj.PathSubs = list(sl)

    if count > 1:
        obj.Count = count

    if xlate:
        obj.Xlate = xlate

    obj.Align = align

    if App.GuiUp:
        if use_link:
            ViewProviderDraftLink(obj.ViewObject)
        else:
            ViewProviderDraftArray(obj.ViewObject)
            gui_utils.formatObject(obj,obj.Base)
            if hasattr(obj.Base.ViewObject, "DiffuseColor"):
                if len(obj.Base.ViewObject.DiffuseColor) > 1:
                    obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        baseobject.ViewObject.hide()
        gui_utils.select(obj)
    return obj


makePathArray = make_path_array
