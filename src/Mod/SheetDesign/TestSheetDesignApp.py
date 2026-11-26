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

    def test_dxf_export(self):
        import SheetDesignDxf

        flange = self.add_end_flange(self.add_base())
        flat = self.add_flat(flange)
        path = os.path.join(tempfile.mkdtemp(), "flat.dxf")
        count = len(self.doc.Objects)
        SheetDesignDxf.export_dxf(flat, path)
        self.assertEqual(len(self.doc.Objects), count)
        with open(path, encoding="utf-8", errors="replace") as handle:
            content = handle.read()
        self.assertIn("ENTITIES", content)
        self.assertGreaterEqual(content.count("\nLINE"), 5)

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
