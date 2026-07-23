# SPDX-License-Identifier: LGPL-2.1-or-later

"""Handrail component generation and synchronization."""

import math

import FreeCAD
import Part

from .geometry_core import balanced_section_top
from .geometry_handrails import (
    handrail_picket_fractions,
    make_handrail_path,
    make_handrail_top_rail_shape,
    make_handrail_vertical_member_shape,
    sample_handrail_path,
)
from .geometry_steps import _local_step_expansion_faces, _section_band_faces
from .geometry_stringer_path import straight_stringer_sections, stringer_flight_runs


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _make_component_group,
    _set_generated_properties,
)

from .object_stringers import (
    _layout_stringer_run_plane,
    _line_intersection,
    _planar_stringer_runs,
    _stringer_center_line,
    _stringer_run_plane,
)

from .object_utils import (
    _add_link_property,
    _add_property,
    _child_placement,
    _has_endpoint_angle,
    _quantity_value,
    get_flights,
)

def _sync_handrail_parts(
    group,
    stair,
    desired,
    allow_structure_changes,
):
    """Synchronize individual top rails, posts, and pickets by source flight."""

    existing = [
        child
        for child in group.Group
        if getattr(child, "GeneratedBy", "") == stair.Name
        and str(getattr(child, "StairDesignerRole", "")).startswith(
            "Handrail"
        )
    ]
    unused = list(existing)
    role_order = {
        "HandrailTopRail": 0,
        "HandrailPost": 1,
        "HandrailPicket": 2,
    }
    for item in desired:
        part = next(
            (
                candidate
                for candidate in unused
                if str(getattr(candidate, "StairDesignerRole", ""))
                == item["role"]
                and getattr(candidate, "SourceFlight", None)
                == item["flight"]
                and str(getattr(candidate, "Side", "")) == item["side"]
                and int(getattr(candidate, "ElementIndex", -1))
                == item["element_index"]
            ),
            None,
        )
        if part is None and allow_structure_changes:
            part = stair.Document.addObject(
                "Part::Feature", f"{stair.Name}_{item['role']}"
            )
            _set_generated_properties(part, stair, item["role"])
            group.addObject(part)
        if part is None:
            continue
        if part in unused:
            unused.remove(part)
        _add_link_property(
            part,
            "App::PropertyLinkGlobal",
            "SourceFlight",
            "Handrail",
            "Source flight for this handrail component",
            editor_mode=2,
        )
        _add_property(
            part,
            "App::PropertyString",
            "Side",
            "Handrail",
            "Side of the source flight",
            item["side"],
            editor_mode=1,
        )
        _add_property(
            part,
            "App::PropertyInteger",
            "ElementIndex",
            "Handrail",
            "Zero-based index within this component type",
            item["element_index"],
            editor_mode=1,
        )
        part.SourceFlight = item["flight"]
        part.Side = item["side"]
        part.ElementIndex = item["element_index"]
        flights = get_flights(stair)
        flight_index = flights.index(item["flight"]) + 1
        part.Index = (
            flight_index * 10000
            + (0 if item["side"] == "Left" else 5000)
            + role_order[item["role"]] * 1000
            + item["element_index"]
        )
        part.FlightIndex = flight_index
        part.Label = item["label"]
        part.Shape = item["shape"]
        part.Placement = _child_placement(stair)
        if FreeCAD.GuiUp:
            if str(stair.StairType) == "Concrete":
                part.ViewObject.ShapeColor = (
                    (0.68, 0.70, 0.72)
                    if item["role"] == "HandrailTopRail"
                    else (0.56, 0.58, 0.61)
                )
            else:
                part.ViewObject.ShapeColor = (
                    (0.35, 0.20, 0.08)
                    if item["role"] == "HandrailTopRail"
                    else (0.42, 0.25, 0.10)
                )

    if allow_structure_changes:
        for part in unused:
            group.removeObject(part)
            stair.Document.removeObject(part.Name)


class StairHandrailMixin:
    """Implementation methods grouped by responsibility."""

    def _update_handrails(
        self,
        stair,
        layouts,
        balanced_sections,
        balanced_footprint,
        balanced_plan_shapes,
        riser_height,
        allow_structure_changes,
    ):
        flights = get_flights(stair)
        any_enabled = any(
            bool(getattr(flight, f"{side}HandrailEnabled", False))
            for flight in flights
            for side in ("Left", "Right")
        )
        group = stair.HandrailsGroup
        if not any_enabled:
            if group and allow_structure_changes:
                self._remove_handrails_group(stair)
            return
        if not group and allow_structure_changes:
            group = _make_component_group(
                stair, "HandrailsGroup", "Handrails", "handrails"
            )
        if not group:
            return
        group.PanelSection = "handrails"
        group.Proxy.Section = "handrails"

        if balanced_sections:
            flight_runs = stringer_flight_runs(
                balanced_sections,
                [str(flight.FlightType) for flight in flights],
            )
        elif len(layouts) == 1:
            flight_runs = [
                (
                    0,
                    straight_stringer_sections(
                        layouts[0]["metrics"],
                        layouts[0]["width"],
                        layouts[0]["tread_goings"],
                        layouts[0]["section_top_elevations"],
                    ),
                )
            ]
        else:
            flight_runs = []
        if not flight_runs:
            return

        stringers = (
            list(stair.StringersGroup.Group)
            if stair.StringersGroup
            else []
        )
        rail_shape = str(stair.HandrailTopRailShape)
        rail_width = _quantity_value(stair.HandrailTopRailWidth)
        rail_thickness = _quantity_value(stair.HandrailTopRailThickness)
        picket_shape = str(stair.HandrailPicketShape)
        picket_width = _quantity_value(stair.HandrailPicketWidth)
        picket_thickness = _quantity_value(
            stair.HandrailPicketThickness
        )
        picket_path_size = (
            picket_width
            if picket_shape == "Circular"
            else picket_thickness
        )
        post_shape = str(stair.HandrailPostShape)
        post_width = _quantity_value(stair.HandrailPostWidth)
        post_thickness = _quantity_value(stair.HandrailPostThickness)
        post_path_size = (
            post_width if post_shape == "Circular" else post_thickness
        )
        global_offset = _quantity_value(stair.HandrailOffset)
        desired = []
        post_positions = set()
        first_flight_index = flight_runs[0][0]
        last_flight_index = flight_runs[-1][0]
        concrete_support_faces = []
        if (
            str(stair.StairType) == "Concrete"
            and balanced_sections
            and balanced_plan_shapes
        ):
            for index, plan_shape in enumerate(balanced_plan_shapes):
                elevation = balanced_section_top(
                    balanced_sections[index],
                    index,
                    riser_height,
                )
                concrete_support_faces.extend(
                    (face, elevation)
                    for face in plan_shape.Faces
                )
            if stair.EndWithRiser:
                terminal_elevation = balanced_section_top(
                    balanced_sections[-1],
                    len(balanced_sections) - 1,
                    riser_height,
                )
                concrete_support_faces.extend(
                    (face, terminal_elevation)
                    for face in _local_step_expansion_faces(
                        balanced_sections[-1],
                        balanced_sections[-1],
                        balanced_footprint,
                        0.0,
                        max(
                            _quantity_value(stair.ConcreteThickness),
                            0.01,
                        ),
                    )
                )
                concrete_support_faces.extend(
                    (face, terminal_elevation)
                    for face in _section_band_faces(
                        balanced_sections[-1],
                        balanced_footprint,
                        -min(
                            0.1,
                            max(
                                balanced_footprint.BoundBox.DiagonalLength
                                * 1e-6,
                                0.01,
                            ),
                        ),
                    )
                )
        prepared_handrail_runs = {}
        corner_post_points = {}
        for prepared_side in ("Left", "Right"):
            side_profiles = {}
            for part in stringers:
                if str(getattr(part, "Side", "")) != prepared_side:
                    continue
                source_flight = getattr(part, "SourceFlight", None)
                if source_flight not in flights:
                    continue
                side_profiles[flights.index(source_flight)] = {
                    "Thickness": _quantity_value(part.Thickness),
                    "StepOverlap": _quantity_value(part.StepOverlap),
                    "StringerType": str(part.StringerType),
                    "Nosing": _quantity_value(stair.Nosing),
                }
            prepared_handrail_runs[prepared_side] = {
                flight_index: prepared_sections
                for flight_index, prepared_sections in (
                    _planar_stringer_runs(
                        flight_runs,
                        flights,
                        prepared_side,
                        side_profiles or None,
                        layouts,
                    )
                )
            }
            if side_profiles:
                center_lines = {}
                for run_index, (flight_index, sections) in enumerate(
                    flight_runs
                ):
                    if (
                        flight_index not in side_profiles
                        or not str(
                            flights[flight_index].FlightType
                        ).startswith("Straight")
                    ):
                        continue
                    plane = (
                        _layout_stringer_run_plane(
                            layouts[flight_index],
                            prepared_side,
                        )
                        if str(flights[flight_index].FlightType)
                        == "Straight"
                        and _has_endpoint_angle(flights[flight_index])
                        else _stringer_run_plane(
                            sections,
                            prepared_side,
                            max(
                                _quantity_value(
                                    flights[flight_index].Width
                                ),
                                0.01,
                            ),
                        )
                    )
                    center_line = _stringer_center_line(
                        plane[1],
                        prepared_side,
                        side_profiles[flight_index],
                    )
                    origin, direction = center_line
                    inward = (
                        (direction[1], -direction[0])
                        if prepared_side == "Left"
                        else (-direction[1], direction[0])
                    )
                    center_lines[run_index] = (
                        (
                            origin[0] + inward[0] * global_offset,
                            origin[1] + inward[1] * global_offset,
                        ),
                        direction,
                    )
                for run_index in range(len(flight_runs) - 1):
                    if (
                        run_index not in center_lines
                        or run_index + 1 not in center_lines
                    ):
                        continue
                    incoming_index = flight_runs[run_index][0]
                    outgoing_index = flight_runs[run_index + 1][0]
                    if incoming_index + 1 != outgoing_index:
                        continue
                    junction = _line_intersection(
                        center_lines[run_index],
                        center_lines[run_index + 1],
                    )
                    if junction is None:
                        continue
                    corner_post_points[
                        (prepared_side, incoming_index, 1)
                    ] = junction
                    corner_post_points[
                        (prepared_side, outgoing_index, 0)
                    ] = junction

        for flight_index, sections in flight_runs:
            flight = flights[flight_index]
            for side in ("Left", "Right"):
                if not bool(
                    getattr(flight, f"{side}HandrailEnabled", False)
                ):
                    continue
                stringer = next(
                    (
                        part
                        for part in stringers
                        if getattr(part, "SourceFlight", None) == flight
                        and str(getattr(part, "Side", "")) == side
                    ),
                    None,
                )
                center_offset = 0.0
                if stringer is not None:
                    thickness = _quantity_value(stringer.Thickness)
                    overlap = _quantity_value(stringer.StepOverlap)
                    center_offset = (
                        overlap - thickness / 2.0
                        if str(stringer.StringerType) == "Housed stringer"
                        else overlap + thickness / 2.0
                    )
                path_sections = prepared_handrail_runs.get(
                    side, {}
                ).get(
                    flight_index, sections
                )
                path = make_handrail_path(
                    path_sections,
                    riser_height,
                    side,
                    center_offset + global_offset,
                    _quantity_value(stair.HandrailHeightAboveNosing),
                    (
                        _quantity_value(stair.StringerStartExtension)
                        if stringer is not None
                        and str(stringer.StringerType)
                        == "Housed stringer"
                        and flight_index == first_flight_index
                        else 0.0
                    ),
                    (
                        _quantity_value(stair.StringerEndExtension)
                        if stringer is not None
                        and flight_index == last_flight_index
                        else 0.0
                    ),
                )
                if path is None:
                    continue
                junction_path = make_handrail_path(
                    sections,
                    riser_height,
                    side,
                    center_offset + global_offset,
                    _quantity_value(stair.HandrailHeightAboveNosing),
                )
                path_slope = (
                    (
                        path["top_elevations"][-1]
                        - path["top_elevations"][0]
                    )
                    / path["length"]
                    if path["length"] > 1e-9
                    else 0.0
                )
                rail_depth = (
                    rail_width / math.sqrt(1.0 + path_slope * path_slope)
                    if rail_shape == "Circular"
                    else rail_thickness
                )
                top_rail_shape = make_handrail_top_rail_shape(
                    path,
                    rail_shape,
                    rail_width,
                    rail_thickness,
                    _quantity_value(
                        stair.HandrailTopRailPostPenetration
                    ),
                    post_path_size,
                )
                side_label = translate("BIM", side.lower())
                desired.append(
                    {
                        "role": "HandrailTopRail",
                        "flight": flight,
                        "side": side,
                        "element_index": 0,
                        "shape": top_rail_shape,
                        "label": (
                            f"{translate('BIM', 'Flight')} "
                            f"{flight_index + 1}: {side_label} "
                            f"{translate('BIM', 'top rail')}"
                        ),
                    }
                )

                def stringer_span(sample):
                    if stringer is None or stringer.Shape.isNull():
                        return None
                    tangent = sample["tangent"]
                    inward = (
                        (tangent[1], -tangent[0])
                        if side == "Left"
                        else (-tangent[1], tangent[0])
                    )
                    point = (
                        sample["point"][0] - inward[0] * global_offset,
                        sample["point"][1] - inward[1] * global_offset,
                    )
                    probe = Part.makeLine(
                        FreeCAD.Vector(point[0], point[1], -100000.0),
                        FreeCAD.Vector(point[0], point[1], 100000.0),
                    )
                    intersection = stringer.Shape.common(probe)
                    if not intersection.Vertexes:
                        return None
                    elevations = [
                        vertex.Point.z for vertex in intersection.Vertexes
                    ]
                    return min(elevations), max(elevations)

                def concrete_support(
                    sample,
                    member_shape,
                    member_width,
                    member_thickness,
                ):
                    if not concrete_support_faces:
                        return None
                    tangent_length = max(
                        math.hypot(*sample["tangent"]), 1e-9
                    )
                    tangent = (
                        sample["tangent"][0] / tangent_length,
                        sample["tangent"][1] / tangent_length,
                    )
                    inward = (
                        (tangent[1], -tangent[0])
                        if side == "Left"
                        else (-tangent[1], tangent[0])
                    )
                    half_width = max(float(member_width) / 2.0, 0.01)
                    if str(member_shape) == "Circular":
                        radius = max(half_width - 0.01, 0.0)
                        diagonal = radius / math.sqrt(2.0)
                        offsets = (
                            (0.0, 0.0),
                            (radius, 0.0),
                            (-radius, 0.0),
                            (0.0, radius),
                            (0.0, -radius),
                            (diagonal, diagonal),
                            (diagonal, -diagonal),
                            (-diagonal, diagonal),
                            (-diagonal, -diagonal),
                        )
                    else:
                        half_depth = max(
                            float(member_thickness) / 2.0,
                            0.01,
                        )
                        lateral = max(half_width - 0.01, 0.0)
                        longitudinal = max(half_depth - 0.01, 0.0)
                        offsets = tuple(
                            (normal_offset, tangent_offset)
                            for normal_offset in (
                                -lateral,
                                0.0,
                                lateral,
                            )
                            for tangent_offset in (
                                -longitudinal,
                                0.0,
                                longitudinal,
                            )
                        )
                    elevations = []
                    for normal_offset, tangent_offset in offsets:
                        point = (
                            sample["point"][0]
                            + inward[0] * normal_offset
                            + tangent[0] * tangent_offset,
                            sample["point"][1]
                            + inward[1] * normal_offset
                            + tangent[1] * tangent_offset,
                        )
                        probe = FreeCAD.Vector(
                            point[0], point[1], 0.0
                        )
                        for face, elevation in concrete_support_faces:
                            bounds = face.BoundBox
                            if (
                                point[0] < bounds.XMin - 1e-6
                                or point[0] > bounds.XMax + 1e-6
                                or point[1] < bounds.YMin - 1e-6
                                or point[1] > bounds.YMax + 1e-6
                            ):
                                continue
                            if face.isInside(probe, 1e-6, True):
                                elevations.append(elevation)
                    return max(elevations) if elevations else None

                for post_index, fraction in enumerate((0.0, 1.0)):
                    sample = sample_handrail_path(path, fraction)
                    attachment_sample = sample
                    corner_point = corner_post_points.get(
                        (side, flight_index, post_index)
                    )
                    if corner_point is not None:
                        sample = dict(sample)
                        sample["point"] = corner_point
                    junction_sample = sample_handrail_path(
                        junction_path or path, fraction
                    )
                    position_key = (
                        round(junction_sample["point"][0], 6),
                        round(junction_sample["point"][1], 6),
                    )
                    if position_key in post_positions:
                        continue
                    post_positions.add(position_key)
                    span = stringer_span(attachment_sample)
                    local_concrete_support = concrete_support(
                        sample,
                        post_shape,
                        post_width,
                        post_thickness,
                    )
                    first_floor_post = (
                        flight_index == 0 and post_index == 0
                    )
                    if first_floor_post:
                        bottom = 0.0
                    elif str(stair.StairType) == "Wood" and span:
                        bottom = span[0] - _quantity_value(
                            stair.HandrailPostBelowStringer
                        )
                    elif local_concrete_support is not None:
                        bottom = local_concrete_support
                    else:
                        bottom = sample["support"]
                    top = sample["top"] + _quantity_value(
                        stair.HandrailPostAboveTopRail
                    )
                    desired.append(
                        {
                            "role": "HandrailPost",
                            "flight": flight,
                            "side": side,
                            "element_index": post_index,
                            "shape": (
                                make_handrail_vertical_member_shape(
                                    sample["point"],
                                    sample["tangent"],
                                    bottom,
                                    top,
                                    post_shape,
                                    post_width,
                                    post_thickness,
                                )
                            ),
                            "label": (
                                f"{translate('BIM', 'Flight')} "
                                f"{flight_index + 1}: {side_label} "
                                f"{translate('BIM', 'post')} "
                                f"{post_index + 1}"
                            ),
                        }
                    )

                fractions = handrail_picket_fractions(
                    path["length"],
                    post_path_size,
                    picket_path_size,
                    _quantity_value(
                        stair.HandrailPicketMaximumSpacing
                    ),
                )
                for picket_index, fraction in enumerate(fractions):
                    sample = sample_handrail_path(path, fraction)
                    span = stringer_span(sample)
                    local_concrete_support = concrete_support(
                        sample,
                        picket_shape,
                        picket_width,
                        picket_thickness,
                    )
                    if str(stair.StairType) == "Wood" and span:
                        bottom = span[1] - _quantity_value(
                            stair.HandrailPicketStringerPenetration
                        )
                    elif str(stair.StairType) == "Wood":
                        bottom = sample["support"] - _quantity_value(
                            stair.HandrailPicketStringerPenetration
                        )
                    elif local_concrete_support is not None:
                        bottom = local_concrete_support
                    else:
                        bottom = sample["support"]
                    top = (
                        sample["top"]
                        - rail_depth
                        + _quantity_value(
                            stair.HandrailPicketTopRailPenetration
                        )
                    )
                    desired.append(
                        {
                            "role": "HandrailPicket",
                            "flight": flight,
                            "side": side,
                            "element_index": picket_index,
                            "shape": (
                                make_handrail_vertical_member_shape(
                                    sample["point"],
                                    sample["tangent"],
                                    bottom,
                                    top,
                                    picket_shape,
                                    picket_width,
                                    picket_thickness,
                                )
                            ),
                            "label": (
                                f"{translate('BIM', 'Flight')} "
                                f"{flight_index + 1}: {side_label} "
                                f"{translate('BIM', 'picket')} "
                                f"{picket_index + 1}"
                            ),
                        }
                    )

        _sync_handrail_parts(
            group,
            stair,
            desired,
            allow_structure_changes,
        )
