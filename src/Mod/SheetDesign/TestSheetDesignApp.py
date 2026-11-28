# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Tests of the SheetDesign module."""

import math
import os
import tempfile
import unittest

import FreeCAD
import Part
import SheetDesign  # noqa: F401  Registers the document object types.

V = FreeCAD.Vector


def find_edge(shape, start, end):
    """Return the name of the straight edge between two points."""
    for index, edge in enumerate(shape.Edges):
        if len(edge.Vertexes) != 2:
            continue
        p, q = edge.Vertexes[0].Point, edge.Vertexes[1].Point
        if (p.distanceToPoint(start) < 1e-6 and q.distanceToPoint(end) < 1e-6) or (
            p.distanceToPoint(end) < 1e-6 and q.distanceToPoint(start) < 1e-6
        ):
            return f"Edge{index + 1}"
    raise AssertionError(f"no edge between {start} and {end}")


class TestSheetDesign(unittest.TestCase):
    T = 2.0
    R = 1.0
    K = 0.5
    LENGTH = 100.0
    WIDTH = 50.0

    def setUp(self):
        self.doc = FreeCAD.newDocument("SheetDesignTest")
        self.rule = self.doc.addObject("App::VarSet", "Rule")
        self.rule.addProperty("App::PropertyLength", "Thickness", "Sheet")
        self.rule.addProperty("App::PropertyLength", "BendRadius", "Sheet")
        self.rule.addProperty("App::PropertyFloat", "KFactor", "Sheet")
        self.rule.Thickness = self.T
        self.rule.BendRadius = self.R
        self.rule.KFactor = self.K

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    # ---------------------------------------------------------------------------------- helpers

    def allowance(self, angle=90.0):
        return (self.R + self.K * self.T) * math.radians(angle)

    def add_shape(self, shape, name="Profile"):
        obj = self.doc.addObject("Part::Feature", name)
        obj.Shape = shape
        return obj

    def add_base(self):
        wire = Part.makePolygon(
            [V(0, 0, 0), V(self.LENGTH, 0, 0), V(self.LENGTH, self.WIDTH, 0), V(0, self.WIDTH, 0), V(0, 0, 0)]
        )
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(wire)
        base.Rule = self.rule
        self.doc.recompute()
        return base

    def add_flange(self, source, start, end, **properties):
        flange = self.doc.addObject("SheetDesign::Flange", "Flange")
        flange.Edge = (source, [find_edge(source.Shape, start, end)])
        for name, value in properties.items():
            setattr(flange, name, value)
        self.doc.recompute()
        return flange

    def add_end_flange(self, source, **properties):
        """Flange on the top edge at x = LENGTH. It goes up (+z)."""
        properties.setdefault("Length", 20.0)
        return self.add_flange(
            source, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), **properties
        )

    def add_flat(self, source):
        flat = self.doc.addObject("SheetDesign::FlatPattern", "Flat")
        flat.Source = source
        self.doc.recompute()
        return flat

    def add_cut(self, source, profile, space):
        cut = self.doc.addObject("SheetDesign::Cut", "Cut")
        cut.Source = source
        cut.Profile = profile
        cut.Space = space
        self.doc.recompute()
        return cut

    def assertSolid(self, obj):
        self.assertTrue(obj.isValid(), obj.getStatusString())
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(len(obj.Shape.Solids), 1)

    # ------------------------------------------------------------------------------------ tests

    def test_base_panel(self):
        base = self.add_base()
        self.assertSolid(base)
        self.assertAlmostEqual(base.Shape.Volume, self.LENGTH * self.WIDTH * self.T, places=6)

    def test_flange_folded_shape(self):
        flange = self.add_end_flange(self.add_base())
        self.assertSolid(flange)
        box = flange.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH + self.R + self.T, places=6)
        self.assertAlmostEqual(box.ZMax, self.T + self.R + 20.0, places=6)
        self.assertAlmostEqual(box.ZMin, 0.0, places=6)
        bend_volume = math.pi / 4 * ((self.R + self.T) ** 2 - self.R**2) * self.WIDTH
        expected = (self.LENGTH + 20.0) * self.WIDTH * self.T + bend_volume
        self.assertAlmostEqual(flange.Shape.Volume, expected, places=5)

    def test_flange_down(self):
        base = self.add_base()
        flange = self.add_flange(
            base, V(self.LENGTH, 0, 0), V(self.LENGTH, self.WIDTH, 0), Length=20.0
        )
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.ZMin, -(self.R + 20.0), places=6)
        flange.Reversed = True
        self.doc.recompute()
        self.assertAlmostEqual(flange.Shape.BoundBox.ZMax, self.T + self.R + 20.0, places=6)

    def test_flat_pattern(self):
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertEqual(len(flat.Shape.Faces), 1)
        flat_length = self.LENGTH + self.allowance() + 20.0
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, flat_length * self.WIDTH, places=6)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, flat_length, places=6)
        # One outline with four edges, and one bend line.
        self.assertEqual(len(flat.Shape.Faces[0].Edges), 4)
        self.assertEqual(len(flat.Shape.Edges), 5)

    def test_bend_allowance_reference_case(self):
        """The test case of the SheetMetal addon README: end to mold line = 50 mm."""
        self.rule.Thickness = 2.0
        self.rule.BendRadius = 1.64
        self.rule.KFactor = 0.38
        flange = self.add_end_flange(self.add_base(), Length=48.12)
        self.assertAlmostEqual(flange.Shape.BoundBox.ZMax, 51.76, places=6)
        flat = self.add_flat(flange)
        bend_line = [e for e in flat.Shape.Edges if e not in flat.Shape.Faces[0].Edges]
        bend_x = [e for e in flat.Shape.Edges if abs(e.BoundBox.XLength) < 1e-9]
        positions = sorted({round(e.BoundBox.XMin, 6) for e in bend_x})
        self.assertTrue(bend_line)
        # Positions: start of the base, the bend line, and the end of the flange.
        self.assertEqual(len(positions), 3)
        self.assertAlmostEqual(positions[2] - positions[1], 50.0, places=2)

    def test_gaps(self):
        flange = self.add_end_flange(self.add_base(), Gap1=5.0, Gap2=10.0)
        self.assertSolid(flange)
        flat = self.add_flat(flange)
        expected = self.LENGTH * self.WIDTH + (self.allowance() + 20.0) * (self.WIDTH - 15.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=6)

    def test_flange_on_flange(self):
        first = self.add_end_flange(self.add_base())
        # The outer top edge of the first flange.
        x = self.LENGTH + self.R + self.T
        z = self.T + self.R + 20.0
        second = self.add_flange(first, V(x, 0, z), V(x, self.WIDTH, z), Length=15.0, Angle=45.0)
        self.assertSolid(second)
        flat = self.add_flat(second)
        flat_length = self.LENGTH + self.allowance() + 20.0 + self.allowance(45.0) + 15.0
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, flat_length * self.WIDTH, places=6)

    def test_edge_with_bend_is_rejected(self):
        first = self.add_end_flange(self.add_base())
        again = self.add_flange(first, V(self.LENGTH, 0, 0), V(self.LENGTH, self.WIDTH, 0))
        self.assertFalse(again.isValid())

    def test_cut_in_folded_state_shows_in_flat_pattern(self):
        flange = self.add_end_flange(self.add_base())
        # A circle on the outer face of the flange.
        x = self.LENGTH + self.R + self.T
        height = 10.0
        circle = Part.Face(
            Part.Wire(Part.makeCircle(3.0, V(x, 25, self.T + self.R + height), V(1, 0, 0)))
        )
        cut = self.add_cut(flange, self.add_shape(circle, "Circle"), "Folded")
        self.assertSolid(cut)
        self.assertAlmostEqual(
            flange.Shape.Volume - cut.Shape.Volume, math.pi * 9.0 * self.T, places=5
        )
        flat = self.add_flat(cut)
        face = flat.Shape.Faces[0]
        self.assertEqual(len(face.Wires), 2)
        hole = [w for w in face.Wires if not w.isSame(face.OuterWire)][0]
        center = hole.Edges[0].Curve.Center
        self.assertAlmostEqual(center.x, self.LENGTH + self.allowance() + height, places=6)
        self.assertAlmostEqual(center.y, 25.0, places=6)

    def test_cut_in_flat_state_shows_in_folded_solid(self):
        flange = self.add_end_flange(self.add_base())
        height = 10.0
        circle = Part.Face(
            Part.Wire(Part.makeCircle(4.0, V(self.LENGTH + self.allowance() + height, 25, 0)))
        )
        cut = self.add_cut(flange, self.add_shape(circle, "Circle"), "Flat")
        self.assertSolid(cut)
        self.assertAlmostEqual(
            flange.Shape.Volume - cut.Shape.Volume, math.pi * 16.0 * self.T, places=5
        )
        # The hole is in the flange: a cylinder with its axis along X. The radius is different
        # from the two radii of the bend.
        cylinders = [
            f for f in cut.Shape.Faces
            if f.Surface.TypeId == "Part::GeomCylinder" and abs(f.Surface.Radius - 4.0) < 1e-6
        ]
        self.assertTrue(cylinders)
        for cylinder in cylinders:
            self.assertAlmostEqual(abs(cylinder.Surface.Axis.x), 1.0, places=6)
            self.assertAlmostEqual(cylinder.Surface.Center.z, self.T + self.R + height, places=6)

    def test_cut_across_bend_in_folded_space_is_rejected(self):
        """A profile on a panel has no clear position in the bend."""
        flange = self.add_end_flange(self.add_base())
        circle = Part.Face(Part.Wire(Part.makeCircle(3.0, V(self.LENGTH, 25, 0))))
        cut = self.add_cut(flange, self.add_shape(circle, "Circle"), "Folded")
        self.assertFalse(cut.isValid())

    def test_rule_change_updates_all_outputs(self):
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.rule.Thickness = 3.0
        self.rule.KFactor = 0.4
        self.doc.recompute()
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH + self.R + 3.0, places=6)
        allowance = (self.R + 0.4 * 3.0) * math.pi / 2
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, self.LENGTH + allowance + 20.0, places=6
        )

    def test_base_on_other_plane(self):
        wire = Part.makePolygon([V(0, 0, 0), V(0, 40, 0), V(0, 40, 30), V(0, 0, 30), V(0, 0, 0)])
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(wire)
        base.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(base)
        self.assertAlmostEqual(base.Shape.Volume, 40 * 30 * self.T, places=6)
        self.assertAlmostEqual(base.Shape.BoundBox.XLength, self.T, places=6)
        flat = self.add_flat(base)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, 0.0, places=6)

    def test_flat_pattern_after_document_restore(self):
        flange = self.add_end_flange(self.add_base())
        name = flange.Name
        path = os.path.join(tempfile.mkdtemp(), "sheet.FCStd")
        self.doc.saveAs(path)
        FreeCAD.closeDocument(self.doc.Name)
        self.doc = FreeCAD.openDocument(path)
        flat = self.add_flat(self.doc.getObject(name))
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, self.LENGTH + self.allowance() + 20.0, places=6
        )

    def test_profile_with_holes(self):
        outer = Part.makePolygon([V(0, 0, 0), V(60, 0, 0), V(60, 40, 0), V(0, 40, 0), V(0, 0, 0)])
        holes = [Part.Wire(Part.makeCircle(2.0, V(10 + 10 * i, 20, 0))) for i in range(5)]
        # One hole has the opposite direction.
        holes[2] = Part.Wire(Part.makeCircle(2.0, V(30, 20, 0), V(0, 0, -1)))
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(Part.makeCompound([outer] + holes))
        base.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(base)
        area = 60 * 40 - 5 * math.pi * 4.0
        self.assertAlmostEqual(base.Shape.Volume, area * self.T, places=5)

    def test_profile_with_island_in_hole(self):
        """Wires that are one in the other use the general face maker."""
        wires = [
            Part.Wire(Part.makeCircle(20.0, V(0, 0, 0))),
            Part.Wire(Part.makeCircle(10.0, V(0, 0, 0))),
        ]
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(Part.makeCompound(wires))
        base.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(base)
        self.assertAlmostEqual(base.Shape.Volume, math.pi * (400 - 100) * self.T, places=5)

    def test_rule_without_property_is_rejected(self):
        empty = self.doc.addObject("App::VarSet", "Empty")
        base = self.add_base()
        base.Rule = empty
        self.doc.recompute()
        self.assertFalse(base.isValid())

    def test_rule_value_from_expression(self):
        self.rule.addProperty("App::PropertyLength", "Gauge", "Sheet")
        self.rule.Gauge = 1.5
        self.rule.setExpression("Thickness", "Gauge * 2")
        base = self.add_base()
        self.assertAlmostEqual(base.Shape.BoundBox.ZMax, 3.0, places=6)
        flange = self.add_flange(base, V(self.LENGTH, 0, 3.0), V(self.LENGTH, self.WIDTH, 3.0))
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH + self.R + 3.0, places=6)

    # ------------------------------------------------------- entry from a finished flat pattern

    def add_refold(self, face, lines, angles):
        obj = self.doc.addObject("SheetDesign::Refold", "Refold")
        obj.Profile = self.add_shape(face, "FlatProfile")
        obj.BendLines = self.add_shape(Part.makeCompound(lines), "BendLines")
        obj.Angles = angles
        obj.Rule = self.rule
        self.doc.recompute()
        return obj

    def flat_rectangle(self, length):
        return Part.Face(
            Part.makePolygon(
                [V(0, 0, 0), V(length, 0, 0), V(length, self.WIDTH, 0), V(0, self.WIDTH, 0), V(0, 0, 0)]
            )
        )

    def test_refold_one_bend(self):
        allowance = self.allowance()
        face = self.flat_rectangle(self.LENGTH + allowance + 20.0)
        x = self.LENGTH + allowance / 2
        line = Part.makeLine(V(x, 10, 0), V(x, 40, 0))  # The line can be shorter than the part.
        folded = self.add_refold(face, [line], [90.0])
        self.assertSolid(folded)
        box = folded.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH + self.R + self.T, places=6)
        self.assertAlmostEqual(box.ZMax, self.T + self.R + 20.0, places=6)
        # The flat pattern of the result is the input.
        flat = self.add_flat(folded)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, face.Area, places=6)
        self.assertEqual(len(flat.Shape.Faces[0].Edges), 4)

    def test_refold_negative_angle_bends_down(self):
        allowance = self.allowance()
        face = self.flat_rectangle(self.LENGTH + allowance + 20.0)
        x = self.LENGTH + allowance / 2
        folded = self.add_refold(face, [Part.makeLine(V(x, 0, 0), V(x, 50, 0))], [-90.0])
        self.assertSolid(folded)
        self.assertAlmostEqual(folded.Shape.BoundBox.ZMin, -(self.R + 20.0), places=6)

    def test_refold_agrees_with_flanges(self):
        """The flat pattern of a part with flanges gives the same part again."""
        first = self.add_end_flange(self.add_base(), Gap1=5.0, Gap2=5.0)
        second = self.add_flange(
            first, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=15.0, Angle=45.0
        )
        flat = self.add_flat(second)
        face = flat.Shape.Faces[0]
        lines = flat.Shape.SubShapes[1:]
        self.assertEqual(len(lines), 2)
        folded = self.add_refold(face, lines, [90.0, 45.0])
        self.assertSolid(folded)
        self.assertAlmostEqual(folded.Shape.Volume, second.Shape.Volume, places=4)
        for name in ("XMin", "XMax", "YMin", "YMax", "ZMin", "ZMax"):
            self.assertAlmostEqual(
                getattr(folded.Shape.BoundBox, name), getattr(second.Shape.BoundBox, name), places=5
            )
        # More features can follow.
        z = self.T + self.R + 20.0
        x = self.LENGTH + self.R + self.T
        more = self.add_flange(folded, V(x, 5, z), V(x, self.WIDTH - 5, z), Length=8.0)
        self.assertSolid(more)

    def test_refold_line_off_the_part_is_rejected(self):
        face = self.flat_rectangle(100.0)
        folded = self.add_refold(face, [Part.makeLine(V(200, 0, 0), V(200, 50, 0))], [90.0])
        self.assertFalse(folded.isValid())

    # ----------------------------------------------------------------- entry from a folded solid

    def part_bracket(self):
        """An L bracket with two holes, made only with the Part module."""
        t, r, width = self.T, self.R, self.WIDTH
        base = Part.makeBox(self.LENGTH, width, t)
        section = Part.Face(
            Part.makePolygon(
                [
                    V(self.LENGTH, 0, 0),
                    V(self.LENGTH, width, 0),
                    V(self.LENGTH, width, t),
                    V(self.LENGTH, 0, t),
                    V(self.LENGTH, 0, 0),
                ]
            )
        )
        bend = section.revolve(V(self.LENGTH, 0, t + r), V(0, -1, 0), 90)
        wall = Part.makeBox(t, width, 20.0, V(self.LENGTH + r, 0, t + r))
        solid = base.fuse([bend, wall]).removeSplitter()
        solid = solid.cut(Part.makeCylinder(4.0, 10, V(50, 25, -5)))
        solid = solid.cut(
            Part.makeCylinder(3.0, 10, V(self.LENGTH - 2, 25, t + r + 10.0), V(1, 0, 0))
        )
        self.assertTrue(solid.isValid())
        self.assertEqual(len(solid.Solids), 1)
        return solid

    def add_unfold(self, shape, normal_z):
        source = self.add_shape(shape, "Solid")
        name = None
        best = 0.0
        for index, face in enumerate(shape.Faces):
            if face.Surface.TypeId != "Part::GeomPlane":
                continue
            if face.normalAt(0, 0).z * normal_z > 0.99 and face.Area > best:
                best = face.Area
                name = f"Face{index + 1}"
        obj = self.doc.addObject("SheetDesign::Unfold", "Unfold")
        obj.Face = (source, [name])
        obj.Rule = self.rule
        self.doc.recompute()
        return obj

    def check_bracket(self, converted, shape):
        self.assertSolid(converted)
        self.assertAlmostEqual(converted.Shape.Volume, shape.Volume, places=4)
        flat = self.add_flat(converted)
        face = flat.Shape.Faces[0]
        expected = (self.LENGTH + self.allowance() + 20.0) * self.WIDTH - math.pi * (16.0 + 9.0)
        self.assertAlmostEqual(face.Area, expected, places=5)
        self.assertEqual(len(face.Wires), 3)
        self.assertEqual(len(flat.Shape.SubShapes), 2)  # The face and one bend line.

    def test_unfold_made_with_part(self):
        shape = self.part_bracket()
        converted = self.add_unfold(shape, 1.0)
        self.check_bracket(converted, shape)
        # The hole of the wall is at the correct position in the flat pattern.
        flat = [o for o in self.doc.Objects if o.TypeId == "SheetDesign::FlatPattern"][0]
        face = flat.Shape.Faces[0]
        centers = sorted(
            w.Edges[0].Curve.Center.x for w in face.Wires if not w.isSame(face.OuterWire)
        )
        self.assertAlmostEqual(centers[0], 50.0, places=5)
        self.assertAlmostEqual(centers[1], self.LENGTH + self.allowance() + 10.0, places=5)

    def test_unfold_root_on_other_side(self):
        shape = self.part_bracket()
        converted = self.add_unfold(shape, -1.0)
        self.check_bracket(converted, shape)

    def test_unfold_reads_thickness_and_radius(self):
        shape = self.part_bracket()
        self.rule.Thickness = 9.0  # The thickness of the rule is not used.
        self.rule.BendRadius = 7.0  # Only later bends use the radius of the rule.
        converted = self.add_unfold(shape, 1.0)
        self.check_bracket(converted, shape)

    def test_unfold_then_flange(self):
        shape = self.part_bracket()
        converted = self.add_unfold(shape, 1.0)
        flange = self.add_flange(converted, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=12.0)
        self.assertSolid(flange)
        flat = self.add_flat(flange)
        expected = (
            self.LENGTH + 2 * self.allowance() + 20.0 + 12.0
        ) * self.WIDTH - math.pi * (16.0 + 9.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    def test_unfold_of_flange_part(self):
        """A part with three flanges, without its history, gives the same flat pattern."""
        first = self.add_end_flange(self.add_base(), Gap1=5.0, Gap2=5.0)
        second = self.add_flange(
            first, V(0, 0, 0), V(0, self.WIDTH, 0), Length=15.0, Angle=60.0
        )
        x = self.LENGTH + self.R + self.T
        z = self.T + self.R + 20.0
        third = self.add_flange(second, V(x, 5, z), V(x, self.WIDTH - 5, z), Length=8.0)
        expected = self.add_flat(third).Shape.Faces[0].Area
        converted = self.add_unfold(third.Shape.copy(), 1.0)
        self.assertSolid(converted)
        self.assertAlmostEqual(converted.Shape.Volume, third.Shape.Volume, places=4)
        flat = self.add_flat(converted)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)
        self.assertEqual(len(flat.Shape.SubShapes), 4)

    # ------------------------------------------------------------ layers: offset and overlap

    def test_flange_offset_moves_the_bend(self):
        flange = self.add_end_flange(self.add_base(), Offset=5.0)
        self.assertSolid(flange)
        thick = self.R + self.T
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH + 5.0 + thick, places=6)
        flat = self.add_flat(flange)
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, self.LENGTH + 5.0 + self.allowance() + 20.0, places=6
        )
        flange.Offset = -5.0
        self.doc.recompute()
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH - 5.0 + thick, places=6)
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, self.LENGTH - 5.0 + self.allowance() + 20.0, places=6
        )

    def test_overlap_is_zero_for_a_part_that_can_exist(self):
        flange = self.add_end_flange(self.add_base())
        self.assertEqual(flange.Overlap.Value, 0.0)

    def test_overlap_reports_a_collision(self):
        first = self.add_end_flange(self.add_base())
        x = self.LENGTH + self.R + self.T
        z = self.T + self.R + 20.0
        # Back across the base, then down through the base.
        inner = x - self.T
        second = self.add_flange(first, V(inner, 0, z), V(inner, self.WIDTH, z), Length=30.0)
        self.assertSolid(second)
        self.assertEqual(second.Overlap.Value, 0.0)
        x3 = x - self.T - self.R - 30.0
        z3 = z + self.R
        third = self.add_flange(second, V(x3, 0, z3), V(x3, self.WIDTH, z3), Length=40.0)
        self.assertSolid(third)
        self.assertGreater(third.Overlap.Value, 100.0)
        # The solid is the correct union: its volume is smaller than the volume of the parts.
        parts = second.Shape.Volume + (self.allowance() + 40.0) * self.WIDTH * self.T
        self.assertAlmostEqual(third.Shape.Volume, parts - third.Overlap.Value, places=3)

    def corner_with_two_flaps(self, offset):
        """Two walls at 90 degrees, each with a flap that folds across the opening."""
        a = self.allowance()
        length, width, depth = 80.0, 60.0, 40.0
        x2 = length + a
        faces = [
            self.flat_rectangle_at(0, 0, length, depth),
            self.flat_rectangle_at(length, 0, x2, depth),
            self.flat_rectangle_at(x2, 0, x2 + width, depth),
            # The first wall is longer by the offset. Its flap is the outer flap.
            self.flat_rectangle_at(0, depth, length, depth + offset + a + 30.0),
            self.flat_rectangle_at(x2, depth, x2 + width, depth + a + 20.0),
        ]
        face = faces[0].fuse(faces[1:]).removeSplitter().Faces[0]
        lines = [
            Part.makeLine(V(length + a / 2, 0, 0), V(length + a / 2, depth, 0)),
            Part.makeLine(V(0, depth + offset + a / 2, 0), V(length, depth + offset + a / 2, 0)),
            Part.makeLine(V(x2, depth + a / 2, 0), V(x2 + width, depth + a / 2, 0)),
        ]
        return self.add_refold(face, lines, [90.0])

    def flat_rectangle_at(self, x0, y0, x1, y1):
        return Part.Face(
            Part.makePolygon([V(x0, y0, 0), V(x1, y0, 0), V(x1, y1, 0), V(x0, y1, 0), V(x0, y0, 0)])
        )

    def test_scores_on_one_line_give_flaps_in_the_same_layer(self):
        folded = self.corner_with_two_flaps(0.0)
        self.assertSolid(folded)
        self.assertGreater(folded.Overlap.Value, 100.0)

    def test_score_with_offset_acts_only_on_its_panel(self):
        """The outer flap score is one thickness higher. It must not bend the other wall."""
        folded = self.corner_with_two_flaps(self.T)
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)
        flat = self.add_flat(folded)
        self.assertEqual(len(flat.Shape.SubShapes), 4)  # The face and three bend lines.

    # ------------------------------------------------------------------ stable element names

    def names(self, obj, prefix):
        return {v for k, v in obj.Shape.ElementReverseMap.items() if k.startswith(prefix)}

    def test_base_panel_names(self):
        base = self.add_base()
        edges = self.names(base, "Edge")
        self.assertEqual(
            edges, {f"Base_E{i}_{side}" for i in range(1, 5) for side in ("T", "B")}
        )
        faces = self.names(base, "Face")
        self.assertEqual(faces, {"Base_Top", "Base_Bottom"} | {f"Base_E{i}_S" for i in range(1, 5)})

    def test_flange_names(self):
        flange = self.add_end_flange(self.add_base())
        name = flange.Name
        edges = self.names(flange, "Edge")
        for role in ("End", "Side1", "Side2"):
            for side in ("T", "B"):
                self.assertIn(f"{name}_{role}_{side}", edges)
        faces = self.names(flange, "Face")
        self.assertTrue({"Base_Top", "Base_Bottom", f"{name}_Top", f"{name}_Bottom"} <= faces)
        # The names of the base panel stay the same after the flange.
        self.assertTrue({"Base_E1_T", "Base_E3_T", "Base_E4_T"} <= edges)

    def test_flat_pattern_names(self):
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        edges = self.names(flat, "Edge")
        self.assertIn("Base_E1_F", edges)
        self.assertIn(f"{flange.Name}_End_F", edges)
        self.assertIn(f"{flange.Name}_BendLine", edges)
        # The bend line name is on the bend line: the one edge that is not on the face.
        reverse = {v: k for k, v in flat.Shape.ElementReverseMap.items()}
        line = flat.Shape.getElement(reverse[f"{flange.Name}_BendLine"])
        self.assertAlmostEqual(line.CenterOfMass.x, self.LENGTH + self.allowance() / 2, places=6)

    def test_refold_and_unfold_names(self):
        allowance = self.allowance()
        face = self.flat_rectangle(self.LENGTH + allowance + 20.0)
        x = self.LENGTH + allowance / 2
        folded = self.add_refold(face, [Part.makeLine(V(x, 0, 0), V(x, 50, 0))], [90.0])
        faces = self.names(folded, "Face")
        self.assertTrue({f"{folded.Name}_Root_Top", f"{folded.Name}_L1_Top"} <= faces)
        converted = self.add_unfold(self.part_bracket(), 1.0)
        faces = self.names(converted, "Face")
        self.assertTrue({f"{converted.Name}_P1_Top", f"{converted.Name}_P2_Top"} <= faces)

    def box_from_flanges(self):
        """A tube of four panels with a flap on each panel, as a chain of flanges."""
        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        t, length, width, depth = 4.0, 300.0, 200.0, 150.0
        wire = Part.makePolygon(
            [V(0, 0, 0), V(length, 0, 0), V(length, depth, 0), V(0, depth, 0), V(0, 0, 0)]
        )
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(wire)
        base.Rule = self.rule
        self.doc.recompute()
        top = t + width
        steps = [
            (V(length, 0, t), V(length, depth, t), width, 0.0),
            (V(length, 0, top), V(length, depth, top), length, 0.0),
            (V(0, 0, top), V(0, depth, top), width, 0.0),
            (V(0, 0, t), V(length, 0, t), width / 2, t),  # Outer flap.
            (V(0, 0, top), V(length, 0, top), width / 2, t),  # Outer flap.
            (V(length, 0, t), V(length, 0, top), width / 2, 0.0),  # Inner flap.
            (V(0, 0, t), V(0, 0, top), width / 2, 0.0),  # Inner flap.
        ]
        features = [base]
        for start, end, flange_length, offset in steps:
            features.append(
                self.add_flange(features[-1], start, end, Length=flange_length, Offset=offset)
            )
        return features

    def referenced_names(self, features):
        return [
            f.Edge[0].Shape.ElementReverseMap[f.Edge[1][0]]
            for f in features
            if f.TypeId == "SheetDesign::Flange"
        ]

    def test_references_follow_a_change_of_the_topology(self):
        """An offset on an early flange adds edges. The later flanges must keep their edges."""
        features = self.box_from_flanges()
        last = features[-1]
        self.assertSolid(last)
        self.assertEqual(last.Overlap.Value, 0.0)
        before = self.referenced_names(features)
        indexes = [f.Edge[1][0] for f in features[1:]]

        for flap in (features[4], features[5]):
            flap.Offset = 0.0
        self.doc.recompute()
        for feature in features:
            self.assertTrue(feature.isValid(), feature.Name + ": " + feature.getStatusString())
        self.assertEqual(self.referenced_names(features), before)
        # The test is of use only if the change moved the number of an edge that is in use.
        self.assertNotEqual([f.Edge[1][0] for f in features[1:]], indexes)
        # The flaps are now in one layer: two inner flaps, each across two outer flaps.
        self.assertSolid(last)
        self.assertAlmostEqual(last.Overlap.Value, 160000.0, places=1)
        self.assertAlmostEqual(last.Shape.BoundBox.YLength, 150.0 + 4.0, places=6)

    def test_references_follow_after_document_restore(self):
        features = self.box_from_flanges()
        names = [f.Name for f in features]
        before = self.referenced_names(features)
        path = os.path.join(tempfile.mkdtemp(), "box.FCStd")
        self.doc.saveAs(path)
        FreeCAD.closeDocument(self.doc.Name)
        self.doc = FreeCAD.openDocument(path)
        features = [self.doc.getObject(name) for name in names]
        for flap in (features[4], features[5]):
            flap.Offset = 0.0
        self.doc.recompute()
        for feature in features:
            self.assertTrue(feature.isValid(), feature.Name + ": " + feature.getStatusString())
        self.assertEqual(self.referenced_names(features), before)

    # ------------------------------------------------------------- bend zones of any shape

    def rectangle(self, x0, y0, x1, y1):
        return self.flat_rectangle_at(x0, y0, x1, y1)

    def check_cut_across_bend(self, tool):
        """With the K-factor 0.5, the volume of a cut is its area multiplied by the thickness."""
        flange = self.add_end_flange(self.add_base())
        cut = self.add_cut(flange, self.add_shape(tool, "Tool"), "Flat")
        self.assertSolid(cut)
        self.assertEqual(cut.Overlap.Value, 0.0)
        flat = self.add_flat(cut)
        face = flat.Shape.Faces[0]
        self.assertAlmostEqual(self.add_flat(flange).Shape.Faces[0].Area - face.Area, tool.Area, places=5)
        return flange, cut

    def test_slot_across_bend(self):
        x = self.LENGTH + self.allowance() / 2
        flange, cut = self.check_cut_across_bend(self.rectangle(x - 6, 20, x + 6, 30))
        self.assertAlmostEqual(flange.Shape.Volume - cut.Shape.Volume, 120.0 * self.T, places=4)
        # The slot divides the bend into two solids, and the part is still one solid.
        cylinders = [f for f in cut.Shape.Faces if f.Surface.TypeId == "Part::GeomCylinder"]
        self.assertEqual(len(cylinders), 4)

    def test_slot_inside_bend_zone(self):
        x = self.LENGTH + self.allowance() / 2
        quarter = self.allowance() / 4
        flange, cut = self.check_cut_across_bend(self.rectangle(x - quarter, 20, x + quarter, 30))
        removed = 2 * quarter * 10.0 * self.T
        self.assertAlmostEqual(flange.Shape.Volume - cut.Shape.Volume, removed, places=4)

    def test_skew_slot_across_bend(self):
        x = self.LENGTH + self.allowance() / 2
        tool = Part.Face(
            Part.makePolygon(
                [V(x - 8, 15, 0), V(x + 2, 15, 0), V(x + 8, 35, 0), V(x - 2, 35, 0), V(x - 8, 15, 0)]
            )
        )
        flange, cut = self.check_cut_across_bend(tool)
        self.assertAlmostEqual(flange.Shape.Volume - cut.Shape.Volume, tool.Area * self.T, places=3)

    def test_round_hole_across_bend(self):
        x = self.LENGTH + self.allowance() / 2
        tool = Part.Face(Part.Wire(Part.makeCircle(4.0, V(x, 25, 0))))
        flange, cut = self.check_cut_across_bend(tool)
        # The standard volume of a shape with a free-form face is not accurate, thus use a
        # wide limit here. The Overlap check above uses an accurate volume.
        self.assertAlmostEqual(
            flange.Shape.Volume - cut.Shape.Volume, tool.Area * self.T, delta=0.05 * tool.Area * self.T
        )

    def test_cut_that_removes_a_bend_is_rejected(self):
        flange = self.add_end_flange(self.add_base())
        x = self.LENGTH + self.allowance() / 2
        cut = self.add_cut(
            flange, self.add_shape(self.rectangle(x - 6, -5, x + 6, self.WIDTH + 5), "Tool"), "Flat"
        )
        self.assertFalse(cut.isValid())

    def test_slot_across_bend_with_radius_zero(self):
        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        base = self.add_base()
        flange = self.add_flange(base, V(self.LENGTH, 0, 4), V(self.LENGTH, self.WIDTH, 4), Length=20.0)
        x = self.LENGTH + 0.5 * 4.0 * math.pi / 4
        cut = self.add_cut(flange, self.add_shape(self.rectangle(x - 6, 20, x + 6, 30), "Tool"), "Flat")
        self.assertSolid(cut)
        self.assertEqual(cut.Overlap.Value, 0.0)
        self.assertAlmostEqual(flange.Shape.Volume - cut.Shape.Volume, 120.0 * 4.0, places=4)

    def tapered_flap(self):
        a = self.allowance()
        faces = [
            self.rectangle(0, 0, 200, 100),
            Part.Face(
                Part.makePolygon(
                    [V(0, 100, 0), V(200, 100, 0), V(170, 100 + a + 50, 0), V(30, 100 + a + 50, 0), V(0, 100, 0)]
                )
            ),
        ]
        face = faces[0].fuse(faces[1:]).removeSplitter().Faces[0]
        return face, [Part.makeLine(V(0, 100 + a / 2, 0), V(200, 100 + a / 2, 0))]

    def test_refold_flap_that_tapers_from_the_bend_line(self):
        face, lines = self.tapered_flap()
        folded = self.add_refold(face, lines, [90.0])
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)
        self.assertAlmostEqual(folded.Shape.Volume, face.Area * self.T, places=3)
        flat = self.add_flat(folded)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, face.Area, places=5)

    def test_refold_tapered_flap_with_radius_zero(self):
        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        face, lines = self.tapered_flap()
        folded = self.add_refold(face, lines, [90.0])
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)

    def test_refold_with_hole_on_the_bend_line(self):
        allowance = self.allowance()
        face = self.flat_rectangle(self.LENGTH + allowance + 20.0)
        x = self.LENGTH + allowance / 2
        face = face.cut(self.rectangle(x - 6, 20, x + 6, 30)).Faces[0]
        folded = self.add_refold(face, [Part.makeLine(V(x, 0, 0), V(x, self.WIDTH, 0))], [90.0])
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)
        self.assertAlmostEqual(folded.Shape.Volume, face.Area * self.T, places=4)

    def test_unfold_of_part_with_cut_across_bend(self):
        x = self.LENGTH + self.allowance() / 2
        tools = Part.makeCompound(
            [self.rectangle(x - 6, 20, x + 6, 30), self.rectangle(x - 8, -1, x + 8, 6)]
        )
        flange = self.add_end_flange(self.add_base())
        cut = self.add_cut(flange, self.add_shape(tools, "Tools"), "Flat")
        self.assertSolid(cut)
        expected = self.add_flat(cut).Shape.Faces[0].Area
        converted = self.add_unfold(cut.Shape.copy(), 1.0)
        self.assertSolid(converted)
        self.assertEqual(converted.Overlap.Value, 0.0)
        self.assertAlmostEqual(converted.Shape.Volume, cut.Shape.Volume, places=3)
        flat = self.add_flat(converted)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    def test_unfold_of_part_with_round_hole_across_bend(self):
        x = self.LENGTH + self.allowance() / 2
        tool = Part.Face(Part.Wire(Part.makeCircle(4.0, V(x, 25, 0))))
        flange = self.add_end_flange(self.add_base())
        cut = self.add_cut(flange, self.add_shape(tool, "Tool"), "Flat")
        expected = self.add_flat(cut).Shape.Faces[0].Area
        converted = self.add_unfold(cut.Shape.copy(), 1.0)
        self.assertSolid(converted)
        flat = self.add_flat(converted)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, delta=1e-3 * expected)

    # ---------------------------------------------------------------------- PartDesign Body

    def body_with_base(self):
        import Sketcher

        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        corners = [(0, 0), (self.LENGTH, 0), (self.LENGTH, self.WIDTH), (0, self.WIDTH)]
        for i in range(4):
            sketch.addGeometry(Part.LineSegment(V(*corners[i], 0), V(*corners[(i + 1) % 4], 0)))
        for i in range(4):
            sketch.addConstraint(Sketcher.Constraint("Coincident", i, 2, (i + 1) % 4, 1))
        base = body.newObject("SheetDesign::BasePanel", "Base")
        base.Profile = sketch
        base.Rule = self.rule
        self.doc.recompute()
        return body, base

    def body_flange(self, body, source, start, end, **properties):
        flange = body.newObject("SheetDesign::Flange", "Flange")
        flange.Edge = (source, [find_edge(source.Shape, start, end)])
        for name, value in properties.items():
            setattr(flange, name, value)
        self.doc.recompute()
        return flange

    def test_features_in_a_body(self):
        body, base = self.body_with_base()
        self.assertSolid(base)
        self.assertEqual(body.Tip, base)
        flange = self.body_flange(
            body, base, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0
        )
        self.assertSolid(flange)
        self.assertEqual(body.Tip, flange)
        self.assertEqual(flange.BaseFeature, base)
        self.assertTrue(body.isValid(), body.getStatusString())
        self.assertAlmostEqual(body.Shape.Volume, flange.Shape.Volume, places=6)
        self.assertEqual(flange.Overlap.Value, 0.0)

    def test_flat_pattern_of_a_part_in_a_body(self):
        body, base = self.body_with_base()
        flange = self.body_flange(
            body, base, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0
        )
        flat = self.add_flat(flange)
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertAlmostEqual(flat.Shape.BoundBox.XMin, 0.0, places=6)
        # The flat pattern is in the coordinates of the body. It stays valid after a move.
        body.Placement = FreeCAD.Placement(V(500, 0, 30), FreeCAD.Rotation())
        flange.Length = 25.0
        self.doc.recompute()
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, self.LENGTH + self.allowance() + 25.0, places=6
        )

    def test_profile_outside_a_moved_body(self):
        """A profile at document level is in global coordinates. The body can be at any place."""
        body = self.doc.addObject("PartDesign::Body", "Body")
        body.Placement = FreeCAD.Placement(V(0, 0, 100), FreeCAD.Rotation())
        wire = Part.makePolygon([V(0, 0, 0), V(40, 0, 0), V(40, 30, 0), V(0, 30, 0), V(0, 0, 0)])
        base = body.newObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(wire)
        base.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(base)
        # In global coordinates the part is at the place of its profile.
        self.assertAlmostEqual(body.Shape.BoundBox.ZMin, 0.0, places=6)

    def pocket_after(self, body):
        sketch = body.newObject("Sketcher::SketchObject", "PocketSketch")
        sketch.addGeometry(Part.Circle(V(40, 25, 0), V(0, 0, 1), 6))
        pocket = body.newObject("PartDesign::Pocket", "Pocket")
        pocket.Profile = sketch
        pocket.Type = "ThroughAll"
        pocket.Midplane = True
        self.doc.recompute()
        return pocket

    def test_partdesign_feature_after_a_sheet_feature(self):
        body, base = self.body_with_base()
        flange = self.body_flange(
            body, base, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0
        )
        pocket = self.pocket_after(body)
        self.assertTrue(pocket.isValid(), pocket.getStatusString())
        self.assertAlmostEqual(
            flange.Shape.Volume - pocket.Shape.Volume, math.pi * 36.0 * self.T, places=4
        )
        self.assertEqual(body.Tip, pocket)

    def test_sheet_feature_after_a_partdesign_feature_needs_unfold(self):
        body, base = self.body_with_base()
        flange = self.body_flange(
            body, base, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0
        )
        pocket = self.pocket_after(body)
        # A flange cannot continue the pocket: the pocket has a solid, but no sheet model.
        refused = self.body_flange(
            body, pocket, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=10.0
        )
        self.assertFalse(refused.isValid())
        self.assertIn("Unfold", refused.getStatusString())
        self.doc.removeObject(refused.Name)

        # Unfold reads the solid of the pocket and gives a model again.
        top = None
        for index, face in enumerate(pocket.Shape.Faces):
            if face.Surface.TypeId == "Part::GeomPlane" and face.normalAt(0, 0).z > 0.99:
                if abs(face.CenterOfMass.z - self.T) < 1e-6:
                    top = f"Face{index + 1}"
        unfold = body.newObject("SheetDesign::Unfold", "Unfold")
        unfold.Face = (pocket, [top])
        unfold.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(unfold)
        self.assertEqual(body.Tip, unfold)
        self.assertAlmostEqual(unfold.Shape.Volume, pocket.Shape.Volume, places=4)
        flat = self.add_flat(unfold)
        face = flat.Shape.Faces[0]
        self.assertEqual(len(face.Wires), 2)
        expected = (self.LENGTH + self.allowance() + 20.0) * self.WIDTH - math.pi * 36.0
        self.assertAlmostEqual(face.Area, expected, places=5)
        # A flange can follow again.
        more = self.body_flange(body, unfold, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=10.0)
        self.assertSolid(more)

    def test_suppressed_feature_gives_the_model_before_it(self):
        body, base = self.body_with_base()
        first = self.body_flange(
            body, base, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0
        )
        second = self.body_flange(
            body, first, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=10.0
        )
        both = second.Shape.Volume
        first.Suppressed = True
        self.doc.recompute()
        self.assertSolid(second)
        expected = (self.LENGTH + self.allowance() + 10.0) * self.WIDTH * self.T
        self.assertAlmostEqual(second.Shape.Volume, expected, places=4)
        first.Suppressed = False
        self.doc.recompute()
        self.assertAlmostEqual(second.Shape.Volume, both, places=6)

    # ------------------------------------------------- radius zero, and folds that close a loop

    def board(self):
        """Corrugated board: thickness 4 mm, the radius of a score is zero."""
        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        return 0.5 * 4.0 * math.pi / 2

    def test_refold_round_flap_with_radius_zero(self):
        """Box style 0427: the arcs of the flap start on the score."""
        a = self.board()
        arc1 = Part.Arc(
            V(200, 100, 0), V(200 - 40 * (1 - math.cos(math.pi / 4)), 100 + 40 * math.sin(math.pi / 4), 0), V(160, 140, 0)
        ).toShape()
        arc2 = Part.Arc(
            V(40, 140, 0), V(40 * (1 - math.cos(math.pi / 4)), 100 + 40 * math.sin(math.pi / 4), 0), V(0, 100, 0)
        ).toShape()
        flap = Part.Face(
            Part.Wire(
                [Part.makeLine(V(0, 100, 0), V(200, 100, 0)), arc1, Part.makeLine(V(160, 140, 0), V(40, 140, 0)), arc2]
            )
        )
        face = self.rectangle(0, 0, 200, 100).fuse(flap).removeSplitter().Faces[0]
        folded = self.add_refold(face, [Part.makeLine(V(0, 100 + a / 2, 0), V(200, 100 + a / 2, 0))], [90.0])
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)
        self.assertAlmostEqual(self.add_flat(folded).Shape.Faces[0].Area, face.Area, places=5)

    def test_round_hole_in_a_bend_with_radius_zero_is_rejected(self):
        a = self.board()
        base = self.add_base()
        flange = self.add_flange(base, V(self.LENGTH, 0, 4), V(self.LENGTH, self.WIDTH, 4), Length=20.0)
        tool = Part.Face(Part.Wire(Part.makeCircle(1.0, V(self.LENGTH + a / 2, 25, 0))))
        cut = self.add_cut(flange, self.add_shape(tool, "Tool"), "Flat")
        self.assertFalse(cut.isValid())
        self.assertIn("radius zero", cut.getStatusString())

    def test_refold_line_with_angle_zero_stays_flat(self):
        allowance = self.allowance()
        face = self.flat_rectangle(self.LENGTH + allowance + 20.0)
        x = self.LENGTH + allowance / 2
        lines = [Part.makeLine(V(50, 0, 0), V(50, self.WIDTH, 0)), Part.makeLine(V(x, 0, 0), V(x, self.WIDTH, 0))]
        folded = self.add_refold(face, lines, [0.0, 90.0])
        self.assertSolid(folded)
        self.assertAlmostEqual(folded.Shape.BoundBox.ZMax, self.T + self.R + 20.0, places=6)
        self.assertEqual(len(self.add_flat(folded).Shape.SubShapes), 2)  # The face and one bend line.

    def web_corner(self, angles):
        """A tray with two walls and a corner web. The web folds flat on wall 1 in two layers.

        Wall 2 is shorter by two thicknesses, thus the two layers of the web have their space.
        """
        a = self.board()
        t, length, width, depth = 4.0, 120.0, 80.0, 40.0
        face = self.rectangle(0, 0, length + a + depth, width + a + depth)
        face = face.cut(self.rectangle(length - 2 * t, width, length + a, width + a)).Faces[0]
        x5 = length - 2 * t + a / 2
        lines = [
            Part.makeLine(V(length + a / 2, 0, 0), V(length + a / 2, width, 0)),  # Bottom, wall 1.
            Part.makeLine(V(0, width + a / 2, 0), V(length - 2 * t, width + a / 2, 0)),  # Wall 2.
            Part.makeLine(V(length + a, width + a, 0), V(length + a + depth, width + a, 0)),  # Web.
            Part.makeLine(V(length + a, width + 2 * a, 0), V(length + depth - a, width + depth, 0)),
            Part.makeLine(V(x5, width + a, 0), V(x5, width + a + depth, 0)),  # Web to wall 2.
        ]
        return face, self.add_refold(face, lines, angles)

    def test_refold_corner_web_closes_a_loop_of_panels(self):
        """Box styles 0226 and 0760: two walls and the two halves of a web meet at a corner."""
        face, folded = self.web_corner([90.0, 90.0, 180.0, 180.0, 90.0])
        self.assertSolid(folded)
        self.assertEqual(folded.Overlap.Value, 0.0)
        box = folded.Shape.BoundBox
        self.assertAlmostEqual(box.XLength, 120.0 + 4.0, places=4)
        self.assertAlmostEqual(box.ZLength, 40.0 + 4.0, places=4)
        # The web is inside the tray: nothing is outside wall 2.
        self.assertLess(box.YLength, 80.0 + 4.0 + 0.5)
        flat = self.add_flat(folded)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, face.Area, places=5)
        self.assertEqual(len(flat.Shape.SubShapes), 6)  # The face and five bend lines.

    def test_refold_corner_web_with_wrong_angle_is_rejected(self):
        _face, folded = self.web_corner([90.0, 90.0, 180.0, 180.0, 45.0])
        self.assertFalse(folded.isValid())
        self.assertIn("do not close", folded.getStatusString())

    # ------------------------------------------------------- flange position and length types

    def test_flange_position(self):
        """Where the flange is against the selected edge, for a bend of 90 degrees."""
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        expected = {
            "Material Outside": (self.LENGTH + self.R + self.T, self.LENGTH),
            "Material Inside": (self.LENGTH, self.LENGTH - self.R - self.T),
            "Thickness Outside": (self.LENGTH + self.T, self.LENGTH - self.R),
        }
        for position, (outer_face, bend_start) in expected.items():
            with self.subTest(position=position):
                flange.Position = position
                self.doc.recompute()
                self.assertSolid(flange)
                self.assertEqual(flange.Overlap.Value, 0.0)
                self.assertAlmostEqual(flange.Shape.BoundBox.XMax, outer_face, places=6)
                self.assertAlmostEqual(
                    flat.Shape.BoundBox.XLength, bend_start + self.allowance() + 20.0, places=6
                )

    def test_flange_position_adds_to_offset(self):
        flange = self.add_end_flange(self.add_base(), Position="Material Inside", Offset=5.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH + 5.0, places=6)

    def test_flange_position_inside_with_gaps(self):
        """The bend line moves into the panel only where the flange is."""
        flange = self.add_end_flange(self.add_base(), Position="Material Inside", Gap1=10.0, Gap2=10.0)
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH, places=6)
        flat = self.add_flat(flange)
        notch = (self.R + self.T) * (self.WIDTH - 20.0)
        expected = self.LENGTH * self.WIDTH - notch + (self.allowance() + 20.0) * (self.WIDTH - 20.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    def test_flange_length_types_at_90_degrees(self):
        """The height of the flange above the bottom face of the base, for Length = 20."""
        flange = self.add_end_flange(self.add_base())
        expected = {
            "Leg": self.T + self.R + 20.0,
            "Outer Sharp": 20.0,
            "Tangential": 20.0,
            "Inner Sharp": 20.0 + self.T,
        }
        for length_type, height in expected.items():
            with self.subTest(length_type=length_type):
                flange.LengthType = length_type
                self.doc.recompute()
                self.assertSolid(flange)
                self.assertAlmostEqual(flange.Shape.BoundBox.ZMax, height, places=6)

    def test_flange_length_types_at_60_degrees(self):
        flange = self.add_end_flange(self.add_base(), Angle=60.0)
        flat = self.add_flat(flange)
        half = math.tan(math.radians(30.0))
        expected = {
            "Leg": 20.0,
            "Outer Sharp": 20.0 - (self.R + self.T) * half,
            "Inner Sharp": 20.0 - self.R * half,
            "Tangential": 20.0 - (self.R + self.T) * half,
        }
        for length_type, leg in expected.items():
            with self.subTest(length_type=length_type):
                flange.LengthType = length_type
                self.doc.recompute()
                self.assertSolid(flange)
                self.assertAlmostEqual(
                    flat.Shape.BoundBox.XLength, self.LENGTH + self.allowance(60.0) + leg, places=6
                )
        # Outer Sharp: the tip of the outer face is at the length from the outer sharp corner.
        flange.LengthType = "Outer Sharp"
        self.doc.recompute()
        corner = V(self.LENGTH + (self.R + self.T) * half, 0, 0)
        tip = corner + V(math.cos(math.radians(60.0)), 0, math.sin(math.radians(60.0))) * 20.0
        distance = min(v.Point.distanceToPoint(V(tip.x, v.Point.y, tip.z)) for v in flange.Shape.Vertexes)
        self.assertAlmostEqual(distance, 0.0, places=6)

    def test_flange_length_too_small_for_its_type_is_rejected(self):
        flange = self.add_end_flange(self.add_base(), Length=2.0, LengthType="Tangential")
        self.assertFalse(flange.isValid())
        self.assertIn("too small", flange.getStatusString())

    # ------------------------------------------------------------------------------ relief

    def relief_flange(self, **properties):
        properties.setdefault("Gap1", 10.0)
        properties.setdefault("Gap2", 10.0)
        properties.setdefault("ReliefWidth", 2.0)
        properties.setdefault("ReliefDepth", 3.0)
        flange = self.add_end_flange(self.add_base(), **properties)
        return flange, self.add_flat(flange)

    def flange_area(self, gaps=20.0):
        return self.LENGTH * self.WIDTH + (self.allowance() + 20.0) * (self.WIDTH - gaps)

    def test_relief_rectangle(self):
        flange, flat = self.relief_flange(ReliefType="Rectangle")
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * 2.0 * 3.0, places=5)
        # The slots are beside the bend, in the gap.
        box = flange.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH + self.R + self.T, places=6)

    def test_relief_round(self):
        flange, flat = self.relief_flange(ReliefType="Round")
        self.assertSolid(flange)
        slot = 2.0 * (3.0 - 1.0) + math.pi * 1.0**2 / 2
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * slot, places=5)

    def test_relief_comes_from_the_rule_by_default(self):
        """A rule with no relief values gives no relief."""
        flange, flat = self.relief_flange()
        self.assertEqual(flange.ReliefType, "Rule")
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area(), places=5)

    def add_rule_relief(self, relief_type, width, depth):
        self.rule.addProperty("App::PropertyEnumeration", "ReliefType", "Sheet")
        self.rule.addProperty("App::PropertyLength", "ReliefWidth", "Sheet")
        self.rule.addProperty("App::PropertyLength", "ReliefDepth", "Sheet")
        self.rule.ReliefType = ["None", "Rectangle", "Round"]
        self.rule.ReliefType = relief_type
        self.rule.ReliefWidth = width
        self.rule.ReliefDepth = depth

    def test_relief_of_the_rule_applies_to_each_flange(self):
        self.add_rule_relief("Rectangle", 1.5, 4.0)
        # The values of the flange are not used with the type Rule.
        flange, flat = self.relief_flange(ReliefWidth=9.0, ReliefDepth=9.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * 1.5 * 4.0, places=5)
        # A change of the rule changes the part.
        self.rule.ReliefDepth = 6.0
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * 1.5 * 6.0, places=5)
        self.rule.ReliefType = "None"
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area(), places=5)

    def test_flange_can_replace_the_relief_of_the_rule(self):
        self.add_rule_relief("Rectangle", 1.5, 4.0)
        flange, flat = self.relief_flange(ReliefType="None")
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area(), places=5)
        flange.ReliefType = "Rectangle"
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * 2.0 * 3.0, places=5)

    def test_relief_only_at_an_end_with_a_gap(self):
        flange, flat = self.relief_flange(ReliefType="Rectangle", Gap2=0.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area(10.0) - 2.0 * 3.0, places=5)

    def test_relief_width_is_not_larger_than_the_gap(self):
        flange, flat = self.relief_flange(ReliefType="Rectangle", ReliefWidth=25.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - 2 * 10.0 * 3.0, places=5)

    def test_relief_with_material_inside(self):
        """The bend line is inside the panel. The slot goes from its depth out to the edge."""
        flange, flat = self.relief_flange(ReliefType="Rectangle", Position="Material Inside")
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        inside = self.R + self.T
        notch = inside * (self.WIDTH - 20.0)
        slots = 2 * 2.0 * (inside + 3.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.flange_area() - notch - slots, places=5)

    def test_relief_with_a_size_of_zero_is_rejected(self):
        flange, _flat = self.relief_flange(ReliefType="Rectangle", ReliefDepth=0.0)
        self.assertFalse(flange.isValid())

    def test_flange_after_a_relief(self):
        """A later flange on the same panel keeps its edge."""
        flange, _flat = self.relief_flange(ReliefType="Rectangle")
        second = self.add_flange(flange, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=10.0)
        self.assertSolid(second)
        flange.ReliefDepth = 6.0
        flange.ReliefType = "Round"
        self.doc.recompute()
        self.assertSolid(second)
        self.assertAlmostEqual(second.Shape.BoundBox.XMin, -(self.R + self.T), places=6)

    # ------------------------------------------------------------------------------- miter

    def test_miter_makes_the_flange_narrower(self):
        flange = self.add_end_flange(self.add_base(), Length=10.0, Miter1=45.0, Miter2=45.0)
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        flat = self.add_flat(flange)
        depth = self.allowance() + 10.0
        # The sides start at the bend line: the width at the depth u is WIDTH - 2 u.
        expected = self.LENGTH * self.WIDTH + depth * self.WIDTH - depth**2
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)
        # The far end of the flange is shorter than the edge.
        top = self.T + self.R + 10.0
        ends = [v.Point.y for v in flange.Shape.Vertexes if abs(v.Point.z - top) < 1e-6]
        self.assertAlmostEqual(max(ends) - min(ends), self.WIDTH - 2 * depth, places=5)

    def test_miter_on_one_side(self):
        flange = self.add_end_flange(self.add_base(), Length=10.0, Miter2=30.0)
        self.assertSolid(flange)
        flat = self.add_flat(flange)
        depth = self.allowance() + 10.0
        expected = self.LENGTH * self.WIDTH + depth * self.WIDTH - depth**2 * math.tan(math.radians(30)) / 2
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    def test_miter_that_leaves_no_width_is_rejected(self):
        flange = self.add_end_flange(self.add_base(), Length=30.0, Miter1=45.0, Miter2=45.0)
        self.assertFalse(flange.isValid())

    def two_flanges_at_inner_corner(self, **properties):
        """An L-shaped panel with a flange on each of the two edges of its inner corner.

        In the flat pattern the two flanges go into the same region.
        """
        wire = Part.makePolygon(
            [V(0, 0, 0), V(100, 0, 0), V(100, 30, 0), V(60, 30, 0), V(60, 50, 0), V(0, 50, 0), V(0, 0, 0)]
        )
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(wire)
        base.Rule = self.rule
        self.doc.recompute()
        first = self.add_flange(base, V(60, 30, self.T), V(100, 30, self.T), Length=10.0)
        # The first bend divides the top edge of the second side. The bottom edge is one
        # edge, thus use it, and turn the flange to the top side.
        second = self.add_flange(
            first, V(60, 30, 0), V(60, 50, 0), Length=10.0, Reversed=True, **properties
        )
        return first, second

    def test_automatic_miter_of_two_flanges_at_an_inner_corner(self):
        first, second = self.two_flanges_at_inner_corner()
        self.assertSolid(second)
        self.assertEqual(second.Overlap.Value, 0.0)
        flat = self.add_flat(second)
        self.assertEqual(len(flat.Shape.Faces), 1)
        depth = self.allowance() + 10.0
        panel = 100 * 30 + 60 * 20
        # Each flange loses the half of the square that the two had together.
        expected = panel + (40 * depth - depth**2 / 2) + (20 * depth - depth**2 / 2)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=4)
        self.assertEqual(len(flat.Shape.SubShapes), 3)  # The face and two bend lines.

    def test_automatic_miter_can_be_set_to_off(self):
        _first, second = self.two_flanges_at_inner_corner(AutoMiter=False)
        self.assertTrue(second.isValid(), second.getStatusString())
        # The two flanges are in the same space at the corner.
        self.assertGreater(second.Overlap.Value, 0.0)

    def test_miter_gap(self):
        _first, second = self.two_flanges_at_inner_corner()
        flat = self.add_flat(second)
        closed = flat.Shape.Faces[0].Area
        second.MiterGap = 1.0
        self.doc.recompute()
        self.assertSolid(second)
        self.assertEqual(second.Overlap.Value, 0.0)
        # The gap is a band of 1 mm along the seam of the two flanges.
        seam = (self.allowance() + 10.0) * math.sqrt(2)
        self.assertAlmostEqual(closed - flat.Shape.Faces[0].Area, seam * 1.0, delta=1.0)

    def test_no_miter_for_flanges_that_do_not_overlap(self):
        """Two flanges at an outer corner are apart in the flat pattern. They stay rectangles."""
        first = self.add_end_flange(self.add_base())
        second = self.add_flange(first, V(0, self.WIDTH, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0)
        self.assertSolid(second)
        flat = self.add_flat(second)
        depth = self.allowance() + 20.0
        expected = self.LENGTH * self.WIDTH + depth * self.WIDTH + depth * self.LENGTH
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    # ------------------------------------------------------------------ flange from a sketch

    def test_flange_with_a_shape_from_a_closed_profile(self):
        """The profile is in the plane of the panel. Its side on the edge gives the bend line."""
        outline = Part.Face(
            Part.makePolygon(
                [V(100, 10, 0), V(118, 18, 0), V(118, 32, 0), V(100, 40, 0), V(100, 10, 0)]
            )
        )
        hole = Part.Face(Part.Wire(Part.makeCircle(3.0, V(109, 25, 0))))
        profile = self.add_shape(outline.cut(hole), "Shape")
        flange = self.add_end_flange(self.add_base(), Profile=profile)
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        # The flange is as high as the profile is deep.
        self.assertAlmostEqual(flange.Shape.BoundBox.ZMax, self.T + self.R + 18.0, places=6)
        flat = self.add_flat(flange)
        face = flat.Shape.Faces[0]
        expected = self.LENGTH * self.WIDTH + self.allowance() * 30.0 + outline.Area - hole.Area
        self.assertAlmostEqual(face.Area, expected, places=5)
        self.assertEqual(len(face.Wires), 2)
        # The bend line is as long as the side of the profile on the edge.
        line = flat.Shape.SubShapes[1]
        self.assertAlmostEqual(line.Length, 30.0, places=6)

    def test_flange_profile_that_is_not_on_the_edge_is_rejected(self):
        outline = Part.Face(
            Part.makePolygon([V(105, 10, 0), V(118, 10, 0), V(118, 40, 0), V(105, 40, 0), V(105, 10, 0)])
        )
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(outline, "Shape"))
        self.assertFalse(flange.isValid())
        self.assertIn("on the selected edge", flange.getStatusString())

    def test_contour_from_an_open_profile(self):
        """A Z section: up by 90 degrees, then to the outer side by 90 degrees.

        The line is the outline of the top side of the sheet with sharp corners.
        """
        t, r = self.T, self.R
        section = Part.makePolygon([V(100, 25, t), V(100, 25, 22), V(115, 25, 22)])
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(section, "Section"))
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        box = flange.Shape.BoundBox
        # The top side of the sheet is on the line.
        self.assertAlmostEqual(box.ZMax, 22.0, places=6)
        self.assertAlmostEqual(box.XMax, 115.0, places=6)
        flat = self.add_flat(flange)
        self.assertEqual(len(flat.Shape.SubShapes), 3)  # The face and two bend lines.
        # First bend: the top side is the inner side. Second bend: it is the outer side.
        leg1 = 20.0 - r - (r + t)
        leg2 = 15.0 - (r + t)
        length = (self.LENGTH - r) + self.allowance() + leg1 + self.allowance() + leg2
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)
        # The inner face of the first panel is on the first segment of the line.
        faces = [
            f for f in flange.Shape.Faces
            if f.Surface.TypeId == "Part::GeomPlane" and abs(f.CenterOfMass.x - 100.0) < 1e-6
            and abs(abs(f.normalAt(0, 0).x) - 1.0) < 1e-6
        ]
        self.assertEqual(len(faces), 1)

    def test_contour_from_the_bottom_side(self):
        """A section that starts on the bottom side and goes down."""
        section = Part.makePolygon([V(100, 25, 0), V(100, 25, -18)])
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(section, "Section"))
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.ZMin, -18.0, places=6)
        # The bottom side is the inner side of this bend: the outer face is one thickness out.
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, 100.0 + self.T, places=6)

    def test_contour_with_an_angle_that_is_not_90_degrees(self):
        angle = math.radians(60.0)
        end = V(100 + 20 * math.cos(angle), 25, self.T + 20 * math.sin(angle))
        section = Part.makePolygon([V(100, 25, self.T), end])
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(section, "Section"))
        self.assertSolid(flange)
        flat = self.add_flat(flange)
        corner = self.R * math.tan(angle / 2)
        length = (self.LENGTH - corner) + self.allowance(60.0) + (20.0 - corner)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)
        # The end of the line is on the top side of the flange.
        distance = min(v.Point.distanceToPoint(V(end.x, v.Point.y, end.z)) for v in flange.Shape.Vertexes)
        self.assertAlmostEqual(distance, 0.0, places=6)

    def test_contour_that_does_not_start_on_the_edge_is_rejected(self):
        section = Part.makePolygon([V(103, 25, self.T), V(103, 25, 22)])
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(section, "Section"))
        self.assertFalse(flange.isValid())
        self.assertIn("must start on the selected edge", flange.getStatusString())

    def test_contour_names_and_later_flange(self):
        """Each panel of a contour has its own name, and a flange can follow."""
        section = Part.makePolygon([V(100, 25, self.T), V(100, 25, 22), V(115, 25, 22)])
        flange = self.add_end_flange(self.add_base(), Profile=self.add_shape(section, "Section"))
        faces = self.names(flange, "Face")
        self.assertTrue({f"{flange.Name}_Top", f"{flange.Name}_S2_Top"} <= faces)
        more = self.add_flange(flange, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=10.0)
        self.assertSolid(more)

    # ----------------------------------------------------------- bend data and file export

    def part_for_export(self):
        """A base with a hole, one flange up and one flange down."""
        base = self.add_base()
        up = self.add_end_flange(base)
        down = self.add_flange(up, V(0, 0, 0), V(0, self.WIDTH, 0), Length=15.0, Angle=45.0)
        hole = Part.Face(Part.Wire(Part.makeCircle(5.0, V(50, 25, 0))))
        cut = self.add_cut(down, self.add_shape(hole, "Hole"), "Flat")
        return self.add_flat(cut)

    def dxf_entities(self, text):
        """Return (type, layer, values) for each entity of a DXF text."""
        lines = text.splitlines()
        pairs = [(lines[i].strip(), lines[i + 1].strip()) for i in range(0, len(lines) - 1, 2)]
        start = pairs.index(("2", "ENTITIES"))
        entities = []
        for code, value in pairs[start + 1 :]:
            if code == "0":
                entities.append([value, None, {}])
            elif code == "8":
                entities[-1][1] = value
            else:
                entities[-1][2][code] = value
        return [tuple(entity) for entity in entities if entity[0] not in ("ENDSEC", "EOF")]

    def test_flat_pattern_gives_the_bend_data(self):
        flat = self.part_for_export()
        self.assertEqual([round(a, 6) for a in flat.BendAngles], [90.0, -45.0])
        self.assertEqual([round(r, 6) for r in flat.BendRadii], [self.R, self.R])

    def test_dxf_has_a_layer_for_each_kind_of_line(self):
        import SheetDesignExport

        flat = self.part_for_export()
        text = SheetDesignExport.dxf_text(flat)
        entities = self.dxf_entities(text)
        by_layer = {}
        for kind, layer, _values in entities:
            by_layer.setdefault(layer, []).append(kind)
        # The outline is four lines. The hole is one circle. Each bend is one line.
        self.assertEqual(sorted(by_layer["OUTLINE"]), ["LINE"] * 4)
        self.assertEqual(by_layer["CUTOUT"], ["CIRCLE"])
        self.assertEqual(by_layer["BEND_UP"], ["LINE"])
        self.assertEqual(by_layer["BEND_DOWN"], ["LINE"])
        self.assertEqual(sorted(by_layer["BEND_TEXT"]), ["TEXT", "TEXT"])
        texts = sorted(values["1"] for kind, _layer, values in entities if kind == "TEXT")
        self.assertEqual(texts, ["1 UP 90%%d R1", "2 DOWN 45%%d R1"])
        # The bend layers have the dashed line type.
        self.assertIn("BEND_UP\n70\n0\n62\n3\n6\nDASHED", text)
        self.assertIn("OUTLINE\n70\n0\n62\n7\n6\nCONTINUOUS", text)
        # The file is in the XY plane and starts at the left end of the down flange.
        xs = [float(v["10"]) for kind, layer, v in entities if layer == "OUTLINE"]
        self.assertAlmostEqual(min(xs), -(self.allowance(45.0) + 15.0), places=5)

    def test_dxf_without_labels(self):
        import SheetDesignExport

        text = SheetDesignExport.dxf_text(self.part_for_export(), labels=False)
        self.assertNotIn("TEXT", [kind for kind, _layer, _values in self.dxf_entities(text)])

    def test_dxf_arcs_and_curves(self):
        import SheetDesignExport

        outline = [
            Part.makeLine(V(0, 0, 0), V(80, 0, 0)),
            Part.Arc(V(80, 0, 0), V(80 + 20 * math.sin(math.pi / 4), 20 - 20 * math.cos(math.pi / 4), 0), V(100, 20, 0)).toShape(),
            Part.makeLine(V(100, 20, 0), V(100, 50, 0)),
            Part.makeLine(V(100, 50, 0), V(0, 50, 0)),
            Part.makeLine(V(0, 50, 0), V(0, 0, 0)),
        ]
        ellipse = Part.Wire(Part.Ellipse(V(40, 25, 0), 12.0, 6.0).toShape())
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(Part.makeCompound([Part.Wire(outline), ellipse]))
        base.Rule = self.rule
        self.doc.recompute()
        self.assertSolid(base)
        entities = self.dxf_entities(SheetDesignExport.dxf_text(self.add_flat(base)))
        arcs = [values for kind, layer, values in entities if kind == "ARC"]
        self.assertEqual(len(arcs), 1)
        # The arc is a quarter with its center at (80, 20).
        self.assertAlmostEqual(float(arcs[0]["10"]), 80.0, places=5)
        self.assertAlmostEqual(float(arcs[0]["40"]), 20.0, places=5)
        swept = (float(arcs[0]["51"]) - float(arcs[0]["50"])) % 360.0
        self.assertAlmostEqual(swept, 90.0, places=4)
        # An ellipse has no DXF R12 entity: it is a polyline on the layer of the openings.
        kinds = [kind for kind, layer, _values in entities if layer == "CUTOUT"]
        self.assertEqual(kinds[0], "POLYLINE")
        self.assertGreater(kinds.count("VERTEX"), 20)

    def test_svg_has_the_same_layers(self):
        import xml.dom.minidom

        import SheetDesignExport

        flat = self.part_for_export()
        text = SheetDesignExport.svg_text(flat)
        document = xml.dom.minidom.parseString(text)
        groups = {g.getAttribute("id"): g for g in document.getElementsByTagName("g")}
        self.assertEqual(sorted(groups), ["BEND_DOWN", "BEND_TEXT", "BEND_UP", "CUTOUT", "OUTLINE"])
        self.assertEqual(len(groups["OUTLINE"].getElementsByTagName("path")), 1)
        self.assertEqual(len(groups["CUTOUT"].getElementsByTagName("path")), 1)
        self.assertEqual(len(groups["BEND_UP"].getElementsByTagName("line")), 1)
        self.assertIn("stroke-dasharray", groups["BEND_UP"].toxml())
        self.assertNotIn("stroke-dasharray", groups["OUTLINE"].toxml())
        labels = sorted(node.firstChild.data for node in document.getElementsByTagName("text"))
        self.assertEqual(labels, ["1 UP 90° R1", "2 DOWN 45° R1"])
        # The size of the drawing is the size of the flat pattern, with a margin, in mm.
        svg = document.documentElement
        box = flat.Shape.BoundBox
        self.assertAlmostEqual(float(svg.getAttribute("width")[:-2]), box.XLength + 10.0, places=2)
        self.assertTrue(svg.getAttribute("width").endswith("mm"))

    def test_svg_arc_turns_in_the_direction_of_the_part(self):
        import re

        import SheetDesignExport

        outline = [
            Part.makeLine(V(0, 0, 0), V(80, 0, 0)),
            Part.Arc(V(80, 0, 0), V(80 + 20 * math.sin(math.pi / 4), 20 - 20 * math.cos(math.pi / 4), 0), V(100, 20, 0)).toShape(),
            Part.makeLine(V(100, 20, 0), V(100, 50, 0)),
            Part.makeLine(V(100, 50, 0), V(0, 50, 0)),
            Part.makeLine(V(0, 50, 0), V(0, 0, 0)),
        ]
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(Part.Wire(outline))
        base.Rule = self.rule
        self.doc.recompute()
        text = SheetDesignExport.svg_text(self.add_flat(base))
        found = re.search(r"([-\d.]+) ([-\d.]+) A [\d.]+ [\d.]+ 0 (\d) (\d) ([-\d.]+) ([-\d.]+)", text)
        self.assertIsNotNone(found)
        x0, y0, large, sweep, x1, y1 = (float(v) for v in found.groups())
        # Back to the coordinates of the part: Y up, with the margin of 5 mm.
        y0, y1 = 55.0 - y0, 55.0 - y1
        # The center of the corner is (80, 20). The direction of travel around it:
        turn = (x0 - 80.0) * (y1 - 20.0) - (y0 - 20.0) * (x1 - 80.0)
        self.assertEqual(large, 0)
        self.assertEqual(sweep, 0 if turn > 0 else 1)

    def test_export_selects_the_format_by_the_file_name(self):
        import SheetDesignExport

        flat = self.part_for_export()
        folder = tempfile.mkdtemp()
        count = len(self.doc.Objects)
        SheetDesignExport.export(flat, os.path.join(folder, "part.dxf"))
        SheetDesignExport.export(flat, os.path.join(folder, "part.SVG"))
        self.assertEqual(len(self.doc.Objects), count)
        with open(os.path.join(folder, "part.dxf"), encoding="utf-8") as handle:
            self.assertTrue(handle.read().endswith("EOF\n"))
        with open(os.path.join(folder, "part.SVG"), encoding="utf-8") as handle:
            self.assertIn("<svg", handle.read())
        with self.assertRaises(ValueError):
            SheetDesignExport.export(flat, os.path.join(folder, "part.pdf"))

    # --------------------------------------------------------------------- K-factor table

    def add_k_table(self, rows=((1, 0.38), (3, 0.43), (99, 0.5)), standard=None):
        """Put a K-factor table on the rule."""
        self.rule.addProperty("App::PropertyFloatList", "KFactorRatios", "Sheet")
        self.rule.addProperty("App::PropertyFloatList", "KFactorValues", "Sheet")
        self.rule.KFactorRatios = [float(ratio) for ratio, _factor in rows]
        self.rule.KFactorValues = [float(factor) for _ratio, factor in rows]
        if standard:
            self.rule.addProperty("App::PropertyEnumeration", "KFactorStandard", "Sheet")
            self.rule.KFactorStandard = ["ANSI", "DIN"]
            self.rule.KFactorStandard = standard

    def flat_length(self, factor, radius=None):
        radius = self.R if radius is None else radius
        return self.LENGTH + (radius + factor * self.T) * math.pi / 2 + 20.0

    def test_k_factor_table_by_the_ratio_of_radius_to_thickness(self):
        """The same values as the test of the SheetMetal addon: 1 -> 0.38, 3 -> 0.43, 99 -> 0.5."""
        self.add_k_table()
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        # Radius 1, thickness 2: the ratio is 0.5, thus the row of the ratio 1.
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.38), places=6)
        # Radius 5: the ratio is 2.5, thus the row of the ratio 3.
        self.rule.BendRadius = 5.0
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.43, 5.0), places=6)
        # Radius 8: the ratio is 4, thus the row of the ratio 99.
        self.rule.BendRadius = 8.0
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.5, 8.0), places=6)

    def test_k_factor_table_change_updates_the_part(self):
        self.add_k_table()
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.rule.KFactorValues = [0.30, 0.43, 0.5]
        self.doc.recompute()
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.30), places=6)

    def test_k_factor_table_in_the_din_convention(self):
        self.add_k_table(rows=((1, 0.76), (99, 1.0)), standard="DIN")
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.38), places=6)

    def test_single_k_factor_in_the_din_convention(self):
        self.rule.addProperty("App::PropertyEnumeration", "KFactorStandard", "Sheet")
        self.rule.KFactorStandard = ["ANSI", "DIN"]
        self.rule.KFactorStandard = "DIN"
        self.rule.KFactor = 0.8
        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(0.4), places=6)

    def test_k_factor_table_with_unequal_lists_is_rejected(self):
        self.add_k_table()
        self.rule.KFactorValues = [0.38, 0.43]
        base = self.add_base()
        self.assertFalse(base.isValid())
        self.assertIn("K-factor table", base.getStatusString())

    def test_empty_k_factor_table_uses_the_single_value(self):
        self.add_k_table(rows=())
        flat = self.add_flat(self.add_end_flange(self.add_base()))
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.flat_length(self.K), places=6)

    def test_unfold_uses_the_k_factor_table_for_each_bend(self):
        """Unfold reads the radius of each bend, and each bend gets its own row."""
        self.add_k_table()
        converted = self.add_unfold(self.part_bracket(), 1.0)
        flat = self.add_flat(converted)
        allowance = (self.R + 0.38 * self.T) * math.pi / 2
        expected = (self.LENGTH + allowance + 20.0) * self.WIDTH - math.pi * (16.0 + 9.0)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, expected, places=5)

    # ------------------------------------------------------------------------------- extend

    def add_feature(self, kind, name, **properties):
        obj = self.doc.addObject("SheetDesign::" + kind, name)
        for key, value in properties.items():
            setattr(obj, key, value)
        self.doc.recompute()
        return obj

    def end_edge(self, source, top=True):
        z = self.T if top else 0.0
        return (source, [find_edge(source.Shape, V(self.LENGTH, 0, z), V(self.LENGTH, self.WIDTH, z))])

    def quad(self, points):
        return Part.Face(Part.makePolygon(points + [points[0]]))

    def test_extend_moves_an_edge(self):
        base = self.add_base()
        extend = self.add_feature("Extend", "Extend", Edge=self.end_edge(base), Length=15.0)
        self.assertSolid(extend)
        self.assertAlmostEqual(extend.Shape.Volume, (self.LENGTH + 15.0) * self.WIDTH * self.T, places=6)
        extend.Length = -10.0
        self.doc.recompute()
        self.assertAlmostEqual(extend.Shape.BoundBox.XMax, self.LENGTH - 10.0, places=6)
        extend.Length = 15.0
        extend.Gap1 = 5.0
        extend.Gap2 = 10.0
        self.doc.recompute()
        expected = self.LENGTH * self.WIDTH + 15.0 * (self.WIDTH - 15.0)
        self.assertAlmostEqual(self.add_flat(extend).Shape.Faces[0].Area, expected, places=6)

    def test_flange_on_an_extended_edge(self):
        extend = self.add_feature("Extend", "Extend", Edge=self.end_edge(self.add_base()), Length=15.0)
        x = self.LENGTH + 15.0
        flange = self.add_flange(extend, V(x, 0, self.T), V(x, self.WIDTH, self.T), Length=20.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, x + self.R + self.T, places=6)

    def test_extend_of_an_edge_with_a_bend_is_rejected(self):
        flange = self.add_end_flange(self.add_base())
        extend = self.add_feature("Extend", "Extend", Edge=self.end_edge(flange, top=False), Length=5.0)
        self.assertFalse(extend.isValid())
        self.assertIn("has a bend", extend.getStatusString())

    def test_extend_by_a_profile(self):
        base = self.add_base()
        tab = self.quad([V(100, 10, 0), V(120, 10, 0), V(120, 30, 0), V(100, 30, 0)])
        extend = self.add_feature("Extend", "Extend", Edge=(base, []), Profile=self.add_shape(tab, "Tab"))
        self.assertSolid(extend)
        self.assertAlmostEqual(extend.Shape.Volume, (self.LENGTH * self.WIDTH + 400.0) * self.T, places=6)

    def test_extend_by_a_profile_on_a_folded_panel(self):
        flange = self.add_end_flange(self.add_base())
        x = self.LENGTH + self.R + self.T
        top = self.T + self.R + 20.0
        tab = self.quad([V(x, 10, top), V(x, 30, top), V(x, 30, top + 10), V(x, 10, top + 10)])
        extend = self.add_feature("Extend", "Extend", Edge=(flange, []), Profile=self.add_shape(tab, "Tab"))
        self.assertSolid(extend)
        self.assertAlmostEqual(extend.Shape.Volume, flange.Shape.Volume + 200.0 * self.T, places=5)
        self.assertAlmostEqual(extend.Shape.BoundBox.ZMax, top + 10.0, places=6)

    def test_extend_profile_that_does_not_touch_is_rejected(self):
        base = self.add_base()
        tab = self.quad([V(110, 10, 0), V(120, 10, 0), V(120, 30, 0), V(110, 30, 0)])
        extend = self.add_feature("Extend", "Extend", Edge=(base, []), Profile=self.add_shape(tab, "Tab"))
        self.assertFalse(extend.isValid())

    # --------------------------------------------------------------------------------- fold

    def bend_line(self, x, z=None):
        z = self.T if z is None else z
        return self.add_shape(Part.makeLine(V(x, 0, z), V(x, self.WIDTH, z)), "Line")

    def test_fold_divides_a_panel(self):
        base = self.add_base()
        fold = self.add_feature("Fold", "Fold", Source=base, BendLines=self.bend_line(60.0))
        self.assertSolid(fold)
        half = self.allowance() / 2
        box = fold.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, 60.0 - half + self.R + self.T, places=6)
        self.assertAlmostEqual(box.ZMax, self.T + self.R + 40.0 - half, places=6)
        # The flat pattern is the same as before the fold.
        flat = self.add_flat(fold)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.LENGTH * self.WIDTH, places=6)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, self.LENGTH, places=6)
        self.assertEqual([round(a, 6) for a in flat.BendAngles], [90.0])
        fold.Reversed = True
        self.doc.recompute()
        self.assertAlmostEqual(fold.Shape.BoundBox.ZMin, -(self.R + 40.0 - half), places=6)

    def test_fold_moves_the_features_after_the_line(self):
        flange = self.add_end_flange(self.add_base())
        before = self.add_flat(flange).Shape.Faces[0].Area
        fold = self.add_feature("Fold", "Fold", Source=flange, BendLines=self.bend_line(60.0))
        self.assertSolid(fold)
        self.assertEqual(fold.Overlap.Value, 0.0)
        half = self.allowance() / 2
        # The flange is now on the top end of the folded part and points back.
        self.assertAlmostEqual(fold.Shape.BoundBox.ZMax, self.T + self.R + 40.0 - half + self.R + self.T, places=6)
        self.assertAlmostEqual(self.add_flat(fold).Shape.Faces[0].Area, before, places=6)

    def test_fold_on_a_flange_panel(self):
        flange = self.add_end_flange(self.add_base(), Length=40.0)
        x = self.LENGTH + self.R + self.T
        z = self.T + self.R + 20.0
        line = self.add_shape(Part.makeLine(V(x, 0, z), V(x, self.WIDTH, z)), "Line")
        fold = self.add_feature("Fold", "Fold", Source=flange, BendLines=line, Angle=45.0)
        self.assertSolid(fold)
        self.assertEqual(fold.Overlap.Value, 0.0)
        flat = self.add_flat(fold)
        self.assertEqual(len(flat.BendAngles), 2)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.add_flat(flange).Shape.Faces[0].Area, places=6)

    def test_fold_through_a_hole(self):
        base = self.add_base()
        hole = Part.Face(Part.Wire(Part.makeCircle(5.0, V(60, 25, 0))))
        cut = self.add_cut(base, self.add_shape(hole, "Hole"), "Flat")
        fold = self.add_feature("Fold", "Fold", Source=cut, BendLines=self.bend_line(60.0))
        self.assertSolid(fold)
        self.assertEqual(fold.Overlap.Value, 0.0)
        expected = self.LENGTH * self.WIDTH - math.pi * 25.0
        self.assertAlmostEqual(self.add_flat(fold).Shape.Faces[0].Area, expected, places=5)

    def test_fold_line_off_the_part_is_rejected(self):
        base = self.add_base()
        fold = self.add_feature("Fold", "Fold", Source=base, BendLines=self.bend_line(60.0, z=10.0))
        self.assertFalse(fold.isValid())
        self.assertIn("not on a panel", fold.getStatusString())

    # ------------------------------------------------------------- base from an open profile

    def add_open_base(self, points, **properties):
        base = self.doc.addObject("SheetDesign::BasePanel", "Base")
        base.Profile = self.add_shape(Part.makePolygon(points), "Section")
        base.Rule = self.rule
        base.Width = self.WIDTH
        for key, value in properties.items():
            setattr(base, key, value)
        self.doc.recompute()
        return base

    def test_base_from_an_open_profile(self):
        base = self.add_open_base([V(0, 0, 0), V(60, 0, 0), V(60, 40, 0)])
        self.assertSolid(base)
        box = base.Shape.BoundBox
        # The line is one side of the sheet. The material is on its outer side.
        self.assertAlmostEqual(box.XMin, 0.0, places=6)
        self.assertAlmostEqual(box.XMax, 60.0 + self.T, places=6)
        self.assertAlmostEqual(box.YMin, -self.T, places=6)
        self.assertAlmostEqual(box.YMax, 40.0, places=6)
        self.assertAlmostEqual(box.ZMin, 0.0, places=6)
        self.assertAlmostEqual(box.ZMax, self.WIDTH, places=6)
        flat = self.add_flat(base)
        length = (60.0 - self.R) + self.allowance() + (40.0 - self.R)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, length * self.WIDTH, places=6)
        base.Symmetric = True
        self.doc.recompute()
        self.assertAlmostEqual(base.Shape.BoundBox.ZMin, -self.WIDTH / 2, places=6)

    def test_base_from_an_open_profile_with_more_segments(self):
        base = self.add_open_base([V(0, 30, 0), V(0, 0, 0), V(60, 0, 0), V(60, 30, 0), V(80, 30, 0)])
        self.assertSolid(base)
        self.assertEqual(base.Overlap.Value, 0.0)
        self.assertEqual(len(self.add_flat(base).BendAngles), 3)

    def test_base_from_one_segment(self):
        base = self.add_open_base([V(0, 0, 0), V(60, 0, 0)])
        self.assertSolid(base)
        self.assertAlmostEqual(base.Shape.Volume, 60.0 * self.WIDTH * self.T, places=6)

    def test_base_shapes(self):
        bends = {"Plate": 0, "Angle": 1, "Channel": 2, "Tub": 4, "Hat": 4, "Box": 5}
        for kind, count in bends.items():
            shape = self.add_feature("BaseShape", "Shape" + kind, Rule=self.rule, Type=kind)
            self.assertSolid(shape)
            self.assertEqual(shape.Overlap.Value, 0.0, kind)
            self.assertEqual(len(self.add_flat(shape).BendAngles), count, kind)
        tub = self.doc.getObject("ShapeTub")
        box = tub.Shape.BoundBox
        self.assertAlmostEqual(box.XMin, -(self.R + self.T), places=6)
        self.assertAlmostEqual(box.XMax, 100.0 + self.R + self.T, places=6)
        self.assertAlmostEqual(box.ZMax, self.T + self.R + 30.0, places=6)
        hat = self.doc.getObject("ShapeHat")
        self.assertAlmostEqual(hat.Shape.BoundBox.XMax, 100.0 + 2 * self.R + self.T + 15.0, places=6)

    # ---------------------------------------------------------------------------------- hem

    def add_hem(self, **properties):
        base = self.add_base()
        return self.add_feature("Hem", "Hem", Edge=self.end_edge(base), **properties)

    def test_flat_hem(self):
        hem = self.add_hem(Type="Flat", Length=8.0)
        self.assertSolid(hem)
        self.assertEqual(hem.Overlap.Value, 0.0)
        box = hem.Shape.BoundBox
        self.assertAlmostEqual(box.ZMax, 2 * self.T, places=6)
        self.assertAlmostEqual(box.XMax, self.LENGTH + self.T, places=6)
        flat = self.add_flat(hem)
        length = self.LENGTH + self.K * self.T * math.pi + 8.0
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)
        self.assertEqual([round(a, 6) for a in flat.BendAngles], [180.0])
        self.assertEqual(list(flat.BendRadii), [0.0])

    def test_open_hem(self):
        hem = self.add_hem(Type="Open", Length=8.0, Gap=3.0)
        self.assertSolid(hem)
        self.assertEqual(hem.Overlap.Value, 0.0)
        box = hem.Shape.BoundBox
        self.assertAlmostEqual(box.ZMax, 2 * self.T + 3.0, places=6)
        self.assertAlmostEqual(box.XMax, self.LENGTH + 1.5 + self.T, places=6)
        hem.Reversed = True
        self.doc.recompute()
        self.assertAlmostEqual(hem.Shape.BoundBox.ZMin, -(self.T + 3.0), places=6)

    def test_teardrop_hem(self):
        hem = self.add_hem(Type="Teardrop", Length=8.0, Gap=3.0)
        self.assertSolid(hem)
        self.assertEqual(hem.Overlap.Value, 0.0)
        angle = self.add_flat(hem).BendAngles[0]
        self.assertAlmostEqual(angle, 180.0 + math.degrees(math.asin(1.5 / 8.0)), places=6)

    def test_rolled_hem(self):
        hem = self.add_hem(Type="Rolled", Gap=6.0)
        self.assertSolid(hem)
        self.assertEqual(hem.Overlap.Value, 0.0)
        flat = self.add_flat(hem)
        self.assertAlmostEqual(flat.BendAngles[0], 270.0, places=6)
        self.assertAlmostEqual(flat.BendRadii[0], 3.0, places=6)
        box = hem.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH + 3.0 + self.T, places=6)
        self.assertAlmostEqual(box.ZMax, 2 * self.T + 6.0, places=6)

    # ------------------------------------------------------------------------ corner relief

    def corner_part(self):
        base = self.add_base()
        first = self.add_end_flange(base)
        return self.add_flange(first, V(0, self.WIDTH, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0)

    def test_corner_relief_circle(self):
        part = self.corner_part()
        before = self.add_flat(part).Shape.Faces[0].Area
        relief = self.add_feature("CornerRelief", "Relief", Source=part, Type="Circle", Size=4.0)
        self.assertSolid(relief)
        self.assertEqual(relief.Overlap.Value, 0.0)
        # The circle is on the corner of the bottom panel: three quarters of it are in material.
        self.assertAlmostEqual(self.add_flat(relief).Shape.Faces[0].Area, before - 3.0 * math.pi, places=5)

    def test_corner_relief_square(self):
        part = self.corner_part()
        before = self.add_flat(part).Shape.Faces[0].Area
        relief = self.add_feature("CornerRelief", "Relief", Source=part, Type="Square", Size=4.0)
        self.assertSolid(relief)
        self.assertAlmostEqual(self.add_flat(relief).Shape.Faces[0].Area, before - 12.0, places=5)

    def test_corner_relief_needs_a_corner(self):
        relief = self.add_feature("CornerRelief", "Relief", Source=self.add_end_flange(self.add_base()))
        self.assertFalse(relief.isValid())
        self.assertIn("no corner", relief.getStatusString())

    def test_corner_relief_tells_the_size_that_reaches_the_corner(self):
        base = self.add_base()
        first = self.add_end_flange(base, Gap2=6.0)
        part = self.add_flange(first, V(0, self.WIDTH, self.T), V(self.LENGTH, self.WIDTH, self.T), Length=20.0)
        relief = self.add_feature("CornerRelief", "Relief", Source=part, Size=4.0)
        self.assertFalse(relief.isValid())
        self.assertIn("size of 6 mm", relief.getStatusString())
        relief.Size = 6.0
        self.doc.recompute()
        self.assertSolid(relief)

    # -------------------------------------------------------------------------- wrapped cut

    def test_wrapped_cut_goes_into_the_bend(self):
        flange = self.add_end_flange(self.add_base())
        x = self.LENGTH + self.R + self.T
        low = self.T + self.R
        slot = self.quad([V(x, 20, low - 2), V(x, 30, low - 2), V(x, 30, low + 10), V(x, 20, low + 10)])
        profile = self.add_shape(slot, "Slot")
        folded = self.add_cut(flange, profile, "Folded")
        self.assertFalse(folded.isValid())
        wrapped = self.add_cut(flange, profile, "Wrapped")
        self.assertSolid(wrapped)
        self.assertEqual(wrapped.Overlap.Value, 0.0)
        before = self.add_flat(flange).Shape.Faces[0].Area
        self.assertAlmostEqual(self.add_flat(wrapped).Shape.Faces[0].Area, before - 120.0, places=5)

    # ---------------------------------------------------------------- unfold of sharp folds

    def sharp_bracket(self, height=30.0):
        base = Part.makeBox(self.LENGTH, self.WIDTH, self.T)
        wall = Part.makeBox(self.T, self.WIDTH, height, V(self.LENGTH - self.T, 0, 0))
        return base.fuse(wall).removeSplitter()

    def test_unfold_of_a_sharp_fold_from_the_inner_side(self):
        unfold = self.add_unfold(self.sharp_bracket(), 1.0)
        self.assertSolid(unfold)
        box = unfold.Shape.BoundBox
        # The bend is tangent to the faces of the solid, thus the outer sizes are the same.
        self.assertAlmostEqual(box.XMax, self.LENGTH, places=6)
        self.assertAlmostEqual(box.ZMax, 30.0, places=6)
        flat = self.add_flat(unfold)
        self.assertEqual([round(a, 6) for a in flat.BendAngles], [90.0])
        self.assertEqual([round(r, 6) for r in flat.BendRadii], [self.R])
        length = (self.LENGTH - self.T - self.R) + self.allowance() + (30.0 - self.T - self.R)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)

    def test_unfold_of_a_sharp_fold_from_the_outer_side(self):
        unfold = self.add_unfold(self.sharp_bracket(), -1.0)
        self.assertSolid(unfold)
        box = unfold.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH, places=6)
        self.assertAlmostEqual(box.ZMax, 30.0, places=6)
        flat = self.add_flat(unfold)
        self.assertEqual(len(flat.BendAngles), 1)
        length = (self.LENGTH - self.T - self.R) + self.allowance() + (30.0 - self.T - self.R)
        self.assertAlmostEqual(max(flat.Shape.BoundBox.XLength, flat.Shape.BoundBox.YLength), length, places=6)

    def test_unfold_of_a_sharp_fold_with_radius_zero(self):
        self.rule.BendRadius = 0.0
        unfold = self.add_unfold(self.sharp_bracket(), 1.0)
        self.assertSolid(unfold)
        flat = self.add_flat(unfold)
        length = (self.LENGTH - self.T) + self.K * self.T * math.pi / 2 + (30.0 - self.T)
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)

    # ------------------------------------------------------------ convert a solid (step 20)

    def add_convert(self, shape, normals, **properties):
        """Convert the faces of a solid. Each face is given by its outer normal."""
        source = self.add_shape(shape, "Block")
        names = []
        for normal in normals:
            for index, face in enumerate(shape.Faces):
                if face.Surface.TypeId == "Part::GeomPlane" and face.normalAt(0, 0).dot(normal) > 0.99:
                    names.append(f"Face{index + 1}")
        return self.add_feature("Convert", "Convert", Faces=(source, names), Rule=self.rule, **properties)

    BOX = (100.0, 60.0, 30.0)
    SIDES = [V(0, 0, -1), V(1, 0, 0), V(-1, 0, 0), V(0, 1, 0), V(0, -1, 0)]

    def test_convert_a_box_to_a_tub(self):
        length, width, height = self.BOX
        convert = self.add_convert(Part.makeBox(length, width, height), self.SIDES)
        self.assertSolid(convert)
        self.assertEqual(convert.Overlap.Value, 0.0)
        # The sheet is in the solid: the part has the outer sizes of the box.
        box = convert.Shape.BoundBox
        for got, expected in zip((box.XMin, box.YMin, box.ZMin, box.XMax, box.YMax, box.ZMax), (0, 0, 0, length, width, height)):
            self.assertAlmostEqual(got, expected, places=6)
        flat = self.add_flat(convert)
        self.assertEqual(len(flat.Shape.Faces), 1)
        self.assertEqual(len(flat.BendAngles), 4)
        setback = self.R + self.T
        self.assertAlmostEqual(
            flat.Shape.BoundBox.XLength, (length - 2 * setback) + 2 * self.allowance() + 2 * (height - setback), places=6
        )
        self.assertAlmostEqual(
            flat.Shape.BoundBox.YLength, (width - 2 * setback) + 2 * self.allowance() + 2 * (height - setback), places=6
        )

    def test_convert_with_a_cover(self):
        convert = self.add_convert(Part.makeBox(*self.BOX), self.SIDES + [V(0, 0, 1)])
        self.assertSolid(convert)
        self.assertEqual(convert.Overlap.Value, 0.0)
        self.assertEqual(len(self.add_flat(convert).BendAngles), 5)
        self.assertAlmostEqual(convert.Shape.BoundBox.ZMax, self.BOX[2], places=6)

    def test_convert_with_the_sheet_on_the_outer_side(self):
        length, width, height = self.BOX
        convert = self.add_convert(Part.makeBox(length, width, height), self.SIDES, Inside=False)
        self.assertSolid(convert)
        self.assertEqual(convert.Overlap.Value, 0.0)
        box = convert.Shape.BoundBox
        self.assertAlmostEqual(box.ZMin, -self.T, places=6)
        self.assertAlmostEqual(box.XMax, length + self.T, places=6)

    def test_convert_rip_gap(self):
        tight = self.add_convert(Part.makeBox(*self.BOX), self.SIDES)
        area = self.add_flat(tight).Shape.Faces[0].Area
        tight.RipGap = 1.0
        self.doc.recompute()
        self.assertSolid(tight)
        self.assertLess(self.add_flat(tight).Shape.Faces[0].Area, area)

    def test_convert_needs_faces_with_a_common_edge(self):
        convert = self.add_convert(Part.makeBox(*self.BOX), [V(0, 0, -1), V(0, 0, 1)])
        self.assertFalse(convert.isValid())
        self.assertIn("common", convert.getStatusString())

    def test_flange_on_a_converted_part(self):
        convert = self.add_convert(Part.makeBox(*self.BOX), self.SIDES)
        length, width, height = self.BOX
        edge = find_edge(convert.Shape, V(length, self.T, height), V(length, width - self.T, height))
        flange = self.add_feature("Flange", "Lip", Edge=(convert, [edge]), Length=10.0)
        self.assertSolid(flange)
        self.assertEqual(len(self.add_flat(flange).BendAngles), 5)

    # ----------------------------------------------------------------------- forming (step 21)

    def add_form(self, source, tool, base_normal, open_normals=()):
        obj = self.add_shape(tool, "Tool")
        names = []
        for normal in [base_normal] + list(open_normals):
            for index, face in enumerate(tool.Faces):
                if face.Surface.TypeId == "Part::GeomPlane" and face.normalAt(0, 0).dot(normal) > 0.99:
                    names.append(f"Face{index + 1}")
        return self.add_feature("Form", "Form", Source=source, Tool=(obj, names))

    def test_form_adds_a_shell_of_the_tool(self):
        base = self.add_base()
        tool = Part.makeBox(30.0, 20.0, 5.0, V(35, 15, self.T))
        form = self.add_form(base, tool, V(0, 0, -1))
        self.assertSolid(form)
        self.assertEqual(form.Overlap.Value, 0.0)
        self.assertAlmostEqual(form.Shape.BoundBox.ZMax, self.T + 5.0 + self.T, places=5)
        # The space of the tool is empty.
        self.assertAlmostEqual(form.Shape.common(tool).Volume, 0.0, places=5)
        # The flat pattern keeps the material, and has the outline as a mark.
        flat = self.add_flat(form)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.LENGTH * self.WIDTH, places=6)
        marks = [part for part in flat.Shape.SubShapes if part.ShapeType == "Compound"]
        self.assertEqual(len(marks), 1)
        self.assertAlmostEqual(marks[0].Wires[0].Length, 100.0, places=6)

    def test_form_with_an_open_face_is_a_louver(self):
        base = self.add_base()
        tool = Part.makeBox(30.0, 20.0, 5.0, V(35, 15, self.T))
        closed = self.add_form(base, tool, V(0, 0, -1))
        louver = self.add_form(base, tool, V(0, 0, -1), [V(1, 0, 0)])
        self.assertSolid(louver)
        self.assertLess(louver.Shape.Volume, closed.Shape.Volume)

    def test_form_on_the_bottom_side_and_on_a_flange(self):
        flange = self.add_end_flange(self.add_base(), Length=40.0)
        x = self.LENGTH + self.R + self.T
        tool = Part.makeBox(4.0, 20.0, 15.0, V(x, 15, self.T + self.R + 10.0))
        form = self.add_form(flange, tool, V(-1, 0, 0))
        self.assertSolid(form)
        self.assertEqual(form.Overlap.Value, 0.0)
        self.assertAlmostEqual(form.Shape.BoundBox.XMax, x + 4.0 + self.T, places=5)

    def test_two_forms_with_a_round_tool_give_no_false_overlap(self):
        base = self.add_base()
        dome = Part.makeSphere(40, V(30, 25, self.T - 34)).common(Part.makeBox(200, 200, 100, V(-50, -50, self.T)))
        first = self.add_form(base, dome, V(0, 0, -1))
        wedge = Part.Face(
            Part.makePolygon([V(60, 10, self.T), V(80, 10, self.T), V(80, 10, self.T + 7), V(60, 10, self.T)])
        ).extrude(V(0, 30, 0))
        second = self.add_form(first, wedge, V(0, 0, -1), [V(1, 0, 0)])
        self.assertSolid(second)
        self.assertEqual(second.Overlap.Value, 0.0)

    def test_form_outline_must_be_in_a_panel(self):
        base = self.add_base()
        tool = Part.makeBox(30.0, 20.0, 5.0, V(85, 15, self.T))
        form = self.add_form(base, tool, V(0, 0, -1))
        self.assertFalse(form.isValid())
        self.assertIn("fully in it", form.getStatusString())

    def test_export_has_a_layer_for_the_formed_regions(self):
        import SheetDesignExport

        base = self.add_base()
        form = self.add_form(base, Part.makeBox(30.0, 20.0, 5.0, V(35, 15, self.T)), V(0, 0, -1))
        entities = self.dxf_entities(SheetDesignExport.dxf_text(self.add_flat(form)))
        self.assertEqual([kind for kind, layer, _v in entities if layer == "FORM"], ["LINE"] * 4)
        self.assertEqual(len([1 for _k, layer, _v in entities if layer == "CUTOUT"]), 0)

    # ------------------------------------------------------------------- perforation (step 22)

    def test_perforation_cuts_slots_in_the_bend_zone(self):
        flange = self.add_end_flange(self.add_base())
        before = self.add_flat(flange).Shape.Faces[0].Area
        perforation = self.add_feature("Perforation", "Perforation", Edges=(flange, []), CutLength=10.0, TabLength=4.0)
        self.assertSolid(perforation)
        self.assertEqual(perforation.Overlap.Value, 0.0)
        # A bend line of 50 mm takes three cuts of 10 mm with tabs of 4 mm.
        flat = self.add_flat(perforation)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, before - 3 * 10.0 * self.allowance(), places=5)
        self.assertEqual(len(flat.Shape.Faces[0].Wires), 4)

    def test_perforation_of_a_selected_bend(self):
        first = self.add_end_flange(self.add_base())
        part = self.add_flange(first, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=20.0)
        edge = find_edge(part.Shape, V(self.LENGTH, 0, 0), V(self.LENGTH, self.WIDTH, 0))
        one = self.add_feature("Perforation", "One", Edges=(part, [edge]), CutLength=10.0, TabLength=4.0)
        both = self.add_feature("Perforation", "Both", Edges=(part, []), CutLength=10.0, TabLength=4.0)
        self.assertSolid(one)
        self.assertSolid(both)
        self.assertEqual(len(self.add_flat(one).Shape.Faces[0].Wires), 4)
        self.assertEqual(len(self.add_flat(both).Shape.Faces[0].Wires), 7)

    def test_perforation_with_a_width(self):
        flange = self.add_end_flange(self.add_base())
        before = self.add_flat(flange).Shape.Faces[0].Area
        perforation = self.add_feature("Perforation", "Perforation", Edges=(flange, []), CutLength=10.0, TabLength=4.0, Width=1.0)
        self.assertSolid(perforation)
        self.assertAlmostEqual(self.add_flat(perforation).Shape.Faces[0].Area, before - 30.0, places=5)

    def test_perforation_needs_a_long_bend(self):
        flange = self.add_end_flange(self.add_base())
        perforation = self.add_feature("Perforation", "Perforation", Edges=(flange, []), CutLength=60.0)
        self.assertFalse(perforation.isValid())
        self.assertIn("long enough", perforation.getStatusString())

    # ----------------------------------------------------- flange on several edges (step 24)

    def corner_edges(self, source):
        return (
            source,
            [
                find_edge(source.Shape, V(self.LENGTH, 0, self.T), V(self.LENGTH, self.WIDTH, self.T)),
                find_edge(source.Shape, V(0, self.WIDTH, self.T), V(self.LENGTH, self.WIDTH, self.T)),
            ],
        )

    def walls_area(self):
        return self.LENGTH * self.WIDTH + (self.allowance() + 20.0) * (self.LENGTH + self.WIDTH)

    def test_flange_on_several_edges(self):
        base = self.add_base()
        flange = self.add_feature("Flange", "Flange", Edge=self.corner_edges(base), Length=20.0)
        self.assertSolid(flange)
        self.assertEqual(flange.Overlap.Value, 0.0)
        flat = self.add_flat(flange)
        self.assertEqual(len(flat.BendAngles), 2)
        self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.walls_area(), places=6)
        box = flange.Shape.BoundBox
        self.assertAlmostEqual(box.XMax, self.LENGTH + self.R + self.T, places=6)
        self.assertAlmostEqual(box.YMax, self.WIDTH + self.R + self.T, places=6)

    def test_closed_corner_types(self):
        base = self.add_base()
        flange = self.add_feature("Flange", "Flange", Edge=self.corner_edges(base), Length=20.0)
        outer = self.R + self.T
        # The panel of each flange becomes wider by these sizes at the corner.
        cases = {"Open": 0.0, "Butt": 2 * (outer - self.T), "Overlap": outer + (outer - self.T), "Underlap": outer + (outer - self.T)}
        flat = self.add_flat(flange)
        for kind, added in cases.items():
            flange.CornerType = kind
            self.doc.recompute()
            self.assertSolid(flange)
            self.assertEqual(flange.Overlap.Value, 0.0, kind)
            self.assertAlmostEqual(flat.Shape.Faces[0].Area, self.walls_area() + 20.0 * added, places=5, msg=kind)
        # Overlap: the first flange covers the end of the second.
        flange.CornerType = "Overlap"
        self.doc.recompute()
        wall = [f for f in flange.Shape.Faces if abs(f.normalAt(0, 0).x - 1.0) < 1e-6 and f.Area > 500.0]
        self.assertAlmostEqual(max(f.BoundBox.YMax for f in wall), self.WIDTH + outer, places=6)
        flange.CornerGap = 0.5
        self.doc.recompute()
        self.assertEqual(flange.Overlap.Value, 0.0)
        self.assertAlmostEqual(
            flat.Shape.Faces[0].Area, self.walls_area() + 20.0 * (outer + outer - self.T - 0.5), places=5
        )

    def test_closed_corners_of_a_tub(self):
        base = self.add_base()
        t, length, width = self.T, self.LENGTH, self.WIDTH
        edges = [
            find_edge(base.Shape, V(length, 0, t), V(length, width, t)),
            find_edge(base.Shape, V(0, width, t), V(length, width, t)),
            find_edge(base.Shape, V(0, 0, t), V(0, width, t)),
            find_edge(base.Shape, V(0, 0, t), V(length, 0, t)),
        ]
        tub = self.add_feature("Flange", "Tub", Edge=(base, edges), Length=20.0, CornerType="Overlap", CornerGap=0.2)
        self.assertSolid(tub)
        self.assertEqual(tub.Overlap.Value, 0.0)
        flat = self.add_flat(tub)
        self.assertEqual(len(flat.Shape.Faces), 1)
        self.assertEqual(len(flat.BendAngles), 4)

    def test_flange_extension(self):
        flange = self.add_end_flange(self.add_base(), Extend2=5.0)
        self.assertSolid(flange)
        expected = self.LENGTH * self.WIDTH + self.allowance() * self.WIDTH + 20.0 * (self.WIDTH + 5.0)
        self.assertAlmostEqual(self.add_flat(flange).Shape.Faces[0].Area, expected, places=6)
        self.assertAlmostEqual(flange.Shape.BoundBox.YMax, self.WIDTH + 5.0, places=6)

    def test_flange_with_its_own_radius(self):
        flange = self.add_end_flange(self.add_base(), UseRuleRadius=False, Radius=3.0)
        self.assertSolid(flange)
        self.assertAlmostEqual(flange.Shape.BoundBox.XMax, self.LENGTH + 3.0 + self.T, places=6)
        flat = self.add_flat(flange)
        self.assertEqual([round(r, 6) for r in flat.BendRadii], [3.0])
        length = self.LENGTH + (3.0 + self.K * self.T) * math.pi / 2 + 20.0
        self.assertAlmostEqual(flat.Shape.BoundBox.XLength, length, places=6)

    def test_profile_with_several_edges_is_rejected(self):
        base = self.add_base()
        shape = self.quad([V(100, 10, self.T), V(120, 10, self.T), V(120, 30, self.T), V(100, 30, self.T)])
        flange = self.add_feature("Flange", "Flange", Edge=self.corner_edges(base), Profile=self.add_shape(shape, "Shape"))
        self.assertFalse(flange.isValid())
        self.assertIn("only one edge", flange.getStatusString())

    # ------------------------------------------------------------- drawing (step 26)

    def test_bend_order_gives_the_numbers(self):
        import SheetDesignExport

        flat = self.part_for_export()
        self.assertEqual(SheetDesignExport.bend_numbers(flat), [1, 2])
        flat.BendOrder = [2, 1]
        self.assertEqual(SheetDesignExport.bend_numbers(flat), [2, 1])
        self.assertIn("2 UP 90%%d R1", SheetDesignExport.dxf_text(flat))
        # A list that is not a sequence from 1 is not used.
        flat.BendOrder = [1, 1]
        self.assertEqual(SheetDesignExport.bend_numbers(flat), [1, 2])

    def test_drawing_has_a_view_notes_and_a_table(self):
        import SheetDesignDrawing

        flat = self.part_for_export()
        page, view, notes, table = SheetDesignDrawing.make_drawing(flat)
        self.assertEqual({o.Name for o in page.Views}, {view.Name, notes.Name, table.Name})
        # The view shows the outline with no bend lines. Its direction is the normal of the part.
        self.assertEqual(len(view.Source[0].Shape.Edges), len(flat.Shape.Edges) - 2)
        self.assertAlmostEqual(view.Direction.z, 1.0, places=6)
        self.assertEqual(view.Scale, 1.0)
        # The notes have one line and one number for each bend, on the position of the view.
        self.assertEqual(notes.Symbol.count("<line"), 2)
        self.assertEqual(notes.Symbol.count("<circle"), 2)
        self.assertAlmostEqual(notes.X.Value, view.X.Value, places=6)
        rows, layout = SheetDesignDrawing.table_rows(flat)
        self.assertEqual(rows, [(1, "Up", "90°", "1"), (2, "Down", "45°", "1")])
        self.assertIn(f"Flat size: {layout.box.XLength:.2f} x {layout.box.YLength:.2f} mm", table.Symbol)
        self.assertIn("Thickness: 2 mm", table.Symbol)
        self.assertIn("Scale of the view: 1:1", table.Symbol)

    def test_drawing_follows_the_part(self):
        import xml.dom.minidom

        import SheetDesignDrawing

        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        _page, view, notes, table = SheetDesignDrawing.make_drawing(flat)
        self.assertEqual(notes.Symbol.count("<line"), 1)
        first = notes.Symbol
        second = self.add_flange(flange, V(0, 0, 0), V(0, self.WIDTH, 0), Length=15.0)
        flat.Source = second
        view.Source[0].Source = second
        self.doc.recompute()
        self.assertEqual(notes.Symbol.count("<line"), 2)
        self.assertNotEqual(notes.Symbol, first)
        self.assertEqual(len(SheetDesignDrawing.table_rows(flat)[0]), 2)
        self.assertIn(">Down<", table.Symbol)
        # The symbols are correct XML, and the bend line is at the paper position of the bend.
        xml.dom.minidom.parseString(table.Symbol)
        document = xml.dom.minidom.parseString(notes.Symbol)
        line = document.getElementsByTagName("line")[0]
        box = flat.Shape.BoundBox
        expected = (self.LENGTH + self.allowance() / 2 - (box.XMin + box.XMax) / 2) * view.Scale
        self.assertAlmostEqual(float(line.getAttribute("x1")), expected, places=3)

    def test_drawing_selects_a_scale_for_a_large_part(self):
        import SheetDesignDrawing

        shape = self.add_feature("BaseShape", "Large", Rule=self.rule, Type="Tub", Length=900.0, Width=400.0, Height=80.0)
        flat = self.add_flat(shape)
        _page, view, _notes, _table = SheetDesignDrawing.make_drawing(flat)
        self.assertLess(view.Scale, 1.0)
        self.assertIn(f"Scale of the view: {SheetDesignDrawing.scale_text(view.Scale)}", _table.Symbol)
        template = view.findParentPage().Template
        self.assertLess(flat.Shape.BoundBox.XLength * view.Scale, template.Width.Value)

    # --------------------------------------------------------------------------- fold state

    def test_fold_state_from_flat_to_folded(self):
        flange = self.add_end_flange(self.add_base())
        folded, overlap = SheetDesign.foldedShape(flange, [1.0], True)
        self.assertAlmostEqual(folded.Volume, flange.Shape.Volume, places=5)
        self.assertEqual(overlap, 0.0)
        # At zero, the part has the size of the flat pattern.
        flat, _overlap = SheetDesign.foldedShape(flange, [0.0])
        length = self.LENGTH + self.allowance() + 20.0
        self.assertAlmostEqual(flat.BoundBox.XLength, length, places=2)
        self.assertLess(flat.BoundBox.ZLength, self.T + 0.2)
        # In a middle state, the bend has the same length of material: the volume stays.
        half, overlap = SheetDesign.foldedShape(flange, [0.5], True)
        self.assertTrue(half.isValid())
        self.assertEqual(len(half.Solids), 1)
        self.assertAlmostEqual(half.Volume, flange.Shape.Volume, delta=0.02 * flange.Shape.Volume)
        self.assertEqual(overlap, 0.0)
        # The flange is at 45 degrees: its end is lower than in the folded state.
        self.assertLess(half.BoundBox.ZMax, flange.Shape.BoundBox.ZMax - 3.0)
        self.assertGreater(half.BoundBox.ZMax, self.T + 5.0)
        # The document did not change.
        self.assertFalse(flange.State and "Touched" in flange.State)

    def test_fold_state_for_each_bend(self):
        first = self.add_end_flange(self.add_base())
        second = self.add_flange(first, V(0, 0, self.T), V(0, self.WIDTH, self.T), Length=20.0)
        only_first, _o = SheetDesign.foldedShape(second, [1.0, 0.0])
        only_second, _o = SheetDesign.foldedShape(second, [0.0, 1.0])
        self.assertAlmostEqual(only_first.BoundBox.XMax, self.LENGTH + self.R + self.T, places=5)
        self.assertLess(only_first.BoundBox.XMin, -20.0)
        self.assertAlmostEqual(only_second.BoundBox.XMin, -(self.R + self.T), places=5)
        self.assertGreater(only_second.BoundBox.XMax, self.LENGTH + 20.0)

    def test_fold_state_finds_a_collision_on_the_way(self):
        """A lip that is clear when folded goes through the base on its way if it folds first."""
        import SheetDesignFold

        base = self.add_base()
        wall = self.add_end_flange(base, Length=20.0)
        x = self.LENGTH + self.R + self.T
        z = self.T + self.R + 20.0
        lip = self.add_flange(wall, V(x, 0, z), V(x, self.WIDTH, z), Length=30.0, Reversed=True)
        self.assertEqual(lip.Overlap.Value, 0.0)
        flat = self.add_flat(lip)
        fractions = SheetDesignFold.fractions(flat, 0.5, in_sequence=False)
        self.assertEqual(fractions, [0.5, 0.5])
        flat.BendOrder = [2, 1]
        # The second bend line of the model folds first: it is complete at half of the state.
        self.assertEqual(SheetDesignFold.fractions(flat, 0.5, in_sequence=True), [0.0, 1.0])
        self.assertEqual(SheetDesignFold.fractions(flat, 0.75, in_sequence=True), [0.5, 1.0])
        worst = SheetDesignFold.worst_overlap(flat, steps=8, in_sequence=True)
        self.assertGreaterEqual(worst[0], 0.0)

    def test_refold_with_all_angles_zero_is_the_flat_blank(self):
        face = self.flat_rectangle(140.0)
        line = Part.makeLine(V(100, 0, 0), V(100, self.WIDTH, 0))
        obj = self.add_refold(face, [line], [0.0])
        self.assertSolid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 140.0 * self.WIDTH * self.T, places=6)

    def test_fold_state_does_not_need_the_bend_data_of_the_flat_pattern(self):
        """A flat pattern from an older file has no BendAngles before its first recompute."""
        import SheetDesignFold

        flange = self.add_end_flange(self.add_base())
        flat = self.doc.addObject("SheetDesign::FlatPattern", "Flat")
        flat.Source = flange  # No recompute: the bend data is empty.
        self.assertEqual(list(flat.BendAngles), [])
        self.assertEqual(SheetDesign.bendCount(flange), 1)
        self.assertEqual(SheetDesignFold.fractions(flat, 0.5, in_sequence=True), [0.5])
        shape, _overlap = SheetDesignFold.shape(flat, 0.0)
        self.assertLess(shape.BoundBox.ZLength, self.T + 0.2)

    def test_fold_state_of_a_part_with_closing_bends(self):
        """The panels of a webbed corner move with their tree bends in a middle state."""
        import SheetDesignBoxStyles
        import SheetDesignFold

        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        piece = SheetDesignBoxStyles.STYLES["0226 Bellows Style Top and Bottom Container"](4.0, 300.0, 200.0, 150.0)[0]
        box = SheetDesignBoxStyles.fold(self.doc, self.rule, piece, "Bellows")
        self.doc.recompute()
        self.assertSolid(box)
        flat = self.add_flat(box)
        folded = box.Shape.BoundBox
        half, _overlap = SheetDesignFold.shape(flat, 0.5, in_sequence=False)
        opened, _overlap = SheetDesignFold.shape(flat, 0.0, in_sequence=False)
        self.assertTrue(half.isValid())
        # The fold solver gives the motion: early in the fold, the tube is not yet square.
        early, _overlap = SheetDesignFold.shape(flat, 0.1, in_sequence=False)
        self.assertGreater(early.BoundBox.XLength, folded.XLength + 50.0)
        self.assertLess(opened.BoundBox.ZLength, 10.0)
        # The smallest fold angle is not zero, thus the sizes agree to some millimeters.
        self.assertAlmostEqual(opened.BoundBox.XLength, flat.Shape.BoundBox.XLength, delta=3.0)

    # ------------------------------------------------------------------------------- scores

    def bellows(self):
        import SheetDesignBoxStyles

        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        piece = SheetDesignBoxStyles.STYLES["0226 Bellows Style Top and Bottom Container"](4.0, 300.0, 200.0, 150.0)[0]
        box = SheetDesignBoxStyles.fold(self.doc, self.rule, piece, "Bellows")
        self.doc.recompute()
        return box, self.add_flat(box)

    def test_bends_on_one_line_are_one_score(self):
        """The bellows blank: 32 bends of the model are 16 scores of the designer."""
        import SheetDesignExport

        _box, flat = self.bellows()
        self.assertEqual(len(flat.BendAngles), 32)
        groups, numbers = SheetDesignExport.scores_of(flat)
        # Four vertical scores, two horizontal scores, and ten diagonals.
        self.assertEqual(len(groups), 16)
        self.assertEqual(numbers, list(range(1, 17)))
        sizes = sorted(len(group) for group in groups)
        self.assertEqual(sizes, [1] * 10 + [3] * 4 + [5] * 2)
        layout = SheetDesignExport.Layout(flat)
        vertical = [s for s in layout.scores if len(s.bends) == 3]
        self.assertEqual(vertical[0].angles(), [("UP", 90.0), ("UP", 180.0)])
        self.assertIn("UP 90°/UP 180°", vertical[0].text())

    def test_a_double_score_is_two_scores(self):
        import SheetDesignExport

        V2 = FreeCAD.Vector
        lines = [
            (V2(0, 0, 0), V2(100, 0, 0)),
            (V2(0, 6, 0), V2(100, 6, 0)),  # Beside the first line: a double score.
            (V2(110, 2, 0), V2(200, 2, 0)),  # After the first line, a small step to the side.
            (V2(300, 0, 0), V2(400, 0, 0)),  # On the same line, but far.
        ]
        self.assertEqual(SheetDesignExport.group_scores(lines, 4.0), [[0, 2], [1], [3]])

    def test_sequence_numbers_are_for_scores(self):
        import SheetDesignExport
        import SheetDesignFold

        _box, flat = self.bellows()
        groups, _numbers = SheetDesignExport.scores_of(flat)
        # A number for each bend is not a valid sequence, because the scores have more bends.
        flat.BendOrder = list(range(1, 33))
        self.assertEqual(SheetDesignExport.scores_of(flat)[1], list(range(1, 17)))
        # The first score folds last.
        numbers = [16] + list(range(1, 16))
        order = [0] * 32
        for group, number in zip(groups, numbers):
            for index in group:
                order[index] = number
        flat.BendOrder = order
        self.assertEqual(SheetDesignExport.scores_of(flat)[1], numbers)
        # The bellows box has coupled folds: the fold solver gives the part of each bend.
        fractions = SheetDesignFold.fractions(flat, 0.5, in_sequence=True)
        self.assertEqual(len(fractions), 32)
        self.assertTrue(all(0.0 <= value <= 1.0 for value in fractions))

    def test_drawing_table_has_a_row_for_each_score(self):
        import SheetDesignDrawing

        _box, flat = self.bellows()
        rows, _layout = SheetDesignDrawing.table_rows(flat)
        self.assertEqual(len(rows), 16)
        self.assertIn("90°/180°", [row[2] for row in rows])
        notes = SheetDesignDrawing.notes_svg(flat, 0.2)
        self.assertEqual(notes.count("<circle"), 16)
        self.assertEqual(notes.count("<line"), 32)

    # --------------------------------------------------------------------------- fold solver

    def designer_order(self, box, groups):
        """The vertical scores, then the horizontal scores, then the other scores."""
        bends = SheetDesign.modelInfo(box)["Bends"]

        def kind(group):
            def length(i):
                return math.hypot(bends[i]["End"][0] - bends[i]["Start"][0], bends[i]["End"][1] - bends[i]["Start"][1])

            main = max(group, key=length)
            dx = bends[main]["End"][0] - bends[main]["Start"][0]
            dy = bends[main]["End"][1] - bends[main]["Start"][1]
            return 0 if abs(dx) < 1e-6 else (1 if abs(dy) < 1e-6 else 2)

        ranked = sorted(range(len(groups)), key=lambda g: (kind(groups[g]), g))
        order = [0] * len(groups)
        for position, g in enumerate(ranked):
            order[g] = position + 1
        return order

    def test_model_info(self):
        flange = self.add_end_flange(self.add_base())
        info = SheetDesign.modelInfo(flange)
        self.assertEqual(info["Panels"], 2)
        self.assertEqual(info["Root"], 0)
        self.assertEqual(len(info["Bends"]), 1)
        bend = info["Bends"][0]
        self.assertEqual((bend["Parent"], bend["Child"], bend["Closing"], bend["Up"]), (0, 1, False, True))
        self.assertAlmostEqual(bend["Angle"], math.pi / 2, places=9)
        self.assertAlmostEqual(bend["Allowance"], self.allowance(), places=9)

    def test_fold_solver_closes_the_loops_of_the_bellows_box(self):
        import SheetDesignExport

        box, flat = self.bellows()
        groups, _numbers = SheetDesignExport.scores_of(flat)
        path = SheetDesign.foldPath(box, groups, self.designer_order(box, groups), 12)
        self.assertTrue(path["Loops"])
        self.assertTrue(path["Complete"], path["Notes"])
        # At each step, the two sides of each closing bend are at the same place.
        self.assertLess(path["Mismatch"], 1e-3)
        self.assertLess(math.degrees(path["EndError"]), 0.01)
        final = [bend["Angle"] for bend in SheetDesign.modelInfo(box)["Bends"]]
        closing = [bend["Closing"] for bend in SheetDesign.modelInfo(box)["Bends"]]
        for angle, end, loop in zip(path["States"][-1], final, closing):
            if not loop:
                self.assertAlmostEqual(angle, end, places=4)
        self.assertEqual(path["States"][0], [0.0] * 32)

    def test_fold_solver_agrees_with_the_formula_of_the_bellows_corner(self):
        """A long flap at the angle t has the center triangle at acos(tan(45 deg - t/2))."""
        import SheetDesignExport

        box, flat = self.bellows()
        groups, _numbers = SheetDesignExport.scores_of(flat)
        order = self.designer_order(box, groups)
        path = SheetDesign.foldPath(box, groups, order, 12)
        bends = SheetDesign.modelInfo(box)["Bends"]
        # The hinge of a long flap on the root panel, and the hinge of a center triangle on
        # the first end wall: the two bends of 90 degrees whose parent is the root or the wall.
        flap = next(i for i, b in enumerate(bends) if b["Parent"] == 0 and abs(b["End"][1] - b["Start"][1]) < 1e-6)
        wall = next(i for i, b in enumerate(bends) if b["Parent"] == 0 and abs(b["End"][0] - b["Start"][0]) < 1e-6 and b["Start"][0] > 100)
        center = next(
            i
            for i, b in enumerate(bends)
            if b["Parent"] == bends[wall]["Child"] and abs(b["End"][1] - b["Start"][1]) < 1e-6
            and (b["Start"][1] > 75) == (bends[flap]["Start"][1] > 75)
        )
        number = order[next(g for g, group in enumerate(groups) if flap in group)]
        checked = 0
        for state, driver in zip(path["States"], path["Drivers"]):
            if driver != number or state[flap] < 0.01:
                continue
            expected = math.acos(max(-1.0, min(1.0, math.tan(math.pi / 4 - state[flap] / 2))))
            self.assertAlmostEqual(state[center], expected, places=4)
            checked += 1
        self.assertGreater(checked, 8)

    def test_fold_sequence_that_is_not_possible_is_changed(self):
        """In the model sequence, the flaps close before two walls fold. Those walls are then
        locked. The fold module moves them to the start of the sequence."""
        import SheetDesignExport
        import SheetDesignFold

        box, flat = self.bellows()
        groups, numbers = SheetDesignExport.scores_of(flat)
        direct = SheetDesign.foldPath(box, groups, numbers, 12)
        self.assertFalse(direct["Complete"])
        self.assertTrue(any("cannot move" in note for note in direct["Notes"]))
        motion = SheetDesignFold.motion(flat)
        self.assertTrue(motion.complete, motion.notes)
        self.assertTrue(motion.changed)
        self.assertIn("Folded first", SheetDesignFold.message(flat))
        self.assertEqual(SheetDesignFold.fractions(flat, 0.0), [0.0] * 32)
        ends = SheetDesignFold.fractions(flat, 1.0)
        closing = [bend["Closing"] for bend in SheetDesign.modelInfo(box)["Bends"]]
        self.assertTrue(all(abs(value - 1.0) < 1e-4 for value, loop in zip(ends, closing) if not loop))
        # A middle state is a valid solid with no overlap of the panels of the tube.
        shape, _overlap = SheetDesignFold.shape(flat, 0.3)
        self.assertTrue(shape.isValid())

    def test_part_with_no_loop_does_not_use_the_solver(self):
        import SheetDesignExport
        import SheetDesignFold

        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        self.assertIsNone(SheetDesignFold.motion(flat))
        self.assertEqual(SheetDesignFold.message(flat), "")
        groups, numbers = SheetDesignExport.scores_of(flat)
        path = SheetDesign.foldPath(flange, groups, numbers, 4)
        self.assertFalse(path["Loops"])
        self.assertTrue(path["Complete"])
        self.assertEqual(len(path["States"]), 5)
        self.assertAlmostEqual(path["States"][-1][0], math.pi / 2, places=9)
