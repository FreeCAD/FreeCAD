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
"""Provides functions to create WorkingPlaneProxy objects."""
## @package make_wpproxy
# \ingroup draftmake
# \brief Provides functions to create WorkingPlaneProxy objects.

## \addtogroup draftmake
# @{
import FreeCAD as App

from draftobjects.wpproxy import WorkingPlaneProxy

if App.GuiUp:
    from draftviewproviders.view_wpproxy import ViewProviderWorkingPlaneProxy


def make_workingplaneproxy(placement):
    """make_working_plane_proxy(placement)

    Creates a Working Plane proxy object in the current document.

    Parameters
    ----------
    placement : Base.Placement
        specify the p.
    """
    if App.ActiveDocument:
        obj = App.ActiveDocument.addObject("App::FeaturePython","WPProxy")
        WorkingPlaneProxy(obj)
        if App.GuiUp:
            ViewProviderWorkingPlaneProxy(obj.ViewObject)
            obj.ViewObject.Proxy.writeCamera()
            obj.ViewObject.Proxy.writeState()
        obj.Placement = placement
        return obj


makeWorkingPlaneProxy = make_workingplaneproxy

## @}
