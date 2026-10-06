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
import os
import uuid
import pathlib
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.library import Library
from Path.Tool.shape import ToolBitShapeBullnose
from Path.Tool.toolbit import ToolBitEndmill, ToolBitBullnose

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

    def testBooleanSimulationToolProfiles(self):
        """Modern toolbits produce valid sweeps with their actual cutting profiles."""
        import math
        from Path.Main.Gui.Simulator import PathSimulation
        import Part
        import Path

        simulation = PathSimulation.__new__(PathSimulation)
        simulation.debug = False
        for name in ("5mm_Endmill", "6mm_Ball_End", "6mm_Bullnose"):
            with self.subTest(toolbit=name):
                tool = self.assets.get("toolbit://" + name).obj
                self.assertFalse(hasattr(tool, "ToolType"))
                radius = float(tool.Diameter) / 2.0
                origin = FreeCAD.Vector(0, 0, 0)
                profile = simulation.CreateToolProfile(
                    tool, FreeCAD.Vector(1, 0, 0), origin, radius
                )
                self.assertTrue(profile.isValid())
                solid, end = simulation.GetPathSolid(
                    tool, Path.Command("G1", {"X": 10.0, "Y": 0.0, "Z": 0.0}), origin
                )
                self.assertIsNotNone(solid)
                self.assertTrue(solid.isValid())
                self.assertEqual(end, FreeCAD.Vector(10, 0, 0))
                if tool.ShapeType in ("Ballend", "Bullnose"):
                    corner = radius if tool.ShapeType == "Ballend" else float(tool.CornerRadius)
                    self.assertFalse(
                        solid.isInside(
                            FreeCAD.Vector(5, radius - corner * 0.1, corner * 0.1), 1e-7, True
                        )
                    )
                    self.assertTrue(solid.isInside(FreeCAD.Vector(5, 0, corner * 0.1), 1e-7, True))
                if tool.ShapeType == "Bullnose":
                    cutter = Part.makeSolid(profile.revolve(origin, FreeCAD.Vector(0, 0, 1), 360))
                    height = float(tool.CuttingEdgeHeight)
                    flat = radius - corner
                    expected = math.pi * radius**2 * height - math.pi * (
                        2 * flat * corner**2 * (1 - math.pi / 4) + corner**3 / 3
                    )
                    self.assertAlmostEqual(cutter.Volume, expected, places=5)

        # The legacy pointed profile omits the modern chamfer's flat tip.
        # Fail explicitly until its actual geometry is supported.
        chamfer = self.assets.get("toolbit://45degree_chamfer").obj
        with self.assertRaises(NotImplementedError):
            simulation.CreateToolProfile(
                chamfer,
                FreeCAD.Vector(1, 0, 0),
                FreeCAD.Vector(0, 0, 0),
                float(chamfer.Diameter) / 2.0,
            )

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
