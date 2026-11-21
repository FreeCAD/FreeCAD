# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stair footprints, plan partitions, and plan geometry."""

import bisect
from dataclasses import replace
import math

import FreeCAD
import Part

from .geometry_core import (
    BalancedSection,
    _cross,
    _shifted,
)

from .geometry_tangent import (
    _primitive_point,
    _primitive_tangent,
    _tangent_junction_modes,
    _tangent_path_primitives,
)

from .geometry_winders import (
    _safe_angle_tangent,
)


def corner_post_sections(sections, entry_direction, exit_direction, balanced=None, reach=0.0):
    """Terminate wooden endpoint winders at the inner newel.

    The monotone Herse fit separates coincident inner endpoints to keep a
    positive going along both rails. At a side entrance this produces a
    nearly vertical stringer. The newel supports these fan treads instead;
    retain the outer nosings and the perpendicular treads between the turns.

    With the ``balanced`` sections, as they were before being fitted to the
    footprint (which spreads risers meeting at one point of a side), only
    the fan is moved: going away from the newel, the risers whose balanced
    line meets the inner side in the opening, or less than ``reach`` (the
    newel's size) beyond the previous one, die into the newel. The first
    riser leaving at least that much inner going is housed in the board,
    and so are the ones after it: they keep their balance. Without
    ``balanced``, every riser of that half of the flight is moved.
    """
    if len(sections) < 3:
        return list(sections)
    result = list(sections)
    for at_start, direction in ((True, entry_direction), (False, exit_direction)):
        direction = str(direction).lower()
        if direction == "straight":
            continue
        side = "left" if "left" in direction else "right"
        endpoint = sections[0 if at_start else -1]
        corner = getattr(endpoint, side)
        outer = endpoint.right if side == "left" else endpoint.left
        axis_length = math.dist(corner, outer)
        if axis_length < 1e-7:
            continue
        into_flight = tuple((corner[i] - outer[i]) / axis_length for i in (0, 1))
        indices = [
            i for i, section in enumerate(sections) if section.flight_index == endpoint.flight_index
        ]
        if not at_start:
            indices.reverse()
        # Retain a central section even when the two balancing zones meet.
        halfway = (sections[indices[0]].station + sections[indices[-1]].station) / 2.0
        previous_landing = 0.0
        for index in indices[1:]:
            section = result[index]
            if section.locked_to_flight or (
                section.station >= halfway if at_start else section.station <= halfway
            ):
                break
            at_face = False
            if balanced is not None:
                landing = _side_landing(balanced[index], corner, into_flight)
                if landing is not None and landing - previous_landing >= max(float(reach), 1e-7):
                    # The balanced riser meets the inner board far enough
                    # from the fan to be housed there, as do the ones after.
                    break
                if landing is not None:
                    previous_landing = max(previous_landing, landing)
                    # Just past the post, it ends against the post's face.
                    at_face = landing > _newel_half(reach) + 1e-7
            outer = section.right if side == "left" else section.left
            advance = sum((outer[i] - corner[i]) * into_flight[i] for i in (0, 1))
            # Once the outer nosing has passed the newel, this is a straight
            # tread. Continuing to pin its inner end to the post reverses
            # the fan angle. Preserve its outer station and square the tread.
            along = _newel_half(reach) if at_face else max(advance, 0.0)
            inner = tuple(corner[i] + along * into_flight[i] for i in (0, 1))
            left = inner if side == "left" else section.left
            right = inner if side == "right" else section.right
            width = math.dist(left, right)
            if width < 1e-7:
                break
            result[index] = replace(
                section,
                left=left,
                right=right,
                width=width,
                center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
                tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
                locked_to_flight=advance > 1e-7 and not at_face,
            )
    return result


def newel_post_square(center, direction, size):
    """Return a newel post's plan square: centre, side ``direction``, size."""

    length = math.hypot(*direction)
    along = (direction[0] / length, direction[1] / length)
    across = (-along[1], along[0])
    half = max(float(size), 0.01) / 2.0
    return [
        (
            center[0] + along[0] * half * a + across[0] * half * b,
            center[1] + along[1] * half * a + across[1] * half * b,
        )
        for a, b in ((-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0))
    ]


def newel_post_plan(sections, footprint, squares, posts=None):
    """Return the tread plan's sections and outline around newel posts.

    ``squares`` are the outlines the steps stop at inside the newel posts,
    ``posts`` the posts' own, by default the same. The squares are cut out
    of the ``footprint``. The risers ending at a post, within its own
    outline, are spread evenly along the faces of its square standing in
    the stair, between the stair's sides it meets, in their order: their
    treads each take a share of the post, as they take a share of a board
    along the stair's sides. A first or last section ending at a post ends
    where its faces meet the stair's side, and its tread, opening there
    onto the floor beside the post, takes no share: the riser next to it
    ends there too. The other sections are unchanged.
    Returns ``(sections, footprint)`` unchanged when no riser meets a post
    or the plan does not partition the notched outline.
    """

    if not squares or len(sections) < 3 or footprint.isNull():
        return sections, footprint
    posts = list(posts) if posts is not None else list(squares)
    elevation = footprint.BoundBox.ZMin
    cutters = [_horizontal_face(_without_duplicate_points(square), elevation) for square in squares]
    try:
        notched = footprint
        for cutter in cutters:
            notched = notched.cut(cutter)
        notched = max(notched.Faces, key=lambda face: face.Area)
    except (Part.OCCError, ValueError):
        return sections, footprint
    if notched.Area >= footprint.Area - 1e-6:
        return sections, footprint
    wire = notched.OuterWire
    vertices = [(vertex.Point.x, vertex.Point.y) for vertex in wire.OrderedVertexes]

    def on_square(point, square):
        return any(
            _segment_distance(point, a, b) < 1e-6 for a, b in zip(square, square[1:] + square[:1])
        )

    result = list(sections)
    changed = False
    last = len(sections) - 1
    for square, post in zip(squares, posts):
        # The square's faces in the stair: the run of the notched outline
        # along the square, from one of the stair's sides to the other.
        count = len(vertices)
        along = [
            on_square(
                (
                    (vertices[i][0] + vertices[(i + 1) % count][0]) / 2.0,
                    (vertices[i][1] + vertices[(i + 1) % count][1]) / 2.0,
                ),
                square,
            )
            for i in range(count)
        ]
        if not any(along) or all(along):
            continue
        first = next(i for i in range(count) if along[i] and not along[i - 1])
        path = [vertices[first]]
        index = first
        while along[index % count]:
            index += 1
            path.append(vertices[index % count])
        ends = [
            (index, side)
            for index, section in enumerate(sections)
            for side in ("left", "right")
            if _in_square(getattr(section, side), post)
        ]
        if not ends:
            continue
        inner = sorted((index, side) for index, side in ends if 0 < index < last)
        placed = {}
        if inner:
            # Run from the side the risers come from to the one they go to.
            before = getattr(sections[inner[0][0] - 1], inner[0][1])
            after = getattr(sections[inner[-1][0] + 1], inner[-1][1])
            if math.dist(path[-1], before) + math.dist(path[0], after) < math.dist(
                path[0], before
            ) + math.dist(path[-1], after):
                path.reverse()
            length = sum(math.dist(a, b) for a, b in zip(path, path[1:]))
            opens_before = inner[0][0] == 1 and (0, inner[0][1]) in ends
            opens_after = inner[-1][0] == last - 1 and (last, inner[-1][1]) in ends
            shares = max(len(inner) + 1 - opens_before - opens_after, 1)
            first_share = 0 if opens_before else 1
            placed = {
                (index, side): _polyline_point(
                    path, min(length * (rank + first_share) / shares, length)
                )
                for rank, (index, side) in enumerate(inner)
            }
        for index, side in ends:
            if (index, side) in placed:
                continue
            if inner:
                placed[(index, side)] = path[0] if index == 0 else path[-1]
            else:
                end = getattr(sections[index], side)
                placed[(index, side)] = min(
                    (path[0], path[-1]), key=lambda point: math.dist(point, end)
                )
        for (index, side), point in placed.items():
            current = result[index]
            if math.dist(point, getattr(current, side)) < 1e-9:
                continue
            left = point if side == "left" else current.left
            right = point if side == "right" else current.right
            width = math.dist(left, right)
            if width < 1e-7:
                continue
            result[index] = replace(
                current,
                left=left,
                right=right,
                width=width,
                center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
                tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
                locked_to_flight=False,
                post_ends=tuple(sorted(set(current.post_ends) | {side})),
            )
            changed = True
    if not changed:
        return sections, footprint
    try:
        faces = balanced_tread_faces(result, notched)
    except ValueError:
        return sections, footprint
    if not balanced_partition_is_valid(faces, notched, len(result) - 1):
        return sections, footprint
    return result, notched


def _segment_distance(point, start, end):
    delta = (end[0] - start[0], end[1] - start[1])
    squared = delta[0] * delta[0] + delta[1] * delta[1]
    ratio = 0.0
    if squared > 1e-18:
        ratio = ((point[0] - start[0]) * delta[0] + (point[1] - start[1]) * delta[1]) / squared
        ratio = min(max(ratio, 0.0), 1.0)
    return math.dist(point, (start[0] + delta[0] * ratio, start[1] + delta[1] * ratio))


def _polyline_point(points, distance):
    for start, end in zip(points, points[1:]):
        length = math.dist(start, end)
        if distance <= length or end is points[-1]:
            ratio = min(max(distance / length, 0.0), 1.0) if length > 1e-12 else 0.0
            return (start[0] + (end[0] - start[0]) * ratio, start[1] + (end[1] - start[1]) * ratio)
        distance -= length
    return points[-1]


def _newel_half(reach):
    """Return how far a newel of size ``reach`` reaches from its corner.

    The post stands centred on the corner of the turn.
    """

    return max(float(reach), 0.0) / 2.0


def inner_corner_newel_sections(sections, footprint, reach):
    """End the winders converging on an inner corner at the newel there.

    A turn's inner corner carries a newel, centred on it. Going away from
    the corner along either side, the risers meeting the side less than
    ``reach`` (the newel's size) beyond the previous one are carried by the
    newel, as the stringers carry their nosings: within the post they end
    at the corner, past it against the post's face. The first riser leaving
    at least that much inner going is housed in the board, and so are the
    ones after it. The first and last sections are left alone.
    """

    if len(sections) < 3 or footprint.isNull() or reach <= 1e-7:
        return list(sections)
    boundary = _boundary_data(footprint)
    vertices = boundary["vertices"]
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    result = list(sections)
    count = len(vertices)
    for index, corner in enumerate(vertices):
        previous = vertices[index - 1]
        following = vertices[(index + 1) % count]
        turn = (corner[0] - previous[0]) * (following[1] - corner[1]) - (
            corner[1] - previous[1]
        ) * (following[0] - corner[0])
        if turn * orientation > -1e-7:
            # Not a reflex corner: no newel stands inside the turn.
            continue
        for neighbour in (previous, following):
            length = math.dist(corner, neighbour)
            if length < 1e-7:
                continue
            direction = tuple((neighbour[i] - corner[i]) / length for i in (0, 1))
            landings = []
            for section_index in range(1, len(result) - 1):
                for side in ("left", "right"):
                    point = getattr(result[section_index], side)
                    offset = (point[0] - corner[0], point[1] - corner[1])
                    along = offset[0] * direction[0] + offset[1] * direction[1]
                    across = offset[0] * direction[1] - offset[1] * direction[0]
                    if abs(across) < 1e-6 and 1e-7 < along <= length:
                        landings.append((along, section_index, side))
            previous_landing = 0.0
            for along, section_index, side in sorted(landings):
                if along - previous_landing >= reach:
                    break
                previous_landing = along
                section = result[section_index]
                half = _newel_half(reach)
                end = (
                    corner
                    if along <= half + 1e-7
                    else tuple(corner[i] + direction[i] * half for i in (0, 1))
                )
                left = end if side == "left" else section.left
                right = end if side == "right" else section.right
                width = math.dist(left, right)
                if width < 1e-7:
                    continue
                result[section_index] = replace(
                    section,
                    left=left,
                    right=right,
                    width=width,
                    center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
                    tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
                    locked_to_flight=False,
                )
    return result


def walking_line_points(sections, newels=(), widths=None, offset=None):
    """Return the walking line across the ``sections`` as a plan polyline.

    The walking line runs in the middle of the steps, or ``offset`` from
    the inner side, the shorter of the sections' sides, where that is
    nearer: on steps wider than twice the offset. Along each stretch of the
    inner side the steps' width is the narrowest, square to it, of the
    sections ending on it, off a newel, and lying across it to its outer
    side. A stretch without one, as one only a turn's risers leave, takes
    the width of the stretch it runs on straight from, or else of the next
    one, or the last one, or without any its flight's width, from
    ``widths``. Where the inner side turns, about a newel or an inner
    corner, the walking line runs round it on an arc, easing from one
    stretch's distance to the next so it meets both lines square. A section
    end on one of the ``newels`` plan squares is taken at its centre.
    Without ``widths`` each flight's narrowest section is its width; without
    ``offset`` the walking line runs along the middle. The line runs on past
    the first and last sections. ``None`` without two sections.
    """

    if len(sections) < 2:
        return None
    inner = _inner_side(sections)
    outward = 1.0 if inner == "right" else -1.0

    def distance(flight):
        if widths is not None and 0 <= flight < len(widths):
            width = float(widths[flight])
        else:
            width = min(section.width for section in sections if section.flight_index == flight)
        if offset is None or float(offset) <= 0.0:
            return width / 2.0
        return min(float(offset), width / 2.0)

    def inner_point(section):
        point = getattr(section, inner)
        for square in newels:
            if _in_square(point, square):
                return (
                    sum(corner[0] for corner in square) / len(square),
                    sum(corner[1] for corner in square) / len(square),
                )
        return point

    def on_newel(section):
        return any(_in_square(getattr(section, inner), square) for square in newels)

    # The inner side, as a path through the newels' centres.
    path = []
    for section in sections:
        point = inner_point(section)
        if path and math.dist(path[-1][0], point) < 1e-6:
            path[-1][1].add(section.flight_index)
        else:
            path.append([point, {section.flight_index}])
    # Straight runs of the inner side need only their ends.
    index = 1
    while index < len(path) - 1:
        a, b, c = path[index - 1][0], path[index][0], path[index + 1][0]
        ab, bc = math.dist(a, b), math.dist(b, c)
        cross = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
        dot = (b[0] - a[0]) * (c[0] - b[0]) + (b[1] - a[1]) * (c[1] - b[1])
        if ab < 1e-6 or bc < 1e-6 or (abs(cross) <= 1e-4 * ab * bc and dot > 0.0):
            path[index - 1][1] |= path[index][1]
            path[index + 1][1] |= path[index][1]
            del path[index]
        else:
            index += 1
    first, last = sections[0], sections[-1]
    reach = 1000.0 + sum(math.dist(a[0], b[0]) for a, b in zip(path, path[1:]))
    path.insert(
        0,
        [
            (
                path[0][0][0] - first.tangent[0] * reach,
                path[0][0][1] - first.tangent[1] * reach,
            ),
            set(path[0][1]),
        ],
    )
    path.append(
        [
            (
                path[-1][0][0] + last.tangent[0] * reach,
                path[-1][0][1] + last.tangent[1] * reach,
            ),
            set(path[-1][1]),
        ]
    )
    segments = []
    for a, b in zip(path, path[1:]):
        length = math.dist(a[0], b[0])
        if length < 1e-6:
            continue
        direction = ((b[0][0] - a[0][0]) / length, (b[0][1] - a[0][1]) / length)
        normal = (-direction[1] * outward, direction[0] * outward)
        # The steps across this stretch: their ends' distances from it.
        inner_ends, outer_ends = [], []
        for section in sections:
            if on_newel(section):
                continue
            start = getattr(section, inner)
            finish = getattr(section, "left" if inner == "right" else "right")
            ends = [(point[0] - a[0][0], point[1] - a[0][1]) for point in (start, finish)]
            if not all(
                -1e-6 <= x * direction[0] + y * direction[1] <= length + 1e-6 for x, y in ends
            ):
                continue
            near, far = (x * normal[0] + y * normal[1] for x, y in ends)
            if near < -1e-6 or far <= near:
                continue
            inner_ends.append(near)
            outer_ends.append(far)
        base = max([0.0] + inner_ends)
        across = min(outer_ends) - base if outer_ends else 0.0
        radius = None
        if across > 1e-6:
            if offset is None or float(offset) <= 0.0:
                radius = base + across / 2.0
            else:
                radius = base + min(float(offset), across / 2.0)
        flights = (a[1] & b[1]) or (a[1] | b[1])
        segments.append([a[0], b[0], direction, normal, radius, flights])
    if not segments:
        return None
    measured = [segment[4] for segment in segments if segment[4] is not None]

    def straight(first, second):
        return abs(first[2][0] * second[2][1] - first[2][1] * second[2][0]) <= 1e-4 and (
            first[2][0] * second[2][0] + first[2][1] * second[2][1] > 0.0
        )

    unmeasured = [index for index, segment in enumerate(segments) if segment[4] is None]
    # Along a straight run first, both ways, then from the next stretch.
    for _sweep in range(len(segments)):
        for index in unmeasured:
            for other in (index - 1, index + 1):
                if (
                    segments[index][4] is None
                    and 0 <= other < len(segments)
                    and other not in unmeasured
                    and straight(segments[index], segments[other])
                ):
                    segments[index][4] = segments[other][4]
        unmeasured = [index for index in unmeasured if segments[index][4] is None]
    for index in reversed(unmeasured):
        following = segments[index + 1][4] if index + 1 < len(segments) else None
        if following is None and measured:
            following = measured[-1]
        if following is None:
            following = min(distance(flight) for flight in segments[index][5])
        segments[index][4] = following
    segments = [tuple(segment[:5]) for segment in segments]
    points = []
    for index, (a, b, direction, normal, radius) in enumerate(segments):
        start = (a[0] + normal[0] * radius, a[1] + normal[1] * radius)
        if not points:
            points.append(start)
        end = (b[0] + normal[0] * radius, b[1] + normal[1] * radius)
        if index + 1 == len(segments):
            points.append(end)
            break
        _a, _b, following, next_normal, next_radius = segments[index + 1]
        cross = direction[0] * following[1] - direction[1] * following[0]
        if cross * outward < -1e-9:
            # Turning toward the inner side: round the corner.
            points.append(end)
            first_angle = math.atan2(normal[1], normal[0])
            turn = math.remainder(
                math.atan2(next_normal[1], next_normal[0]) - first_angle, 2.0 * math.pi
            )
            count = max(int(abs(turn) / math.radians(2.0)), 1)
            for step in range(1, count + 1):
                ratio = step / count
                angle = first_angle + turn * ratio
                # Eased, the arc leaves and meets the flights' lines square.
                ease = (1.0 - math.cos(math.pi * ratio)) / 2.0
                arc_radius = radius + (next_radius - radius) * ease
                points.append(
                    (b[0] + math.cos(angle) * arc_radius, b[1] + math.sin(angle) * arc_radius)
                )
        elif cross * outward > 1e-9:
            # Turning toward the outer side: the offset lines meet.
            joint = _line_intersection(
                end,
                direction,
                (b[0] + next_normal[0] * next_radius, b[1] + next_normal[1] * next_radius),
                following,
            )
            points.append(joint if joint is not None else end)
        else:
            points.append(end)
            points.append(
                (b[0] + next_normal[0] * next_radius, b[1] + next_normal[1] * next_radius)
            )
    return _without_duplicate_points(points)


def _inner_side(sections):
    """Return the sections' inner side, the shorter of their ends' paths."""

    def path_length(side):
        points = [getattr(section, side) for section in sections]
        return sum(math.dist(a, b) for a, b in zip(points, points[1:]))

    return "left" if path_length("left") < path_length("right") - 1e-6 else "right"


def _opens_inward(sections, inner):
    """Return the indices of the treads between ``sections`` wider on the
    ``inner`` side, from riser to riser, than on the other."""

    outer = "right" if inner == "left" else "left"
    return {
        index
        for index, (a, b) in enumerate(zip(sections, sections[1:]))
        if math.dist(getattr(a, outer), getattr(b, outer))
        < math.dist(getattr(a, inner), getattr(b, inner)) - 1e-6
    }


def _polyline_lengths(points):
    """Return ``(points, cumulative lengths)``, or ``None`` without a line."""

    if not points or len(points) < 2:
        return None
    cumulative = [0.0]
    for a, b in zip(points, points[1:]):
        cumulative.append(cumulative[-1] + math.dist(a, b))
    return points, cumulative


def _polyline_station(first, second, line, after=None):
    """Return how far along ``line`` the segment ``first``-``second``, run a
    millimetre past each end, crosses it, the first crossing not before
    ``after``; ``None`` if it does not."""

    points, cumulative = line
    length = math.dist(first, second)
    if length < 1e-9:
        return None
    direction = ((second[0] - first[0]) / length, (second[1] - first[1]) / length)
    a = (first[0] - direction[0], first[1] - direction[1])
    span = (direction[0] * (length + 2.0), direction[1] * (length + 2.0))
    found = None
    for index, (p, q) in enumerate(zip(points, points[1:])):
        edge = (q[0] - p[0], q[1] - p[1])
        denominator = span[0] * edge[1] - span[1] * edge[0]
        if abs(denominator) < 1e-12:
            continue
        relative = (p[0] - a[0], p[1] - a[1])
        t = (relative[0] * edge[1] - relative[1] * edge[0]) / denominator
        u = (relative[0] * span[1] - relative[1] * span[0]) / denominator
        if -1e-9 <= t <= 1.0 + 1e-9 and -1e-9 <= u <= 1.0 + 1e-9:
            value = cumulative[index] + u * (cumulative[index + 1] - cumulative[index])
            if after is not None and value < after - 1e-6:
                continue
            if found is None or value < found:
                found = value
    return found


def _walking_point(line, value):
    """Return the point ``value`` along the walking ``line``, a polyline and
    its lengths."""

    points, cumulative = line
    index = max(min(bisect.bisect_right(cumulative, value) - 1, len(points) - 2), 0)
    length = cumulative[index + 1] - cumulative[index]
    ratio = (value - cumulative[index]) / length if length > 1e-12 else 0.0
    p, q = points[index], points[index + 1]
    return (p[0] + (q[0] - p[0]) * ratio, p[1] + (q[1] - p[1]) * ratio)


def walking_line_treads(sections, newels=(), widths=None, offset=None):
    """Return each tread's stretch of the walking line (see
    ``walking_line_points``): ``(points, going)`` from its front section to
    the next, ``None`` for a tread the line does not cross both ends of."""

    line = _polyline_lengths(walking_line_points(sections, newels, widths, offset))
    if line is None:
        return [None] * max(len(sections) - 1, 0)
    points, cumulative = line
    stations = []
    previous = None
    for section in sections:
        value = _polyline_station(section.left, section.right, line, previous)
        stations.append(value)
        if value is not None:
            previous = value

    def point_at(value):
        return _walking_point(line, value)

    result = []
    for begin, finish in zip(stations, stations[1:]):
        if begin is None or finish is None or finish <= begin:
            result.append(None)
            continue
        inside = [point for point, value in zip(points, cumulative) if begin < value < finish]
        result.append(([point_at(begin)] + inside + [point_at(finish)], finish - begin))
    return result


def newel_regions(pivots, reach, squares=(), posts=None):
    """Return the plan squares the risers fan about: the newel posts'
    ``squares``, and a newel's size, ``reach``, about each of the
    ``pivots`` without a post (of ``posts``, the squares by default)."""

    return list(squares) + [
        newel_post_square(pivot, (1.0, 0.0), reach + 2e-6)
        for pivot in pivots
        if not any(_in_square(pivot, post) for post in (posts or squares))
    ]


def newel_fan_plan(
    sections,
    footprint,
    entry_direction,
    exit_direction,
    reach,
    squares=(),
    posts=None,
    walking_line=None,
):
    """Return the stair's plan about its newels.

    Returns ``(sections, step_sections, step_footprint)``: the sections the
    boards and rails follow, ending on the stair's sides, and those the
    steps follow with the outline they partition, ending on the newel posts
    (see ``newel_post_plan``), or ``None`` for both where no step meets a
    post. The treads turning about each newel then share the walking line
    about it (see ``balance_newel_fans``, and its ``walking_line``), as the
    steps outline them; the boards' sections take the same outer ends.
    ``reach`` is a newel's size.
    """

    pivots = newel_corners(sections, footprint, entry_direction, exit_direction)
    margin = _newel_half(reach)
    steps, notched = newel_post_plan(sections, footprint, squares, posts)
    if steps is sections:
        regions = [newel_post_square(pivot, (1.0, 0.0), reach + 2e-6) for pivot in pivots]
        return (
            balance_newel_fans(sections, footprint, regions, margin, walking_line),
            None,
            None,
        )
    regions = newel_regions(pivots, reach, squares, posts)
    balanced = balance_newel_fans(steps, notched, regions, margin, walking_line)
    boards = list(sections)
    for index, (board, before, after) in enumerate(zip(sections, steps, balanced)):
        if after is before:
            continue
        left = after.left if after.left != before.left else board.left
        right = after.right if after.right != before.right else board.right
        width = math.dist(left, right)
        boards[index] = replace(
            board,
            left=left,
            right=right,
            width=width,
            center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
            tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
            locked_to_flight=False,
        )
    if boards != list(sections):
        try:
            faces = balanced_tread_faces(boards, footprint)
        except ValueError:
            faces = []
        if not balanced_partition_is_valid(faces, footprint, len(boards) - 1):
            return list(sections), steps, notched
    return boards, balanced, notched


def _straight_outline(footprint):
    """Return the outline's boundary data, ``None`` if it has a curved edge."""

    if footprint.isNull() or any(
        abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
        for edge in footprint.Edges
    ):
        return None
    return _boundary_data(footprint)


def _slide_section(sections, index, shifts, boundary):
    """Return section ``index`` with its ends moved along the outline.

    ``shifts`` maps a side to its shift: a positive one moves that end up
    the stair, toward the next section's end on that side. ``None`` when an
    end cannot be placed on the outline or the section collapses.
    """

    section = sections[index]
    moved = {}
    for side, offset in shifts.items():
        if abs(offset) <= 1e-9:
            continue
        end = getattr(section, side)
        for neighbour, sign in ((index + 1, 1.0), (index - 1, -1.0)):
            if not 0 <= neighbour < len(sections):
                continue
            other = getattr(sections[neighbour], side)
            if math.dist(other, end) < 1e-6:
                continue
            try:
                start, toward = _unwrap_boundary_parameters([end, other], boundary)
            except ValueError:
                return None
            direction = sign if toward > start else -sign
            moved[side] = _boundary_point(start + direction * offset, boundary)
            break
    if not moved:
        return section
    left = moved.get("left", section.left)
    right = moved.get("right", section.right)
    width = math.dist(left, right)
    if width < 1e-7:
        return None
    return replace(
        section,
        left=left,
        right=right,
        width=width,
        center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
        tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
        locked_to_flight=False,
    )


def shift_section_ends(sections, footprint, offsets, keep=()):
    """Return the sections with their ends moved along the stair's outline.

    ``offsets`` gives each section's ``(left, right)`` shift, the first
    section's first: a positive one moves that end up the stair, toward the
    next section's end on that side. The ``(index, side)`` ends in ``keep``
    stay. Returns ``None`` when the moved sections do not partition the
    ``footprint``.
    """

    if footprint.isNull() or not any(abs(value) > 1e-9 for pair in offsets for value in pair):
        return list(sections)
    boundary = _straight_outline(footprint)
    if boundary is None:
        return None
    result = list(sections)
    for index, pair in enumerate(offsets[: len(sections)]):
        shifts = {
            side: offset
            for side, offset in zip(("left", "right"), pair)
            if (index, side) not in keep
        }
        moved = _slide_section(sections, index, shifts, boundary)
        if moved is None:
            return None
        result[index] = moved
    try:
        faces = balanced_tread_faces(result, footprint)
    except ValueError:
        return None
    if not balanced_partition_is_valid(faces, footprint, len(result) - 1):
        return None
    return result


def even_walking_line_goings(
    sections, footprint, newels=(), widths=None, offset=None, reach=0.0, outline=None
):
    """Return the sections moved so the treads have their goings along the
    walking line (see ``walking_line_points``).

    The first and last sections stay: between them, the treads share the
    walking line evenly, each with its own extra going kept, as the
    difference of its sections' stations gives it. A riser ending on one of
    the ``newels`` plan squares keeps that end and turns about it to cross
    the walking line there, its other end sliding along the outline. The
    risers between two such, or the first or last section, cross it there
    with their angles, as they lie between those two's, spread between
    those two's new ones: the balancing keeps the order of its angles. Between two risers the balancing left
    parallel, they share those two's turns in proportion to the walking
    line between. An end landing less than half a
    newel's size, ``reach``,
    from a corner of the stair's ``outline`` (the ``footprint`` by default)
    goes that far from it, on the nearer side: about the corner, the tread
    would leave the boards a sliver to be cut or housed to. ``None`` when
    the sections so moved do not partition the ``footprint``, or leave a
    tread wider on the inner side than on the outer that was not: so close
    to the newels, the walking line is too short about them for the risers
    they hold.
    """

    if len(sections) < 3:
        return list(sections)
    boundary = _straight_outline(footprint)
    line = _polyline_lengths(walking_line_points(sections, newels, widths, offset))
    if boundary is None or line is None:
        return None
    stations = []
    previous = None
    for section in sections:
        value = _polyline_station(section.left, section.right, line, previous)
        if value is None:
            return None
        stations.append(value)
        previous = value
    length = stations[-1] - stations[0]
    if length <= 1e-6:
        return None
    goings = [b.station - a.station for a, b in zip(sections, sections[1:])]
    if sum(goings) <= 1e-9:
        goings = [0.0] * len(goings)
    # The same going added to each, so the extra goings stay.
    even = (length - sum(goings)) / len(goings)
    goings = [going + even for going in goings]
    if min(goings) <= 1e-6:
        return None
    corners = _boundary_data(outline)["vertices"] if outline is not None else boundary["vertices"]
    margin = _newel_half(reach)
    result = list(sections)
    targets = [stations[0]]
    for going in goings:
        targets.append(targets[-1] + going)
    # How far the risers about the newels turn; the first and last stay.
    turns = {0: 0.0, len(sections) - 1: 0.0}
    loose = []
    for index in range(1, len(sections) - 1):
        target = targets[index]
        section = sections[index]
        fixed = [
            side
            for side in ("left", "right")
            if any(_in_square(getattr(section, side), square) for square in newels)
        ]
        if len(fixed) == 2:
            turns[index] = 0.0
            continue
        if not fixed:
            loose.append(index)
            continue
        free = ["right" if fixed[0] == "left" else "left"]

        def crossing(distance):
            moved = _slide_section(result, index, {free[0]: distance}, boundary)
            if moved is None:
                return None, None
            return moved, _polyline_station(moved.left, moved.right, line)

        # Within its neighbours' reach on the walking line.
        reach_along = 2.0 * (stations[index + 1] - stations[index - 1])
        low, high = -reach_along, reach_along
        for _iteration in range(60):
            middle = (low + high) / 2.0
            _moved, value = crossing(middle)
            if value is not None and value < target:
                low = middle
            else:
                high = middle
        moved, _value = crossing((low + high) / 2.0)
        if moved is None:
            return None
        value = _polyline_station(moved.left, moved.right, line)
        if value is None or abs(value - target) > 0.5:
            return None
        moved = _clear_of_corners(
            moved, free, [result[index - 1], sections[index + 1]], boundary, margin, corners
        )
        if moved is None:
            return None
        result[index] = moved
        turns[index] = _section_turn(section, moved)
    for index in loose:
        target = targets[index]
        before = max(key for key in turns if key < index)
        after = min(key for key in turns if key > index)
        ratio = (target - targets[before]) / (targets[after] - targets[before])
        # Angles from the riser before, as the balancing left them and now.
        span = _section_turn(sections[before], sections[after])
        spread = span + turns[after] - turns[before]
        if abs(span) > 1e-6 and spread / span > 0.0:
            share = _section_turn(sections[before], sections[index]) / span
        else:
            share = ratio
        turn = turns[before] + spread * share - _section_turn(sections[before], sections[index])
        moved = _section_through(sections[index], _walking_point(line, target), boundary, turn)
        if moved is None:
            return None
        value = _polyline_station(moved.left, moved.right, line)
        if value is None or abs(value - target) > 0.5:
            return None
        moved = _clear_of_corners(
            moved,
            ["left", "right"],
            [result[index - 1], result[index + 1]],
            boundary,
            margin,
            corners,
        )
        if moved is None:
            return None
        result[index] = moved
    inner = _inner_side(sections)
    if _opens_inward(result, inner) - _opens_inward(sections, inner):
        return None
    try:
        faces = balanced_tread_faces(result, footprint)
    except ValueError:
        return None
    if not balanced_partition_is_valid(faces, footprint, len(result) - 1):
        return None
    return result


def _section_turn(section, moved):
    """Return the angle ``section`` turned by to become ``moved``."""

    def angle(item):
        return math.atan2(item.right[1] - item.left[1], item.right[0] - item.left[0])

    return math.remainder(angle(moved) - angle(section), 2.0 * math.pi)


def _section_through(section, point, boundary, turn=0.0):
    """Return ``section`` turned by ``turn`` and moved to run through
    ``point``, its ends where its line meets the outline nearest the point
    on either side; ``None`` if the line leaves the outline on one side."""

    length = math.dist(section.left, section.right)
    if length < 1e-9:
        return None
    angle = (
        math.atan2(section.right[1] - section.left[1], section.right[0] - section.left[0]) + turn
    )
    across = (math.cos(angle), math.sin(angle))
    hits = []
    vertices = boundary["vertices"]
    for a, b in zip(vertices, vertices[1:] + vertices[:1]):
        edge = (b[0] - a[0], b[1] - a[1])
        denominator = _cross(across, edge)
        if abs(denominator) < 1e-12:
            continue
        relative = (a[0] - point[0], a[1] - point[1])
        t = _cross(relative, edge) / denominator
        u = _cross(relative, across) / denominator
        if -1e-9 <= u <= 1.0 + 1e-9:
            hits.append(t)
    before = [t for t in hits if t < -1e-6]
    after = [t for t in hits if t > 1e-6]
    if not before or not after:
        return None
    left = (point[0] + across[0] * max(before), point[1] + across[1] * max(before))
    right = (point[0] + across[0] * min(after), point[1] + across[1] * min(after))
    width = math.dist(left, right)
    return replace(
        section,
        left=left,
        right=right,
        width=width,
        center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
        tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
        locked_to_flight=False,
    )


def _clear_of_corners(section, sides, neighbours, boundary, margin, corners):
    """Return ``section`` with its ``sides`` ends at least ``margin`` before
    the ``corners`` lying on the outline: one closer to a corner moves back
    along the outline, toward the previous of its ``neighbours``, to that
    distance before it, unless that passes the neighbour. A corner a
    ``neighbours`` end stands on is left."""

    if margin <= 1e-9:
        return section
    perimeter = boundary["perimeter"]
    ends = {}
    for side in sides:
        point = getattr(section, side)
        try:
            previous, parameter = _unwrap_boundary_parameters(
                [getattr(neighbours[0], side), point], boundary
            )
        except ValueError:
            return None
        forward = 1.0 if parameter >= previous else -1.0
        for corner_point in corners:
            if any(math.dist(getattr(other, side), corner_point) < 1e-6 for other in neighbours):
                continue
            try:
                candidates = _boundary_candidates(corner_point, boundary)
            except ValueError:
                # Within a post, off the steps' outline.
                continue
            for corner in candidates:
                corner += round((parameter - corner) / perimeter) * perimeter
                clear = corner - forward * margin
                if abs(parameter - corner) >= margin - 1e-9 or (clear - previous) * forward <= 1e-6:
                    continue
                ends[side] = _boundary_point(clear, boundary)
                break
            if side in ends:
                break
    if not ends:
        return section
    left = ends.get("left", section.left)
    right = ends.get("right", section.right)
    width = math.dist(left, right)
    if width < 1e-7:
        return None
    return replace(
        section,
        left=left,
        right=right,
        width=width,
        center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
        tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
        locked_to_flight=False,
    )


def follow_moved_ends(sections, before, after):
    """Return ``sections`` with the ends that moved from ``before`` to
    ``after``, other plans of the same risers, moved with them: the boards
    take the steps' ends, but where those stand on a post."""

    result = list(sections)
    for index, (section, old, new) in enumerate(zip(sections, before, after)):
        if new is old:
            continue
        left = new.left if math.dist(new.left, old.left) > 1e-9 else section.left
        right = new.right if math.dist(new.right, old.right) > 1e-9 else section.right
        if left == section.left and right == section.right:
            continue
        width = math.dist(left, right)
        if width < 1e-7:
            return None
        result[index] = replace(
            section,
            left=left,
            right=right,
            width=width,
            center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
            tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
            locked_to_flight=False,
        )
    return result


def newel_corners(sections, footprint, entry_direction, exit_direction):
    """Return the corners newels stand at: a turned entry's or exit's inner
    end, and each reflex corner of the ``footprint``."""

    pivots = []
    for at_start, direction in ((True, entry_direction), (False, exit_direction)):
        direction = str(direction).lower()
        if direction != "straight" and len(sections) >= 2:
            side = "left" if "left" in direction else "right"
            pivots.append(getattr(sections[0 if at_start else -1], side))
    if footprint.isNull():
        return pivots
    boundary = _boundary_data(footprint)
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    vertices = boundary["vertices"]
    count = len(vertices)
    for index, corner in enumerate(vertices):
        previous = vertices[index - 1]
        following = vertices[(index + 1) % count]
        turn = (corner[0] - previous[0]) * (following[1] - corner[1]) - (
            corner[1] - previous[1]
        ) * (following[0] - corner[0])
        if turn * orientation < -1e-7:
            pivots.append(corner)
    return pivots


def balance_newel_fans(sections, footprint, newels, margin=0.0, walking_line=None):
    """Give the treads turning about each newel their walking-line goings.

    The risers ending at a newel, one of the ``newels`` plan squares, fan
    out from it. Pulled there from where the walking line placed them, they
    no longer cross it at their stations. The walking line runs round the
    newel (see ``walking_line_points``): the treads lying wholly in the fan,
    between two risers ending at the newel or between one and the stair's
    first or last section, share its length there. The risers between them
    keep their end at the newel and their other ends move along the stair's
    outline so that each tread takes a share in proportion to its going.
    ``walking_line`` gives the flights' step widths and the walking line's
    offset from the outer side; without it the walking line runs along the
    middle of the flights.
    A tread between the newel and a riser housed in a board is balanced
    along that board, and keeps the walking line's place. No end is moved
    to less than ``margin`` from a corner of the outline. Where the treads
    so placed would not partition the ``footprint``, the sections are
    returned as they were.
    """

    if len(sections) < 4 or footprint.isNull() or not newels:
        return list(sections)
    if any(
        abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
        for edge in footprint.Edges
    ):
        return list(sections)
    boundary = _boundary_data(footprint)
    widths, offset = walking_line if walking_line is not None else (None, None)
    # The risers keep their inner ends, which the walking line follows.
    line = _polyline_lengths(walking_line_points(sections, newels, widths, offset))
    if line is None:
        return list(sections)
    result = list(sections)
    for newel in newels:
        # The runs of inner sections with an end on this newel.
        runs = []
        for index in range(1, len(result) - 1):
            side = next(
                (
                    side
                    for side in ("left", "right")
                    if _in_square(getattr(result[index], side), newel)
                ),
                None,
            )
            if side is None:
                continue
            if runs and runs[-1][1] == index - 1 and runs[-1][2] == side:
                runs[-1][1] = index
            else:
                runs.append([index, index, side])
        for first, last, side in runs:
            front = first - 1 if first == 1 else first
            rear = last + 1 if last == len(result) - 2 else last
            if rear - front < 2:
                continue
            moved = _balanced_fan(result, front, rear, side, boundary, margin, line)
            if moved is not None:
                result[front + 1 : rear] = moved
    if result != list(sections):
        try:
            faces = balanced_tread_faces(result, footprint)
        except ValueError:
            faces = []
        if not balanced_partition_is_valid(faces, footprint, len(result) - 1):
            # Left pinned to the newels as they were.
            return list(sections)
    return result


def _in_square(point, square):
    """Return whether ``point`` lies in a convex plan polygon or on its outline."""

    edges = list(zip(square, square[1:] + square[:1]))
    crossings = [
        (b[0] - a[0]) * (point[1] - a[1]) - (b[1] - a[1]) * (point[0] - a[0]) for a, b in edges
    ]
    return all(value >= -1e-6 for value in crossings) or all(value <= 1e-6 for value in crossings)


def _balanced_fan(sections, front, rear, side, boundary, margin, line):
    """Return the sections strictly between ``front`` and ``rear``, their
    ``side`` ends kept and their other ends moved along the outline so the
    treads between ``front`` and ``rear`` share the walking ``line`` (see
    ``walking_line``) in proportion to their goings.

    An end landing less than ``margin`` from a corner of the outline stops
    that far before it: about the corner, the tread would leave the boards a
    sliver to be cut or housed to.
    """

    other = "right" if side == "left" else "left"
    span = sections[front : rear + 1]
    try:
        other_parameters = _unwrap_boundary_parameters(
            [getattr(section, other) for section in span], boundary
        )
    except ValueError:
        return None
    start, end = other_parameters[0], other_parameters[-1]
    if abs(end - start) < 1e-6:
        return None

    def ends(section_side, section_other):
        return (section_side, section_other) if side == "left" else (section_other, section_side)

    def station(fixed, outer):
        # Where the riser from its ``fixed`` end through ``outer`` crosses
        # the walking line.
        return _polyline_station(fixed, outer, line)

    stations = [station(getattr(section, side), getattr(section, other)) for section in span]
    if any(value is None for value in stations):
        return None
    if stations[-1] - stations[0] < 1e-6:
        return None

    goings = [max(b.station - a.station, 0.0) for a, b in zip(span, span[1:])]
    total_going = sum(goings)
    if total_going <= 1e-9:
        goings = [1.0] * len(goings)
        total_going = float(len(goings))
    moved = []
    lower = start
    share = 0.0
    for offset, section in enumerate(span[1:-1], start=1):
        share += goings[offset - 1]
        target = stations[0] + (stations[-1] - stations[0]) * share / total_going
        fixed = getattr(section, side)
        low, high = lower, end
        for _iteration in range(60):
            middle = (low + high) / 2.0
            value = station(fixed, _boundary_point(middle, boundary))
            if value is not None and value < target:
                low = middle
            else:
                high = middle
        parameter = (low + high) / 2.0
        perimeter = boundary["perimeter"]
        forward = 1.0 if end > start else -1.0
        for corner in boundary["cumulative"][:-1]:
            corner += round((parameter - corner) / perimeter) * perimeter
            clear = corner - forward * margin
            if (
                abs(corner - parameter) < margin
                and (corner - start) * forward > 1e-6
                and (end - corner) * forward > 1e-6
                and (clear - lower) * forward > 1e-6
            ):
                parameter = clear
                break
        lower = parameter
        left, right = ends(fixed, _boundary_point(parameter, boundary))
        width = math.dist(left, right)
        if width < 1e-7:
            return None
        moved.append(
            replace(
                section,
                left=left,
                right=right,
                width=width,
                center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
                tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
                locked_to_flight=False,
            )
        )
    return moved


def _side_landing(section, corner, into_flight):
    """Return where a riser's line meets a side, measured from ``corner``.

    The side runs through ``corner`` along ``into_flight``, pointing into
    the flight, away from the opening beyond the newel: the distance is
    positive on the flight's board, negative in the opening. ``None`` when
    the riser runs along the side.
    """

    length = math.hypot(*section.tangent)
    if length < 1e-9:
        return None
    along = (-section.tangent[1] / length, section.tangent[0] / length)
    crossing = into_flight[0] * along[1] - into_flight[1] * along[0]
    if abs(crossing) < 1e-9:
        return None
    offset = (section.center[0] - corner[0], section.center[1] - corner[1])
    return (offset[0] * along[1] - offset[1] * along[0]) / crossing


def make_tangent_stair_footprint(
    flight_specs,
    turn_types=None,
    start_angle=0.0,
    end_angle=0.0,
):
    """Return the exact strip footprint of a mixed straight/circular path."""

    if not flight_specs:
        return Part.Shape()
    specs, primitives = _tangent_path_primitives(flight_specs)
    corner_types = list(turn_types or [])
    corner_types.extend(["Herse balancing"] * (len(specs) - 1 - len(corner_types)))
    modes = _tangent_junction_modes(primitives, corner_types[: len(specs) - 1])
    start_extensions = [0.0] * len(primitives)
    end_extensions = [0.0] * len(primitives)
    for index, mode in enumerate(modes):
        if mode == "Tangent":
            continue
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        turn_sine = abs(_cross(incoming["end_tangent"], outgoing["tangent"]))
        end_extensions[index] = outgoing["width"] / (2.0 * turn_sine)
        start_extensions[index + 1] = incoming["width"] / (2.0 * turn_sine)

    faces = []
    for index, primitive in enumerate(primitives):
        if primitive["type"] == "Circular":
            faces.append(_circular_primitive_face(primitive))
        else:
            direction = primitive["tangent"]
            normal = (-direction[1], direction[0])
            primitive_start = primitive.get("face_start", primitive["start"])
            primitive_end = primitive.get("face_end", primitive["end"])
            start = (
                primitive_start[0] - direction[0] * start_extensions[index],
                primitive_start[1] - direction[1] * start_extensions[index],
            )
            end = (
                primitive_end[0] + direction[0] * end_extensions[index],
                primitive_end[1] + direction[1] * end_extensions[index],
            )
            half_width = primitive["width"] / 2.0
            start_offset = 0.0
            end_offset = 0.0
            if index == 0:
                start_offset = half_width * _safe_angle_tangent(start_angle)
            if index == len(primitives) - 1:
                end_offset = half_width * _safe_angle_tangent(end_angle)
            faces.append(
                _horizontal_face(
                    (
                        (
                            start[0] + normal[0] * half_width + direction[0] * start_offset,
                            start[1] + normal[1] * half_width + direction[1] * start_offset,
                        ),
                        (
                            end[0] + normal[0] * half_width + direction[0] * end_offset,
                            end[1] + normal[1] * half_width + direction[1] * end_offset,
                        ),
                        (
                            end[0] - normal[0] * half_width - direction[0] * end_offset,
                            end[1] - normal[1] * half_width - direction[1] * end_offset,
                        ),
                        (
                            start[0] - normal[0] * half_width - direction[0] * start_offset,
                            start[1] - normal[1] * half_width - direction[1] * start_offset,
                        ),
                    ),
                    0.0,
                )
            )
    return _fuse_plan_faces(faces)


def tangent_tread_faces(sections, flight_specs):
    """Return tread cells clipped to their own tangent-path primitive.

    Explicit landing flights introduce internal plan seams, so their cells
    cannot be recovered by walking only the outer wire of the stair union.
    """

    _specs, primitives = _tangent_path_primitives(flight_specs)
    primitive_faces = [
        (
            _circular_primitive_face(primitive)
            if primitive["type"] == "Circular"
            else _straight_primitive_face(primitive)
        )
        for primitive in primitives
    ]
    result = []
    for front, rear in zip(sections, sections[1:]):
        if front.level_to_next:
            result.append(primitive_faces[front.flight_index])
            continue
        primitive_index = (
            rear.flight_index
            if rear.flight_index != front.flight_index and not rear.level_to_next
            else front.flight_index
        )
        primitive_face = primitive_faces[primitive_index]
        faces = _balanced_step_faces(front, rear, primitive_face)
        if not faces:
            return []
        result.append(faces[0] if len(faces) == 1 else _fuse_plan_faces(faces))
    return result


def _straight_primitive_face(primitive):
    direction = primitive["tangent"]
    normal = (-direction[1], direction[0])
    half_width = primitive["width"] / 2.0
    start = primitive.get("face_start", primitive["start"])
    end = primitive.get("face_end", primitive["end"])
    return _horizontal_face(
        (
            (
                start[0] + normal[0] * half_width,
                start[1] + normal[1] * half_width,
            ),
            (
                end[0] + normal[0] * half_width,
                end[1] + normal[1] * half_width,
            ),
            (
                end[0] - normal[0] * half_width,
                end[1] - normal[1] * half_width,
            ),
            (
                start[0] - normal[0] * half_width,
                start[1] - normal[1] * half_width,
            ),
        ),
        0.0,
    )


def _circular_primitive_face(primitive):
    def vector(point):
        return FreeCAD.Vector(point[0], point[1], 0.0)

    def side_point(distance, side):
        center = _primitive_point(primitive, distance)
        tangent = _primitive_tangent(primitive, distance)
        normal = (-tangent[1], tangent[0])
        offset = side * primitive["width"] / 2.0
        return center[0] + normal[0] * offset, center[1] + normal[1] * offset

    half = primitive["length"] / 2.0
    left_start = side_point(0.0, 1.0)
    left_middle = side_point(half, 1.0)
    left_end = side_point(primitive["length"], 1.0)
    right_start = side_point(0.0, -1.0)
    right_middle = side_point(half, -1.0)
    right_end = side_point(primitive["length"], -1.0)
    edges = (
        Part.Arc(vector(left_start), vector(left_middle), vector(left_end)).toShape(),
        Part.makeLine(vector(left_end), vector(right_end)),
        Part.Arc(vector(right_end), vector(right_middle), vector(right_start)).toShape(),
        Part.makeLine(vector(right_start), vector(left_start)),
    )
    return Part.Face(Part.Wire(edges))


def _fuse_plan_faces(faces):
    result = faces[0]
    for face in faces[1:]:
        result = result.fuse(face)
        if len(result.Faces) == 1:
            result = result.Faces[0]
    result = result.removeSplitter()
    if len(result.Faces) == 1 and result.Faces[0].isValid():
        return result.Faces[0]
    solids = [face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0)) for face in result.Faces]
    solid = solids[0]
    for addition in solids[1:]:
        solid = solid.fuse(addition)
    solid = solid.removeSplitter()
    horizontal = [face for face in solid.Faces if face.BoundBox.ZLength < 1e-7]
    if horizontal:
        return min(horizontal, key=lambda face: face.BoundBox.ZMin)
    return result


def fit_tangent_sections_to_footprint(sections, footprint):
    """Extend radial/tangent sections to the exact mixed-flight boundary."""

    if len(sections) < 2 or footprint.isNull():
        return sections
    extent = max(footprint.BoundBox.DiagonalLength * 2.0, 1000.0)
    fitted = []
    for section_index, section in enumerate(sections):
        center_vertex = Part.Vertex(FreeCAD.Vector(section.center[0], section.center[1], 0.0))
        if (
            section_index in {0, len(sections) - 1}
            and footprint.OuterWire.distToShape(center_vertex)[0] < 1e-7
        ):
            # Entry/exit sections can intentionally occupy only part of an
            # outer side. Extending their coincident line would turn them
            # into the complete rail and destroy the endpoint winding.
            fitted.append(section)
            continue
        normal = (-section.tangent[1], section.tangent[0])
        line = Part.makeLine(
            FreeCAD.Vector(
                section.center[0] - normal[0] * extent,
                section.center[1] - normal[1] * extent,
                0.0,
            ),
            FreeCAD.Vector(
                section.center[0] + normal[0] * extent,
                section.center[1] + normal[1] * extent,
                0.0,
            ),
        )
        intervals = []
        for edge in footprint.common(line).Edges:
            parameters = [
                (vertex.Point.x - section.center[0]) * normal[0]
                + (vertex.Point.y - section.center[1]) * normal[1]
                for vertex in edge.Vertexes
            ]
            if parameters:
                intervals.append((min(parameters), max(parameters)))
        containing = [
            interval for interval in intervals if interval[0] - 1e-7 <= 0.0 <= interval[1] + 1e-7
        ]
        if containing:
            lower, upper = max(containing, key=lambda interval: interval[1] - interval[0])
            left = (
                section.center[0] + normal[0] * upper,
                section.center[1] + normal[1] * upper,
            )
            right = (
                section.center[0] + normal[0] * lower,
                section.center[1] + normal[1] * lower,
            )
            width = upper - lower
        else:
            left, right, width = section.left, section.right, section.width
        fitted.append(
            BalancedSection(
                center=section.center,
                tangent=section.tangent,
                left=left,
                right=right,
                station=section.station,
                width=width,
                flight_index=section.flight_index,
                landing_to_next=section.landing_to_next,
                locked_to_flight=section.locked_to_flight,
                level_to_next=section.level_to_next,
                riser_index=section.riser_index,
            )
        )
    return fitted


def make_stair_footprint(flight_specs, start_angle=0.0, end_angle=0.0):
    """Return the mitered strip footprint of connected straight flights."""

    if not flight_specs:
        return Part.Shape()
    specs = [
        (max(float(length), 0.01), max(float(width), 0.01), float(heading))
        for length, width, heading in flight_specs
    ]
    directions = []
    for _length, _width, heading in specs:
        radians = math.radians(heading)
        directions.append((math.cos(radians), math.sin(radians)))
    center = (0.0, specs[0][1] / 2.0)
    vertices = [center]
    for (length, _width, _heading), direction in zip(specs, directions):
        center = (
            center[0] + direction[0] * length,
            center[1] + direction[1] * length,
        )
        vertices.append(center)

    left_starts = []
    right_starts = []
    left_ends = []
    right_ends = []
    for index, (_length, width, _heading) in enumerate(specs):
        direction = directions[index]
        normal = (-direction[1], direction[0])
        half_width = width / 2.0
        start = vertices[index]
        end = vertices[index + 1]
        left_starts.append((start[0] + normal[0] * half_width, start[1] + normal[1] * half_width))
        right_starts.append((start[0] - normal[0] * half_width, start[1] - normal[1] * half_width))
        left_ends.append((end[0] + normal[0] * half_width, end[1] + normal[1] * half_width))
        right_ends.append((end[0] - normal[0] * half_width, end[1] - normal[1] * half_width))

    for index in range(len(specs) - 1):
        incoming = directions[index]
        outgoing = directions[index + 1]
        left_corner = _line_intersection(
            left_ends[index], incoming, left_starts[index + 1], outgoing
        )
        right_corner = _line_intersection(
            right_ends[index], incoming, right_starts[index + 1], outgoing
        )
        if left_corner is None or right_corner is None:
            continue
        left_ends[index] = left_corner
        left_starts[index + 1] = left_corner
        right_ends[index] = right_corner
        right_starts[index + 1] = right_corner

    start_offset = specs[0][1] / 2.0 * _safe_angle_tangent(start_angle)
    start_direction = directions[0]
    left_starts[0] = (
        left_starts[0][0] + start_direction[0] * start_offset,
        left_starts[0][1] + start_direction[1] * start_offset,
    )
    right_starts[0] = (
        right_starts[0][0] - start_direction[0] * start_offset,
        right_starts[0][1] - start_direction[1] * start_offset,
    )
    end_offset = specs[-1][1] / 2.0 * _safe_angle_tangent(end_angle)
    end_direction = directions[-1]
    left_ends[-1] = (
        left_ends[-1][0] + end_direction[0] * end_offset,
        left_ends[-1][1] + end_direction[1] * end_offset,
    )
    right_ends[-1] = (
        right_ends[-1][0] - end_direction[0] * end_offset,
        right_ends[-1][1] - end_direction[1] * end_offset,
    )

    faces = []
    for index in range(len(specs)):
        try:
            face = _horizontal_face(
                (
                    left_starts[index],
                    left_ends[index],
                    right_ends[index],
                    right_starts[index],
                ),
                0.0,
            )
        except Part.OCCError:
            return Part.Shape()
        if not face.isValid() or face.Area < 1e-7:
            return Part.Shape()
        faces.append(face)
    planar_result = faces[0]
    for face in faces[1:]:
        planar_result = planar_result.fuse(face)
        if len(planar_result.Faces) == 1:
            planar_result = planar_result.Faces[0]
    planar_result = planar_result.removeSplitter()
    if len(planar_result.Faces) == 1 and planar_result.Faces[0].isValid():
        return planar_result.Faces[0]

    # Coplanar fusion sometimes leaves a shell with internal seams (notably
    # on a U footprint).  Fusing a thin extrusion is more reliable in OCCT;
    # its lower face is the same two-dimensional outline with one clean wire.
    solids = [face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0)) for face in planar_result.Faces]
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    result = result.removeSplitter()
    horizontal_faces = [face for face in result.Faces if face.BoundBox.ZLength < 1e-7]
    if horizontal_faces:
        return min(horizontal_faces, key=lambda face: face.BoundBox.ZMin)
    return planar_result


def _line_intersection(first_point, first_direction, second_point, second_direction):
    denominator = _cross(first_direction, second_direction)
    if abs(denominator) < 1e-9:
        return None
    relative = (
        second_point[0] - first_point[0],
        second_point[1] - first_point[1],
    )
    parameter = _cross(relative, second_direction) / denominator
    return (
        first_point[0] + first_direction[0] * parameter,
        first_point[1] + first_direction[1] * parameter,
    )


def fit_balanced_sections_to_footprint(sections, footprint):
    """Make tread endpoints advance monotonically along both stair rails."""

    if len(sections) < 2 or footprint.isNull():
        return sections
    sections = fit_tangent_sections_to_footprint(sections, footprint)
    boundary = _boundary_data(footprint)
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    left_parameters = _monotone_boundary_parameters(
        [section.left for section in sections],
        sections,
        boundary,
        -orientation,
    )
    right_parameters = _monotone_boundary_parameters(
        [section.right for section in sections],
        sections,
        boundary,
        orientation,
    )

    fitted = []
    for index, (section, left_parameter, right_parameter) in enumerate(
        zip(sections, left_parameters, right_parameters)
    ):
        center_on_boundary = (
            footprint.OuterWire.distToShape(
                Part.Vertex(FreeCAD.Vector(section.center[0], section.center[1], 0.0))
            )[0]
            < 1e-7
        )
        if (
            section.locked_to_flight
            or center_on_boundary
            or section.landing_to_next
            or (index and sections[index - 1].landing_to_next)
        ):
            fitted.append(section)
            continue
        left = _boundary_point(left_parameter, boundary)
        right = _boundary_point(right_parameter, boundary)
        left, right = _clip_chord_to_boundary(left, right, boundary)
        chord = (left[0] - right[0], left[1] - right[1])
        chord_length = math.hypot(chord[0], chord[1])
        if chord_length > 1e-9:
            tangent = (chord[1] / chord_length, -chord[0] / chord_length)
            if tangent[0] * section.tangent[0] + tangent[1] * section.tangent[1] < 0.0:
                tangent = (-tangent[0], -tangent[1])
        else:
            tangent = section.tangent
        center = (
            (left[0] + right[0]) / 2.0,
            (left[1] + right[1]) / 2.0,
        )
        fitted.append(
            BalancedSection(
                center=center,
                tangent=tangent,
                left=left,
                right=right,
                station=section.station,
                width=chord_length,
                flight_index=section.flight_index,
                landing_to_next=section.landing_to_next,
                locked_to_flight=section.locked_to_flight,
                level_to_next=section.level_to_next,
                riser_index=section.riser_index,
            )
        )
    return fitted


def inset_flight_specs(
    flight_specs,
    insets,
    start_angle=0.0,
    end_angle=0.0,
    entry_direction="Straight",
    exit_direction="Straight",
):
    """Return the steps' flights within a stair's flights.

    ``flight_specs`` are the stair's ``(length, width, heading)`` flights,
    as ``make_stair_footprint`` lays them out: their outline is the opening
    the stair stands in. ``insets`` gives each flight's ``(left, right)``
    room its boards and posts take beside its steps. The steps' flights are
    narrower by those, along a centre line moved to stay between them, and
    run between the same entry and exit. An entry or exit turned to a side
    opens along that side; the board of the other side runs across the
    flight's end there, and the steps stop short of it by its room. Returns ``(specs, shift)``: laid
    out from the origin as the stair's are, the steps' plan is moved by
    ``shift`` into place. ``None`` when two flights in line would need
    different centre lines.
    """

    specs = [
        (max(float(length), 0.01), max(float(width), 0.01), float(heading))
        for length, width, heading in flight_specs
    ]
    if not specs:
        return None
    insets = list(insets) + [(0.0, 0.0)] * (len(specs) - len(insets))
    directions = [
        (math.cos(math.radians(heading)), math.sin(math.radians(heading)))
        for _length, _width, heading in specs
    ]
    vertices = [(0.0, specs[0][1] / 2.0)]
    for (length, _width, _heading), direction in zip(specs, directions):
        vertices.append(
            (vertices[-1][0] + direction[0] * length, vertices[-1][1] + direction[1] * length)
        )
    # Each centre line moves to the left by half the difference of its
    # sides' insets.
    shifts = [(right - left) / 2.0 for left, right in insets[: len(specs)]]
    lines = [
        (
            (
                vertices[index][0] - direction[1] * shift,
                vertices[index][1] + direction[0] * shift,
            ),
            direction,
        )
        for index, (direction, shift) in enumerate(zip(directions, shifts))
    ]
    start_tangent = _safe_angle_tangent(start_angle)
    end_tangent = _safe_angle_tangent(end_angle)
    first_direction, last_direction = directions[0], directions[-1]
    moved = [
        (
            lines[0][0][0] + first_direction[0] * shifts[0] * start_tangent,
            lines[0][0][1] + first_direction[1] * shifts[0] * start_tangent,
        )
    ]
    for index in range(1, len(specs)):
        joint = _line_intersection(
            lines[index - 1][0], lines[index - 1][1], lines[index][0], lines[index][1]
        )
        if joint is None:
            if abs(shifts[index] - shifts[index - 1]) > 1e-9:
                return None
            joint = (
                vertices[index][0] - directions[index][1] * shifts[index],
                vertices[index][1] + directions[index][0] * shifts[index],
            )
        moved.append(joint)
    moved.append(
        (
            vertices[-1][0]
            - last_direction[1] * shifts[-1]
            + last_direction[0] * shifts[-1] * end_tangent,
            vertices[-1][1]
            + last_direction[0] * shifts[-1]
            + last_direction[1] * shifts[-1] * end_tangent,
        )
    )
    for index, direction, at_start, sign in (
        (0, first_direction, entry_direction, 1.0),
        (-1, last_direction, exit_direction, -1.0),
    ):
        turned = str(at_start).lower()
        if turned == "straight":
            continue
        flight = 0 if index == 0 else len(specs) - 1
        far = insets[flight][1 if "left" in turned else 0]
        moved[index] = (
            moved[index][0] + direction[0] * far * sign,
            moved[index][1] + direction[1] * far * sign,
        )
    result = []
    for index, ((_length, width, heading), direction) in enumerate(zip(specs, directions)):
        left, right = insets[index]
        length = (moved[index + 1][0] - moved[index][0]) * direction[0] + (
            moved[index + 1][1] - moved[index][1]
        ) * direction[1]
        width = width - left - right
        if length <= 1e-6 or width <= 1e-6:
            return None
        result.append((length, width, heading))
    shift = (moved[0][0], moved[0][1] - result[0][1] / 2.0)
    return result, shift


def translated_plan(sections, footprint, shift):
    """Return ``sections`` and ``footprint`` moved by the plan vector ``shift``."""

    if math.hypot(*shift) < 1e-12:
        return list(sections), footprint

    def moved(point):
        return (point[0] + shift[0], point[1] + shift[1])

    result = [
        replace(
            section,
            center=moved(section.center),
            left=moved(section.left),
            right=moved(section.right),
        )
        for section in sections
    ]
    if footprint is not None and not footprint.isNull():
        footprint = footprint.copy()
        footprint.translate(FreeCAD.Vector(shift[0], shift[1], 0.0))
    return result, footprint


def open_turned_endpoints(sections, footprint, openings, posts=()):
    """Return the steps' plan reaching out to a turned entry's or exit's floor.

    A turned entry or exit lies along a side of the stair. Its board ends at
    the newel, inside the stair's outline (see ``inset_flight_specs``), but
    the step there meets the floor at the outline. ``openings`` gives, for
    the first (``0``) or last (``-1``) of the ``sections``, how far the
    floor lies beyond it. That section moves out to the floor, and its tread
    takes the strip between, less the newel ``posts``' outlines. Returns
    ``(sections, footprint)``, unchanged without openings, or ``None`` when
    the plan so widened does not partition its outline.
    """

    openings = [(index, distance) for index, distance in openings if distance > 1e-9]
    if not openings or len(sections) < 2 or footprint.isNull():
        return sections, footprint
    elevation = footprint.BoundBox.ZMin
    result = list(sections)
    outline = footprint
    for index, distance in openings:
        section = result[index]
        neighbour = result[1 if index == 0 else -2]
        width = math.dist(section.left, section.right)
        if width < 1e-7:
            return None
        normal = (
            (section.left[1] - section.right[1]) / width,
            (section.right[0] - section.left[0]) / width,
        )
        towards = (neighbour.center[0] - section.center[0], neighbour.center[1] - section.center[1])
        if normal[0] * towards[0] + normal[1] * towards[1] > 0.0:
            normal = (-normal[0], -normal[1])
        shift = (normal[0] * distance, normal[1] * distance)
        left = (section.left[0] + shift[0], section.left[1] + shift[1])
        right = (section.right[0] + shift[0], section.right[1] + shift[1])
        try:
            strip = _horizontal_face((section.left, section.right, right, left), elevation)
            for post in posts:
                strip = strip.cut(_horizontal_face(_without_duplicate_points(post), elevation))
            outline = _planar_union((outline, strip))
        except (Part.OCCError, ValueError):
            return None
        # The section runs along the floor's edge, the outline's edges in
        # line with it there.
        direction = ((left[0] - right[0]) / width, (left[1] - right[1]) / width)

        def along(point):
            return (point[0] - right[0]) * direction[0] + (point[1] - right[1]) * direction[1]

        def on_line(point):
            return abs(_cross((point[0] - right[0], point[1] - right[1]), direction)) < 1e-6

        runs = sorted(
            sorted((along(edge.Vertexes[0].Point), along(edge.Vertexes[-1].Point)))
            for edge in outline.OuterWire.Edges
            if on_line(edge.Vertexes[0].Point) and on_line(edge.Vertexes[-1].Point)
        )
        merged = []
        for low, high in runs:
            if merged and low <= merged[-1][1] + 1e-6:
                merged[-1][1] = max(merged[-1][1], high)
            else:
                merged.append([low, high])
        if not merged:
            return None
        low, high = min(
            merged, key=lambda run: max(run[0] - width / 2.0, width / 2.0 - run[1], 0.0)
        )
        right = (right[0] + direction[0] * low, right[1] + direction[1] * low)
        left = (right[0] + direction[0] * (high - low), right[1] + direction[1] * (high - low))
        width = math.dist(left, right)
        if width < 1e-7:
            return None
        result[index] = replace(
            section,
            left=left,
            right=right,
            width=width,
            center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
            tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
        )
    try:
        faces = balanced_tread_faces(result, outline)
    except ValueError:
        return None
    if not balanced_partition_is_valid(faces, outline, len(result) - 1):
        return None
    return result, outline


def _planar_union(faces):
    """Return the one plan face covering ``faces`` lying in one plane.

    Fusing coplanar faces leaves them apart; a thin extrusion of each fuses
    into one solid, whose lower face is their union.
    """

    faces = [face for shape in faces for face in shape.Faces]
    elevation = min(face.BoundBox.ZMin for face in faces)
    solid = faces[0].extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
    for face in faces[1:]:
        solid = solid.fuse(face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0)))
    solid = solid.removeSplitter()
    lower = [
        face
        for face in solid.Faces
        if face.BoundBox.ZLength < 1e-7 and abs(face.BoundBox.ZMin - elevation) < 1e-7
    ]
    if len(lower) != 1:
        raise ValueError("The faces do not join")
    return lower[0]


def _clip_chord_to_boundary(left, right, boundary):
    """Keep only the connected part of a nosing that lies in the footprint."""

    midpoint = ((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0)
    chord = (left[0] - right[0], left[1] - right[1])
    chord_length = math.hypot(chord[0], chord[1])
    if chord_length < 1e-9:
        return left, right
    direction = (chord[0] / chord_length, chord[1] / chord_length)
    parameters = []
    vertices = boundary["vertices"]
    for first, second in zip(vertices, vertices[1:] + vertices[:1]):
        edge = (second[0] - first[0], second[1] - first[1])
        relative = (first[0] - midpoint[0], first[1] - midpoint[1])
        denominator = direction[0] * edge[1] - direction[1] * edge[0]
        if abs(denominator) < 1e-12:
            if abs(relative[0] * direction[1] - relative[1] * direction[0]) < 1e-7:
                parameters.extend(
                    (
                        relative[0] * direction[0] + relative[1] * direction[1],
                        (second[0] - midpoint[0]) * direction[0]
                        + (second[1] - midpoint[1]) * direction[1],
                    )
                )
            continue
        edge_ratio = (relative[0] * direction[1] - relative[1] * direction[0]) / denominator
        if -1e-9 <= edge_ratio <= 1.0 + 1e-9:
            parameters.append((relative[0] * edge[1] - relative[1] * edge[0]) / denominator)
    parameters.sort()
    unique = []
    for parameter in parameters:
        if not unique or abs(parameter - unique[-1]) > 1e-7:
            unique.append(parameter)
    intervals = []
    for lower, upper in zip(unique, unique[1:]):
        probe = (lower + upper) / 2.0
        point = (
            midpoint[0] + direction[0] * probe,
            midpoint[1] + direction[1] * probe,
        )
        if _point_in_boundary(point, boundary):
            intervals.append((lower, upper))
    if not intervals:
        return left, right
    lower, upper = min(
        intervals,
        key=lambda interval: (
            (
                0.0
                if interval[0] - 1e-7 <= 0.0 <= interval[1] + 1e-7
                else min(abs(interval[0]), abs(interval[1]))
            ),
            -(interval[1] - interval[0]),
        ),
    )
    return (
        (midpoint[0] + direction[0] * upper, midpoint[1] + direction[1] * upper),
        (midpoint[0] + direction[0] * lower, midpoint[1] + direction[1] * lower),
    )


def _point_in_boundary(point, boundary):
    inside = False
    previous = boundary["vertices"][-1]
    for current in boundary["vertices"]:
        if (current[1] > point[1]) != (previous[1] > point[1]):
            crossing = (previous[0] - current[0]) * (point[1] - current[1]) / (
                previous[1] - current[1]
            ) + current[0]
            if point[0] < crossing:
                inside = not inside
        previous = current
    return inside


def balanced_tread_faces(sections, footprint):
    """Partition a stair footprint into consecutive, non-overlapping faces."""

    if len(sections) < 2 or footprint.isNull():
        return []
    if any(
        abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
        for edge in footprint.Edges
    ):
        return _tangent_tread_faces(sections, footprint)
    boundary = _boundary_data(footprint)
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    left_parameters = _unwrap_boundary_parameters([section.left for section in sections], boundary)
    right_parameters = _unwrap_boundary_parameters(
        [section.right for section in sections], boundary
    )
    left_direction = -orientation
    right_direction = orientation

    faces = []
    for index, (front, rear) in enumerate(zip(sections, sections[1:])):
        points = [front.left, front.right]
        points.extend(
            _boundary_vertices_between(
                right_parameters[index],
                right_parameters[index + 1],
                right_direction,
                boundary,
            )
        )
        points.extend((rear.right, rear.left))
        points.extend(
            _boundary_vertices_between(
                left_parameters[index + 1],
                left_parameters[index],
                -left_direction,
                boundary,
            )
        )
        faces.append(_horizontal_face(_without_duplicate_points(points), 0.0))
    return faces


def _tangent_tread_faces(sections, footprint):
    """Partition a curved footprint with consecutive tangent half-planes."""

    result = []
    for front, rear in zip(sections, sections[1:]):
        candidates = _balanced_step_faces(front, rear, footprint)
        if not candidates:
            result.append(Part.Shape())
            continue
        tread = candidates[0]
        for candidate in candidates[1:]:
            tread = tread.fuse(candidate)
        result.append(tread.removeSplitter())
    return result


def balanced_partition_is_valid(faces, footprint, expected_count):
    """Return whether a footprint partition is safe to turn into solids."""

    if len(faces) != expected_count or footprint.isNull():
        return False
    if any(face.isNull() or not face.isValid() for face in faces):
        return False
    tolerance = max(footprint.Area * 1e-7, 1e-3)
    return abs(sum(face.Area for face in faces) - footprint.Area) <= tolerance


def _boundary_data(footprint):
    face = max(footprint.Faces, key=lambda item: item.Area)
    vertices = [(vertex.Point.x, vertex.Point.y) for vertex in face.OuterWire.OrderedVertexes]
    cumulative = [0.0]
    for first, second in zip(vertices, vertices[1:] + vertices[:1]):
        cumulative.append(cumulative[-1] + math.hypot(second[0] - first[0], second[1] - first[1]))
    signed_area = 0.5 * sum(
        first[0] * second[1] - second[0] * first[1]
        for first, second in zip(vertices, vertices[1:] + vertices[:1])
    )
    return {
        "vertices": vertices,
        "cumulative": cumulative,
        "perimeter": cumulative[-1],
        "signed_area": signed_area,
    }


def _boundary_candidates(point, boundary):
    candidates = []
    vertices = boundary["vertices"]
    cumulative = boundary["cumulative"]
    scale = max(boundary["perimeter"], 1.0)
    tolerance = scale * 1e-7
    for index, (first, second) in enumerate(zip(vertices, vertices[1:] + vertices[:1])):
        delta = (second[0] - first[0], second[1] - first[1])
        squared_length = delta[0] * delta[0] + delta[1] * delta[1]
        if squared_length < 1e-18:
            continue
        ratio = (
            (point[0] - first[0]) * delta[0] + (point[1] - first[1]) * delta[1]
        ) / squared_length
        ratio = min(max(ratio, 0.0), 1.0)
        projected = (
            first[0] + delta[0] * ratio,
            first[1] + delta[1] * ratio,
        )
        if math.hypot(projected[0] - point[0], projected[1] - point[1]) <= tolerance:
            candidates.append(cumulative[index] + math.sqrt(squared_length) * ratio)
    if not candidates:
        raise ValueError("Balanced section does not meet the stair footprint boundary")
    return candidates


def _unwrap_boundary_parameters(points, boundary):
    perimeter = boundary["perimeter"]
    result = []
    for point in points:
        candidates = _boundary_candidates(point, boundary)
        if not result:
            result.append(min(candidates))
            continue
        previous = result[-1]
        expanded = []
        for candidate in candidates:
            cycle = round((previous - candidate) / perimeter)
            expanded.extend(candidate + (cycle + offset) * perimeter for offset in (-1, 0, 1))
        result.append(min(expanded, key=lambda value: abs(value - previous)))
    return result


def _monotone_boundary_parameters(points, sections, boundary, direction):
    raw = _unwrap_boundary_parameters(points, boundary)
    start = raw[0]
    total = direction * (raw[-1] - start)
    if total <= 1e-7:
        total += boundary["perimeter"]
    tolerance = max(boundary["perimeter"] * 1e-7, 1e-7)
    progress = []
    for parameter in raw:
        value = direction * (parameter - start)
        # An endpoint outside the rail span lies on the opposite rail or on
        # an entry/exit edge, e.g. a section that crosses a whole outgoing
        # flight because the incoming inner side is shorter than its going.
        # Its position says nothing about this rail; clamping it to a rail end
        # would pull the neighbouring nosings onto it.
        progress.append(
            min(max(value, 0.0), total) if -tolerance <= value <= total + tolerance else None
        )
    progress[0] = 0.0
    progress[-1] = total
    progress = _interpolate_missing_progress(progress, sections)
    monotone = _isotonic_increasing(progress)
    first_station = sections[0].station
    station_span = max(sections[-1].station - first_station, 1e-9)
    result = []
    for index, (section, fitted) in enumerate(zip(sections, monotone)):
        linear = total * (section.station - first_station) / station_span
        blended = fitted * 0.9 + linear * 0.1
        if index == 0:
            blended = 0.0
        elif index == len(sections) - 1:
            blended = total
        result.append(start + direction * blended)
    return result


def _interpolate_missing_progress(progress, sections):
    """Fill unknown rail progress linearly by walking-line station."""

    known = [index for index, value in enumerate(progress) if value is not None]
    result = list(progress)
    for index, value in enumerate(progress):
        if value is not None:
            continue
        before = max(item for item in known if item < index)
        after = min(item for item in known if item > index)
        span = sections[after].station - sections[before].station
        ratio = (
            (sections[index].station - sections[before].station) / span
            if abs(span) > 1e-9
            else (index - before) / (after - before)
        )
        result[index] = progress[before] + (progress[after] - progress[before]) * ratio
    return result


def _isotonic_increasing(values):
    blocks = []
    last_index = len(values) - 1
    for index, value in enumerate(values):
        weight = 1e12 if index in (0, last_index) else 1.0
        blocks.append([index, index, weight, value * weight])
        while len(blocks) > 1:
            previous = blocks[-2]
            current = blocks[-1]
            if previous[3] / previous[2] <= current[3] / current[2]:
                break
            blocks[-2:] = [
                [
                    previous[0],
                    current[1],
                    previous[2] + current[2],
                    previous[3] + current[3],
                ]
            ]
    result = [0.0] * len(values)
    for start, end, weight, weighted_value in blocks:
        value = weighted_value / weight
        for index in range(start, end + 1):
            result[index] = value
    result[0] = values[0]
    result[-1] = values[-1]
    return result


def _boundary_point(parameter, boundary):
    perimeter = boundary["perimeter"]
    parameter %= perimeter
    cumulative = boundary["cumulative"]
    vertices = boundary["vertices"]
    index = min(
        bisect.bisect_right(cumulative, parameter) - 1,
        len(vertices) - 1,
    )
    start = cumulative[index]
    end = cumulative[index + 1]
    ratio = (parameter - start) / (end - start) if end > start else 0.0
    first = vertices[index]
    second = vertices[(index + 1) % len(vertices)]
    return (
        first[0] + (second[0] - first[0]) * ratio,
        first[1] + (second[1] - first[1]) * ratio,
    )


def _boundary_vertices_between(start, end, direction, boundary):
    perimeter = boundary["perimeter"]
    lower = min(start, end)
    upper = max(start, end)
    candidates = []
    first_cycle = math.floor(lower / perimeter) - 1
    last_cycle = math.ceil(upper / perimeter) + 1
    for cycle in range(first_cycle, last_cycle + 1):
        for parameter, point in zip(boundary["cumulative"][:-1], boundary["vertices"]):
            unwrapped = parameter + cycle * perimeter
            if lower + 1e-7 < unwrapped < upper - 1e-7:
                candidates.append((unwrapped, point))
    candidates.sort(key=lambda item: item[0], reverse=direction < 0.0)
    return [point for _parameter, point in candidates]


def _without_duplicate_points(points):
    result = []
    for point in points:
        if not result or math.hypot(point[0] - result[-1][0], point[1] - result[-1][1]) > 1e-7:
            result.append(point)
    if (
        len(result) > 1
        and math.hypot(result[0][0] - result[-1][0], result[0][1] - result[-1][1]) < 1e-7
    ):
        result.pop()
    return result


def _horizontal_face(points, elevation):
    vectors = [FreeCAD.Vector(point[0], point[1], elevation) for point in points]
    vectors.append(vectors[0])
    return Part.Face(Part.makePolygon(vectors))


def _balanced_step_faces(
    front,
    rear,
    footprint,
    front_offset=0.0,
    rear_offset=0.0,
):
    front_center = _shifted(front.center, front.tangent, front_offset)
    rear_center = _shifted(rear.center, rear.tangent, rear_offset)
    extent = max(
        footprint.BoundBox.DiagonalLength * 4.0,
        abs(front_offset) + abs(rear_offset) + 1000.0,
    )
    after_front = _half_plane_face(front_center, front.tangent, extent)
    before_rear = _half_plane_face(
        rear_center,
        (-rear.tangent[0], -rear.tangent[1]),
        extent,
    )
    clipped = footprint.common(after_front).common(before_rear)
    candidates = clipped.Faces
    if not candidates:
        return []
    midpoint = FreeCAD.Vector(
        (front.center[0] + rear.center[0]) / 2.0,
        (front.center[1] + rear.center[1]) / 2.0,
        0.0,
    )
    for face in candidates:
        if face.isInside(midpoint, 1e-6, True):
            seed = face
            break
    else:
        point = Part.Vertex(midpoint)
        seed = min(candidates, key=lambda face: face.distToShape(point)[0])

    selected = [seed]
    remaining = [face for face in candidates if not face.isSame(seed)]
    changed = True
    while changed:
        changed = False
        for face in list(remaining):
            if any(face.distToShape(current)[0] < 1e-7 for current in selected):
                selected.append(face)
                remaining.remove(face)
                changed = True
    return selected


def _half_plane_face(center, forward, extent):
    normal = (-forward[1], forward[0])
    start_left = (
        center[0] + normal[0] * extent,
        center[1] + normal[1] * extent,
    )
    start_right = (
        center[0] - normal[0] * extent,
        center[1] - normal[1] * extent,
    )
    return _horizontal_face(
        (
            start_left,
            start_right,
            (
                start_right[0] + forward[0] * extent,
                start_right[1] + forward[1] * extent,
            ),
            (
                start_left[0] + forward[0] * extent,
                start_left[1] + forward[1] * extent,
            ),
        ),
        0.0,
    )
