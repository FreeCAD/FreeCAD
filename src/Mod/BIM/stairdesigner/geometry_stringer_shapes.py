# SPDX-License-Identifier: LGPL-2.1-or-later

"""Housed and notched stringer solids."""

import math

import FreeCAD
import Part

from .geometry_core import (
    _translated_section,
)

from .geometry_helical import (
    _annular_sector_face,
    _circular_profile_between,
    _make_helical_annular_solid,
    _make_helical_band_solid,
    _make_sectioned_helical_band_solid,
)

from .geometry_stringer_path import (
    PROFILE_MAXIMUM_PITCH,
    STRINGER_TIMBER_BELOW_STEPS,
    EdgeBlock,
    _circular_stringer_data,
    _edges_above_floor,
    _make_planar_housed_stringer_shape,
    _stringer_cross_section,
    _stringer_elevations,
    _stringer_inward,
    _stringer_section_runs,
    _stringer_slope,
    notched_board_edges,
    stringer_straight_pitch,
)


def _local_pitches(sections, elevations, side, rear_edge=False):
    """Return the board pitch at each nosing of a planar board.

    Winders make the nosings climb faster at one end of a board than at the
    other, so the position above the nosings follows the local pitch. Fan
    treads carried by a newel post, and the final tread's ``rear_edge``,
    take the pitch of the nearest nosing that bears on the board. Returns
    ``None`` when fewer than two nosings do.

    A straight board profiles most nosings where their tips cross it, but
    its ends at the riser lines. Mixing both would bend the pitch near the
    ends, so the tip crossings alone set it when the board has them.
    """

    points = [section.left if side == "Left" else section.right for section in sections]
    distances = [math.dist(points[0], point) for point in points]
    borne = [
        index
        for index, section in enumerate(sections)
        if not section.profile_at_newel and not (rear_edge and index == len(sections) - 1)
    ]
    aligned = [index for index in borne if sections[index].profile_nosing_aligned]
    if len(aligned) >= 2:
        borne = aligned
    if len(borne) < 2:
        return None
    borne_pitches = []
    for position, index in enumerate(borne):
        previous = borne[max(position - 1, 0)]
        following = borne[min(position + 1, len(borne) - 1)]
        run = distances[following] - distances[previous]
        if run < 1e-6:
            return None
        borne_pitches.append((elevations[following] - elevations[previous]) / run)
    return [
        borne_pitches[min(range(len(borne)), key=lambda position: abs(borne[position] - index))]
        for index in range(len(sections))
    ]


def make_housed_stringer_shape(
    sections,
    riser_height,
    side,
    thickness,
    width,
    penetration,
    start_extension,
    end_extension,
    nosing_offset,
    offset_direction="Perpendicular",
    nosing=0.0,
    single_pitch=False,
    supports=(),
    underside_offset=STRINGER_TIMBER_BELOW_STEPS,
    underside_direction="Perpendicular",
    final_riser=True,
    start_block=None,
    end_block=None,
):
    """Create one smooth closed stringer housing the tread and riser ends.

    ``single_pitch`` keeps the upper edge to one straight pitch, for a board
    too short to follow its nosings. ``supports`` are housing corners,
    from ``housed_board_supports``, that the lower edge clears by
    ``underside_offset``, measured as ``underside_direction`` says.
    ``final_riser`` tells whether a riser rises behind the board's last
    tread; without one, as at the top of a stair ending on its last tread,
    that tread is the floor and the board runs on level over it.
    ``start_block`` and ``end_block`` are the ``EdgeBlock`` a flat board's
    ends run level into, see ``level_board_corners``.
    """

    if len(sections) < 2:
        return Part.Shape()
    runs = _stringer_section_runs(sections)
    if any(section.level_to_next for section in sections[:-1]):
        shapes = [
            _make_housed_stringer_run(
                run,
                riser_height,
                side,
                thickness,
                width,
                penetration,
                start_extension if index == 0 else 0.0,
                end_extension if index == len(runs) - 1 else 0.0,
                nosing_offset,
                offset_direction,
                nosing,
                single_pitch,
                supports,
                underside_offset,
                underside_direction,
                final_riser or index < len(runs) - 1,
                start_block if index == 0 else None,
                end_block if index == len(runs) - 1 else None,
            )
            for index, run in enumerate(runs)
        ]
        if not shapes:
            return Part.Shape()
        return shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
    return _make_housed_stringer_run(
        sections,
        riser_height,
        side,
        thickness,
        width,
        penetration,
        start_extension,
        end_extension,
        nosing_offset,
        offset_direction,
        nosing,
        single_pitch,
        supports,
        underside_offset,
        underside_direction,
        final_riser,
        start_block,
        end_block,
    )


def _make_housed_stringer_run(
    sections,
    riser_height,
    side,
    thickness,
    width,
    penetration,
    start_extension,
    end_extension,
    nosing_offset,
    offset_direction,
    nosing,
    single_pitch=False,
    supports=(),
    underside_offset=STRINGER_TIMBER_BELOW_STEPS,
    underside_direction="Perpendicular",
    final_riser=True,
    start_block=None,
    end_block=None,
):
    """Create one continuous housed-stringer run."""

    thickness = max(float(thickness), 0.01)
    width = max(float(width), 0.01)
    start_extension = max(float(start_extension), 0.0)
    end_extension = max(float(end_extension), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    rear_edge = (
        len(elevations) >= 2
        and abs(elevations[-1] - elevations[-2]) < 1e-9
        and not any(section.landing_to_next for section in sections)
        and math.dist(sections[-1].center, sections[-2].center) > 1e-6
    )
    # The last section is the rear edge of the final tread, rather than
    # another nosing.
    level_end = rear_edge and not final_riser
    if rear_edge and final_riser:
        # A riser rises behind it: continue the board at the stair pitch
        # through it; otherwise a smooth loft flattens and overshoots before
        # its extension.
        elevations[-1] = elevations[-2] + float(riser_height)
    circular = _circular_stringer_data(
        sections,
        side,
        thickness,
        penetration,
        True,
    )
    if circular is not None:
        circular_run = abs(circular["profile"].sweep) * circular["path_radius"]
        slope = (elevations[-1] - elevations[0]) / circular_run if circular_run > 1e-9 else 0.0
    else:
        slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    pitches = (
        None if circular is not None else _local_pitches(sections, elevations, side, rear_edge)
    )

    def top_above(section, elevation, pitch):
        # The offset is square to the board edge, measured from the nosing
        # tip, which lies ``nosing`` further down that pitch than the riser.
        # A fan of winders around a newel can climb almost vertically along
        # a short board; an offset square to that would rise without bound.
        steepest = 2.0 * PROFILE_MAXIMUM_PITCH
        pitch = max(-steepest, min(pitch, steepest))
        vertical_offset = max(float(nosing_offset), 0.0)
        if str(offset_direction) == "Perpendicular":
            vertical_offset *= math.sqrt(1.0 + pitch * pitch)
        compensation = 0.0 if section.profile_nosing_aligned else pitch * max(float(nosing), 0.0)
        return elevation + compensation + vertical_offset

    if pitches is None:
        pitches = [slope] * len(sections)
    else:
        # When the nosings fit one straight pitch, the board edge has that
        # pitch everywhere: measure every offset square to it, keeping the
        # board parallel to the rail fitted over the same nosings.
        # A uniform offset leaves the fitted pitch unchanged, so this settles
        # on the second pass.
        points = [section.left if side == "Left" else section.right for section in sections]
        distances = [math.dist(points[0], point) for point in points]
        carried = [bool(section.profile_at_newel) for section in sections]
        for _attempt in range(3):
            straight = stringer_straight_pitch(
                distances,
                [
                    top_above(section, elevation, pitch)
                    for section, elevation, pitch in zip(sections, elevations, pitches)
                ],
                single_pitch,
                carried,
            )
            if straight is None or all(abs(pitch - straight) < 1e-12 for pitch in pitches):
                break
            pitches = [straight] * len(sections)

    stations = list(sections)
    tops = [
        top_above(section, elevation, pitch)
        for section, elevation, pitch in zip(sections, elevations, pitches)
    ]
    if level_end:
        # The final tread is the floor: the board runs on level over it, at
        # the height it has above the last nosing, rather than climbing a
        # riser that is not there.
        tops[-1] = tops[-2]
        pitches = list(pitches)
        pitches[-1] = 0.0
        if end_block is None:
            # Its upper edge arrives level at the last nosing; nothing
            # asks the lower one to.
            last_nosing = sections[-2].left if side == "Left" else sections[-2].right
            end_block = EdgeBlock(last_nosing, bottom=False)
    if circular is not None:
        profile = circular["profile"]
        direction = 1.0 if profile.sweep > 0.0 else -1.0
        start_angle_extension = start_extension / circular["path_radius"]
        end_angle_extension = end_extension / circular["path_radius"]
        angles = list(circular["angles"])
        circular_tops = list(tops)
        circular_bottoms = [max(top - vertical_width, 0.0) for top in tops]
        if start_angle_extension > 1e-9:
            angles.insert(0, profile.start_angle - direction * start_angle_extension)
            circular_tops.insert(0, tops[0] - slope * start_extension)
            circular_bottoms.insert(
                0,
                max(
                    tops[0] - vertical_width - slope * start_extension,
                    0.0,
                ),
            )
        if end_angle_extension > 1e-9:
            angles.append(profile.start_angle + profile.sweep + direction * end_angle_extension)
            end_slope = 0.0 if level_end else slope
            circular_tops.append(tops[-1] + end_slope * end_extension)
            circular_bottoms.append(
                max(
                    tops[-1] - vertical_width + end_slope * end_extension,
                    0.0,
                )
            )
        circular_shape = _make_sectioned_helical_band_solid(
            profile,
            angles,
            circular_tops,
            circular_bottoms,
        )
        if circular_shape is not None:
            return circular_shape

    if start_extension > 1e-9:
        stations.insert(0, _translated_section(sections[0], -start_extension))
        tops.insert(0, tops[0] - pitches[0] * start_extension)
    if end_extension > 1e-9:
        stations.append(_translated_section(sections[-1], end_extension))
        tops.append(tops[-1] + pitches[-1] * end_extension)

    planar_shape = _make_planar_housed_stringer_shape(
        stations,
        tops,
        side,
        thickness,
        penetration,
        width,
        single_pitch,
        supports,
        underside_offset,
        underside_direction,
        start_block,
        end_block,
    )
    if planar_shape is not None:
        return planar_shape

    wires = [
        _stringer_cross_section(
            section,
            side,
            max(top - vertical_width, 0.0),
            top,
            thickness,
            float(penetration),
            True,
        )
        for section, top in zip(stations, tops)
    ]
    result = Part.makeLoft(wires, True, False)
    if result.isValid() and len(result.Solids) == 1:
        try:
            return result.removeSplitter()
        except Part.OCCError:
            return result

    segments = [
        Part.makeLoft([first, second], True, True) for first, second in zip(wires, wires[1:])
    ]
    result = segments[0]
    for segment in segments[1:]:
        result = result.fuse(segment)
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def housed_board_supports(board, sections, side, thickness, penetration, steps, newel_reach=0.0):
    """Return the lowest corners of the steps housed in a planar board.

    ``board`` is a housed board built over ``sections`` and ``steps`` are the
    tread and riser solids, in the same coordinates. Each step gives the
    lowest corner of the part of it inside the board's thickness, furthest
    up the board: the lower edge is fitted under these as the upper edge is
    fitted over the nosings. Winders turning a corner, or
    converging on a newel, sink those corners far below the edge of a
    regular board. Corners on the floor are left out, where the board is cut
    level, and so are those within ``newel_reach`` of a newel the board's
    fan treads meet: the post carries them. Returns an empty list when the
    board is not planar.
    """

    if board.isNull() or len(sections) < 2:
        return []
    points = [section.left if side == "Left" else section.right for section in sections]
    span = (points[-1][0] - points[0][0], points[-1][1] - points[0][1])
    length = math.hypot(*span)
    if length < 1e-6:
        return []
    tangent = (span[0] / length, span[1] / length)
    if any(
        abs((point[0] - points[0][0]) * tangent[1] - (point[1] - points[0][1]) * tangent[0]) > 1e-3
        for point in points
    ):
        return []
    inward = _stringer_inward(sections[0], side)
    normal = FreeCAD.Vector(inward[0], inward[1], 0.0)
    along = FreeCAD.Vector(tangent[0], tangent[1], 0.0)
    origin = FreeCAD.Vector(points[0][0], points[0][1], 0.0)
    projections = [(vertex.Point - origin).dot(along) for vertex in board.Vertexes]
    low = min(projections) - 1.0
    high = max(projections) + 1.0
    bottom = min(board.BoundBox.ZMin, 0.0) - 1.0
    corners = [
        origin + along * u + normal * v + FreeCAD.Vector(0.0, 0.0, bottom)
        for u, v in (
            (low, float(penetration)),
            (high, float(penetration)),
            (high, float(penetration) - float(thickness)),
            (low, float(penetration) - float(thickness)),
        )
    ]
    slab = Part.Face(Part.makePolygon(corners + corners[:1])).extrude(
        FreeCAD.Vector(0.0, 0.0, board.BoundBox.ZMax - bottom + 1.0)
    )
    supports = []
    for step in steps:
        if not step.BoundBox.intersect(slab.BoundBox):
            continue
        housing = step.common(slab)
        if housing.Volume < 1e-6:
            continue
        lowest = min(vertex.Point.z for vertex in housing.Vertexes)
        if lowest < 1.0:
            continue
        # The lower edge climbs along the board, so the underside's end
        # furthest up it comes closest to the edge.
        supports.append(
            max(
                (vertex.Point for vertex in housing.Vertexes if vertex.Point.z < lowest + 1e-6),
                key=lambda point: (point - origin).dot(along),
            )
        )
    newels = [
        (FreeCAD.Vector(point[0], point[1], 0.0) - origin).dot(along)
        for point, section in zip(points, sections)
        if section.profile_at_newel
    ]
    return [
        point
        for point in supports
        if all(abs((point - origin).dot(along) - newel) >= newel_reach for newel in newels)
    ]


def make_short_housed_stringer_shape(
    sections,
    riser_height,
    side,
    thickness,
    penetration,
    start_extension,
    nosing_offset,
    nosing=0.0,
    steps=(),
    newel_reach=0.0,
):
    """Create the short board between the floor and a corner newel.

    A side shorter than one going runs from the first riser to the newel its
    winders converge on. They climb far too steeply along it for a board of
    the regular width to follow their nosings: it would stand almost plumb
    beside the post. The board is a panel instead. It stands on the floor
    over its whole length. Its front end is a plumb cut as high as the
    first tread asks, and from there its upper edge sweeps up over the
    nosings in a quarter of an ellipse, to run level into the newel above
    the last tread whose end it houses.

    ``sections`` end at the corner. ``steps`` are the tread solids, in the
    same coordinates: the board houses those reaching it, except the ones
    starting within ``newel_reach`` of the corner, which the newel carries.
    Winders are clipped where they converge on a newel, so their solids
    tell this, not their nosing lines. ``nosing_offset`` is the timber kept
    above each nosing tip, and in front of those above the first, which
    ``start_extension`` counts from. Returns a null shape when the side is
    not straight.
    """

    if len(sections) < 2:
        return Part.Shape()
    points = [section.left if side == "Left" else section.right for section in sections]
    span = (points[-1][0] - points[0][0], points[-1][1] - points[0][1])
    length = math.hypot(*span)
    if length < 1e-6:
        return Part.Shape()
    direction = (span[0] / length, span[1] / length)
    if any(
        abs((point[0] - points[0][0]) * direction[1] - (point[1] - points[0][1]) * direction[0])
        > 1e-3
        for point in points
    ):
        return Part.Shape()
    distances = [
        (point[0] - points[0][0]) * direction[0] + (point[1] - points[0][1]) * direction[1]
        for point in points
    ]
    clearance = max(float(nosing_offset), 0.0)
    inward = _stringer_inward(sections[0], side)
    along = FreeCAD.Vector(direction[0], direction[1], 0.0)
    start = FreeCAD.Vector(points[0][0], points[0][1], 0.0)
    # Where the first nosing tip reaches the board, and the tread's top
    # there: the first step is always the board's.
    tips = [
        (
            distances[0] - (0.0 if sections[0].profile_nosing_aligned else max(float(nosing), 0.0)),
            _stringer_elevations(sections, riser_height)[0],
        )
    ]
    # In front of the first tip, the timber is the board's start extension,
    # as on the other side of the flight.
    front = min(-max(float(start_extension), 0.0), tips[0][0])
    # The other treads housed: those with an end in the board's thickness.
    corners = [
        start + along * u + FreeCAD.Vector(inward[0], inward[1], 0.0) * v
        for u, v in (
            (front, float(penetration)),
            (length, float(penetration)),
            (length, float(penetration) - float(thickness)),
            (front, float(penetration) - float(thickness)),
        )
    ]
    for step in steps:
        bounds = step.BoundBox
        if bounds.ZMax < tips[0][1] + 1.0:
            continue
        slab = Part.Face(
            Part.makePolygon(
                [corner + FreeCAD.Vector(0.0, 0.0, bounds.ZMin - 1.0) for corner in corners]
                + [corners[0] + FreeCAD.Vector(0.0, 0.0, bounds.ZMin - 1.0)]
            )
        ).extrude(FreeCAD.Vector(0.0, 0.0, bounds.ZLength + 2.0))
        if not bounds.intersect(slab.BoundBox):
            continue
        housing = step.common(slab)
        if housing.Volume < 1e-6:
            continue
        tip = min((vertex.Point - start).dot(along) for vertex in housing.Vertexes)
        if tip < length - float(newel_reach) - 1e-6:
            tips.append((tip, housing.BoundBox.ZMax))
    level = max(z for _u, z in tips) + clearance
    # The shoulder ends level above the highest tip, and starts plumb at the
    # front, as low as the first tread and every tip under it allow.
    crest = max(u for u, z in tips if z + clearance > level - 1e-6)
    reach = crest - front
    shoulder = tips[0][1] + clearance
    for u, z in tips:
        for corner in ((u - clearance, z), (u, z + clearance)):
            if not front < corner[0] < crest:
                continue
            rise = math.sqrt(max(1.0 - ((crest - corner[0]) / reach) ** 2, 0.0))
            if rise < 1.0 - 1e-9:
                shoulder = max(shoulder, (corner[1] - level * rise) / (1.0 - rise))
    shoulder = min(shoulder, level)

    origin = (
        points[0][0] + inward[0] * float(penetration),
        points[0][1] + inward[1] * float(penetration),
    )

    def point(u, z=0.0):
        return FreeCAD.Vector(origin[0] + direction[0] * u, origin[1] + direction[1] * u, z)

    try:
        edges = [Part.makeLine(point(front), point(front, shoulder))]
        if level - shoulder > 1.0 and reach > 1.0:
            # A quarter of an ellipse, exactly: a conic through its ends
            # whose tangents meet above the front.
            sweep = Part.BSplineCurve()
            sweep.buildFromPolesMultsKnots(
                [point(front, shoulder), point(front, level), point(crest, level)],
                [3, 3],
                [0.0, 1.0],
                False,
                2,
                [1.0, math.sqrt(0.5), 1.0],
            )
            edges.append(sweep.toShape())
        else:
            crest = front
            if level - shoulder > 1e-6:
                edges.append(Part.makeLine(point(front, shoulder), point(front, level)))
        if length - crest > 1e-6:
            edges.append(Part.makeLine(point(crest, level), point(length, level)))
        edges.append(Part.makeLine(point(length, level), point(length)))
        edges.append(Part.makeLine(point(length), point(front)))
        result = Part.Face(Part.Wire(edges)).extrude(
            FreeCAD.Vector(-inward[0] * float(thickness), -inward[1] * float(thickness), 0.0)
        )
    except (Part.OCCError, ValueError):
        return Part.Shape()
    if result.isNull() or not result.isValid() or len(result.Solids) != 1:
        return Part.Shape()
    return result


# How deep a corner's level block is where one of its boards stands on the
# floor, and so gives the block no underside of its own.
CORNER_BLOCK_DEPTH = 60.0


def _board_frame(piece, side):
    sections = piece["sections"]
    points = [section.left if side == "Left" else section.right for section in sections]
    span = (points[-1][0] - points[0][0], points[-1][1] - points[0][1])
    length = math.hypot(*span)
    if length < 1e-6:
        return None
    along = FreeCAD.Vector(span[0] / length, span[1] / length, 0.0)
    inward = FreeCAD.Vector(*_stringer_inward(sections[0], side), 0.0)
    # Only a flat board has one plane to meet its neighbour on.
    if abs(inward.dot(along)) > 1e-6 or any(
        abs((point[0] - points[0][0]) * along.y - (point[1] - points[0][1]) * along.x) > 1e-3
        for point in points
    ):
        return None
    inner = float(piece["penetration"])
    outer = inner - float(piece["thickness"])
    return {
        "origin": FreeCAD.Vector(points[0][0], points[0][1], 0.0),
        "along": along,
        "inward": inward,
        "length": length,
        "inner": inner,
        "outer": outer,
        "centre": (inner + outer) / 2.0,
        "thickness": float(piece["thickness"]),
    }


def _board_point(frame, u, v, z=0.0):
    return frame["origin"] + frame["along"] * u + frame["inward"] * v + FreeCAD.Vector(0.0, 0.0, z)


def _band_corner(first, first_offset, second, second_offset):
    """Return where two boards' faces, at the given offsets, cross in plan."""

    a, b = first["inward"], second["inward"]
    rhs_a = first_offset + first["origin"].dot(a)
    rhs_b = second_offset + second["origin"].dot(b)
    determinant = a.x * b.y - a.y * b.x
    return FreeCAD.Vector(
        (rhs_a * b.y - rhs_b * a.y) / determinant,
        (a.x * rhs_b - b.x * rhs_a) / determinant,
        0.0,
    )


def _edge_at(shape, point, lower=False):
    """Return the board's upper, or ``lower``, edge height above ``point``."""

    probe = Part.makeLine(
        FreeCAD.Vector(point.x, point.y, shape.BoundBox.ZMin - 1.0),
        FreeCAD.Vector(point.x, point.y, shape.BoundBox.ZMax + 1.0),
    )
    crossing = shape.common(probe)
    if not crossing.Edges:
        return None
    return crossing.BoundBox.ZMin if lower else crossing.BoundBox.ZMax


# Side outlines of the boards being joined, by shape: probing an outline is
# far cheaper than a boolean with the whole solid.
_SIDE_OUTLINES = {}


def _side_outline(shape, frame):
    """Return a flat board's side outline as ``(u, z)`` segments, or ``None``.

    Before joinery a board is its side profile extruded through its
    thickness, so its edges are the same anywhere across it. Lines are kept
    exact; curves are sampled to a thousandth of a millimetre.
    """

    key = (
        shape.hashCode(),
        round(frame["origin"].x, 6),
        round(frame["origin"].y, 6),
        round(frame["along"].x, 9),
        round(frame["along"].y, 9),
    )
    if key in _SIDE_OUTLINES:
        return _SIDE_OUTLINES[key]
    plane = frame["origin"].dot(frame["inward"]) + frame["inner"]
    side = None
    for face in shape.Faces:
        if face.Surface.TypeId != "Part::GeomPlane":
            continue
        normal = face.normalAt(0.0, 0.0)
        if abs(abs(normal.dot(frame["inward"])) - 1.0) > 1e-9:
            continue
        if abs(face.Vertexes[0].Point.dot(frame["inward"]) - plane) < 1e-6:
            side = face
            break
    segments = None
    if side is not None and len(side.Wires) == 1:
        segments = []
        for edge in side.OuterWire.Edges:
            if edge.Curve.TypeId in ("Part::GeomLine", "Part::GeomLineSegment"):
                points = [vertex.Point for vertex in edge.Vertexes]
            else:
                points = edge.discretize(Deflection=1e-3)
            coordinates = [
                ((point - frame["origin"]).dot(frame["along"]), point.z) for point in points
            ]
            segments.extend(zip(coordinates, coordinates[1:]))
    if len(_SIDE_OUTLINES) > 64:
        _SIDE_OUTLINES.clear()
    _SIDE_OUTLINES[key] = segments
    return segments


def _board_edge(shape, frame, u, lower=False):
    """Return a flat board's upper, or ``lower``, edge height at ``u``."""

    segments = _side_outline(shape, frame)
    if segments is None:
        return _edge_at(shape, _board_point(frame, u, frame["centre"]), lower)
    heights = []
    for (u0, z0), (u1, z1) in segments:
        if min(u0, u1) - 1e-9 <= u <= max(u0, u1) + 1e-9:
            if abs(u1 - u0) < 1e-9:
                heights.extend((z0, z1))
            else:
                heights.append(z0 + (z1 - z0) * (u - u0) / (u1 - u0))
    if not heights:
        return None
    return min(heights) if lower else max(heights)


def _reshaped(shape, removed=None, added=None):
    """Cut ``removed`` from a board and fuse ``added``, or leave it.

    Booleans over near-coincident faces can fail silently and return an
    empty or gutted shape. A corner block only trims a little or adds a
    little, so a result losing more than ``removed`` is retried with a fuzzy
    tolerance, then dropped. The volume of a board with curved edges is
    only integrated to some ten-thousandth of itself, which the comparison
    allows for.
    """

    def checked(operation, other, lost):
        for tolerance in (None, 1e-4):
            result = operation(other) if tolerance is None else operation([other], tolerance)
            slack = 1e-4 * shape_volume + 1.0
            if not result.isNull() and result.Volume >= shape_volume - lost - slack:
                return result
        return None

    result = shape
    if removed is not None:
        shape_volume = result.Volume
        cut = checked(result.cut, removed, result.common(removed).Volume)
        if cut is None:
            return shape
        result = cut
    if added is not None:
        shape_volume = result.Volume
        fused = checked(result.fuse, added, 0.0)
        if fused is None:
            return shape
        result = fused
    return result


def _outer_corner(first, second, side):
    """Return the block two housed boards turning an outer corner meet on.

    ``first`` and ``second`` are consecutive boards of one stair side. The
    block is the square where their bands cross: its plan corners as
    ``square``, the boards' ``frames``, and where the first board enters it
    and the second one leaves it, along each. ``None`` when the boards do
    not turn an outer corner together; on the inner side of a turn the
    corner is the newel's, which carries the joint.
    """

    frame_a, frame_b = _board_frame(first, side), _board_frame(second, side)
    if frame_a is None or frame_b is None:
        return None
    turn = frame_a["along"].cross(frame_b["along"]).z
    if abs(turn) < math.sin(math.radians(10.0)):
        return None
    if (turn > 0.0) == (side == "Left"):
        return None
    square = [
        _band_corner(frame_a, offset_a, frame_b, offset_b)
        for offset_a, offset_b in (
            (frame_a["inner"], frame_b["inner"]),
            (frame_a["inner"], frame_b["outer"]),
            (frame_a["outer"], frame_b["outer"]),
            (frame_a["outer"], frame_b["inner"]),
        )
    ]
    along_a = [(point - frame_a["origin"]).dot(frame_a["along"]) for point in square]
    along_b = [(point - frame_b["origin"]).dot(frame_b["along"]) for point in square]
    # The square must end the first board and start the second one.
    reach = 3.0 * max(frame_a["thickness"], frame_b["thickness"])
    if abs(frame_a["length"] - sum(along_a) / 4.0) > reach or abs(sum(along_b) / 4.0) > reach:
        return None
    return {
        "frames": (frame_a, frame_b),
        "square": square,
        "enter": min(along_a),
        "leave": max(along_b),
    }


def _corner_levels(shapes, corner, halfway=False):
    """Return a corner block's ``(top, underside, level underside)``.

    ``shapes`` are the two boards, whose edges already run level into the
    block: it stands as high as the higher upper edge and as deep as the
    lower underside, so that neither board loses the timber over its
    nosings or under its housings. Boards not built for the block,
    ``halfway``, meet between their edges instead. A board standing on the
    floor at the corner is a base the other one simply meets: the block
    then has no level underside. ``None`` when an edge cannot be probed.
    """

    frame_a, frame_b = corner["frames"]

    def edge_at(shape, frame, edge, direction, lower=False):
        # Probe just off the square, on the board's own side, and carry its
        # pitch back to the edge.
        near = _board_edge(shape, frame, edge + direction, lower)
        far = _board_edge(shape, frame, edge + 2.0 * direction, lower)
        return None if near is None or far is None else 2.0 * near - far

    top_a = edge_at(shapes[0], frame_a, corner["enter"], -1.0)
    top_b = edge_at(shapes[1], frame_b, corner["leave"], 1.0)
    bottom_a = edge_at(shapes[0], frame_a, corner["enter"], -1.0, True)
    bottom_b = edge_at(shapes[1], frame_b, corner["leave"], 1.0, True)
    if None in (top_a, top_b, bottom_a, bottom_b):
        return None
    level = (top_a + top_b) / 2.0 if halfway else max(top_a, top_b)
    level_underside = min(bottom_a, bottom_b) > 1.0
    if not level_underside:
        return level, level - CORNER_BLOCK_DEPTH, False
    return level, (bottom_a + bottom_b) / 2.0 if halfway else min(bottom_a, bottom_b), True


def _join_corner(shapes, corner, level, floor, level_underside):
    """Give two boards their common corner block, between ``floor`` and
    ``level``: one board owns it, the other only meets it, face to face."""

    square = corner["square"]

    def prism(bottom, height):
        outline = [point + FreeCAD.Vector(0.0, 0.0, bottom) for point in square]
        return Part.Face(Part.makePolygon(outline + outline[:1])).extrude(
            FreeCAD.Vector(0.0, 0.0, height)
        )

    shape_a, shape_b = shapes
    top = max(shape_a.BoundBox.ZMax, shape_b.BoundBox.ZMax, level) + 10.0
    base = min(shape_a.BoundBox.ZMin, shape_b.BoundBox.ZMin, floor) - 10.0
    outside = prism(level, top - level)
    if level_underside:
        outside = outside.fuse(prism(base, floor - base))
    block = prism(floor, level - floor)
    whole = prism(base, top - base)
    if shape_a.common(block).Volume >= shape_b.common(block).Volume:
        shape_a = _reshaped(shape_a, outside, block)
        shape_b = _reshaped(shape_b, whole)
    else:
        shape_b = _reshaped(shape_b, outside, block)
        shape_a = _reshaped(shape_a, whole)
    return shape_a.removeSplitter(), shape_b.removeSplitter()


def _sound(pair, fused):
    """Tell whether two joined boards are sound solids, ``fused`` into one
    part afterwards or not: curved faces meeting at a block can make that
    fuse self-intersect."""

    try:
        (pair[0].fuse(pair[1]) if fused else pair[0]).check(True)
        if not fused:
            pair[1].check(True)
    except Exception:
        return False
    return True


def level_board_corners(pieces, side):
    """Join the housed boards of one stair side on level blocks at the
    outer corners they turn.

    ``pieces`` are the boards in order, each a dict with the board
    ``shape``, its ``sections``, ``thickness`` and ``penetration``, whether
    it is ``housed``, the ``part`` it belongs to, and ``build``, which
    returns the board's shape for a start and an end ``EdgeBlock``. Their
    shapes are replaced by the joined ones.

    The stair rises through a corner, so two sloping boards cannot be
    flush across their thickness there. The square where their bands cross
    becomes one level block, and each board is rebuilt with its edges
    running level into it: not a ramp added to a finished edge, but the
    one fair curve of that edge, sprung to arrive there level. The block
    takes the higher of the two upper edges and the lower of the two
    undersides. Where boards cannot be rebuilt or joined soundly, they
    keep their own edges and meet the block halfway between them.
    """

    corners = {}
    for index, (first, second) in enumerate(zip(pieces, pieces[1:])):
        if first["housed"] and second["housed"]:
            corner = _outer_corner(first, second, side)
            if corner is not None:
                corners[index] = corner
    if not corners:
        return
    plain = [piece["shape"] for piece in pieces]

    def plan(frame, u):
        point = _board_point(frame, u, 0.0)
        return (point.x, point.y)

    def built(active, levels):
        """Return every board built for its ``active`` corners, or the
        corner whose board cannot be."""

        shapes = list(plain)
        for index, piece in enumerate(pieces):
            start = end = None
            if index - 1 in active:
                corner = corners[index - 1]
                start = EdgeBlock(
                    plan(corner["frames"][1], corner["leave"]),
                    *levels.get(index - 1, (None, None)),
                )
            if index in active:
                corner = corners[index]
                end = EdgeBlock(
                    plan(corner["frames"][0], corner["enter"]),
                    *levels.get(index, (None, None)),
                )
            if start is None and end is None:
                continue
            shape = piece["build"](start, end)
            if shape.isNull() or not shape.isValid() or len(shape.Solids) != 1:
                return None, index if end is not None else index - 1
            shapes[index] = shape
        return shapes, None

    def levelled(active):
        """Return the boards joined at their ``active`` corners, or the
        corner that cannot be."""

        shapes, failed = built(active, {})
        if shapes is None:
            return None, failed
        found = {}
        for index in sorted(active):
            found[index] = _corner_levels(shapes[index : index + 2], corners[index])
            if found[index] is None:
                return None, index
        shapes, failed = built(
            active,
            {
                index: (level, floor if level_underside else False)
                for index, (level, floor, level_underside) in found.items()
            },
        )
        if shapes is None:
            return None, failed
        for index in sorted(active):
            pair = _join_corner(shapes[index : index + 2], corners[index], *found[index])
            if not _sound(pair, pieces[index]["part"] is pieces[index + 1]["part"]):
                return None, index
            shapes[index : index + 2] = pair
        return shapes, None

    active = set(corners)
    shapes = None
    while active and shapes is None:
        shapes, failed = levelled(active)
        active.discard(failed)
    if shapes is None:
        shapes = list(plain)
    for index in sorted(set(corners) - active):
        found = _corner_levels(shapes[index : index + 2], corners[index], halfway=True)
        if found is not None:
            shapes[index : index + 2] = _join_corner(
                shapes[index : index + 2], corners[index], *found
            )
    for piece, shape in zip(pieces, shapes):
        piece["shape"] = shape


def make_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance=0.0,
    throat=0.0,
):
    """Create one notched board supporting the undersides of the treads.

    ``throat`` is the timber a flat board keeps under each of its cuts, see
    ``notched_stringer_throat``.
    """

    if len(sections) < 2:
        return Part.Shape()
    runs = _stringer_section_runs(sections)
    if any(section.level_to_next for section in sections[:-1]):
        shapes = [
            _make_notched_stringer_run(
                run,
                riser_height,
                step_thickness,
                side,
                thickness,
                width,
                lateral_offset,
                end_extension if index == len(runs) - 1 else 0.0,
                riser_clearance,
                throat,
            )
            for index, run in enumerate(runs)
        ]
        if not shapes:
            return Part.Shape()
        return shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
    return _make_notched_stringer_run(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
        throat,
    )


def _make_circular_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
):
    """Cut a sawtooth top into one exact helical annular board."""

    circular = _circular_stringer_data(
        sections,
        side,
        thickness,
        lateral_offset,
        False,
    )
    if circular is None:
        return None
    profile = circular["profile"]
    angles = circular["angles"]
    path_radius = circular["path_radius"]
    direction = 1.0 if profile.sweep > 0.0 else -1.0
    step_thickness = max(float(step_thickness), 0.0)
    width = max(float(width), step_thickness + 0.01)
    riser_clearance = max(float(riser_clearance), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    bottoms = [max(elevation - vertical_width, 0.0) for elevation in elevations]
    cell_count = len(sections) - 1

    def shifted_boundary(index):
        if riser_clearance < 1e-9 or index >= cell_count:
            return angles[index]
        available = abs(angles[index + 1] - angles[index]) * path_radius
        clearance = min(riser_clearance, max(available - 0.01, 0.0))
        return angles[index] + direction * clearance / path_radius

    main_top = max(elevation - step_thickness for elevation in elevations[:-1])
    base = _make_helical_annular_solid(
        profile,
        main_top,
        bottoms[0],
        bottoms[-1],
    )
    if base is None:
        return None

    envelope_bottom = min(bottoms) - max(width, float(riser_height), 1.0)
    envelopes = []

    def add_envelope(start_angle, end_angle, top):
        if abs(end_angle - start_angle) < 1e-8:
            return
        sector = _circular_profile_between(profile, start_angle, end_angle)
        face = _annular_sector_face(sector, envelope_bottom)
        envelopes.append(face.extrude(FreeCAD.Vector(0.0, 0.0, top - envelope_bottom)))

    first_start = shifted_boundary(0)
    first_riser_index = int(getattr(sections[0], "riser_index", 0))
    if first_riser_index > 1 and abs(first_start - angles[0]) > 1e-8:
        add_envelope(
            angles[0],
            first_start,
            elevations[0] - step_thickness - float(riser_height),
        )
    for index in range(cell_count):
        start_angle = shifted_boundary(index)
        end_angle = shifted_boundary(index + 1) if index + 1 < cell_count else angles[index + 1]
        add_envelope(
            start_angle,
            end_angle,
            elevations[index] - step_thickness,
        )
    if not envelopes:
        return None
    envelope = envelopes[0]
    for addition in envelopes[1:]:
        envelope = envelope.fuse(addition)
    result = base.common(envelope).removeSplitter()
    if not result.isValid() or len(result.Solids) != 1:
        return None

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        extension_sweep = direction * end_extension / path_radius
        extension_profile = _circular_profile_between(
            profile,
            angles[-1],
            angles[-1] + extension_sweep,
        )
        extension = _make_helical_band_solid(
            extension_profile,
            elevations[-1] - step_thickness,
            elevations[-1] - step_thickness + slope * end_extension,
            bottoms[-1],
            max(bottoms[-1] + slope * end_extension, 0.0),
        )
        if extension is not None:
            try:
                extended = result.fuse(extension).removeSplitter()
            except (Part.OCCError, ValueError):
                extended = Part.Shape()
            if not extended.isNull() and extended.isValid() and len(extended.Solids) == 1:
                result = extended
    return result


def _planar_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
    throat=0.0,
):
    """Extrude one sawtooth side profile for a straight-flight board.

    Its lower edge keeps ``throat`` of timber under every cut.
    """

    first_rail = sections[0].left if side == "Left" else sections[0].right
    tangent_length = math.hypot(*sections[0].tangent)
    if tangent_length < 1e-9:
        return None
    tangent = (
        sections[0].tangent[0] / tangent_length,
        sections[0].tangent[1] / tangent_length,
    )
    plan_normal = (-tangent[1], tangent[0])
    for section in sections:
        section_length = math.hypot(*section.tangent)
        if section_length < 1e-9:
            return None
        section_tangent = (
            section.tangent[0] / section_length,
            section.tangent[1] / section_length,
        )
        if (
            abs(tangent[0] * section_tangent[1] - tangent[1] * section_tangent[0]) > 1e-7
            or tangent[0] * section_tangent[0] + tangent[1] * section_tangent[1] < 0.0
        ):
            return None
        rail = section.left if side == "Left" else section.right
        if (
            abs(
                (rail[0] - first_rail[0]) * plan_normal[0]
                + (rail[1] - first_rail[1]) * plan_normal[1]
            )
            > 1e-5
        ):
            return None

    thickness = max(float(thickness), 0.01)
    width = max(float(width), float(step_thickness) + 0.01)
    step_thickness = max(float(step_thickness), 0.0)
    lateral_offset = float(lateral_offset)
    riser_clearance = max(float(riser_clearance), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)

    def rail_distance(section):
        rail = section.left if side == "Left" else section.right
        return (rail[0] - first_rail[0]) * tangent[0] + (rail[1] - first_rail[1]) * tangent[1]

    distances = [rail_distance(section) for section in sections]
    # A board with no run, e.g. across a side-entry landing, has no pitch.
    level = max(distances) - min(distances) < 1.0

    inward = _stringer_inward(sections[0], side)

    def point(section, elevation, distance=0.0):
        rail = section.left if side == "Left" else section.right
        return FreeCAD.Vector(
            rail[0] + tangent[0] * distance + inward[0] * lateral_offset,
            rail[1] + tangent[1] * distance + inward[1] * lateral_offset,
            elevation,
        )

    def run_length(first, second):
        first_rail_point = first.left if side == "Left" else first.right
        second_rail_point = second.left if side == "Left" else second.right
        return max(
            (second_rail_point[0] - first_rail_point[0]) * tangent[0]
            + (second_rail_point[1] - first_rail_point[1]) * tangent[1],
            0.0,
        )

    def clearance_after(section_index):
        if riser_clearance < 1e-9 or section_index + 1 >= len(sections):
            return 0.0
        available = run_length(sections[section_index], sections[section_index + 1])
        return min(riser_clearance, max(available - 0.01, 0.0))

    points = []
    # The inner corner of each cut, along the board: where it comes
    # closest to the lower edge.
    cuts = []

    def append(point_to_add):
        if not points or (point_to_add - points[-1]).Length > 1e-7:
            points.append(point_to_add)

    first_top = elevations[0] - step_thickness
    first_riser_index = int(getattr(sections[0], "riser_index", 0))
    start_clearance = clearance_after(0)
    shifted_start = first_riser_index <= 1 and start_clearance > 1e-9
    if shifted_start:
        append(point(sections[0], first_top, start_clearance))
    else:
        append(point(sections[0], first_top))
        if start_clearance > 1e-9:
            lower_top = first_top - float(riser_height)
            points[-1] = point(sections[0], lower_top)
            append(point(sections[0], lower_top, start_clearance))
            append(point(sections[0], first_top, start_clearance))
            cuts.append((distances[0] + start_clearance, lower_top))

    cell_count = len(sections) - 1
    for index, rear in enumerate(sections[1:]):
        tread_bottom = elevations[index] - step_thickness
        append(point(rear, tread_bottom))
        if index + 1 >= cell_count:
            continue
        clearance = clearance_after(index + 1)
        if clearance > 1e-9:
            append(point(rear, tread_bottom, clearance))
        next_tread_bottom = elevations[index + 1] - step_thickness
        append(point(rear, next_tread_bottom, clearance))
        cuts.append((distances[index + 1] + clearance, tread_bottom))

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        end_top = elevations[-1] - step_thickness
        append(point(sections[-1], end_top))
        append(
            point(
                sections[-1],
                end_top + slope * end_extension,
                end_extension,
            )
        )

    origin = (
        first_rail[0] + inward[0] * lateral_offset,
        first_rail[1] + inward[1] * lateral_offset,
    )
    start = distances[0] + (start_clearance if shifted_start else 0.0)
    end = distances[-1] + end_extension
    try:
        if level:
            bottoms = [max(elevation - width, 0.0) for elevation in elevations]
            bottom_points = [point(section, bottom) for section, bottom in zip(sections, bottoms)]
            if shifted_start:
                bottom_points[0] = point(sections[0], bottoms[-1], start_clearance)
            if end_extension > 1e-9:
                bottom_points.append(point(sections[-1], bottoms[-1], end_extension))
            for bottom_point in reversed(bottom_points):
                append(bottom_point)
            if len(points) < 3:
                return None
            if (points[0] - points[-1]).Length > 1e-7:
                points.append(points[0])
            side_face = Part.Face(Part.makePolygon(points))
        elif len(points) < 2:
            return None
        else:
            # The lower edge keeps the same timber under every cut; where
            # it dips under the floor it is cut level rather than bending
            # the board.
            upper_line, lower_line = notched_board_edges(
                distances,
                elevations,
                width,
                start,
                end,
                cuts,
                throat,
                [bool(section.profile_at_newel) for section in sections],
            )
            lower = _edges_above_floor(origin, tangent, upper_line, lower_line, start, end)
            lower_start = lower[0].Vertexes[0].Point
            lower_end = lower[-1].Vertexes[-1].Point
            returning = []
            for edge in reversed(lower):
                edge = edge.copy()
                edge.reverse()
                returning.append(edge)
            side_face = Part.Face(
                Part.Wire(
                    [
                        *Part.makePolygon(points).Edges,
                        Part.makeLine(points[-1], lower_end),
                        *returning,
                        Part.makeLine(lower_start, points[0]),
                    ]
                )
            )
        result = side_face.extrude(
            FreeCAD.Vector(
                inward[0] * thickness,
                inward[1] * thickness,
                0.0,
            )
        )
    except (Part.OCCError, ValueError):
        return None
    if not result.isValid() or len(result.Solids) != 1:
        return None
    if result.BoundBox.ZMin < -1e-6:
        bounds = result.BoundBox
        floor = Part.makeBox(
            bounds.XLength + 2.0,
            bounds.YLength + 2.0,
            bounds.ZMax + 1.0,
            FreeCAD.Vector(bounds.XMin - 1.0, bounds.YMin - 1.0, 0.0),
        )
        try:
            trimmed = result.common(floor)
        except Part.OCCError:
            trimmed = Part.Shape()
        if not trimmed.isNull() and trimmed.isValid() and len(trimmed.Solids) == 1:
            result = trimmed
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def _make_notched_stringer_run(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
    throat=0.0,
):
    """Create one continuous notched-stringer run."""

    circular = _make_circular_notched_stringer_shape(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
    )
    if circular is not None:
        return circular

    planar = _planar_notched_stringer_shape(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
        throat,
    )
    if planar is not None:
        return planar

    thickness = max(float(thickness), 0.01)
    width = max(float(width), float(step_thickness) + 0.01)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    bottoms = [max(elevation - vertical_width, 0.0) for elevation in elevations]
    solids = []
    for index, (front, rear) in enumerate(zip(sections, sections[1:])):
        tread_bottom = elevations[index] - float(step_thickness)
        front_wire = _stringer_cross_section(
            front,
            side,
            bottoms[index],
            tread_bottom,
            thickness,
            float(lateral_offset),
            False,
        )
        rear_wire = _stringer_cross_section(
            rear,
            side,
            bottoms[index + 1],
            tread_bottom,
            thickness,
            float(lateral_offset),
            False,
        )
        solids.append(Part.makeLoft([front_wire, rear_wire], True, True))

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        end = sections[-1]
        extended = _translated_section(end, end_extension)
        start_top = elevations[-1] - float(step_thickness)
        start_wire = _stringer_cross_section(
            end,
            side,
            bottoms[-1],
            start_top,
            thickness,
            float(lateral_offset),
            False,
        )
        end_wire = _stringer_cross_section(
            extended,
            side,
            max(bottoms[-1] + slope * end_extension, 0.0),
            start_top + slope * end_extension,
            thickness,
            float(lateral_offset),
            False,
        )
        solids.append(Part.makeLoft([start_wire, end_wire], True, True))

    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()
