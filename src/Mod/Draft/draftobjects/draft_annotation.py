# ***************************************************************************
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provide the basic object code for all Draft annotation objects.

This is used by many objects that show dimensions and text created on screen
through Coin (pivy).
- DimensionBase
- LinearDimension
- AngularDimension
- Label
- Text
"""
## @package draft_annotation
# \ingroup DRAFT
# \brief Provide the basic object code for all Draft annotation objects.

from PySide.QtCore import QT_TRANSLATE_NOOP

from draftutils.messages import _wrn
from draftutils.translate import _tr


class DraftAnnotation(object):
    """The Draft Annotation Base object.

    This class is not used directly, but inherited by all Draft annotation
    objects.

    LinearDimension through DimensionBase
    AngularDimension through DimensionBase
    Label
    Text
    """

    def __init__(self, obj, typ="Annotation"):
        self.Type = typ
        obj.Proxy = self

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Check if new properties are present after the object is restored
        in order to migrate older objects.
        """
        self.add_missing_properties_0v19(obj)

    def add_missing_properties_0v19(self, obj):
        """Provide missing annotation properties, if they don't exist."""
        if (hasattr(obj, "ViewObject") and obj.ViewObject
                and not hasattr(obj.ViewObject, 'ScaleMultiplier')):
            vobj = obj.ViewObject
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Dimension size overall multiplier")
            vobj.addProperty("App::PropertyFloat",
                             "ScaleMultiplier",
                             "Annotation",
                             _tip)
            vobj.ScaleMultiplier = 1.00

            _info = "added view property 'ScaleMultiplier'"
            _wrn("v0.19, " + obj.Label + ", " + _tr(_info))

    def __getstate__(self):
        """Return a tuple of objects to save or None.

        Save the Type.
        """
        return self.Type

    def __setstate__(self, state):
        """Set the internal properties from the restored state.

        Restore the Type of the object.
        """
        if state:
            if isinstance(state, dict) and ("Type" in state):
                self.Type = state["Type"]
            else:
                self.Type = state

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Does nothing.
        """
        return

    def onChanged(self, obj, prop):
        """Execute when a property is changed.

        Does nothing.
        """
        return
