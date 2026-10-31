# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared exact helical and annular geometry primitives."""

import math

import FreeCAD
import Part

from .geometry_core import (
    _CircularProfile,
)


def _make_helical_annular_solid(
    profile,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build an annular solid between a horizontal top and helical bottom."""
    return _make_helical_band_solid(
        profile,
        top_elevation,
        top_elevation,
        bottom_front,
        bottom_rear,
    )


def _circular_profile_between(profile, start_angle, end_angle):
    return _CircularProfile(
        profile.center,
        profile.inner_radius,
        profile.outer_radius,
        start_angle,
        end_angle - start_angle,
    )


def _annular_sector_face(profile, elevation):
    """Return an exact horizontal annular-sector face."""

    start_angle = profile.start_angle
    middle_angle = start_angle + profile.sweep / 2.0
    end_angle = start_angle + profile.sweep

    def point(radius, angle):
        return FreeCAD.Vector(
            profile.center[0] + radius * math.cos(angle),
            profile.center[1] + radius * math.sin(angle),
            elevation,
        )

    inner_start = point(profile.inner_radius, start_angle)
    outer_start = point(profile.outer_radius, start_angle)
    inner_end = point(profile.inner_radius, end_angle)
    outer_end = point(profile.outer_radius, end_angle)
    outer_arc = Part.Arc(
        outer_start,
        point(profile.outer_radius, middle_angle),
        outer_end,
    ).toShape()
    inner_arc = Part.Arc(
        inner_end,
        point(profile.inner_radius, middle_angle),
        inner_start,
    ).toShape()
    return Part.Face(
        Part.Wire(
            (
                Part.makeLine(inner_start, outer_start),
                outer_arc,
                Part.makeLine(outer_end, inner_end),
                inner_arc,
            )
        )
    )


def _helical_profile_edge(profile, radius, start_elevation, end_elevation):
    """Return an arc or exact helix spanning one circular profile."""

    height = float(end_elevation) - float(start_elevation)
    if abs(height) < 1e-7:
        start_angle = profile.start_angle
        middle_angle = start_angle + profile.sweep / 2.0
        end_angle = start_angle + profile.sweep

        def point(angle):
            return FreeCAD.Vector(
                profile.center[0] + radius * math.cos(angle),
                profile.center[1] + radius * math.sin(angle),
                start_elevation,
            )

        return Part.Arc(point(start_angle), point(middle_angle), point(end_angle)).toShape()
    cylinder = Part.Cylinder()
    cylinder.Radius = radius
    cylinder.Center = FreeCAD.Vector(profile.center[0], profile.center[1], 0.0)
    cylinder.Axis = FreeCAD.Vector(0.0, 0.0, 1.0)
    cylinder.rotate(
        FreeCAD.Placement(
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Rotation(
                FreeCAD.Vector(0.0, 0.0, 1.0),
                math.degrees(profile.start_angle),
            ),
        )
    )
    return Part.Geom2d.Line2dSegment(
        FreeCAD.Base.Vector2d(0.0, start_elevation),
        FreeCAD.Base.Vector2d(profile.sweep, end_elevation),
    ).toShape(cylinder)


def _make_helical_band_solid(
    profile,
    top_front,
    top_rear,
    bottom_front,
    bottom_rear,
):
    """Create a curved board bounded by four exact coaxial helices."""

    maximum_sweep = math.pi / 2.0
    if abs(profile.sweep) > maximum_sweep + 1e-3:
        segment_count = int(math.ceil(abs(profile.sweep) / maximum_sweep))
        segments = []
        for index in range(segment_count):
            front_fraction = index / segment_count
            rear_fraction = (index + 1) / segment_count
            segment_profile = _CircularProfile(
                profile.center,
                profile.inner_radius,
                profile.outer_radius,
                profile.start_angle + profile.sweep * front_fraction,
                profile.sweep / segment_count,
            )
            segment = _make_helical_band_solid(
                segment_profile,
                top_front + (top_rear - top_front) * front_fraction,
                top_front + (top_rear - top_front) * rear_fraction,
                bottom_front + (bottom_rear - bottom_front) * front_fraction,
                bottom_front + (bottom_rear - bottom_front) * rear_fraction,
            )
            if segment is None:
                return None
            segments.append(segment)
        result = segments[0]
        for segment in segments[1:]:
            try:
                result = result.fuse(segment).removeSplitter()
            except (Part.OCCError, ValueError):
                return None
        if result.isValid() and len(result.Solids) == 1:
            return result
        return None

    circle_center = profile.center
    profile = _CircularProfile(
        (0.0, 0.0),
        profile.inner_radius,
        profile.outer_radius,
        profile.start_angle,
        profile.sweep,
    )

    top_inner = _helical_profile_edge(profile, profile.inner_radius, top_front, top_rear)
    top_outer = _helical_profile_edge(profile, profile.outer_radius, top_front, top_rear)
    bottom_inner = _helical_profile_edge(profile, profile.inner_radius, bottom_front, bottom_rear)
    bottom_outer = _helical_profile_edge(profile, profile.outer_radius, bottom_front, bottom_rear)
    if any(edge is None for edge in (top_inner, top_outer, bottom_inner, bottom_outer)):
        return None

    def ruled_face(first, second):
        surface = Part.makeRuledSurface(first, second)
        return surface.Faces[0] if surface.Faces else None

    def cylindrical_face(radius):
        cylinder = Part.Cylinder()
        cylinder.Radius = radius
        cylinder.Center = FreeCAD.Vector(0.0, 0.0, 0.0)
        cylinder.Axis = FreeCAD.Vector(0.0, 0.0, 1.0)
        cylinder.rotate(
            FreeCAD.Placement(
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Rotation(
                    FreeCAD.Vector(0.0, 0.0, 1.0),
                    math.degrees(profile.start_angle),
                ),
            )
        )
        vector = FreeCAD.Base.Vector2d
        start_angle = 0.0
        end_angle = profile.sweep
        parameter_points = (
            vector(start_angle, bottom_front),
            vector(end_angle, bottom_rear),
            vector(end_angle, top_rear),
            vector(start_angle, top_front),
        )
        edges = [
            Part.Geom2d.Line2dSegment(first, second).toShape(cylinder)
            for first, second in zip(
                parameter_points,
                (*parameter_points[1:], parameter_points[0]),
            )
        ]
        return Part.Face(cylinder, Part.Wire(edges))

    top_face = (
        _annular_sector_face(profile, top_front)
        if abs(top_rear - top_front) < 1e-7
        else ruled_face(top_inner, top_outer)
    )
    bottom_face = (
        _annular_sector_face(profile, bottom_front)
        if abs(bottom_rear - bottom_front) < 1e-7
        else ruled_face(bottom_inner, bottom_outer)
    )
    inner_face = cylindrical_face(profile.inner_radius)
    outer_face = cylindrical_face(profile.outer_radius)
    if any(face is None for face in (top_face, bottom_face, inner_face, outer_face)):
        return None

    start_points = (
        bottom_inner.Vertexes[0].Point,
        bottom_outer.Vertexes[0].Point,
        top_outer.Vertexes[0].Point,
        top_inner.Vertexes[0].Point,
    )
    end_points = (
        bottom_inner.Vertexes[-1].Point,
        bottom_outer.Vertexes[-1].Point,
        top_outer.Vertexes[-1].Point,
        top_inner.Vertexes[-1].Point,
    )
    start_face = Part.Face(Part.makePolygon((*start_points, start_points[0])))
    end_face = Part.Face(Part.makePolygon((*end_points, end_points[0])))
    try:
        sewn = Part.makeCompound(
            (
                top_face,
                bottom_face,
                inner_face,
                outer_face,
                start_face,
                end_face,
            )
        )
        sewn.sewShape()
        if len(sewn.Shells) != 1 or not sewn.Shells[0].isClosed():
            return None
        solid = Part.makeSolid(sewn.Shells[0])
    except Part.OCCError:
        return None
    if solid.isValid() and len(solid.Solids) == 1:
        result = solid.removeSplitter()
        translation = FreeCAD.Matrix()
        translation.move(FreeCAD.Vector(circle_center[0], circle_center[1], 0.0))
        return result.transformShape(translation, True)
    return None


def _make_sectioned_helical_band_solid(
    profile,
    angles,
    top_elevations,
    bottom_elevations,
):
    """Create a circular board whose pitch follows every stair section."""

    if not (len(angles) == len(top_elevations) == len(bottom_elevations) and len(angles) >= 2):
        return None

    def interval_pitch(values, index):
        sweep = angles[index + 1] - angles[index]
        if abs(sweep) < 1e-9:
            return None
        return (values[index + 1] - values[index]) / sweep

    ranges = []
    range_start = 0
    previous_pitches = (
        interval_pitch(top_elevations, 0),
        interval_pitch(bottom_elevations, 0),
    )
    for index in range(1, len(angles) - 1):
        pitches = (
            interval_pitch(top_elevations, index),
            interval_pitch(bottom_elevations, index),
        )
        same_pitch = all(
            first is not None
            and second is not None
            and math.isclose(first, second, rel_tol=1e-8, abs_tol=1e-7)
            for first, second in zip(previous_pitches, pitches)
        )
        if not same_pitch:
            ranges.append((range_start, index))
            range_start = index
        previous_pitches = pitches
    ranges.append((range_start, len(angles) - 1))

    shapes = []
    for first, last in ranges:
        segment_profile = _circular_profile_between(profile, angles[first], angles[last])
        segment = _make_helical_band_solid(
            segment_profile,
            top_elevations[first],
            top_elevations[last],
            bottom_elevations[first],
            bottom_elevations[last],
        )
        if segment is None:
            return None
        shapes.append(segment)

    result = shapes[0]
    for shape in shapes[1:]:
        try:
            result = result.fuse(shape)
        except (Part.OCCError, ValueError):
            return None
    try:
        result = result.removeSplitter()
    except Part.OCCError:
        pass
    if result.isValid() and len(result.Solids) == 1:
        return result
    return None
