# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provides functions to create LinearDimension or AngularDinemsion objects.

This includes linear dimensions, radial dimensions, and angular dimensions.
"""
## @package make_dimension
# \ingroup draftmake
# \brief Provides functions to create Linear or AngularDimension objects.

## \addtogroup draftmake
# @{
import math

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import translate
from draftobjects.dimension import (LinearDimension,
                                    AngularDimension)

if App.GuiUp:
    from draftviewproviders.view_dimension \
        import (ViewProviderLinearDimension,
                ViewProviderAngularDimension)


def make_dimension(p1, p2, p3=None, p4=None):
    """Create one of three types of dimension objects.

    In all dimensions the p3 parameter defines a point through which
    the dimension line will go through.

    The current line width and color will be used.

    Linear dimension
    ----------------
    - (p1, p2, p3): a simple linear dimension from p1 to p2

    - (object, i1, i2, p3): creates a linked dimension to the provided
      object (edge), measuring the distance between its vertices
      indexed i1 and i2

    Circular dimension
    ------------------
    - (arc, i1, mode, p3): creates a linked dimension to the given arc
      object, i1 is the index of the arc edge that will be measured;
      mode is either "radius" or "diameter".
    """
    if not App.ActiveDocument:
        _err("No active document. Aborting")
        return None

    new_obj = App.ActiveDocument.addObject("App::FeaturePython",
                                           "Dimension")
    LinearDimension(new_obj)

    if App.GuiUp:
        ViewProviderLinearDimension(new_obj.ViewObject)

    if isinstance(p1, App.Vector) and isinstance(p2, App.Vector):
        # Measure a straight distance between p1 and p2
        new_obj.Start = p1
        new_obj.End = p2
        if not p3:
            p3 = p2.sub(p1)
            p3.multiply(0.5)
            p3 = p1.add(p3)

    elif isinstance(p2, int) and isinstance(p3, int):
        # p1 is an object, and measure the distance between vertices p2 and p3
        # of this object
        linked = []
        idx = (p2, p3)
        linked.append((p1, "Vertex" + str(p2 + 1)))
        linked.append((p1, "Vertex" + str(p3 + 1)))
        new_obj.LinkedGeometry = linked
        new_obj.Support = p1

        # p4, and now p3, is the point through which the dimension line
        # will go through
        p3 = p4
        if not p3:
            # When used from the GUI command, this will never run
            # because p4 will always be assigned to a vector,
            # so p3 will never be `None`.
            # Moreover, `new_obj.Base` doesn't exist, and certainly `Shape`
            # doesn't exist, so if this ever runs it will be an error.
            v1 = new_obj.Base.Shape.Vertexes[idx[0]].Point
            v2 = new_obj.Base.Shape.Vertexes[idx[1]].Point
            p3 = v2.sub(v1)
            p3.multiply(0.5)
            p3 = v1.add(p3)

    elif isinstance(p3, str):
        # If the original p3 is a string, we are measuring a circular arc
        # p2 should be an integer number starting from 0
        linked = []
        linked.append((p1, "Edge" + str(p2 + 1)))

        if p3 == "radius":
            # linked.append((p1, "Center"))
            if App.GuiUp:
                new_obj.ViewObject.Override = "R $dim"
            new_obj.Diameter = False
        elif p3 == "diameter":
            # linked.append((p1, "Diameter"))
            if App.GuiUp:
                new_obj.ViewObject.Override = "Ø $dim"
            new_obj.Diameter = True
        new_obj.LinkedGeometry = linked
        new_obj.Support = p1

        # p4, and now p3, is the point through which the dimension line
        # will go through
        p3 = p4
        if not p3:
            p3 = p1.Shape.Edges[p2].Curve.Center.add(App.Vector(1, 0, 0))

    # This p3 is the point through which the dimension line will pass,
    # but this may not be the original p3, it could have been p4
    # depending on the first three parameter values
    new_obj.Dimline = p3

    if hasattr(App, "DraftWorkingPlane"):
        normal = App.DraftWorkingPlane.axis
    else:
        normal = App.Vector(0, 0, 1)

    if App.GuiUp:
        # invert the normal if we are viewing it from the back
        vnorm = gui_utils.get3DView().getViewDirection()

        if vnorm.getAngle(normal) < math.pi/2:
            normal = normal.negative()

    new_obj.Normal = normal

    if App.GuiUp:
        gui_utils.format_object(new_obj)
        gui_utils.select(new_obj)

    return new_obj


def makeDimension(p1, p2, p3=None, p4=None):
    """Create a dimension. DEPRECATED. Use 'make_dimension'."""
    _wrn(translate("draft","This function is deprecated. Do not use this function directly."))
    _wrn(translate("draft","Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'."))

    return make_dimension(p1, p2, p3, p4)


def make_linear_dimension(p1, p2, dim_line=None):
    """Create a free linear dimension from two main points.

    Parameters
    ----------
    p1: Base::Vector3
        First point of the measurement.

    p2: Base::Vector3
        Second point of the measurement.

    dim_line: Base::Vector3, optional
        It defaults to `None`.
        This is a point through which the extension of the dimension line
        will pass.
        This point controls how close or how far the dimension line is
        positioned from the measured segment that goes from `p1` to `p2`.

        If it is `None`, this point will be calculated from the intermediate
        distance between `p1` and `p2`.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'LinearDimension'`.
        This object does not have a `Shape` attribute, as the text and lines
        are created on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_linear_dimension"
    utils.print_header(_name, "Linear dimension")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    _msg("p1: {}".format(p1))
    try:
        utils.type_check([(p1, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("p2: {}".format(p2))
    try:
        utils.type_check([(p2, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("dim_line: {}".format(dim_line))
    if dim_line:
        try:
            utils.type_check([(dim_line, App.Vector)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a vector."))
            return None
    else:
        diff = p2.sub(p1)
        diff.multiply(0.5)
        dim_line = p1.add(diff)

    new_obj = make_dimension(p1, p2, dim_line)

    return new_obj


def make_linear_dimension_obj(edge_object, i1=1, i2=2, dim_line=None):
    """Create a linear dimension from an object.

    Parameters
    ----------
    edge_object: Part::Feature
        The object which has an edge which will be measured.
        It must have a `Part::TopoShape`, and at least one element
        in `Shape.Vertexes`, to be able to measure a distance.

    i1: int, optional
        It defaults to `1`.
        It is the index of the first vertex in `edge_object` from which
        the measurement will be taken.
        The minimum value should be `1`, which will be interpreted
        as `'Vertex1'`.

        If the value is below `1`, it will be set to `1`.

    i2: int, optional
        It defaults to `2`, which will be converted to `'Vertex2'`.
        It is the index of the second vertex in `edge_object` that determines
        the endpoint of the measurement.

        If it is the same value as `i1`, the resulting measurement will be
        made from the origin `(0, 0, 0)` to the vertex indicated by `i1`.

        If the value is below `1`, it will be set to the last vertex
        in `edge_object`.

        Then to measure the first and last, this could be used
        ::
            make_linear_dimension_obj(edge_object, i1=1, i2=-1)

    dim_line: Base::Vector3
        It defaults to `None`.
        This is a point through which the extension of the dimension line
        will pass.
        This point controls how close or how far the dimension line is
        positioned from the measured segment in `edge_object`.

        If it is `None`, this point will be calculated from the intermediate
        distance between the vertices defined by `i1` and `i2`.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'LinearDimension'`.
        This object does not have a `Shape` attribute, as the text and lines
        are created on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_linear_dimension_obj"
    utils.print_header(_name, "Linear dimension")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if isinstance(edge_object, str):
        edge_object_str = edge_object

    if isinstance(edge_object, (list, tuple)):
        _msg("edge_object: {}".format(edge_object))
        _err(translate("draft","Wrong input: object must not be a list."))
        return None

    found, edge_object = utils.find_object(edge_object, doc)
    if not found:
        _msg("edge_object: {}".format(edge_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("edge_object: {}".format(edge_object.Label))
    if not hasattr(edge_object, "Shape"):
        _err(translate("draft","Wrong input: object doesn't have a 'Shape' to measure."))
        return None
    if (not hasattr(edge_object.Shape, "Vertexes")
            or len(edge_object.Shape.Vertexes) < 1):
        _err(translate("draft","Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring."))
        return None

    _msg("i1: {}".format(i1))
    try:
        utils.type_check([(i1, int)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be an integer."))
        return None

    if i1 < 1:
        i1 = 1
        _wrn(translate("draft","i1: values below 1 are not allowed; will be set to 1."))

    vx1 = edge_object.getSubObject("Vertex" + str(i1))
    if not vx1:
        _err(translate("draft","Wrong input: vertex not in object."))
        return None

    _msg("i2: {}".format(i2))
    try:
        utils.type_check([(i2, int)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    if i2 < 1:
        i2 = len(edge_object.Shape.Vertexes)
        _wrn(translate("draft","i2: values below 1 are not allowed; will be set to the last vertex in the object."))

    vx2 = edge_object.getSubObject("Vertex" + str(i2))
    if not vx2:
        _err(translate("draft","Wrong input: vertex not in object."))
        return None

    _msg("dim_line: {}".format(dim_line))
    if dim_line:
        try:
            utils.type_check([(dim_line, App.Vector)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a vector."))
            return None
    else:
        diff = vx2.Point.sub(vx1.Point)
        diff.multiply(0.5)
        dim_line = vx1.Point.add(diff)

    # TODO: the internal function expects an index starting with 0
    # so we need to decrease the value here.
    # This should be changed in the future in the internal function.
    i1 -= 1
    i2 -= 1

    new_obj = make_dimension(edge_object, i1, i2, dim_line)

    return new_obj


def make_radial_dimension_obj(edge_object, index=1, mode="radius",
                              dim_line=None):
    """Create a radial or diameter dimension from an arc object.

    Parameters
    ----------
    edge_object: Part::Feature
        The object which has a circular edge which will be measured.
        It must have a `Part::TopoShape`, and at least one element
        must be a circular edge in `Shape.Edges` to be able to measure
        its radius.

    index: int, optional
        It defaults to `1`.
        It is the index of the edge in `edge_object` which is going to
        be measured.
        The minimum value should be `1`, which will be interpreted
        as `'Edge1'`. If the value is below `1`, it will be set to `1`.

    mode: str, optional
        It defaults to `'radius'`; the other option is `'diameter'`.
        It determines whether the dimension will be shown as a radius
        or as a diameter.

    dim_line: Base::Vector3, optional
        It defaults to `None`.
        This is a point through which the extension of the dimension line
        will pass. The dimension line will be a radius or diameter
        of the measured arc, extending from the center to the arc itself.

        If it is `None`, this point will be set to one unit to the right
        of the center of the arc, which will create a dimension line that is
        horizontal, that is, parallel to the +X axis.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'LinearDimension'`.
        This object does not have a `Shape` attribute, as the text and lines
        are created on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_radial_dimension_obj"
    utils.print_header(_name, "Radial dimension")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if isinstance(edge_object, str):
        edge_object_str = edge_object

    found, edge_object = utils.find_object(edge_object, doc)
    if not found:
        _msg("edge_object: {}".format(edge_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("edge_object: {}".format(edge_object.Label))
    if not hasattr(edge_object, "Shape"):
        _err(translate("draft","Wrong input: object doesn't have a 'Shape' to measure."))
        return None
    if (not hasattr(edge_object.Shape, "Edges")
            or len(edge_object.Shape.Edges) < 1):
        _err(translate("draft","Wrong input: object doesn't have at least one element in 'Edges' to use for measuring."))
        return None

    _msg("index: {}".format(index))
    try:
        utils.type_check([(index, int)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be an integer."))
        return None

    if index < 1:
        index = 1
        _wrn(translate("draft","index: values below 1 are not allowed; will be set to 1."))

    edge = edge_object.getSubObject("Edge" + str(index))
    if not edge:
        _err(translate("draft","Wrong input: index doesn't correspond to an edge in the object."))
        return None

    if not hasattr(edge, "Curve") or edge.Curve.TypeId != 'Part::GeomCircle':
        _err(translate("draft","Wrong input: index doesn't correspond to a circular edge."))
        return None

    _msg("mode: {}".format(mode))
    try:
        utils.type_check([(mode, str)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a string, 'radius' or 'diameter'."))
        return None

    if mode not in ("radius", "diameter"):
        _err(translate("draft","Wrong input: must be a string, 'radius' or 'diameter'."))
        return None

    _msg("dim_line: {}".format(dim_line))
    if dim_line:
        try:
            utils.type_check([(dim_line, App.Vector)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a vector."))
            return None
    else:
        center = edge_object.Shape.Edges[index - 1].Curve.Center
        dim_line = center + App.Vector(1, 0, 0)

    # TODO: the internal function expects an index starting with 0
    # so we need to decrease the value here.
    # This should be changed in the future in the internal function.
    index -= 1

    new_obj = make_dimension(edge_object, index, mode, dim_line)

    return new_obj


def make_angular_dimension(center=App.Vector(0, 0, 0),
                           angles=None, # If None, set to [0,90]
                           dim_line=App.Vector(10, 10, 0), normal=None):
    """Create an angular dimension from the given center and angles.

    Parameters
    ----------
    center: Base::Vector3, optional
        It defaults to the origin `Vector(0, 0, 0)`.
        Center of the dimension line, which is a circular arc.

    angles: list of two floats, optional
        It defaults to `[0, 90]`.
        It is a list of two angles, given in degrees, that determine
        the aperture of the dimension line, that is, of the circular arc.
        It is drawn counter-clockwise.
        ::
            angles = [0 90]
            angles = [330 60]  # the arc crosses the X axis
            angles = [-30 60]  # same angle

    dim_line: Base::Vector3, optional
        It defaults to `Vector(10, 10, 0)`.
        This is a point through which the extension of the dimension line
        will pass. This defines the radius of the dimension line,
        the circular arc.

    normal: Base::Vector3, optional
        It defaults to `None`, in which case the `normal` is taken
        from the currently active `App.DraftWorkingPlane.axis`.

        If the working plane is not available, then the `normal`
        defaults to +Z or `Vector(0, 0, 1)`.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'AngularDimension'`.
        This object does not have a `Shape` attribute, as the text and lines
        are created on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_angular_dimension"
    utils.print_header(_name, "Angular dimension")

    # Prevent later modification of a default parameter by using a placeholder
    if angles is None:
        angles = [0, 90]

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    _msg("center: {}".format(center))
    try:
        utils.type_check([(center, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("angles: {}".format(angles))
    try:
        utils.type_check([(angles, (tuple, list))], name=_name)

        if len(angles) != 2:
            _err(translate("draft","Wrong input: must be a list with two angles."))
            return None

        ang1, ang2 = angles
        utils.type_check([(ang1, (int, float)),
                          (ang2, (int, float))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a list with two angles."))
        return None

    # If the angle is larger than 360 degrees, make sure
    # it is smaller than 360
    for n in range(len(angles)):
        if angles[n] > 360:
            angles[n] = angles[n] - 360

    _msg("dim_line: {}".format(dim_line))
    try:
        utils.type_check([(dim_line, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("normal: {}".format(normal))
    if normal:
        try:
            utils.type_check([(dim_line, App.Vector)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a vector."))
            return None

    if not normal:
        if hasattr(App, "DraftWorkingPlane"):
            normal = App.DraftWorkingPlane.axis
        else:
            normal = App.Vector(0, 0, 1)

    new_obj = App.ActiveDocument.addObject("App::FeaturePython",
                                           "Dimension")
    AngularDimension(new_obj)

    new_obj.Center = center
    new_obj.FirstAngle = angles[0]
    new_obj.LastAngle = angles[1]
    new_obj.Dimline = dim_line

    if App.GuiUp:
        ViewProviderAngularDimension(new_obj.ViewObject)

        # Invert the normal if we are viewing it from the back.
        # This is determined by the angle between the current
        # 3D view and the provided normal being below 90 degrees
        vnorm = gui_utils.get3DView().getViewDirection()
        if vnorm.getAngle(normal) < math.pi/2:
            normal = normal.negative()

    new_obj.Normal = normal

    if App.GuiUp:
        gui_utils.format_object(new_obj)
        gui_utils.select(new_obj)

    return new_obj


def makeAngularDimension(center, angles, p3, normal=None):
    """Create an angle dimension. DEPRECATED. Use 'make_angular_dimension'."""
    utils.use_instead("make_angular_dimension")

    ang1, ang2 = angles
    angles = [math.degrees(ang2), math.degrees(ang1)]

    return make_angular_dimension(center=center, angles=angles,
                                  dim_line=p3, normal=normal)

## @}
