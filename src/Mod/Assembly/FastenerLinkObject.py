# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD <hello@astocad.com>                         *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""The fasteners that the Insert Fastener tool puts in an assembly.

A fastener link is an App::LinkPython to a fastener of the Fasteners workbench. It stands
for one fastener, or, with an element count, for several: its elements are then the
fasteners, each of them a component of the assembly. A fastener sits on a hole through a
Fixed joint between the circular edge of the hole and the origin of the fastener. The
fastener link does not store these edges: the joints do, and are looked up when needed, so
a fastener whose joint was deleted is just unattached.

Nothing here needs the Fasteners workbench: a fastener link links to any object.
"""

import FreeCAD as App
import Part

import UtilsAssembly

__title__ = "Assembly fastener link"
__author__ = "AstoCAD"
__url__ = "https://www.freecad.org"


class FastenerLink:
    """The proxy that marks an App::LinkPython as a fastener link."""

    def __init__(self, link):
        link.Proxy = self

    def allowDuplicateLabel(self, link):
        # Like the assembly links, the fastener links are named after the fastener they use,
        # and several use the same fastener. A unique number would replace the digits that
        # end the name of a fastener: its size.
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None


class ViewProviderFastenerLink:
    """Opens the Insert Fastener task again on a double-click."""

    def __init__(self, vobj):
        vobj.Proxy = self

    def doubleClicked(self, vobj):
        import CommandInsertFastener

        return CommandInsertFastener.editFastenerLink(vobj.Object)

    def dumps(self):
        return None

    def loads(self, state):
        return None


def isFastenerLink(obj):
    return obj is not None and isinstance(getattr(obj, "Proxy", None), FastenerLink)


def makeFastenerLink(assembly, fastener):
    """Add to the assembly a fastener link with one fastener, a link to 'fastener'."""
    link = assembly.newObject("App::LinkPython", "Fasteners")
    FastenerLink(link)
    if App.GuiUp:
        ViewProviderFastenerLink(link.ViewObject)
    link.LinkedObject = fastener
    return link


def components(link):
    """The fasteners of the link, as components of the assembly: the link itself when it
    stands for one fastener, else its elements."""
    if link.ElementCount == 0:
        return [link]
    return list(link.ElementList)


def setSource(link, fastener):
    """Make the link, with all its fasteners, a link to 'fastener'."""
    for obj in [link] + list(link.ElementList):
        if obj.LinkedObject != fastener:
            obj.LinkedObject = fastener


def otherUsers(link):
    """The objects that use the fastener that 'link' links to, apart from this link."""
    source = link.LinkedObject
    if source is None:
        return []
    mine = [link] + list(link.ElementList)
    return [
        obj
        for obj in source.InList
        if obj not in mine and not obj.isDerivedFrom("App::DocumentObjectGroup")
    ]


# ---------------------------------------------------------------------------------------
# Joints


def _references(component):
    """The joint references to 'component', as (joint, property name)."""
    result = []
    for obj in component.InList:
        for prop in ("Reference1", "Reference2"):
            ref = getattr(obj, prop, None)
            if ref and len(ref) == 2 and ref[0] == component:
                result.append((obj, prop))
    return result


def _groundings(component):
    return [obj for obj in component.InList if getattr(obj, "ObjectToGround", None) == component]


def jointLabel(fastener):
    """The label of a joint that holds a fastener on its hole. The fastener comes first: the
    unique number that the document adds to a label in use goes after the name of the
    fastener, and does not replace the digits that end it."""
    return fastener.Label + "_Fixed"


def attachment(component):
    """The joint that holds a fastener on a hole, and its reference to the hole:
    (joint, reference). (None, None) for an unattached fastener."""
    for joint, prop in _references(component):
        if getattr(joint, "JointType", None) != "Fixed":
            continue
        if any(sub for sub in getattr(joint, prop)[1]):
            continue  # not by the origin of the fastener: a joint of the user
        other = getattr(joint, "Reference1" if prop == "Reference2" else "Reference2", None)
        if other and len(other) == 2 and other[0] is not None:
            return joint, other
    return None, None


def holeInfo(ref):
    """What a reference to a circular edge tells about the hole: its center, axis and radius,
    and the direction that points out of the material ('normal'), which is None when the edge
    has no planar face around it. In the coordinates of the container of the component.
    None when the reference is not a circular edge."""
    if not ref or len(ref) != 2 or ref[0] is None or not ref[1]:
        return None
    component, sub = ref[0], ref[1][0]
    if not UtilsAssembly.getElementName(sub).startswith("Edge"):
        return None
    try:
        edge = component.getSubObject(sub)
    except Exception:
        return None
    curve = getattr(edge, "Curve", None)
    if curve is None or curve.TypeId not in ("Part::GeomCircle", "Part::GeomEllipse"):
        return None
    if not hasattr(curve, "Radius"):
        return None
    center = curve.Location

    normal = None
    try:
        owner = sub[: sub.rfind(".") + 1]
        shape = Part.getShape(component, owner, needSubElement=False)
        element = shape.getElement(sub[sub.rfind(".") + 1 :])
        for face in shape.ancestorsOfType(element, Part.Face):
            if face.Surface.TypeId != "Part::GeomPlane":
                continue
            u, v = face.Surface.parameter(center)
            normal = face.normalAt(u, v)
            break
    except Exception:
        normal = None

    return {"center": center, "axis": curve.Axis, "radius": curve.Radius, "normal": normal}


def holePlacement(info, reverse=False):
    """Where a fastener goes on a hole: on its center, the Z axis of the fastener (shank to
    head) pointing out of the material. Without a face to tell the outside, along the axis
    of the hole, on the side of the Z axis of the container."""
    direction = info["normal"]
    if direction is None:
        direction = App.Vector(info["axis"])
        if direction.z < 0:
            direction = -direction
    rotation = App.Rotation(App.Vector(0, 0, 1), direction)
    if reverse:
        rotation = rotation * App.Rotation(App.Vector(1, 0, 0), 180)
    return App.Placement(info["center"], rotation)


def attach(component, ref):
    """Put a fastener on the hole of a reference to a circular edge, with a Fixed joint.
    The assembly is not solved. Returns the joint, or None if 'ref' is not a circular edge."""
    import JointObject

    info = holeInfo(ref)
    if info is None:
        return None
    assembly = UtilsAssembly.getLinkGroup(component) or component
    assembly = assembly.getParentGeoFeatureGroup()
    joint = UtilsAssembly.getJointGroup(assembly).newObject("App::FeaturePython", "Joint")
    JointObject.Joint(joint, 0)  # 0: Fixed
    if App.GuiUp:
        JointObject.ViewProviderJoint(joint.ViewObject)
    linked = component.getLinkedObject()
    joint.Label = jointLabel(linked if linked else component)
    joint.Visibility = False
    # The edge, with the edge itself as vertex: its center. Then the origin of the fastener.
    joint.Reference1 = (ref[0], [ref[1][0], ref[1][0]])
    joint.Reference2 = (component, ["", ""])
    # A Fixed joint is solved in the direction that is the closest to the current one
    component.Placement = holePlacement(info)
    return joint


def reverse(component):
    """Turn a fastener upside down, around its origin. A Fixed joint keeps it so when the
    assembly is solved."""
    flip = App.Placement(App.Vector(), App.Rotation(App.Vector(1, 0, 0), 180))
    component.Placement = component.Placement * flip


# ---------------------------------------------------------------------------------------
# Number of fasteners


def _restructure(link, placements, origins):
    """Give the link one fastener for each placement. origins[i] is the index of the current
    fastener that the new fastener i continues, whose joints it takes over, or None."""
    old = components(link)
    references = []
    groundings = []
    for index, origin in enumerate(origins):
        if origin is None:
            continue
        for joint, prop in _references(old[origin]):
            references.append((joint, prop, list(getattr(joint, prop)[1]), index))
        for joint in _groundings(old[origin]):
            groundings.append((joint, index))

    count = 0 if len(placements) == 1 else len(placements)
    if link.ElementCount != count:
        link.ElementCount = count
    if count:
        if not link.ShowElement:
            link.ShowElement = True  # elements that are objects, so components
        # The elements are placed in the link: keep them in the assembly coordinates
        link.Placement = App.Placement()

    new = components(link)
    for component, placement in zip(new, placements):
        component.Placement = placement
    for joint, prop, subs, index in references:
        ref = getattr(joint, prop)
        if not ref or ref[0] != new[index]:
            setattr(joint, prop, (new[index], subs))
    for joint, index in groundings:
        if joint.ObjectToGround != new[index]:
            joint.ObjectToGround = new[index]
    return new


def addFastener(link, placement):
    """Add a fastener to the link, at 'placement'. Returns its component."""
    old = components(link)
    placements = [component.Placement for component in old] + [placement]
    return _restructure(link, placements, list(range(len(old))) + [None])[-1]


def removeFastener(link, index):
    """Remove the fastener of this index, with the joints that refer to it. The last
    fastener of a link stays: remove the link instead."""
    old = components(link)
    if len(old) < 2:
        return
    doc = link.Document
    names = {joint.Name for joint, _prop in _references(old[index])}
    names.update(joint.Name for joint in _groundings(old[index]))
    for name in names:
        doc.removeObject(name)
    kept = [i for i in range(len(old)) if i != index]
    _restructure(link, [old[i].Placement for i in kept], kept)
