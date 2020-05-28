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
"""This module provides the code for Draft make_point_array function.
"""
## @package make_pointarray
# \ingroup DRAFT
# \brief This module provides the code for Draft make_point_array function.

import FreeCAD as App

import draftutils.gui_utils as gui_utils

from draftobjects.pointarray import PointArray
if App.GuiUp:
    from draftviewproviders.view_array import ViewProviderDraftArray


def make_point_array(base, ptlst):
    """make_point_array(base,pointlist)

    Make a Draft PointArray object.

    Parameters
    ----------
    base :
        TODO: describe

    plist :
        TODO: describe
    
    """
    obj = App.ActiveDocument.addObject("Part::FeaturePython", "PointArray")
    PointArray(obj, base, ptlst)
    obj.Base = base
    obj.PointList = ptlst
    if App.GuiUp:
        ViewProviderDraftArray(obj.ViewObject)
        base.ViewObject.hide()
        gui_utils.formatObject(obj,obj.Base)
        if len(obj.Base.ViewObject.DiffuseColor) > 1:
            obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        gui_utils.select(obj)
    return obj


makePointArray = make_point_array
