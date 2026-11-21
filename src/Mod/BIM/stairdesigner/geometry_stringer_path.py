# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stringer paths, profiles, and flight-run preparation."""

from dataclasses import replace
import math
from typing import NamedTuple

import FreeCAD
import Part

from .geometry_core import (
    BalancedSection,
    _CircularProfile,
    _cross,
    _dot,
    balanced_section_top,
)
from .geometry_fairing import FairCurve, Pin, fair_curve


def straight_stringer_sections(
    metrics,
    width,
    tread_goings=None,
    top_elevations=None,
):
    """Return balanced-section compatible stations for one straight flight."""

    sections = []
    width = max(float(width), 0.01)
    goings = list(tread_goings or [])
    if len(goings) != metrics.tread_count:
        goings = [metrics.tread_width] * metrics.tread_count
    stations = [0.0]
    for going in goings:
        stations.append(stations[-1] + going)
    elevations = list(top_elevations or [])
    if len(elevations) != metrics.tread_count + 1:
        elevations = [
            (index + 1) * metrics.riser_height for index in range(metrics.tread_count + 1)
        ]
    for index in range(metrics.tread_count + 1):
        station = stations[index]
        sections.append(
            BalancedSection(
                center=(station, width / 2.0),
                tangent=(1.0, 0.0),
                left=(station, width),
                right=(station, 0.0),
                station=station,
                width=width,
                flight_index=0,
                riser_index=index + 1,
                top_elevation=elevations[index],
            )
        )
    return sections


def _stringer_inward(section, side):
    rail = section.left if side == "Left" else section.right
    inward = (
        section.center[0] - rail[0],
        section.center[1] - rail[1],
    )
    length = math.hypot(inward[0], inward[1])
    if length < 1e-9:
        if side == "Left":
            return (section.tangent[1], -section.tangent[0])
        return (-section.tangent[1], section.tangent[0])
    return (inward[0] / length, inward[1] / length)


def _stringer_cross_section(
    section,
    side,
    bottom,
    top,
    thickness,
    offset,
    housed,
):
    rail = section.left if side == "Left" else section.right
    inward = _stringer_inward(section, side)
    first = (
        rail[0] + inward[0] * offset,
        rail[1] + inward[1] * offset,
    )
    thickness_direction = -1.0 if housed else 1.0
    second = (
        first[0] + inward[0] * thickness * thickness_direction,
        first[1] + inward[1] * thickness * thickness_direction,
    )
    top = max(float(top), float(bottom) + 0.01)
    points = (
        FreeCAD.Vector(first[0], first[1], bottom),
        FreeCAD.Vector(second[0], second[1], bottom),
        FreeCAD.Vector(second[0], second[1], top),
        FreeCAD.Vector(first[0], first[1], top),
    )
    return Part.makePolygon((*points, points[0]))


# Largest amount a straight pitch may stand above any nosing it serves
# before the edge is sprung to a curve instead.
PROFILE_FIT_TOLERANCE = 20.0
# Stiffness of the batten a curved edge is sprung with: a hollow between
# nosings much shorter than this is bridged, a longer one is followed.
PROFILE_FAIRING_LENGTH = 400.0
# Nosings rising faster than this along a board converge on a newel.
PROFILE_MAXIMUM_PITCH = 1.5
# Default timber left under the step housings of an automatic-width board.
STRINGER_TIMBER_BELOW_STEPS = 50.0
# Where no step bears on a board, its lower edge is drawn towards the
# regular width at points this far apart, this strongly beside a housing.
PROFILE_GUIDE_SPACING = 250.0
PROFILE_GUIDE_WEIGHT = 0.3
# A lower edge kept parallel to the upper one changes level into a corner
# block over at least this length, and no steeper than this added pitch.
CORNER_RAMP_PITCH = 0.25
CORNER_RAMP_MINIMUM = 250.0


class EdgeBlock(NamedTuple):
    """Where a board's edges run level into a corner block.

    The block takes the board's end from ``at`` on: a plan point for a
    board, a position along it for its profile. ``top`` and ``bottom`` are
    the levels the upper and the lower edge reach there. Without a ``top``
    the upper edge takes the lowest level clearing the board's own nosings
    at the block, and without a ``bottom`` the lower one the level it comes
    to by itself; a ``bottom`` of ``False`` leaves the lower edge alone.
    """

    at: object
    top: object = None
    bottom: object = None


def _fit_profile_line(points):
    """Fit one pitch through ``points``, raised to clear every one of them."""

    if len(points) == 1:
        return 0.0, points[0][1], 0.0
    mean_u = sum(u for u, _ in points) / len(points)
    mean_z = sum(z for _, z in points) / len(points)
    variance = sum((u - mean_u) ** 2 for u, _ in points)
    slope = (
        sum((u - mean_u) * (z - mean_z) for u, z in points) / variance if variance > 1e-9 else 0.0
    )
    intercept = max(z - slope * u for u, z in points)
    deviation = max(intercept + slope * u - z for u, z in points)
    return slope, intercept, deviation


def _bearing_points(parameters, values, carried=None):
    """Return the ``(u, z)`` points a board's upper edge has to clear.

    Several points at (almost) the same board position mark fan treads
    meeting at a newel: only the lowest one bears on the board, the others
    are carried by the post, so they must not bend the board's upper edge.

    ``carried`` flags the points known to be carried by a newel post. When it
    is given, only those are left off the board: steep winder nosings beside
    a post are still housed in the board and keep its upper edge above them.
    """

    if carried is not None:
        kept = [(u, z) for u, z, flag in zip(parameters, values, carried) if not flag]
        if len(kept) >= 2:
            parameters, values = zip(*kept)
    points = []
    for u, z in sorted(zip(parameters, values)):
        if points and (
            u - points[-1][0] < 1.0
            or (carried is None and z - points[-1][1] > PROFILE_MAXIMUM_PITCH * (u - points[-1][0]))
        ):
            points[-1] = (points[-1][0], min(points[-1][1], z))
        else:
            points.append((u, z))
    return points


def stringer_straight_pitch(parameters, values, single=False, carried=None):
    """Return the one pitch a board's upper edge runs at, or ``None``.

    The edge is straight when ``single`` asks for it, or when one pitch over
    the nosings stands no more than ``PROFILE_FIT_TOLERANCE`` above any of
    them. Otherwise it is sprung to a curve, see ``_upper_edge``.
    """

    points = _bearing_points(parameters, values, carried)
    slope, _intercept, deviation = _fit_profile_line(points)
    if single or len(points) < 3 or deviation <= PROFILE_FIT_TOLERANCE:
        return slope
    return None


def _sprung_pins(points, low, high, guides=()):
    """Return the pins of a batten sprung over ``points`` from ``low`` to
    ``high``, with the highest point bearing on each end.

    Points at an end or beyond it, in a corner block, bear on that end.
    ``guides`` are ``(u, value)`` points only drawing the batten, where no
    point bears within a millimetre.
    """

    bearing = {low: [], high: []}
    inner = []
    for u, value in sorted(points):
        if u <= low + 1.0:
            bearing[low].append(value)
        elif u >= high - 1.0:
            bearing[high].append(value)
        elif inner and u - inner[-1][0] < 1.0:
            inner[-1] = (inner[-1][0], max(inner[-1][1], value))
        else:
            inner.append((u, value))
    pins = [Pin(u, value) for u, value in inner]
    for end in (low, high):
        if bearing[end]:
            pins.append(Pin(end, max(bearing[end])))
    taken = [pin.u for pin in pins]
    for u, value in guides:
        u = min(max(u, low), high)
        if all(abs(u - other) >= 1.0 for other in taken):
            pins.append(Pin(u, value, False, PROFILE_GUIDE_WEIGHT))
            taken.append(u)
    for end in (low, high):
        if all(abs(end - other) >= 1.0 for other in taken):
            # Only an end of the curve: nothing bears on it or draws it.
            pins.append(Pin(end, 0.0, False, 0.0))
            taken.append(end)
    pins.sort()
    return pins, (
        max(bearing[low]) if bearing[low] else None,
        max(bearing[high]) if bearing[high] else None,
    )


def _upper_edge(points, u_start, u_end, single=False, start_block=None, end_block=None):
    """Return a board's upper edge over the ``points`` it has to clear.

    One straight pitch serves when it stands close enough above them all,
    as along a straight flight. Winders space the nosings unevenly along a
    board: there the edge is the fair curve of a batten sprung over them,
    which follows their general line and stands off some of them rather
    than wobble from one to the next. Into a corner block the edge runs
    level, see ``EdgeBlock``. ``single`` keeps a board too short to follow
    its nosings to one pitch, or as stiff as its blocks allow.
    """

    slope, intercept, deviation = _fit_profile_line(points)
    straight = FairCurve.line(u_start, u_end, slope, intercept)
    blocked = start_block is not None or end_block is not None
    if not blocked and (single or len(points) < 3 or deviation <= PROFILE_FIT_TOLERANCE):
        return straight
    low = start_block.at if start_block is not None else u_start
    high = end_block.at if end_block is not None else u_end
    if high - low < 2.0:
        return straight
    pins, bearing = _sprung_pins(points, low, high)
    levels = [
        None if block is None else float(block.top) if block.top is not None else level
        for block, level in zip((start_block, end_block), bearing)
    ]
    try:
        return fair_curve(
            pins,
            PROFILE_FAIRING_LENGTH * (4.0 if single else 1.0),
            start_slope=None if start_block is None else 0.0,
            end_slope=None if end_block is None else 0.0,
            start_value=levels[0],
            end_value=levels[1],
        )
    except ValueError:
        return straight


def _square_below(top, u, width):
    """Return the height at ``u`` of the edge ``width`` square below ``top``.

    That edge is the envelope of circles of radius ``width`` centred on
    ``top``: unlike an offset curve it cannot loop under a tight crest.
    """

    def height(s):
        return top.value(s) - math.sqrt(max(width * width - (u - s) ** 2, 0.0))

    count = 32
    stations = [u - width + 2.0 * width * index / count for index in range(count + 1)]
    best = min(range(count + 1), key=lambda index: height(stations[index]))
    low = stations[max(best - 1, 0)]
    high = stations[min(best + 1, count)]
    ratio = (math.sqrt(5.0) - 1.0) / 2.0
    for _step in range(40):
        first = high - ratio * (high - low)
        second = low + ratio * (high - low)
        if height(first) < height(second):
            high = second
        else:
            low = first
    return height((low + high) / 2.0)


def _lower_edge_parallel(top, u_start, u_end, width, start_block=None, end_block=None):
    """Return a board's lower edge, ``width`` square below its upper one.

    A corner block's ``bottom`` level is reached by leaving the parallel
    some way before the block and running level into it.
    """

    if top.is_straight():
        slope = top.slope(u_start)
        return FairCurve.line(
            u_start, u_end, slope, top.value(0.0) - width * math.sqrt(1.0 + slope * slope)
        )
    count = max(int(math.ceil((u_end - u_start) / 50.0)), 2)
    points = [
        (u, _square_below(top, u, width))
        for u in (u_start + (u_end - u_start) * index / count for index in range(count + 1))
    ]
    slopes = [None, None]
    for index, block in enumerate((start_block, end_block)):
        if block is None or block.bottom is None or block.bottom is False:
            continue
        level = float(block.bottom)
        natural = _square_below(top, block.at, width)
        ramp = max(CORNER_RAMP_MINIMUM, abs(level - natural) / CORNER_RAMP_PITCH)
        if index == 0:
            kept = [point for point in points if point[0] > block.at + ramp]
            points = [(block.at, level)] + (kept or points[-1:])
        else:
            kept = [point for point in points if point[0] < block.at - ramp]
            points = (kept or points[:1]) + [(block.at, level)]
        slopes[index] = 0.0
    return FairCurve.through(points, slopes[0], slopes[1])


def _lower_edge_under_supports(
    top,
    supports,
    u_start,
    u_end,
    width,
    single=False,
    guided_lead=True,
    below=STRINGER_TIMBER_BELOW_STEPS,
    below_direction="Perpendicular",
    start_block=None,
    end_block=None,
):
    """Fit a board's lower edge under the steps it houses.

    ``supports`` are ``(u, z)`` housing corners, one per step. The lower
    edge is sprung under them exactly as the upper edge ``top`` is sprung
    over the nosings, standing ``below`` each corner, measured as
    ``below_direction`` says: vertically, or square to the board (taken as
    the upper edge's pitch above it). It then goes only as deep as its own
    steps need, wherever winders bunch or spread them, in one fair curve.

    Before the first corner and after the last one no step bears on the
    board: there the edge is drawn towards the regular one, ``width`` square
    below the upper edge, which keeps a board starting at a corner level
    with its neighbour and takes one starting on the floor down to it.
    Without ``guided_lead`` it runs on straight before the first corner, as
    under the fan treads of a board starting at a newel. Into a corner
    block it runs level, see ``EdgeBlock``. Returns ``None`` with fewer
    than two corners on the board.
    """

    below = max(float(below), 0.0)
    limits = {}
    for u, z in supports:
        # A corner just past an end, in the board's end cut, binds there.
        u = min(max(u, u_start), u_end)
        slope = top.slope(u)
        if str(below_direction) == "Vertical":
            limit = z - below
        else:
            limit = z - below * math.sqrt(1.0 + slope * slope)
        # Corners closer than a millimetre share a pin; keep the lowest.
        key = round(u)
        if key not in limits or limit < limits[key][1]:
            limits[key] = (u, limit)
    if len(limits) < 2:
        return None
    # Spring the mirrored edge as an upper edge over the mirrored corners.
    mirrored = [(u, -limit) for u, limit in sorted(limits.values())]

    def regular(u):
        slope = top.slope(u)
        return top.value(u) - width * math.sqrt(1.0 + slope * slope)

    blocks = [
        None if block is None or block.bottom is False else block
        for block in (start_block, end_block)
    ]
    slope, intercept, deviation = _fit_profile_line(mirrored)
    if blocks == [None, None] and (
        single
        or (
            top.is_straight()
            and deviation <= PROFILE_FIT_TOLERANCE
            and (
                not guided_lead
                # The pitch reaches the floor, or stays about the regular
                # edge, before the first corner.
                or -(intercept + slope * u_start) <= 0.0
                or -(intercept + slope * u_start) >= regular(u_start) - PROFILE_FIT_TOLERANCE
            )
        )
    ):
        return FairCurve.line(u_start, u_end, -slope, -intercept)
    low = blocks[0].at if blocks[0] is not None else u_start
    high = blocks[1].at if blocks[1] is not None else u_end
    if high - low < 2.0:
        return None
    first, last = mirrored[0][0], mirrored[-1][0]
    stations = []
    if guided_lead:
        station = first - PROFILE_GUIDE_SPACING
        while station > low + 1.0:
            stations.append(station)
            station -= PROFILE_GUIDE_SPACING
        if first > low + 1.0:
            stations.append(low)
    station = last + PROFILE_GUIDE_SPACING
    while station < high - 1.0:
        stations.append(station)
        station += PROFILE_GUIDE_SPACING
    if last < high - 1.0:
        stations.append(high)
    pins, _bearing = _sprung_pins(mirrored, low, high, [(u, -regular(u)) for u in stations])
    try:
        return fair_curve(
            pins,
            PROFILE_FAIRING_LENGTH * (4.0 if single else 1.0),
            start_slope=None if blocks[0] is None else 0.0,
            end_slope=None if blocks[1] is None else 0.0,
            start_value=(
                None if blocks[0] is None or blocks[0].bottom is None else -float(blocks[0].bottom)
            ),
            end_value=(
                None if blocks[1] is None or blocks[1].bottom is None else -float(blocks[1].bottom)
            ),
        ).negated()
    except ValueError:
        return None


def notched_stringer_throat(riser_height, going, step_thickness, width, riser_clearance=0.0):
    """Return the timber a notched board keeps under the cut of a regular
    step, square to the board.

    What is left under the cuts is all that carries a notched board. Its
    ``width``, measured square below the line of the nosings, leaves this
    much of it under a step of the stair's ``going``. Winders stretch a
    step along the board, which deepens its cut: the board keeps at least
    this under those cuts too, see ``notched_board_edges``.
    """

    going = max(float(going), 0.01)
    riser_height = max(float(riser_height), 0.0)
    cosine = going / math.hypot(going, riser_height)
    # The cut's inner corner lies a rise, a tread and the riser's room
    # under the line of the nosings.
    depth = (
        riser_height
        + max(float(step_thickness), 0.0)
        + riser_height / going * max(float(riser_clearance), 0.0)
    ) * cosine
    return max(float(width) - depth, 0.0)


def notched_board_edges(
    parameters, values, width, u_start, u_end, cuts=(), throat=0.0, carried=None
):
    """Return a notched board's ``(upper, lower)`` edges as fair curves.

    The upper one is the line of the nosings at ``values``, which the
    board's cuts hang from; the lower edge runs ``width`` square below it.
    ``cuts`` are the ``(u, z)`` inner corners of the cuts. Along a regular
    flight that edge leaves the same timber under every one of them, and
    is kept as it is. Winders stretch some steps along the board and
    deepen their cuts: the lower edge is then sprung under the cuts as a
    housed board's is under its housings, leaving under each what the
    board's shallowest cut has, and no less than ``throat``, square to the
    board. ``carried`` flags the nosings of fan treads that a newel post
    carries.

    Cuts climbing almost plumb along a short board are those of a fan of
    winders around a newel: only the lowest of them bears on the board. A
    board left with fewer than two cuts keeps to its ``width`` below the
    nosings that bear on it.
    """

    width = float(width)
    steepest = 2.0 * PROFILE_MAXIMUM_PITCH
    bearing = []
    for u, z in sorted(cuts):
        if bearing and (
            u - bearing[-1][0] < 1.0 or z - bearing[-1][1] > steepest * (u - bearing[-1][0])
        ):
            bearing[-1] = (bearing[-1][0], min(bearing[-1][1], z))
        else:
            bearing.append((u, z))
    if len(bearing) >= 2:
        top = _upper_edge(_bearing_points(parameters, values, carried), u_start, u_end)
        regular = _lower_edge_parallel(top, u_start, u_end, width)
        timbers = [(z - regular.value(u)) / math.sqrt(1.0 + top.slope(u) ** 2) for u, z in bearing]
        if max(timbers) - min(timbers) < 1.0:
            return top, regular
        lower = _lower_edge_under_supports(
            top, bearing, u_start, u_end, width, below=max(float(throat), max(timbers))
        )
        if lower is not None:
            return top, lower
    top = _upper_edge(_bearing_points(parameters, values), u_start, u_end)
    return top, _lower_edge_parallel(top, u_start, u_end, width)


def _curve_edges(origin, direction, curve, u_start, u_end):
    """Build the connected 3D edges of one board edge from ``u_start`` to
    ``u_end``: a line where it is straight, one spline where it curves."""

    def point(u):
        return FreeCAD.Vector(
            origin[0] + direction[0] * u,
            origin[1] + direction[1] * u,
            curve.value(u),
        )

    def line(first, second):
        if second - first < 1e-6:
            return []
        return [Part.makeLine(point(first), point(second))]

    low = min(max(curve.knots[0], u_start), u_end)
    high = max(min(curve.knots[-1], u_end), u_start)
    if curve.is_straight() or high - low < 1e-3:
        return line(u_start, u_end)
    stations = [low] + [knot for knot in curve.knots if low + 1e-3 < knot < high - 1e-3] + [high]
    # With the board position as parameter the spline is the curve itself:
    # it runs evenly along the board and takes the curve's heights.
    spline = Part.BSplineCurve()
    spline.interpolate(
        Points=[point(u) for u in stations],
        Parameters=stations,
        InitialTangent=FreeCAD.Vector(direction[0], direction[1], curve.slope(low)),
        FinalTangent=FreeCAD.Vector(direction[0], direction[1], curve.slope(high)),
        Scale=False,
    )
    return [*line(u_start, low), spline.toShape(), *line(high, u_end)]


def _edges_above_floor(origin, direction, top, bottom, u_start, u_end):
    """Build a board's lower edge, level along the floor where it dips under.

    Cutting the curve itself where it meets the floor, rather than the
    finished face, leaves no part of a spline under the floor: the board's
    bounding box then stands on it. A board whose upper edge reaches the
    floor too is left to the level cut of its face.
    """

    count = 64
    stations = [u_start + (u_end - u_start) * index / count for index in range(count + 1)]
    heights = [bottom.value(u) for u in stations]
    if min(heights) >= 0.0 or min(top.value(u) for u in stations) < 1.0:
        return _curve_edges(origin, direction, bottom, u_start, u_end)
    cuts = [u_start]
    for (low, low_height), (high, high_height) in zip(
        zip(stations, heights), zip(stations[1:], heights[1:])
    ):
        if (low_height < 0.0) == (high_height < 0.0):
            continue
        for _step in range(60):
            middle = (low + high) / 2.0
            if (bottom.value(middle) < 0.0) == (low_height < 0.0):
                low = middle
            else:
                high = middle
        cuts.append((low + high) / 2.0)
    cuts.append(u_end)
    edges = []
    for start, end in zip(cuts, cuts[1:]):
        if end - start < 1e-6:
            continue
        if bottom.value((start + end) / 2.0) < 0.0:
            edges.append(
                Part.makeLine(
                    FreeCAD.Vector(
                        origin[0] + direction[0] * start, origin[1] + direction[1] * start, 0.0
                    ),
                    FreeCAD.Vector(
                        origin[0] + direction[0] * end, origin[1] + direction[1] * end, 0.0
                    ),
                )
            )
        else:
            edges.extend(_curve_edges(origin, direction, bottom, start, end))
    return edges


def make_stringer_profile_face(
    origin,
    direction,
    parameters,
    tops,
    width,
    straight_profile=False,
    carried=None,
    supports=(),
    below=STRINGER_TIMBER_BELOW_STEPS,
    below_direction="Perpendicular",
    start_block=None,
    end_block=None,
):
    """Return the flat side face of a board running over ``tops``.

    The upper edge is one straight pitch along a straight flight and a fair
    curve over winders, see ``_upper_edge``; the lower edge runs at the
    board ``width`` measured square to it, the ends are plumb cuts, and
    anything below the floor is removed by a level cut.
    ``carried`` flags the tops of fan treads that a newel post carries.
    ``supports`` are ``(u, z)`` housing corners the lower edge must clear by
    ``below``, see ``_lower_edge_under_supports``. ``start_block`` and
    ``end_block`` are the ``EdgeBlock`` the board's ends run level into,
    placed along the board.
    """

    points = _bearing_points(parameters, tops, carried)
    u_start = parameters[0]
    u_end = parameters[-1]
    blocks = []
    for at_end, block in enumerate((start_block, end_block)):
        if block is not None:
            # A board may start or end right at its block, and still has to
            # arrive level there; one lying wholly in it has no edge left.
            at = min(max(block.at, u_start), u_end)
            room = at - u_start if at_end else u_end - at
            block = block._replace(at=at) if room > 2.0 else None
        blocks.append(block)
    top = _upper_edge(points, u_start, u_end, straight_profile, *blocks)
    width = float(width)

    def clear(edge):
        # A lower edge crossing the upper one leaves no board.
        return edge is not None and all(
            top.value(u) - edge.value(u) > 1.0
            for u in (u_start + (u_end - u_start) * index / 32.0 for index in range(33))
        )

    bottom = _lower_edge_under_supports(
        top,
        supports,
        u_start,
        u_end,
        width,
        straight_profile,
        # A board starting at a newel runs on straight under its fan treads.
        guided_lead=not (carried and carried[0]),
        below=below,
        below_direction=below_direction,
        start_block=blocks[0],
        end_block=blocks[1],
    )
    if not clear(bottom):
        bottom = _lower_edge_parallel(top, u_start, u_end, width, *blocks)
    top_edges = _curve_edges(origin, direction, top, u_start, u_end)
    bottom_edges = _edges_above_floor(origin, direction, top, bottom, u_start, u_end)
    top_start = top_edges[0].Vertexes[0].Point
    top_end = top_edges[-1].Vertexes[-1].Point
    bottom_start = bottom_edges[0].Vertexes[0].Point
    bottom_end = bottom_edges[-1].Vertexes[-1].Point
    reversed_bottom = []
    for edge in reversed(bottom_edges):
        reversed_edge = edge.copy()
        reversed_edge.reverse()
        reversed_bottom.append(reversed_edge)
    face = Part.Face(
        Part.Wire(
            [
                *top_edges,
                Part.makeLine(top_end, bottom_end),
                *reversed_bottom,
                Part.makeLine(bottom_start, top_start),
            ]
        )
    )
    if face.BoundBox.ZMin < -1e-6:
        span = face.BoundBox.DiagonalLength + 1.0
        start = FreeCAD.Vector(
            origin[0] + direction[0] * (u_start - span),
            origin[1] + direction[1] * (u_start - span),
            0.0,
        )
        end = FreeCAD.Vector(
            origin[0] + direction[0] * (u_end + span),
            origin[1] + direction[1] * (u_end + span),
            0.0,
        )
        floor = Part.Face(
            Part.makePolygon(
                [
                    start,
                    end,
                    end + FreeCAD.Vector(0.0, 0.0, face.BoundBox.ZMax + span),
                    start + FreeCAD.Vector(0.0, 0.0, face.BoundBox.ZMax + span),
                    start,
                ]
            )
        )
        cut = face.common(floor).Faces
        if cut:
            face = cut[0]
    return face


def _make_planar_housed_stringer_shape(
    sections,
    tops,
    side,
    thickness,
    penetration,
    width,
    straight_profile=False,
    supports=(),
    below=STRINGER_TIMBER_BELOW_STEPS,
    below_direction="Perpendicular",
    start_block=None,
    end_block=None,
):
    """Extrude a stable, shape-preserving side profile for a flat board.

    ``supports`` are 3D housing corners the lower edge must clear by
    ``below``, measured as ``below_direction`` says. ``start_block`` and
    ``end_block`` are the ``EdgeBlock`` the board's ends run level into,
    placed in plan.
    """

    if len(sections) < 2 or len(sections) != len(tops):
        return None
    first_tangent = sections[0].tangent
    tangent_length = math.hypot(*first_tangent)
    if tangent_length < 1e-9:
        return None
    tangent = (
        first_tangent[0] / tangent_length,
        first_tangent[1] / tangent_length,
    )
    first_inward = _stringer_inward(sections[0], side)
    surface_points = []
    for section in sections:
        section_tangent = section.tangent
        section_length = math.hypot(*section_tangent)
        if section_length < 1e-9:
            return None
        section_tangent = (
            section_tangent[0] / section_length,
            section_tangent[1] / section_length,
        )
        if abs(_cross(tangent, section_tangent)) > 1e-7 or _dot(tangent, section_tangent) < 0.0:
            return None
        inward = _stringer_inward(section, side)
        if _dot(first_inward, inward) < 1.0 - 1e-7:
            return None
        rail = section.left if side == "Left" else section.right
        surface_points.append(
            (
                rail[0] + inward[0] * float(penetration),
                rail[1] + inward[1] * float(penetration),
            )
        )

    axis = (
        surface_points[-1][0] - surface_points[0][0],
        surface_points[-1][1] - surface_points[0][1],
    )
    axis_length = math.hypot(*axis)
    if axis_length < 1e-7:
        return None
    direction = (axis[0] / axis_length, axis[1] / axis_length)
    if _dot(direction, tangent) < 0.0:
        direction = (-direction[0], -direction[1])
    normal = (-direction[1], direction[0])
    origin = surface_points[0]
    tolerance = max(axis_length, 1.0) * 1e-7
    if any(
        abs((point[0] - origin[0]) * normal[0] + (point[1] - origin[1]) * normal[1]) > tolerance
        for point in surface_points
    ):
        return None

    parameters = [
        (point[0] - origin[0]) * direction[0] + (point[1] - origin[1]) * direction[1]
        for point in surface_points
    ]
    if parameters[-1] < 1e-7:
        return None
    if any(following <= previous + 1e-7 for previous, following in zip(parameters, parameters[1:])):
        # Aggressive winding can make the tread intersections backtrack
        # along a straight board.  A global loft then produces loops.  Keep
        # their distribution but restore the only physically meaningful
        # ordering before constructing the side profile.
        total = parameters[-1]
        ordered = sorted(min(max(parameter, 0.0), total) for parameter in parameters)
        blend = 1e-5
        parameters = [
            (1.0 - blend) * parameter + blend * total * index / (len(ordered) - 1)
            for index, parameter in enumerate(ordered)
        ]
        parameters[0] = 0.0
        parameters[-1] = total

    def along_board(block):
        if block is None:
            return None
        return block._replace(
            at=(block.at[0] - origin[0]) * direction[0] + (block.at[1] - origin[1]) * direction[1]
        )

    try:
        face = make_stringer_profile_face(
            origin,
            direction,
            parameters,
            [float(top) for top in tops],
            width,
            straight_profile,
            [bool(section.profile_at_newel) for section in sections],
            [
                (
                    (point[0] - origin[0]) * direction[0] + (point[1] - origin[1]) * direction[1],
                    point[2],
                )
                for point in supports
            ],
            below,
            below_direction,
            along_board(start_block),
            along_board(end_block),
        )
        result = face.extrude(
            FreeCAD.Vector(
                -first_inward[0] * float(thickness),
                -first_inward[1] * float(thickness),
                0.0,
            )
        )
    except (Part.OCCError, ValueError):
        return None
    if result.isNull() or not result.isValid() or len(result.Solids) != 1:
        return None
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def _stringer_elevations(sections, riser_height):
    return [
        balanced_section_top(section, index, riser_height) for index, section in enumerate(sections)
    ]


def _stringer_slope(sections, elevations):
    if len(sections) < 2:
        return 0.0
    run = 0.0
    rise = 0.0
    for first, second, first_top, second_top in zip(
        sections, sections[1:], elevations, elevations[1:]
    ):
        elevation_change = second_top - first_top
        if abs(elevation_change) < 1e-9:
            continue
        section_run = math.hypot(
            second.center[0] - first.center[0],
            second.center[1] - first.center[1],
        )
        if section_run < 1e-9:
            section_run = max(second.station - first.station, 0.0)
        run += section_run
        rise += elevation_change
    if run < 1e-9:
        return 0.0
    return rise / run


def automatic_stringer_width(
    riser_height,
    going,
    step_thickness,
    nosing,
    nosing_offset,
    offset_direction="Perpendicular",
    underside_offset=STRINGER_TIMBER_BELOW_STEPS,
    underside_direction="Perpendicular",
):
    """Return a practical board width with ``underside_offset`` below the
    step profile, each offset measured as its direction says."""

    going = max(float(going), 0.01)
    slope = math.atan(max(float(riser_height), 0.0) / going)
    slope_cosine = max(math.cos(slope), 0.01)
    slope_sine = math.sin(slope)
    upper = max(float(nosing_offset), 0.0)
    lower = max(float(underside_offset), 0.0)
    if str(offset_direction) == "Vertical":
        upper *= slope_cosine
    if str(underside_direction) == "Vertical":
        lower *= slope_cosine
    step_envelope = (
        max(float(riser_height), 0.0) + max(float(step_thickness), 0.0)
    ) * slope_cosine + max(float(nosing), 0.0) * slope_sine
    return max(235.0, upper + step_envelope + lower)


def stringer_flight_runs(sections, flight_types=None):
    """Return ``(flight_index, sections)`` for each stair-bearing flight."""

    sections = [
        (
            replace(section, riser_index=index + 1)
            if int(getattr(section, "riser_index", 0)) <= 0 and not section.level_to_next
            else section
        )
        for index, section in enumerate(sections)
    ]
    runs = []
    start = None
    flight_index = None
    for index, section in enumerate(sections[:-1]):
        if section.level_to_next:
            if start is not None and index > start:
                runs.append((flight_index, sections[start : index + 1]))
            section_type = (
                str(flight_types[section.flight_index])
                if flight_types and section.flight_index < len(flight_types)
                else ""
            )
            if section_type.endswith("landing"):
                runs.append(
                    (
                        section.flight_index,
                        [
                            replace(section, level_to_next=False),
                            replace(
                                sections[index + 1],
                                level_to_next=False,
                            ),
                        ],
                    )
                )
            start = None
            flight_index = None
            continue
        if start is None:
            start = index
            flight_index = section.flight_index
        elif section.flight_index != flight_index:
            previous_type = (
                str(flight_types[flight_index])
                if flight_types and flight_index < len(flight_types)
                else ""
            )
            following_type = (
                str(flight_types[section.flight_index])
                if flight_types and section.flight_index < len(flight_types)
                else ""
            )
            tangent_junction = previous_type.startswith("Circular") or following_type.startswith(
                "Circular"
            )
            if tangent_junction and index - start >= 2:
                runs.append((flight_index, sections[start:index]))
                start = index - 1
            else:
                runs.append((flight_index, sections[start : index + 1]))
                start = index
            flight_index = section.flight_index
    if start is not None and len(sections) - start >= 2:
        runs.append((flight_index, sections[start:]))
    return runs


def planar_stringer_sections(
    sections,
    side,
    origin,
    heading,
    width,
    start_seam=None,
    end_seam=None,
    nosing=0.0,
):
    """Project a straight-flight stringer onto one vertical board plane.

    Winding changes the longitudinal positions and elevations of the section
    cuts, but it must not bend the board in plan. The resulting upper and
    lower profiles can curve within the flat side face of the plank.
    """

    radians = math.radians(float(heading))
    tangent = (math.cos(radians), math.sin(radians))
    normal = (-tangent[1], tangent[0])
    width = max(float(width), 0.01)
    right_origin = (float(origin[0]), float(origin[1]))
    left_origin = (
        right_origin[0] + normal[0] * width,
        right_origin[1] + normal[1] * width,
    )
    selected_origin = left_origin if side == "Left" else right_origin

    def longitudinal(point):
        return (point[0] - selected_origin[0]) * tangent[0] + (
            point[1] - selected_origin[1]
        ) * tangent[1]

    distances = [
        longitudinal(section.left if side == "Left" else section.right) for section in sections
    ]
    nosing_aligned = [False] * len(sections)
    nosing = max(float(nosing), 0.0)
    if nosing > 1e-9:
        for index, section in enumerate(sections[1:-1], start=1):
            section_length = math.hypot(*section.tangent)
            if section_length < 1e-9:
                continue
            section_tangent = (
                section.tangent[0] / section_length,
                section.tangent[1] / section_length,
            )
            crossing_cosine = _dot(tangent, section_tangent)
            if crossing_cosine > 1e-5:
                # The physical nosing is a parallel line ``nosing`` before
                # the concealed riser.  Its intersection with a straight
                # side board moves by nosing / cos(angle), not merely by the
                # nosing distance.  Profile the board at that real crossing.
                distances[index] -= nosing / crossing_cosine
                nosing_aligned[index] = True
    if start_seam is not None:
        distances[0] = longitudinal(start_seam)
    if end_seam is not None:
        distances[-1] = longitudinal(end_seam)

    result = []
    for section, distance, aligned in zip(sections, distances, nosing_aligned):
        right = (
            right_origin[0] + tangent[0] * distance,
            right_origin[1] + tangent[1] * distance,
        )
        left = (
            right[0] + normal[0] * width,
            right[1] + normal[1] * width,
        )
        center = (
            right[0] + normal[0] * width / 2.0,
            right[1] + normal[1] * width / 2.0,
        )
        result.append(
            replace(
                section,
                center=center,
                tangent=tangent,
                left=left,
                right=right,
                profile_nosing_aligned=aligned,
            )
        )
    return result


def _stringer_section_runs(sections):
    """Split stringers at explicit level landing cells.

    The cross-sections on either side of a landing describe the incoming and
    outgoing flights. Lofting directly between them would twist a board across
    the entire landing, so each adjacent stair run must remain a separate
    manufactured part.
    """

    runs = []
    start = 0
    for index, section in enumerate(sections[:-1]):
        if not section.level_to_next:
            continue
        if index - start >= 1:
            runs.append(sections[start : index + 1])
        start = index + 1
    if len(sections) - start >= 2:
        runs.append(sections[start:])
    return runs


def boundary_stringer_runs(
    sections,
    side,
    footprint,
    include_post_sections=False,
    start_seam=None,
    end_seam=None,
):
    """Follow a side-entry flight's boundary, splitting boards at corners.

    A tread can cross a footprint corner without a riser at that corner.
    Insert that boundary vertex before splitting, interpolating its elevation
    between the two surrounding nosings. Each resulting board is planar.

    ``start_seam`` and ``end_seam`` are the points, on this flight's board
    line, where the board butts the previous or the following flight, as
    ``planar_stringer_sections`` places them. ``sections`` may then include
    the neighbouring flight's nosings beyond the seam so that the seam takes
    their interpolated elevation; the boards are cut at the seam.
    """
    from .geometry_plan import (
        _boundary_data,
        _boundary_vertices_between,
        _unwrap_boundary_parameters,
    )

    if len(sections) < 2:
        return []
    boundary = _boundary_data(footprint)
    points = [section.left if side == "Left" else section.right for section in sections]
    newel_points = {first for first, second in zip(points, points[1:]) if first == second}
    parameters = _unwrap_boundary_parameters(points, boundary)
    expanded = [(points[0], sections[0])]
    for index, (first, second) in enumerate(zip(parameters, parameters[1:])):
        corners = _boundary_vertices_between(
            first, second, 1.0 if second > first else -1.0, boundary
        )
        start = points[index]
        distance = 0.0
        for corner in corners:
            distance += math.dist(start, corner)
            ratio = distance / max(abs(second - first), 1e-9)
            front, rear = sections[index : index + 2]
            expanded.append(
                (
                    corner,
                    replace(
                        front,
                        station=front.station + ratio * (rear.station - front.station),
                        top_elevation=(
                            front.top_elevation + ratio * (rear.top_elevation - front.top_elevation)
                        ),
                    ),
                )
            )
            start = corner
        expanded.append((points[index + 1], sections[index + 1]))

    runs = []
    current = []
    previous_direction = None

    def projected(point, section, tangent):
        normal = (-tangent[1], tangent[0])
        sign = -1.0 if side == "Left" else 1.0
        other = (
            point[0] + sign * normal[0] * section.width,
            point[1] + sign * normal[1] * section.width,
        )
        return replace(
            section,
            left=point if side == "Left" else other,
            right=other if side == "Left" else point,
            center=((point[0] + other[0]) / 2.0, (point[1] + other[1]) / 2.0),
            tangent=tangent,
            locked_to_flight=True,
            profile_at_newel=point in newel_points,
        )

    for (first, front), (second, rear) in zip(expanded, expanded[1:]):
        length = math.dist(first, second)
        if length < 1e-7:
            continue
        direction = ((second[0] - first[0]) / length, (second[1] - first[1]) / length)
        if previous_direction is None or _dot(previous_direction, direction) < 1.0 - 1e-7:
            if current:
                runs.append(current)
            current = [projected(first, front, direction)]
        current.append(projected(second, rear, direction))
        previous_direction = direction
    if current:
        runs.append(current)
    if start_seam is not None:
        runs = _cut_boundary_runs_at_seam(runs, side, start_seam, False, projected)
    if end_seam is not None:
        runs = _cut_boundary_runs_at_seam(runs, side, end_seam, True, projected)
    if include_post_sections:
        # A board stops at a newel, but its railing must still clear the fan
        # treads sharing that point. Retain their different elevations for
        # the rail fit rather than dropping zero-length plan intervals.
        for run in runs:
            first = run[0].left if side == "Left" else run[0].right
            last = run[-1].left if side == "Left" else run[-1].right
            preceding = [
                projected(point, section, run[0].tangent)
                for point, section in expanded
                if math.dist(point, first) < 1e-7 and section.station < run[0].station - 1e-7
            ]
            following = [
                projected(point, section, run[-1].tangent)
                for point, section in expanded
                if math.dist(point, last) < 1e-7 and section.station > run[-1].station + 1e-7
            ]
            run[:] = preceding + run + following
    return runs


def _cut_boundary_runs_at_seam(runs, side, seam, at_end, projected):
    """Start, or end, a flight's boundary boards exactly at its seam.

    Boards before the seam follow the neighbouring flight's side, which that
    flight's own board covers; they are dropped. The board whose line carries
    the seam is cut there, with the elevation interpolated between the
    surrounding nosings, or extended to it when the seam lies beyond its free
    end. Without such a board the runs are returned unchanged.
    """

    def point(section):
        return section.left if side == "Left" else section.right

    runs = [list(run) for run in runs]
    if at_end:
        runs = [run[::-1] for run in runs[::-1]]
    sign = -1.0 if at_end else 1.0

    def seam_offset(run):
        tangent = (sign * run[0].tangent[0], sign * run[0].tangent[1])
        anchor = point(run[0])
        relative = (seam[0] - anchor[0], seam[1] - anchor[1])
        offset = _dot(relative, tangent)
        on_line = abs(relative[0] * tangent[1] - relative[1] * tangent[0]) < 1e-3
        return tangent, anchor, offset, on_line

    selected = None
    for index, run in enumerate(runs):
        _tangent, anchor, offset, on_line = seam_offset(run)
        if on_line and offset < math.dist(anchor, point(run[-1])) - 1e-7:
            selected = index
            break
    if selected is None:
        return [run[::-1] for run in runs[::-1]] if at_end else runs

    runs = runs[selected:]
    run = runs[0]
    tangent, anchor, offset, _on_line = seam_offset(run)
    seam_point = (anchor[0] + tangent[0] * offset, anchor[1] + tangent[1] * offset)
    if offset < -1e-7:
        runs[0] = [projected(seam_point, run[0], run[0].tangent)] + run
    elif offset > 1e-7:
        coordinates = [
            _dot((point(section)[0] - anchor[0], point(section)[1] - anchor[1]), tangent)
            for section in run
        ]
        rear = next(index for index, value in enumerate(coordinates) if value > offset + 1e-7)
        front = rear - 1
        span = coordinates[rear] - coordinates[front]
        ratio = (offset - coordinates[front]) / span if span > 1e-9 else 0.0
        first, second = run[front], run[rear]
        section = replace(
            first,
            station=first.station + ratio * (second.station - first.station),
            top_elevation=(
                first.top_elevation + ratio * (second.top_elevation - first.top_elevation)
            ),
        )
        runs[0] = [projected(seam_point, section, run[0].tangent)] + run[rear:]
    if at_end:
        runs = [run[::-1] for run in runs[::-1]]
    return runs


def _circular_stringer_data(
    sections,
    side,
    thickness,
    lateral_offset,
    housed,
):
    """Return exact coaxial radii and unwrapped angles for a curved board."""

    if len(sections) < 2:
        return None
    radial_lines = []
    for section in sections:
        direction = (
            section.left[0] - section.right[0],
            section.left[1] - section.right[1],
        )
        length = math.hypot(*direction)
        if length < 1e-9:
            return None
        radial_lines.append(
            (
                section.center,
                (direction[0] / length, direction[1] / length),
            )
        )

    best_pair = None
    best_cross = 0.0
    for first_index, first in enumerate(radial_lines[:-1]):
        for second_index in range(first_index + 1, len(radial_lines)):
            second = radial_lines[second_index]
            magnitude = abs(_cross(first[1], second[1]))
            if magnitude > best_cross:
                best_cross = magnitude
                best_pair = first, second
    if best_pair is None or best_cross < 1e-6:
        return None

    first_line, second_line = best_pair
    denominator = _cross(first_line[1], second_line[1])
    relative = (
        second_line[0][0] - first_line[0][0],
        second_line[0][1] - first_line[0][1],
    )
    distance = _cross(relative, second_line[1]) / denominator
    circle_center = (
        first_line[0][0] + first_line[1][0] * distance,
        first_line[0][1] + first_line[1][1] * distance,
    )

    path_radii = [
        math.hypot(
            section.center[0] - circle_center[0],
            section.center[1] - circle_center[1],
        )
        for section in sections
    ]
    path_radius = sum(path_radii) / len(path_radii)
    tolerance = max(path_radius, sections[0].width, 1.0) * 1e-5
    if max(path_radii) - min(path_radii) > tolerance:
        return None

    raw_angles = [
        math.atan2(
            section.center[1] - circle_center[1],
            section.center[0] - circle_center[0],
        )
        for section in sections
    ]
    angles = [raw_angles[0]]
    for angle in raw_angles[1:]:
        difference = math.atan2(
            math.sin(angle - angles[-1]),
            math.cos(angle - angles[-1]),
        )
        angles.append(angles[-1] + difference)
    sweep = angles[-1] - angles[0]
    if abs(sweep) < 1e-6:
        return None

    thickness = max(float(thickness), 0.01)
    thickness_direction = -1.0 if housed else 1.0
    surface_radii = []
    for section in sections:
        rail = section.left if side == "Left" else section.right
        inward = _stringer_inward(section, side)
        first = (
            rail[0] + inward[0] * float(lateral_offset),
            rail[1] + inward[1] * float(lateral_offset),
        )
        second = (
            first[0] + inward[0] * thickness * thickness_direction,
            first[1] + inward[1] * thickness * thickness_direction,
        )
        surface_radii.extend(
            (
                math.hypot(
                    first[0] - circle_center[0],
                    first[1] - circle_center[1],
                ),
                math.hypot(
                    second[0] - circle_center[0],
                    second[1] - circle_center[1],
                ),
            )
        )
    inner_radius = sum(surface_radii[0::2]) / len(sections)
    outer_radius = sum(surface_radii[1::2]) / len(sections)
    inner_radius, outer_radius = sorted((inner_radius, outer_radius))
    if inner_radius < 0.01 or outer_radius - inner_radius < 0.005:
        return None

    return {
        "profile": _CircularProfile(
            circle_center,
            inner_radius,
            outer_radius,
            angles[0],
            sweep,
        ),
        "angles": angles,
        "path_radius": path_radius,
    }
