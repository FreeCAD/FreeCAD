# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the viewprovider code for the Point object."""
## @package view_point
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Point object.

## \addtogroup draftviewproviders
# @{
from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderPoint(ViewProviderDraft):
    """A viewprovider for the Draft Point object"""
    def __init__(self, obj):
        super(ViewProviderPoint, self).__init__(obj)

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineColor', mode)
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Deviation', mode)
        vobj.setEditorMode('DiffuseColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('Lighting', mode)
        vobj.setEditorMode('LineMaterial', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('ShapeMaterial', mode)
        vobj.setEditorMode('Transparency', mode)

    def getIcon(self):
        return ":/icons/Draft_Dot.svg"

    def doubleClicked(self, vobj):
        # See setEdit in ViewProviderDraft.
        import FreeCADGui as Gui
        Gui.runCommand("Std_TransformManip")
        return True


# Alias for compatibility with v0.18 and earlier
_ViewProviderPoint = ViewProviderPoint

## @}
