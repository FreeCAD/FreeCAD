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
    if (
        name in obj.PropertiesList
        and obj.getTypeIdOfProperty(name) != type_id
    ):
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
        obj
        for obj in group.Group
        if getattr(getattr(obj, "Proxy", None), "Type", "") == "Flight"
    ]


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

    return (
        FreeCAD.Placement()
        if _uses_native_container_placement(stair)
        else stair.Placement
    )


def _combined_placement(stair, local_placement):
    if _uses_native_container_placement(stair):
        return local_placement
    return stair.Placement.multiply(local_placement)
