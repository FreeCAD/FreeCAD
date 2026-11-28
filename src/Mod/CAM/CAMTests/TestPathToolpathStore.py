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

"""Tests for the stored form of a Toolpath in a FreeCAD document."""

import os
import shutil
import tempfile
import zipfile

import FreeCAD
import Path
from CAMTests.PathTestUtils import PathTestBase

RESOLUTION = 1e-6


class TestPathToolpathStore(PathTestBase):
    """Save a document with a Toolpath, open it again, and compare."""

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.docs = []

    def tearDown(self):
        for name in self.docs:
            if name in FreeCAD.listDocuments():
                FreeCAD.closeDocument(name)
        shutil.rmtree(self.dir, ignore_errors=True)

    def _saveAndOpen(self, path):
        """Store `path` on a Path::Feature, save, and return (restored Path, file name)."""
        doc = FreeCAD.newDocument("ToolpathStoreSave")
        self.docs.append(doc.Name)
        obj = doc.addObject("Path::Feature", "Stored")
        obj.Path = path
        fileName = os.path.join(self.dir, "store.FCStd")
        doc.saveAs(fileName)
        FreeCAD.closeDocument(doc.Name)
        restored = FreeCAD.openDocument(fileName)
        self.docs.append(restored.Name)
        return restored.Stored.Path, fileName

    def _mixedCommands(self):
        return [
            Path.Command("(a comment)"),
            Path.Command("G90"),
            Path.Command("M6", {"T": 3}, {"tool_change_format": "T_M6", "next_tool": "4"}),
            Path.Command("G0", {"X": 1.25, "Y": -2.5, "Z": 10}),
            Path.Command(
                "G1",
                {"X": 1 / 3, "Y": 2 / 3, "Z": -0.1234567891, "F": 16.6667},
                {"type": "linking", "weight": 0.25},
            ),
            Path.Command("G2", {"X": 5, "Y": 5, "I": 2.5, "J": 0, "K": 0, "F": 10}),
            Path.Command("G81", {"X": 10, "Y": 10, "Z": -12, "R": 3, "F": 2.5}),
            Path.Command("G1", {"A": 45.5, "B": -10, "X": -123456.654321}),
            Path.Command("G80"),
            Path.Command("M5"),
        ]

    def test00(self):
        """A mixed Toolpath comes back with the same commands."""
        commands = self._mixedCommands()
        restored, _ = self._saveAndOpen(Path.Path(commands))
        back = restored.Commands
        self.assertEqual(len(back), len(commands))
        for original, copy in zip(commands, back):
            self.assertEqual(copy.Name, original.Name)
            self.assertEqual(set(copy.Parameters), set(original.Parameters))
            for key, value in original.Parameters.items():
                self.assertRoughly(copy.Parameters[key], value, 0.5 * RESOLUTION + 1e-12)
            self.assertEqual(set(copy.Annotations), set(original.Annotations))
            for key, value in original.Annotations.items():
                if isinstance(value, str):
                    self.assertEqual(copy.Annotations[key], value)
                else:
                    self.assertRoughly(copy.Annotations[key], value, 0.5 * RESOLUTION + 1e-12)

    def test01(self):
        """The G-code text of a restored Toolpath is the text of the original."""
        path = Path.Path(self._mixedCommands())
        restored, _ = self._saveAndOpen(path)
        self.assertEqual(restored.toGCode(), path.toGCode())

    def test02(self):
        """The document holds the binary form, not G-code text."""
        _, fileName = self._saveAndOpen(Path.Path(self._mixedCommands()))
        with zipfile.ZipFile(fileName) as archive:
            names = archive.namelist()
            self.assertIn("Stored.ncb", names)
            self.assertNotIn("Stored.nc", names)
            self.assertEqual(archive.read("Stored.ncb")[:4], b"FCTP")

    def test03(self):
        """An empty Toolpath comes back empty."""
        restored, _ = self._saveAndOpen(Path.Path())
        self.assertEqual(restored.Size, 0)

    def test04(self):
        """Many small steps come back with no accumulated rounding error."""
        commands = [Path.Command("G1", {"X": i / 3.0, "Y": i * 0.1, "F": 100}) for i in range(3000)]
        restored, _ = self._saveAndOpen(Path.Path(commands))
        back = restored.Commands
        self.assertEqual(len(back), len(commands))
        self.assertRoughly(back[-1].Parameters["X"], 2999 / 3.0, 0.5 * RESOLUTION + 1e-12)
        self.assertRoughly(back[-1].Parameters["Y"], 299.9, 0.5 * RESOLUTION + 1e-12)

    def test05(self):
        """A second save of a restored document gives the same stored bytes."""
        _, fileName = self._saveAndOpen(Path.Path(self._mixedCommands()))
        with zipfile.ZipFile(fileName) as archive:
            first = archive.read("Stored.ncb")
        doc = FreeCAD.openDocument(fileName)
        self.docs.append(doc.Name)
        again = os.path.join(self.dir, "again.FCStd")
        doc.saveAs(again)
        with zipfile.ZipFile(again) as archive:
            self.assertEqual(archive.read("Stored.ncb"), first)

    def test10(self):
        """A document that holds a Toolpath as G-code text still opens."""
        source = FreeCAD.getHomePath() + "Mod/CAM/CAMTests/boxtest.fcstd"
        with zipfile.ZipFile(source) as archive:
            lines = [
                line for line in archive.read("Profile.nc").decode().splitlines() if line.strip()
            ]
        self.assertGreater(len(lines), 0)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        try:
            doc = FreeCAD.openDocument(source)
        finally:
            FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")
        self.docs.append(doc.Name)
        commands = doc.Profile.Path.Commands
        self.assertEqual(len(commands), len(lines))
        self.assertEqual(commands[0].toGCode(), Path.Command(lines[0]).toGCode())
        self.assertEqual(commands[-1].toGCode(), Path.Command(lines[-1]).toGCode())

    def test11(self):
        """A text line that is not valid G-code does not discard the other commands."""
        _, fileName = self._saveAndOpen(Path.Path([Path.Command("G0", {"X": 1})]))
        # Make a document in the form of before the binary store, with one bad line.
        oldForm = os.path.join(self.dir, "old.FCStd")
        with zipfile.ZipFile(fileName) as source, zipfile.ZipFile(oldForm, "w") as target:
            for item in source.infolist():
                if item.filename == "Stored.ncb":
                    continue
                content = source.read(item.filename)
                if item.filename == "Document.xml":
                    self.assertIn(b"Stored.ncb", content)
                    content = content.replace(b"Stored.ncb", b"Stored.nc")
                target.writestr(item, content)
            target.writestr(
                "Stored.nc",
                "(first)\nta comment\nG0 X1.000000 Y2.000000\nG1 F100.000000 X5.000000\n",
            )
        doc = FreeCAD.openDocument(oldForm)
        self.docs.append(doc.Name)
        commands = doc.Stored.Path.Commands
        self.assertEqual(
            [c.Name for c in commands],
            ["(first)", "(unreadable line: ta comment)", "G0", "G1"],
        )
        self.assertRoughly(commands[3].Parameters["X"], 5.0)
