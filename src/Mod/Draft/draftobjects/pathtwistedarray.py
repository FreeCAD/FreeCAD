# ***************************************************************************
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
"""Provides the object code for the PathTwistedArray object.

The copies will be placed along a path like a polyline, spline, or bezier
curve, and the copies are twisted around the path by a given rotation
parameter.

This array was developed in order to build a `twisted bridge` object.

See https://forum.freecad.org/viewtopic.php?f=23&t=49617

A `twisted bridge` would consist of three parts:
 1. The ribcage composed of a twisted array generated from a frame
    and a path.
 2. The `tunnel` object produced by lofting or sweeping the internal twisted
    profiles of the ribcage along the path.
 3. The `walkway` object on which the person can stand; it is generated
    from the path, and the internal width of the ribcage profile.

This module builds only the first element, the twisted ribcage.

The tunnel and walkway are built with the `twisted bridge`
object in the Arch Workbench.
"""
## @package pathtwistedarray
# \ingroup draftobjects
# \brief Provides the object code for the TwistedArray object.

import draftgeoutils.geo_arrays as geo
from draftutils.translate import translate
def QT_TRANSLATE_NOOP(ctx,txt): return txt
from draftobjects.draftlink import DraftLink

## \addtogroup draftobjects
# @{


class PathTwistedArray(DraftLink):
    """The PathTwistedArray object.

    This array distributes copies of an object along a path like a polyline,
    spline, or bezier curve, and the copies are twisted around the path
    by a given rotation parameter.
    """

    def __init__(self, obj):
        super(PathTwistedArray, self).__init__(obj, "PathTwistedArray")

    def attach(self, obj):
        """Set up the properties when the object is attached."""
        self.set_properties(obj)
        super(PathTwistedArray, self).attach(obj)

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if hasattr(obj, "PropertiesList"):
            properties = obj.PropertiesList
        else:
            properties = []

        if "Base" not in properties:
            obj.addProperty("App::PropertyLink",
                            "Base",
                            "Objects",
                            QT_TRANSLATE_NOOP("App::Property","The base object that will be duplicated."))
            obj.Base = None

        if "PathObject" not in properties:
            obj.addProperty("App::PropertyLink",
                            "PathObject",
                            "Objects",
                            QT_TRANSLATE_NOOP("App::Property","The object along which the copies will be distributed. It must contain 'Edges'."))
            obj.PathObject = None

        if "Count" not in properties:
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Objects",
                            QT_TRANSLATE_NOOP("App::Property","Number of copies to create."))
            obj.Count = 15

        if "RotationFactor" not in properties:
            obj.addProperty("App::PropertyFloat",
                            "RotationFactor",
                            "Objects",
                            QT_TRANSLATE_NOOP("App::Property","Rotation factor of the twisted array."))
            obj.RotationFactor = 0.25

        if self.use_link and "ExpandArray" not in properties:
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "Objects",
                            QT_TRANSLATE_NOOP("App::Property","Show the individual array elements (only for Link arrays)"))
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape', 'Transient')

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super(PathTwistedArray, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Add properties that don't exist.
        """
        self.set_properties(obj)
        super(PathTwistedArray, self).onDocumentRestored(obj)

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if self.props_changed_placement_only(obj) \
                or not obj.Base \
                or not obj.PathObject:
            self.props_changed_clear()
            return

        # placement of entire PathArray object
        array_placement = obj.Placement

        path = obj.PathObject
        count = obj.Count
        rot_factor = obj.RotationFactor

        copy_placements, _ = geo.get_twisted_placements(path,
                                                        count=count,
                                                        rot_factor=rot_factor)

        self.buildShape(obj, array_placement, copy_placements)
        self.props_changed_clear()
        return (not self.use_link)

## @}
