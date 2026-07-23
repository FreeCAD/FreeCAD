# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stair property schema and rebuild orchestration."""

import math

import FreeCAD

from .geometry_core import (
    BLONDEL_MAXIMUM,
    BLONDEL_MINIMUM,
    assign_section_elevations,
    distribute_treads,
    flight_stair_metrics,
    riser_stations,
    tread_goings,
)
from .geometry_plan import (
    balanced_partition_is_valid,
    balanced_tread_faces,
    fit_balanced_sections_to_footprint,
    fit_tangent_sections_to_footprint,
    make_stair_footprint,
    make_tangent_stair_footprint,
    tangent_tread_faces,
)
from .geometry_stringer_path import automatic_stringer_width
from .geometry_tangent import tangent_flight_sections
from .geometry_winders import balanced_winder_sections


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _tread_extra_heights,
    _tread_extra_widths,
)

from .object_utils import (
    _add_link_property,
    _add_property,
    _flight_length,
    _flight_path_dimension,
    _is_circular_flight,
    _is_landing_flight,
    _quantity_value,
    _uses_native_container_placement,
    get_flights,
)

class StairBaseMixin:
    """Implementation methods grouped by responsibility."""

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
            "App::PropertyBool",
            "EndWithRiser",
            "Stair",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Ends the last tread one rise below the upper floor and creates "
                "a final riser to floor height",
            ),
            True,
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
            "BottomCutDistance",
            "Concrete",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Vertical distance below the stair base plane at which the "
                "concrete underside is cut",
            ),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "TopCutDistance",
            "Concrete",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Horizontal distance beyond the stair end at which the "
                "concrete is cut by a vertical plane",
            ),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StructureWidthOffset",
            "Steps",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Insets the concrete structure equally from both sides "
                "without changing the finish tread width",
            ),
            0.0,
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
            "App::PropertyBool",
            "StepsEnabled",
            "Steps",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Adds separate finish treads and optional risers to a "
                "concrete stair",
            ),
            False,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StepThickness",
            "Steps",
            QT_TRANSLATE_NOOP("App::Property", "The thickness of the treads"),
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
            QT_TRANSLATE_NOOP("App::Property", "Creates individual risers"),
            True,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "RiserThickness",
            "Risers",
            QT_TRANSLATE_NOOP("App::Property", "The thickness of the risers"),
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
            _add_link_property(
                obj,
                "App::PropertyLinkChild",
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
        if prop == "Placement" and _uses_native_container_placement(obj):
            return
        if prop == "StairType" and str(obj.StairType) == "Concrete":
            self._updating = True
            try:
                obj.StepsEnabled = False
                obj.StepRiserOverlap = 0.0
            finally:
                self._updating = False
        if prop in {
            "StairType",
            "FloorHeight",
            "EndWithRiser",
            "NumberOfSteps",
            "Placement",
            "ConcreteThickness",
            "BottomCutDistance",
            "TopCutDistance",
            "StructureWidthOffset",
            "StepsEnabled",
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
            total_risers = max(int(obj.NumberOfSteps), 2)
            total_treads = max(
                total_risers - (1 if obj.EndWithRiser else 0),
                1,
            )
            extra_widths = (
                _tread_extra_widths(obj, total_treads)
                if str(obj.StairType) == "Wood"
                else [0.0] * total_treads
            )
            extra_heights = (
                _tread_extra_heights(obj, total_treads)
                if str(obj.StairType) == "Wood"
                else [0.0] * total_treads
            )
            riser_height, height_stations = riser_stations(
                floor_height,
                total_risers,
                extra_heights,
            )
            effective_riser_heights = [
                rear - front
                for front, rear in zip(
                    height_stations, height_stations[1:]
                )
            ]
            flight_lengths = [_flight_length(flight) for flight in flights]
            stair_indices = [
                index
                for index, flight in enumerate(flights)
                if not _is_landing_flight(flight)
            ]
            stair_tread_counts = distribute_treads(
                [flight_lengths[index] for index in stair_indices],
                total_treads,
            )
            tread_counts = [0] * len(flights)
            for index, count in zip(stair_indices, stair_tread_counts):
                tread_counts[index] = count
            layouts = self._flight_layouts(
                flights,
                tread_counts,
                riser_height,
                extra_widths,
                effective_riser_heights,
            )
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
                    tangent_flight_sections(
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
                        extra_widths=extra_widths,
                    )
                )
                balanced_footprint = make_tangent_stair_footprint(
                    flight_specs, turn_types, start_angle, end_angle
                )
                balanced_sections = (
                    fit_tangent_sections_to_footprint(
                        balanced_sections, balanced_footprint
                    )
                )
                if has_landing_flight:
                    partition_faces = tangent_tread_faces(
                        balanced_sections, flight_specs
                    )
                    winding_geometry_valid = (
                        len(partition_faces) == len(balanced_sections) - 1
                        and all(face.isValid() for face in partition_faces)
                    )
                else:
                    partition_faces = balanced_tread_faces(balanced_sections, balanced_footprint)
                    winding_geometry_valid = balanced_partition_is_valid(
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
                balanced_sections, average_going = balanced_winder_sections(
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
                    nosing=(
                        _quantity_value(obj.Nosing)
                        if obj.StairType == "Wood"
                        else 0.0
                    ),
                    extra_widths=extra_widths,
                )
                balanced_footprint = make_stair_footprint(flight_specs, start_angle, end_angle)
                balanced_sections = fit_balanced_sections_to_footprint(
                    balanced_sections, balanced_footprint
                )
                partition_faces = balanced_tread_faces(balanced_sections, balanced_footprint)
                winding_geometry_valid = balanced_partition_is_valid(
                    partition_faces,
                    balanced_footprint,
                    len(balanced_sections) - 1,
                )
                balanced_plan_shapes = partition_faces
            if balanced_sections:
                balanced_sections = assign_section_elevations(
                    balanced_sections,
                    height_stations,
                )
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
                average_going, _goings = tread_goings(
                    sum(flight_lengths[index] for index in stair_indices),
                    total_treads,
                    extra_widths,
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
                BLONDEL_MINIMUM
                <= blondel_value
                <= BLONDEL_MAXIMUM
            )
            obj.setEditorMode(
                "StringerWidth", 0 if obj.StringerCustomWidth else 1
            )
            if not obj.StringerCustomWidth:
                obj.StringerWidth = automatic_stringer_width(
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
                obj.setEditorMode("StepsEnabled", 0)
                obj.setEditorMode("StepRiserOverlap", 2)
                obj.setEditorMode(
                    "StructureWidthOffset",
                    0 if obj.StepsEnabled else 1,
                )
                obj.StepRiserOverlap = 0.0
                if allow_structure_changes:
                    self._remove_stringers_group(obj)
                self._update_concrete(
                    obj,
                    layouts,
                    balanced_sections,
                    balanced_footprint,
                    balanced_plan_shapes,
                    allow_structure_changes,
                )
                if obj.StepsEnabled:
                    self._update_wood(
                        obj,
                        layouts,
                        balanced_sections,
                        balanced_footprint,
                        balanced_plan_shapes,
                        allow_structure_changes,
                        concrete_dressing=True,
                    )
                elif allow_structure_changes:
                    self._remove_steps_group(obj)
            else:
                obj.setEditorMode("StepsEnabled", 2)
                obj.setEditorMode("StepRiserOverlap", 0)
                obj.setEditorMode("StructureWidthOffset", 2)
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
                balanced_footprint,
                balanced_plan_shapes,
                riser_height,
                allow_structure_changes,
            )
        finally:
            self._updating = False

    def _flight_layouts(
        self,
        flights,
        tread_counts,
        riser_height,
        extra_widths=None,
        riser_heights=None,
    ):
        layouts = []
        extras = list(extra_widths or [])
        extra_cursor = 0
        heights = list(riser_heights or [])
        required_heights = sum(tread_counts) + 1
        if len(heights) < required_heights:
            heights.extend(
                [riser_height] * (required_heights - len(heights))
            )
        height_cursor = 0
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
            flight_extras = extras[
                extra_cursor : extra_cursor + tread_count
            ]
            extra_cursor += tread_count
            flight_heights = heights[
                height_cursor : height_cursor + tread_count
            ]
            section_heights = heights[
                height_cursor : height_cursor + tread_count + 1
            ]
            section_top_elevations = []
            elevation = 0.0
            for height in section_heights:
                elevation += height
                section_top_elevations.append(elevation)
            metrics = flight_stair_metrics(
                length,
                tread_count,
                riser_height,
                flight_extras,
            )
            _general_going, goings = tread_goings(
                length,
                tread_count,
                flight_extras,
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
                    "tread_goings": goings,
                    "riser_heights": flight_heights,
                    "section_riser_heights": section_heights,
                    "section_top_elevations": section_top_elevations,
                }
            )
            center += direction * length
            if not _is_landing_flight(flight):
                base_z += sum(flight_heights)
                height_cursor += tread_count
        return layouts

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
