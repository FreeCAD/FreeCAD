# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2018 Benjamin Alterauge (ageeye)                        *
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides the object code for the PointArray object.
"""
## @package pointarray
# \ingroup draftobjects
# \brief Provides the object code for the PointArray object.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import draftutils.utils as utils

from draftutils.messages import _wrn, _err
from draftutils.translate import translate
from draftobjects.draftlink import DraftLink

## \addtogroup draftobjects
# @{


class PointArray(DraftLink):
    """The Draft Point Array object."""

    def __init__(self, obj):
        super().__init__(obj, "PointArray")

    def attach(self, obj):
        """Set up the properties when the object is attached."""
        self.set_properties(obj)
        super().attach(obj)

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super().linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Base" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Base object that will be duplicated")
            obj.addProperty("App::PropertyLink",
                            "Base",
                            "Objects",
                            _tip,
                            locked=True)
            obj.Base = None

        if "PointObject" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Object containing points used to distribute the copies.")
            obj.addProperty("App::PropertyLink",
                            "PointObject",
                            "Objects",
                            _tip,
                            locked=True)
            obj.PointObject = None

        if "Fuse" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Specifies if the copies "
                                     "should be fused together "
                                     "if they touch each other (slower)")
            obj.addProperty("App::PropertyBool",
                            "Fuse",
                            "Objects",
                            _tip,
                            locked=True)
            obj.Fuse = False

        if "Count" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Number of copies in the array.\nThis property is read-only, as the number depends on the points in 'Point Object'.")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Objects",
                            _tip,
                            locked=True)
            obj.Count = 0
            obj.setEditorMode("Count", 1)  # Read only

        if "ExtraPlacement" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Additional placement, shift and rotation, that will be applied to each copy")
            obj.addProperty("App::PropertyPlacement",
                            "ExtraPlacement",
                            "Objects",
                            _tip,
                            locked=True)
            obj.ExtraPlacement = App.Placement()

        if self.use_link and "ExpandArray" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Show the individual array elements (only for Link arrays)")
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "Objects",
                            _tip,
                            locked=True)
            obj.setPropertyStatus('Shape', 'Transient')

        if not self.use_link:
            if "PlacementList" not in properties:
                _tip = QT_TRANSLATE_NOOP("App::Property",
                                         "The placement for each array element")
                obj.addProperty("App::PropertyPlacementList",
                                "PlacementList",
                                "Objects",
                                _tip,
                                locked=True)
                obj.PlacementList = []

    def execute(self, obj):
        """Run when the object is created or recomputed."""
        if self.props_changed_placement_only(obj) \
                or not obj.Base \
                or not obj.PointObject:
            self.props_changed_clear()
            return

        pt_list = get_point_list(obj.PointObject)
        obj.Count = len(pt_list)
        pls = build_placements(obj.Base, pt_list, obj.ExtraPlacement)

        self.buildShape(obj, obj.Placement, pls)
        self.props_changed_clear()
        return (not self.use_link)

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        # Fuse property was added in v1.0 and PlacementList property was added
        # for non-link arrays in v1.1, obj should be OK if both are present:
        if hasattr(obj, "Fuse") and hasattr(obj, "PlacementList"):
            return

        if not hasattr(obj, "ExtraPlacement"):
            _wrn("v0.19, " + obj.Label + ", " + translate("draft", "added 'ExtraPlacement' property"))
        if hasattr(obj, "PointList"):
            _wrn("v0.19, " + obj.Label + ", " + translate("draft", "migrated 'PointList' property to 'PointObject'"))
        if not hasattr(obj, "Fuse"):
            _wrn("v1.0, " + obj.Label + ", " + translate("draft", "added 'Fuse' property"))
        if not hasattr(obj, "PlacementList"):
            _wrn("v1.1, " + obj.Label + ", " + translate("draft", "added hidden property 'PlacementList'"))

        self.set_properties(obj)
        if hasattr(obj, "PointList"):
            obj.PointObject = obj.PointList
            obj.removeProperty("PointList")
        self.execute(obj) # Required to update PlacementList.

def remove_equal_vecs (vec_list):
    """Remove equal vectors from a list.

    Parameters
    ----------
    vec_list: list of App.Vectors

    Returns
    -------
    list of App.Vectors
    """
    res_list = []
    for vec in vec_list:
        for res in res_list:
            if DraftVecUtils.equals(vec, res):
                break
        else:
            res_list.append(vec)
    return res_list

def get_point_list(point_object):
    """Extract a list of points from a point object.

    Parameters
    ----------
    point_object: Part::Feature, Sketcher::SketchObject, Mesh::Feature
                  or Points::FeatureCustom
        The object must have vertices and/or points.

    Returns
    -------
    list of App.Vectors
    """
    if hasattr(point_object, "Shape") and hasattr(point_object.Shape, "Vertexes"):
        pt_list = [v.Point for v in point_object.Shape.Vertexes]
        # For compatibility with previous versions: add all points from sketch (including construction geometry):
        if hasattr(point_object, 'Geometry'):
            place = point_object.Placement
            for geo in point_object.Geometry:
                if geo.TypeId == "Part::GeomPoint":
                    pt_list.append(place.multVec(App.Vector(geo.X, geo.Y, geo.Z)))
    elif hasattr(point_object, "Mesh"):
        pt_list = [p.Vector for p in point_object.Mesh.Points]
    elif hasattr(point_object, "Points"):
        pt_list = point_object.Points.Points
    else:
        return []

    return remove_equal_vecs(pt_list)

def build_placements(base_object, pt_list=None, placement=App.Placement()):
    """Build a placements from the base object and list of points.

    Returns
    -------
    list of App.Placements
    """
    if not pt_list:
        _err(translate("Draft",
                       "Point object doesn't have a discrete point, "
                       "it cannot be used for an array."))
        return []

    pls = list()

    for point in pt_list:
        new_pla = base_object.Placement.copy()
        original_rotation = new_pla.Rotation

        # Reset the position of the copy, and combine the original rotation
        # with the provided rotation. Two rotations (quaternions)
        # are combined by multiplying them.
        new_pla.Base = placement.Base
        new_pla.Rotation = original_rotation * placement.Rotation
        new_pla.translate(point)

        pls.append(new_pla)

    return pls


# Alias for compatibility with v0.18 and earlier
_PointArray = PointArray

## @}
