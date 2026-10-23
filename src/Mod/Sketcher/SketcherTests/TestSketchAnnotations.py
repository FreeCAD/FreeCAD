# SPDX-License-Identifier: LGPL-2.1-or-later
import os
import tempfile
import unittest
import FreeCAD as App
import Part


class TestSketchAnnotations(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("SketchAnnotations")
        self.doc.UndoMode = 1
        self.s = self.doc.addObject("Sketcher::SketchObject", "Sketch")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def text(self, **fields):
        return self.s.addAnnotation(dict(Type="Text", Html="<b>Note α</b>", **fields))

    def square(self, x0=0, y0=0, size=20):
        pts = [
            App.Vector(x0 + x, y0 + y, 0) for x, y in ((0, 0), (size, 0), (size, size), (0, size))
        ]
        indices = self.s.addGeometry([Part.LineSegment(pts[i], pts[(i + 1) % 4]) for i in range(4)])
        return [self.s.GeometryFacadeList[i].Id for i in indices]

    def testSeparateFromGeometryAndSolver(self):
        self.square()
        self.doc.recompute()
        before = (
            self.s.GeometryCount,
            self.s.ConstraintCount,
            self.s.FullyConstrained,
            self.s.Shape.Length,
            self.s.Shape.Area,
        )
        ident = self.text()
        self.s.updateAnnotation(ident, {"Position": (3.0, 5.0, 0.0), "Construction": True})
        self.s.addAnnotation({"Type": "Leader", "Points": [(0.0, 0.0, 0.0), (4.0, 5.0, 0.0)]})
        self.s.delAnnotations([ident])
        self.doc.recompute()
        self.assertEqual(
            before,
            (
                self.s.GeometryCount,
                self.s.ConstraintCount,
                self.s.FullyConstrained,
                self.s.Shape.Length,
                self.s.Shape.Area,
            ),
        )

    def testSnapshotsAndValidationAreAtomic(self):
        ident = self.text()
        snapshot = self.s.Annotations
        snapshot[0]["Html"] = "changed"
        self.assertNotEqual(self.s.Annotations[0]["Html"], "changed")
        with self.assertRaises((AttributeError, TypeError)):
            self.s.Annotations = []
        for change in (
            {"Position": (0.0, 0.0, 1.0)},
            {"TextSize": 0},
            {"Id": 5},
            {"Type": "Dimension"},
            {"Rotation": float("nan")},
            {"Points": [(0.0, 0.0, 0.0)], "Type": "Leader"},
            {"Unknown": True},
        ):
            with self.assertRaises((ValueError, TypeError)):
                self.s.updateAnnotation(ident, change)
        self.assertEqual(len(self.s.Annotations), 1)
        self.assertEqual(self.s.Annotations[0]["Type"], "Text")
        with self.assertRaises(ValueError):
            self.s.delAnnotations([ident, 999])
        self.assertEqual(len(self.s.Annotations), 1)

    def testUndoRedoAndStableIds(self):
        first = self.text()
        self.doc.openTransaction("Add annotation")
        second = self.text()
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual([a["Id"] for a in self.s.Annotations], [first])
        self.doc.redo()
        self.assertEqual([a["Id"] for a in self.s.Annotations], [first, second])
        self.s.delAnnotations([first])
        self.assertGreater(self.text(), second)

    def testRoundTrip(self):
        html = '<p title="quotes &amp;">Étage α\n<b>bold</b><br/>line</p>'
        self.s.addAnnotation(
            {
                "Type": "Text",
                "Html": html,
                "Position": (1.234567891, 2.987654321, 0.0),
                "TextWidth": 25.125,
                "Label": "Notes α",
            }
        )
        self.s.addAnnotation(
            {
                "Type": "Leader",
                "Points": [(1.0, 2.0, 0.0), (3.0, 4.0, 0.0), (5.0, 2.0, 0.0)],
                "ArrowStyle": "Dot",
            }
        )
        refs = self.square()
        self.s.addAnnotation({"Type": "Hatch", "Boundary": refs, "Pattern": "ANSI37"})
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "annotations.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            restored = self.doc = App.openDocument(path)
            try:
                values = restored.Sketch.Annotations
                self.assertEqual(values[0]["Html"], html)
                self.assertAlmostEqual(values[0]["Position"].x, 1.234567891, places=9)
                self.assertEqual(len(values[1]["Points"]), 3)
                self.assertEqual(values[1]["ArrowStyle"], "Dot")
                self.assertEqual(values[2]["Boundary"], refs)
                self.assertEqual(values[2]["Pattern"], "ANSI37")
                self.assertAlmostEqual(
                    restored.Sketch.getAnnotationFace(values[2]["Id"]).Area, 400.0
                )
            finally:
                pass

    def testHatchHoleAndClipping(self):
        refs = self.square() + self.square(5, 5, 10)
        ident = self.s.addAnnotation(
            {"Type": "Hatch", "Boundary": refs, "Spacing": 2.0, "Rotation": 0.0}
        )
        self.assertAlmostEqual(self.s.getAnnotationFace(ident).Area, 300.0)
        strokes = self.s.getAnnotationStrokes(ident)
        self.assertGreater(len(strokes), 0)
        for p, q in zip(strokes[::2], strokes[1::2]):
            middle = (p + q) * 0.5
            self.assertFalse(5.0001 < middle.x < 14.9999 and 5.0001 < middle.y < 14.9999)
        self.s.updateAnnotation(ident, {"Position": (1.0e20, 1.0e20, 0.0)})
        self.assertGreater(len(self.s.getAnnotationStrokes(ident)), 0)

    def testStableHatchBoundaryAndBrokenReferences(self):
        self.s.addGeometry(Part.LineSegment(App.Vector(-10, 0, 0), App.Vector(-5, 0, 0)))
        refs = self.square()
        ident = self.s.addAnnotation({"Type": "Hatch", "Boundary": refs})
        self.s.delGeometry(0)
        self.assertAlmostEqual(self.s.getAnnotationFace(ident).Area, 400.0)
        self.doc.openTransaction("Remove boundary")
        self.s.delGeometry(0)
        self.doc.commitTransaction()
        with self.assertRaises(ValueError):
            self.s.getAnnotationFace(ident)
        self.assertEqual(self.s.Annotations[0]["Boundary"], refs)
        self.doc.undo()
        self.assertAlmostEqual(self.s.getAnnotationFace(ident).Area, 400.0)

    def testOpenBoundaryRejected(self):
        refs = self.square()
        with self.assertRaises(ValueError):
            self.s.addAnnotation({"Type": "Hatch", "Boundary": refs[:-1]})
        self.assertEqual(self.s.Annotations, [])

    def testHatchPatterns(self):
        refs = self.square()
        ident = self.s.addAnnotation({"Type": "Hatch", "Boundary": refs, "Spacing": 2.0})
        self.assertEqual(self.s.Annotations[0]["Pattern"], "ANSI31")
        solid = len(self.s.getAnnotationStrokes(ident))
        for name in (
            "ANSI31",
            "ANSI32",
            "ANSI33",
            "ANSI34",
            "ANSI35",
            "ANSI36",
            "ANSI37",
            "ANSI38",
            "NET",
            "BRICK",
            "EARTH",
            "HBONE",
            "CROSS",
            "HONEY",
            "INSUL",
            "DOTS",
            "AR-CONC",
            "AR-SAND",
        ):
            self.s.updateAnnotation(ident, {"Pattern": name})
            strokes = self.s.getAnnotationStrokes(ident)
            self.assertGreater(len(strokes), 0, name)
            self.assertEqual(len(strokes) % 2, 0, name)
            for p in strokes:
                self.assertTrue(-1e-6 <= p.x <= 20 + 1e-6 and -1e-6 <= p.y <= 20 + 1e-6, name)
        # Dashed families split the clipped lines into more strokes.
        self.s.updateAnnotation(ident, {"Pattern": "ANSI33"})
        self.assertGreater(len(self.s.getAnnotationStrokes(ident)), solid // 2)
        self.s.updateAnnotation(ident, {"Pattern": "ANSI32", "Rotation": 10.0})
        families = self.s.getAnnotationPattern(ident)
        self.assertEqual(len(families), 2)
        for family in families:
            self.assertAlmostEqual(family["Direction"].getAngle(App.Vector(1, 0, 0)), 0.959931, 5)
            self.assertAlmostEqual(abs(family["Offset"].Length), 6.0, 6)
        with self.assertRaises(ValueError):
            self.s.updateAnnotation(ident, {"Pattern": "NoSuchPattern"})
        self.assertEqual(self.s.Annotations[0]["Pattern"], "ANSI32")

    def testLegacyCrosshatchIsMigrated(self):
        import re
        import zipfile

        refs = self.square()
        self.s.addAnnotation({"Type": "Hatch", "Boundary": refs, "Rotation": 0.0})
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "legacy.FCStd")
            legacy = os.path.join(directory, "legacy-old.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            with zipfile.ZipFile(path) as source, zipfile.ZipFile(legacy, "w") as target:
                for item in source.infolist():
                    data = source.read(item.filename)
                    if item.filename == "Document.xml":
                        text = data.decode("utf-8")
                        element = re.search(r'<Annotation id="[^>]*type="Hatch"[^>]*>', text).group(
                            0
                        )
                        old = element.replace('pattern="ANSI31"', 'crosshatch="1"')
                        old = re.sub(r'rotation="[^"]*"', 'rotation="45"', old)
                        data = text.replace(element, old).encode("utf-8")
                    target.writestr(item, data)
            self.doc = App.openDocument(legacy)
            hatch = self.doc.Sketch.Annotations[0]
            self.assertEqual(hatch["Pattern"], "ANSI37")
            self.assertAlmostEqual(hatch["Rotation"], 0.0)

    def testLeaderArrowStyles(self):
        ident = self.s.addAnnotation(
            {"Type": "Leader", "Points": [(0.0, 0.0, 0.0), (10.0, 0.0, 0.0)], "ArrowSize": 2.0}
        )
        self.assertEqual(self.s.Annotations[0]["ArrowStyle"], "Open arrow")
        for style in (
            "Filled arrow",
            "Open arrow",
            "Tick",
            "Dot",
            "Open circle",
            "Fork",
            "Filled triangle",
            "None",
        ):
            self.s.updateAnnotation(ident, {"ArrowStyle": style})
            strokes = self.s.getAnnotationStrokes(ident)
            self.assertEqual(len(strokes) % 2, 0, style)
            for p in strokes:
                self.assertLessEqual(p.Length, 10.0 + 1e-9, style)
        self.s.updateAnnotation(ident, {"ArrowStyle": "None"})
        self.assertEqual(len(self.s.getAnnotationStrokes(ident)), 2)
        with self.assertRaises(ValueError):
            self.s.updateAnnotation(ident, {"ArrowStyle": "Harpoon"})
        self.assertEqual(self.s.Annotations[0]["ArrowStyle"], "None")

    def testSnapshotCopyAndTextValidation(self):
        ident = self.text()
        # A snapshot of an existing annotation can be added again; it gets a new ID.
        copy = self.s.addAnnotation(self.s.Annotations[0])
        self.assertNotEqual(copy, ident)
        with self.assertRaises(ValueError):
            self.s.updateAnnotation(ident, {"Id": copy})
        # Control characters cannot be written to the document and are refused.
        with self.assertRaises(ValueError):
            self.s.updateAnnotation(ident, {"Label": "bad\x01name"})
        self.doc.openTransaction("Nothing")
        self.s.delAnnotations([])
        self.doc.commitTransaction()
        self.assertEqual(len(self.s.Annotations), 2)

    def testLeaderStrokes(self):
        ident = self.s.addAnnotation(
            {"Type": "Leader", "Points": [(0.0, 0.0, 0.0), (10.0, 0.0, 0.0), (10.0, 10.0, 0.0)]}
        )
        points = self.s.getAnnotationStrokes(ident)
        self.assertEqual(len(points), 8)
        self.assertEqual(points[0], App.Vector(0, 0, 0))
        self.assertEqual(points[3], App.Vector(10, 10, 0))

    def testIntegerCoordinatesAndStableSubelement(self):
        ident = self.s.addAnnotation(
            {"Type": "Text", "Html": "Integer coordinates", "Position": (1, 2, 0), "TextSize": 3}
        )
        data = self.s.getSubObject("Annotation" + str(ident))
        self.assertEqual(data["Id"], ident)
        self.assertEqual(data["Position"], App.Vector(1, 2, 0))
        with self.assertRaises(ValueError):
            self.s.updateAnnotation(ident, {"Type": "Leader", "Points": [(0, 0, 0), (1, 1, 0)]})
        copy = self.doc.copyObject(self.s)
        self.assertEqual(copy.Annotations, self.s.Annotations)

    def testUndoDoesNotReuseLinkedIdentity(self):
        self.doc.openTransaction("Temporary note")
        first = self.text()
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(self.s.Annotations, [])
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "after-undo.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.s = self.doc.Sketch
            second = self.s.addAnnotation({"Type": "Leader", "Points": [(0, 0, 0), (1, 1, 0)]})
            self.assertGreater(second, first)
