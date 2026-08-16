# SPDX-License-Identifier: LGPL-2.1-or-later

"""Wood tread, riser, and concrete stair solids."""

from dataclasses import replace
import math

import FreeCAD
import Part

from .geometry_core import (
    _CircularProfile,
    _cross,
    _shifted,
    balanced_section_top,
)

from .geometry_helical import (
    _annular_sector_face,
    _make_helical_annular_solid,
)

from .geometry_plan import (
    _balanced_step_faces,
    _half_plane_face,
    _horizontal_face,
    _line_intersection,
    balanced_tread_faces,
)


def make_balanced_tread_shape(
    front,
    rear,
    footprint,
    elevation,
    thickness,
    nosing,
    back_extension=0.0,
    base_face=None,
    local_expansion=False,
):
    """Create one polygonal tread between balanced nosing sections."""

    if base_face is None:
        faces = _balanced_step_faces(
            front,
            rear,
            footprint,
            front_offset=-nosing,
            rear_offset=back_extension,
        )
    else:
        plan_shape = base_face.copy()
        if nosing > 0.0 or back_extension > 0.0:
            expansions = _local_step_expansion_faces(
                front,
                rear,
                footprint,
                nosing,
                back_extension,
                local_expansion,
            )
            for expansion in expansions:
                plan_shape = plan_shape.fuse(expansion)
            plan_shape = plan_shape.removeSplitter()
        faces = plan_shape.Faces
    solids = []
    for face in faces:
        placed_face = face.copy()
        placed_face.translate(FreeCAD.Vector(0.0, 0.0, elevation - thickness))
        solids.append(placed_face.extrude(FreeCAD.Vector(0.0, 0.0, thickness)))
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def _local_step_expansion_faces(
    front,
    rear,
    footprint,
    nosing,
    back_extension,
    local_expansion=False,
):
    """Return edge-local tread bands without following another flight."""

    faces = []
    for section, distance in (
        (front, -nosing),
        (rear, back_extension),
    ):
        if abs(distance) < 1e-9:
            continue
        shifted_left = _shifted(section.left, section.tangent, distance)
        shifted_right = _shifted(section.right, section.tangent, distance)
        raw_strip = _horizontal_face(
            (
                section.left,
                section.right,
                shifted_right,
                shifted_left,
            ),
            0.0,
        )
        shifted_center = _shifted(section.center, section.tangent, distance)
        probe = FreeCAD.Vector(
            (section.center[0] + shifted_center[0]) / 2.0,
            (section.center[1] + shifted_center[1]) / 2.0,
            0.0,
        )
        # At the entrance or exit the expansion deliberately projects beyond
        # the footprint.  There is no adjoining tread there to provide a
        # clipping outline, so preserve the complete band.
        if not footprint.isInside(probe, 1e-6, True):
            faces.append(raw_strip)
            continue
        if distance < 0.0 and not local_expansion:
            # A regular balanced nosing can cross a footprint vertex.  Its
            # side boundary then changes rails inside the expansion and the
            # complete clipped section band is required to retain the corner.
            faces.extend(_section_band_faces(section, footprint, distance))
            continue
        side = (-section.tangent[1], section.tangent[0])
        shifted_left = _continued_section_endpoint(
            section.left,
            shifted_left,
            side,
            section.tangent,
            footprint,
        )
        shifted_right = _continued_section_endpoint(
            section.right,
            shifted_right,
            (-side[0], -side[1]),
            section.tangent,
            footprint,
        )
        strip = _horizontal_face(
            (
                section.left,
                section.right,
                shifted_right,
                shifted_left,
            ),
            0.0,
        )
        clipped = strip.common(footprint)
        candidates = clipped.Faces
        if not candidates:
            continue
        selected = next(
            (candidate for candidate in candidates if candidate.isInside(probe, 1e-6, True)),
            None,
        )
        if selected is None:
            point = Part.Vertex(probe)
            selected = min(
                candidates,
                key=lambda candidate: candidate.distToShape(point)[0],
            )
        faces.append(selected)
    return faces


def _continued_section_endpoint(
    original,
    shifted,
    outward,
    tangent,
    footprint,
):
    """Continue a translated section endpoint to its straight stair rail."""

    original_vertex = Part.Vertex(FreeCAD.Vector(original[0], original[1], 0.0))
    tolerance = max(footprint.BoundBox.DiagonalLength * 1e-7, 1e-6)
    candidates = []
    for edge in footprint.OuterWire.Edges:
        if edge.distToShape(original_vertex)[0] > tolerance:
            continue
        middle = (edge.FirstParameter + edge.LastParameter) / 2.0
        if abs(edge.curvatureAt(middle)) > 1e-9:
            continue
        vertices = edge.Vertexes
        if len(vertices) < 2:
            continue
        first = (vertices[0].Point.x, vertices[0].Point.y)
        second = (vertices[-1].Point.x, vertices[-1].Point.y)
        rail = (second[0] - first[0], second[1] - first[1])
        rail_length = math.hypot(rail[0], rail[1])
        if rail_length < 1e-9:
            continue
        rail = (rail[0] / rail_length, rail[1] / rail_length)
        if abs(rail[0] * tangent[0] + rail[1] * tangent[1]) < 1e-7:
            continue
        intersection = _line_intersection(shifted, outward, first, rail)
        if intersection is None:
            continue
        ray_parameter = (intersection[0] - shifted[0]) * outward[0] + (
            intersection[1] - shifted[1]
        ) * outward[1]
        rail_parameter = (intersection[0] - first[0]) * rail[0] + (
            intersection[1] - first[1]
        ) * rail[1]
        if ray_parameter >= -tolerance and -tolerance <= rail_parameter <= rail_length + tolerance:
            candidates.append((max(ray_parameter, 0.0), intersection))
    if not candidates:
        return shifted
    return min(candidates, key=lambda candidate: candidate[0])[1]


def _section_band_faces(section, footprint, distance):
    """Return the footprint band between a section and its parallel offset."""

    if abs(distance) < 1e-9:
        return []
    shifted_center = _shifted(section.center, section.tangent, distance)
    if distance > 0.0:
        start = section.center
        end = shifted_center
    else:
        start = shifted_center
        end = section.center
    extent = max(
        footprint.BoundBox.DiagonalLength * 4.0,
        abs(distance) + 1000.0,
    )
    after_start = _half_plane_face(start, section.tangent, extent)
    before_end = _half_plane_face(
        end,
        (-section.tangent[0], -section.tangent[1]),
        extent,
    )
    clipped = footprint.common(after_start).common(before_end)
    candidates = clipped.Faces
    if not candidates:
        return []

    probe = FreeCAD.Vector(
        (section.center[0] + shifted_center[0]) / 2.0,
        (section.center[1] + shifted_center[1]) / 2.0,
        0.0,
    )
    for face in candidates:
        if face.isInside(probe, 1e-6, True):
            return [face]
    point = Part.Vertex(probe)
    return [min(candidates, key=lambda face: face.distToShape(point)[0])]


def make_balanced_riser_shape(
    section,
    base_elevation,
    height,
    thickness,
    footprint=None,
    local_expansion=False,
    concrete_dressing=False,
):
    """Create one vertical riser following a balanced nosing section."""

    if footprint is None:
        distance = -thickness if concrete_dressing else thickness
        rear_left = _shifted(section.left, section.tangent, distance)
        rear_right = _shifted(section.right, section.tangent, distance)
        faces = [
            _horizontal_face(
                (section.left, section.right, rear_right, rear_left),
                base_elevation,
            )
        ]
    else:
        faces = _local_step_expansion_faces(
            section,
            section,
            footprint,
            thickness if concrete_dressing else 0.0,
            0.0 if concrete_dressing else thickness,
            local_expansion,
        )

    solids = []
    for face in faces:
        placed_face = face.copy()
        placed_face.translate(FreeCAD.Vector(0.0, 0.0, base_elevation))
        solids.append(placed_face.extrude(FreeCAD.Vector(0.0, 0.0, max(height, 0.01))))
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def _inset_structure_plan(sections, footprint, plan_shapes, side_offset):
    """Inset both stair sides while retaining the original end stations."""

    requested_offset = max(float(side_offset), 0.0)
    if requested_offset <= 1e-9:
        return sections, footprint, plan_shapes

    half_widths = []
    for section in sections:
        for point in (section.left, section.right):
            half_widths.append(
                math.hypot(
                    point[0] - section.center[0],
                    point[1] - section.center[1],
                )
            )
    maximum_offset = max(min(half_widths) - 0.005, 0.0)
    effective_offset = min(requested_offset, maximum_offset)
    if effective_offset <= 1e-9:
        return sections, footprint, plan_shapes

    def inset_point(point, center):
        dx = center[0] - point[0]
        dy = center[1] - point[1]
        distance = math.hypot(dx, dy)
        if distance <= 1e-9:
            return point
        scale = effective_offset / distance
        return (
            point[0] + dx * scale,
            point[1] + dy * scale,
        )

    inset_sections = [
        replace(
            section,
            left=inset_point(section.left, section.center),
            right=inset_point(section.right, section.center),
            width=max(section.width - 2.0 * effective_offset, 0.01),
        )
        for section in sections
    ]

    circular_profiles = []
    for index, plan_shape in enumerate(plan_shapes):
        if index + 1 >= len(sections):
            circular_profiles = []
            break
        if len(plan_shape.Faces) != 1:
            circular_profiles = []
            break
        profile = _circular_profile_data(
            plan_shape.Faces[0],
            sections[index],
            sections[index + 1],
        )
        if profile is None or (
            circular_profiles and not _circular_profiles_join(circular_profiles[-1], profile)
        ):
            circular_profiles = []
            break
        circular_profiles.append(profile)
    if plan_shapes and len(circular_profiles) == len(plan_shapes):
        inset_profiles = [
            replace(
                profile,
                inner_radius=profile.inner_radius + effective_offset,
                outer_radius=profile.outer_radius - effective_offset,
            )
            for profile in circular_profiles
        ]
        inset_plan_shapes = [_annular_sector_face(profile, 0.0) for profile in inset_profiles]
        first_profile = inset_profiles[0]
        inset_footprint = _annular_sector_face(
            replace(
                first_profile,
                sweep=sum(profile.sweep for profile in inset_profiles),
            ),
            0.0,
        )
        return inset_sections, inset_footprint, inset_plan_shapes

    try:
        corridor = Part.Face(footprint.OuterWire).makeOffset2D(-effective_offset)
    except Exception:
        return sections, footprint, plan_shapes
    if corridor.isNull() or not corridor.Faces:
        return sections, footprint, plan_shapes

    cap_length = max(footprint.BoundBox.DiagonalLength, 1.0)
    bridge = max(2.0 * effective_offset, 0.1)

    def cap_face(section, front_distance, rear_distance):
        front_left = _shifted(section.left, section.tangent, front_distance)
        front_right = _shifted(section.right, section.tangent, front_distance)
        rear_left = _shifted(section.left, section.tangent, rear_distance)
        rear_right = _shifted(section.right, section.tangent, rear_distance)
        return _horizontal_face(
            (front_left, front_right, rear_right, rear_left),
            0.0,
        )

    corridor = corridor.fuse(cap_face(inset_sections[0], -cap_length, bridge))
    corridor = corridor.fuse(cap_face(inset_sections[-1], -bridge, cap_length)).removeSplitter()

    inset_plan_shapes = []
    for plan_shape in plan_shapes:
        inset_shape = plan_shape.common(corridor)
        if inset_shape.isNull() or not inset_shape.Faces:
            return sections, footprint, plan_shapes
        inset_plan_shapes.append(inset_shape.removeSplitter())
    return inset_sections, corridor, inset_plan_shapes


def make_balanced_concrete_shape(
    sections,
    footprint,
    riser_height,
    thickness,
    plan_shapes=None,
    bottom_cut_distance=0.0,
    top_cut_distance=0.0,
    finish_thickness=0.0,
    structure_width_offset=0.0,
):
    """Create a monolithic stepped concrete stair along balanced sections."""

    if len(sections) < 2:
        return Part.Shape()
    if plan_shapes is None:
        plan_shapes = balanced_tread_faces(sections, footprint)
    sections, footprint, plan_shapes = _inset_structure_plan(
        sections,
        footprint,
        plan_shapes,
        structure_width_offset,
    )
    going = max(sections[1].station - sections[0].station, 0.01)
    slope = math.atan(riser_height / going)
    slope_cosine = max(math.cos(slope), 0.01)
    minimum_waist = riser_height * slope_cosine
    concrete_thickness = max(float(thickness), 0.01)
    effective_waist = minimum_waist + concrete_thickness
    vertical_waist = effective_waist / slope_cosine
    bottom_cut_level = -max(float(bottom_cut_distance), 0.0)

    finish_thickness = max(float(finish_thickness), 0.0)
    top_elevations = [
        balanced_section_top(section, index, riser_height) - finish_thickness
        for index, section in enumerate(sections)
    ]
    bottom_elevations = [max(top - vertical_waist, bottom_cut_level) for top in top_elevations]
    bottom_fronts = bottom_elevations[:-1]
    bottom_rears = bottom_elevations[1:]
    if bottom_rears:
        bottom_rears[-1] = max(
            top_elevations[-2] - concrete_thickness,
            bottom_cut_level,
        )
    for index, section in enumerate(sections[:-1]):
        if section.landing_to_next:
            landing_top = top_elevations[index]
            landing_bottom = max(
                landing_top - concrete_thickness,
                bottom_cut_level,
            )
            bottom_fronts[index] = landing_bottom
            bottom_rears[index] = landing_bottom
            if index + 1 < len(bottom_fronts):
                bottom_fronts[index + 1] = max(bottom_fronts[index + 1], landing_bottom)

    _align_straight_concrete_bottoms(
        sections,
        top_elevations,
        bottom_fronts,
        bottom_rears,
        bottom_cut_level,
    )

    solids = []
    circular_profiles = []
    for front, rear, plan_shape in zip(sections, sections[1:], plan_shapes):
        profile = None
        if len(plan_shape.Faces) == 1:
            profile = _circular_profile_data(plan_shape.Faces[0], front, rear)
        circular_profiles.append(profile)

    index = 0
    while index < len(plan_shapes):
        if sections[index].landing_to_next:
            landing_top = top_elevations[index]
            landing_bottom = bottom_fronts[index]
            landing_solids = []
            for plan_face in plan_shapes[index].Faces:
                bottom_face = plan_face.copy()
                bottom_face.translate(FreeCAD.Vector(0.0, 0.0, landing_bottom))
                landing_solids.append(
                    bottom_face.extrude(FreeCAD.Vector(0.0, 0.0, landing_top - landing_bottom))
                )

            incoming_bottom = bottom_elevations[index]
            predecessor_reaches_landing = (
                index > 0 and abs(top_elevations[index - 1] - landing_top) < 1e-7
            )
            if not predecessor_reaches_landing and landing_bottom - incoming_bottom > 1e-7:
                joint_depth = min(0.1, max(footprint.BoundBox.DiagonalLength * 1e-6, 0.01))
                for joint_face in _section_band_faces(
                    sections[index], plan_shapes[index], joint_depth
                ):
                    bottom_face = joint_face.copy()
                    bottom_face.translate(FreeCAD.Vector(0.0, 0.0, incoming_bottom))
                    landing_solids.append(
                        bottom_face.extrude(FreeCAD.Vector(0.0, 0.0, landing_top - incoming_bottom))
                    )

            landing = landing_solids[0]
            for addition in landing_solids[1:]:
                landing = landing.fuse(addition)
            solids.append(landing.removeSplitter())
            index += 1
            continue

        profile = circular_profiles[index]
        span_end = index
        if profile is not None and abs(bottom_rears[index] - bottom_fronts[index]) > 1e-7:
            pitch = (bottom_rears[index] - bottom_fronts[index]) / profile.sweep
            while span_end + 1 < len(plan_shapes):
                following = circular_profiles[span_end + 1]
                if following is None or not _circular_profiles_join(
                    circular_profiles[span_end], following
                ):
                    break
                following_pitch = (
                    bottom_rears[span_end + 1] - bottom_fronts[span_end + 1]
                ) / following.sweep
                if abs(following_pitch - pitch) > max(abs(pitch), 1.0) * 1e-6:
                    break
                span_end += 1

        if span_end > index:
            circular = _make_circular_concrete_span(
                sections,
                plan_shapes,
                circular_profiles,
                index,
                span_end,
                riser_height,
                bottom_fronts,
                bottom_rears,
                top_elevations,
            )
            if circular is not None:
                solids.append(circular)
                index = span_end + 1
                continue

        front = sections[index]
        rear = sections[index + 1]
        plan_shape = plan_shapes[index]
        top = top_elevations[index]
        bottom_front = bottom_fronts[index]
        bottom_rear = bottom_rears[index]
        tread_solids = [
            _make_profiled_plan_solid(plan_face, front, rear, top, bottom_front, bottom_rear)
            for plan_face in plan_shape.Faces
        ]
        tread = tread_solids[0]
        for tread_solid in tread_solids[1:]:
            tread = tread.fuse(tread_solid)
        solids.append(tread.removeSplitter())
        index += 1

    top_cut_distance = max(float(top_cut_distance), 0.0)
    terminal_top = top_elevations[-2]
    terminal_bottom = bottom_rears[-1]
    pitch = max(float(riser_height) / going, 1e-9)
    available_top_extension = max(
        (terminal_top - terminal_bottom) / pitch,
        0.0,
    )
    extension = min(top_cut_distance, available_top_extension)
    if available_top_extension > 1e-9 and top_cut_distance >= available_top_extension:
        tip_tolerance = min(0.01, available_top_extension / 2.0)
        extension = available_top_extension - tip_tolerance
    if extension > 1e-9:
        terminal = sections[-1]
        joint_depth = min(
            0.1,
            max(footprint.BoundBox.DiagonalLength * 1e-6, 0.01),
        )
        front = replace(
            terminal,
            center=_shifted(terminal.center, terminal.tangent, -joint_depth),
            left=_shifted(terminal.left, terminal.tangent, -joint_depth),
            right=_shifted(terminal.right, terminal.tangent, -joint_depth),
            station=terminal.station - joint_depth,
        )
        rear = replace(
            terminal,
            center=_shifted(terminal.center, terminal.tangent, extension),
            left=_shifted(terminal.left, terminal.tangent, extension),
            right=_shifted(terminal.right, terminal.tangent, extension),
            station=terminal.station + extension,
        )
        extension_faces = _local_step_expansion_faces(
            terminal,
            terminal,
            footprint,
            0.0,
            extension,
            True,
        )
        extension_faces.extend(
            _section_band_faces(
                terminal,
                footprint,
                -joint_depth,
            )
        )
        extension_plan = extension_faces[0]
        for extension_face in extension_faces[1:]:
            extension_plan = extension_plan.fuse(extension_face)
        for extension_face in extension_plan.Faces:
            solids.append(
                _make_profiled_plan_solid(
                    extension_face,
                    front,
                    rear,
                    terminal_top,
                    terminal_bottom - pitch * joint_depth,
                    terminal_bottom + pitch * extension,
                )
            )

    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def _align_straight_concrete_bottoms(
    sections,
    top_elevations,
    bottom_fronts,
    bottom_rears,
    bottom_cut_level=0.0,
):
    """Make each straight concrete run use one continuous soffit plane."""

    cell_count = min(len(bottom_fronts), len(sections) - 1)

    def is_straight_cell(index):
        front = sections[index]
        rear = sections[index + 1]
        if front.landing_to_next or front.level_to_next:
            return False
        cross = abs(_cross(front.tangent, rear.tangent))
        dot = front.tangent[0] * rear.tangent[0] + front.tangent[1] * rear.tangent[1]
        return cross < 1e-7 and dot > 0.0

    index = 0
    while index < cell_count:
        if not is_straight_cell(index):
            index += 1
            continue

        run_start = index
        flight_index = sections[index].flight_index
        run_tangent = sections[index].tangent
        index += 1
        while index < cell_count:
            if not is_straight_cell(index):
                break
            front = sections[index]
            if front.flight_index != flight_index:
                break
            if abs(_cross(run_tangent, front.tangent)) >= 1e-7:
                break
            index += 1
        run_end = index - 1

        start_bottom = bottom_fronts[run_start]
        if run_start > 0 and sections[run_start - 1].landing_to_next:
            start_bottom = bottom_rears[run_start - 1]

        end_bottom = bottom_rears[run_end]
        if (
            run_end + 1 < cell_count
            and sections[run_end + 1].landing_to_next
            and abs(top_elevations[run_end] - top_elevations[run_end + 1]) < 1e-7
        ):
            end_bottom = bottom_fronts[run_end + 1]

        smooth_start = run_start
        if run_start == 0 and run_end > run_start and abs(start_bottom - bottom_cut_level) < 1e-7:
            bottom_fronts[run_start] = bottom_cut_level
            bottom_rears[run_start] = bottom_cut_level
            smooth_start += 1
            start_bottom = bottom_cut_level

        if smooth_start > run_end:
            continue

        distances = [0.0]
        for boundary in range(smooth_start, run_end + 1):
            first = sections[boundary].center
            second = sections[boundary + 1].center
            distances.append(distances[-1] + math.hypot(second[0] - first[0], second[1] - first[1]))
        total_distance = distances[-1]
        if total_distance < 1e-9:
            continue

        for offset, cell_index in enumerate(range(smooth_start, run_end + 1)):
            front_ratio = distances[offset] / total_distance
            rear_ratio = distances[offset + 1] / total_distance
            bottom_fronts[cell_index] = start_bottom + (end_bottom - start_bottom) * front_ratio
            bottom_rears[cell_index] = start_bottom + (end_bottom - start_bottom) * rear_ratio


def _make_profiled_plan_solid(
    plan_face,
    front,
    rear,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build a square-edged tread volume above a locally sloping underside."""

    if abs(bottom_rear - bottom_front) < 1e-7:
        bottom_face = plan_face.copy()
        bottom_face.translate(FreeCAD.Vector(0.0, 0.0, bottom_front))
        return bottom_face.extrude(FreeCAD.Vector(0.0, 0.0, top_elevation - bottom_front))

    helical = _make_helical_profiled_plan_solid(
        plan_face,
        front,
        rear,
        top_elevation,
        bottom_front,
        bottom_rear,
    )
    if helical is not None:
        return helical

    mesh_points, facets = plan_face.tessellate(0.1)

    def bottom_elevation(point):
        front_distance = (point.x - front.center[0]) * front.tangent[0] + (
            point.y - front.center[1]
        ) * front.tangent[1]
        rear_distance = -(
            (point.x - rear.center[0]) * rear.tangent[0]
            + (point.y - rear.center[1]) * rear.tangent[1]
        )
        denominator = front_distance + rear_distance
        ratio = front_distance / denominator if abs(denominator) > 1e-9 else 0.5
        ratio = min(max(ratio, 0.0), 1.0)
        return bottom_front + (bottom_rear - bottom_front) * ratio

    top_points = [FreeCAD.Vector(point.x, point.y, top_elevation) for point in mesh_points]
    bottom_points = [
        FreeCAD.Vector(point.x, point.y, bottom_elevation(point)) for point in mesh_points
    ]
    faces = []
    oriented_facets = []
    for first, second, third in facets:
        top_triangle = (top_points[first], top_points[second], top_points[third])
        cross = (top_triangle[1] - top_triangle[0]).cross(top_triangle[2] - top_triangle[0])
        if cross.z < 0.0:
            second, third = third, second
            top_triangle = (top_triangle[0], top_triangle[2], top_triangle[1])
        oriented_facets.append((first, second, third))
        faces.append(_triangle_face(*top_triangle))
        faces.append(
            _triangle_face(
                bottom_points[first],
                bottom_points[third],
                bottom_points[second],
            )
        )

    boundary_edges = {}
    for first, second, third in oriented_facets:
        for start, end in ((first, second), (second, third), (third, first)):
            key = tuple(sorted((start, end)))
            if key in boundary_edges:
                del boundary_edges[key]
            else:
                boundary_edges[key] = (start, end)
    for first, second in boundary_edges.values():
        faces.append(
            _triangle_face(
                bottom_points[first],
                bottom_points[second],
                top_points[second],
            )
        )
        faces.append(
            _triangle_face(
                bottom_points[first],
                top_points[second],
                top_points[first],
            )
        )
    return Part.makeSolid(Part.makeShell(faces))


def _make_helical_profiled_plan_solid(
    plan_face,
    front,
    rear,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build an annular tread cell with an exact OCC helical underside."""

    profile = _circular_profile_data(plan_face, front, rear)
    if profile is None:
        return None
    return _make_helical_annular_solid(
        profile,
        top_elevation,
        bottom_front,
        bottom_rear,
    )


def _circular_profile_data(plan_face, front, rear):
    """Return exact circle data when ``plan_face`` is an annular sector."""

    curved_edges = []
    for edge in plan_face.Edges:
        parameter = (edge.FirstParameter + edge.LastParameter) / 2.0
        if abs(edge.curvatureAt(parameter)) > 1e-9:
            curved_edges.append(edge)
    if len(curved_edges) != 2:
        return None

    front_direction = (
        front.left[0] - front.right[0],
        front.left[1] - front.right[1],
    )
    rear_direction = (
        rear.left[0] - rear.right[0],
        rear.left[1] - rear.right[1],
    )
    denominator = _cross(front_direction, rear_direction)
    if abs(denominator) < 1e-9:
        return None
    relative = (
        rear.center[0] - front.center[0],
        rear.center[1] - front.center[1],
    )
    front_parameter = _cross(relative, rear_direction) / denominator
    circle_center = (
        front.center[0] + front_direction[0] * front_parameter,
        front.center[1] + front_direction[1] * front_parameter,
    )
    front_radial = (
        front.center[0] - circle_center[0],
        front.center[1] - circle_center[1],
    )
    rear_radial = (
        rear.center[0] - circle_center[0],
        rear.center[1] - circle_center[1],
    )
    radial_dot = front_radial[0] * rear_radial[0] + front_radial[1] * rear_radial[1]
    sweep = math.atan2(_cross(front_radial, rear_radial), radial_dot)
    if abs(sweep) < 1e-7:
        return None

    def radii(section):
        return sorted(
            math.hypot(point[0] - circle_center[0], point[1] - circle_center[1])
            for point in (section.left, section.right)
        )

    front_radii = radii(front)
    rear_radii = radii(rear)
    tolerance = max(front.width, rear.width, 1.0) * 1e-6
    if any(abs(first - second) > tolerance for first, second in zip(front_radii, rear_radii)):
        return None
    inner_radius = (front_radii[0] + rear_radii[0]) / 2.0
    outer_radius = (front_radii[1] + rear_radii[1]) / 2.0
    expected_area = 0.5 * (outer_radius * outer_radius - inner_radius * inner_radius) * abs(sweep)
    if abs(plan_face.Area - expected_area) > max(expected_area * 1e-6, 0.01):
        return None

    return _CircularProfile(
        circle_center,
        inner_radius,
        outer_radius,
        math.atan2(front_radial[1], front_radial[0]),
        sweep,
    )


def _circular_profiles_join(first, second):
    """Return whether two annular cells belong to one circular flight."""

    tolerance = max(first.outer_radius, second.outer_radius, 1.0) * 1e-6
    if (
        math.hypot(
            first.center[0] - second.center[0],
            first.center[1] - second.center[1],
        )
        > tolerance
    ):
        return False
    if abs(first.inner_radius - second.inner_radius) > tolerance:
        return False
    if abs(first.outer_radius - second.outer_radius) > tolerance:
        return False
    if first.sweep * second.sweep <= 0.0:
        return False
    first_end = first.start_angle + first.sweep
    angle_difference = math.atan2(
        math.sin(second.start_angle - first_end),
        math.cos(second.start_angle - first_end),
    )
    return abs(angle_difference) < 1e-6


def _make_circular_concrete_span(
    sections,
    plan_shapes,
    profiles,
    start_index,
    end_index,
    riser_height,
    bottom_fronts,
    bottom_rears,
    top_elevations,
):
    """Build one circular flight with two cylinders and one helical soffit."""

    first = profiles[start_index]
    sweep = sum(profiles[index].sweep for index in range(start_index, end_index + 1))
    bottom_front = bottom_fronts[start_index]
    bottom_rear = bottom_rears[end_index]
    base_elevation = min(bottom_front, bottom_rear) - max(riser_height, 1.0)
    envelope_solids = []
    for index in range(start_index, end_index + 1):
        top = top_elevations[index]
        for plan_face in plan_shapes[index].Faces:
            placed_face = plan_face.copy()
            placed_face.translate(FreeCAD.Vector(0.0, 0.0, base_elevation))
            envelope_solids.append(
                placed_face.extrude(FreeCAD.Vector(0.0, 0.0, top - base_elevation))
            )
    envelope = envelope_solids[0]
    for solid in envelope_solids[1:]:
        envelope = envelope.fuse(solid)
    envelope = envelope.removeSplitter()

    radial_margin = max(first.outer_radius - first.inner_radius, 1.0) * 1e-3
    angular_margin = 1e-4
    direction = 1.0 if sweep > 0.0 else -1.0
    height_per_angle = (bottom_rear - bottom_front) / abs(sweep)
    expanded = _CircularProfile(
        first.center,
        max(first.inner_radius - radial_margin, 0.01),
        first.outer_radius + radial_margin,
        first.start_angle - direction * angular_margin,
        sweep + direction * 2.0 * angular_margin,
    )
    profiled_envelope = _make_helical_annular_solid(
        expanded,
        top_elevations[end_index] + max(riser_height, 1.0),
        bottom_front - height_per_angle * angular_margin,
        bottom_rear + height_per_angle * angular_margin,
    )
    if profiled_envelope is None:
        return None
    result = envelope.common(profiled_envelope).removeSplitter()
    if result.isValid() and len(result.Solids) == 1:
        return result
    return None


def _triangle_face(first, second, third):
    return Part.Face(Part.makePolygon([first, second, third, first]))


def _make_triangulated_solid(top_points, bottom_points):
    """Make a solid whose potentially warped quadrilateral faces are triangulated."""

    def triangle(first, second, third):
        return Part.Face(Part.makePolygon([first, second, third, first]))

    faces = [
        triangle(top_points[0], top_points[1], top_points[2]),
        triangle(top_points[0], top_points[2], top_points[3]),
        triangle(bottom_points[0], bottom_points[2], bottom_points[1]),
        triangle(bottom_points[0], bottom_points[3], bottom_points[2]),
    ]
    for index in range(4):
        following = (index + 1) % 4
        faces.extend(
            (
                triangle(
                    bottom_points[index],
                    bottom_points[following],
                    top_points[following],
                ),
                triangle(
                    bottom_points[index],
                    top_points[following],
                    top_points[index],
                ),
            )
        )
    return Part.makeSolid(Part.makeShell(faces))


def balanced_plan_segments(sections, footprint):
    """Return the fixed flight boundary and balanced nosings."""

    segments = []
    outline_edges = {}
    for face in footprint.Faces:
        for edge in face.OuterWire.Edges:
            if len(edge.Vertexes) >= 2:
                first = edge.Vertexes[0].Point
                last = edge.Vertexes[-1].Point
                endpoints = sorted(
                    (
                        (round(first.x, 7), round(first.y, 7)),
                        (round(last.x, 7), round(last.y, 7)),
                    )
                )
                key = tuple(endpoints)
                outline_edges.setdefault(key, []).append(((first.x, first.y), (last.x, last.y)))
    for matches in outline_edges.values():
        if len(matches) == 1:
            segments.append(matches[0])
    for section in sections:
        segments.append((section.left, section.right))
    return segments


def balanced_plan_geometry(sections, footprint):
    """Return bounded line/arc geometry for the generated plan sketch."""

    result = []
    seen = set()
    for face in footprint.Faces:
        for edge in face.OuterWire.Edges:
            first = edge.valueAt(edge.FirstParameter)
            last = edge.valueAt(edge.LastParameter)
            endpoints = tuple(
                sorted(
                    (
                        (round(first.x, 7), round(first.y, 7)),
                        (round(last.x, 7), round(last.y, 7)),
                    )
                )
            )
            key = endpoints, round(edge.Length, 7)
            if key in seen:
                continue
            seen.add(key)
            middle = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2.0)
            if abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9:
                result.append(Part.Arc(first, middle, last))
            else:
                result.append(Part.LineSegment(first, last))
    result.extend(
        Part.LineSegment(
            FreeCAD.Vector(section.left[0], section.left[1], 0.0),
            FreeCAD.Vector(section.right[0], section.right[1], 0.0),
        )
        for section in sections
    )
    return result
