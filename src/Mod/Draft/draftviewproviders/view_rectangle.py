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
"""Provides the viewprovider code for the Rectangle object."""
## @package view_rectangle
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Rectangle object.

## \addtogroup draftviewproviders
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderRectangle(ViewProviderDraft):

    def __init__(self,vobj):
        super(ViewProviderRectangle, self).__init__(vobj)

        _tip = "Defines a texture image (overrides hatch patterns)"
        vobj.addProperty("App::PropertyFile","TextureImage",
                         "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))


# Alias for compatibility with v0.18 and earlier
_ViewProviderRectangle = ViewProviderRectangle

## @}
