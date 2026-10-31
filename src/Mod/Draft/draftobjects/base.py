# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the object code for the base Draft object."""

## @package base
# \ingroup draftobjects
# \brief Provides the object code for the base Draft object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

ATTACHED_NODES_PROP = "AttachedNodes"
ATTACHED_NODE_INDEXES_PROP = "AttachedNodeIndexes"


def assure_point_attachment_properties(obj):
    """Add generic node attachment properties to a Draft object."""
    if ATTACHED_NODES_PROP not in obj.PropertiesList:
        _tip = QT_TRANSLATE_NOOP(
            "App::Property",
            "Geometry references used to drive individual Draft edit nodes",
        )
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            ATTACHED_NODES_PROP,
            "Node Attachments",
            _tip,
            locked=True,
        )
    if ATTACHED_NODE_INDEXES_PROP not in obj.PropertiesList:
        _tip = QT_TRANSLATE_NOOP(
            "App::Property",
            "Zero-based indexes of Draft edit nodes driven by AttachedNodes",
        )
        obj.addProperty(
            "App::PropertyIntegerList",
            ATTACHED_NODE_INDEXES_PROP,
            "Node Attachments",
            _tip,
            locked=True,
        )


def has_point_attachments(obj):
    """Return True if the object has at least one attached edit node."""
    return (
        hasattr(obj, ATTACHED_NODES_PROP)
        and hasattr(obj, ATTACHED_NODE_INDEXES_PROP)
        and bool(getattr(obj, ATTACHED_NODES_PROP))
        and bool(getattr(obj, ATTACHED_NODE_INDEXES_PROP))
    )


def _subnames_from_link(link):
    if len(link) < 2:
        return ()
    subnames = link[1]
    if isinstance(subnames, str):
        return (subnames,) if subnames else ()
    return tuple(subnames)


def point_from_attachment_link(link):
    """Resolve a LinkSub entry to a point in global coordinates."""
    if not link:
        return None
    obj = link[0]
    if obj is None:
        return None

    subnames = _subnames_from_link(link)
    subname = subnames[0] if subnames else ""
    if subname:
        try:
            subobj = obj.getSubObject(subname)
        except (AttributeError, ValueError, TypeError, ReferenceError):
            subobj = None
        if subobj is not None and getattr(subobj, "ShapeType", None) == "Vertex":
            return subobj.Point

    shape = getattr(obj, "Shape", None)
    if shape is not None and not shape.isNull() and len(shape.Vertexes) == 1:
        return shape.Vertexes[0].Point

    if hasattr(obj, "getGlobalPlacement"):
        return obj.getGlobalPlacement().Base
    if hasattr(obj, "Placement"):
        return App.Vector(obj.Placement.Base)
    return None


def get_point_attachment_map(obj):
    """Return a dict mapping node index to LinkSub entry."""
    if not has_point_attachments(obj):
        return {}
    return dict(zip(obj.AttachedNodeIndexes, get_flat_point_attachment_links(obj)))


def get_flat_point_attachment_links(obj):
    """Return one LinkSub entry per stored attached node."""
    links = []
    for link in obj.AttachedNodes:
        source = link[0]
        subnames = _subnames_from_link(link)
        if subnames:
            for subname in subnames:
                links.append((source, (subname,)))
        else:
            links.append((source, ("",)))
    return links


def apply_point_attachments(obj, points):
    """Return a copy of points with attached nodes updated from their links."""
    if not has_point_attachments(obj):
        return points

    result = list(points)
    inv_placement = obj.getGlobalPlacement().inverse()
    for node_idx, link in get_point_attachment_map(obj).items():
        if node_idx < 0 or node_idx >= len(result):
            continue
        point = point_from_attachment_link(link)
        if point is not None:
            result[node_idx] = inv_placement.multVec(point)
    return result


def apply_point_attachment_to_point(obj):
    """Return the global point for an attached Draft Point, or None."""
    link = get_point_attachment_map(obj).get(0)
    if link:
        return point_from_attachment_link(link)
    return None


def set_point_attachment(obj, node_idx, link):
    """Attach a Draft edit node to a LinkSub entry."""
    assure_point_attachment_properties(obj)
    links_by_index = get_point_attachment_map(obj)
    links_by_index[node_idx] = link
    _set_point_attachment_map(obj, links_by_index)


def remove_point_attachment(obj, node_idx):
    """Remove the attachment for a Draft edit node."""
    if not has_point_attachments(obj):
        return
    links_by_index = get_point_attachment_map(obj)
    links_by_index.pop(node_idx, None)
    _set_point_attachment_map(obj, links_by_index)


def shift_point_attachments(obj, start_idx, delta):
    """Shift stored node indexes after a point insertion or deletion."""
    if not has_point_attachments(obj):
        return
    shifted = {}
    for node_idx, link in get_point_attachment_map(obj).items():
        if delta < 0 and node_idx == start_idx:
            continue
        if node_idx >= start_idx:
            node_idx += delta
        if node_idx >= 0:
            shifted[node_idx] = link
    _set_point_attachment_map(obj, shifted)


def reverse_point_attachments(obj, point_count):
    """Reverse stored node indexes after reversing a point-list object."""
    if not has_point_attachments(obj):
        return
    reversed_links = {}
    for node_idx, link in get_point_attachment_map(obj).items():
        if 0 <= node_idx < point_count:
            reversed_links[point_count - node_idx - 1] = link
    _set_point_attachment_map(obj, reversed_links)


def rotate_point_attachments(obj, point_count, first_idx):
    """Move node attachments with a point-list rotation."""
    if not has_point_attachments(obj) or point_count <= 0:
        return
    rotated_links = {}
    for node_idx, link in get_point_attachment_map(obj).items():
        if 0 <= node_idx < point_count:
            rotated_links[(node_idx - first_idx) % point_count] = link
    _set_point_attachment_map(obj, rotated_links)


def _set_point_attachment_map(obj, links_by_index):
    items = sorted(links_by_index.items())
    obj.AttachedNodeIndexes = [idx for idx, _link in items]
    obj.AttachedNodes = [link for _idx, link in items]


class DraftObject(object):
    """The base class for Draft objects.

    Parameters
    ----------
    obj : a base C++ object
        The base object instantiated during creation,
        which commonly may be of types `Part::Part2DObjectPython`,
        `Part::FeaturePython`, or `App::FeaturePython`.

            >>> obj = App.ActiveDocument.addObject('Part::Part2DObjectPython')
            >>> DraftObject(obj)

        This class instance is stored in the `Proxy` attribute
        of the base object.
        ::
            obj.Proxy = self

    tp : str, optional
        It defaults to `'Unknown'`.
        It indicates the type of this scripted object,
        which will be assigned to the Proxy's `Type` attribute.

        This is useful to distinguish different types of scripted objects
        that may be derived from the same C++ class.

    Attributes
    ----------
    Type : str
        It indicates the type of scripted object.
        Normally `Type = tp`.

        All objects have a `TypeId` attribute, but this attribute
        refers to the C++ class only. Using the `Type` attribute
        allows distinguishing among various types of objects
        derived from the same C++ class.

            >>> print(obj.TypeId, "->", obj.Proxy.Type)
            Part::Part2DObjectPython -> Circle

    This class attribute is accessible through the `Proxy` object:
    `obj.Proxy.Type`.
    """

    def __init__(self, obj, tp="Unknown"):
        # This class is assigned to the Proxy attribute
        if obj:
            obj.Proxy = self
        self.Type = tp

    def onDocumentRestored(self, obj):
        # Object properties are updated when the document is opened.
        self.props_changed_clear()

    def dumps(self):
        """Return a tuple of all serializable objects or None.

        When saving the document this object gets stored
        using Python's `json` module.

        Override this method to define the serializable objects to return.

        By default it returns the `Type` attribute.

        Returns
        -------
        str
            Returns the value of `Type`
        """
        return self.Type

    def loads(self, state):
        """Set some internal properties for all restored objects.

        When a document is restored this method is used to set some properties
        for the objects stored with `json`.

        Override this method to define the properties to change for the
        restored serialized objects.

        By default the `Type` was serialized, so `state` holds this value,
        which is re-assigned to the `Type` attribute.
        ::
            self.Type = state

        Parameters
        ----------
        state : state
            A serialized object.
        """
        if state:
            self.Type = state

    def execute(self, obj):
        """Run this method when the object is created or recomputed.

        Override this method to produce effects when the object
        is newly created, and whenever the document is recomputed.

        By default it does nothing.

        Parameters
        ----------
        obj : the scripted object.
            This commonly may be of types `Part::Part2DObjectPython`,
            `Part::FeaturePython`, or `App::FeaturePython`.
        """
        pass

    def onChanged(self, obj, prop):
        """Run this method when a property is changed.

        Override this method to handle the behavior
        of the object depending on changes that occur to its properties.

        By default it does nothing.

        Parameters
        ----------
        obj : the scripted object.
            This commonly may be of types `Part::Part2DObjectPython`,
            `Part::FeaturePython`, or `App::FeaturePython`.

        prop : str
            Name of the property that was modified.
        """
        pass

    def props_changed_store(self, prop):
        """Store the name of the property that will be changed in the
        self.props_changed list.

        The function should be called at the start of onChanged or onBeforeChange.

        The list is used to detect Placement-only changes. In such cases the
        Shape of an object does not need to be updated. Not updating the Shape
        avoids Uno/Redo issues for the Windows version and is also more efficient.
        """
        if not hasattr(self, "props_changed"):
            self.props_changed = [prop]
        else:
            self.props_changed.append(prop)

    def props_changed_clear(self):
        """Remove the self.props_changed attribute if it exists.

        The function should be called just before execute returns.
        """
        if hasattr(self, "props_changed"):
            delattr(self, "props_changed")

    def props_changed_placement_only(self, obj=None):
        """Return `True` if the self.props_changed list, after removing
        `_LinkTouched`, `Shape`, `Density`, `Volume` and `Mass` items,
        only contains `Placement` items.

        Parameters
        ----------
        obj : the scripted object. Need only be supplied if the Shape of obj
            is, or can be, derived from other objects.

        """
        if not hasattr(self, "props_changed"):
            return False

        # For some objects a dummy Placement property change (new and old
        # Placement are the same) is used in cases where a full recompute is
        # required. This function should then return `False`. The common
        # denominator seems to be a non-empty OutList.
        # https://github.com/FreeCAD/FreeCAD/issues/8771
        # https://forum.freecad.org/viewtopic.php?t=82436
        if obj is not None and obj.OutList:
            return False

        props = set(self.props_changed)
        for prop in ("_LinkTouched", "Shape", "Density", "Volume", "Mass"):
            if prop in props:
                props.remove(prop)
        return props == {"Placement"}


# Alias for compatibility with v0.18 and earlier
_DraftObject = DraftObject

## @}
