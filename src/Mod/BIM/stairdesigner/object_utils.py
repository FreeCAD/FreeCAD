# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared Stair Designer object and flight utilities."""

import math

import FreeCAD

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


def _add_link_property(
    obj,
    type_id,
    name,
    group,
    description,
    editor_mode=None,
):
    """Add a link with explicit scope, migrating an older link if needed."""

    previous = None
    if name in obj.PropertiesList and obj.getTypeIdOfProperty(name) != type_id:
        previous = getattr(obj, name)
        obj.setPropertyStatus(name, "-LockDynamic")
        obj.removeProperty(name)
    added = _add_property(
        obj,
        type_id,
        name,
        group,
        description,
        editor_mode=editor_mode,
    )
    if previous is not None:
        setattr(obj, name, previous)
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
        obj for obj in group.Group if getattr(getattr(obj, "Proxy", None), "Type", "") == "Flight"
    ]


def first_step_rounded(stair, side):
    """Tell whether a wooden stair's first step is rounded on ``side``.

    A rounded end stands free in front of the stair: that side's board and
    rail start at the second riser. Only a straight flight entered straight
    has such an end: a stair entered from the side has its first step
    against a board there.
    """

    flights = get_flights(stair)
    return (
        bool(flights)
        and str(stair.StairType) != "Concrete"
        and str(flights[0].FlightType).startswith("Straight")
        and str(flights[0].EntryDirection) == "Straight"
        and str(getattr(stair, "RoundedFirstStep", "None")) in (side, "Both sides")
    )


def _side_insets(stair, flights):
    """Return each flight's ``(left, right)`` room beyond its steps' ends.

    A flight's width is the stair's whole width, as the opening it stands
    in: its boards and handrail posts stand within it, and its steps end
    short of it by the room they take on each side. A housed board stands
    out its thickness less the steps' housing in it, a notched one runs
    under the steps' ends; a post stands out half
    its size from the board's centre line, or from the steps' ends without
    a board, less the handrail's offset in. Only a wooden stair of straight
    flights has boards and posts beside its steps.
    """

    if str(stair.StairType) != "Wood" or any(
        _is_circular_flight(flight) or _is_landing_flight(flight) for flight in flights
    ):
        return [(0.0, 0.0)] * len(flights)
    stringers = list(getattr(getattr(stair, "StringersGroup", None), "Group", []) or [])
    post = (
        max(
            _quantity_value(stair.HandrailPostWidth),
            _quantity_value(stair.HandrailPostThickness),
        )
        / 2.0
    )
    insets = []
    for flight in flights:
        sides = []
        for side in ("Left", "Right"):
            stringer_type = str(getattr(flight, f"{side}StringerType", "None"))
            thickness = _quantity_value(stair.StringerThickness)
            overlap = _quantity_value(stair.StringerStepOverlap)
            part = next(
                (
                    part
                    for part in stringers
                    if getattr(part, "SourceFlight", None) == flight
                    and str(getattr(part, "Side", "")) == side
                ),
                None,
            )
            if part is not None:
                if getattr(part, "OverrideThickness", False):
                    thickness = _quantity_value(part.Thickness)
                if getattr(part, "OverrideStepOverlap", False):
                    overlap = _quantity_value(part.StepOverlap)
            # The board's centre line, out from the steps' ends.
            if stringer_type == "Housed stringer":
                center = thickness / 2.0 - overlap
            elif stringer_type in ("Notched stringer", "Cut stringer"):
                # Under the steps' ends.
                center = -(overlap + thickness / 2.0)
            else:
                center = None
            room = 0.0 if center is None else center + thickness / 2.0
            if bool(getattr(flight, f"{side}HandrailEnabled", False)):
                room = max(
                    room,
                    (center or 0.0) - _quantity_value(stair.HandrailOffset) + post,
                )
            sides.append(max(room, 0.0))
        insets.append(tuple(sides))
    return insets


def _turned_opening(stair, flight, at_start):
    """Return how far beyond its boards' plan a turned entry or exit opens.

    The steps there reach the floor at the stair's outline, past the room
    the turned side's board and posts take (see ``_side_insets``). 0 for a
    straight entry or exit.
    """

    flights = get_flights(stair)
    if flight not in flights:
        return 0.0
    direction = str(flight.EntryDirection if at_start else flight.ExitDirection).lower()
    if direction == "straight":
        return 0.0
    # As the stair's plan last took them.
    insets = getattr(getattr(stair, "Proxy", None), "plan_insets", None)
    if insets is None or len(insets) != len(flights):
        insets = _side_insets(stair, flights)
    return insets[flights.index(flight)][0 if "left" in direction else 1]


def _is_circular_flight(flight):
    return str(flight.FlightType).startswith("Circular")


def _is_landing_flight(flight):
    return str(flight.FlightType).endswith("landing")


def _has_endpoint_angle(flight):
    return (
        abs(_quantity_value(flight.StartAngle)) > 1e-7
        or abs(_quantity_value(flight.EndAngle)) > 1e-7
    )


def _flight_length(flight):
    if _is_circular_flight(flight) and all(
        name in flight.PropertiesList for name in ("InnerRadius", "OuterRadius")
    ):
        center_radius = (
            _quantity_value(flight.InnerRadius) + _quantity_value(flight.OuterRadius)
        ) / 2.0
        sweep = math.radians(min(abs(_quantity_value(flight.Angle)), 359.999))
        return max(center_radius * sweep, 0.01)
    return (_quantity_value(flight.LeftLength) + _quantity_value(flight.RightLength)) / 2.0


def _flight_path_dimension(flight):
    if _is_circular_flight(flight):
        return _quantity_value(flight.InnerRadius)
    return _flight_length(flight)


def linked_flight_side_lengths(
    left_length,
    right_length,
    next_width=0.0,
    next_rotation=None,
    driver=None,
):
    """Return linked rail lengths while preserving the requested input."""

    difference = max(float(next_width), 0.0) if next_rotation else 0.0
    signed_difference = -difference if str(next_rotation) == "Right" else difference
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
    return (max(float(next_width), 0.0) - max(float(current_width), 0.0) * math.cos(radians)) / sine


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
    all_straight = all(str(item.FlightType) == "Straight" for item in flights)
    if all_straight and index == 0:
        difference += _quantity_value(flight.Width) * math.tan(
            math.radians(min(max(_quantity_value(flight.StartAngle), -89.0), 89.0))
        )
    if all_straight and index == len(flights) - 1:
        difference -= _quantity_value(flight.Width) * math.tan(
            math.radians(min(max(_quantity_value(flight.EndAngle), -89.0), 89.0))
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
    if flight not in flights:
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


def _uses_native_container_placement(stair):
    """Return whether FreeCAD applies the Stair placement to its children."""

    return stair.hasExtension("App::GeoFeatureGroupExtensionPython")


def _child_placement(stair):
    """Return the placement for geometry already expressed in stair space."""

    return FreeCAD.Placement() if _uses_native_container_placement(stair) else stair.Placement


def _set_global_shape(part, shape):
    """Assign a shape built in document coordinates, keeping the placement.

    The part's placement moves whatever shape it is given, so the shape is
    first taken back to the part's own coordinates, geometry included: a
    shape that only carried the inverse placement would lose it again as
    the part's placement is restored.
    """

    placement = part.Placement
    part.Shape = (
        shape.copy()
        if placement.isIdentity()
        else shape.transformed(placement.inverse().toMatrix(), True)
    )
    part.Placement = placement


def _combined_placement(stair, local_placement):
    if _uses_native_container_placement(stair):
        return local_placement
    return stair.Placement.multiply(local_placement)
