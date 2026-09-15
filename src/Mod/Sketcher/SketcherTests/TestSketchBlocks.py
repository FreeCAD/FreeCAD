# SPDX-License-Identifier: LGPL-2.1-or-later

import math
from pathlib import Path
import tempfile
import unittest

import FreeCAD as App
import Part
import Sketcher
import SketcherBlock


class TestSketchBlocks(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.file = Path(self.directory.name) / "block 'é.txt"
        self.doc = App.newDocument("SketchBlocksTest")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.write()

    def tearDown(self):
        App.closeDocument(self.doc.Name)
        self.directory.cleanup()

    def write(self, replacement=False):
        geometries = (
            ["Part.Circle(App.Vector(5, 5, 0), App.Vector(0, 0, 1), 5)"]
            if replacement
            else [
                "Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0))",
                "Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 5, 0))",
            ]
        )
        text = "# Copied from sketcher. From:\nlastGeoId = len(objectStr.Geometry)\ngeoList = []\n"
        text += "\n".join("geoList.append(" + geo + ")" for geo in geometries)
        text += "\nobjectStr.addGeometry(geoList, False)\ndel geoList\nconstraintList = []\n"
        self.file.write_text(text, encoding="utf-8")

    def insert(self):
        return SketcherBlock.insert_geometry(self.sketch, SketcherBlock.read(self.file), self.file)

    def snapshot(self):
        return [geo.Content for geo in self.sketch.Geometry]

    def testMetadataAndLegacyGroup(self):
        legacy = Sketcher.Constraint("Group", [0, 0, 1, 0])
        self.assertEqual(legacy.File, "")
        self.assertFalse(legacy.FileHeight)
        group = Sketcher.Constraint("Group", [0, 0, 1, 0], str(self.file), True, False)
        self.assertEqual(group.File, str(self.file))
        self.assertTrue(group.FileHeight)
        self.assertFalse(group.IsActive)
        group.File = ""
        self.assertEqual(group.File, "")
        self.assertFalse(group.FileHeight)

    def testReloadPreservesHandleDimensionsAndExpressions(self):
        s = self.sketch
        unrelated = s.addGeometry(Part.Circle(App.Vector(-5, -5, 0), App.Vector(0, 0, 1), 1))
        index = self.insert()
        group = s.Constraints[index]
        handle = group.First
        # This dimension is dormant under the Group and must disappear on replacement.
        s.addConstraint(Sketcher.Constraint("Distance", group.Second, 10.0))
        length = s.addConstraint(Sketcher.Constraint("Distance", handle, 20.0))
        s.renameConstraint(length, "BlockWidth")
        s.addProperty("App::PropertyLength", "DesiredWidth")
        s.DesiredWidth = 20
        s.setExpression("Constraints.BlockWidth", "DesiredWidth")
        s.addConstraint(Sketcher.Constraint("Angle", handle, math.pi / 2))
        self.assertEqual(s.solve(), 0)
        before = s.Geometry[handle]
        outside = s.Geometry[unrelated].Content
        self.write(True)
        result = SketcherBlock.reload(s, index)
        self.assertEqual(s.solve(), 0)
        updated = s.Constraints[result]
        self.assertEqual(updated.Name, group.Name)
        self.assertEqual(updated.File, group.File)
        after = s.Geometry[updated.First]
        self.assertLess((after.StartPoint - before.StartPoint).Length, 1e-6)
        self.assertLess((after.EndPoint - before.EndPoint).Length, 1e-6)
        self.assertEqual(s.Geometry[unrelated].Content, outside)
        self.assertEqual(s.GeometryCount, 3)
        self.assertAlmostEqual(s.Geometry[updated.Second].Radius, 10, places=5)
        s.DesiredWidth = 30
        self.doc.recompute()
        self.assertAlmostEqual(s.Geometry[s.Constraints[result].Second].Radius, 15, places=5)

    def testHeightAndConstruction(self):
        index = self.insert()
        s = self.sketch
        constraints = s.Constraints
        group = constraints[index]
        group.FileHeight = True
        constraints[index] = group
        s.Constraints = constraints
        s.setConstruction(group.Second, True)
        s.setConstruction(group.Third, True)
        self.write(True)
        result = SketcherBlock.reload(s, index)
        self.assertEqual(s.solve(), 0)
        self.assertTrue(s.getConstruction(s.Constraints[result].Second))
        self.assertTrue(s.Constraints[result].FileHeight)

    def testReloadUndoRedo(self):
        index = self.insert()
        before = self.snapshot()
        self.write(True)
        self.doc.openTransaction("Reload block")
        SketcherBlock.reload(self.sketch, index)
        self.doc.commitTransaction()
        self.assertEqual(self.sketch.solve(), 0)
        after = self.snapshot()
        self.doc.undo()
        self.assertEqual(self.snapshot(), before)
        self.doc.redo()
        self.assertEqual(self.snapshot(), after)

    def testSaveRestoreWithoutSource(self):
        index = self.insert()
        target = str(Path(self.directory.name) / "block.FCStd")
        self.doc.recompute()
        self.doc.saveAs(target)
        App.closeDocument(self.doc.Name)
        self.file.unlink()
        self.doc = App.openDocument(target)
        self.sketch = self.doc.Sketch
        self.assertEqual(self.sketch.Constraints[index].File, str(self.file))
        self.assertEqual(self.sketch.solve(), 0)
        before = self.snapshot()
        with self.assertRaises(FileNotFoundError):
            SketcherBlock.reload(self.sketch, index)
        self.assertEqual(self.snapshot(), before)

    def testReloadPreservesOtherGroup(self):
        index = self.insert()
        s = self.sketch
        other_index = self.insert()
        self.assertEqual(s.solve(), 0)
        other = s.Constraints[other_index]
        before = [s.Geometry[i].Content for i in (other.First, other.Second, other.Third)]
        self.write(True)
        SketcherBlock.reload(s, index)
        self.assertEqual(s.solve(), 0)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        other = s.Constraints[other_index]
        self.assertEqual(
            [s.Geometry[i].Content for i in (other.First, other.Second, other.Third)], before
        )
        self.assertEqual(other.File, str(self.file))
        self.assertEqual(s.GeometryCount, 5)
        self.assertAlmostEqual(s.Geometry[s.Constraints[index].Second].Radius, 5, places=5)

    def testInvalidSourceLeavesGroupUnchanged(self):
        index = self.insert()
        before = self.snapshot()
        for text in (
            "",
            "# Copied from sketcher.\nimport os\n",
            "# Copied from sketcher.\nobjectStr.Document.clearUndos()\n",
            "# Copied from sketcher.\ngeoList=[]\n",
        ):
            with self.subTest(text=text):
                self.file.write_text(text, encoding="utf-8")
                with self.assertRaises(ValueError):
                    SketcherBlock.reload(self.sketch, index)
                self.assertEqual(self.snapshot(), before)
        with self.assertRaises(ValueError):
            self.sketch.replaceGroupGeometry(index, [])
        self.assertEqual(self.snapshot(), before)

    def testLibrary(self):
        directory = Path(App.getResourceDir()) / "Mod/Sketcher/Blocks"
        files = list(directory.glob("*.txt"))
        self.assertEqual(len(files), 9)
        for filename in files:
            with self.subTest(filename=filename.name):
                geometry = SketcherBlock.read(filename)
                self.assertTrue(geometry)
                self.assertTrue(all(geo.toShape().isValid() for geo in geometry))
