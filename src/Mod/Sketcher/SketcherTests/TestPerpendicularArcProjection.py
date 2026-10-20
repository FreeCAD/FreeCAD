# SPDX-License-Identifier: LGPL-2.1-or-later
"""Perpendicular circular-arc projection: exact archive and analytic extrema."""

import math
from pathlib import Path
import unittest
from zipfile import ZipFile
import xml.etree.ElementTree as ET

import FreeCAD as App
import Part
import PartDesign
import Sketcher


class TestPerpendicularArcProjection(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("PerpendicularArc26585")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    @staticmethod
    def placement(root, name):
        node = root.find("./ObjectData/Object[@name='%s']/Properties/"
                         "Property[@name='Placement']/PropertyPlacement" % name)
        if node is None:
            raise AssertionError("Archive placement missing: " + name)
        a = node.attrib
        return App.Placement(App.Vector(*[float(a[k]) for k in ("Px", "Py", "Pz")]),
                             App.Rotation(*[float(a[k]) for k in ("Q0", "Q1", "Q2", "Q3")]))

    def assertPlacementEqual(self, left, right):
        self.assertLess(max(abs(x - y) for x, y in zip(left.toMatrix().A, right.toMatrix().A)),
                        1e-9)

    @staticmethod
    def analyticExtrema(center, radius, first, last, source_placement, target_placement):
        # The input circle is defined independently in its own XY frame.
        # Project its scalar sinusoid analytically; do not use OCC's arc bbox
        # or the production branch's normalized-angle decision tree.
        target_inverse = target_placement.inverse()
        relative = target_inverse.multiply(source_placement)
        projected_center = relative.multVec(center)
        projected_center.z = 0
        x_axis = relative.Rotation.multVec(App.Vector(radius, 0, 0))
        y_axis = relative.Rotation.multVec(App.Vector(0, radius, 0))
        x_axis.z = 0
        y_axis.z = 0
        direction = x_axis if x_axis.Length > 1e-8 else y_axis
        direction = direction * (1.0 / direction.Length)
        if direction.x < -1e-9 or (abs(direction.x) < 1e-9 and direction.y < 0):
            direction = -direction
        a = direction.dot(x_axis)
        b = direction.dot(y_axis)
        candidates = [first, last]
        critical = math.atan2(b, a)
        lower = math.floor((first - critical) / math.pi) - 1
        upper = math.ceil((last - critical) / math.pi) + 1
        candidates.extend(critical + k * math.pi for k in range(lower, upper + 1)
                          if first - 1e-12 <= critical + k * math.pi <= last + 1e-12)
        values = [a * math.cos(t) + b * math.sin(t) for t in candidates]
        return (projected_center + direction * min(values),
                projected_center + direction * max(values))

    def assertProjection(self, target, expected_start, expected_end):
        self.assertEqual(len(target.ExternalGeo), 3)
        actual = target.ExternalGeo[2]
        self.assertIsInstance(actual, Part.LineSegment)
        start, end = actual.StartPoint, actual.EndPoint
        endpoint_error = min(max((start - expected_start).Length, (end - expected_end).Length),
                             max((end - expected_start).Length, (start - expected_end).Length))
        length_error = abs((end - start).Length - (expected_end - expected_start).Length)
        self.assertLess(endpoint_error, 1e-6,
                        "Projected endpoints must equal the true trimmed-arc extrema")
        self.assertLess(length_error, 1e-6)

    def checkArc(self, first, last, reverse=False, source_placement=None,
                 target_placement=None, full_circle=False):
        center = App.Vector(-25, 6, 4)
        radius = 17.5
        circle = Part.Circle(center, App.Vector(0, 0, 1), radius)
        geometry = circle if full_circle else Part.ArcOfCircle(circle, first, last)
        original_start = geometry.value(first)
        original_end = geometry.value(last)
        self.assertLess((original_start - center -
                         App.Vector(radius * math.cos(first), radius * math.sin(first), 0)).Length,
                        1e-8, "Fixture circle parameter frame must match the independent oracle")
        if reverse:
            geometry.reverse()
            if not full_circle:
                self.assertLess((geometry.StartPoint - original_end).Length, 1e-8)
                self.assertLess((geometry.EndPoint - original_start).Length, 1e-8)
        source_placement = source_placement or App.Placement()
        target_placement = target_placement or App.Placement(
            App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90))
        source = self.doc.addObject("Part::Feature", "Source")
        source.Shape = geometry.toShape()
        source.Placement = source_placement
        target = self.doc.addObject("Sketcher::SketchObject", "Target")
        target.Placement = target_placement
        source_normal = source_placement.Rotation.multVec(App.Vector(0, 0, 1))
        target_normal = target_placement.Rotation.multVec(App.Vector(0, 0, 1))
        self.assertLess(abs(source_normal.dot(target_normal)), 1e-8)
        self.assertAlmostEqual(source.Shape.Length, radius * (last - first), places=6)
        target.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        self.assertNotIn("Invalid", target.State)
        self.assertProjection(target, *self.analyticExtrema(center, radius, first, last,
                                                          source_placement, target_placement))

    def checkNormalizedInterval(self, normalized_start, normalized_end, **kwargs):
        # For target normal -Y and source normal +Z, dirOrientation is -X,
        # hence alpha=pi. Choose the underlying source parameters explicitly.
        first = (normalized_start - math.pi) % (2 * math.pi)
        last = first + normalized_end - normalized_start
        self.assertGreater(last, first)
        self.assertLess(last - first, 2 * math.pi)
        self.checkArc(first, last, **kwargs)

    def testPublishedSketchEdge3(self):
        archive = Path(__file__).with_suffix(".FCStd")
        with ZipFile(archive) as saved:
            root = ET.fromstring(saved.read("Document.xml"))
            brep = saved.read("Sketch.Shape.brp").decode("utf-8")
        reference = root.find("./ObjectData/Object[@name='Sketch001']/Properties/"
                              "Property[@name='ExternalGeometry']/LinkSubList/Link")
        self.assertEqual(reference.get("obj"), "Sketch")
        self.assertEqual(reference.get("sub"), "Edge3")
        self.assertIn(";g1;SKT", reference.get("shadow"))
        arc = root.find("./ObjectData/Object[@name='Sketch']/Properties/"
                        "Property[@name='Geometry']/GeometryList/Geometry[@id='1']/ArcOfCircle")
        a = arc.attrib
        self.assertEqual([float(a[k]) for k in ("NormalX", "NormalY", "NormalZ")], [0, 0, 1])
        self.assertEqual(float(a["AngleXU"]), 0)
        center = App.Vector(*[float(a["Center" + k]) for k in ("X", "Y", "Z")])
        radius, first, last = [float(a[k]) for k in ("Radius", "StartAngle", "EndAngle")]
        body_placement = self.placement(root, "Body")
        source_placement = self.placement(root, "Sketch")
        target_placement = self.placement(root, "Sketch001")
        body = self.doc.addObject("PartDesign::Body", "Body")
        body.Placement = body_placement
        shape = Part.Shape()
        shape.importBrepFromString(brep, False)
        self.assertPlacementEqual(shape.Placement, source_placement)
        self.assertGreaterEqual(len(shape.Edges), 3)
        edge = shape.Edges[2]
        self.assertAlmostEqual(edge.Curve.Radius, radius)
        self.assertLess((edge.Curve.Center - center).Length, 1e-8)
        self.assertAlmostEqual(edge.FirstParameter, first)
        self.assertAlmostEqual(edge.LastParameter, last)
        source = body.newObject("PartDesign::Feature", "PublishedSource")
        source.Shape = shape
        self.assertPlacementEqual(source.Placement, source_placement)
        target = body.newObject("Sketcher::SketchObject", "PublishedTarget")
        target.Placement = target_placement
        target.addExternal(source.Name, "Edge3")
        self.doc.recompute()
        expected = self.analyticExtrema(center, radius, first, last,
                                       body_placement.multiply(source_placement),
                                       body_placement.multiply(target_placement))
        self.assertProjection(target, *expected)

    def testReversedArcOrientation(self):
        self.checkNormalizedInterval(3.6, 9.1, reverse=True)

    def testIndependentTranslatedRotatedSource(self):
        source = App.Placement(App.Vector(8, -3, 9), App.Rotation(App.Vector(0, 0, 1), 37))
        self.checkNormalizedInterval(3.6, 9.1, source_placement=source)

    def testCommonRigidFrame(self):
        common = App.Placement(App.Vector(37, -21, 53), App.Rotation(App.Vector(1, 2, 3), 73))
        target = common.multiply(App.Placement(App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90)))
        self.checkNormalizedInterval(3.6, 9.1, source_placement=common, target_placement=target)

    def testFullCircleControl(self):
        self.checkArc(0, 2 * math.pi, full_circle=True)

    def testReversedFullCircleControl(self):
        self.checkArc(0, 2 * math.pi, full_circle=True, reverse=True)


_INTERVALS = (
    ("NegativeEndpoints", -0.6, -0.2),
    ("NegativeCrossZeroShort", -0.6, 0.4),
    ("NegativeCrossZeroLong", -0.6, 1.2),
    ("NegativeBothExtrema", -0.6, 3.5),
    ("PositiveEndpoints", 0.6, 2.4),
    ("PositiveCrossPiShort", 0.6, 4.2),
    ("PositiveCrossPiLong", 0.6, 6.0),
    ("PositiveBothExtrema", 0.6, 6.5),
    ("AfterPiEndpoints", 3.6, 5.8),
    ("AfterPiCrossTwoPiShort", 3.6, 7.0),
    ("AfterPiCrossTwoPiLong", 3.6, 9.1),
    ("AfterPiBothExtrema", 3.6, 9.6),
)


def _intervalTest(start, end):
    def test(self):
        self.checkNormalizedInterval(start, end)
    return test


for _name, _start, _end in _INTERVALS:
    setattr(TestPerpendicularArcProjection, "testInterval" + _name, _intervalTest(_start, _end))
