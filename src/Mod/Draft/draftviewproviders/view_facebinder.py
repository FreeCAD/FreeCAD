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
"""Provides the viewprovider code for the Facebinder object."""
## @package view_facebinder
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Facebinder object.

## \addtogroup draftviewproviders
# @{
import FreeCADGui as Gui

from draftviewproviders.view_base import ViewProviderDraft

class ViewProviderFacebinder(ViewProviderDraft):

    def __init__(self, vobj):
        super(ViewProviderFacebinder, self).__init__(vobj)

    def getIcon(self):
        return ":/icons/Draft_Facebinder_Provider.svg"

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        import DraftGui # Moving this to the top of the file results in a circular import.
        taskd = DraftGui.FacebinderTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        Gui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        return True


# Alias for compatibility with v0.18 and earlier
_ViewProviderFacebinder = ViewProviderFacebinder

## @}
