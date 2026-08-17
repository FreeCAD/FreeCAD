########################################################################
#
#  SheetMetalBendSolid.py
#
#  Copyright 2020 Jaise James <jaisejames at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import math

import Part


def get_point_on_cylinder(zero_vert, point, radius, center, axis, zero_vert_normal):
    """Calculate a point on a cylinder's surface projected from
    a given point.

    Args:
        zero_vert: The reference vertex on the cylinder.
        point: The point of interest to project onto the cylinder.
        radius: The radius of the cylinder.
        center: The center point of the cylinder's base.
        axis: The axis along which the cylinder extends.
        zero_vert_normal: A normal vector to the plane defined
            by `zero_vert`.

    Returns:
        A point on the cylinder surface.

    """
    # Calculate distance from point to the plane defined by `zero_vert`
    # and its normal.
    distance = zero_vert.distanceToPlane(point, zero_vert_normal)
    # Project the point onto the plane to find its base projection.
    projected_point = point.projectToPlane(zero_vert, zero_vert_normal)
    # Calculate the angle for the cylindrical projection based on the
    # distance and radius.
    angle = distance / radius
    # Create a copy of the axis inverted for transformation, avoiding
    # mutation of the original axis.
    inverted_axis = axis * -1
    # Compute the vector from the center to the projected point.
    vector = projected_point - center
    # Rotate the vector around the axis by the calculated angle to find
    # its new position.
    rotated_vector = (vector * math.cos(angle)
                      + inverted_axis * inverted_axis.dot(vector) * (1 - math.cos(angle))
                      + vector.cross(inverted_axis) * math.sin(angle)
                      )
    # Calculate the final point on the cylinder by adding the rotated
    # vector to the center.
    final_point = center + rotated_vector
    return final_point


def wrap_bspline(bspline, radius, zero_vert, center, axis, zero_vert_normal):
    """Wraps a B-Spline curve onto the surface of a cylinder.

    Args:
        bspline: The B-Spline curve to wrap.
        radius: The radius of the cylinder.
        zero_vert: The reference vertex on the cylinder.
        center: The center point of the cylinder's base.
        axis: The axis along which the cylinder extends.
        zero_vert_normal: A normal vector to the plane defined
            by `zero_vert`.

    Returns:
        The wrapped B-Spline curve as a Part.Shape object.

    """
    poles = bspline.getPoles()
    new_poles = []

    for point in poles:
        wrapped_point = get_point_on_cylinder(zero_vert, point, radius, center, axis,
                                              zero_vert_normal)
        new_poles.append(wrapped_point)

    new_bspline = bspline
    new_bspline.buildFromPolesMultsKnots(
        new_poles,
        bspline.getMultiplicities(),
        bspline.getKnots(),
        bspline.isPeriodic(),
        bspline.Degree,
        bspline.getWeights(),
    )

    return new_bspline.toShape()


def wrap_face(face, radius, axis, normal, zero_vert, center, zero_vert_normal):
    """Wraps a face onto the surface of a cylinder by wrapping
    its edges.

    Args:
        face: The face to wrap.
        radius: The radius of the cylinder.
        axis: The axis along which the cylinder extends.
        normal: The normal vector to the face.
        zero_vert: The reference vertex on the cylinder.
        center: The center point of the cylinder's base.
        zero_vert_normal: A normal vector to the plane defined
            by `zero_vert`.

    Returns:
        A list of Part.Shape objects representing the wrapped edges of
        the face.

    """
    edges = []
    for e in face.Edges:
        if isinstance(e.Curve, (Part.Circle, Part.ArcOfCircle)):
            poles = e.discretize(Number=50)
            bspline = Part.BSplineCurve()
            bspline.interpolate(poles)
            edges.append(wrap_bspline(bspline, radius, zero_vert, center, axis, zero_vert_normal))
        elif isinstance(e.Curve, Part.BSplineCurve):
            edges.append(wrap_bspline(e.Curve, radius, zero_vert, center, axis, zero_vert_normal))
        elif isinstance(e.Curve, Part.Line):
            start_point = e.valueAt(e.FirstParameter)
            end_point = e.valueAt(e.LastParameter)
            line_normal = end_point - start_point
            mid_point = start_point + line_normal/2.0
            line_normal.normalize()

            if (line_normal.dot(axis) == 0.0
                    and abs(
                            start_point.distanceToPlane(center, normal)
                            - end_point.distanceToPlane(center, normal)
                            ) == 0.0
            ):
                # Handling for specific geometric conditions.
                point1 = get_point_on_cylinder(zero_vert, start_point, radius, center, axis,
                                               zero_vert_normal)
                point2 = get_point_on_cylinder(zero_vert, mid_point, radius, center, axis,
                                               zero_vert_normal)
                point3 = get_point_on_cylinder(zero_vert, end_point, radius, center, axis,
                                               zero_vert_normal)
                arc = Part.Arc(point1, point2, point3)
                edges.append(arc.toShape())
            elif line_normal.dot(axis) in [1.0, -1.0]:
                # Direct line along the axis.
                point1 = get_point_on_cylinder(zero_vert, start_point, radius, center, axis,
                                               zero_vert_normal)
                point2 = get_point_on_cylinder(zero_vert, end_point, radius, center, axis,
                                               zero_vert_normal)
                edges.append(Part.makeLine(point1, point2))
            elif line_normal.dot(normal) in [1.0, -1.0]:
                # Direct line along the normal.
                point1 = get_point_on_cylinder(zero_vert, start_point, radius, center, axis,
                                               zero_vert_normal)
                point2 = get_point_on_cylinder(zero_vert, end_point, radius, center, axis,
                                               zero_vert_normal)
                edges.append(Part.makeLine(point1, point2))
            else:
                # Generic case for any other line type.
                poles = e.discretize(Number=50)
                bspline = Part.BSplineCurve()
                bspline.interpolate(poles, PeriodicFlag=False)
                edges.append(
                    wrap_bspline(bspline, radius, zero_vert, center, axis, zero_vert_normal))
    return edges


def bend_solid(sel_face, sel_edge, bend_r, thickness, neutral_radius, axis, flipped):
    """Bends a solid along a specified axis and radius.

    Args:
        sel_face: The selected face to bend.
        sel_edge: The selected edge to define the bending start point.
        bend_r: The radius of bending.
        thickness: Thickness of the solid.
        neutral_radius: The neutral radius of bending.
        axis: The axis along which to bend.
        flipped: Boolean indicating if the bend is inverted.

    Returns:
        A Part.Shape object representing the bent solid.

    """
    normal = sel_face.normalAt(0, 0)
    zero_vert = sel_edge.Vertexes[0].Point

    if not flipped:
        center = zero_vert + normal*bend_r
        zero_vert_normal = normal.cross(axis) * -1
        shape = Part.makeCylinder(bend_r, 100, center, axis, 360)
    else:
        center = zero_vert - normal*(bend_r+thickness)
        zero_vert_normal = normal.cross(axis)
        shape = Part.makeCylinder(bend_r + thickness, 100, center, axis, 360)

    face_elt = shape.Face1
    outWire = sel_face.OuterWire
    # Part.show(outWire, "outWire")
    wrap_wire = wrap_face(outWire, neutral_radius, axis, normal, zero_vert, center,
                          zero_vert_normal)
    edge_list = Part.__sortEdges__(wrap_wire)
    wire = Part.Wire(edge_list)
    # Part.show(myWire, "myWire")
    OuterFace = Part.Face(face_elt.Surface, wire)
    # f.check(True)
    OuterFace.validate()
    # Part.show(OuterFace, "OuterFace")
    OuterFace.check(True)  # No output = good.
    # Part.show(OuterFace, "OuterFace")
    for fWire in sel_face.Wires:
        if not outWire.isEqual(fWire):
            wrap_wire = wrap_face(fWire, neutral_radius, axis, normal, zero_vert, center,
                                  zero_vert_normal)
            edge_list = Part.__sortEdges__(wrap_wire)
            wire = Part.Wire(edge_list)
            # Part.show(myWire, "myWire")
            InnerFace = Part.Face(face_elt.Surface, wire)
            # f.check(True)
            InnerFace.validate()
            # Part.show(InnerFace, "InnerFace")
            InnerFace.check(True)  # No output = good
            # Part.show(InnerFace, "InnerFace")
            OuterFace = OuterFace.cut(InnerFace)

    if not flipped:
        bent_solid = OuterFace.makeOffsetShape(thickness, 0.0, fill=True)
    else:
        bent_solid = OuterFace.makeOffsetShape(-thickness, 0.0, fill=True)
    # Part.show(bendsolid, "bendsolid")
    return bent_solid
