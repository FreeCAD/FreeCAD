# SPDX-License-Identifier: LGPL-2.1-or-later

"""Step, riser, plan, and concrete object generation."""

import FreeCAD
import Part

from .geometry_core import balanced_section_top
from .geometry_plan import balanced_tread_faces
from .geometry_stringer_path import straight_stringer_sections, stringer_flight_runs
from .geometry_steps import (
    balanced_plan_geometry,
    make_balanced_concrete_shape,
    make_balanced_riser_shape,
    make_balanced_tread_shape,
)
from .geometry_straight import (
    make_concrete_shape,
    make_riser_shape,
    make_tread_shape,
    plan_segments,
)

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_components import (
    _generated_parts,
    _make_component_group,
    _resize_generated_parts,
    _set_generated_properties,
    _set_tread_properties,
)

from .object_utils import (
    _child_placement,
    _combined_placement,
    _quantity_value,
    _set_global_shape,
    first_step_rounded,
    get_flights,
)


def _rounded_first_step_cutter(stair, sections, side, riser_height, placement):
    """Return the region that rounds the first step's free corner.

    The first step's end stands free on that side, in front of the board
    starting at the second riser. Its corner is rounded in plan from the
    nosing to that riser, as on traditional stairs, and by no more than
    half the step's width, which leaves room for the other side's.
    ``sections`` are the first flight's, which ``placement`` takes to the
    document. Returns ``None`` when the first step has no room for it.
    """

    if len(sections) < 2:
        return None
    first = sections[0]
    origin = FreeCAD.Vector(*getattr(first, side.lower()), 0.0)
    other = FreeCAD.Vector(*getattr(first, "right" if side == "Left" else "left"), 0.0)
    along = FreeCAD.Vector(*first.tangent, 0.0)
    if along.Length < 1e-9 or (other - origin).Length < 1e-9:
        return None
    along.normalize()
    across = other - origin
    across = (across - along * across.dot(along)).normalize()
    nose = origin - along * _quantity_value(stair.Nosing)
    second = FreeCAD.Vector(*getattr(sections[1], side.lower()), 0.0)
    radius = min((second - nose).dot(along), (other - origin).Length / 2.0)
    if radius < 1.0:
        return None
    # The square clears the nosing and the tread's side by a margin.
    margin = 50.0
    bottom = -margin

    def point(forward, inward):
        return nose + along * forward + across * inward + FreeCAD.Vector(0.0, 0.0, bottom)

    square = Part.Face(
        Part.makePolygon(
            [
                point(-margin, -margin),
                point(radius, -margin),
                point(radius, radius),
                point(-margin, radius),
                point(-margin, -margin),
            ]
        )
    )
    disc = Part.Face(
        Part.Wire(Part.makeCircle(radius, point(radius, radius), FreeCAD.Vector(0.0, 0.0, 1.0)))
    )
    # Up to the first step's top only: the steps above are left whole.
    top = balanced_section_top(first, 0, riser_height) + 1.0
    cutter = square.cut(disc).extrude(FreeCAD.Vector(0.0, 0.0, top - bottom))
    cutter.transformShape(placement.toMatrix())
    return cutter


class StairStepsMixin:
    """Implementation methods grouped by responsibility."""

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
            lines.extend(balanced_plan_geometry(balanced_sections, balanced_footprint))
        else:
            nosing = (
                _quantity_value(stair.Nosing)
                if str(stair.StairType) == "Wood" or stair.StepsEnabled
                else 0.0
            )
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
                for start, end in plan_segments(
                    layout["metrics"],
                    layout["width"],
                    nosing,
                    layout["tread_goings"],
                ):
                    start_point = plan_placement.multVec(FreeCAD.Vector(start[0], start[1], 0.0))
                    end_point = plan_placement.multVec(FreeCAD.Vector(end[0], end[1], 0.0))
                    lines.append(Part.LineSegment(start_point, end_point))
        sketch.addGeometry(lines, False)
        sketch.Placement = _child_placement(stair)

    def _update_wood(
        self,
        stair,
        layouts,
        balanced_sections,
        balanced_footprint,
        balanced_plan_shapes,
        allow_structure_changes,
        concrete_dressing=False,
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
        riser_count = (
            total_treads + (1 if stair.EndWithRiser else 0)
            if stair.RisersEnabled and _quantity_value(stair.RiserHeight) > 1e-7
            else 0
        )
        risers = _generated_parts(group, stair, "Riser")
        if allow_structure_changes:
            treads = _resize_generated_parts(group, stair, "Tread", total_treads)
            risers = _resize_generated_parts(group, stair, "Riser", riser_count)
        for tread in treads:
            _set_tread_properties(tread)

        step_thickness = max(_quantity_value(stair.StepThickness), 0.01)
        nosing = max(_quantity_value(stair.Nosing), 0.0)
        riser_thickness = max(_quantity_value(stair.RiserThickness), 0.01)
        step_riser_overlap = (
            0.0 if concrete_dressing else max(_quantity_value(stair.StepRiserOverlap), 0.0)
        )
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
                concrete_dressing,
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
                back_extension = 0.0
                if stair.RisersEnabled:
                    if concrete_dressing:
                        is_last_tread = (
                            layout is layouts[-1] and local_index == metrics.tread_count - 1
                        )
                        if stair.PriorityToRiser and (not is_last_tread or stair.EndWithRiser):
                            back_extension = -riser_thickness
                    else:
                        back_extension = step_riser_overlap
                        if not stair.PriorityToRiser:
                            back_extension += riser_thickness
                tread_shape = make_tread_shape(
                    local_index,
                    metrics,
                    layout["width"],
                    step_thickness,
                    nosing,
                    back_extension,
                    layout["tread_goings"],
                    layout["riser_heights"],
                )
                tread.Shape = tread_shape
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
            local_riser_count = metrics.tread_count + (
                1 if stair.EndWithRiser and layout is layouts[-1] else 0
            )
            for local_index in range(local_riser_count):
                riser = risers[generated_index]
                riser.Label = f"{translate('BIM', 'Riser')} {generated_index + 1}"
                riser.Index = generated_index + 1
                riser.FlightIndex = layout["index"] + 1
                riser.Shape = make_riser_shape(
                    local_index,
                    metrics,
                    layout["width"],
                    riser_thickness,
                    step_thickness,
                    upper_offset,
                    lower_offset,
                    stair.PriorityToRiser,
                    layout["tread_goings"],
                    layout["section_riser_heights"],
                    concrete_dressing,
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
        concrete_dressing=False,
    ):
        tread_count = len(sections) - 1
        riser_height = _quantity_value(stair.RiserHeight)
        base_faces = plan_shapes or balanced_tread_faces(sections, footprint)
        corner_points = (
            {
                getattr(front, side)
                for front, rear in zip(sections, sections[1:])
                for side in ("left", "right")
                if getattr(front, side) == getattr(rear, side)
            }
            # Ends on a newel post: their bands stop at the post too.
            | {
                getattr(section, side)
                for section in sections
                for side in getattr(section, "post_ends", ())
            }
            if not concrete_dressing
            else set()
        )
        for index, (front, rear, base_face) in enumerate(zip(sections, sections[1:], base_faces)):
            tread = treads[index]
            tread.Label = f"{translate('BIM', 'Step')} {index + 1}"
            tread.Index = index + 1
            tread.FlightIndex = front.flight_index + 1
            back_extension = 0.0
            if stair.RisersEnabled:
                if concrete_dressing:
                    if stair.PriorityToRiser and (index < tread_count - 1 or stair.EndWithRiser):
                        back_extension = -riser_thickness
                else:
                    back_extension = step_riser_overlap
                    if not stair.PriorityToRiser:
                        back_extension += riser_thickness
            tread_shape = make_balanced_tread_shape(
                front,
                rear,
                footprint,
                balanced_section_top(front, index, riser_height),
                step_thickness,
                nosing,
                back_extension,
                base_face,
                local_expansion=(
                    front.landing_to_next
                    or rear.landing_to_next
                    or (index > 0 and sections[index - 1].landing_to_next)
                ),
                corner_points=corner_points,
            )
            tread.Shape = tread_shape
            tread.Placement = _child_placement(stair)
            if FreeCAD.GuiUp:
                tread.ViewObject.ShapeColor = (0.72, 0.48, 0.25)

        if not stair.RisersEnabled:
            return
        upper_offset = _quantity_value(stair.RiserUpperOffset)
        lower_offset = _quantity_value(stair.RiserLowerOffset)
        riser_sections = list(enumerate(sections if stair.EndWithRiser else sections[:-1]))
        for generated_index, (riser, (index, section)) in enumerate(zip(risers, riser_sections)):
            top = balanced_section_top(section, index, riser_height)
            previous_top = (
                balanced_section_top(
                    sections[index - 1],
                    index - 1,
                    riser_height,
                )
                if index
                else 0.0
            )
            rise = max(top - previous_top, 0.01)
            if concrete_dressing:
                base = (
                    previous_top
                    - (step_thickness if stair.PriorityToRiser and index > 0 else 0.0)
                    + lower_offset
                )
                finished_top = top - (step_thickness if index < tread_count else 0.0) - upper_offset
                height = finished_top - base
            else:
                bottom_extension = step_thickness if stair.PriorityToRiser and index > 0 else 0.0
                height = (
                    rise
                    - (0.0 if index == tread_count else step_thickness)
                    + bottom_extension
                    - upper_offset
                    - lower_offset
                )
                base = top - rise - bottom_extension + lower_offset
            riser.Label = f"{translate('BIM', 'Riser')} {generated_index + 1}"
            riser.Index = generated_index + 1
            riser.FlightIndex = section.flight_index + 1
            riser.Shape = make_balanced_riser_shape(
                section,
                base,
                height,
                riser_thickness,
                footprint,
                local_expansion=(
                    section.landing_to_next or (index > 0 and sections[index - 1].landing_to_next)
                ),
                concrete_dressing=concrete_dressing,
                corner_points=corner_points,
            )
            riser.Placement = _child_placement(stair)
            if FreeCAD.GuiUp:
                riser.ViewObject.ShapeColor = (0.58, 0.36, 0.18)

    def _round_first_step(self, stair, layouts, balanced_sections, riser_height):
        """Round the first step's end on the sides the stair asks it for."""

        group = stair.StepsGroup
        sides = [side for side in ("Left", "Right") if first_step_rounded(stair, side)]
        if not group or not sides:
            return
        if balanced_sections:
            runs = stringer_flight_runs(
                balanced_sections,
                [str(flight.FlightType) for flight in get_flights(stair)],
            )
            sections = runs[0][1] if runs else []
            placement = _child_placement(stair)
        elif len(layouts) == 1:
            layout = layouts[0]
            sections = straight_stringer_sections(
                layout["metrics"],
                layout["width"],
                layout["tread_goings"],
                layout["section_top_elevations"],
            )
            # A single flight's sections are the flight's own.
            placement = _combined_placement(stair, layout["placement"])
        else:
            return
        steps = [
            part
            for part in group.Group
            if str(getattr(part, "StairDesignerRole", "")) in ("Tread", "Riser")
            and not part.Shape.isNull()
        ]
        for side in sides:
            cutter = _rounded_first_step_cutter(stair, sections, side, riser_height, placement)
            if cutter is None:
                continue
            for part in steps:
                shape = part.Shape
                if not shape.BoundBox.intersect(cutter.BoundBox):
                    continue
                rounded = shape.cut(cutter)
                if len(rounded.Solids) == 1 and abs(rounded.Volume - shape.Volume) > 1e-6:
                    _set_global_shape(part, rounded.Solids[0])

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
            result = make_balanced_concrete_shape(
                balanced_sections,
                balanced_footprint,
                _quantity_value(stair.RiserHeight),
                _quantity_value(stair.ConcreteThickness),
                balanced_plan_shapes,
                _quantity_value(stair.BottomCutDistance),
                _quantity_value(stair.TopCutDistance),
                (_quantity_value(stair.StepThickness) if stair.StepsEnabled else 0.0),
                (_quantity_value(stair.StructureWidthOffset) if stair.StepsEnabled else 0.0),
            )
        else:
            shapes = []
            for index, layout in enumerate(layouts):
                shape = make_concrete_shape(
                    layout["metrics"],
                    layout["width"],
                    _quantity_value(stair.ConcreteThickness),
                    (_quantity_value(stair.BottomCutDistance) if index == 0 else 0.0),
                    (_quantity_value(stair.TopCutDistance) if index == len(layouts) - 1 else 0.0),
                    (_quantity_value(stair.StepThickness) if stair.StepsEnabled else 0.0),
                    (_quantity_value(stair.StructureWidthOffset) if stair.StepsEnabled else 0.0),
                )
                shape.Placement = layout["placement"]
                shapes.append(shape)
            result = shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
        concrete.Shape = result
        concrete.Placement = _child_placement(stair)
        if FreeCAD.GuiUp:
            concrete.ViewObject.ShapeColor = (0.72, 0.72, 0.72)
