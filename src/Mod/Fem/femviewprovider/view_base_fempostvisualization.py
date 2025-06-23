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

import FreeCAD
import FreeCADGui


class VPPostVisualization:
    """
    A View Provider for visualization objects
    """

    def __init__(self, vobj):
        vobj.Proxy = self
        self._setup_properties(vobj)
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython")

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
        # Mark ourself as visible in the tree
        return True

    def getDisplayModes(self, obj):
        return ["Dialog"]

    def doubleClicked(self, vobj):

        guidoc = FreeCADGui.getDocument(vobj.Object.Document)

        # check if another VP is in edit mode and close it then
        if guidoc.getInEdit():
            FreeCADGui.Control.closeDialog()
            guidoc.resetEdit()

        # open task dialog
        guidoc.setEdit(vobj.Object.Name)

        # show visualization
        self.show_visualization()

        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def updateData(self, obj, prop):
        # If the data changed we need to update the visualization
        if prop == "Table":
            self.update_visualization()

    def onChanged(self, vobj, prop):
        # for all property changes we need to update the visualization
        self.update_visualization()

    def childViewPropertyChanged(self, vobj, prop):
        # One of the extractors view properties has changed, we need to
        # update the visualization
        self.update_visualization()

    def dumps(self):
        return None

    def loads(self, state):
        return None

    # To be implemented by subclasses:
    # ################################

    def update_visualization(self):
        # The visualization data or any relevant view property has changed,
        # and the visualization itself needs to update to reflect that
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def show_visualization(self):
        # Shows the visualization without going into edit mode
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_next_default_color(self):
        # Returns the next default color a new object should use
        # Returns color in FreeCAD property notation (r,g,b,a)
        # If the relevant extractors do not have color properties, this
        # can stay unimplemented
        raise FreeCAD.Base.FreeCADError("Not implemented")
