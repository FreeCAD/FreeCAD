# ***************************************************************************
# *   (c) 2020 Carlo Pavan                                                  *
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
"""This module provides the object code for Draft Annotation.
"""
## @package annotation
# \ingroup DRAFT
# \brief This module provides the object code for Draft Annotation.

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

    def __init__(self, obj, tp="Annotation"):
        self.Type = tp

    def onDocumentRestored(self, obj):
        """Run when the document that is using this class is restored.

        Check if new properties are present after the object is restored
        in order to migrate older objects.
        """
        if hasattr(obj, "ViewObject") and obj.ViewObject:
            if not hasattr(obj.ViewObject, 'ScaleMultiplier'):
                # annotation properties
                vobj = obj.ViewObject
                _tip = "Dimension size overall multiplier"
                vobj.addProperty("App::PropertyFloat",
                                 "ScaleMultiplier",
                                 "Annotation",
                                 QT_TRANSLATE_NOOP("App::Property", _tip))
                vobj.ScaleMultiplier = 1.00

                _info = "added view property 'ScaleMultiplier'"
                _wrn("v0.19, " + obj.Label + ", " + _tr(_info))

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            if isinstance(state, dict) and ("Type" in state):
                self.Type = state["Type"]
            else:
                self.Type = state

    def execute(self, obj):
        """Do something when recompute object."""
        return

    def onChanged(self, obj, prop):
        """Do something when a property has changed."""
        return
