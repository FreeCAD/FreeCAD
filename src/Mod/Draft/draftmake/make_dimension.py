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
"""Provides functions to crate dimension objects."""
## @package make_dimension
# \ingroup DRAFT
# \brief Provides functions to crate dimension objects.

import math

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import _tr
from draftobjects.dimension import (LinearDimension,
                                    AngularDimension)

if App.GuiUp:
    from draftviewproviders.view_dimension import ViewProviderLinearDimension
    from draftviewproviders.view_dimension import ViewProviderAngularDimension


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
            # because p4 will always be assinged to a vector,
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
    _wrn(_tr("This function is deprecated. "
             "Do not use this function directly."))
    _wrn(_tr("Use one of 'make_linear_dimension', or "
             "'make_linear_dimension_obj'."))

    return make_dimension(p1, p2, p3, p4)


def make_angular_dimension(center=App.Vector(0, 0, 0),
                           angles=[0, 90],
                           dim_line=App.Vector(3, 3, 0), normal=None):
    """Create an angular dimension from the given center and angles.

    Parameters
    ----------
    center: Base::Vector3, optional
        It defaults to the origin `Vector(0, 0, 0)`.
        Center of the dimension line, which is a circular arc.

    angles: list of two floats, optional
        It defaults to `[0, 90]`.
        It is a list of two angles, given in degrees, that determine
        the apperture of the dimension line, that is, of the circular arc.
        It is drawn counter-clockwise.
        ::
            angles = [0 90]
            angles = [330 60]  # the arc crosses the X axis
            angles = [-30 60]  # same angle

    dim_line: Base::Vector3, optional
        It defaults to `Vector(3, 3, 0)`.
        Point through which the dimension line will pass.
        This defines the radius of the dimension line, the circular arc.

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

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(_tr("No active document. Aborting."))
        return None

    _msg("center: {}".format(center))
    try:
        utils.type_check([(center, App.Vector)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a vector."))
        return None

    _msg("angles: {}".format(angles))
    try:
        utils.type_check([(angles, (tuple, list))], name=_name)

        if len(angles) != 2:
            _err(_tr("Wrong input: must be a list with two angles."))
            return None

        ang1, ang2 = angles
        utils.type_check([(ang1, (int, float)),
                          (ang2, (int, float))], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a list with two angles."))
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
        _err(_tr("Wrong input: must be a vector."))
        return None

    _msg("normal: {}".format(normal))
    if normal:
        try:
            utils.type_check([(dim_line, App.Vector)], name=_name)
        except TypeError:
            _err(_tr("Wrong input: must be a vector."))
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
