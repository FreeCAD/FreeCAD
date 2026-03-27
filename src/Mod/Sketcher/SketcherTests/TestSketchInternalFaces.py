# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import unittest

import FreeCAD
import Part
import Sketcher

App = FreeCAD


def add_rectangle(sketch, x0, y0, x1, y1):
    """Add a constrained rectangle to the sketch. Returns the starting geometry index."""
    i = int(sketch.GeometryCount)
    sketch.addGeometry(Part.LineSegment(App.Vector(x0, y0, 0), App.Vector(x1, y0, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(x1, y0, 0), App.Vector(x1, y1, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(x1, y1, 0), App.Vector(x0, y1, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(x0, y1, 0), App.Vector(x0, y0, 0)))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 0, 2, i + 1, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, i + 0, 1))
    return i


def add_circle(sketch, cx, cy, radius):
    """Add a circle to the sketch. Returns the geometry index."""
    i = int(sketch.GeometryCount)
    sketch.addGeometry(Part.Circle(App.Vector(cx, cy, 0), App.Vector(0, 0, 1), radius), False)
    return i


def add_triangle(sketch, p1, p2, p3):
    """Add a constrained triangle to the sketch. Returns the starting geometry index."""
    i = int(sketch.GeometryCount)
    sketch.addGeometry(Part.LineSegment(App.Vector(*p1, 0), App.Vector(*p2, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(*p2, 0), App.Vector(*p3, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(*p3, 0), App.Vector(*p1, 0)))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 0, 2, i + 1, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 0, 1))
    return i


def add_arc(sketch, cx, cy, radius, start_angle, end_angle):
    """Add an arc of circle to the sketch. Returns the geometry index."""
    i = int(sketch.GeometryCount)
    sketch.addGeometry(
        Part.ArcOfCircle(
            Part.Circle(App.Vector(cx, cy, 0), App.Vector(0, 0, 1), radius),
            start_angle,
            end_angle,
        )
    )
    return i


def get_internal_faces(sketch):
    """Get the list of faces from the InternalShape, or empty list if null."""
    shape = sketch.InternalShape
    if shape.isNull():
        return []
    return shape.Faces


def total_face_area(faces):
    """Sum of areas of all faces."""
    return sum(f.Area for f in faces)


def faces_overlap(faces):
    """Check that no two faces share interior area (simple BBox pre-check)."""
    for i, f1 in enumerate(faces):
        for f2 in faces[i + 1 :]:
            common = f1.common(f2)
            if common.Faces:
                return True
    return False


class TestSketchInternalFaces(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("InternalFacesTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def _make_sketch(self):
        sk = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        sk.MakeInternals = True
        return sk

    # ==================================================================
    # 1. Single closed shapes — basic regression
    # ==================================================================

    def testSingleRectangle(self):
        """A closed rectangle produces 1 face with correct area."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=3)

    def testSingleCircle(self):
        """A single circle produces 1 face with correct area."""
        sk = self._make_sketch()
        r = 10
        add_circle(sk, 0, 0, r)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, math.pi * r * r, places=1)

    def testSingleTriangle(self):
        """A closed triangle produces 1 face with correct area."""
        sk = self._make_sketch()
        add_triangle(sk, (0, 0), (10, 0), (5, 10))
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 50.0, places=3)

    def testSemiCircle(self):
        """An arc + line forming a closed half-circle produces 1 face."""
        sk = self._make_sketch()
        i = add_arc(sk, 0, 0, 10, 0, math.pi)
        j = int(sk.GeometryCount)
        sk.addGeometry(Part.LineSegment(App.Vector(-10, 0, 0), App.Vector(10, 0, 0)))
        sk.addConstraint(Sketcher.Constraint("Coincident", i, 1, j, 2))
        sk.addConstraint(Sketcher.Constraint("Coincident", i, 2, j, 1))
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, math.pi * 100 / 2, places=1)

    # ==================================================================
    # 2. MakeInternals toggle
    # ==================================================================

    def testMakeInternalsDisabled(self):
        """With MakeInternals=False, InternalShape should be null."""
        sk = self._make_sketch()
        sk.MakeInternals = False
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        self.assertTrue(sk.InternalShape.isNull())

    def testMakeInternalsToggle(self):
        """Toggling MakeInternals on/off should add/remove faces."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        self.assertEqual(len(get_internal_faces(sk)), 1)
        sk.MakeInternals = False
        self.Doc.recompute()
        self.assertTrue(sk.InternalShape.isNull())
        sk.MakeInternals = True
        self.Doc.recompute()
        self.assertEqual(len(get_internal_faces(sk)), 1)

    # ==================================================================
    # 3. Open / degenerate geometry — should produce no faces
    # ==================================================================

    def testOpenLine(self):
        """A single line segment (open) should produce no faces."""
        sk = self._make_sketch()
        sk.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 0)

    def testOpenPolyline(self):
        """An open polyline (3 connected segments, not closed) should produce no faces."""
        sk = self._make_sketch()
        i = int(sk.GeometryCount)
        sk.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 10, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(10, 10, 0), App.Vector(0, 10, 0)))
        sk.addConstraint(Sketcher.Constraint("Coincident", i, 2, i + 1, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 0)

    # ==================================================================
    # 4. Non-overlapping shapes — independent faces
    # ==================================================================

    def testTwoSeparateRectangles(self):
        """Two non-overlapping rectangles produce 2 independent faces."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 5, 5)
        add_rectangle(sk, 20, 20, 25, 25)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 50.0, places=3)

    def testTwoSeparateCircles(self):
        """Two non-overlapping circles produce 2 independent faces."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 5)
        add_circle(sk, 30, 0, 5)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)

    # ==================================================================
    # 5. Touching shapes (shared edge/vertex) — boundary cases
    # ==================================================================

    def testTwoRectanglesSharedEdge(self):
        """Two rectangles sharing an edge produce 2 faces, no overlap."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        add_rectangle(sk, 10, 0, 20, 10)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 200.0, places=3)

    def testTwoRectanglesTouchingCorner(self):
        """Two rectangles touching at a corner produce 2 independent faces."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        add_rectangle(sk, 10, 10, 20, 20)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 200.0, places=3)

    # ==================================================================
    # 6. Nesting — shapes fully inside other shapes
    # ==================================================================

    def testCircleInsideRectangle(self):
        """Circle inside rectangle: 2 faces (ring + disc), total area = rectangle."""
        sk = self._make_sketch()
        add_rectangle(sk, -20, -20, 20, 20)
        add_circle(sk, 0, 0, 5)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 1600.0, places=1)

    def testConcentricCircles(self):
        """Two concentric circles: 2 faces (annulus + inner disc)."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 0, 0, 5)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        outer_area = math.pi * 100
        self.assertAlmostEqual(total_face_area(faces), outer_area, places=1)

    def testRectangleInsideRectangle(self):
        """Small rectangle inside a large one: 2 faces (frame + inner)."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 20, 20)
        add_rectangle(sk, 5, 5, 15, 15)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 400.0, places=3)

    # ==================================================================
    # 7. Two overlapping shapes — intersection splitting
    # ==================================================================

    def testTwoOverlappingCircles(self):
        """Two overlapping circles: 3 faces (Venn diagram), no overlap between faces."""
        sk = self._make_sketch()
        r = 10
        d = 12
        add_circle(sk, 0, 0, r)
        add_circle(sk, d, 0, r)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 3)
        # Total area = union of two circles
        cos_arg = d / (2 * r)
        lens = 2 * r * r * math.acos(cos_arg) - (d / 2) * math.sqrt(4 * r * r - d * d)
        expected = 2 * math.pi * r * r - lens
        self.assertAlmostEqual(total_face_area(faces), expected, places=1)

    def testTwoOverlappingRectangles(self):
        """Two overlapping rectangles: 3 faces, total area = union."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        add_rectangle(sk, 5, 5, 15, 15)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 3)
        self.assertAlmostEqual(total_face_area(faces), 175.0, places=2)

    def testTwoOverlappingRectanglesNoOverlap(self):
        """Faces from two overlapping rectangles must not share interior area."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        add_rectangle(sk, 5, 5, 15, 15)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertFalse(faces_overlap(faces), "Generated faces must not overlap")

    def testCircleOverlappingRectangle(self):
        """Circle partially overlapping rectangle produces correct face count."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 20, 20)
        add_circle(sk, 20, 10, 8)  # circle centered on right edge
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        # rect-only + intersection + circle-only = 3
        self.assertEqual(len(faces), 3)

    # ==================================================================
    # 8. Subdivision by internal lines
    # ==================================================================

    def testRectangleWithDiagonal(self):
        """Rectangle with a diagonal line should produce 2 triangular faces."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        sk.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 10, 0)))
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_face_area(faces), 100.0, places=3)

    def testCrossPattern(self):
        """Two perpendicular overlapping rectangles (+ shape): 5 faces."""
        sk = self._make_sketch()
        add_rectangle(sk, -5, -15, 5, 15)  # vertical bar
        add_rectangle(sk, -15, -5, 15, 5)  # horizontal bar
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        # center + 4 arms = 5
        self.assertEqual(len(faces), 5)
        # Total area = union = 2*(10*30) - 10*10 = 500
        self.assertAlmostEqual(total_face_area(faces), 500.0, places=2)

    # ==================================================================
    # 9. Three overlapping shapes — the bug (issue #23406)
    # ==================================================================

    def testThreeOverlappingCircles(self):
        """Three overlapping circles (Venn diagram) should produce 7 faces.

        This reproduces GitHub issue #23406.
        3 exclusive regions + 3 pairwise lens regions + 1 central region = 7.
        """
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 8, 0, 10)
        add_circle(sk, 4, 7, 10)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertEqual(len(faces), 7, "Three overlapping circles should produce 7 faces")

    def testThreeOverlappingRectangles(self):
        """Three overlapping rectangles should produce 7 face regions."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 20, 20)
        add_rectangle(sk, 10, 5, 30, 25)
        add_rectangle(sk, 5, 10, 25, 30)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertGreaterEqual(
            len(faces), 7, "Three overlapping rectangles should produce >= 7 faces"
        )

    def testThreeOverlappingCirclesAreaConservation(self):
        """Total area of faces from 3 circles must equal the union area."""
        sk = self._make_sketch()
        r = 10
        add_circle(sk, 0, 0, r)
        add_circle(sk, 8, 0, r)
        add_circle(sk, 4, 7, r)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        if len(faces) == 0:
            self.skipTest("Bug #23406: 3-circle face generation currently produces 0 faces")
        total = total_face_area(faces)
        # Compute union area via inclusion-exclusion (approximate: use shape boolean)
        c1 = Part.Face(Part.Wire(Part.makeCircle(r, App.Vector(0, 0, 0))))
        c2 = Part.Face(Part.Wire(Part.makeCircle(r, App.Vector(8, 0, 0))))
        c3 = Part.Face(Part.Wire(Part.makeCircle(r, App.Vector(4, 7, 0))))
        union = c1.fuse(c2).fuse(c3)
        expected = sum(f.Area for f in union.Faces)
        self.assertAlmostEqual(total, expected, places=0)

    def testRectangleTriangleCircleOverlap(self):
        """Mixed geometry: overlapping rectangle, triangle, and circle."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 20, 20)
        add_triangle(sk, (10, -5), (30, 15), (10, 15))
        add_circle(sk, 15, 10, 8)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertGreaterEqual(
            len(faces), 4, "Mixed overlapping geometry should produce >= 4 faces"
        )

    def testFourOverlappingCircles(self):
        """Four overlapping circles in a square arrangement."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 8, 0, 10)
        add_circle(sk, 0, 8, 10)
        add_circle(sk, 8, 8, 10)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        # 4 exclusive + 4 pairwise + (at least) center regions
        self.assertGreaterEqual(len(faces), 9, "Four overlapping circles should produce >= 9 faces")

    # ==================================================================
    # 10. Faces must not overlap (geometric correctness)
    # ==================================================================

    def testTwoCirclesNoOverlap(self):
        """Faces from two overlapping circles must not share interior area."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 12, 0, 10)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertFalse(faces_overlap(faces), "Faces must be non-overlapping")

    def testCrossPatternNoOverlap(self):
        """Faces from cross pattern must not share interior area."""
        sk = self._make_sketch()
        add_rectangle(sk, -5, -15, 5, 15)
        add_rectangle(sk, -15, -5, 15, 5)
        self.Doc.recompute()
        faces = get_internal_faces(sk)
        self.assertFalse(faces_overlap(faces), "Faces must be non-overlapping")

    # ==================================================================
    # 11. Element naming
    # ==================================================================

    def testInternalShapeHasEdgeNames(self):
        """InternalShape edges should have mapped element names."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        shape = sk.InternalShape
        self.assertFalse(shape.isNull())
        edge_names = [name for name in shape.ElementReverseMap.keys() if name.startswith("Edge")]
        self.assertGreater(len(edge_names), 0, "Edges should have element names")

    def testInternalShapeHasFaceNames(self):
        """InternalShape faces should have mapped element names."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        shape = sk.InternalShape
        self.assertFalse(shape.isNull())
        face_names = [name for name in shape.ElementReverseMap.keys() if name.startswith("Face")]
        self.assertGreater(len(face_names), 0, "Internal faces should have element names")

    def testElementNamingStableAfterRecompute(self):
        """Element names must be identical across recomputes."""
        sk = self._make_sketch()
        add_rectangle(sk, 0, 0, 10, 10)
        self.Doc.recompute()
        names_before = sorted(sk.InternalShape.ElementReverseMap.keys())
        self.Doc.recompute()
        names_after = sorted(sk.InternalShape.ElementReverseMap.keys())
        self.assertEqual(names_before, names_after)

    def testElementNamingUniqueness(self):
        """Element names should be unique across a multi-face internal shape."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 12, 0, 10)
        self.Doc.recompute()
        shape = sk.InternalShape
        all_names = list(shape.ElementReverseMap.keys())
        self.assertEqual(len(all_names), len(set(all_names)), "All element names must be unique")

    def testFaceNamingWithMultipleFaces(self):
        """Each face should have a unique Face element name."""
        sk = self._make_sketch()
        add_circle(sk, 0, 0, 10)
        add_circle(sk, 12, 0, 10)
        self.Doc.recompute()
        shape = sk.InternalShape
        face_names = [name for name in shape.ElementReverseMap.keys() if name.startswith("Face")]
        self.assertEqual(len(face_names), len(set(face_names)))
        self.assertEqual(len(face_names), 3, "Should have 3 face names for 3 faces")
