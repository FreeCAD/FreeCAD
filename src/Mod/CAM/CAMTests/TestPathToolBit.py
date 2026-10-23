# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from typing import cast
import json
import math
import os
import tempfile
import uuid
import pathlib
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.library import Library
from Path.Tool.shape import (
    ToolBitShapeBullnose,
    ToolBitShapeDovetail,
    ToolBitShapeKeyway,
    ToolBitShapeLollipop,
    ToolBitShapeReamer,
)
from Path.Tool.toolbit import (
    ToolBitEndmill,
    ToolBitBullnose,
    ToolBitDovetail,
    ToolBitKeyway,
    ToolBitLollipop,
    ToolBitReamer,
)
from Path.Tool.toolbit.migration import ParameterAccessor, migrate_parameters

TOOL_DIR = pathlib.Path(os.path.realpath(__file__)).parent.parent / "Tools"
SHAPE_DIR = TOOL_DIR / "Shape"
BIT_DIR = TOOL_DIR / "Bit"


class TestPathToolBit(PathTestWithAssets):
    def testGetToolBit(self):
        """Find a tool bit from file name"""
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        self.assertIsInstance(toolbit, ToolBitEndmill)
        self.assertEqual(toolbit.id, "5mm_Endmill")

    def testGetLibrary(self):
        """Find a tool library from file name"""
        library = self.assets.get("toolbitlibrary://Default")
        self.assertIsInstance(library, Library)
        self.assertEqual(library.id, "Default")

    def testBullnose(self):
        """Test ToolBitBullnose basic parameters"""
        shape = self.assets.get("toolbitshape://bullnose")
        shape = cast(ToolBitShapeBullnose, shape)

        bullnose_bit = ToolBitBullnose(shape, id="mybullnose")
        self.assertEqual(bullnose_bit.get_id(), "mybullnose")

        bullnose_bit = ToolBitBullnose(shape)
        uuid.UUID(bullnose_bit.get_id())  # will raise if not valid UUID

        # Parameters should be loaded from the shape file and set on the tool bit's object
        self.assertEqual(bullnose_bit.obj.Diameter, FreeCAD.Units.Quantity("5.0 mm"))
        self.assertEqual(bullnose_bit.obj.CornerRadius, FreeCAD.Units.Quantity("1.5 mm"))

    def testToolBitPickle(self):
        """Test if ToolBit is picklable"""
        import pickle

        shape = self.assets.get("toolbitshape://bullnose")
        shape = cast(ToolBitShapeBullnose, shape)
        bullnose_bit = ToolBitBullnose(shape, id="mybullnose")
        try:
            pickled_bit = pickle.dumps(bullnose_bit)
            unpickled_bit = pickle.loads(pickled_bit)
            self.assertIsInstance(unpickled_bit, ToolBitBullnose)
            self.assertEqual(unpickled_bit.get_id(), "mybullnose")
            # Add more assertions here to check if other attributes are preserved
        except Exception as e:
            self.fail(f"ToolBit is not picklable: {e}")


class TestPathToolBitMigration(PathTestWithAssets):
    """Legacy parameter migration, for .fctb dicts and for document objects."""

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestToolBitMigration")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        super().tearDown()

    def _old_dovetail_dict(self, unit):
        return {
            "version": 2,
            "name": "Old dovetail",
            "shape": "dovetail.fcstd",
            "shape-type": "Dovetail",
            "parameter": {
                "CuttingEdgeAngle": "60.0000 \u00b0",
                "CuttingEdgeHeight": f"9.0000 {unit}",
                "Diameter": f"20.0000 {unit}",
                "Length": f"55.0000 {unit}",
                "NeckDiameter": f"8.0000 {unit}",
                "NeckHeight": f"5.0000 {unit}",
                "ShankDiameter": f"8.0000 {unit}",
                "TipDiameter": f"5.0000 {unit}",
            },
            "attribute": {},
        }

    def testDovetailDictGetsNeckCuttingHeight(self):
        """An old dovetail .fctb gains a zero NeckCuttingHeight in its own unit"""
        for unit in ("mm", "in"):
            with self.subTest(unit=unit):
                attrs = self._old_dovetail_dict(unit)
                self.assertTrue(migrate_parameters(ParameterAccessor(attrs)))
                self.assertEqual(attrs["parameter"]["NeckCuttingHeight"], f"0.0000 {unit}")
                # Nothing else changes.
                old = self._old_dovetail_dict(unit)["parameter"]
                for name, value in old.items():
                    self.assertEqual(attrs["parameter"][name], value)

    def testDovetailDictMigrationIsIdempotent(self):
        """A dovetail that has NeckCuttingHeight keeps its value"""
        attrs = self._old_dovetail_dict("mm")
        attrs["parameter"]["NeckCuttingHeight"] = "3.0000 mm"
        attrs["parameter"]["Units"] = "Metric"
        self.assertFalse(migrate_parameters(ParameterAccessor(attrs)))
        self.assertEqual(attrs["parameter"]["NeckCuttingHeight"], "3.0000 mm")

    def testDovetailObjectGetsNeckCuttingHeight(self):
        """A dovetail document object saved without NeckCuttingHeight gains it"""
        shape = cast(ToolBitShapeDovetail, self.assets.get("toolbitshape://dovetail"))
        obj = ToolBitDovetail(shape, id="olddovetail").attach_to_doc(self.doc)
        obj.removeProperty("NeckCuttingHeight")
        self.assertNotIn("NeckCuttingHeight", obj.PropertiesList)
        volume = obj.BitBody.Shape.Volume

        self.assertTrue(migrate_parameters(ParameterAccessor(obj)))
        self.assertIn("NeckCuttingHeight", obj.PropertiesList)
        self.assertEqual(obj.getGroupOfProperty("NeckCuttingHeight"), "Shape")
        self.assertEqual(obj.NeckCuttingHeight, FreeCAD.Units.Quantity("0 mm"))
        self.assertFalse(migrate_parameters(ParameterAccessor(obj)))

        # NeckCuttingHeight is not geometry: the tool is drawn as before.
        self.doc.recompute()
        self.assertAlmostEqual(obj.BitBody.Shape.Volume, volume, places=6)

    def _old_reamer_dict(self, height="20.0000 mm"):
        return {
            "version": 2,
            "name": "Old reamer",
            "shape": "reamer.fcstd",
            "shape-type": "Reamer",
            "parameter": {
                "CuttingEdgeHeight": height,
                "Diameter": "5.0000 mm",
                "Length": "50.0000 mm",
                "ShankDiameter": "3.0000 mm",
            },
            "attribute": {},
        }

    def testReamerDictGetsFluteLength(self):
        """An old reamer .fctb keeps its fluted length, unit and all, as FluteLength"""
        for height in ("20.0000 mm", "0.7500 in"):
            with self.subTest(height=height):
                attrs = self._old_reamer_dict(height)
                self.assertTrue(migrate_parameters(ParameterAccessor(attrs)))
                self.assertEqual(attrs["parameter"]["FluteLength"], height)
                self.assertEqual(attrs["parameter"]["CuttingEdgeHeight"], height)
        attrs = self._old_reamer_dict()
        attrs["parameter"]["FluteLength"] = "12.0000 mm"
        attrs["parameter"]["Units"] = "Metric"
        self.assertFalse(migrate_parameters(ParameterAccessor(attrs)))
        self.assertEqual(attrs["parameter"]["FluteLength"], "12.0000 mm")

    def testReamerObjectGetsFluteLength(self):
        """A reamer document object saved without FluteLength gains it from CuttingEdgeHeight"""
        shape = cast(ToolBitShapeReamer, self.assets.get("toolbitshape://reamer"))
        obj = ToolBitReamer(shape, id="oldreamer").attach_to_doc(self.doc)
        obj.removeProperty("FluteLength")
        # As restored from the file: no derived recompute has run yet.
        obj.Proxy._suppress_visual_update = True
        obj.CuttingEdgeHeight = FreeCAD.Units.Quantity("20 mm")
        obj.Proxy._suppress_visual_update = False

        self.assertTrue(migrate_parameters(ParameterAccessor(obj)))
        self.assertEqual(obj.getGroupOfProperty("FluteLength"), "Shape")
        self.assertEqual(obj.FluteLength, FreeCAD.Units.Quantity("20 mm"))
        self.assertFalse(migrate_parameters(ParameterAccessor(obj)))

    def testOldReamerDocumentRestores(self):
        """A saved document with an old reamer reopens with FluteLength carried over"""
        shape = cast(ToolBitShapeReamer, self.assets.get("toolbitshape://reamer"))
        obj = ToolBitReamer(shape, id="oldreamer").attach_to_doc(self.doc)
        obj.removeProperty("FluteLength")
        obj.Proxy._suppress_visual_update = True
        obj.CuttingEdgeHeight = FreeCAD.Units.Quantity("20 mm")
        obj.Proxy._suppress_visual_update = False
        name = obj.Name
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "oldreamer.FCStd")
            self.doc.saveAs(path)
            FreeCAD.closeDocument(self.doc.Name)
            self.doc = FreeCAD.openDocument(path)
            obj = self.doc.getObject(name)
            self.assertEqual(obj.FluteLength, FreeCAD.Units.Quantity("20 mm"))
            self.assertEqual(obj.CuttingEdgeHeight, FreeCAD.Units.Quantity("0 mm"))

    def testOldReamerFileLoadsAsBefore(self):
        """An old reamer .fctb is drawn as before, and now has no cutting edge height"""
        old = self._old_reamer_dict()
        new = self._old_reamer_dict()
        new["parameter"]["FluteLength"] = new["parameter"].pop("CuttingEdgeHeight")
        bodies = []
        for asset_id, attrs in (("old_reamer", old), ("new_reamer", new)):
            self.assets.add_raw(
                "toolbit", asset_id, json.dumps(attrs).encode(), store=self.asset_store.name
            )
            toolbit = self.assets.get(f"toolbit://{asset_id}")
            self.assertIsInstance(toolbit, ToolBitReamer)
            obj = toolbit.attach_to_doc(self.doc)
            self.doc.recompute()
            self.assertEqual(obj.FluteLength, FreeCAD.Units.Quantity("20 mm"))
            self.assertEqual(obj.CuttingEdgeHeight, FreeCAD.Units.Quantity("0 mm"))
            self.assertIn("ReadOnly", obj.getEditorMode("CuttingEdgeHeight"))
            bodies.append(obj.BitBody.Shape)
        self.assertAlmostEqual(bodies[0].Volume, bodies[1].Volume, places=6)
        self.assertTrue(bodies[0].BoundBox.isInside(bodies[1].BoundBox.Center))
        # 20 mm of 5 mm flutes, then a 3 mm shank to 50 mm
        expected = math.pi * (2.5**2 * 20 + 1.5**2 * 30)
        self.assertAlmostEqual(bodies[0].Volume, expected, delta=0.5)

    def testDovetailNeckCuttingHeightIsNotGeometry(self):
        """Changing NeckCuttingHeight leaves the dovetail's solid alone"""
        shape = cast(ToolBitShapeDovetail, self.assets.get("toolbitshape://dovetail"))
        obj = ToolBitDovetail(shape, id="dovetail").attach_to_doc(self.doc)
        self.doc.recompute()
        volume = obj.BitBody.Shape.Volume
        obj.NeckCuttingHeight = FreeCAD.Units.Quantity("3 mm")
        self.doc.recompute()
        obj.Proxy._process_queued_visual_update()
        self.assertAlmostEqual(obj.BitBody.Shape.Volume, volume, places=6)


class TestPathToolBitDerivedShapes(PathTestWithAssets):
    """Shapes whose CuttingEdgeHeight follows from their other parameters."""

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestToolBitDerived")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        super().tearDown()

    def _recompute(self, obj):
        self.doc.recompute()
        obj.Proxy._process_queued_visual_update()

    def testKeyway(self):
        """Keyway: CuttingEdgeHeight = LowerCuttingHeight + NeckCuttingHeight, read-only"""
        shape = cast(ToolBitShapeKeyway, self.assets.get("toolbitshape://keyway"))
        obj = ToolBitKeyway(shape, id="keyway").attach_to_doc(self.doc)
        self._recompute(obj)
        self.assertIn("CuttingEdgeHeight", obj.PropertiesList)
        self.assertIn("ReadOnly", obj.getEditorMode("CuttingEdgeHeight"))
        self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, 3.0)

        solid = obj.BitBody.Shape
        self.assertTrue(solid.isValid())
        self.assertAlmostEqual(solid.BoundBox.ZMin, 0.0, places=6)
        self.assertAlmostEqual(solid.BoundBox.ZMax, 50.0, places=6)
        self.assertAlmostEqual(solid.BoundBox.XMax, 10.0, places=6)
        volume = solid.Volume

        obj.NeckCuttingHeight = FreeCAD.Units.Quantity("4 mm")
        self._recompute(obj)
        self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, 7.0)
        # The fluted part of the neck is not drawn: same solid.
        self.assertAlmostEqual(obj.BitBody.Shape.Volume, volume, places=6)

        obj.LowerCuttingHeight = FreeCAD.Units.Quantity("5 mm")
        self._recompute(obj)
        self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, 9.0)
        self.assertGreater(obj.BitBody.Shape.Volume, volume)

    def testLollipop(self):
        """Lollipop: CuttingEdgeHeight is where the neck meets the ball, read-only"""
        shape = cast(ToolBitShapeLollipop, self.assets.get("toolbitshape://lollipop"))
        obj = ToolBitLollipop(shape, id="lollipop").attach_to_doc(self.doc)
        self._recompute(obj)
        self.assertIn("ReadOnly", obj.getEditorMode("CuttingEdgeHeight"))
        self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, 3 + math.sqrt(9 - 2.25), places=6)

        solid = obj.BitBody.Shape
        self.assertTrue(solid.isValid())
        self.assertAlmostEqual(solid.BoundBox.ZMin, 0.0, places=6)
        self.assertAlmostEqual(solid.BoundBox.ZMax, 50.0, places=6)
        self.assertAlmostEqual(solid.BoundBox.XMax, 3.0, places=6)

        obj.NeckDiameter = FreeCAD.Units.Quantity("2 mm")
        self._recompute(obj)
        self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, 3 + math.sqrt(9 - 1), places=6)

    def testLollipopBallOnAShaft(self):
        """A ball on a plain shaft (neck as wide as the shank) still builds"""
        shape = cast(ToolBitShapeLollipop, self.assets.get("toolbitshape://lollipop"))
        obj = ToolBitLollipop(shape, id="ballonshaft").attach_to_doc(self.doc)
        obj.Diameter = FreeCAD.Units.Quantity("6 mm")
        obj.NeckDiameter = FreeCAD.Units.Quantity("4 mm")
        obj.ShankDiameter = FreeCAD.Units.Quantity("4 mm")
        self._recompute(obj)
        solid = obj.BitBody.Shape
        self.assertTrue(solid.isValid())
        self.assertGreater(solid.Volume, 0)
        self.assertAlmostEqual(solid.BoundBox.ZMax, 50.0, places=6)
        self.assertAlmostEqual(solid.BoundBox.XMax, 3.0, places=6)
        # ball, plus a 2 mm radius shaft from where it leaves the ball to the top
        junction = 3 + math.sqrt(9 - 4)
        cap = math.pi * junction**2 * (9 - junction) / 3
        shaft = math.pi * 4 * (50 - junction)
        self.assertAlmostEqual(solid.Volume, cap + shaft, delta=0.05)

    def testSampleKeyway(self):
        """The shipped 3/8in keyway loads, builds, and derives its cutting edge height"""
        toolbit = self.assets.get("toolbit://375_Keyway")
        self.assertIsInstance(toolbit, ToolBitKeyway)
        obj = toolbit.attach_to_doc(self.doc)
        self._recompute(obj)
        self.assertEqual(obj.Units, "Imperial")
        self.assertAlmostEqual(obj.CuttingEdgeHeight.getValueAs("in").Value, 0.125, places=6)
        solid = obj.BitBody.Shape
        self.assertTrue(solid.isValid())
        self.assertAlmostEqual(solid.BoundBox.XMax, 0.375 * 25.4 / 2, places=4)
        self.assertAlmostEqual(solid.BoundBox.ZMax, 2.0 * 25.4, places=4)

    def testSampleLollipop(self):
        """The shipped 1/4in lollipop loads, builds, and derives its cutting edge height"""
        toolbit = self.assets.get("toolbit://250_Lollipop")
        self.assertIsInstance(toolbit, ToolBitLollipop)
        obj = toolbit.attach_to_doc(self.doc)
        self._recompute(obj)
        self.assertEqual(obj.Units, "Imperial")
        expected = 0.125 + math.sqrt(0.125**2 - 0.0625**2)
        self.assertAlmostEqual(obj.CuttingEdgeHeight.getValueAs("in").Value, expected, places=6)
        solid = obj.BitBody.Shape
        self.assertTrue(solid.isValid())
        self.assertAlmostEqual(solid.BoundBox.XMax, 0.125 * 25.4, places=4)
        self.assertAlmostEqual(solid.BoundBox.ZMax, 2.0 * 25.4, places=4)

    def testDefaultLibraryHasTheSamples(self):
        """The default library lists the keyway and lollipop samples"""
        library = self.assets.get("toolbitlibrary://Default")
        labels = [bit.label for bit in library]
        self.assertIn("3/8in Keyway", labels)
        self.assertIn("1/4in Lollipop", labels)

    def testSampleDrillsAndTaps(self):
        """Shipped drills and taps load with their derived CuttingEdgeHeight"""
        cases = (
            ("5mm_Drill", None),
            ("M8x1.25_Tap", 0.0),
            ("375-16_Tap", 0.0),
        )
        for asset_id, expected in cases:
            with self.subTest(asset_id=asset_id):
                toolbit = self.assets.get(f"toolbit://{asset_id}")
                obj = toolbit.attach_to_doc(self.doc)
                self._recompute(obj)
                self.assertIn("CuttingEdgeHeight", obj.PropertiesList)
                self.assertIn("ReadOnly", obj.getEditorMode("CuttingEdgeHeight"))
                if expected is None:
                    radius = obj.Diameter.Value / 2
                    expected = radius / math.tan(math.radians(obj.TipAngle.Value / 2))
                self.assertAlmostEqual(obj.CuttingEdgeHeight.Value, expected, places=6)
                self.assertTrue(obj.BitBody.Shape.isValid())
                self.assertAlmostEqual(obj.BitBody.Shape.BoundBox.ZMax, obj.Length.Value, places=4)

    def testOldTapDocumentGetsCuttingEdgeHeight(self):
        """A tap saved before it had a CuttingEdgeHeight gains the derived one on reopening"""
        toolbit = self.assets.get("toolbit://M8x1.25_Tap")
        obj = toolbit.attach_to_doc(self.doc)
        obj.removeProperty("CuttingEdgeHeight")
        name = obj.Name
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "oldtap.FCStd")
            self.doc.saveAs(path)
            FreeCAD.closeDocument(self.doc.Name)
            self.doc = FreeCAD.openDocument(path)
            obj = self.doc.getObject(name)
            self.assertIn("CuttingEdgeHeight", obj.PropertiesList)
            self.assertEqual(obj.CuttingEdgeHeight, FreeCAD.Units.Quantity("0 mm"))
            self.assertIn("ReadOnly", obj.getEditorMode("CuttingEdgeHeight"))
