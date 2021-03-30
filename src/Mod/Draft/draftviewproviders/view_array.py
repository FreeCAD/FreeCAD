# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the viewprovider code for the Array object."""
## @package view_array
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Array object.

## \addtogroup draftviewproviders
# @{
from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderDraftArray(ViewProviderDraft):
    """a view provider that displays a Array icon instead of a Draft icon"""

    def __init__(self,vobj):
        super(ViewProviderDraftArray, self).__init__(vobj)

    def getIcon(self):
        if hasattr(self.Object, "ArrayType"):
            if self.Object.ArrayType == 'ortho':
                return ":/icons/Draft_Array.svg"
            elif self.Object.ArrayType == 'polar':
                return ":/icons/Draft_PolarArray.svg"
            elif self.Object.ArrayType == 'circular':
                return ":/icons/Draft_CircularArray.svg"
        elif hasattr(self.Object, "PointList"):
            return ":/icons/Draft_PointArray.svg"
        elif self.Object.Proxy.Type == "PathTwistedArray":
            return ":/icons/Draft_PathTwistedArray.svg"
        else:
            return ":/icons/Draft_PathArray.svg"

    def resetColors(self, vobj):
        colors = []
        if vobj.Object.Base:
            if vobj.Object.Base.isDerivedFrom("Part::Feature"):
                if len(vobj.Object.Base.ViewObject.DiffuseColor) > 1:
                    colors = vobj.Object.Base.ViewObject.DiffuseColor
                else:
                    c = vobj.Object.Base.ViewObject.ShapeColor
                    c = (c[0],c[1],c[2],vobj.Object.Base.ViewObject.Transparency/100.0)
                    colors += [c] * len(vobj.Object.Base.Shape.Faces)
        if colors:
            n = 1
            if hasattr(vobj.Object,"ArrayType"):
                if vobj.Object.ArrayType == "ortho":
                    n = vobj.Object.NumberX * vobj.Object.NumberY * vobj.Object.NumberZ
                else:
                    n = vobj.Object.NumberPolar
            elif hasattr(vobj.Object,"Count"):
                n = vobj.Object.Count
            colors = colors * n
            vobj.DiffuseColor = colors


# Alias for compatibility with v0.18 and earlier
_ViewProviderDraftArray = ViewProviderDraftArray

## @}
