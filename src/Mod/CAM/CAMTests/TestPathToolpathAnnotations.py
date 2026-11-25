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

"""Tests for the annotations of a Toolpath as a whole (operation level)."""

import os
import shutil
import tempfile

import FreeCAD
import Path
import Path.Main.Job as PathJob
from Path.Op import Custom
from Path.Post import PostList
from CAMTests.PathTestUtils import PathTestBase


class TestPathToolpathAnnotations(PathTestBase):
    """Path.Path.Annotations: get, set, copy and document storage."""

    def _path(self):
        return Path.Path([Path.Command("G0", {"X": 1}), Path.Command("G1", {"X": 2, "F": 100})])

    def test00(self):
        """A new Toolpath has no annotations."""
        self.assertEqual(Path.Path().Annotations, {})
        self.assertEqual(self._path().Annotations, {})
        self.assertEqual(Path.Path("G0 X1\nG1 X2").Annotations, {})

    def test01(self):
        """An assignment replaces all items; a value is a string or a number."""
        path = self._path()
        path.Annotations = {"strategy": "waterline", "stepover": 0.25, "passes": 3}
        self.assertEqual(
            path.Annotations, {"strategy": "waterline", "stepover": 0.25, "passes": 3.0}
        )
        self.assertIsInstance(path.Annotations["passes"], float)
        path.Annotations = {"other": "x"}
        self.assertEqual(path.Annotations, {"other": "x"})
        path.Annotations = {}
        self.assertEqual(path.Annotations, {})

    def test02(self):
        """The attribute returns a copy."""
        path = self._path()
        path.Annotations = {"a": "1"}
        got = path.Annotations
        got["b"] = "2"
        self.assertEqual(path.Annotations, {"a": "1"})

    def test03(self):
        """addAnnotations() adds items, keeps the others, and returns the path."""
        path = self._path()
        path.Annotations = {"a": "1", "b": "2"}
        result = path.addAnnotations({"b": "changed", "c": 3})
        self.assertIs(result, path)
        self.assertEqual(path.Annotations, {"a": "1", "b": "changed", "c": 3.0})

    def test04(self):
        """A refused item leaves the annotations as they were."""
        path = self._path()
        path.Annotations = {"a": "1"}
        with self.assertRaises(TypeError):
            path.Annotations = {"ok": "x", "bad": [1, 2]}
        with self.assertRaises(TypeError):
            path.addAnnotations({"ok": "x", 5: "bad key"})
        with self.assertRaises(TypeError):
            path.addAnnotations("not a dictionary")
        self.assertEqual(path.Annotations, {"a": "1"})

    def test05(self):
        """The annotations are independent of the commands and of the G-code text."""
        plain = self._path()
        path = self._path()
        path.Annotations = {"a": "1"}
        self.assertEqual(path.toGCode(), plain.toGCode())
        self.assertEqual(path.Size, 2)
        path.Commands = [Path.Command("G0", {"Z": 5})]
        self.assertEqual(path.Annotations, {"a": "1"})
        path.setFromGCode("G0 X9")
        self.assertEqual(path.Annotations, {"a": "1"})
        self.assertEqual(path.Commands[0].Annotations, {})

    def test06(self):
        """copy() and the assignment to a document object keep the annotations."""
        path = self._path()
        path.Annotations = {"a": "1", "n": 2.5}
        self.assertEqual(path.copy().Annotations, {"a": "1", "n": 2.5})
        # Path.Path(commands) makes a new path from the commands only
        self.assertEqual(Path.Path(path.Commands).Annotations, {})

        doc = FreeCAD.newDocument("ToolpathAnnotations")
        try:
            obj = doc.addObject("Path::Feature", "Stored")
            obj.Path = path
            self.assertEqual(obj.Path.Annotations, {"a": "1", "n": 2.5})
            # a method call and an assignment through obj.Path change the object
            obj.Path.addAnnotations({"added": "x"})
            self.assertEqual(obj.Path.Annotations, {"a": "1", "n": 2.5, "added": "x"})
            obj.Path.Annotations = {"only": "this"}
            self.assertEqual(obj.Path.Annotations, {"only": "this"})
            self.assertEqual(obj.Path.Size, 2)
            # a change to the returned dictionary does not
            obj.Path.Annotations["lost"] = "x"
            self.assertEqual(obj.Path.Annotations, {"only": "this"})
        finally:
            FreeCAD.closeDocument(doc.Name)

    def _saveAndOpen(self, path, check):
        folder = tempfile.mkdtemp()
        try:
            doc = FreeCAD.newDocument("ToolpathAnnotationsSave")
            obj = doc.addObject("Path::Feature", "Stored")
            obj.Path = path
            fileName = os.path.join(folder, "annotations.FCStd")
            doc.saveAs(fileName)
            FreeCAD.closeDocument(doc.Name)
            doc = FreeCAD.openDocument(fileName)
            try:
                check(doc.Stored.Path)
            finally:
                FreeCAD.closeDocument(doc.Name)
        finally:
            shutil.rmtree(folder, ignore_errors=True)

    def test10(self):
        """The annotations survive a document save and restore; numbers are exact."""
        path = self._path()
        path.Annotations = {"strategy": "waterline; fine", "ratio": 1 / 3, "count": 7}

        def check(restored):
            self.assertEqual(
                restored.Annotations, {"strategy": "waterline; fine", "ratio": 1 / 3, "count": 7.0}
            )
            self.assertEqual(restored.Size, 2)

        self._saveAndOpen(path, check)

    def test11(self):
        """A Toolpath with annotations and no commands is stored."""
        path = Path.Path()
        path.Annotations = {"note": "empty"}

        def check(restored):
            self.assertEqual(restored.Size, 0)
            self.assertEqual(restored.Annotations, {"note": "empty"})

        self._saveAndOpen(path, check)


class TestPathOperationAnnotations(PathTestBase):
    """An operation fills self.annotations in opExecute(); a post reads them."""

    @classmethod
    def setUpClass(cls):
        cls.doc = FreeCAD.newDocument("OperationAnnotations")
        box = cls.doc.addObject("Part::Box", "TestBox")
        cls.job = PathJob.Create("Job", [box])
        cls.op = Custom.Create(name="Custom", parentJob=cls.job)
        cls.op.Source = "Text"
        cls.op.Gcode = ["G0 X1 Y1", "G1 X2 F100"]
        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)

    def _executeWith(self, items):
        """Run the operation with an opExecute() that also adds `items`."""
        proxy = self.op.Proxy
        original = proxy.opExecute

        def opExecute(obj):
            original(obj)
            proxy.annotations.update(items)

        proxy.opExecute = opExecute
        try:
            proxy.execute(self.op)
        finally:
            del proxy.opExecute

    def test00(self):
        """An operation that adds no annotations has none on its Toolpath."""
        self._executeWith({})
        self.assertEqual(self.op.Path.Annotations, {})
        self.assertGreater(self.op.Path.Size, 0)

    def test01(self):
        """Items added in opExecute() go to obj.Path.Annotations."""
        self._executeWith({"strategy": "test", "passes": 2})
        self.assertEqual(self.op.Path.Annotations, {"strategy": "test", "passes": 2.0})

    def test02(self):
        """Each execute starts with no annotations."""
        self._executeWith({"first": "run"})
        self._executeWith({"second": "run"})
        self.assertEqual(self.op.Path.Annotations, {"second": "run"})

    def test03(self):
        """The Postable of an operation gives the annotations to the post."""
        self._executeWith({"strategy": "test"})
        postable = PostList._wrap_op(self.op)
        self.assertEqual(postable.data["annotations"], {"strategy": "test"})

        self._executeWith({})
        self.assertEqual(PostList._wrap_op(self.op).data["annotations"], {})
