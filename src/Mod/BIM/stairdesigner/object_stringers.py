# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stringer object generation and planar connection helpers."""

import math

import FreeCAD

from .geometry_stringer_path import (
    planar_stringer_sections,
    straight_stringer_sections,
    stringer_flight_runs,
)
from .geometry_stringer_shapes import make_housed_stringer_shape, make_notched_stringer_shape

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
            planes[run_index] = (
                _layout_stringer_run_plane(layouts[flight_index], side)
                if (
                    str(flights[flight_index].FlightType) == "Straight"
                    and flight_index < len(layouts)
                    and _has_endpoint_angle(flights[flight_index])
                )
                else _stringer_run_plane(
                    sections,
                    side,
                    max(
                        _quantity_value(flights[flight_index].Width),
                        0.01,
                    ),
                )
            )
            if flight_index in profiles:
                center_lines[run_index] = _stringer_center_line(
                    planes[run_index][1],
                    side,
                    profiles[flight_index],
                )
    result = []
    for run_index, (flight_index, sections) in enumerate(flight_runs):
        flight = flights[flight_index]
        if not str(flight.FlightType).startswith("Straight"):
            result.append((flight_index, sections))
            continue

        start_seam = None
        end_seam = None
        if run_index:
            previous_index = flight_runs[run_index - 1][0]
            if previous_index + 1 == flight_index and str(
                flights[previous_index].FlightType
            ).startswith("Straight"):
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
        if run_index + 1 < len(flight_runs):
            following_index = flight_runs[run_index + 1][0]
            if flight_index + 1 == following_index and str(
                flights[following_index].FlightType
            ).startswith("Straight"):
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
                    max(_quantity_value(flight.Width), 0.01),
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
    return result


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

    added = _add_property(
        part,
        "App::PropertyEnumeration",
        "NosingOffsetDirection",
        "Stringer overrides",
        "Direction used for the position above nosings",
    )
    current_direction = (
        defaults["NosingOffsetDirection"] if added else str(part.NosingOffsetDirection)
    )
    directions = ["Perpendicular", "Vertical"]
    part.NosingOffsetDirection = directions
    if not part.OverrideNosingPosition:
        current_direction = defaults["NosingOffsetDirection"]
    part.NosingOffsetDirection = (
        current_direction if current_direction in directions else "Perpendicular"
    )
    part.setEditorMode(
        "NosingOffsetDirection",
        0 if part.OverrideNosingPosition else 1,
    )


class StairStringerMixin:
    """Implementation methods grouped by responsibility."""

    def _update_stringers(
        self,
        stair,
        layouts,
        balanced_sections,
        riser_height,
        allow_structure_changes,
    ):
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
            "NosingOffsetDirection": str(stair.StringerNosingOffsetDirection),
        }
        for side_index, side in enumerate(("Left", "Right"), start=1):
            role = f"{side}Stringer"
            source_runs = [
                (flight_index, sections)
                for flight_index, sections in flight_runs
                if str(
                    getattr(
                        flights[flight_index],
                        f"{side}StringerType",
                    )
                )
                != "None"
            ]
            if not source_runs:
                if allow_structure_changes:
                    _resize_generated_parts(group, stair, role, 0)
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
                (flight_index, sections)
                for flight_index, sections in prepared_runs
                if flight_index in profiles
            ]
            labels = {
                ("Left", "Housed stringer"): translate("BIM", "Left housed stringer"),
                ("Right", "Housed stringer"): translate("BIM", "Right housed stringer"),
                ("Left", "Notched stringer"): translate("BIM", "Left notched stringer"),
                ("Right", "Notched stringer"): translate("BIM", "Right notched stringer"),
            }
            for (flight_index, sections), part in zip(side_runs, parts):
                stringer_type = profiles[flight_index]["StringerType"]
                thickness = _quantity_value(part.Thickness)
                width = _quantity_value(part.Width)
                overlap = _quantity_value(part.StepOverlap)
                start_extension = (
                    _quantity_value(stair.StringerStartExtension)
                    if flight_index == first_flight_index
                    else 0.0
                )
                end_extension = (
                    _quantity_value(stair.StringerEndExtension)
                    if flight_index == last_flight_index
                    else 0.0
                )
                if stringer_type == "Housed stringer":
                    part.Shape = make_housed_stringer_shape(
                        sections,
                        riser_height,
                        side,
                        thickness,
                        width,
                        overlap,
                        start_extension,
                        end_extension,
                        _quantity_value(part.NosingOffset),
                        str(part.NosingOffsetDirection),
                        _quantity_value(stair.Nosing),
                    )
                else:
                    part.Shape = make_notched_stringer_shape(
                        sections,
                        riser_height,
                        _quantity_value(stair.StepThickness),
                        side,
                        thickness,
                        width,
                        overlap,
                        end_extension,
                        (_quantity_value(stair.RiserThickness) if stair.RisersEnabled else 0.0),
                    )
                part.Label = (
                    f"{translate('BIM', 'Flight')} {flight_index + 1}: "
                    f"{labels[(side, stringer_type)]}"
                )
                part.Index = flight_index * 2 + side_index
                part.FlightIndex = flight_index + 1
                part.Placement = _child_placement(stair)
                if FreeCAD.GuiUp:
                    part.ViewObject.ShapeColor = (0.46, 0.27, 0.12)
