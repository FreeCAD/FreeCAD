# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

import json
import math
import os
import pathlib
import tempfile
import unittest.mock

import FreeCAD
import Part

from Path.Tool.assets import AssetManager, FileStore
from Path.Tool.camassets import (
    TOOLHOLDER_TEMPLATE,
    asset_mapping,
    builtin_asset_mapping,
    ensure_toolholder_assets_initialized,
)
from Path.Tool.holder import ToolHolder, ToolHolderSerializer, available_holders
from Path.Tool.holder import sketch as holderSketch
from Path.Tool.toolbit import ToolBit
from Path.Tool.UpdateDocumentTools import diff_tool_setup
from CAMTests.PathTestUtils import PathTestWithAssets


class TestPathToolHolder(unittest.TestCase):
    """The tool holders FreeCAD ships, and those a user adds to the CAM asset folder."""

    tool_dir = pathlib.Path(os.path.realpath(__file__)).parent.parent / "Tools"

    def setUp(self):
        self.local_dir = tempfile.TemporaryDirectory()
        local = pathlib.Path(self.local_dir.name)
        self.assets = AssetManager()
        self.assets.register_store(FileStore(name="local", base_dir=local, mapping=asset_mapping))
        self.assets.register_store(
            FileStore(name="builtin", base_dir=self.tool_dir, mapping=builtin_asset_mapping)
        )
        self.assets.register_asset(ToolHolder, ToolHolderSerializer)

    def tearDown(self):
        self.local_dir.cleanup()

    def write_local(self, id, data):
        folder = pathlib.Path(self.local_dir.name) / "Tools" / "ToolHolder"
        folder.mkdir(parents=True, exist_ok=True)
        text = data if isinstance(data, str) else json.dumps(data)
        (folder / f"{id}.fcholder").write_text(text)

    def test_template_in_the_store_folder(self):
        """The template for drawing a holder goes into the holder folder of the store given."""
        ensure_toolholder_assets_initialized(self.assets, "local")
        folder = pathlib.Path(self.local_dir.name) / "Tools" / "ToolHolder"
        self.assertTrue((folder / TOOLHOLDER_TEMPLATE).is_file())

    def test00_builtin_holders_load(self):
        """Every holder that ships with FreeCAD reads, and the ER series is all there."""
        holders = available_holders(self.assets)
        for size in ("ER11", "ER16", "ER20", "ER25", "ER32", "ER40", "ER50"):
            self.assertIn(f"{size}_Standard", holders)
        for size in ("ER8", "ER11", "ER16", "ER20", "ER25"):
            self.assertIn(f"{size}_Mini", holders)
        self.assertTrue(any(id.startswith("Router_Nut_") for id in holders))
        for id, holder in holders.items():
            self.assertEqual(holder.get_id(), id)
            self.assertGreater(holder.height, 0, id)
            self.assertGreater(holder.diameter, 0, id)

    def test01_er32_size(self):
        """A standard ER32 nut is 50 mm across and reaches 22.5 mm above its face."""
        holder = self.assets.get("toolholder://ER32_Standard", store="builtin")
        self.assertEqual(holder.label, "ER32 nut, standard")
        self.assertEqual(holder.holder_type, "ER32")
        self.assertAlmostEqual(holder.diameter, 50.0)
        self.assertAlmostEqual(holder.height, 22.5)

    def test02_mini_is_slimmer(self):
        """A mini nut is slimmer than the standard nut for the same collet."""
        holders = available_holders(self.assets)
        for size in ("ER11", "ER16", "ER20", "ER25"):
            self.assertLess(
                holders[f"{size}_Mini"].diameter, holders[f"{size}_Standard"].diameter, size
            )

    def test03_local_holder_replaces_builtin(self):
        """A holder in the CAM asset folder replaces the built-in one of the same name, and
        one of its own is added to them."""
        self.write_local(
            "ER32_Standard",
            {"name": "My ER32 nut", "profile": [[0, 0], [26, 0], [26, 24], [0, 24]]},
        )
        self.write_local(
            "Shrink_Fit_6mm", {"name": "Shrink fit, 6 mm", "profile": [[0, 0], [10, 0], [0, 40]]}
        )
        holders = available_holders(self.assets)
        self.assertEqual(holders["ER32_Standard"].label, "My ER32 nut")
        self.assertAlmostEqual(holders["ER32_Standard"].diameter, 52.0)
        self.assertIn("Shrink_Fit_6mm", holders)
        self.assertIn("ER16_Standard", holders)

    def test04_unreadable_local_holder_skipped(self):
        """A holder that cannot be read is left out; the rest are still there."""
        self.write_local("Broken", "{ not json")
        self.write_local("Upside_Down", {"name": "Bad", "profile": [[0, 0], [-5, 10]]})
        holders = available_holders(self.assets)
        self.assertNotIn("Broken", holders)
        self.assertNotIn("Upside_Down", holders)
        self.assertIn("ER32_Standard", holders)

    def test05_round_trip(self):
        """A holder written out reads back the same."""
        holder = ToolHolder(
            "Test_Nut",
            "Test nut",
            [[0, 0], [9, 0], [10, 1], [10, 15], [0, 15]],
            holder_type="ER16",
            notes="A note",
            source="Measured",
        )
        data = ToolHolderSerializer.serialize(holder)
        again = ToolHolderSerializer.deserialize(data, "Test_Nut", None)
        self.assertEqual(again.to_dict(), holder.to_dict())
        self.assertEqual(json.loads(data)["profile"][2], [10.0, 1.0])

    def test06_outline_checked(self):
        """An outline needs two points, none below the face or inside the axis, and is in mm."""
        for profile in ([[0, 0]], [[0, 0], [5, -1]], [[0, 0], [-5, 1]]):
            with self.assertRaises(ValueError):
                ToolHolder("A", "A", profile)
        with self.assertRaises(ValueError):
            ToolHolder.from_dict("D", {"units": "in", "profile": [[0, 0], [1, 1]]})


class TestPathToolBitHolder(PathTestWithAssets):
    """A bit set in a holder, sticking out of it."""

    def bit(self, **parameters):
        attrs = self.assets.get("toolbit://5mm_Endmill").to_dict()
        attrs["parameter"].update(parameters)
        return ToolBit.from_dict(attrs)

    def test10_bit_in_no_holder(self):
        """A bit in no holder has none, sticks out by nothing, and saves as before."""
        bit = self.bit()
        self.assertIsNone(bit.get_holder_id())
        self.assertIsNone(bit.get_holder(self.assets))
        self.assertEqual(bit.get_stickout().Value, 0)
        self.assertNotIn("ToolHolder", bit.to_dict()["parameter"])
        self.assertNotIn("Stickout", bit.to_dict()["parameter"])

    def test11_bit_in_holder(self):
        """A bit's holder and stickout load, find the holder, and save again."""
        bit = self.bit(ToolHolder="ER20_Standard", Stickout="1.2500 in")
        self.assertEqual(bit.get_holder_id(), "ER20_Standard")
        self.assertAlmostEqual(bit.get_stickout().getValueAs("mm").Value, 31.75)
        self.assertAlmostEqual(bit.get_holder(self.assets).diameter, 35.0)
        again = ToolBit.from_dict(bit.to_dict())
        self.assertEqual(again.get_holder_id(), "ER20_Standard")
        self.assertAlmostEqual(again.get_stickout().getValueAs("mm").Value, 31.75)

    def test12_unknown_holder_kept(self):
        """A holder that is not here is kept by its id, not lost on saving."""
        bit = self.bit(ToolHolder="Shop_Shrink_Fit")
        self.assertEqual(bit.get_holder_id(), "Shop_Shrink_Fit")
        self.assertIsNone(bit.get_holder(self.assets))
        self.assertEqual(bit.to_dict()["parameter"]["ToolHolder"], "Shop_Shrink_Fit")

    def test14_unreadable_holder_is_none(self):
        """A holder whose file cannot be read is no holder, not an error for the caller."""

        class Broken:
            def get_or_none(self, uri, *args, **kwargs):
                raise ValueError("not a holder file")

        bit = self.bit(ToolHolder="ER20_Standard")
        self.assertIsNone(bit.get_holder(Broken()))

    def test13_change_holder(self):
        """A bit can be moved to another holder, and out of one."""
        bit = self.bit(ToolHolder="ER20_Standard")
        bit.set_holder_id("ER11_Mini")
        self.assertEqual(bit.get_holder_id(), "ER11_Mini")
        bit.set_holder_id(None)
        self.assertIsNone(bit.get_holder_id())
        self.assertNotIn("ToolHolder", bit.to_dict()["parameter"])

    def test15_stickout_in_bit_units(self):
        """Stickout saves in the bit's units, also for a bit whose file does not name them."""
        attrs = self.assets.get("toolbit://5mm_Endmill").to_dict()
        attrs["parameter"] = {
            "Diameter": "0.1250 in",
            "Length": "2.5000 in",
            "ShankDiameter": "0.1250 in",
            "CuttingEdgeHeight": "0.5000 in",
            "Stickout": "1.3650 in",
        }
        bit = ToolBit.from_dict(attrs)
        self.assertEqual(bit.obj.Units, "Imperial")
        self.assertEqual(bit.to_dict()["parameter"]["Stickout"], "1.3650 in")
        bit.obj.Units = "Metric"
        self.assertEqual(bit.to_dict()["parameter"]["Stickout"], "34.671 mm")


class TestPathToolHolderSketch(unittest.TestCase):
    """A holder's outline as a sketch: opened from a .fcholder, exported back to one."""

    ER32 = [(0, 0), (23.0, 0), (25.0, 2.0), (25.0, 22.5), (0, 22.5)]

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        self.doc = FreeCAD.newDocument("HolderSketch")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        self.tmp.cleanup()

    def test_opened_holder_exports_the_same(self):
        """A .fcholder opened as a sketch, then exported, gives the same outline and details."""
        holder = ToolHolder("ER32", "ER32", self.ER32, holder_type="ISO30-ER32", notes="nut")
        path = os.path.join(self.dir, "ER32.fcholder")
        out = os.path.join(self.dir, "ER32_Copy.fcholder")
        pathlib.Path(path).write_bytes(ToolHolderSerializer.serialize(holder))
        sketch = holderSketch.insert(path, self.doc.Name)
        self.assertTrue(sketch.Shape.isClosed())
        holderSketch.export([sketch], out)
        self.assertEqual(holderSketch.read(out).to_dict(), holder.to_dict())

    def test_drawn_either_way_round(self):
        """A closed outline drawn clockwise or counterclockwise exports from the bottom up."""
        for profile in (self.ER32, list(reversed(self.ER32))):
            sketch, _ = holderSketch.newHolderDocument(self.doc)
            holderSketch.drawOutline(sketch, profile)
            self.doc.recompute()
            self.assertEqual(holderSketch.outline(sketch), self.ER32)

    def test_open_outline_and_arc(self):
        """An outline left open along the axis works too, and an arc is written as short
        segments that stay on it."""
        V = FreeCAD.Vector
        sketch, _ = holderSketch.newHolderDocument(self.doc)
        sketch.addGeometry(Part.LineSegment(V(0, 0, 0), V(20, 0, 0)))
        circle = Part.Circle(V(20, 5, 0), V(0, 0, 1), 5)
        sketch.addGeometry(Part.ArcOfCircle(circle, -math.pi / 2, math.pi / 2))
        sketch.addGeometry(Part.LineSegment(V(20, 10, 0), V(0, 10, 0)))
        self.doc.recompute()
        points = holderSketch.outline(sketch)
        self.assertEqual(points[0], (0.0, 0.0))
        self.assertEqual(points[-1], (0.0, 10.0))
        self.assertGreater(len(points), 6)
        for r, z in points[2:-2]:
            self.assertAlmostEqual(((r - 20) ** 2 + (z - 5) ** 2) ** 0.5, 5, delta=0.02)

    def test_outline_off_the_axis_refused(self):
        """An outline that never reaches the axis is refused, and nothing is written."""
        sketch, _ = holderSketch.newHolderDocument(self.doc)
        holderSketch.drawOutline(sketch, [(5, 0), (20, 0), (20, 10), (5, 10)])
        self.doc.recompute()
        path = os.path.join(self.dir, "Bad.fcholder")
        with self.assertRaises(ValueError):
            holderSketch.export([sketch], path)
        self.assertFalse(os.path.exists(path))

    def test_two_holders_in_one_document_refused(self):
        """With two holders' details in the document, which belong to the sketch is unknown, so
        it is not exported."""
        path = os.path.join(self.dir, "ER32.fcholder")
        pathlib.Path(path).write_bytes(
            ToolHolderSerializer.serialize(ToolHolder("ER32", "ER32", self.ER32))
        )
        holderSketch.insert(path, self.doc.Name)
        sketch = holderSketch.insert(path, self.doc.Name)
        out = os.path.join(self.dir, "Out.fcholder")
        with self.assertRaises(ValueError):
            holderSketch.export([sketch], out)
        self.assertFalse(os.path.exists(out))

    def test_template_is_blank(self):
        """The template shipped with FreeCAD holds an empty outline sketch and the details."""
        path = TestPathToolHolder.tool_dir / "ToolHolder" / TOOLHOLDER_TEMPLATE
        doc = FreeCAD.openDocument(str(path))
        try:
            sketches = [o for o in doc.Objects if o.isDerivedFrom("Sketcher::SketchObject")]
            self.assertEqual(len(sketches), 1)
            self.assertEqual(sketches[0].GeometryCount, 0)
            self.assertTrue(hasattr(doc.getObject("Holder"), "HolderName"))
        finally:
            FreeCAD.closeDocument(doc.Name)


class TestPathToolBitHolderCopy(PathTestWithAssets):
    """A bit in a document keeps a copy of its holder's outline, so the document keeps its holder
    on a computer without the holder's file; an edited holder file reaches it only through the
    library update."""

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestToolBitHolderCopy")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        super().tearDown()

    def _bit(self):
        attrs = self.assets.get("toolbit://5mm_Endmill").to_dict()
        attrs["parameter"].update(ToolHolder="ER32_Standard", Stickout="40 mm")
        return ToolBit.from_dict(attrs)

    def _bit_in_document(self):
        obj = self._bit().attach_to_doc(self.doc)
        obj.Proxy.keep_holder_copy(self.assets)
        return obj

    def test_copy_used_without_the_file(self):
        """The copy is the holder when no holder file can be found."""
        obj = self._bit_in_document()
        self.assertTrue(obj.ToolHolderOutline)
        holder = obj.Proxy.get_holder(AssetManager())
        self.assertIsNotNone(holder)
        self.assertEqual(holder.get_id(), "ER32_Standard")
        self.assertAlmostEqual(holder.diameter, 50.0)

    def test_edited_file_reported_not_taken(self):
        """An edited holder file leaves the bit's copy as it was and shows in the library
        update."""
        obj = self._bit_in_document()
        library_obj = self._bit().obj
        self.assertEqual(diff_tool_setup(obj, library_obj, self.assets), [])
        self.assets.add(ToolHolder("ER32_Standard", "Edited", [(0, 0), (30, 0), (30, 60), (0, 60)]))
        self.assertAlmostEqual(obj.Proxy.get_holder(self.assets).diameter, 50.0)
        changes = diff_tool_setup(obj, library_obj, self.assets)
        self.assertEqual([c.name for c in changes], ["ToolHolderOutline"])
        library_obj.Stickout = FreeCAD.Units.Quantity("45 mm")
        changes = diff_tool_setup(obj, library_obj, self.assets)
        self.assertEqual([c.name for c in changes], ["ToolHolderOutline", "Stickout"])

    def test_other_holder_copied_and_no_holder_cleared(self):
        """Picking another holder copies it as its file has it; no holder keeps no copy."""
        obj = self._bit_in_document()
        obj.Proxy.set_holder_id("ER16_Standard")
        obj.Proxy.keep_holder_copy(self.assets)
        self.assertEqual(obj.Proxy.get_holder_copy().get_id(), "ER16_Standard")
        obj.Proxy.set_holder_id(None)
        self.assertEqual(obj.ToolHolderOutline, "")
        self.assertIsNone(obj.Proxy.get_holder(self.assets))

    def test_undo_brings_back_the_copy(self):
        """Undoing another holder brings back the copy the bit had, not the holder's file as it
        is now."""
        self.doc.UndoMode = 1
        obj = self._bit_in_document()
        copy = obj.ToolHolderOutline
        self.doc.openTransaction("Pick a holder")
        obj.ToolHolder = "ER16_Standard"
        self.doc.commitTransaction()
        edited = ToolHolder("ER32_Standard", "Edited", [(0, 0), (30, 0), (30, 60), (0, 60)])
        with unittest.mock.patch.object(obj.Proxy, "holder_from_file", return_value=edited):
            self.doc.undo()
        self.assertEqual(obj.ToolHolder, "ER32_Standard")
        self.assertEqual(obj.ToolHolderOutline, copy)

    def test_missing_copy_reported(self):
        """A bit saved before holders were copied gets no copy when its document opens; the
        library update lists the outline as missing."""
        obj = self._bit_in_document()
        obj.ToolHolderOutline = ""
        obj.Proxy.onDocumentRestored(obj)
        self.assertEqual(obj.ToolHolderOutline, "")
        changes = diff_tool_setup(obj, self._bit().obj, self.assets)
        self.assertEqual([c.name for c in changes], ["ToolHolderOutline"])
        self.assertEqual(changes[0].old_value, "Tool holder outline: not in the Job")

    def test_holders_listed_once_per_opening(self):
        """The bits of a document being opened read the holder files once between them."""
        for _ in range(3):
            self._bit_in_document()
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        path = os.path.join(folder.name, "Holders.FCStd")
        self.doc.saveAs(path)
        FreeCAD.closeDocument(self.doc.Name)
        with unittest.mock.patch(
            "Path.Tool.holder.available_holders", wraps=available_holders
        ) as listed:
            self.doc = FreeCAD.openDocument(path)
        self.assertEqual(listed.call_count, 1)
        for obj in self.doc.Objects:
            if hasattr(obj, "ToolHolder"):
                self.assertIn("ER16_Standard", obj.getEnumerationsOfProperty("ToolHolder"))

    def test_copy_not_saved_in_the_library_file(self):
        """The copy stays in the document: the bit's library file names its holder only."""
        obj = self._bit_in_document()
        self.assertEqual(obj.ToolHolder, "ER32_Standard")
        self.assertAlmostEqual(obj.Stickout.Value, 40.0)
        params = obj.Proxy.to_dict()["parameter"]
        self.assertEqual(params["ToolHolder"], "ER32_Standard")
        self.assertNotIn("ToolHolderOutline", params)
