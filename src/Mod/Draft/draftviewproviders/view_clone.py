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
"""Provides the viewprovider code for the Clone object."""
## @package view_clone
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Clone object.

## \addtogroup draftviewproviders
# @{
import draftutils.groups as groups


class ViewProviderClone:
    """a view provider that displays a Clone icon instead of a Draft icon"""

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Draft_Clone.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getDisplayModes(self, vobj):
        modes=[]
        return modes

    def setDisplayMode(self, mode):
        return mode

    def resetColors(self, vobj):
        colors = []
        for o in groups.get_group_contents(vobj.Object.Objects):
            if o.isDerivedFrom("Part::Feature"):
                if len(o.ViewObject.DiffuseColor) > 1:
                    colors.extend(o.ViewObject.DiffuseColor)
                else:
                    c = o.ViewObject.ShapeColor
                    c = (c[0],c[1],c[2],o.ViewObject.Transparency/100.0)
                    colors += [c] * len(o.Shape.Faces) # TODO: verify this line
            elif o.hasExtension("App::GeoFeatureGroupExtension"):
                for so in vobj.Object.Group:
                    if so.isDerivedFrom("Part::Feature"):
                        if len(so.ViewObject.DiffuseColor) > 1:
                            colors.extend(so.ViewObject.DiffuseColor)
                        else:
                            c = so.ViewObject.ShapeColor
                            c = (c[0],c[1],c[2],so.ViewObject.Transparency/100.0)
                            colors += [c] * len(so.Shape.Faces)
        if colors:
            vobj.DiffuseColor = colors


# Alias for compatibility with v0.18 and earlier
_ViewProviderClone = ViewProviderClone

## @}
