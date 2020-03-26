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
from draftobjects.draft_annotation import DraftAnnotation
from PySide.QtCore import QT_TRANSLATE_NOOP
from draftobjects.draft_annotation import AnnotationStylesContainer

if App.GuiUp:
    import FreeCADGui as Gui
    from draftviewproviders.view_dimensionstyle import ViewProviderDraftDimensionStyle
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
        ViewProviderDraftDimensionStyle(obj.ViewObject, existing_dimension)
    get_dimension_style_container().addObject(obj)
    return obj

def get_dimension_style_container():
    """get_dimension_style_container(): returns a group object to put dimensions in"""
    for obj in App.ActiveDocument.Objects:
        if obj.Name == "DimensionStyleContainer":
            return obj
    obj = App.ActiveDocument.addObject("App::DocumentObjectGroupPython", "DimensionStyleContainer")
    obj.Label = QT_TRANSLATE_NOOP("draft", "Dimension Styles")
    DimensionStylesContainer(obj)
    if App.GuiUp:
        ViewProviderDimensionStylesContainer(obj.ViewObject)
    return obj


class DimensionStylesContainer(AnnotationStylesContainer):
    """The Dimension Container"""

    def __init__(self, obj):
        super().__init__(obj)
        self.Type = "DimensionStyleContainer"
        obj.Proxy = self

    def execute(self, obj):

        g = obj.Group
        g.sort(key=lambda o: o.Label)
        obj.Group = g


class DimensionStyle(DraftAnnotation):
    def __init__(self, obj):
        super().__init__(obj, "DimensionStyle")
    
    def set_current(self, obj):
        "turn non visible all the concurrent styles"
        for o in get_dimension_style_container().Group:
            if hasattr(o, "Visibility"):
                o.Visibility = False
        if hasattr(obj, "Visibility"):
            obj.Visibility = True
