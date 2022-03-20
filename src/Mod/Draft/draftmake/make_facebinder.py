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
"""Provides functions to create Facebinder objects."""
## @package make_facebinder
# \ingroup draftmake
# \brief Provides functions to create Facebinder objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftobjects.facebinder import Facebinder

if App.GuiUp:
    from draftviewproviders.view_facebinder import ViewProviderFacebinder


def make_facebinder(selectionset, name="Facebinder"):
    """make_facebinder(selectionset, [name])

    Creates a Facebinder object from a selection set.

    Parameters
    ----------
    selectionset :
        Only faces will be added.

    name : string (default = "Facebinder")
        Name of the created object
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(selectionset,list):
        selectionset = [selectionset]
    fb = App.ActiveDocument.addObject("Part::FeaturePython",name)
    Facebinder(fb)
    if App.GuiUp:
        ViewProviderFacebinder(fb.ViewObject)
    faces = [] # unused variable?
    fb.Proxy.addSubobjects(fb, selectionset)
    gui_utils.select(fb)
    return fb


makeFacebinder = make_facebinder

## @}
