# SPDX-License-Identifier: LGPL-2.1-or-later

"""Document objects used by the BIM Stair Designer."""

import math

import FreeCAD
import Part

from stairdesigner import geometry


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


def _add_property(obj, type_id, name, group, description, default=None, editor_mode=None):
    added = False
    if name not in obj.PropertiesList:
        obj.addProperty(type_id, name, group, description, locked=True)
        added = True
        if default is not None:
            setattr(obj, name, default)
    if editor_mode is not None:
        obj.setEditorMode(name, editor_mode)
    return added


def _quantity_value(value):
    return float(value.Value) if hasattr(value, "Value") else float(value)


def _first_flight(stair):
    flights = get_flights(stair)
    return flights[0] if flights else None


def get_flights(stair):
    """Return the stair flights in their document order."""

    group = getattr(stair, "FlightsGroup", None)
    if not group:
        return []
    return [
        obj
        for obj in group.Group
        if getattr(getattr(obj, "Proxy", None), "Type", "") == "Flight"
    ]


def _is_circular_flight(flight):
    return str(flight.FlightType).startswith("Circular")


def _is_landing_flight(flight):
    return str(flight.FlightType).endswith("landing")


def _flight_length(flight):
    if _is_circular_flight(flight) and all(
        name in flight.PropertiesList for name in ("InnerRadius", "OuterRadius")
    ):
        center_radius = (
            _quantity_value(flight.InnerRadius)
            + _quantity_value(flight.OuterRadius)
        ) / 2.0
        sweep = math.radians(min(abs(_quantity_value(flight.Angle)), 359.999))
        return max(center_radius * sweep, 0.01)
    return (
        _quantity_value(flight.LeftLength) + _quantity_value(flight.RightLength)
    ) / 2.0


def _flight_path_dimension(flight):
    if _is_circular_flight(flight):
        return _quantity_value(flight.InnerRadius)
    return _flight_length(flight)


def _stringer_run_plane(sections, side, width):
    """Return a flat board plane and its selected side-rail line."""

    candidates = [
        section for section in sections if section.locked_to_flight
    ] or list(sections)
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
        key=lambda section: (
            section.tangent[0] - tangent_key[0]
        ) ** 2
        + (section.tangent[1] - tangent_key[1]) ** 2,
    )
    length = math.hypot(*tangent_key)
    direction = (
        tangent_key[0] / max(length, 1e-9),
        tangent_key[1] / max(length, 1e-9),
    )
    normal = (-direction[1], direction[0])
    selected_origin = (
        reference.left if side == "Left" else reference.right
    )
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


def _line_intersection(first, second):
    first_origin, first_direction = first
    second_origin, second_direction = second
    denominator = (
        first_direction[0] * second_direction[1]
        - first_direction[1] * second_direction[0]
    )
    if abs(denominator) < 1e-9:
        return None
    difference = (
        second_origin[0] - first_origin[0],
        second_origin[1] - first_origin[1],
    )
    distance = (
        difference[0] * second_direction[1]
        - difference[1] * second_direction[0]
    ) / denominator
    return (
        first_origin[0] + first_direction[0] * distance,
        first_origin[1] + first_direction[1] * distance,
    )


def _stringer_center_line(rail_line, side, profile):
    """Return the plan centerline of a straight stringer board."""

    origin, direction = rail_line
    inward = (
        (direction[1], -direction[0])
        if side == "Left"
        else (-direction[1], direction[0])
    )
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


def _planar_stringer_runs(flight_runs, flights, side, profiles=None):
    """Keep each straight-flight board planar and butt adjacent boards."""

    profiles = profiles or {}
    planes = {}
    center_lines = {}
    for run_index, (flight_index, sections) in enumerate(flight_runs):
        if str(flights[flight_index].FlightType).startswith("Straight"):
            planes[run_index] = _stringer_run_plane(
                sections,
                side,
                max(_quantity_value(flights[flight_index].Width), 0.01),
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
            if (
                previous_index + 1 == flight_index
                and str(flights[previous_index].FlightType).startswith(
                    "Straight"
                )
            ):
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
            if (
                flight_index + 1 == following_index
                and str(flights[following_index].FlightType).startswith(
                    "Straight"
                )
            ):
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
                geometry.planar_stringer_sections(
                    sections,
                    side,
                    right_origin,
                    math.degrees(math.atan2(direction[1], direction[0])),
                    max(_quantity_value(flight.Width), 0.01),
                    start_seam,
                    end_seam,
                ),
            )
        )
    return result


def linked_flight_side_lengths(
    left_length,
    right_length,
    next_width=0.0,
    next_rotation=None,
    driver=None,
):
    """Return linked rail lengths while preserving the requested input."""

    difference = max(float(next_width), 0.0) if next_rotation else 0.0
    signed_difference = (
        -difference if str(next_rotation) == "Right" else difference
    )
    return linked_flight_side_lengths_for_difference(
        left_length,
        right_length,
        signed_difference,
        driver,
    )


def linked_flight_side_lengths_for_difference(
    left_length,
    right_length,
    signed_difference,
    driver=None,
):
    """Link rails to a signed ``right length - left length`` constraint."""

    minimum = 1.0
    left = max(float(left_length), minimum)
    right = max(float(right_length), minimum)
    difference = float(signed_difference)

    if driver == "LeftLength":
        left = max(left, minimum, minimum - difference)
        right = left + difference
    elif driver == "RightLength":
        right = max(right, minimum, minimum + difference)
        left = right - difference
    else:
        center_length = (left + right) / 2.0
        left = center_length - difference / 2.0
        right = center_length + difference / 2.0
        if min(left, right) < minimum:
            adjustment = minimum - min(left, right)
            left += adjustment
            right += adjustment
    return left, right


def straight_turn_side_difference(current_width, next_width, angle):
    """Return the signed outer-minus-inner length at a straight-flight miter."""

    radians = math.radians(abs(float(angle)))
    sine = abs(math.sin(radians))
    if sine < 1e-7:
        return 0.0
    return (
        max(float(next_width), 0.0)
        - max(float(current_width), 0.0) * math.cos(radians)
    ) / sine


def flight_side_length_difference(stair, flight):
    """Return the required signed right-minus-left rail length."""

    if _is_circular_flight(flight):
        return 0.0
    flights = get_flights(stair)
    try:
        index = flights.index(flight)
    except ValueError:
        return 0.0
    difference = 0.0
    previous_flight = flights[index - 1] if index > 0 else None
    if (
        previous_flight
        and not _is_circular_flight(previous_flight)
        and abs(_quantity_value(flight.Angle)) > 1e-7
    ):
        turn_difference = straight_turn_side_difference(
            _quantity_value(flight.Width),
            _quantity_value(previous_flight.Width),
            _quantity_value(flight.Angle),
        )
        if str(flight.Rotation) == "Right":
            turn_difference = -turn_difference
        difference += turn_difference
    next_flight = flights[index + 1] if index + 1 < len(flights) else None
    if (
        next_flight
        and not _is_circular_flight(next_flight)
        and abs(_quantity_value(next_flight.Angle)) > 1e-7
    ):
        turn_difference = straight_turn_side_difference(
            _quantity_value(flight.Width),
            _quantity_value(next_flight.Width),
            _quantity_value(next_flight.Angle),
        )
        if str(next_flight.Rotation) == "Right":
            turn_difference = -turn_difference
        difference += turn_difference
    all_straight = all(
        str(item.FlightType) == "Straight" for item in flights
    )
    if all_straight and index == 0:
        difference += _quantity_value(flight.Width) * math.tan(
            math.radians(
                min(max(_quantity_value(flight.StartAngle), -89.0), 89.0)
            )
        )
    if all_straight and index == len(flights) - 1:
        difference -= _quantity_value(flight.Width) * math.tan(
            math.radians(
                min(max(_quantity_value(flight.EndAngle), -89.0), 89.0)
            )
        )
    return difference


def linked_circular_radii(inner_radius, outer_radius, width, driver=None):
    """Return radii linked by the flight width while preserving the driver."""

    minimum = 1.0
    width = max(float(width), minimum)
    inner = max(float(inner_radius), minimum)
    outer = max(float(outer_radius), minimum + width)
    if driver == "OuterRadius":
        inner = outer - width
    else:
        outer = inner + width
    return inner, outer


def sync_circular_radii(flight, driver=None):
    """Link a circular flight's inner and outer radii through its width."""

    inner, outer = linked_circular_radii(
        _quantity_value(flight.InnerRadius),
        _quantity_value(flight.OuterRadius),
        _quantity_value(flight.Width),
        driver,
    )
    proxy = getattr(flight, "Proxy", None)
    was_updating = getattr(proxy, "_updating", False)
    if proxy:
        proxy._updating = True
    try:
        flight.InnerRadius = inner
        flight.OuterRadius = outer
    finally:
        if proxy:
            proxy._updating = was_updating


def sync_flight_side_lengths(stair, flight, driver=None):
    """Link one flight's rail lengths to the turn into its next flight."""

    if _is_circular_flight(flight):
        sync_circular_radii(flight, driver)
        return
    flights = get_flights(stair)
    try:
        index = flights.index(flight)
    except ValueError:
        return
    left, right = linked_flight_side_lengths_for_difference(
        _quantity_value(flight.LeftLength),
        _quantity_value(flight.RightLength),
        flight_side_length_difference(stair, flight),
        driver,
    )
    proxy = getattr(flight, "Proxy", None)
    was_updating = getattr(proxy, "_updating", False)
    if proxy:
        proxy._updating = True
    try:
        flight.LeftLength = left
        flight.RightLength = right
    finally:
        if proxy:
            proxy._updating = was_updating


def sync_all_flight_side_lengths(stair):
    """Link all rail lengths without changing the flights' center lengths."""

    for flight in get_flights(stair):
        sync_flight_side_lengths(stair, flight)


def _combined_placement(stair, local_placement):
    return stair.Placement.multiply(local_placement)


class StairProxy:
    """Parametric root object for a Stair Designer stair."""

    Type = "StairDesigner"

    def __init__(self, obj):
        self._updating = True
        obj.Proxy = self
        self.Object = obj
        self.set_properties(obj)
        self._updating = False

    def set_properties(self, obj):
        for deprecated in (
            "Model",
            "WalkingLineOffset",
            "WindingCoefficient",
            "StepMaterial",
            "RiserMaterial",
            "StringboardsGroup",
            "LateralCutStringsGroup",
            "LeftStringerType",
            "RightStringerType",
            "LeftStringerThickness",
            "LeftStringerWidth",
            "LeftStringerOffset",
            "LeftStringerStartExtension",
            "LeftStringerEndExtension",
            "LeftStringerNosingOffsetDirection",
            "LeftStringerNosingOffset",
            "RightStringerThickness",
            "RightStringerWidth",
            "RightStringerOffset",
            "RightStringerStartExtension",
            "RightStringerEndExtension",
            "RightStringerNosingOffsetDirection",
            "RightStringerNosingOffset",
        ):
            if deprecated in obj.PropertiesList:
                obj.removeProperty(deprecated)
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "StairType",
            "Stair",
            QT_TRANSLATE_NOOP("App::Property", "The construction type of this stair"),
        )
        if added:
            obj.StairType = ["Wood", "Concrete"]
        _add_property(
            obj,
            "App::PropertyLength",
            "FloorHeight",
            "Stair",
            QT_TRANSLATE_NOOP("App::Property", "The floor-to-floor height"),
            2800.0,
        )
        _add_property(
            obj,
            "App::PropertyIntegerConstraint",
            "NumberOfSteps",
            "Stair",
            QT_TRANSLATE_NOOP("App::Property", "The number of risers in this stair"),
            (15, 2, 1000, 1),
        )
        _add_property(
            obj,
            "App::PropertyPlacement",
            "Placement",
            "Stair",
            QT_TRANSLATE_NOOP("App::Property", "The placement of this stair"),
        )
        _add_property(
            obj,
            "App::PropertyString",
            "IfcType",
            "BIM",
            QT_TRANSLATE_NOOP("App::Property", "The IFC entity type"),
            "Stair",
        )

        _add_property(
            obj,
            "App::PropertyInteger",
            "NumberOfTreads",
            "Design check",
            QT_TRANSLATE_NOOP("App::Property", "The number of manufactured treads"),
            editor_mode=1,
        )

        _add_property(
            obj,
            "App::PropertyLength",
            "ConcreteThickness",
            "Concrete",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Additional waist thickness below the inner step edges",
            ),
            150.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "RiserHeight",
            "Design check",
            QT_TRANSLATE_NOOP("App::Property", "The computed riser height"),
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "TreadWidth",
            "Design check",
            QT_TRANSLATE_NOOP("App::Property", "The computed going between nosings"),
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "BlondelValue",
            "Design check",
            QT_TRANSLATE_NOOP("App::Property", "The computed Blondel value (2 risers + going)"),
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "BlondelCompliant",
            "Design check",
            QT_TRANSLATE_NOOP(
                "App::Property", "Whether the Blondel value is between 620 and 640 mm"
            ),
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyString",
            "GeometryStatus",
            "Design check",
            QT_TRANSLATE_NOOP("App::Property", "Current geometry implementation status"),
            "Straight flight",
            editor_mode=1,
        )

        _add_property(
            obj,
            "App::PropertyLength",
            "StepThickness",
            "Steps",
            QT_TRANSLATE_NOOP("App::Property", "The thickness of wooden treads"),
            40.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "Nosing",
            "Steps",
            QT_TRANSLATE_NOOP("App::Property", "The tread projection beyond the riser"),
            30.0,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "RisersEnabled",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "Creates individual wooden risers"),
            True,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "RiserThickness",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "The thickness of wooden risers"),
            18.0,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "PriorityToRiser",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "Gives the riser priority at tread intersections"),
            False,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StepRiserOverlap",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "The overlap between a tread and its riser"),
            10.0,
        )
        _add_property(
            obj,
            "App::PropertyDistance",
            "RiserUpperOffset",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "The upper riser offset"),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyDistance",
            "RiserLowerOffset",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "The lower riser offset"),
            0.0,
        )

        _add_property(
            obj,
            "App::PropertyLength",
            "StringerThickness",
            "Stringers",
            QT_TRANSLATE_NOOP("App::Property", "Default stringer thickness"),
            40.0,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "StringerCustomWidth",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property", "Uses a manually specified stringer width"
            ),
            False,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerWidth",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property", "Default stringer board width"
            ),
            300.0,
        )
        _add_property(
            obj,
            "App::PropertyDistance",
            "StringerStepOverlap",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Tread overlap into or beyond the stringer",
            ),
            20.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerStartExtension",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property", "Length beyond the first step of the stair"
            ),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerEndExtension",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property", "Length beyond the last step of the stair"
            ),
            0.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "StringerNosingOffsetDirection",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "How the stringer offset above nosings is measured",
            ),
        )
        current_direction = (
            "Perpendicular"
            if added
            else str(obj.StringerNosingOffsetDirection)
        )
        directions = ["Perpendicular", "Vertical"]
        obj.StringerNosingOffsetDirection = directions
        obj.StringerNosingOffsetDirection = (
            current_direction
            if current_direction in directions
            else "Perpendicular"
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerNosingOffset",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property", "Default board position above the nosings"
            ),
            50.0,
        )
        obj.setEditorMode(
            "StringerWidth", 0 if obj.StringerCustomWidth else 1
        )

        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailHeightAboveNosing",
            "Handrails",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Vertical distance from the nosings to the top of the top rail",
            ),
            900.0,
        )
        _add_property(
            obj,
            "App::PropertyDistance",
            "HandrailOffset",
            "Handrails",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Offset from the stringer center toward the stair interior",
            ),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketMaximumSpacing",
            "Handrails",
            QT_TRANSLATE_NOOP(
                "App::Property", "Maximum clear spacing between pickets"
            ),
            100.0,
        )
        self._set_shape_property(
            obj,
            "HandrailPicketShape",
            "Pickets",
            "Cross-section shape of the pickets",
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketWidth",
            "Pickets",
            QT_TRANSLATE_NOOP("App::Property", "Picket width or diameter"),
            20.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketThickness",
            "Pickets",
            QT_TRANSLATE_NOOP("App::Property", "Picket thickness"),
            20.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketStringerPenetration",
            "Pickets",
            QT_TRANSLATE_NOOP(
                "App::Property", "Picket penetration into the stringer"
            ),
            20.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketTopRailPenetration",
            "Pickets",
            QT_TRANSLATE_NOOP(
                "App::Property", "Picket penetration into the top rail"
            ),
            10.0,
        )
        self._set_shape_property(
            obj,
            "HandrailPostShape",
            "Posts",
            "Cross-section shape of the posts",
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPostWidth",
            "Posts",
            QT_TRANSLATE_NOOP("App::Property", "Post width or diameter"),
            70.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPostThickness",
            "Posts",
            QT_TRANSLATE_NOOP("App::Property", "Post thickness"),
            70.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPostAboveTopRail",
            "Posts",
            QT_TRANSLATE_NOOP(
                "App::Property", "Post length above the top rail"
            ),
            70.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPostBelowStringer",
            "Posts",
            QT_TRANSLATE_NOOP(
                "App::Property", "Post length below a wooden stringer"
            ),
            100.0,
        )
        self._set_shape_property(
            obj,
            "HandrailTopRailShape",
            "Top rail",
            "Cross-section shape of the top rail",
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailTopRailWidth",
            "Top rail",
            QT_TRANSLATE_NOOP("App::Property", "Top-rail width or diameter"),
            50.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailTopRailThickness",
            "Top rail",
            QT_TRANSLATE_NOOP("App::Property", "Top-rail thickness"),
            40.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailTopRailPostPenetration",
            "Top rail",
            QT_TRANSLATE_NOOP(
                "App::Property", "Top-rail penetration into end posts"
            ),
            35.0,
        )

        for name in (
            "PlanSketch",
            "FlightsGroup",
            "StepsGroup",
            "StringersGroup",
            "HandrailsGroup",
            "ConcreteGeometry",
        ):
            _add_property(
                obj,
                "App::PropertyLink",
                name,
                "Generated objects",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A generated Stair Designer child object",
                ),
                editor_mode=2,
            )

    @staticmethod
    def _set_shape_property(obj, name, group, description):
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            name,
            group,
            QT_TRANSLATE_NOOP("App::Property", description),
        )
        current = "Square" if added else str(getattr(obj, name))
        choices = ["Square", "Circular"]
        setattr(obj, name, choices)
        setattr(obj, name, current if current in choices else "Square")

    def onDocumentRestored(self, obj):
        self._updating = True
        self.Object = obj
        self.set_properties(obj)
        self._updating = False

    def onChanged(self, obj, prop):
        if (
            getattr(self, "_updating", False)
            or FreeCAD.isRestoring()
            or obj.Document.Transacting
        ):
            return
        if prop in {
            "StairType",
            "FloorHeight",
            "NumberOfSteps",
            "Placement",
            "ConcreteThickness",
            "StepThickness",
            "Nosing",
            "RisersEnabled",
            "RiserThickness",
            "PriorityToRiser",
            "StepRiserOverlap",
            "RiserUpperOffset",
            "RiserLowerOffset",
            "StringerThickness",
            "StringerCustomWidth",
            "StringerWidth",
            "StringerStepOverlap",
            "StringerStartExtension",
            "StringerEndExtension",
            "StringerNosingOffsetDirection",
            "StringerNosingOffset",
        } or prop.startswith("Handrail"):
            if prop == "StringerCustomWidth":
                obj.setEditorMode(
                    "StringerWidth", 0 if obj.StringerCustomWidth else 1
                )
            self.rebuild(obj, allow_structure_changes=True)

    def execute(self, obj):
        # Geometry is rebuilt from onChanged() and the task panel. Rewriting a
        # child Sketch during the parent's recompute would keep the document
        # dirty and trigger additional recompute passes.
        return

    def rebuild(self, obj, allow_structure_changes=True):
        if getattr(self, "_updating", False):
            return
        flights = get_flights(obj)
        if not flights:
            return
        for index, flight in enumerate(flights):
            if any(
                name not in flight.PropertiesList
                for name in (
                    "WindingLocal",
                    "LeftStringerType",
                    "RightStringerType",
                    "LeftHandrailEnabled",
                    "RightHandrailEnabled",
                )
            ) or any(
                str(getattr(flight, f"{side}StringerType", ""))
                == "Cut stringer"
                for side in ("Left", "Right")
            ):
                flight_proxy = flight.Proxy
                was_updating = getattr(flight_proxy, "_updating", False)
                flight_proxy._updating = True
                try:
                    flight_proxy.set_properties(flight)
                finally:
                    flight_proxy._updating = was_updating
            mode = 2 if index == 0 else 0
            flight.setEditorMode("WindingLocal", mode)
            flight.setEditorMode("WindingDistant", mode)

        self._updating = True
        try:
            floor_height = _quantity_value(obj.FloorHeight)
            total_treads = max(int(obj.NumberOfSteps) - 1, 1)
            riser_height = floor_height / max(int(obj.NumberOfSteps), 2)
            flight_lengths = [_flight_length(flight) for flight in flights]
            stair_indices = [
                index
                for index, flight in enumerate(flights)
                if not _is_landing_flight(flight)
            ]
            stair_tread_counts = geometry.distribute_treads(
                [flight_lengths[index] for index in stair_indices],
                total_treads,
            )
            tread_counts = [0] * len(flights)
            for index, count in zip(stair_indices, stair_tread_counts):
                tread_counts[index] = count
            layouts = self._flight_layouts(flights, tread_counts, riser_height)
            balanced_sections = None
            balanced_footprint = None
            balanced_plan_shapes = None
            winding_geometry_valid = True
            has_circular = any(
                _is_circular_flight(flight) for flight in flights
            )
            has_landing_flight = any(
                _is_landing_flight(flight) for flight in flights
            )
            has_tangent_geometry = has_circular or has_landing_flight
            first_flight = flights[0]
            last_flight = flights[-1]
            start_angle = (
                _quantity_value(first_flight.StartAngle)
                if str(first_flight.FlightType) == "Straight"
                else 0.0
            )
            end_angle = (
                _quantity_value(last_flight.EndAngle)
                if str(last_flight.FlightType) == "Straight"
                else 0.0
            )
            entry_direction = (
                str(first_flight.EntryDirection)
                if str(first_flight.FlightType) == "Straight"
                else "Straight"
            )
            exit_direction = (
                str(last_flight.ExitDirection)
                if str(last_flight.FlightType) == "Straight"
                else "Straight"
            )
            has_endpoint_geometry = (
                abs(start_angle) > 1e-7
                or abs(end_angle) > 1e-7
                or entry_direction != "Straight"
                or exit_direction != "Straight"
            )
            winding_parameters = [
                (
                    _quantity_value(flight.WindingLocal),
                    _quantity_value(flight.WindingDistant),
                )
                for flight in flights[1:]
            ]
            if has_tangent_geometry:
                flight_specs = [
                    (
                        str(flight.FlightType),
                        _flight_path_dimension(flight),
                        _quantity_value(flight.Width),
                        _quantity_value(flight.Angle),
                        str(flight.Rotation),
                        str(flight.EntryDirection),
                        str(flight.ExitDirection),
                    )
                    for flight in flights
                ]
                turn_types = [str(flight.TurnType) for flight in flights[1:]]
                balanced_sections, average_going = (
                    geometry.tangent_flight_sections(
                        flight_specs,
                        total_treads,
                        1.0,
                        turn_types,
                        start_angle,
                        end_angle,
                        entry_direction,
                        exit_direction,
                        obj.StairType != "Concrete",
                        winding_parameters=winding_parameters,
                    )
                )
                balanced_footprint = geometry.make_tangent_stair_footprint(
                    flight_specs, turn_types, start_angle, end_angle
                )
                balanced_sections = (
                    geometry.fit_tangent_sections_to_footprint(
                        balanced_sections, balanced_footprint
                    )
                )
                if has_landing_flight:
                    partition_faces = geometry.tangent_tread_faces(
                        balanced_sections, flight_specs
                    )
                    winding_geometry_valid = (
                        len(partition_faces) == len(balanced_sections) - 1
                        and all(face.isValid() for face in partition_faces)
                    )
                else:
                    partition_faces = geometry.balanced_tread_faces(
                        balanced_sections, balanced_footprint
                    )
                    winding_geometry_valid = geometry.balanced_partition_is_valid(
                        partition_faces,
                        balanced_footprint,
                        len(balanced_sections) - 1,
                    )
                balanced_plan_shapes = partition_faces
            elif len(flights) > 1 or has_endpoint_geometry:
                flight_specs = [
                    (
                        layout["metrics"].flight_length,
                        layout["width"],
                        layout["heading"],
                    )
                    for layout in layouts
                ]
                balanced_sections, average_going = geometry.balanced_winder_sections(
                    flight_specs,
                    total_treads,
                    1.0,
                    [str(flight.TurnType) for flight in flights[1:]],
                    start_angle,
                    end_angle,
                    entry_direction,
                    exit_direction,
                    obj.StairType != "Concrete",
                    winding_parameters=winding_parameters,
                )
                balanced_footprint = geometry.make_stair_footprint(
                    flight_specs, start_angle, end_angle
                )
                balanced_sections = geometry.fit_balanced_sections_to_footprint(
                    balanced_sections, balanced_footprint
                )
                partition_faces = geometry.balanced_tread_faces(
                    balanced_sections, balanced_footprint
                )
                winding_geometry_valid = geometry.balanced_partition_is_valid(
                    partition_faces,
                    balanced_footprint,
                    len(balanced_sections) - 1,
                )
                balanced_plan_shapes = partition_faces
            if balanced_sections:
                if winding_geometry_valid:
                    tread_counts = [0] * len(flights)
                    for section in balanced_sections[:-1]:
                        tread_counts[section.flight_index] += 1
                    for flight, tread_count in zip(flights, tread_counts):
                        flight.NumberOfTreads = tread_count
                        flight.TreadWidth = average_going
                else:
                    balanced_sections = None
                    balanced_footprint = None
                    balanced_plan_shapes = None

            if balanced_sections is None:
                average_going = (
                    sum(flight_lengths[index] for index in stair_indices)
                    / total_treads
                )
            blondel_value = 2.0 * riser_height + average_going
            obj.NumberOfTreads = (
                len(balanced_sections) - 1
                if obj.StairType == "Concrete"
                and balanced_sections
                and any(
                    section.landing_to_next
                    for section in balanced_sections[:-1]
                )
                else total_treads
            )
            obj.RiserHeight = riser_height
            obj.TreadWidth = average_going
            obj.BlondelValue = blondel_value
            obj.BlondelCompliant = (
                geometry.BLONDEL_MINIMUM
                <= blondel_value
                <= geometry.BLONDEL_MAXIMUM
            )
            obj.setEditorMode(
                "StringerWidth", 0 if obj.StringerCustomWidth else 1
            )
            if not obj.StringerCustomWidth:
                obj.StringerWidth = geometry.automatic_stringer_width(
                    riser_height,
                    average_going,
                    _quantity_value(obj.StepThickness),
                    _quantity_value(obj.Nosing),
                    _quantity_value(obj.StringerNosingOffset),
                    str(obj.StringerNosingOffsetDirection),
                )
            if has_tangent_geometry and winding_geometry_valid:
                obj.GeometryStatus = translate(
                    "BIM",
                    (
                        "Multi-flight stair with landing"
                        if has_landing_flight
                        else "Tangential circular stair"
                    ),
                )
            elif len(flights) == 1 and not has_endpoint_geometry:
                obj.GeometryStatus = translate("BIM", "Straight flight")
            elif not winding_geometry_valid:
                obj.GeometryStatus = translate(
                    "BIM",
                    "Flights overlap in plan; balanced winding is unavailable",
                )
            else:
                if len(flights) == 1:
                    obj.GeometryStatus = translate(
                        "BIM", "Balanced entry/exit stair"
                    )
                elif any(section.landing_to_next for section in balanced_sections):
                    obj.GeometryStatus = translate(
                        "BIM", "Multi-flight stair with landing"
                    )
                else:
                    obj.GeometryStatus = translate(
                        "BIM", "Balanced multi-flight stair"
                    )

            self._update_plan(
                obj, layouts, balanced_sections, balanced_footprint
            )
            if obj.StairType == "Concrete":
                if allow_structure_changes:
                    self._remove_steps_group(obj)
                    self._remove_stringers_group(obj)
                self._update_concrete(
                    obj,
                    layouts,
                    balanced_sections,
                    balanced_footprint,
                    balanced_plan_shapes,
                    allow_structure_changes,
                )
            else:
                if allow_structure_changes:
                    self._remove_concrete(obj)
                self._update_wood(
                    obj,
                    layouts,
                    balanced_sections,
                    balanced_footprint,
                    balanced_plan_shapes,
                    allow_structure_changes,
                )
                self._update_stringers(
                    obj,
                    layouts,
                    balanced_sections,
                    riser_height,
                    allow_structure_changes,
                )
            self._update_handrails(
                obj,
                layouts,
                balanced_sections,
                balanced_plan_shapes,
                riser_height,
                allow_structure_changes,
            )
        finally:
            self._updating = False

    def _flight_layouts(self, flights, tread_counts, riser_height):
        layouts = []
        first_width = max(_quantity_value(flights[0].Width), 0.01)
        center = FreeCAD.Vector(0.0, first_width / 2.0, 0.0)
        heading = 0.0
        base_z = 0.0
        for index, (flight, tread_count) in enumerate(zip(flights, tread_counts)):
            if index:
                turn = _quantity_value(flight.Angle)
                if str(flight.Rotation) == "Right":
                    turn = -turn
                heading += turn
            width = max(_quantity_value(flight.Width), 0.01)
            length = _flight_length(flight)
            metrics = geometry.flight_stair_metrics(
                length, tread_count, riser_height
            )
            radians = math.radians(heading)
            direction = FreeCAD.Vector(math.cos(radians), math.sin(radians), 0.0)
            normal = FreeCAD.Vector(-direction.y, direction.x, 0.0)
            origin = center - normal * (width / 2.0)
            placement = FreeCAD.Placement(
                FreeCAD.Vector(origin.x, origin.y, base_z),
                FreeCAD.Rotation(FreeCAD.Vector(0.0, 0.0, 1.0), heading),
            )
            flight.NumberOfTreads = tread_count
            flight.TreadWidth = metrics.tread_width
            layouts.append(
                {
                    "flight": flight,
                    "index": index,
                    "metrics": metrics,
                    "width": width,
                    "heading": heading,
                    "placement": placement,
                }
            )
            center += direction * length
            if not _is_landing_flight(flight):
                base_z += tread_count * riser_height
        return layouts

    def _update_plan(
        self,
        stair,
        layouts,
        balanced_sections=None,
        balanced_footprint=None,
    ):
        sketch = stair.PlanSketch
        if not sketch:
            return
        sketch.deleteAllGeometry(True)
        lines = []
        if balanced_sections:
            lines.extend(
                geometry.balanced_plan_geometry(
                    balanced_sections, balanced_footprint
                )
            )
        else:
            for layout in layouts:
                placement = layout["placement"]
                plan_placement = FreeCAD.Placement(
                    FreeCAD.Vector(
                        placement.Base.x,
                        placement.Base.y,
                        0.0,
                    ),
                    placement.Rotation,
                )
                for start, end in geometry.plan_segments(
                    layout["metrics"],
                    layout["width"],
                    _quantity_value(stair.Nosing),
                ):
                    start_point = plan_placement.multVec(
                        FreeCAD.Vector(start[0], start[1], 0.0)
                    )
                    end_point = plan_placement.multVec(
                        FreeCAD.Vector(end[0], end[1], 0.0)
                    )
                    lines.append(Part.LineSegment(start_point, end_point))
        sketch.addGeometry(lines, False)
        sketch.Placement = stair.Placement

    def _update_wood(
        self,
        stair,
        layouts,
        balanced_sections,
        balanced_footprint,
        balanced_plan_shapes,
        allow_structure_changes,
    ):
        group = stair.StepsGroup
        if not group and allow_structure_changes:
            group = _make_component_group(stair, "StepsGroup", "Steps", "steps")
        if not group:
            return
        group.PanelSection = "steps"
        group.Proxy.Section = "steps"

        total_treads = (
            len(balanced_sections) - 1
            if balanced_sections
            else sum(layout["metrics"].tread_count for layout in layouts)
        )
        treads = _generated_parts(group, stair, "Tread")
        riser_count = total_treads if stair.RisersEnabled else 0
        risers = _generated_parts(group, stair, "Riser")
        if allow_structure_changes:
            treads = _resize_generated_parts(group, stair, "Tread", total_treads)
            risers = _resize_generated_parts(group, stair, "Riser", riser_count)

        step_thickness = max(_quantity_value(stair.StepThickness), 0.01)
        nosing = max(_quantity_value(stair.Nosing), 0.0)
        riser_thickness = max(_quantity_value(stair.RiserThickness), 0.01)
        step_riser_overlap = max(_quantity_value(stair.StepRiserOverlap), 0.0)
        if balanced_sections:
            self._update_balanced_wood_parts(
                stair,
                treads,
                risers,
                balanced_sections,
                balanced_footprint,
                balanced_plan_shapes,
                step_thickness,
                nosing,
                riser_thickness,
                step_riser_overlap,
            )
            return
        generated_index = 0
        for layout in layouts:
            metrics = layout["metrics"]
            for local_index in range(metrics.tread_count):
                tread = treads[generated_index]
                tread.Label = f"{translate('BIM', 'Step')} {generated_index + 1}"
                tread.Index = generated_index + 1
                tread.FlightIndex = layout["index"] + 1
                has_next_riser = (
                    stair.RisersEnabled and local_index < metrics.tread_count - 1
                )
                back_extension = 0.0
                if has_next_riser:
                    back_extension = step_riser_overlap
                    if not stair.PriorityToRiser:
                        back_extension += riser_thickness
                tread.Shape = geometry.make_tread_shape(
                    local_index,
                    metrics,
                    layout["width"],
                    step_thickness,
                    nosing,
                    back_extension,
                )
                tread.Placement = _combined_placement(stair, layout["placement"])
                if FreeCAD.GuiUp:
                    tread.ViewObject.ShapeColor = (0.72, 0.48, 0.25)
                generated_index += 1

        if not stair.RisersEnabled:
            return
        upper_offset = _quantity_value(stair.RiserUpperOffset)
        lower_offset = _quantity_value(stair.RiserLowerOffset)
        generated_index = 0
        for layout in layouts:
            metrics = layout["metrics"]
            for local_index in range(metrics.tread_count):
                riser = risers[generated_index]
                riser.Label = f"{translate('BIM', 'Riser')} {generated_index + 1}"
                riser.Index = generated_index + 1
                riser.FlightIndex = layout["index"] + 1
                riser.Shape = geometry.make_riser_shape(
                    local_index,
                    metrics,
                    layout["width"],
                    riser_thickness,
                    step_thickness,
                    upper_offset,
                    lower_offset,
                    stair.PriorityToRiser,
                )
                riser.Placement = _combined_placement(stair, layout["placement"])
                if FreeCAD.GuiUp:
                    riser.ViewObject.ShapeColor = (0.58, 0.36, 0.18)
                generated_index += 1

    def _update_balanced_wood_parts(
        self,
        stair,
        treads,
        risers,
        sections,
        footprint,
        plan_shapes,
        step_thickness,
        nosing,
        riser_thickness,
        step_riser_overlap,
    ):
        tread_count = len(sections) - 1
        riser_height = _quantity_value(stair.RiserHeight)
        base_faces = plan_shapes or geometry.balanced_tread_faces(
            sections, footprint
        )
        for index, (front, rear, base_face) in enumerate(
            zip(sections, sections[1:], base_faces)
        ):
            tread = treads[index]
            tread.Label = f"{translate('BIM', 'Step')} {index + 1}"
            tread.Index = index + 1
            tread.FlightIndex = front.flight_index + 1
            has_next_riser = stair.RisersEnabled and index < tread_count - 1
            back_extension = 0.0
            if has_next_riser:
                back_extension = step_riser_overlap
                if not stair.PriorityToRiser:
                    back_extension += riser_thickness
            tread.Shape = geometry.make_balanced_tread_shape(
                front,
                rear,
                footprint,
                geometry.balanced_section_top(
                    front, index, riser_height
                ),
                step_thickness,
                nosing,
                back_extension,
                base_face,
                local_expansion=(
                    front.landing_to_next
                    or rear.landing_to_next
                    or (index > 0 and sections[index - 1].landing_to_next)
                ),
            )
            tread.Placement = stair.Placement
            if FreeCAD.GuiUp:
                tread.ViewObject.ShapeColor = (0.72, 0.48, 0.25)

        if not stair.RisersEnabled:
            return
        upper_offset = _quantity_value(stair.RiserUpperOffset)
        lower_offset = _quantity_value(stair.RiserLowerOffset)
        riser_sections = list(enumerate(sections[:-1]))
        for generated_index, (riser, (index, section)) in enumerate(
            zip(risers, riser_sections)
        ):
            bottom_extension = (
                step_thickness if stair.PriorityToRiser and index > 0 else 0.0
            )
            height = (
                riser_height
                - step_thickness
                + bottom_extension
                - upper_offset
                - lower_offset
            )
            top = geometry.balanced_section_top(
                section, index, riser_height
            )
            base = top - riser_height - bottom_extension + lower_offset
            riser.Label = (
                f"{translate('BIM', 'Riser')} {generated_index + 1}"
            )
            riser.Index = generated_index + 1
            riser.FlightIndex = section.flight_index + 1
            riser.Shape = geometry.make_balanced_riser_shape(
                section,
                base,
                height,
                riser_thickness,
                footprint,
                local_expansion=(
                    section.landing_to_next
                    or (index > 0 and sections[index - 1].landing_to_next)
                ),
            )
            riser.Placement = stair.Placement
            if FreeCAD.GuiUp:
                riser.ViewObject.ShapeColor = (0.58, 0.36, 0.18)

    def _update_concrete(
        self,
        stair,
        layouts,
        balanced_sections,
        balanced_footprint,
        balanced_plan_shapes,
        allow_structure_changes,
    ):
        concrete = stair.ConcreteGeometry
        if not concrete and allow_structure_changes:
            concrete = stair.Document.addObject("Part::Feature", f"{stair.Name}_Concrete")
            concrete.Label = translate("BIM", "Concrete stair")
            _set_generated_properties(concrete, stair, "Concrete")
            stair.addObject(concrete)
            stair.ConcreteGeometry = concrete
        if not concrete:
            return
        if balanced_sections:
            result = geometry.make_balanced_concrete_shape(
                balanced_sections,
                balanced_footprint,
                _quantity_value(stair.RiserHeight),
                _quantity_value(stair.ConcreteThickness),
                balanced_plan_shapes,
            )
        else:
            shapes = []
            for layout in layouts:
                shape = geometry.make_concrete_shape(
                    layout["metrics"],
                    layout["width"],
                    _quantity_value(stair.ConcreteThickness),
                )
                shape.Placement = layout["placement"]
                shapes.append(shape)
            result = shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
        concrete.Shape = result
        concrete.Placement = stair.Placement
        if FreeCAD.GuiUp:
            concrete.ViewObject.ShapeColor = (0.72, 0.72, 0.72)

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
            group = _make_component_group(
                stair, "StringersGroup", "Stringers", "stringers"
            )
        if not group:
            return
        group.PanelSection = "stringers"
        group.Proxy.Section = "stringers"

        if balanced_sections:
            flight_runs = geometry.stringer_flight_runs(
                balanced_sections,
                [str(flight.FlightType) for flight in flights],
            )
        elif len(layouts) == 1:
            layout = layouts[0]
            flight_runs = [
                (
                    0,
                    geometry.straight_stringer_sections(
                        layout["metrics"], layout["width"]
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
            "NosingOffsetDirection": str(
                stair.StringerNosingOffsetDirection
            ),
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
            for (flight_index, _sections), part in zip(
                source_runs, parts
            ):
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
                }

            prepared_runs = _planar_stringer_runs(
                flight_runs,
                flights,
                side,
                profiles,
            )
            side_runs = [
                (flight_index, sections)
                for flight_index, sections in prepared_runs
                if flight_index in profiles
            ]
            labels = {
                ("Left", "Housed stringer"): translate(
                    "BIM", "Left housed stringer"
                ),
                ("Right", "Housed stringer"): translate(
                    "BIM", "Right housed stringer"
                ),
                ("Left", "Notched stringer"): translate(
                    "BIM", "Left notched stringer"
                ),
                ("Right", "Notched stringer"): translate(
                    "BIM", "Right notched stringer"
                ),
            }
            for (flight_index, sections), part in zip(
                side_runs, parts
            ):
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
                    part.Shape = geometry.make_housed_stringer_shape(
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
                    part.Shape = geometry.make_notched_stringer_shape(
                        sections,
                        riser_height,
                        _quantity_value(stair.StepThickness),
                        side,
                        thickness,
                        width,
                        overlap,
                        end_extension,
                        (
                            _quantity_value(stair.RiserThickness)
                            if stair.RisersEnabled
                            else 0.0
                        ),
                    )
                part.Label = (
                    f"{translate('BIM', 'Flight')} {flight_index + 1}: "
                    f"{labels[(side, stringer_type)]}"
                )
                part.Index = flight_index * 2 + side_index
                part.FlightIndex = flight_index + 1
                part.Placement = stair.Placement
                if FreeCAD.GuiUp:
                    part.ViewObject.ShapeColor = (0.46, 0.27, 0.12)

    def _update_handrails(
        self,
        stair,
        layouts,
        balanced_sections,
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
            flight_runs = geometry.stringer_flight_runs(
                balanced_sections,
                [str(flight.FlightType) for flight in flights],
            )
        elif len(layouts) == 1:
            flight_runs = [
                (
                    0,
                    geometry.straight_stringer_sections(
                        layouts[0]["metrics"], layouts[0]["width"]
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
                elevation = geometry.balanced_section_top(
                    balanced_sections[index],
                    index,
                    riser_height,
                )
                concrete_support_faces.extend(
                    (face, elevation)
                    for face in plan_shape.Faces
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
                }
            prepared_handrail_runs[prepared_side] = {
                flight_index: prepared_sections
                for flight_index, prepared_sections in (
                    _planar_stringer_runs(
                        flight_runs,
                        flights,
                        prepared_side,
                        side_profiles or None,
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
                    plane = _stringer_run_plane(
                        sections,
                        prepared_side,
                        max(
                            _quantity_value(flights[flight_index].Width),
                            0.01,
                        ),
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
                path = geometry.make_handrail_path(
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
                junction_path = geometry.make_handrail_path(
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
                top_rail_shape = geometry.make_handrail_top_rail_shape(
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
                    sample = geometry.sample_handrail_path(path, fraction)
                    attachment_sample = sample
                    corner_point = corner_post_points.get(
                        (side, flight_index, post_index)
                    )
                    if corner_point is not None:
                        sample = dict(sample)
                        sample["point"] = corner_point
                    junction_sample = geometry.sample_handrail_path(
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
                                geometry.make_handrail_vertical_member_shape(
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

                fractions = geometry.handrail_picket_fractions(
                    path["length"],
                    post_path_size,
                    picket_path_size,
                    _quantity_value(
                        stair.HandrailPicketMaximumSpacing
                    ),
                )
                for picket_index, fraction in enumerate(fractions):
                    sample = geometry.sample_handrail_path(path, fraction)
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
                                geometry.make_handrail_vertical_member_shape(
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

    def _remove_steps_group(self, stair):
        group = stair.StepsGroup
        if not group:
            return
        for child in list(group.Group):
            if getattr(child, "GeneratedBy", "") == stair.Name:
                group.removeObject(child)
                stair.Document.removeObject(child.Name)
            else:
                group.removeObject(child)
                stair.addObject(child)
        stair.removeObject(group)
        stair.Document.removeObject(group.Name)

    def _remove_stringers_group(self, stair):
        group = stair.StringersGroup
        if not group:
            return
        for child in list(group.Group):
            if getattr(child, "GeneratedBy", "") == stair.Name:
                group.removeObject(child)
                stair.Document.removeObject(child.Name)
            else:
                group.removeObject(child)
                stair.addObject(child)
        stair.removeObject(group)
        stair.Document.removeObject(group.Name)

    def _remove_handrails_group(self, stair):
        group = stair.HandrailsGroup
        if not group:
            return
        for child in list(group.Group):
            if getattr(child, "GeneratedBy", "") == stair.Name:
                group.removeObject(child)
                stair.Document.removeObject(child.Name)
            else:
                group.removeObject(child)
                stair.addObject(child)
        stair.removeObject(group)
        stair.Document.removeObject(group.Name)

    def _remove_concrete(self, stair):
        concrete = stair.ConcreteGeometry
        if not concrete:
            return
        stair.removeObject(concrete)
        stair.Document.removeObject(concrete.Name)

    def dumps(self):
        return None

    def loads(self, state):
        self._updating = False


class FlightProxy:
    """Stores the dimensions and turn direction of one stair flight."""

    Type = "Flight"

    def __init__(self, obj, stair=None):
        self._updating = True
        obj.Proxy = self
        self.set_properties(obj)
        if stair:
            obj.StairName = stair.Name
        self._updating = False

    def set_properties(self, obj):
        _add_property(
            obj,
            "App::PropertyString",
            "StairName",
            "Flight",
            "The internal name of the owning stair",
            editor_mode=2,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "FlightType",
            "Flight",
            "The flight geometry type",
        )
        flight_type = "Straight" if added else str(obj.FlightType)
        flight_types = [
            "Straight",
            "Circular",
            "Straight landing",
            "Circular landing",
        ]
        obj.FlightType = flight_types
        obj.FlightType = (
            flight_type if flight_type in flight_types else "Straight"
        )
        for side in ("Left", "Right"):
            property_name = f"{side}StringerType"
            added = _add_property(
                obj,
                "App::PropertyEnumeration",
                property_name,
                "Stringers",
                "The stringer type on this side of this flight",
            )
            current_type = (
                "None" if added else str(getattr(obj, property_name))
            )
            if current_type == "Cut stringer":
                current_type = "Notched stringer"
            stringer_types = [
                "None",
                "Housed stringer",
                "Notched stringer",
            ]
            setattr(obj, property_name, stringer_types)
            setattr(
                obj,
                property_name,
                current_type
                if current_type in stringer_types
                else "None",
            )
            _add_property(
                obj,
                "App::PropertyBool",
                f"{side}HandrailEnabled",
                "Handrails",
                f"Create a handrail on the {side.lower()} side of this flight",
                False,
            )
        _add_property(
            obj, "App::PropertyLength", "LeftLength", "Flight", "Left-side length", 3500.0
        )
        _add_property(
            obj, "App::PropertyLength", "RightLength", "Flight", "Right-side length", 3500.0
        )
        _add_property(
            obj, "App::PropertyLength", "Width", "Flight", "Flight width", 1000.0
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "InnerRadius",
            "Flight",
            "Inner radius of a circular flight",
            500.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "OuterRadius",
            "Flight",
            "Outer radius of a circular flight",
            1500.0,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "Angle",
            "Flight",
            "Turn angle or circular-flight sweep angle",
            0.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "Rotation",
            "Flight",
            "Direction of the turn into this flight",
        )
        if added:
            obj.Rotation = ["Left", "Right"]
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "TurnType",
            "Flight",
            "How the turn into this flight is resolved",
        )
        if added:
            obj.TurnType = ["Herse balancing", "Landing"]
        _add_property(
            obj,
            "App::PropertyPercent",
            "WindingLocal",
            "Winding",
            "Winding adjustment for steps nearest the inner corner",
            50,
        )
        _add_property(
            obj,
            "App::PropertyPercent",
            "WindingDistant",
            "Winding",
            "Winding adjustment for steps farther from the inner corner",
            50,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "StartAngle",
            "Flight",
            "Angle of the stair footprint's start edge",
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "EndAngle",
            "Flight",
            "Angle of the stair footprint's end edge",
            0.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "EntryDirection",
            "Flight",
            "Direction from which the user enters the stair",
        )
        if added:
            obj.EntryDirection = ["Straight", "From left", "From right"]
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "ExitDirection",
            "Flight",
            "Direction in which the user leaves the stair",
        )
        if added:
            obj.ExitDirection = ["Straight", "To left", "To right"]
        _add_property(
            obj,
            "App::PropertyInteger",
            "NumberOfTreads",
            "Flight check",
            "The number of manufactured treads assigned to this flight",
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "TreadWidth",
            "Flight check",
            "The computed going in this flight",
            editor_mode=1,
        )
        self._update_dimension_visibility(obj)

    @staticmethod
    def _update_dimension_visibility(obj):
        circular = _is_circular_flight(obj)
        for name in ("LeftLength", "RightLength"):
            obj.setEditorMode(name, 2 if circular else 0)
        for name in ("InnerRadius", "OuterRadius"):
            obj.setEditorMode(name, 0 if circular else 2)

    def onChanged(self, obj, prop):
        if (
            getattr(self, "_updating", False)
            or FreeCAD.isRestoring()
            or obj.Document.Transacting
        ):
            return
        geometry_properties = {
            "FlightType",
            "LeftLength",
            "RightLength",
            "Width",
            "InnerRadius",
            "OuterRadius",
            "Angle",
            "Rotation",
            "TurnType",
            "WindingLocal",
            "WindingDistant",
            "StartAngle",
            "EndAngle",
            "EntryDirection",
            "ExitDirection",
        }
        stringer_properties = {
            "LeftStringerType",
            "RightStringerType",
        }
        handrail_properties = {
            "LeftHandrailEnabled",
            "RightHandrailEnabled",
        }
        if (
            prop in geometry_properties
            or prop in stringer_properties
            or prop in handrail_properties
        ):
            stair = obj.Document.getObject(obj.StairName)
            if stair and getattr(stair, "Proxy", None):
                flights = get_flights(stair)
                if prop in geometry_properties and prop == "FlightType":
                    self._update_dimension_visibility(obj)
                    if _is_circular_flight(obj) or _is_landing_flight(obj):
                        self._updating = True
                        try:
                            obj.Angle = 90.0
                            if _is_circular_flight(obj):
                                obj.StartAngle = 0.0
                                obj.EndAngle = 0.0
                                obj.EntryDirection = "Straight"
                                obj.ExitDirection = "Straight"
                        finally:
                            self._updating = False
                if prop in geometry_properties and prop in {
                    "InnerRadius",
                    "OuterRadius",
                }:
                    sync_circular_radii(obj, prop)
                elif (
                    prop in geometry_properties
                    and _is_circular_flight(obj)
                    and prop in {"FlightType", "Width"}
                ):
                    sync_circular_radii(obj)
                elif prop in geometry_properties and prop in {
                    "LeftLength",
                    "RightLength",
                }:
                    sync_flight_side_lengths(stair, obj, prop)
                elif prop in geometry_properties and prop in {
                    "StartAngle",
                    "EndAngle",
                }:
                    sync_flight_side_lengths(stair, obj, "LeftLength")
                elif prop in geometry_properties and prop in {
                    "FlightType",
                    "Width",
                    "Angle",
                    "Rotation",
                }:
                    sync_flight_side_lengths(stair, obj)
                if prop in geometry_properties and prop in {
                    "FlightType",
                    "Width",
                    "Angle",
                    "Rotation",
                } and obj in flights:
                    index = flights.index(obj)
                    if index:
                        sync_flight_side_lengths(stair, flights[index - 1])
                    if index + 1 < len(flights):
                        sync_flight_side_lengths(stair, flights[index + 1])
                stair.Proxy.rebuild(stair, allow_structure_changes=True)

    def onDocumentRestored(self, obj):
        self._updating = True
        self.set_properties(obj)
        self._updating = False

    def dumps(self):
        return None

    def loads(self, state):
        self._updating = False


class ComponentGroupProxy:
    """Marks a generated component group and links it to its stair."""

    Type = "StairDesignerComponentGroup"

    def __init__(self, obj, stair=None, section="stairs"):
        obj.Proxy = self
        self.Section = section
        _add_property(
            obj,
            "App::PropertyString",
            "StairName",
            "Stair Designer",
            "The internal name of the owning stair",
            stair.Name if stair else "",
            editor_mode=2,
        )
        _add_property(
            obj,
            "App::PropertyString",
            "PanelSection",
            "Stair Designer",
            "The task panel opened for this group",
            section,
            editor_mode=2,
        )

    def onDocumentRestored(self, obj):
        self.Section = getattr(obj, "PanelSection", "stairs")

    def dumps(self):
        return self.Section

    def loads(self, state):
        self.Section = state or "stairs"


class ViewProviderStair:
    """View provider for the Stair Designer root object."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.Object = vobj.Object

    def getIcon(self):
        import Arch_rc

        return ":/icons/Arch_Stairs_Tree.svg"

    def getTransactionText(self):
        return QT_TRANSLATE_NOOP("Command", "Edit Stair Designer")

    def doubleClicked(self, vobj):
        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.setEdit(vobj.Object.Name, 0)
        return True

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui
        from stairdesigner.taskpanels import StairDesignerTaskPanel

        if not FreeCADGui.Control.activeDialog():
            FreeCADGui.Control.showDialog(StairDesignerTaskPanel(vobj.Object))
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None


class ViewProviderComponentGroup:
    """Opens only the task panel section owned by a component group."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.Object = vobj.Object

    def getIcon(self):
        import Arch_rc

        return {
            "handrails": ":/icons/Arch_Handrail_Tree.svg",
            "stringers": ":/icons/Arch_Stringer_Tree.svg",
        }.get(
            getattr(self.Object, "PanelSection", "stairs"),
            ":/icons/Arch_Stairs_Tree.svg",
        )

    def getTransactionText(self):
        return QT_TRANSLATE_NOOP("Command", "Edit Stair Designer component")

    def doubleClicked(self, vobj):
        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.setEdit(vobj.Object.Name, 0)
        return True

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui
        from stairdesigner.taskpanels import StairDesignerTaskPanel

        stair = vobj.Object.Document.getObject(vobj.Object.StairName)
        if stair and not FreeCADGui.Control.activeDialog():
            section = getattr(vobj.Object, "PanelSection", "stairs")
            FreeCADGui.Control.showDialog(
                StairDesignerTaskPanel(stair, sections=(section,), edit_object=vobj.Object)
            )
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None


def _make_component_group(stair, property_name, label, section):
    group = stair.Document.addObject("App::DocumentObjectGroupPython", f"{stair.Name}_{label}")
    group.Label = translate("BIM", label)
    ComponentGroupProxy(group, stair, section)
    if FreeCAD.GuiUp:
        ViewProviderComponentGroup(group.ViewObject)
    stair.addObject(group)
    setattr(stair, property_name, group)
    return group


def _make_flight(
    stair,
    length,
    width,
    angle=0.0,
    rotation="Left",
    turn_type="Herse balancing",
):
    flight = stair.Document.addObject("App::FeaturePython", f"{stair.Name}_Flight")
    FlightProxy(flight, stair)
    flight.Proxy._updating = True
    flight.StairName = stair.Name
    flight.LeftLength = length
    flight.RightLength = length
    flight.Width = width
    flight.OuterRadius = _quantity_value(flight.InnerRadius) + width
    flight.Angle = angle
    flight.Rotation = rotation
    flight.TurnType = turn_type
    stair.FlightsGroup.addObject(flight)
    flight.Label = f"{translate('BIM', 'Flight')} {len(get_flights(stair))}"
    flight.Proxy._updating = False
    return flight


def resize_flights(stair, count, length=None, width=None, rotations=None):
    """Resize the ordered flight collection while preserving existing values."""

    count = max(int(count), 1)
    flights = get_flights(stair)
    template = flights[-1] if flights else None
    default_length = length if length is not None else (
        _flight_length(template) if template else 3500.0
    )
    default_width = width if width is not None else (
        _quantity_value(template.Width) if template else 1000.0
    )
    while len(flights) < count:
        index = len(flights)
        rotation = rotations[index - 1] if rotations and index - 1 < len(rotations) else "Left"
        flights.append(
            _make_flight(
                stair,
                default_length,
                default_width,
                angle=90.0,
                rotation=rotation,
            )
        )
    while len(flights) > count:
        flight = flights.pop()
        stair.FlightsGroup.removeObject(flight)
        stair.Document.removeObject(flight.Name)
    for index, flight in enumerate(flights):
        flight.Label = f"{translate('BIM', 'Flight')} {index + 1}"
    sync_all_flight_side_lengths(stair)
    return flights


def _set_generated_properties(obj, stair, role):
    _add_property(
        obj,
        "App::PropertyString",
        "GeneratedBy",
        "Stair Designer",
        "Name of the owning Stair Designer object",
        stair.Name,
        editor_mode=2,
    )
    _add_property(
        obj,
        "App::PropertyString",
        "StairDesignerRole",
        "Stair Designer",
        "Generated component role",
        role,
        editor_mode=2,
    )
    _add_property(
        obj,
        "App::PropertyInteger",
        "Index",
        "Stair Designer",
        "Sequential component index",
        0,
        editor_mode=1,
    )
    _add_property(
        obj,
        "App::PropertyInteger",
        "FlightIndex",
        "Stair Designer",
        "One-based index of the owning flight",
        1,
        editor_mode=1,
    )
    if "Material" in obj.PropertiesList:
        obj.removeProperty("Material")


def _generated_parts(group, stair, role):
    parts = [
        child
        for child in group.Group
        if getattr(child, "GeneratedBy", "") == stair.Name
        and getattr(child, "StairDesignerRole", "") == role
    ]
    for child in parts:
        if "Material" in child.PropertiesList:
            child.removeProperty("Material")
    return sorted(parts, key=lambda child: getattr(child, "Index", 0))


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
                    if int(getattr(candidate, "FlightIndex", 0))
                    == flight_index + 1
                ),
                None,
            )
        if part is None and allow_structure_changes:
            part = stair.Document.addObject(
                "Part::Feature", f"{stair.Name}_{role}"
            )
            _set_generated_properties(part, stair, role)
            group.addObject(part)
        if part is None:
            continue
        if part in unused:
            unused.remove(part)
        _add_property(
            part,
            "App::PropertyLink",
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
        defaults["NosingOffsetDirection"]
        if added
        else str(part.NosingOffsetDirection)
    )
    directions = ["Perpendicular", "Vertical"]
    part.NosingOffsetDirection = directions
    if not part.OverrideNosingPosition:
        current_direction = defaults["NosingOffsetDirection"]
    part.NosingOffsetDirection = (
        current_direction
        if current_direction in directions
        else "Perpendicular"
    )
    part.setEditorMode(
        "NosingOffsetDirection",
        0 if part.OverrideNosingPosition else 1,
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
        _add_property(
            part,
            "App::PropertyLink",
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
        part.Placement = stair.Placement
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


def _resize_generated_parts(group, stair, role, count):
    parts = _generated_parts(group, stair, role)
    while len(parts) < count:
        part = stair.Document.addObject("Part::Feature", f"{stair.Name}_{role}")
        _set_generated_properties(part, stair, role)
        group.addObject(part)
        parts.append(part)
    while len(parts) > count:
        part = parts.pop()
        group.removeObject(part)
        stair.Document.removeObject(part.Name)
    return parts


def make_stair(
    floor_height=2800.0,
    flight_length=3500.0,
    width=1000.0,
    steps=15,
    stair_type="Wood",
    name=None,
):
    """Create a new straight Stair Designer object and its generated children."""

    doc = FreeCAD.ActiveDocument
    if not doc:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return None

    stair = doc.addObject("App::DocumentObjectGroupPython", "Stair")
    stair.Label = name or translate("BIM", "Stair")
    StairProxy(stair)
    if FreeCAD.GuiUp:
        ViewProviderStair(stair.ViewObject)

    stair.Proxy._updating = True
    stair.FloorHeight = floor_height
    stair.NumberOfSteps = steps
    stair.StairType = stair_type

    sketch = doc.addObject("Sketcher::SketchObject", f"{stair.Name}_Plan")
    sketch.Label = translate("BIM", "Stair plan")
    stair.addObject(sketch)
    stair.PlanSketch = sketch
    if FreeCAD.GuiUp:
        sketch.ViewObject.Visibility = False

    flights = _make_component_group(stair, "FlightsGroup", "Flights", "stairs")
    _make_flight(stair, flight_length, width)
    initial_metrics = geometry.straight_stair_metrics(
        floor_height,
        flight_length,
        steps,
    )
    stair.ConcreteThickness = geometry.default_concrete_thickness(initial_metrics)

    stair.Proxy._updating = False
    stair.Proxy.rebuild(stair, allow_structure_changes=True)
    doc.recompute()
    return stair
