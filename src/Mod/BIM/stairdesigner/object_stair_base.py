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
    corner_post_sections,
    even_walking_line_goings,
    follow_moved_ends,
    fit_balanced_sections_to_footprint,
    inner_corner_newel_sections,
    inset_flight_specs,
    fit_tangent_sections_to_footprint,
    make_stair_footprint,
    make_tangent_stair_footprint,
    newel_corners,
    newel_fan_plan,
    newel_regions,
    newel_post_square,
    open_turned_endpoints,
    shift_section_ends,
    tangent_tread_faces,
    translated_plan,
    walking_line_treads,
)
from .geometry_profiles import MEMBER_SHAPES, POST_CAPS, RAIL_PROFILES
from .geometry_stringer_path import automatic_stringer_width
from .geometry_tangent import tangent_flight_sections
from .geometry_winders import balanced_winder_sections

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _tread_extra_heights,
    _tread_side_offsets,
    _tread_extra_widths,
)

from .object_utils import (
    _add_link_property,
    _add_property,
    _flight_length,
    _side_insets,
    _flight_path_dimension,
    _global_component_shape,
    _is_circular_flight,
    _is_landing_flight,
    _quantity_value,
    _uses_native_container_placement,
    get_flights,
)


def _newel_post_squares(stair, flights, sections, footprint, entry_direction, exit_direction):
    """Return ``(square, post)`` plan outlines where steps meet newel posts.

    A newel stands at each inner corner between flights and at a turned
    entry or exit, where a handrail is enabled and the boards are housed on
    that side: the steps build before the posts, so their squares are set
    here, centred where the boards' centre lines meet as the posts are. The
    steps run into a post by the housing depth, as into a board, so each
    square is the post's less that depth all round. Other boards place
    their posts otherwise; their steps are left alone.
    """

    post_size = max(
        _quantity_value(stair.HandrailPostWidth),
        _quantity_value(stair.HandrailPostThickness),
    )
    size = post_size - 2.0 * _quantity_value(stair.StringerStepOverlap)
    if size <= 1e-7 or len(sections) < 3 or footprint is None or footprint.isNull():
        return []
    face = max(footprint.Faces, key=lambda item: item.Area)
    elevation = face.BoundBox.ZMin
    # The boards' centre lines lie this far outside the stair's sides.
    offset = _quantity_value(stair.StringerThickness) / 2.0 - _quantity_value(
        stair.StringerStepOverlap
    )

    def carries(flight_indices, side):
        return any(
            bool(getattr(flights[index], f"{side}HandrailEnabled", False))
            and str(getattr(flights[index], f"{side}StringerType", "")) == "Housed stringer"
            for index in flight_indices
            if 0 <= index < len(flights)
        )

    def outward(point, along):
        normal = (-along[1], along[0])
        probe = FreeCAD.Vector(point[0] + normal[0], point[1] + normal[1], elevation)
        if face.isInside(probe, 1e-6, True):
            normal = (-normal[0], -normal[1])
        return normal

    def unit(vector):
        length = math.hypot(*vector)
        return (vector[0] / length, vector[1] / length) if length > 1e-9 else None

    squares = []
    for at_start, direction in ((True, entry_direction), (False, exit_direction)):
        direction = str(direction).lower()
        if direction == "straight":
            continue
        side = "Left" if "left" in direction else "Right"
        endpoint = sections[0] if at_start else sections[-1]
        if not carries([endpoint.flight_index], side):
            continue
        corner = getattr(endpoint, side.lower())
        along = unit((endpoint.left[0] - endpoint.right[0], endpoint.left[1] - endpoint.right[1]))
        if along is None:
            continue
        normal = outward(corner, along)
        center = (corner[0] + normal[0] * offset, corner[1] + normal[1] * offset)
        squares.append(
            (newel_post_square(center, along, size), newel_post_square(center, along, post_size))
        )

    vertices = [(vertex.Point.x, vertex.Point.y) for vertex in face.OuterWire.OrderedVertexes]
    signed_area = 0.5 * sum(
        first[0] * second[1] - second[0] * first[1]
        for first, second in zip(vertices, vertices[1:] + vertices[:1])
    )
    for index, corner in enumerate(vertices):
        previous = vertices[index - 1]
        following = vertices[(index + 1) % len(vertices)]
        turn = (corner[0] - previous[0]) * (following[1] - corner[1]) - (
            corner[1] - previous[1]
        ) * (following[0] - corner[0])
        if turn * signed_area >= -1e-7:
            continue
        touching = [
            (section, side)
            for section in sections[1:-1]
            for side in ("Left", "Right")
            if math.dist(getattr(section, side.lower()), corner) < post_size
        ]
        if not touching:
            continue
        side = touching[0][1]
        indices = {
            section.flight_index + step for section, _side in touching for step in (-1, 0, 1)
        }
        if not carries(indices, side):
            continue
        incoming = unit((corner[0] - previous[0], corner[1] - previous[1]))
        outgoing = unit((following[0] - corner[0], following[1] - corner[1]))
        if incoming is None or outgoing is None:
            continue
        shift_a = outward(corner, incoming)
        shift_b = outward(corner, outgoing)
        center = (
            corner[0] + (shift_a[0] + shift_b[0]) * offset,
            corner[1] + (shift_a[1] + shift_b[1]) * offset,
        )
        squares.append(
            (
                newel_post_square(center, outgoing, size),
                newel_post_square(center, outgoing, post_size),
            )
        )
    return squares


class StairBaseMixin:
    """Implementation methods grouped by responsibility."""

    def getIfcExportObjects(self, obj):
        """Export the balustrade separately from the stair's solid body."""
        group = obj.HandrailsGroup
        return [group] if group and group.Group else []

    def getIfcShape(self, obj):
        """Return the stair body in global coordinates for IFC export."""
        parts = []
        for group in (obj.StepsGroup, obj.StringersGroup):
            if group:
                parts.extend(group.Group)
        if obj.ConcreteGeometry:
            parts.append(obj.ConcreteGeometry)
        return _global_component_shape(parts)

    def set_properties(self, obj):
        for deprecated in (
            "Model",
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
        offset_added = _add_property(
            obj,
            "App::PropertyLength",
            "WalkingLineOffset",
            "Stair",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Distance of the walking line from the inner side of the steps; the walking "
                "line runs in the middle of steps narrower than twice this",
            ),
            500.0,
        )
        if (
            _add_property(
                obj,
                "App::PropertyBool",
                "WalkingLineFromInner",
                "Stair",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Walking line offset is measured from the inner side of the steps",
                ),
                True,
                editor_mode=2,
            )
            and not offset_added
            and abs(_quantity_value(obj.WalkingLineOffset) - 300.0) < 1e-6
        ):
            # The offset was measured from the outer side; its default
            # there, 300, put the walking line in the middle of most stairs.
            obj.WalkingLineOffset = 500.0
        for name, type_id, description in (
            (
                "WalkingLinePoints",
                "App::PropertyVectorList",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Points of the walking line, step by step, on the steps"
                ),
            ),
            (
                "WalkingLineCounts",
                "App::PropertyIntegerList",
                QT_TRANSLATE_NOOP("App::Property", "Number of walking line points on each step"),
            ),
            (
                "WalkingLineGoings",
                "App::PropertyFloatList",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Going of each step along the walking line, in mm"
                ),
            ),
        ):
            _add_property(obj, type_id, name, "Stair", description, editor_mode=2)
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
                "Adds separate finish treads and optional risers to a " "concrete stair",
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
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "RoundedFirstStep",
            "Steps",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The sides on which the first step of a wooden stair ends free and "
                "rounded: the stringer and the handrail start at the second riser there",
            ),
        )
        rounded = "None" if added else str(obj.RoundedFirstStep)
        rounded_sides = ["None", "Left", "Right", "Both sides"]
        obj.RoundedFirstStep = rounded_sides
        obj.RoundedFirstStep = rounded if rounded in rounded_sides else "None"
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
            QT_TRANSLATE_NOOP("App::Property", "Uses a manually specified stringer width"),
            False,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerWidth",
            "Stringers",
            QT_TRANSLATE_NOOP("App::Property", "Default stringer board width"),
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
                "App::Property",
                "Length beyond the first step of a stair entering straight, from its nosing for housed stringers",
            ),
            30.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerEndExtension",
            "Stringers",
            QT_TRANSLATE_NOOP("App::Property", "Length beyond the last step of the stair"),
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "StringerJoinery",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Cuts tread, riser and picket housings into the stringers and "
                "tenons the stringers into posts and into each other",
            ),
            True,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "StringerJointType",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "How stringers are joined to each other: by a mortise and tenon, "
                "or by a finger joint a CNC router cuts with the stringer's outline",
            ),
        )
        joint_type = "Mortise and tenon" if added else str(obj.StringerJointType)
        joint_types = ["Mortise and tenon", "Finger joint"]
        obj.StringerJointType = joint_types
        obj.StringerJointType = joint_type if joint_type in joint_types else "Mortise and tenon"
        if "BomPlankWidth" in obj.PropertiesList:
            # Replaced by the tread material's stock width.
            obj.removeProperty("BomPlankWidth")
        if "BomMaterialThicknesses" in obj.PropertiesList:
            # Materials take the thickness of the parts given to them.
            obj.removeProperty("BomMaterialThicknesses")
        if "BomStockLayout" in obj.PropertiesList:
            # The stock layout has a tab of its own in the task panel.
            obj.removeProperty("BomStockLayout")
        if "BomCutSpacing" in obj.PropertiesList:
            # Each material has its own cut spacing.
            obj.removeProperty("BomCutSpacing")
        for name, type_id, description, default in (
            (
                "BomMaterialNames",
                "App::PropertyStringList",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Names of the stock materials",
                ),
                None,
            ),
            (
                "BomMaterialLengths",
                "App::PropertyFloatList",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Length of each stock material's planks or panels, in mm",
                ),
                None,
            ),
            (
                "BomMaterialSpacings",
                "App::PropertyFloatList",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Spacing left between parts on each stock material: saw kerf "
                    "or cutter diameter, in mm",
                ),
                None,
            ),
            (
                "BomMaterialWidths",
                "App::PropertyFloatList",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Width of each stock material's planks or panels, in mm; treads "
                    "wider than their planks are glued up from strips",
                ),
                None,
            ),
            (
                "BomStringerMaterial",
                "App::PropertyString",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stock material the stringers are cut from",
                ),
                "",
            ),
            (
                "BomTreadMaterial",
                "App::PropertyString",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stock material the treads are cut from",
                ),
                "",
            ),
            (
                "BomRiserMaterial",
                "App::PropertyString",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stock material the risers are cut from",
                ),
                "",
            ),
            (
                "BomWedgeMaterial",
                "App::PropertyString",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stock material the housing wedges are cut from, in the room "
                    "the other parts leave on it",
                ),
                "",
            ),
        ):
            _add_property(
                obj,
                type_id,
                name,
                "Bill of materials",
                description,
                default,
            )
        _add_property(
            obj,
            "App::PropertyBool",
            "StringerOpenHousings",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Runs each tread housing of a housed stringer on to the back of the "
                "stringer, so that the steps slide in from behind the stair",
            ),
            False,
        )
        _add_property(
            obj,
            "App::PropertyBool",
            "StringerWedgedHousings",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Tapers the open housings for wedges driven under the treads and "
                "behind the risers",
            ),
            False,
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
        current_direction = "Perpendicular" if added else str(obj.StringerNosingOffsetDirection)
        directions = ["Perpendicular", "Vertical"]
        obj.StringerNosingOffsetDirection = directions
        obj.StringerNosingOffsetDirection = (
            current_direction if current_direction in directions else "Perpendicular"
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerNosingOffset",
            "Stringers",
            QT_TRANSLATE_NOOP("App::Property", "Default board position above the nosings"),
            50.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "StringerUndersideOffsetDirection",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "How the timber left under the step housings is measured",
            ),
        )
        current_direction = "Perpendicular" if added else str(obj.StringerUndersideOffsetDirection)
        obj.StringerUndersideOffsetDirection = directions
        obj.StringerUndersideOffsetDirection = (
            current_direction if current_direction in directions else "Perpendicular"
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "StringerUndersideOffset",
            "Stringers",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Default timber left under the step housings of an " "automatic-width housed board",
            ),
            50.0,
        )
        obj.setEditorMode("StringerUndersideOffsetDirection", 1 if obj.StringerCustomWidth else 0)
        obj.setEditorMode("StringerWidth", 0 if obj.StringerCustomWidth else 1)
        obj.setEditorMode("StringerUndersideOffset", 1 if obj.StringerCustomWidth else 0)

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
            QT_TRANSLATE_NOOP("App::Property", "Maximum clear spacing between pickets"),
            100.0,
        )
        self._set_shape_property(
            obj,
            "HandrailPicketShape",
            "Pickets",
            "Shape of the pickets",
            MEMBER_SHAPES,
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
            QT_TRANSLATE_NOOP("App::Property", "Picket penetration into the stringer"),
            20.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPicketTopRailPenetration",
            "Pickets",
            QT_TRANSLATE_NOOP("App::Property", "Picket penetration into the top rail"),
            10.0,
        )
        self._set_shape_property(
            obj,
            "HandrailPostShape",
            "Posts",
            "Shape of the posts",
            MEMBER_SHAPES,
        )
        self._set_shape_property(
            obj,
            "HandrailPostCap",
            "Posts",
            "Shape of the top end of the posts",
            POST_CAPS,
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
            QT_TRANSLATE_NOOP("App::Property", "Post length above the top rail"),
            70.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "HandrailPostBelowStringer",
            "Posts",
            QT_TRANSLATE_NOOP("App::Property", "Post length below a wooden stringer"),
            100.0,
        )
        self._set_shape_property(
            obj,
            "HandrailTopRailShape",
            "Top rail",
            "Cross-section shape of the top rail",
            RAIL_PROFILES,
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
            QT_TRANSLATE_NOOP("App::Property", "Top-rail penetration into end posts"),
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
    def _set_shape_property(obj, name, group, description, choices):
        """Add a shape choice, or give an older one its new ``choices``:
        the first is the default."""

        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            name,
            group,
            QT_TRANSLATE_NOOP("App::Property", description),
        )
        choices = list(choices)
        current = choices[0] if added else str(getattr(obj, name))
        setattr(obj, name, choices)
        setattr(obj, name, current if current in choices else choices[0])

    def onDocumentRestored(self, obj):
        self._updating = True
        self.Object = obj
        self.set_properties(obj)
        self._updating = False

    @staticmethod
    def _partitions(sections, footprint):
        try:
            faces = balanced_tread_faces(sections, footprint)
        except ValueError:
            return False
        return balanced_partition_is_valid(faces, footprint, len(sections) - 1)

    def _store_walking_line(self, obj, sections, regions, widths):
        """Keep each tread's stretch of the walking line and its going, at
        the tread's top, for the view to show."""

        points, counts, goings = [], [], []
        if sections:
            treads = walking_line_treads(
                sections, regions, widths, _quantity_value(obj.WalkingLineOffset)
            )
            for section, tread in zip(sections, treads):
                elevation = getattr(section, "top_elevation", None)
                if tread is None or elevation is None:
                    counts.append(0)
                    goings.append(0.0)
                    continue
                stretch, going = tread
                points.extend(FreeCAD.Vector(x, y, elevation) for x, y in stretch)
                counts.append(len(stretch))
                goings.append(going)
        updating = getattr(self, "_updating", False)
        self._updating = True
        try:
            obj.WalkingLinePoints = points
            obj.WalkingLineCounts = counts
            obj.WalkingLineGoings = goings
        finally:
            self._updating = updating
        return goings

    def onChanged(self, obj, prop):
        if getattr(self, "_updating", False) or FreeCAD.isRestoring() or obj.Document.Transacting:
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
            "WalkingLineOffset",
            "ConcreteThickness",
            "BottomCutDistance",
            "TopCutDistance",
            "StructureWidthOffset",
            "StepsEnabled",
            "StepThickness",
            "Nosing",
            "RoundedFirstStep",
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
            "StringerUndersideOffsetDirection",
            "StringerUndersideOffset",
            "StringerJoinery",
            "StringerJointType",
            "StringerOpenHousings",
            "StringerWedgedHousings",
        } or prop.startswith("Handrail"):
            if prop == "StringerCustomWidth":
                obj.setEditorMode("StringerWidth", 0 if obj.StringerCustomWidth else 1)
                for name in ("StringerUndersideOffsetDirection", "StringerUndersideOffset"):
                    obj.setEditorMode(name, 1 if obj.StringerCustomWidth else 0)
            self.rebuild(obj, allow_structure_changes=True)

    def execute(self, obj):
        # Geometry is rebuilt from onChanged() and the task panel. Rewriting a
        # child Sketch during the parent's recompute would keep the document
        # dirty and trigger additional recompute passes.
        return

    def defer_rebuilds(self, obj, deferred):
        """Hold rebuilds while ``deferred``, as the task panel's manual mode.

        Changes then only mark the stair as needing a rebuild, done by
        ``regenerate``; ending the deferral regenerates a pending one.
        """

        self._deferred = bool(deferred)
        if not self._deferred and getattr(self, "_rebuild_pending", False):
            self.regenerate(obj)

    def rebuild_pending(self):
        return bool(getattr(self, "_rebuild_pending", False))

    def regenerate(self, obj):
        """Rebuild the stair now, even while rebuilds are deferred."""

        deferred = getattr(self, "_deferred", False)
        self._deferred = False
        try:
            self.rebuild(obj, allow_structure_changes=True)
        finally:
            self._deferred = deferred

    def rebuild(self, obj, allow_structure_changes=True):
        if getattr(self, "_updating", False):
            return
        if getattr(self, "_deferred", False):
            self._rebuild_pending = True
            return
        self._rebuild_pending = False
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
                str(getattr(flight, f"{side}StringerType", "")) == "Cut stringer"
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
                rear - front for front, rear in zip(height_stations, height_stations[1:])
            ]
            flight_lengths = [_flight_length(flight) for flight in flights]
            stair_indices = [
                index for index, flight in enumerate(flights) if not _is_landing_flight(flight)
            ]
            if not stair_indices:
                # A collection of landings has no ascent. Placement controls
                # its elevation; the stored stair rise is retained for switching
                # back to a flight, but must not manufacture miniature risers.
                riser_height = 0.0
                height_stations = [0.0] * len(height_stations)
                effective_riser_heights = [0.0] * len(effective_riser_heights)
            stair_tread_counts = distribute_treads(
                [flight_lengths[index] for index in stair_indices],
                total_treads,
            )
            tread_counts = [0] * len(flights)
            for index, count in zip(stair_indices, stair_tread_counts):
                tread_counts[index] = count
            side_insets = _side_insets(obj, flights)
            layouts = self._flight_layouts(
                flights,
                tread_counts,
                riser_height,
                extra_widths,
                effective_riser_heights,
                side_insets,
            )
            balanced_sections = None
            balanced_footprint = None
            balanced_plan_shapes = None
            # The steps' own plan, ending on the newel posts.
            step_sections = None
            step_footprint = None
            step_plan_shapes = None
            winding_geometry_valid = True
            # What the walking line runs round and the steps' widths.
            walking_regions = []
            walking_widths = None
            walking_balanced = False
            has_circular = any(_is_circular_flight(flight) for flight in flights)
            has_landing_flight = any(_is_landing_flight(flight) for flight in flights)
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
            # The length of side a turned exit opens onto; 0 is the width.
            exit_width = (
                _quantity_value(getattr(last_flight, "ExitWidth", 0.0))
                if exit_direction != "Straight"
                else 0.0
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
                balanced_sections, average_going = tangent_flight_sections(
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
                    exit_width=exit_width,
                )
                balanced_footprint = make_tangent_stair_footprint(
                    flight_specs, turn_types, start_angle, end_angle
                )
                balanced_sections = fit_tangent_sections_to_footprint(
                    balanced_sections, balanced_footprint
                )
                if has_landing_flight:
                    partition_faces = tangent_tread_faces(balanced_sections, flight_specs)
                    winding_geometry_valid = len(partition_faces) == len(
                        balanced_sections
                    ) - 1 and all(face.isValid() for face in partition_faces)
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
                        layout["outer_width"],
                        layout["heading"],
                    )
                    for layout in layouts
                ]
                # The flights' widths are the stair's; the boards and posts
                # stand within them, beside the steps, designed at their own
                # width and moved into place.
                plan_shift = (0.0, 0.0)
                inset = inset_flight_specs(
                    flight_specs,
                    side_insets,
                    start_angle,
                    end_angle,
                    entry_direction,
                    exit_direction,
                )
                if inset is None:
                    side_insets = [(0.0, 0.0)] * len(flights)
                    layouts = self._flight_layouts(
                        flights,
                        tread_counts,
                        riser_height,
                        extra_widths,
                        effective_riser_heights,
                    )
                else:
                    flight_specs, plan_shift = inset
                    if exit_direction != "Straight":
                        # The opening, the flight's width by default, runs
                        # to the far side's board.
                        far_side = 1 if "left" in exit_direction.lower() else 0
                        opening = exit_width if exit_width > 1e-7 else layouts[-1]["outer_width"]
                        exit_width = max(opening - side_insets[-1][far_side], 0.01)
                walking_widths = [width for _length, width, _heading in flight_specs]
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
                    nosing=(_quantity_value(obj.Nosing) if obj.StairType == "Wood" else 0.0),
                    extra_widths=extra_widths,
                    walking_line_offset=_quantity_value(obj.WalkingLineOffset),
                    exit_width=exit_width,
                )
                balanced_footprint = make_stair_footprint(flight_specs, start_angle, end_angle)
                balanced_sections, balanced_footprint = translated_plan(
                    balanced_sections, balanced_footprint, plan_shift
                )
                unfitted_sections = balanced_sections
                balanced_sections = fit_balanced_sections_to_footprint(
                    balanced_sections, balanced_footprint
                )
                if obj.StairType == "Wood":
                    # The newel's size, as the stringers carry the nosings
                    # within it.
                    newel_reach = max(
                        _quantity_value(obj.HandrailPostWidth),
                        _quantity_value(obj.HandrailPostThickness),
                    )
                    balanced_sections = corner_post_sections(
                        balanced_sections,
                        entry_direction,
                        exit_direction,
                        unfitted_sections,
                        newel_reach,
                    )
                    balanced_sections = inner_corner_newel_sections(
                        balanced_sections, balanced_footprint, newel_reach
                    )
                    # The steps end on the newel posts; the boards and
                    # rails keep the sections ending on the stair's sides.
                    newels = _newel_post_squares(
                        obj,
                        flights,
                        balanced_sections,
                        balanced_footprint,
                        entry_direction,
                        exit_direction,
                    )
                    balanced_sections, step_sections, step_footprint = newel_fan_plan(
                        balanced_sections,
                        balanced_footprint,
                        entry_direction,
                        exit_direction,
                        newel_reach,
                        [square for square, _post in newels],
                        [post for _square, post in newels],
                        (
                            [width for _length, width, _heading in flight_specs],
                            _quantity_value(obj.WalkingLineOffset),
                        ),
                    )
                    walking_regions = newel_regions(
                        newel_corners(
                            balanced_sections,
                            balanced_footprint,
                            entry_direction,
                            exit_direction,
                        ),
                        newel_reach,
                        [square for square, _post in newels],
                    )
                    # A turned entry's or exit's step reaches the floor
                    # beyond the board.
                    openings = [
                        (index, side_insets[flight_index][0 if "left" in direction.lower() else 1])
                        for index, direction, flight_index in (
                            (0, entry_direction, 0),
                            (-1, exit_direction, len(flights) - 1),
                        )
                        if direction.lower() != "straight"
                    ]
                    opened = open_turned_endpoints(
                        step_sections if step_sections is not None else balanced_sections,
                        step_footprint if step_sections is not None else balanced_footprint,
                        openings,
                        [square for square, _post in newels],
                    )
                    if opened is not None:
                        step_sections, step_footprint = opened
                        if step_sections is balanced_sections:
                            step_sections = step_footprint = None
                    # Pulled onto the newels and opened to the floor, the
                    # treads take their goings along the walking line again,
                    # as the steps outline them; the boards follow, but on
                    # the posts.
                    if step_sections is not None:
                        even = even_walking_line_goings(
                            step_sections,
                            step_footprint,
                            walking_regions,
                            walking_widths,
                            _quantity_value(obj.WalkingLineOffset),
                            newel_reach,
                            balanced_footprint,
                        )
                        boards = (
                            follow_moved_ends(balanced_sections, step_sections, even)
                            if even is not None
                            else None
                        )
                    else:
                        even = boards = even_walking_line_goings(
                            balanced_sections,
                            balanced_footprint,
                            walking_regions,
                            walking_widths,
                            _quantity_value(obj.WalkingLineOffset),
                            newel_reach,
                        )
                    if boards is not None and self._partitions(boards, balanced_footprint):
                        balanced_sections = boards
                        walking_balanced = True
                        if step_sections is not None:
                            step_sections = even
                    # The steps' own shifts of their fronts along the sides;
                    # on a post only the step moves, along the post, on the
                    # steps' plan opened to the floor, out beyond the board.
                    offsets = _tread_side_offsets(obj, total_treads)
                    keep = (
                        {
                            (index, side)
                            for index, section in enumerate(step_sections)
                            for side in section.post_ends
                        }
                        if step_sections is not None
                        else set()
                    )
                    boards = shift_section_ends(
                        balanced_sections, balanced_footprint, offsets, keep
                    )
                    steps = (
                        shift_section_ends(step_sections, step_footprint, offsets)
                        if step_sections is not None
                        else None
                    )
                    if boards is not None and (step_sections is None or steps is not None):
                        balanced_sections = boards
                        if steps is not None:
                            step_sections = steps
                partition_faces = balanced_tread_faces(balanced_sections, balanced_footprint)
                winding_geometry_valid = balanced_partition_is_valid(
                    partition_faces,
                    balanced_footprint,
                    len(balanced_sections) - 1,
                )
                balanced_plan_shapes = partition_faces
                if not winding_geometry_valid:
                    step_sections = None
                elif step_sections is not None:
                    step_plan_shapes = balanced_tread_faces(step_sections, step_footprint)
            # The room the plan left beside the steps, for the boards.
            self.plan_insets = list(side_insets)
            if balanced_sections:
                balanced_sections = assign_section_elevations(
                    balanced_sections,
                    height_stations,
                )
                if step_sections is not None:
                    step_sections = assign_section_elevations(step_sections, height_stations)
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
                    step_sections = None
            walking_goings = self._store_walking_line(
                obj,
                step_sections if step_sections is not None else balanced_sections,
                walking_regions,
                walking_widths,
            )
            if walking_balanced and walking_goings and min(walking_goings) > 0.0:
                # The treads' going is the walking line's, less their extras.
                average_going = (
                    sum(walking_goings) - sum(list(extra_widths or [])[: len(walking_goings)])
                ) / len(walking_goings)
                for flight in flights:
                    flight.TreadWidth = average_going

            if balanced_sections is None:
                average_going, _goings = tread_goings(
                    sum(flight_lengths[index] for index in stair_indices),
                    total_treads,
                    extra_widths,
                )
            blondel_value = 2.0 * riser_height + average_going
            obj.NumberOfTreads = len(balanced_sections) - 1 if balanced_sections else total_treads
            obj.RiserHeight = riser_height
            obj.TreadWidth = average_going
            obj.BlondelValue = blondel_value
            obj.BlondelCompliant = BLONDEL_MINIMUM <= blondel_value <= BLONDEL_MAXIMUM
            obj.setEditorMode("StringerWidth", 0 if obj.StringerCustomWidth else 1)
            if not obj.StringerCustomWidth:
                obj.StringerWidth = automatic_stringer_width(
                    riser_height,
                    average_going,
                    _quantity_value(obj.StepThickness),
                    _quantity_value(obj.Nosing),
                    _quantity_value(obj.StringerNosingOffset),
                    str(obj.StringerNosingOffsetDirection),
                    _quantity_value(obj.StringerUndersideOffset),
                    str(obj.StringerUndersideOffsetDirection),
                )
            if not stair_indices:
                obj.GeometryStatus = translate("BIM", "Landings only (no rise)")
            elif has_tangent_geometry and winding_geometry_valid:
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
                    obj.GeometryStatus = translate("BIM", "Balanced entry/exit stair")
                elif any(section.landing_to_next for section in balanced_sections):
                    obj.GeometryStatus = translate("BIM", "Multi-flight stair with landing")
                else:
                    obj.GeometryStatus = translate("BIM", "Balanced multi-flight stair")

            self._update_plan(obj, layouts, balanced_sections, balanced_footprint)
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
                    step_sections if step_sections is not None else balanced_sections,
                    step_footprint if step_sections is not None else balanced_footprint,
                    step_plan_shapes if step_sections is not None else balanced_plan_shapes,
                    allow_structure_changes,
                )
                self._round_first_step(obj, layouts, balanced_sections, riser_height)
                self._update_stringers(
                    obj,
                    layouts,
                    balanced_sections,
                    balanced_footprint,
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
            self._update_handrail_joinery(obj)
            if obj.StairType != "Concrete" and getattr(obj, "StringerJoinery", True):
                self._update_stringer_joinery(obj)
        finally:
            self._updating = False

    def _flight_layouts(
        self,
        flights,
        tread_counts,
        riser_height,
        extra_widths=None,
        riser_heights=None,
        side_insets=None,
    ):
        """Return each flight's layout. Its width is the steps' own, the
        flight's less ``side_insets`` (see ``_side_insets``), and lies within
        the flight's."""

        layouts = []
        insets = list(side_insets or [])
        insets.extend([(0.0, 0.0)] * (len(flights) - len(insets)))
        extras = list(extra_widths or [])
        extra_cursor = 0
        heights = list(riser_heights or [])
        required_heights = sum(tread_counts) + 1
        if len(heights) < required_heights:
            heights.extend([riser_height] * (required_heights - len(heights)))
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
            outer_width = max(_quantity_value(flight.Width), 0.01)
            left_inset, right_inset = insets[index]
            width = max(outer_width - left_inset - right_inset, 0.01)
            length = _flight_length(flight)
            flight_extras = extras[extra_cursor : extra_cursor + tread_count]
            extra_cursor += tread_count
            flight_heights = heights[height_cursor : height_cursor + tread_count]
            section_heights = heights[height_cursor : height_cursor + tread_count + 1]
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
            origin = center - normal * (outer_width / 2.0 - right_inset)
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
                    "outer_width": outer_width,
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
