# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the object code for the Fillet object."""
## @package fillet
# \ingroup draftobjects
# \brief Provides the object code for the Fillet object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftobjects.base as base

from draftutils.messages import _msg


class Fillet(base.DraftObject):
    """Proxy class for the Fillet object."""

    def __init__(self, obj):
        super(Fillet, self).__init__(obj, "Fillet")
        self._set_properties(obj)

    def _set_properties(self, obj):
        """Set the properties of objects if they don't exist."""
        if not hasattr(obj, "Start"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "The start point of this line.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Start",
                            "Draft",
                            _tip)
            obj.Start = App.Vector(0, 0, 0)

        if not hasattr(obj, "End"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "The end point of this line.")
            obj.addProperty("App::PropertyVectorDistance",
                            "End",
                            "Draft",
                            _tip)
            obj.End = App.Vector(0, 0, 0)

        if not hasattr(obj, "Length"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "The length of this line.")
            obj.addProperty("App::PropertyLength",
                            "Length",
                            "Draft",
                            _tip)
            obj.Length = 0

        if not hasattr(obj, "FilletRadius"):
            _tip = QT_TRANSLATE_NOOP("App::Property", "Radius to use to fillet the corner.")
            obj.addProperty("App::PropertyLength",
                            "FilletRadius",
                            "Draft",
                            _tip)
            obj.FilletRadius = 0

        # TODO: these two properties should link two straight lines
        # or edges so we can use them to build a fillet from them.
        # if not hasattr(obj, "Edge1"):
        #    _tip = "First line used as reference."
        #    obj.addProperty("App::PropertyLinkGlobal",
        #                    "Edge1",
        #                    "Draft",
        #                    _tip))

        # if not hasattr(obj, "Edge2"):
        #    _tip = "Second line used as reference."
        #    obj.addProperty("App::PropertyLinkGlobal",
        #                    "Edge2",
        #                    "Draft",
        #                    _tip))

        # Read only, change to 0 to make it editable.
        # The Fillet Radius should be made editable
        # when we are able to recalculate the arc of the fillet.
        obj.setEditorMode("Start", 1)
        obj.setEditorMode("End", 1)
        obj.setEditorMode("Length", 1)
        obj.setEditorMode("FilletRadius", 1)
        # obj.setEditorMode("Edge1", 1)
        # obj.setEditorMode("Edge2", 1)

    def execute(self, obj):
        """Run when the object is created or recomputed."""
        if hasattr(obj, "Length"):
            obj.Length = obj.Shape.Length
        if hasattr(obj, "Start"):
            obj.Start = obj.Shape.Vertexes[0].Point
        if hasattr(obj, "End"):
            obj.End = obj.Shape.Vertexes[-1].Point

    def _update_radius(self, obj, radius):
        if (hasattr(obj, "Line1") and hasattr(obj, "Line2")
                and obj.Line1 and obj.Line2):
            _msg("Recalculate the radius with objects.")

        _msg("Update radius currently not implemented: r={}".format(radius))

    def onChanged(self, obj, prop):
        """Change the radius of fillet. NOT IMPLEMENTED.

        This should automatically recalculate the new fillet
        based on the new value of `FilletRadius`.
        """
        if prop in "FilletRadius":
            self._update_radius(obj, obj.FilletRadius)

## @}
