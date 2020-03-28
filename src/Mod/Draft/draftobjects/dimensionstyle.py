# ***************************************************************************
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

"""This module provides the object code for Draft DimensionStyle.
"""
## @package dimensionstyle
# \ingroup DRAFT
# \brief This module provides the object code for Draft DimensionStyle.

import FreeCAD as App
from PySide.QtCore import QT_TRANSLATE_NOOP
from draftobjects.draft_annotation import DraftAnnotation
from draftobjects.draft_annotation import StylesContainerBase

if App.GuiUp:
    import FreeCADGui as Gui
    from draftviewproviders.view_dimensionstyle import ViewProviderDimensionStyle
    from draftviewproviders.view_dimensionstyle import ViewProviderDimensionStylesContainer

def make_dimension_style(existing_dimension = None):
    """
    Make dimension style
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    obj = App.ActiveDocument.addObject("App::FeaturePython","DimensionStyle")
    DimensionStyle(obj)
    if App.GuiUp:
        ViewProviderDimensionStyle(obj.ViewObject, existing_dimension)
    get_dimension_styles_container().addObject(obj)
    return obj

def get_dimension_styles_container():
    """get_dimension_styles_container(): returns a group object to put dimensions in"""
    for obj in App.ActiveDocument.Objects:
        if obj.Name == "DimensionStyles":
            return obj
    obj = App.ActiveDocument.addObject("App::DocumentObjectGroupPython", "DimensionStyles")
    obj.Label = QT_TRANSLATE_NOOP("draft", "Dimension Styles")
    DimensionStylesContainer(obj)
    if App.GuiUp:
        ViewProviderDimensionStylesContainer(obj.ViewObject)
    return obj


class DimensionStylesContainer(StylesContainerBase):
    """The Dimension Container"""

    def __init__(self, obj):
        super().__init__(obj, tp = "DimensionStyles")

        # init properties

        obj.addProperty("App::PropertyLink","ActiveDimensionStyle",
                        "Annotation",
                        QT_TRANSLATE_NOOP("App::Property",
                                          "Active dimension style"))

        # sets properties read only
        obj.setEditorMode("Visibility", 1) 
        obj.setEditorMode("ActiveDimensionStyle", 1)
                                        

    def onChanged(self, obj, prop):
        if prop == "Visibility" and hasattr(obj, "Visibility"):
            if obj.Visibility == False:
                obj.Visibility = True
            if hasattr(obj, "ActiveDimensionStyle"):
                if obj.ActiveDimensionStyle:
                    super().make_unique_visible(obj, obj.ActiveDimensionStyle)

        if prop == "ActiveDimensionStyle" and hasattr(obj, "ActiveDimensionStyle"):
            super().make_unique_visible(obj, obj.ActiveDimensionStyle)


class DimensionStyle(DraftAnnotation):
    def __init__(self, obj):

        super().__init__(obj, "DimensionStyle")

        obj.setEditorMode("Visibility", 1) # sets visibility read only


    def onChanged(self, obj, prop):
        """ visibility property controls setting the activeDimensionStyle
        so the only visible style is the current one
        """
        if prop == "Visibility" and hasattr(obj, "Visibility"):
            if obj.Visibility == True:
                self.set_current(obj)
            elif obj.Visibility == False:
                self.remove_from_current(obj)

    def set_visible(self, obj):
        obj.Visibility = True

    def set_current(self, obj):
        get_dimension_styles_container().ActiveDimensionStyle = obj

    def remove_from_current(self, obj):
        if get_dimension_styles_container().ActiveDimensionStyle:
            if get_dimension_styles_container().ActiveDimensionStyle.Name == obj.Name:
                get_dimension_styles_container().ActiveDimensionStyle = None
