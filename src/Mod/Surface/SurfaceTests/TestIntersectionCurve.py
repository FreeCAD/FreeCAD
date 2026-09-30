# SPDX-License-Identifier: LGPL-2.1-or-later

"""Regression tests for shape intersections and extruded-profile curves."""

import importlib
import os
import tempfile
import unittest
import zipfile
import xml.etree.ElementTree as ET

import FreeCAD as App
import Part

# Register the native document object types used by these tests.
importlib.import_module("Sketcher")
importlib.import_module("Surface")


class TestIntersectionCurve(unittest.TestCase):
    """Exercise geometry, recompute errors, persistence and element naming."""

    def setUp(self):
        self.doc = App.newDocument("TestIntersectionCurve")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def profiles(self):
        """Create two perpendicular profiles and their intersection."""
        first = self.doc.addObject("Sketcher::SketchObject", "First")
        first.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 10, 0)))
        second = self.doc.addObject("Sketcher::SketchObject", "Second")
        second.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 20, 0)))
        second.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1 = first
        curve.Curve2 = second
        self.doc.recompute()
        return first, second, curve

    def assertCurve(self, curve):
        """Check that the result is a valid wire without faces."""
        self.assertNotIn("Invalid", curve.State)
        self.assertFalse(curve.Shape.isNull())
        self.assertTrue(curve.Shape.isValid())
        self.assertEqual(len(curve.Shape.Wires), 1)
        self.assertEqual(len(curve.Shape.Faces), 0)

    def test_sketch_normals_and_recompute(self):
        """Infer sketch normals and update the intersection after moving a profile."""
        _, second, curve = self.profiles()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, App.Vector(10, 10, 20).Length, places=6)
        second.Placement.Base = App.Vector(0, 0, 1000)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, 1000, places=5)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMax, 1020, places=5)

    def test_element_names_survive_recompute(self):
        """Preserve mapped edge and vertex names through the complete shape pipeline."""
        _, second, curve = self.profiles()
        element_map = curve.Shape.ElementMap
        self.assertEqual(set(element_map.values()), {"Edge1", "Vertex1", "Vertex2"})
        second.Placement.Base = App.Vector(0, 0, 5)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertEqual(curve.Shape.ElementMap, element_map)

    def test_explicit_directions_for_edges(self):
        """Use explicit extrusion directions for straight edges."""
        first = self.doc.addObject("Part::Feature", "First")
        first.Shape = Part.makeLine(App.Vector(), App.Vector(10, 10, 0))
        second = self.doc.addObject("Part::Feature", "Second")
        second.Shape = Part.makeLine(App.Vector(), App.Vector(10, 0, 20))
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1, curve.Curve2 = first, second
        curve.Direction1 = App.Vector(0, 0, 1)
        curve.Direction2 = App.Vector(0, 1, 0)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, App.Vector(10, 10, 20).Length, places=6)

    def test_bspline_wires(self):
        """Keep the intersection on both extruded spline profiles."""
        profiles = []
        for points in (
            [App.Vector(0, 0, 0), App.Vector(5, 4, 0), App.Vector(10, 0, 0)],
            [App.Vector(0, 0, 1), App.Vector(5, 0, 6), App.Vector(10, 0, 2)],
        ):
            spline = Part.BSplineCurve()
            spline.interpolate(points)
            profile = self.doc.addObject("Part::Feature", "Profile")
            profile.Shape = Part.Wire(spline.toShape())
            profiles.append(profile)
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1 = profiles[0]
        curve.Curve2 = profiles[1]
        self.doc.recompute()
        self.assertCurve(curve)
        for point in curve.Shape.discretize(20):
            xy = Part.Vertex(App.Vector(point.x, point.y, 0))
            xz = Part.Vertex(App.Vector(point.x, 0, point.z))
            self.assertLess(xy.distToShape(profiles[0].Shape)[0], 1e-5)
            self.assertLess(xz.distToShape(profiles[1].Shape)[0], 1e-5)

    def test_invalid_inputs_clear_result(self):
        """Clear stale geometry when profiles become invalid."""
        first, second, curve = self.profiles()
        second.Placement.Rotation = App.Rotation()
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        curve.Curve2 = first
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        curve.Curve2 = None
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)

    def test_no_intersection(self):
        """Report disjoint extrusions without retaining an old shape."""
        _, second, curve = self.profiles()
        second.Placement.Base = App.Vector(100, 0, 0)
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())

    def test_oblique_directions(self):
        """Extend extrusions far enough for nearly parallel directions."""
        _, second, curve = self.profiles()
        second.Placement = App.Placement(App.Vector(0, 10, 0), App.Rotation())
        curve.Direction1 = App.Vector(0, 0, 1)
        curve.Direction2 = App.Vector(0, 0.01, 1)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, -2000, places=5)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMax, -1000, places=5)

    def test_disconnected_intersections(self):
        """Preserve all disconnected intersection branches."""
        first, _, curve = self.profiles()
        first.delGeometry(0)
        for start, end in ((0, 4), (6, 10)):
            first.addGeometry(
                Part.LineSegment(App.Vector(start, start, 0), App.Vector(end, end, 0))
            )
        self.doc.recompute()
        self.assertNotIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isValid())
        self.assertEqual(len(curve.Shape.Wires), 2)

    def test_empty_input_and_curve_plane(self):
        """Reject empty shapes and preserve curve-plane intersection points."""
        _, _, curve = self.profiles()
        invalid = self.doc.addObject("Part::Feature", "InvalidProfile")
        curve.Curve1 = invalid
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        invalid.Shape = Part.makePlane(10, 10)
        self.doc.recompute()
        self.assertNotIn("Invalid", curve.State)
        self.assertEqual(len(curve.Shape.Edges), 0)
        self.assertEqual(len(curve.Shape.Vertexes), 1)
        self.assertLess(curve.Shape.Vertexes[0].Point.Length, 1e-6)

    def feature(self, shape):
        """Wrap geometry in a document object."""
        obj = self.doc.addObject("Part::Feature", "Input")
        obj.Shape = shape
        return obj

    def intersection(self, first, second, mode="Automatic"):
        """Create and recompute an intersection with the requested mode."""
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1, curve.Curve2 = first, second
        curve.Mode = mode
        self.doc.recompute()
        return curve

    def test_plane_plane(self):
        """Intersect two bounded planes and clear a result after they separate."""
        first = self.feature(Part.makePlane(10, 10))
        second = self.feature(Part.makePlane(10, 10))
        second.Placement = App.Placement(
            App.Vector(0, 5, -5), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        curve = self.intersection(first, second)
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, 10, places=6)
        self.assertAlmostEqual(curve.Shape.BoundBox.YMin, 5, places=6)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, 0, places=6)
        second.Placement.Base.y = 20
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())

    def test_curved_surface_plane_and_solid(self):
        """Accept curved faces, shells, and solids without profile extrusion."""
        cylinder = Part.makeCylinder(5, 10)
        surface = self.feature(cylinder.Faces[0])
        plane = self.feature(Part.makePlane(20, 20, App.Vector(-10, -10, 5)))
        for shape in (cylinder.Faces[0], Part.makeShell([cylinder.Faces[0]]), cylinder):
            with self.subTest(shape=shape.ShapeType):
                surface.Shape = shape
                curve = self.intersection(surface, plane)
                self.assertCurve(curve)
                self.assertAlmostEqual(curve.Shape.Length, 10 * 3.141592653589793, places=5)
                self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, 5, places=6)

    def test_curve_surface_both_orders(self):
        """Preserve the point where an edge crosses a face in either input order."""
        line = self.feature(Part.makeLine(App.Vector(5, 5, -5), App.Vector(5, 5, 5)))
        plane = self.feature(Part.makePlane(10, 10))
        for inputs in ((line, plane), (plane, line)):
            with self.subTest(reverse=inputs[0] == plane):
                curve = self.intersection(*inputs)
                self.assertNotIn("Invalid", curve.State)
                self.assertEqual(len(curve.Shape.Edges), 0)
                self.assertEqual(len(curve.Shape.Vertexes), 1)
                self.assertLess((curve.Shape.Vertexes[0].Point - App.Vector(5, 5, 0)).Length, 1e-6)

    def test_datum_plane_and_curve(self):
        """Accept an unbounded datum plane and retain its intersection with an edge."""
        plane = self.doc.addObject("App::Plane", "DatumPlane")
        line = self.feature(Part.makeLine(App.Vector(5, 5, -5), App.Vector(5, 5, 5)))
        curve = self.intersection(line, plane)
        self.assertNotIn("Invalid", curve.State)
        self.assertEqual(len(curve.Shape.Vertexes), 1)
        self.assertLess((curve.Shape.Vertexes[0].Point - App.Vector(5, 5, 0)).Length, 1e-6)

    def test_datum_plane_plane(self):
        """Keep the unbounded intersection line of two perpendicular datum planes."""
        first = self.doc.addObject("App::Plane", "FirstDatumPlane")
        second = self.doc.addObject("App::Plane", "SecondDatumPlane")
        second.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        curve = self.intersection(first, second)
        self.assertNotIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isValid())
        self.assertEqual(len(curve.Shape.Edges), 1)
        self.assertEqual(len(curve.Shape.Vertexes), 0)
        self.assertAlmostEqual(abs(curve.Shape.Edges[0].Curve.Direction.x), 1, places=6)

    def test_direct_curves_and_mode_recompute(self):
        """Intersect coplanar edges directly and recompute when the mode changes."""
        first = self.feature(Part.makeLine(App.Vector(), App.Vector(10, 10, 0)))
        second = self.feature(Part.makeLine(App.Vector(0, 10, 0), App.Vector(10, 0, 0)))
        curve = self.intersection(first, second, "Direct")
        self.assertNotIn("Invalid", curve.State)
        self.assertEqual(len(curve.Shape.Vertexes), 1)
        self.assertLess((curve.Shape.Vertexes[0].Point - App.Vector(5, 5, 0)).Length, 1e-6)
        curve.Mode = "ExtrudedProfiles"
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        curve.Mode = "Direct"
        self.doc.recompute()
        self.assertNotIn("Invalid", curve.State)

    def test_subelements_of_same_object(self):
        """Resolve distinct faces of one object and reject invalid references."""
        box = self.feature(Part.makeBox(10, 10, 10))
        curve = self.intersection((box, ["Face1"]), (box, ["Face3"]))
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, 10, places=6)
        curve.Curve2 = (box, ["Face1"])
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        curve.Curve2 = (box, ["MissingFace"])
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        curve.Curve2 = (box, ["Face2", "Face3"])
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)

    def test_selected_edges_intersect_directly(self):
        """Treat selected edges as direct geometry in Automatic mode."""
        shape = Part.makeCompound(
            [
                Part.makeLine(App.Vector(), App.Vector(10, 10, 0)),
                Part.makeLine(App.Vector(0, 10, 0), App.Vector(10, 0, 0)),
            ]
        )
        obj = self.feature(shape)
        curve = self.intersection((obj, ["Edge1"]), (obj, ["Edge2"]))
        self.assertNotIn("Invalid", curve.State)
        self.assertEqual(len(curve.Shape.Vertexes), 1)

    @unittest.skipUnless(App.GuiUp, "Requires the GUI command")
    def test_gui_command_selection(self):
        """Create intersections from whole, mixed, and same-object selections."""
        import FreeCADGui as Gui

        __import__("SurfaceGui")
        box = self.feature(Part.makeBox(10, 10, 10))
        plane = self.feature(Part.makePlane(20, 20, App.Vector(-5, -5, 5)))
        for inputs, length in (
            (((box, "Face1"), (box, "Face3")), 10),
            (((box, ""), (plane, "")), 40),
            (((box, "Face1"), (plane, "")), 10),
        ):
            with self.subTest(inputs=inputs):
                Gui.Selection.clearSelection()
                for obj, sub in inputs:
                    Gui.Selection.addSelection(obj, sub)
                before = set(self.doc.Objects)
                Gui.runCommand("Surface_IntersectionCurve", 0)
                added = set(self.doc.Objects) - before
                self.assertEqual(len(added), 1)
                curve = added.pop()
                self.assertCurve(curve)
                self.assertAlmostEqual(curve.Shape.Length, length, places=5)
                self.doc.removeObject(curve.Name)
        Gui.Selection.clearSelection()

    def test_mixed_wire_and_point_result(self):
        """Retain isolated points alongside intersection wires."""
        first = self.feature(
            Part.makeCompound(
                [
                    Part.makePlane(10, 10),
                    Part.makeLine(App.Vector(15, 0, 0), App.Vector(15, 10, 0)),
                ]
            )
        )
        second = self.feature(Part.makePlane(20, 10))
        second.Placement = App.Placement(
            App.Vector(0, 5, -5), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        curve = self.intersection(first, second)
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, 10, places=6)
        self.assertEqual(len(curve.Shape.Vertexes), 3)

    def test_link_placement_and_selected_face_restore(self):
        """Resolve a linked face in document coordinates and restore its reference."""
        surface = self.feature(Part.makeCylinder(5, 10))
        link = self.doc.addObject("App::Link", "SurfaceLink")
        link.LinkedObject = surface
        link.LinkPlacement.Base = App.Vector(20, 0, 0)
        plane = self.feature(Part.makePlane(20, 20, App.Vector(10, -10, 5)))
        curve = self.intersection((link, ["Face1"]), plane)
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.CenterOfMass.x, 20, places=5)
        self.assertAlmostEqual(curve.Shape.Length, 10 * 3.141592653589793, places=5)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "SelectedIntersection.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.IntersectionCurve.touch()
            self.doc.recompute()
            self.assertCurve(self.doc.IntersectionCurve)
            self.assertEqual(self.doc.IntersectionCurve.Curve1[1], ["Face1"])
            App.closeDocument(self.doc.Name)
            self.doc = App.newDocument("TestIntersectionCurve")

    def test_restore_legacy_object_links(self):
        """Migrate old whole-object links and default to the original extrusion mode."""
        self.profiles()
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "LegacyIntersection.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            with zipfile.ZipFile(path) as archive:
                files = {name: archive.read(name) for name in archive.namelist()}
            root = ET.fromstring(files["Document.xml"])
            for obj in root.findall("./ObjectData/Object"):
                if obj.get("name") == "IntersectionCurve":
                    props = obj.find("Properties")
                    for prop in list(props):
                        if prop.get("name") in ("Curve1", "Curve2"):
                            value = prop.find("LinkSub").get("value")
                            prop.set("type", "App::PropertyLink")
                            prop.remove(prop.find("LinkSub"))
                            ET.SubElement(prop, "Link", value=value)
                        elif prop.get("name") == "Mode":
                            props.remove(prop)
                    props.set("Count", str(len(props.findall("Property"))))
            ET.indent(root)
            files["Document.xml"] = (
                ET.tostring(root, encoding="utf-8", xml_declaration=True) + b"\n"
            )
            with zipfile.ZipFile(path, "w") as archive:
                for name, data in files.items():
                    archive.writestr(name, data)
            self.doc = App.openDocument(path)
            self.doc.IntersectionCurve.touch()
            self.doc.recompute()
            self.assertCurve(self.doc.IntersectionCurve)
            self.assertEqual(self.doc.IntersectionCurve.Mode, "Automatic")
            self.assertEqual(self.doc.IntersectionCurve.Curve1[0], self.doc.First)
            self.doc.Second.Placement.Base = App.Vector(0, 0, 5)
            self.doc.recompute()
            self.assertAlmostEqual(self.doc.IntersectionCurve.Shape.BoundBox.ZMin, 5, places=5)
            App.closeDocument(self.doc.Name)
            self.doc = App.newDocument("TestIntersectionCurve")

    def test_save_restore(self):
        """Restore linked profiles and recompute a saved intersection."""
        self.profiles()
        element_map = self.doc.IntersectionCurve.Shape.ElementMap
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "Intersection.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.Second.Placement.Base = App.Vector(0, 0, 5)
            self.doc.recompute()
            self.assertCurve(self.doc.IntersectionCurve)
            self.assertEqual(self.doc.IntersectionCurve.Shape.ElementMap, element_map)
            self.assertAlmostEqual(self.doc.IntersectionCurve.Shape.BoundBox.ZMin, 5, places=5)
            # Release the document's file handles before removing the temporary directory.
            App.closeDocument(self.doc.Name)
            self.doc = App.newDocument("TestIntersectionCurve")
