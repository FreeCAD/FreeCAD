# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stair and flight object factories."""

import FreeCAD

from .geometry_core import straight_stair_metrics
from .geometry_straight import default_concrete_thickness


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _make_component_group,
)

from .object_proxies import (
    FlightProxy,
    ViewProviderStair,
)

from .object_stair import (
    StairProxy,
)

from .object_utils import (
    _flight_length,
    _quantity_value,
    get_flights,
    sync_all_flight_side_lengths,
)

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

    stair = doc.addObject("Part::FeaturePython", "Stair")
    stair.addExtension("App::GeoFeatureGroupExtensionPython")
    stair.setEditorMode("Shape", 2)
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

    _make_component_group(stair, "FlightsGroup", "Flights", "stairs")
    _make_flight(stair, flight_length, width)
    initial_metrics = straight_stair_metrics(
        floor_height,
        flight_length,
        steps,
    )
    stair.ConcreteThickness = default_concrete_thickness(initial_metrics)

    stair.Proxy._updating = False
    stair.Proxy.rebuild(stair, allow_structure_changes=True)
    doc.recompute()
    return stair
