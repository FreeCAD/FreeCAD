# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM postprocessing glyph filter ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_post_glyphfilter
#  \ingroup FEM
#  \brief view provider for post glyph filter object

import FreeCAD
import FreeCADGui

import FemGui
from PySide import QtGui
from femtaskpanels import task_post_glyphfilter


class VPPostGlyphFilter:
    """
    A View Provider for the PostGlyphFilter object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/FEM_PostFilterGlyph.svg"

    def getDisplayModes(self, obj):
        # Mandatory, as the ViewProviderPostFilterPython does not add any
        # display modes. We can choose here any that is supported by it:
        # "Outline", "Nodes", "Surface", "Surface with Edges",
        # "Wireframe", "Wireframe (surface only)", "Nodes (surface only)"

        # only surface makes sense for the glyphs
        return ["Surface"]

    def setDisplayMode(self, mode):
        # the post object viewprovider implements the different display modes
        # via vtk filter, not via masking modes. Hence we need to make sure
        # to always stay in the "Default" masking mode, no matter the display mode
        return "Default"

    def setEdit(self, vobj, mode):
        # make sure we see what we edit
        vobj.show()

        # build up the task panel
        taskd = task_post_glyphfilter._TaskPanel(vobj)

        # show it
        FreeCADGui.Control.showDialog(taskd)

        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None
