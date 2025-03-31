
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

__title__ = "FreeCAD FEM postprocessing line plot ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_post_lineplot
#  \ingroup FEM
#  \brief view provider for post line plot object

import FreeCAD
import FreeCADGui

import FemGui
from PySide import QtGui

import femobjects.base_fempostextractors as fpe
from femtaskpanels import task_post_extractor

class VPPostExtractor:
    """
    A View Provider for extraction of data
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
        self.Object = vobj.Object  # used on various places, claim childreens, get icon, etc.
        self.ViewObject = vobj

    def isShow(self):
        return True

    def onChanged(self, vobj, prop):

        # one of our view properties was changed. Lets inform our parent plot
        # that this happend, as this is the one that needs to redraw

        if prop == "Proxy":
            return

        group = vobj.Object.getParentGroup()
        if not group:
            return

        if (hasattr(group.ViewObject, "Proxy") and
            hasattr(group.ViewObject.Proxy, "childViewPropertyChanged")):

            group.ViewObject.Proxy.childViewPropertyChanged(vobj, prop)


    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_extractor._ExtractorTaskPanel(vobj.Object)

        #show it
        FreeCADGui.Control.showDialog(taskd)

        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)

        # check if another VP is in edit mode and close it then
        if guidoc.getInEdit():
            FreeCADGui.Control.closeDialog()
            guidoc.resetEdit()

        guidoc.setEdit(vobj.Object.Name)

        return True

    def get_kw_args(self):
        # should return the plot keyword arguments that represent the properties
        # of the object
        return {}

    def get_edit_widgets(self, post_dialog):
        # Returns a list of widgets for editing the object/viewprovider.
        # The widget will be part of the provided post_dialog, and
        # should use its functionality to inform of changes.
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_preview_widget(self, post_dialog):
        # Returns a widget for editing the object/viewprovider.
        # The widget will be part of the provided post_dialog, and
        # should use its functionality to inform of changes.
        raise FreeCAD.Base.FreeCADError("Not implemented")


    def dumps(self):
        return None

    def loads(self, state):
        return None
