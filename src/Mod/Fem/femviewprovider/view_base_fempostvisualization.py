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

__title__ = "FreeCAD FEM postprocessing visualization base ViewProvider"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_base_fempostvisualizations
#  \ingroup FEM
#  \brief view provider for post visualization object

from PySide import QtGui, QtCore

import Plot
import FreeCADGui

from . import view_base_femobject
_GuiPropHelper = view_base_femobject._GuiPropHelper

class VPPostVisualization:
    """
    A View Provider for visualization objects
    """

    def __init__(self, vobj):
        vobj.Proxy = self
        self._setup_properties(vobj)

    def _setup_properties(self, vobj):
        pl = vobj.PropertiesList
        for prop in self._get_properties():
            if not prop.name in pl:
                prop.add_to_object(vobj)

    def _get_properties(self):
        return []

    def attach(self, vobj):
        self.Object = vobj.Object
        self.ViewObject = vobj

    def isShow(self):
        return True

    def doubleClicked(self,vobj):

        guidoc = FreeCADGui.getDocument(vobj.Object.Document)

        # check if another VP is in edit mode and close it then
        if guidoc.getInEdit():
            FreeCADGui.Control.closeDialog()
            guidoc.resetEdit()

        guidoc.setEdit(vobj.Object.Name)
        return True

    def show_visualization(self):
        # shows the visualization without going into edit mode
        # to be implemented by subclasses
        pass

    def get_kw_args(self, obj):
        # returns a dictionary with all visualization options needed for plotting
        # based on the view provider properties
        return {}

    def dumps(self):
        return None

    def loads(self, state):
        return None
