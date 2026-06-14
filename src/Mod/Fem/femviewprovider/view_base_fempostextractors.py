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

from PySide import QtGui

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

        # one of our view properties was changed. Lets inform our parent visualization
        # that this happened, as this is the one that needs to redraw

        if prop == "Proxy":
            return

        group = vobj.Object.getParentGroup()
        if not group:
            return

        if hasattr(group.ViewObject, "Proxy") and hasattr(
            group.ViewObject.Proxy, "childViewPropertyChanged"
        ):

            group.ViewObject.Proxy.childViewPropertyChanged(vobj, prop)

    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_extractor._ExtractorTaskPanel(vobj.Object)

        # show it
        FreeCADGui.Control.showDialog(taskd)

        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)

        # check if another VP is in edit mode and close it then
        if guidoc.getInEdit():
            FreeCADGui.Control.closeDialog()
            guidoc.resetEdit()

        guidoc.setEdit(vobj.Object.Name)

        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    # To be implemented by subclasses:
    # ################################

    def get_default_color_property(self):
        # Returns the property name to set the default color to.
        # Return None if no such property
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_default_field_properties(self):
        # Returns the property name to which the default field name should be set
        # ret: [FieldProperty, ComponentProperty]
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_kw_args(self):
        # Returns the matplotlib plot keyword arguments that represent the
        # properties of the object.
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_app_edit_widget(self, post_dialog):
        # Returns a widgets for editing the object (not viewprovider!)
        # The widget will be part of the provided post_dialog, and
        # should use its functionality to inform of changes.
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_view_edit_widget(self, post_dialog):
        # Returns a widgets for editing the viewprovider (not object!)
        # The widget will be part of the provided post_dialog, and
        # should use its functionality to inform of changes.
        raise FreeCAD.Base.FreeCADError("Not implemented")

    def get_preview(self):
        # Returns the preview tuple of icon and label: (QPixmap, str)
        # Note: QPixmap in ratio 2:1
        raise FreeCAD.Base.FreeCADError("Not implemented")
