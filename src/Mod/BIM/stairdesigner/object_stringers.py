# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stringer object generation and planar connection helpers."""

from dataclasses import replace
import functools
import math

import FreeCAD
import Part

from .geometry_stringer_path import (
    boundary_stringer_runs,
    notched_stringer_throat,
    planar_stringer_sections,
    straight_stringer_sections,
    stringer_flight_runs,
)
from .geometry_joinery import make_stair_joinery
from .geometry_stringer_shapes import (
    housed_board_supports,
    level_board_corners,
    make_housed_stringer_shape,
    make_notched_stringer_shape,
    make_short_housed_stringer_shape,
)

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _generated_parts,
    _make_component_group,
    _resize_generated_parts,
    _set_generated_properties,
)

from .object_utils import (
    _add_link_property,
    _add_property,
    _child_placement,
    _has_endpoint_angle,
    _quantity_value,
    _set_global_shape,
    _turned_opening,
    first_step_rounded,
    get_flights,
)


def _stringer_run_plane(sections, side, width):
    """Return a flat board plane and its selected side-rail line."""

    candidates = [section for section in sections if section.locked_to_flight] or list(sections)
    counts = {}
    for section in candidates:
        key = (
            round(section.tangent[0], 6),
            round(section.tangent[1], 6),
        )
        counts[key] = counts.get(key, 0) + 1
    tangent_key = max(counts, key=counts.get)
    reference = min(
        candidates,
        key=lambda section: (section.tangent[0] - tangent_key[0]) ** 2
        + (section.tangent[1] - tangent_key[1]) ** 2,
    )
    length = math.hypot(*tangent_key)
    direction = (
        tangent_key[0] / max(length, 1e-9),
        tangent_key[1] / max(length, 1e-9),
    )
    normal = (-direction[1], direction[0])
    selected_origin = reference.left if side == "Left" else reference.right
    right_origin = selected_origin
    if side == "Left":
        right_origin = (
            selected_origin[0] - normal[0] * width,
            selected_origin[1] - normal[1] * width,
        )
    return (
        (right_origin, direction),
        (selected_origin, direction),
    )


def _straight_flight_plane(flights, flight_index, layouts, sections, side):
    """Return the board plane and side-rail line of a straight flight's run.

    A straight flight's board lies on its side, known from its layout. The
    sections square to the flight give the same line; without any, as when
    balancing turns every nosing of a short flight, a guess from the others
    would tilt the board.
    """

    flight = flights[flight_index]
    if (
        str(flight.FlightType) == "Straight"
        and flight_index < len(layouts)
        and (
            _has_endpoint_angle(flight) or not any(section.locked_to_flight for section in sections)
        )
    ):
        return _layout_stringer_run_plane(layouts[flight_index], side)
    return _stringer_run_plane(sections, side, _steps_width(flights, flight_index, layouts))


def _steps_width(flights, flight_index, layouts):
    """Return the width of a flight's steps: its layout's, within the
    flight's own width beside its boards and posts."""

    if flight_index < len(layouts):
        return max(float(layouts[flight_index]["width"]), 0.01)
    return max(_quantity_value(flights[flight_index].Width), 0.01)


def _layout_stringer_run_plane(layout, side):
    """Return the exact side plane from a straight flight layout."""

    heading = math.radians(float(layout["heading"]))
    direction = (math.cos(heading), math.sin(heading))
    normal = (-direction[1], direction[0])
    width = max(float(layout["width"]), 0.01)
    placement = layout["placement"]
    right_origin = (placement.Base.x, placement.Base.y)
    selected_origin = right_origin
    if side == "Left":
        selected_origin = (
            right_origin[0] + normal[0] * width,
            right_origin[1] + normal[1] * width,
        )
    return (
        (right_origin, direction),
        (selected_origin, direction),
    )


def _line_intersection(first, second):
    first_origin, first_direction = first
    second_origin, second_direction = second
    denominator = (
        first_direction[0] * second_direction[1] - first_direction[1] * second_direction[0]
    )
    if abs(denominator) < 1e-9:
        return None
    difference = (
        second_origin[0] - first_origin[0],
        second_origin[1] - first_origin[1],
    )
    distance = (
        difference[0] * second_direction[1] - difference[1] * second_direction[0]
    ) / denominator
    return (
        first_origin[0] + first_direction[0] * distance,
        first_origin[1] + first_direction[1] * distance,
    )


def _stringer_center_line(rail_line, side, profile):
    """Return the plan centerline of a straight stringer board."""

    origin, direction = rail_line
    inward = (direction[1], -direction[0]) if side == "Left" else (-direction[1], direction[0])
    thickness = max(float(profile["Thickness"]), 0.01)
    overlap = float(profile["StepOverlap"])
    center_offset = (
        overlap - thickness / 2.0
        if profile["StringerType"] == "Housed stringer"
        else overlap + thickness / 2.0
    )
    return (
        (
            origin[0] + inward[0] * center_offset,
            origin[1] + inward[1] * center_offset,
        ),
        direction,
    )


def _shift_line_point(point, line, distance):
    direction = line[1]
    return (
        point[0] + direction[0] * distance,
        point[1] + direction[1] * distance,
    )


def _planar_stringer_runs(
    flight_runs,
    flights,
    side,
    profiles=None,
    layouts=None,
):
    """Keep each straight-flight board planar and butt adjacent boards."""

    profiles = profiles or {}
    layouts = layouts or []
    planes = {}
    center_lines = {}
    for run_index, (flight_index, sections) in enumerate(flight_runs):
        if str(flights[flight_index].FlightType).startswith("Straight"):
            planes[run_index] = _straight_flight_plane(
                flights, flight_index, layouts, sections, side
            )
            if flight_index in profiles:
                center_lines[run_index] = _stringer_center_line(
                    planes[run_index][1],
                    side,
                    profiles[flight_index],
                )
    result = []
    seamed = {}
    for run_index, (flight_index, sections) in enumerate(flight_runs):
        flight = flights[flight_index]
        if not str(flight.FlightType).startswith("Straight"):
            result.append((flight_index, sections))
            continue

        start_seam = None
        end_seam = None
        butts_previous, butts_following = _run_butts_neighbours(flight_runs, flights, run_index)
        if butts_previous:
            previous_index = flight_runs[run_index - 1][0]
            previous_profile = profiles.get(previous_index)
            current_profile = profiles.get(flight_index)
            if previous_profile and current_profile:
                center_seam = _line_intersection(
                    center_lines[run_index - 1],
                    center_lines[run_index],
                )
                if center_seam is not None:
                    start_seam = _shift_line_point(
                        center_seam,
                        center_lines[run_index],
                        previous_profile["Thickness"] / 2.0,
                    )
            else:
                start_seam = _line_intersection(
                    planes[run_index - 1][1],
                    planes[run_index][1],
                )
        if butts_following:
            following_index = flight_runs[run_index + 1][0]
            current_profile = profiles.get(flight_index)
            following_profile = profiles.get(following_index)
            if current_profile and following_profile:
                center_seam = _line_intersection(
                    center_lines[run_index],
                    center_lines[run_index + 1],
                )
                if center_seam is not None:
                    end_seam = _shift_line_point(
                        center_seam,
                        center_lines[run_index],
                        following_profile["Thickness"] / 2.0,
                    )
            else:
                end_seam = _line_intersection(
                    planes[run_index][1],
                    planes[run_index + 1][1],
                )

        # The riser shared by two boards is projected onto their seam. When
        # it really crosses this board, keep it there as a nosing as well.
        sections = list(sections)
        if end_seam is not None and _section_on_rail(sections[-1], side, planes[run_index][1]):
            sections.append(sections[-1])
        if start_seam is not None and _section_on_rail(sections[0], side, planes[run_index][1]):
            sections.insert(0, sections[0])
        board_line = planes[run_index][0]
        right_origin, direction = board_line
        result.append(
            (
                flight_index,
                planar_stringer_sections(
                    sections,
                    side,
                    right_origin,
                    math.degrees(math.atan2(direction[1], direction[0])),
                    _steps_width(flights, flight_index, layouts),
                    start_seam,
                    end_seam,
                    (
                        profiles[flight_index].get("Nosing", 0.0)
                        if profiles.get(flight_index, {}).get("StringerType") == "Housed stringer"
                        else 0.0
                    ),
                ),
            )
        )
        seamed[len(result) - 1] = (start_seam is not None, end_seam is not None)
    _blend_stringer_seams(result, seamed, side)
    return result


def _run_butts_neighbours(flight_runs, flights, run_index):
    """Return whether a run's board butts the previous and following board.

    Adjacent straight flights share a seam where their side boards meet.
    """

    flight_index = flight_runs[run_index][0]

    def straight_neighbour(neighbour_run_index, step):
        if not 0 <= neighbour_run_index < len(flight_runs):
            return False
        neighbour_index = flight_runs[neighbour_run_index][0]
        return neighbour_index == flight_index + step and str(
            flights[neighbour_index].FlightType
        ).startswith("Straight")

    return (
        straight_neighbour(run_index - 1, -1),
        straight_neighbour(run_index + 1, 1),
    )


def _endpoint_boundary_boards(
    flight_runs,
    flights,
    run_index,
    planar_sections,
    side,
    footprint,
    include_post_sections=False,
):
    """Return the boards of a flight with a turning entry or exit.

    The boards follow the footprint around the entry or exit turn. Where the
    flight butts a neighbouring straight flight, they start or end at the
    seam ``_planar_stringer_runs`` placed in ``planar_sections``, as the
    neighbour's board does, instead of at the shared nosing. That nosing can
    lie far from the corner when a winder spans it.
    """

    sections = list(flight_runs[run_index][1])
    butts_previous, butts_following = _run_butts_neighbours(flight_runs, flights, run_index)
    start_seam = end_seam = None
    if planar_sections and butts_previous:
        start_seam = getattr(planar_sections[0], side.lower())
        sections = list(flight_runs[run_index - 1][1][:-1]) + sections
    if planar_sections and butts_following:
        end_seam = getattr(planar_sections[-1], side.lower())
        sections = sections + list(flight_runs[run_index + 1][1][1:])
    return boundary_stringer_runs(
        sections,
        side,
        footprint,
        include_post_sections=include_post_sections,
        start_seam=start_seam,
        end_seam=end_seam,
    )


def _section_on_rail(section, side, rail_line):
    origin, direction = rail_line
    point = section.left if side == "Left" else section.right
    return abs(direction[0] * (point[1] - origin[1]) - direction[1] * (point[0] - origin[0])) < 1.0


def _blend_stringer_seams(runs, seamed, side):
    """Give two boards meeting at a seam one common, interpolated height.

    The seam lies between the last nosing of one board and the first nosing
    of the next. Using the elevation of the shared riser there instead would
    raise a sharp peak over a winder spanning the corner.
    """

    def distance(first, second):
        first = first.left if side == "Left" else first.right
        second = second.left if side == "Left" else second.right
        return math.dist(first, second)

    for index in range(len(runs) - 1):
        if not seamed.get(index, (False, False))[1] or not seamed.get(index + 1, (False, False))[0]:
            continue
        before = runs[index][1]
        after = runs[index + 1][1]
        if len(before) < 2 or len(after) < 2:
            continue
        incoming = distance(before[-2], before[-1])
        outgoing = distance(after[0], after[1])
        if incoming + outgoing < 1e-6:
            continue
        low = before[-2].top_elevation
        high = after[1].top_elevation
        elevation = low + (high - low) * incoming / (incoming + outgoing)
        before[-1] = replace(before[-1], top_elevation=elevation, profile_nosing_aligned=True)
        after[0] = replace(after[0], top_elevation=elevation, profile_nosing_aligned=True)


def _stringer_parts_for_flights(
    group,
    stair,
    role,
    flight_runs,
    flights,
    allow_structure_changes,
):
    """Match generated stringers to source flights without losing overrides."""

    existing = _generated_parts(group, stair, role)
    unused = list(existing)
    result = []
    for flight_index, _sections in flight_runs:
        source_flight = flights[flight_index]
        part = next(
            (
                candidate
                for candidate in unused
                if "SourceFlight" in candidate.PropertiesList
                and candidate.SourceFlight == source_flight
            ),
            None,
        )
        if part is None:
            part = next(
                (
                    candidate
                    for candidate in unused
                    if int(getattr(candidate, "FlightIndex", 0)) == flight_index + 1
                ),
                None,
            )
        if part is None and allow_structure_changes:
            part = stair.Document.addObject("Part::Feature", f"{stair.Name}_{role}")
            _set_generated_properties(part, stair, role)
            group.addObject(part)
        if part is None:
            continue
        if part in unused:
            unused.remove(part)
        _add_link_property(
            part,
            "App::PropertyLinkGlobal",
            "SourceFlight",
            "Stair Designer",
            "Source flight for this stringer",
            editor_mode=2,
        )
        part.SourceFlight = source_flight
        result.append(part)

    if allow_structure_changes:
        for part in unused:
            group.removeObject(part)
            stair.Document.removeObject(part.Name)
    return result


def _stringer_board_parts(group, stair, side, part, count, allow_structure_changes):
    """Return the parts holding a flight side's boards besides its main one.

    A flight whose entry or exit turns is built from several boards, each a
    piece of timber of its own. The flight's stringer part keeps the main
    board, and the overrides; each other board is a part following it.
    """

    role = f"{side}StringerBoard"
    parts = [
        candidate
        for candidate in _generated_parts(group, stair, role)
        if getattr(candidate, "SourceFlight", None) == part.SourceFlight
    ]
    parts.sort(key=lambda candidate: int(getattr(candidate, "BoardIndex", 0)))
    while allow_structure_changes and len(parts) < count:
        board = stair.Document.addObject("Part::Feature", f"{stair.Name}_{role}")
        _set_generated_properties(board, stair, role)
        group.addObject(board)
        parts.append(board)
    for board in parts:
        _add_link_property(
            board,
            "App::PropertyLinkGlobal",
            "SourceFlight",
            "Stair Designer",
            "Source flight for this stringer board",
            editor_mode=2,
        )
        _add_property(
            board,
            "App::PropertyInteger",
            "BoardIndex",
            "Stair Designer",
            "Position of this board along its flight side",
            0,
            editor_mode=2,
        )
        for name, type_id in (
            ("Side", "App::PropertyString"),
            ("StringerType", "App::PropertyString"),
            ("Thickness", "App::PropertyLength"),
            ("StepOverlap", "App::PropertyDistance"),
        ):
            _add_property(board, type_id, name, "Stringer", f"Stringer {name}", editor_mode=1)
            setattr(board, name, getattr(part, name))
        board.SourceFlight = part.SourceFlight
        board.FlightIndex = part.FlightIndex
        board.Index = part.Index
    return parts


def _remove_generated_parts(group, stair, parts):
    for part in parts:
        group.removeObject(part)
        stair.Document.removeObject(part.Name)


def _flight_side_stringers(stringers, flight, side):
    """Return the stringer parts of a flight side: its main part first."""

    parts = [
        part
        for part in stringers
        if getattr(part, "SourceFlight", None) == flight and str(getattr(part, "Side", "")) == side
    ]
    parts.sort(key=lambda part: str(getattr(part, "StairDesignerRole", "")).endswith("Board"))
    return parts


def _set_stringer_part_properties(
    part,
    stair,
    source_flight,
    side,
    stringer_type,
    defaults,
):
    """Create and synchronize one stringer's per-property overrides."""

    _add_property(
        part,
        "App::PropertyString",
        "Side",
        "Stringer",
        "Side of the source flight",
        side,
        editor_mode=1,
    )
    _add_property(
        part,
        "App::PropertyString",
        "StringerType",
        "Stringer",
        "Construction type inherited from the stair",
        stringer_type,
        editor_mode=1,
    )
    part.Side = side
    part.StringerType = stringer_type
    part.SourceFlight = source_flight

    definitions = (
        (
            "Thickness",
            "App::PropertyLength",
            "OverrideThickness",
            "Override the default stringer thickness",
        ),
        (
            "Width",
            "App::PropertyLength",
            "OverrideWidth",
            "Override the default or automatic stringer width",
        ),
        (
            "StepOverlap",
            "App::PropertyDistance",
            "OverrideStepOverlap",
            "Override the default step overlap",
        ),
        (
            "NosingOffset",
            "App::PropertyLength",
            "OverrideNosingPosition",
            "Override the default position above the nosings",
        ),
        (
            "UndersideOffset",
            "App::PropertyLength",
            "OverrideUndersidePosition",
            "Override the default timber left under the step housings",
        ),
    )
    for name, type_id, override_name, description in definitions:
        _add_property(
            part,
            "App::PropertyBool",
            override_name,
            "Stringer overrides",
            description,
            False,
        )
        _add_property(
            part,
            type_id,
            name,
            "Stringer overrides",
            f"Effective stringer {name.lower()}",
            defaults[name],
        )
        overridden = bool(getattr(part, override_name))
        if not overridden:
            setattr(part, name, defaults[name])
        part.setEditorMode(name, 0 if overridden else 1)

    directions = ["Perpendicular", "Vertical"]
    for name, override_name, description in (
        (
            "NosingOffsetDirection",
            "OverrideNosingPosition",
            "Direction used for the position above nosings",
        ),
        (
            "UndersideOffsetDirection",
            "OverrideUndersidePosition",
            "Direction used for the position below steps",
        ),
    ):
        added = _add_property(
            part,
            "App::PropertyEnumeration",
            name,
            "Stringer overrides",
            description,
        )
        current_direction = defaults[name] if added else str(getattr(part, name))
        setattr(part, name, directions)
        overridden = bool(getattr(part, override_name))
        if not overridden:
            current_direction = defaults[name]
        setattr(
            part,
            name,
            current_direction if current_direction in directions else "Perpendicular",
        )
        part.setEditorMode(name, 0 if overridden else 1)


def _carry_inner_corner_fans(boards, side, at_start, at_end, reach):
    """Hand the fan treads at an inner corner to the newel standing there.

    On the inner side of a turn the winders converge on the corner, where
    the newel stands. Their nosings within ``reach`` of the corner, the
    newel's size, are carried by it as at a flight's end newel: they do not
    bend the board's upper edge, nor sink its lower one.
    """

    def carry(sections, from_end):
        if not sections:
            return sections
        points = [getattr(section, side.lower()) for section in sections]
        corner = points[-1] if from_end else points[0]
        return [
            replace(section, profile_at_newel=True) if math.dist(point, corner) < reach else section
            for section, point in zip(sections, points)
        ]

    boards = list(boards)
    if at_start and boards:
        boards[0] = carry(boards[0], False)
    if at_end and boards:
        boards[-1] = carry(boards[-1], True)
    return boards


def _board_length(sections, side):
    points = [getattr(section, side.lower()) for section in sections]
    return math.dist(points[0], points[-1])


def _too_short_to_house(stair, flight, sections, side):
    """Tell whether a flight side is too short for a regular housed board.

    A side shorter than one going runs from the first riser to a corner
    newel, and the winders converging on that newel end a few centimetres
    in front of it. A board of the regular width following their nosings
    would climb almost vertically. The stair's first flight gets a short
    board standing on the floor there, see
    ``make_short_housed_stringer_shape``; elsewhere the newels at both ends
    of the side carry those steps.
    """

    return str(getattr(flight, f"{side}StringerType")) == "Housed stringer" and _board_length(
        sections, side
    ) < _quantity_value(stair.TreadWidth)


def _housed_start_extension(stair, flight, stringer_type):
    """Return how far the first flight's board runs before its first riser.

    A housed board holds the first tread up to its nosing tip, so its length
    beyond the first step counts from there. A notched board carries the
    tread from below and the nosing overhangs it. A flight entering from the
    side starts at the newel its fan treads converge on, which stays there.
    """

    if str(flight.EntryDirection) != "Straight":
        # The steps reach the floor beyond the boards' plan; the board
        # across the flight's start runs to the first riser there.
        return _turned_opening(stair, flight, True)
    extension = _quantity_value(stair.StringerStartExtension)
    if stringer_type == "Housed stringer":
        extension += _quantity_value(stair.Nosing)
    return extension


def _housed_end_extension(stair, flight, side, stringer_type):
    """Return how far the last flight's last board runs past its end.

    A flight turning at its exit wraps its outer side around the top: that
    side's last board runs across the flight and ends at the final riser,
    with the last tread's nosing overhanging it as the first tread's does at
    the bottom. A housed board holds that nosing too. The final riser stands
    at the floor, beyond the boards' plan by the turned side's room.
    """

    extension = _quantity_value(stair.StringerEndExtension)
    exit_direction = str(flight.ExitDirection)
    if exit_direction != "Straight" and side.lower() not in exit_direction.lower():
        extension += _turned_opening(stair, flight, False)
        if stringer_type == "Housed stringer":
            extension += _quantity_value(stair.Nosing)
    return extension


class StairStringerMixin:
    """Implementation methods grouped by responsibility."""

    def _update_stringer_joinery(self, stair):
        """Cut housings, tenons and mortises once all members exist."""

        group = stair.StringersGroup
        if not group:
            return
        stringers = [
            part for part in group.Group if not part.Shape.isNull() and part.Shape.Volume > 1e-6
        ]
        if not stringers:
            return
        stringers.sort(key=lambda part: (int(getattr(part, "FlightIndex", 0)), part.Name))
        posts = []
        steps = []
        housed = []
        if stair.StepsGroup:
            steps = [
                part
                for part in stair.StepsGroup.Group
                if getattr(part, "StairDesignerRole", "") in ("Tread", "Riser")
                and not part.Shape.isNull()
            ]
            housed.extend(part.Shape for part in steps)
        if stair.HandrailsGroup:
            for part in stair.HandrailsGroup.Group:
                role = getattr(part, "StairDesignerRole", "")
                if part.Shape.isNull():
                    continue
                if role == "HandrailPost":
                    posts.append(part)
                elif role == "HandrailPicket":
                    housed.append(part.Shape)
        slots = None
        if getattr(stair, "StringerOpenHousings", False):
            slots = {
                "boards": [
                    index
                    for index, part in enumerate(stringers)
                    if str(getattr(part, "StringerType", "")) == "Housed stringer"
                    # A short board stands on the floor in front of its
                    # newel: nothing slides into it from behind or below.
                    and part.Name not in getattr(self, "_short_stringers", ())
                ],
                "treads": [
                    index for index, part in enumerate(steps) if part.StairDesignerRole == "Tread"
                ],
                "risers": [
                    index for index, part in enumerate(steps) if part.StairDesignerRole == "Riser"
                ],
                "wedged": bool(getattr(stair, "StringerWedgedHousings", False)),
            }
        stringer_shapes, post_shapes, housed_shapes = make_stair_joinery(
            [part.Shape for part in stringers],
            [part.Shape for part in posts],
            housed,
            getattr(self, "_newel_step_cutters", ()),
            slots,
            finger_joints=str(getattr(stair, "StringerJointType", "")) == "Finger joint",
        )
        for index, (part, shape) in enumerate(zip(stringers, stringer_shapes)):
            _set_global_shape(part, shape)
            # The wedges its housings take, for the bill of materials:
            # (length, tapered length, width) of each.
            found = (slots or {}).get("wedges", {}).get(index, [])
            for kind in ("Tread", "Riser"):
                name = f"{kind}Wedges"
                sizes = [
                    FreeCAD.Vector(wedge["length"], wedge["taper_length"], wedge["width"])
                    for wedge in found
                    if wedge["kind"] == kind
                ]
                if sizes or name in part.PropertiesList:
                    _add_property(
                        part,
                        "App::PropertyVectorList",
                        name,
                        "Stringer",
                        f"Length, tapered length and width of the {kind.lower()} wedges",
                        editor_mode=1,
                    )
                    setattr(part, name, sizes)
        for part, shape in zip(posts, post_shapes):
            _set_global_shape(part, shape)
        # Steps crossing a post come back notched around it.
        for part, original, shape in zip(steps, housed, housed_shapes):
            if shape is not original:
                _set_global_shape(part, shape)

    def _update_stringers(
        self,
        stair,
        layouts,
        balanced_sections,
        balanced_footprint,
        riser_height,
        allow_structure_changes,
    ):
        # The parts built as short boards, for the joinery.
        self._short_stringers = set()
        flights = get_flights(stair)
        any_enabled = any(
            str(getattr(flight, f"{side}StringerType")) != "None"
            for flight in flights
            for side in ("Left", "Right")
        )
        group = stair.StringersGroup
        if not any_enabled:
            if group and allow_structure_changes:
                self._remove_stringers_group(stair)
            return
        if not group and allow_structure_changes:
            group = _make_component_group(stair, "StringersGroup", "Stringers", "stringers")
        if not group:
            return
        group.PanelSection = "stringers"
        group.Proxy.Section = "stringers"

        if balanced_sections:
            flight_runs = stringer_flight_runs(
                balanced_sections,
                [str(flight.FlightType) for flight in flights],
            )
        elif len(layouts) == 1:
            layout = layouts[0]
            flight_runs = [
                (
                    0,
                    straight_stringer_sections(
                        layout["metrics"],
                        layout["width"],
                        layout["tread_goings"],
                        layout["section_top_elevations"],
                    ),
                )
            ]
        else:
            flight_runs = []
        if not flight_runs:
            return

        first_flight_index = flight_runs[0][0]
        last_flight_index = flight_runs[-1][0]
        defaults = {
            "Thickness": _quantity_value(stair.StringerThickness),
            "Width": _quantity_value(stair.StringerWidth),
            "StepOverlap": _quantity_value(stair.StringerStepOverlap),
            "NosingOffset": _quantity_value(stair.StringerNosingOffset),
            "UndersideOffset": _quantity_value(stair.StringerUndersideOffset),
            "UndersideOffsetDirection": str(stair.StringerUndersideOffsetDirection),
            "NosingOffsetDirection": str(stair.StringerNosingOffsetDirection),
        }
        # The treads a housed board holds, in the stair space it is built in.
        # A riser stands on the tread below, whose underside lies deeper.
        housed_steps = []
        if stair.StepsGroup:
            for step in stair.StepsGroup.Group:
                if (
                    str(getattr(step, "StairDesignerRole", "")) == "Tread"
                    and not step.Shape.isNull()
                ):
                    shape = step.Shape.copy()
                    shape.transformShape(step.Placement.inverse().toMatrix())
                    housed_steps.append(shape)
        for side_index, side in enumerate(("Left", "Right"), start=1):
            role = f"{side}Stringer"
            source_runs = []
            # The first step's rounded end stands free, see
            # ``_round_first_step``.
            rounded = first_step_rounded(stair, side)
            # The flights whose side gets a short board on the floor.
            short_flights = set()
            for flight_index, sections in flight_runs:
                flight = flights[flight_index]
                if str(getattr(flight, f"{side}StringerType")) == "None":
                    continue
                if _too_short_to_house(stair, flight, sections, side):
                    if (
                        flight_index != first_flight_index
                        or rounded
                        or str(flight.EntryDirection) != "Straight"
                    ):
                        # The newels at its ends carry the side's steps.
                        continue
                    short_flights.add(flight_index)
                elif rounded and flight_index == first_flight_index and len(sections) < 3:
                    # Nothing is left of the side behind the rounded step.
                    continue
                source_runs.append((flight_index, sections))
            if not source_runs:
                if allow_structure_changes:
                    _resize_generated_parts(group, stair, role, 0)
                    _resize_generated_parts(group, stair, f"{side}StringerBoard", 0)
                continue

            parts = _stringer_parts_for_flights(
                group,
                stair,
                role,
                source_runs,
                flights,
                allow_structure_changes,
            )
            profiles = {}
            for (flight_index, _sections), part in zip(source_runs, parts):
                stringer_type = str(
                    getattr(
                        flights[flight_index],
                        f"{side}StringerType",
                    )
                )
                _set_stringer_part_properties(
                    part,
                    stair,
                    flights[flight_index],
                    side,
                    stringer_type,
                    defaults,
                )
                profiles[flight_index] = {
                    "Thickness": _quantity_value(part.Thickness),
                    "StepOverlap": _quantity_value(part.StepOverlap),
                    "StringerType": stringer_type,
                    "Nosing": _quantity_value(stair.Nosing),
                }

            prepared_runs = _planar_stringer_runs(
                flight_runs,
                flights,
                side,
                profiles,
                layouts,
            )
            side_runs = [
                (
                    flight_index,
                    # Behind a rounded first step the board starts at the
                    # second riser.
                    sections[1:] if rounded and flight_index == first_flight_index else sections,
                )
                for flight_index, sections in prepared_runs
                if flight_index in profiles
            ]
            labels = {
                ("Left", "Housed stringer"): translate("BIM", "Left housed stringer"),
                ("Right", "Housed stringer"): translate("BIM", "Right housed stringer"),
                ("Left", "Notched stringer"): translate("BIM", "Left notched stringer"),
                ("Right", "Notched stringer"): translate("BIM", "Right notched stringer"),
            }
            pieces = []
            for (flight_index, sections), part in zip(side_runs, parts):
                stringer_type = profiles[flight_index]["StringerType"]
                thickness = _quantity_value(part.Thickness)
                width = _quantity_value(part.Width)
                overlap = _quantity_value(part.StepOverlap)
                start_extension = (
                    _housed_start_extension(stair, flights[flight_index], stringer_type)
                    if flight_index == first_flight_index
                    else 0.0
                )
                end_extension = (
                    _housed_end_extension(stair, flights[flight_index], side, stringer_type)
                    if flight_index == last_flight_index
                    else 0.0
                )
                flight = flights[flight_index]
                boards = [sections]
                if (
                    balanced_footprint is not None
                    and str(flight.FlightType) == "Straight"
                    and (
                        (flight_index == 0 and str(flight.EntryDirection) != "Straight")
                        or (
                            flight_index == len(flights) - 1
                            and str(flight.ExitDirection) != "Straight"
                        )
                    )
                ):
                    run_index = next(
                        index
                        for index, (run_flight, _run) in enumerate(flight_runs)
                        if run_flight == flight_index
                    )
                    boards = _endpoint_boundary_boards(
                        flight_runs,
                        flights,
                        run_index,
                        sections,
                        side,
                        balanced_footprint,
                    )
                boards = _carry_inner_corner_fans(
                    boards,
                    side,
                    # The flight turns towards this side at its start, or the
                    # next one does at the end of this one. A turned entry or
                    # exit opening on this side has its newel there too.
                    (flight_index > 0 and str(flight.Rotation) == side)
                    or (flight_index == 0 and side.lower() in str(flight.EntryDirection).lower()),
                    (
                        flight_index + 1 < len(flights)
                        and str(flights[flight_index + 1].Rotation) == side
                    )
                    or (
                        flight_index == len(flights) - 1
                        and side.lower() in str(flight.ExitDirection).lower()
                    ),
                    max(
                        _quantity_value(stair.HandrailPostWidth),
                        _quantity_value(stair.HandrailPostThickness),
                    ),
                )

                def board_shape(
                    board_index,
                    supports=(),
                    start_block=None,
                    end_block=None,
                    # Bound now: the boards are rebuilt for their corner
                    # blocks once every flight's boards exist.
                    boards=boards,
                    part=part,
                    flight_index=flight_index,
                    stringer_type=stringer_type,
                    short=flight_index in short_flights,
                    thickness=thickness,
                    width=width,
                    overlap=overlap,
                    start_extension=start_extension,
                    end_extension=end_extension,
                ):
                    board = boards[board_index]
                    if short:
                        return make_short_housed_stringer_shape(
                            board,
                            riser_height,
                            side,
                            thickness,
                            overlap,
                            start_extension,
                            _quantity_value(part.NosingOffset),
                            _quantity_value(stair.Nosing),
                            housed_steps,
                            # The newel stands half its size and half a
                            # board before the corner, and carries the treads
                            # starting within half its size of its face.
                            max(
                                _quantity_value(stair.HandrailPostWidth),
                                _quantity_value(stair.HandrailPostThickness),
                            )
                            + thickness / 2.0,
                        )
                    if stringer_type == "Housed stringer":
                        return make_housed_stringer_shape(
                            board,
                            riser_height,
                            side,
                            thickness,
                            width,
                            overlap,
                            start_extension if board_index == 0 else 0.0,
                            end_extension if board_index == len(boards) - 1 else 0.0,
                            _quantity_value(part.NosingOffset),
                            str(part.NosingOffsetDirection),
                            _quantity_value(stair.Nosing),
                            # Shorter than one going, it has no room to
                            # follow its nosings.
                            single_pitch=_board_length(board, side)
                            < _quantity_value(stair.TreadWidth),
                            supports=supports,
                            underside_offset=_quantity_value(part.UndersideOffset),
                            underside_direction=str(part.UndersideOffsetDirection),
                            # Only the stair's last board may end on a
                            # tread with no riser behind it.
                            final_riser=bool(stair.EndWithRiser)
                            or flight_index != last_flight_index
                            or board_index != len(boards) - 1,
                            start_block=start_block,
                            end_block=end_block,
                        )
                    riser_clearance = (
                        _quantity_value(stair.RiserThickness) if stair.RisersEnabled else 0.0
                    )
                    return make_notched_stringer_shape(
                        board,
                        riser_height,
                        _quantity_value(stair.StepThickness),
                        side,
                        thickness,
                        width,
                        overlap,
                        end_extension if board_index == len(boards) - 1 else 0.0,
                        riser_clearance,
                        # Under a winder's cut the board keeps the timber
                        # it has under a regular step's.
                        throat=notched_stringer_throat(
                            riser_height,
                            _quantity_value(flights[flight_index].TreadWidth),
                            _quantity_value(stair.StepThickness),
                            width,
                            riser_clearance,
                        ),
                    )

                built = [board_shape(index) for index in range(len(boards))]
                supports = [()] * len(boards)
                if (
                    stringer_type == "Housed stringer"
                    and flight_index not in short_flights
                    and not stair.StringerCustomWidth
                    and not part.OverrideWidth
                ):
                    # The automatic width follows a regular flight. Where
                    # winders turning a corner or converging on a newel house
                    # a step deeper, the lower edge drops under it instead of
                    # the whole board widening.
                    supports = [
                        housed_board_supports(
                            shape,
                            board,
                            side,
                            thickness,
                            overlap,
                            housed_steps,
                            # Half a newel post: it carries the fan treads.
                            max(
                                _quantity_value(stair.HandrailPostWidth),
                                _quantity_value(stair.HandrailPostThickness),
                            )
                            / 2.0,
                        )
                        for shape, board in zip(built, boards)
                    ]
                    if any(supports):
                        built = [
                            board_shape(index, supports[index]) for index in range(len(boards))
                        ]
                pieces.extend(
                    {
                        "part": part,
                        "sections": board,
                        "shape": shape,
                        "housed": stringer_type == "Housed stringer",
                        "thickness": thickness,
                        "penetration": overlap,
                        "build": functools.partial(board_shape, index, supports[index]),
                    }
                    for index, (board, shape) in enumerate(zip(boards, built))
                    if not shape.isNull()
                )
                if flight_index in short_flights:
                    self._short_stringers.add(part.Name)
                part.Label = (
                    f"{translate('BIM', 'Flight')} {flight_index + 1}: "
                    f"{labels[(side, stringer_type)]}"
                )
                part.Index = flight_index * 2 + side_index
                part.FlightIndex = flight_index + 1
                if FreeCAD.GuiUp:
                    part.ViewObject.ShapeColor = (0.46, 0.27, 0.12)

            # Consecutive housed boards turning a corner meet on a level
            # block, once all are built.
            level_board_corners(pieces, side)
            # Each board is a piece of timber of its own: the flight's part
            # keeps the largest, its other boards get parts of their own.
            used_boards = []
            for _run, part in zip(side_runs, parts):
                shapes = [piece["shape"] for piece in pieces if piece["part"] is part]
                main = max(range(len(shapes)), key=lambda i: shapes[i].Volume) if shapes else 0
                others = [(i, shape) for i, shape in enumerate(shapes) if i != main]
                board_parts = _stringer_board_parts(
                    group, stair, side, part, len(others), allow_structure_changes
                )
                if len(board_parts) < len(others):
                    # The boards cannot get parts now: keep them in one.
                    board_parts = []
                    part.Shape = (
                        shapes[0].multiFuse(shapes[1:]).removeSplitter()
                        if len(shapes) > 1
                        else shapes[0] if shapes else Part.Shape()
                    )
                else:
                    part.Shape = shapes[main] if shapes else Part.Shape()
                part.Placement = _child_placement(stair)
                for board_index, ((index, shape), board) in enumerate(zip(others, board_parts)):
                    board.BoardIndex = board_index + 1
                    board.Shape = shape
                    board.Placement = _child_placement(stair)
                    board.Label = f"{part.Label}, " + (
                        translate("BIM", "start board")
                        if index < main
                        else translate("BIM", "end board")
                    )
                    if FreeCAD.GuiUp:
                        board.ViewObject.ShapeColor = (0.46, 0.27, 0.12)
                    used_boards.append(board)
            if allow_structure_changes:
                _remove_generated_parts(
                    group,
                    stair,
                    [
                        board
                        for board in _generated_parts(group, stair, f"{side}StringerBoard")
                        if board not in used_boards
                    ],
                )
