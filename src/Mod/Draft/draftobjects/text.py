# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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

"""This module provides the object code for Draft Text.
"""
## @package text
# \ingroup DRAFT
# \brief This module provides the object code for Draft Text.

import FreeCAD as App
import math
from PySide.QtCore import QT_TRANSLATE_NOOP
import DraftGeomUtils
import draftutils.gui_utils as gui_utils
import draftutils.utils as utils
from draftobjects.draft_annotation import DraftAnnotation

if App.GuiUp:
    from draftviewproviders.view_text import ViewProviderText



def make_text(stringslist, point=App.Vector(0,0,0), screen=False):
    """makeText(strings, point, screen)
    
    Creates a Text object containing the given strings. 
    The current color and text height and font
    specified in preferences are used.
    
    Parameters
    ----------
    stringlist : List
                 Given list of strings, one string by line (strings can also
                 be one single string)

    point : App::Vector
            insert point of the text

    screen : Bool
             If screen is True, the text always faces the view direction.

    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    utils.type_check([(point, App.Vector)], "makeText")
    if not isinstance(stringslist,list): stringslist = [stringslist]

    obj = App.ActiveDocument.addObject("App::FeaturePython","Text")
    Text(obj)
    obj.Text = stringslist
    obj.Placement.Base = point

    if App.GuiUp:
        ViewProviderText(obj.ViewObject)
        if screen:
            obj.ViewObject.DisplayMode = "3D text"
        h = utils.get_param("textheight",0.20)
        if screen:
            h = h*10
        obj.ViewObject.FontSize = h
        obj.ViewObject.FontName = utils.get_param("textfont","")
        obj.ViewObject.LineSpacing = 1
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


class Text(DraftAnnotation):
    """The Draft Text object"""

    def __init__(self, obj):

        super(Text, self).__init__(obj, "Text")

        self.init_properties(obj)

        obj.Proxy = self
    

    def init_properties(self, obj):
        """Add Text specific properties to the object and set them"""

        obj.addProperty("App::PropertyPlacement",
                        "Placement",
                        "Base",
                        QT_TRANSLATE_NOOP("App::Property",
                                          "The placement of this object"))

        obj.addProperty("App::PropertyStringList",
                        "Text",
                        "Base",
                        QT_TRANSLATE_NOOP("App::Property",
                                          "The text displayed by this object"))

    def onDocumentRestored(self, obj):
        super(Text, self).onDocumentRestored(obj)

    def execute(self,obj):
        '''Do something when recompute object'''

        return


    def onChanged(self,obj,prop):
        '''Do something when a property has changed'''
                
        return
