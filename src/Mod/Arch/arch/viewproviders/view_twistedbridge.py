# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
## @package view_twistedbridge
# \ingroup archviewproviders
# \brief Provides the viewprovider code for the Array object.

## \addtogroup archviewproviders
# @{
from draftviewproviders.view_base import ViewProviderDraft


class ViewProviderTwistedBridge(ViewProviderDraft):
    """View provider for the twisted bridge."""

    def __init__(self,vobj):
        super(ViewProviderTwistedBridge, self).__init__(vobj)

    def getIcon(self):
        return ":/icons/Arch_TwistedBridge.svg"

## @}
