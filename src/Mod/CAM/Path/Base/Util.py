# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

"""
The purpose of this file is to collect some handy functions. The reason they
are not in Path.Base.Utils (and there is this confusing naming going on) is that
PathUtils depends on PathJob. Which makes it impossible to use the functions
and classes defined there in PathJob.

So if you add to this file and think about importing anything from PathScripts
other than Path.Log, then it probably doesn't belong here.
"""

import FreeCAD
import Part
import Path
import Path.Geom

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _getProperty(obj, prop):
    o = obj
    attr = obj
    name = None
    for name in prop.split("."):
        o = attr
        if not hasattr(o, name):
            break
        attr = getattr(o, name)

    if o == attr:
        Path.Log.debug(translate("PathGui", "%s has no property %s (%s)") % (obj.Label, prop, name))
        return (None, None, None)

    # Path.Log.debug("found property %s of %s (%s: %s)" % (prop, obj.Label, name, attr))
    return (o, attr, name)


def getProperty(obj, prop):
    """getProperty(obj, prop) ... answer obj's property defined by its canonical name."""
    o, attr, name = _getProperty(obj, prop)
    return attr


def getPropertyValueString(obj, prop):
    """getPropertyValueString(obj, prop) ... answer a string representation of an object's property's value."""
    attr = getProperty(obj, prop)
    if hasattr(attr, "UserString"):
        return attr.UserString
    return str(attr)


def setProperty(obj, prop, value):
    """setProperty(obj, prop, value) ... set the property value of obj's property defined by its canonical name."""
    o, attr, name = _getProperty(obj, prop)
    if attr is not None and isinstance(value, str):
        if isinstance(attr, bool):
            value = value.lower() in ["true", "1", "yes", "ok"]
        elif isinstance(attr, int):
            value = int(value, 0)
    elif isinstance(attr, int) and isinstance(value, float) and value.is_integer():
        value = int(value)
    if o and name:
        setattr(o, name, value)


# NotValidBaseTypeIds = ['Sketcher::SketchObject']
NotValidBaseTypeIds = []


def isValidBaseObject(obj):
    """isValidBaseObject(obj) ... returns true if the object can be used as a base for a job."""
    if hasattr(obj, "getParentGeoFeatureGroup") and obj.getParentGeoFeatureGroup():
        # Can't link to anything inside a geo feature group anymore
        Path.Log.debug("%s is inside a geo feature group" % obj.Label)
        return False
    if hasattr(obj, "BitBody") and hasattr(obj, "ShapeName"):
        # ToolBit's are not valid base objects
        return False
    if hasattr(obj, "ToolBitID"):
        return False
    if any(hasattr(ob, "ToolBitID") for ob in getattr(obj, "InListRecursive", [])):
        return False
    if obj.TypeId in NotValidBaseTypeIds:
        Path.Log.debug("%s is blacklisted (%s)" % (obj.Label, obj.TypeId))
        return False
    if hasattr(obj, "Sheets") or hasattr(obj, "TagText"):  # Arch.Panels and Arch.PanelCut
        Path.Log.debug("%s is not an Arch.Panel" % (obj.Label))
        return False
    import Part

    return not Part.getShape(obj).isNull()


def isSolid(obj):
    """isSolid(obj) ... return True if the object is a valid solid."""
    import Part

    shape = Part.getShape(obj)
    return not shape.isNull() and shape.Volume and shape.isClosed()


def opProperty(op, prop, default=None):
    """opProperty(op, prop) ... return the value of property prop of the underlying operation (or None if prop does not exist)"""
    if hasattr(op, prop):
        return getattr(op, prop)
    if hasattr(op, "Base"):
        return opProperty(op.Base, prop, default)
    return default


def toolControllerForOp(op):
    """toolControllerForOp(op) ... return the tool controller used by the op.
    If the op doesn't have its own tool controller but has a Base object, return its tool controller.
    Otherwise return None."""
    return opProperty(op, "ToolController")


def coolantModeForOp(op):
    """coolantModeForOp(op) ... return the coolant mode used by the op.
    If the op doesn't have its own coolant mode but has a Base object, return its coolant mode.
    Otherwise return "None"."""
    return opProperty(op, "CoolantMode", "None")


def activeForOp(op):
    """activeForOp(op) ... return the active property used by the op.
    If the op doesn't have its own active property but has a Base object, return its active property.
    Otherwise return True."""
    return opProperty(op, "Active", True)


def getPublicObject(obj):
    """getPublicObject(obj) ... returns the object which should be used to reference a feature of the given object."""
    if hasattr(obj, "getParentGeoFeatureGroup"):
        body = obj.getParentGeoFeatureGroup()
        if body:
            return getPublicObject(body)
    return obj


def clearExpressionEngine(obj):
    """clearExpressionEngine(obj) ... removes all expressions from obj.

    There is currently a bug that invalidates the DAG if an object
    is deleted that still has one or more expressions attached to it.
    Use this function to remove all expressions before deletion."""
    if hasattr(obj, "ExpressionEngine"):
        for attr, expr in obj.ExpressionEngine:
            obj.setExpression(attr, None)


def workplaneForOp(op):
    """workplaneForOp(op) ... returns the effective Workplane of op as a Placement.

    This is the single accessor for an operation's frame. Nothing outside this
    module should read ``op.Workplane`` directly: the property has changed
    shape twice already, and beyond indexed machining the tool axis stops being
    a property of the operation at all. One accessor can absorb those changes;
    scattered reads cannot.

    ``Workplane`` is a link to a named work plane held by the Job. No link
    means the Job's own XY, which is ordinary Z-up milling. A document
    restored but not yet migrated may still carry the older vector or
    placement forms; those are read as they were."""
    wp = getattr(op, "Workplane", None)
    if wp is None:
        return FreeCAD.Placement()
    if isinstance(wp, FreeCAD.Vector):
        return placementFromToolAxis(wp)
    if isinstance(wp, FreeCAD.Placement):
        return FreeCAD.Placement(wp)
    if hasattr(wp, "Placement"):
        return FreeCAD.Placement(wp.Placement)
    return FreeCAD.Placement()


def toolAxisForOp(op):
    """toolAxisForOp(op) ... returns the effective tool axis of op as a unit Vector.

    The direction the tool points, from the part toward the tool - the
    Workplane's local +Z. Constant for the whole operation, which is true for
    indexed machining and is the assumption that simultaneous motion removes."""
    return workplaneForOp(op).Rotation.multVec(FreeCAD.Vector(0, 0, 1))


def placementFromToolAxis(axis, origin=None):
    """placementFromToolAxis(axis, origin=None) ... builds a Workplane placement
    whose local +Z is axis.

    The rotation about that axis is not determined by the axis alone. The
    convention here is FreeCAD.Rotation(Vector(0,0,1), axis), the minimal
    rotation carrying global +Z onto axis, which is what the vector form of the
    property implied and so is what document migration must reproduce."""
    if origin is None:
        origin = FreeCAD.Vector(0, 0, 0)
    direction = FreeCAD.Vector(axis)
    if direction.Length < 1e-9:
        direction = FreeCAD.Vector(0, 0, 1)
    else:
        direction.normalize()
    return FreeCAD.Placement(origin, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), direction))


def sameWorkplane(a, b, tol=1e-6):
    """sameWorkplane(a, b, tol=1e-6) ... True if two Workplane placements name
    the same frame for the purpose of reusing generated toolpath geometry.

    Compares tool axes only, and that is deliberate even though a work plane's
    origin is consumed. An operation generates in its plane's frame, but its
    path is *stored* relative to the Job's zero in the rotated frame, which is
    the same frame for every operation sharing a tool axis. Two operations on
    parallel faces at different depths therefore share a stored frame, and
    rest machining can reuse cleared area between them. The one thing that has
    to move to make that work is the querying operation's own bounding box,
    which getClearedAreas() shifts by the plane origin's position in the
    rotated frame. This is a named predicate so that if the storage convention
    ever changes, the callers that depend on it change with it."""
    axis_a = FreeCAD.Placement(a).Rotation.multVec(FreeCAD.Vector(0, 0, 1))
    axis_b = FreeCAD.Placement(b).Rotation.multVec(FreeCAD.Vector(0, 0, 1))
    return axis_a.isEqual(axis_b, tol)


def liesInPlanePerpendicularTo(sub, axis, tol=1e-6):
    """liesInPlanePerpendicularTo(sub, axis) ... True if every point of sub is
    at the same distance along axis, so the feature names a single depth."""
    if "Face" == sub.ShapeType:
        if not isinstance(sub.Surface, Part.Plane):
            return False
        normal = FreeCAD.Vector(sub.Surface.Axis)
        if normal.Length < tol:
            return False
        normal.normalize()
        return Path.Geom.isRoughly(abs(normal.dot(axis)), 1.0)

    if "Edge" == sub.ShapeType:
        try:
            points = sub.discretize(Number=8)
        except Exception:
            points = [v.Point for v in sub.Vertexes]
        if not points:
            return False
        levels = [axis.dot(p) for p in points]
        return Path.Geom.isRoughly(max(levels) - min(levels), 0.0)

    return False


def depthOfFeature(sub, axis, origin=None):
    """depthOfFeature(sub, axis, origin=None) ... the depth named by a selected
    feature, measured along axis from origin, or None if it does not name one.

    A depth is a coordinate in the frame an operation generates in, and that
    frame's up direction is the tool axis. So the useful selection is a feature
    lying in a plane perpendicular to the tool axis, and the depth it names is
    its projection onto that axis.

    For the default work plane the tool axis is +Z and this is the Z level it
    has always been. That path is kept verbatim rather than expressed through
    the general one, because Path.Geom.isHorizontal() accepts shapes - a
    sphere, a surface of revolution - for which a bounding box maximum and a
    maximum over vertices are not the same number, and three-axis behaviour
    must not drift."""
    base = axis.dot(origin) if origin is not None else 0.0

    if "Vertex" == sub.ShapeType:
        # Identical to sub.Z when the tool axis is +Z and the origin is zero.
        return axis.dot(sub.Point) - base

    if Path.Geom.isRoughly(axis.z, 1.0):
        if Path.Geom.isHorizontal(sub):
            if "Edge" == sub.ShapeType:
                return sub.Vertexes[0].Z - base
            if "Face" == sub.ShapeType:
                return sub.BoundBox.ZMax - base
        return None

    if not liesInPlanePerpendicularTo(sub, axis):
        return None
    return max(axis.dot(v.Point) for v in sub.Vertexes) - base


def isPlanarFace(shape):
    """isPlanarFace(shape) ... True if shape is a face lying in a plane."""
    return getattr(shape, "ShapeType", None) == "Face" and isinstance(shape.Surface, Part.Plane)


def jobHasRotaryMachine(job):
    """jobHasRotaryMachine(job) ... True if job's machine has rotary axes.

    Work planes are available on every Job; this decides what a plane may be.
    Without rotary axes a plane must be parallel to the table: a datum for
    depths and a turned X, which any three-axis machine can cut. A tilted
    plane needs rotary axes to point the tool along it."""
    if job is None or not hasattr(job, "Proxy"):
        return False
    try:
        machine = job.Proxy.getMachine()
    except Exception:
        return False
    return bool(machine is not None and getattr(machine, "has_rotary_axes", False))
