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
"""Provides the object code for the Text object."""
## @package text
# \ingroup draftobjects
# \brief Provides the object code for the Text object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from draftutils.messages import _wrn
from draftutils.translate import translate

from draftobjects.draft_annotation import DraftAnnotation


class Text(DraftAnnotation):
    """The Draft Text object."""

    def __init__(self, obj):
        obj.Proxy = self
        self.set_properties(obj)
        self.Type = "Text"

    def set_properties(self, obj):
        """Add properties to the object and set them."""
        properties = obj.PropertiesList

        if "Placement" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The placement of the base point "
                                     "of the first line")
            obj.addProperty("App::PropertyPlacement",
                            "Placement",
                            "Base",
                            _tip)
            obj.Placement = App.Placement()

        if "Text" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The text displayed by this object.\n"
                                     "It is a list of strings; each element "
                                     "in the list will be displayed "
                                     "in its own line.")
            obj.addProperty("App::PropertyStringList",
                            "Text",
                            "Base",
                            _tip)
            obj.Text = []

    def onDocumentRestored(self,obj):
        """Execute code when the document is restored."""
        super().onDocumentRestored(obj)

        # See loads: self.Type is None for new objects.
        if self.Type is not None \
                and hasattr(obj, "ViewObject") \
                and obj.ViewObject:
            self.update_properties_0v21(obj, obj.ViewObject)

        self.Type = "Text"

    def update_properties_0v21(self, obj, vobj):
        """Update view properties."""
        # The DisplayMode is updated automatically but the new values are
        # switched: "2D text" becomes "World" and "3D text" becomes "Screen".
        # It should be the other way around:
        vobj.DisplayMode = "World" if vobj.DisplayMode == "Screen" else "Screen"
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "renamed 'DisplayMode' options to 'World/Screen'"))

    def loads(self,state):
        # Before update_properties_0v21 the self.Type value was stored.
        # We use this to identify older objects that need to be updated.
        self.Type = state


# Alias for compatibility with v0.18 and earlier
DraftText = Text

## @}
