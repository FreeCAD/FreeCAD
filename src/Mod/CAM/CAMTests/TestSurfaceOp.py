# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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


import FreeCAD
import Part
import Path
import unittest
import Path.Main.Job as PathJob
import Path.Op.Surface as PathSurface
from CAMTests.PathTestUtils import PathTestWithAssets

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check if OCL is available
_ocl_available = False
try:
    try:
        import ocl

        _ocl_available = True
    except ImportError:
        import opencamlib as ocl

        _ocl_available = True
except ImportError:
    pass


class TestSurfaceOp(PathTestWithAssets):
    """Integration tests for the unified Surface operation.

    These tests create a FreeCAD document with a Job, model geometry,
    and tool controller, then execute the Surface operation to verify
    end-to-end functionality.
    """

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestSurface")

    def tearDown(self):
        super().tearDown()
        FreeCAD.closeDocument(self.doc.Name)

    def _createJobWithBox(self, length=100, width=100, height=10):
        """Helper: create a Job with a simple box model and endmill tool."""
        box = self.doc.addObject("Part::Feature", "TestBox")
        box.Shape = Part.makeBox(length, width, height)

        job = PathJob.Create("Job", [box])

        # Load a 5mm endmill from assets and assign to tool controller
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        loaded_tool = toolbit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = loaded_tool

        self.doc.recompute()
        return job

    def _createJobWithSphere(self, radius=50):
        """Helper: create a Job with a simple sphere model and endmill tool."""
        sphere = self.doc.addObject("Part::Feature", "TestSphere")
        sphere.Shape = Part.makeSphere(radius)

        job = PathJob.Create("Job", [sphere])

        # Load a 5mm endmill from assets and assign to tool controller
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        loaded_tool = toolbit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = loaded_tool

        self.doc.recompute()
        return job

    # -- Property definition tests --

    def test00(self):
        """
        Verifies that the Surface operation can be created and has the Strategy property.

        INPUT:
        - Function: PathSurface.ObjectSurface()
        - Parameters: None (just instantiation check)
        - Input data: A new Surface operation object

        EXPECTED OUTPUT:
        - Operation object should be created successfully
        - Should have a Strategy property with default value "SurfacePattern"
        - Confirms the operation is properly registered and initialized
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        self.assertTrue(hasattr(op, "Strategy"))
        self.doc.recompute()

    def test01(self):
        """
        Verifies that the Strategy property has the correct enumeration values.

        INPUT:
        - Function: ObjectSurface.propertyEnumerations()
        - Parameters: dataType="data"
        - Input data: Class method call for enumeration data

        EXPECTED OUTPUT:
        - Should contain "Strategy" enumeration
        - Strategy should include: SurfacePattern, Waterline, ZLevelHybrid
        - These are the four supported 3D surfacing strategies
        """
        enums = PathSurface.ObjectSurface.propertyEnumerations()
        enum_dict = {name: values for name, values in enums}

        self.assertIn("Strategy", enum_dict)
        strategies = enum_dict["Strategy"]
        self.assertIn("SurfacePattern", strategies)
        self.assertIn("Waterline", strategies)
        self.assertIn("ZLevelHybrid", strategies)

    def test02(self):
        """
        Verifies that the CutPattern property has the correct enumeration values.

        INPUT:
        - Function: ObjectSurface.propertyEnumerations()
        - Parameters: dataType="data"
        - Input data: Class method call for enumeration data

        EXPECTED OUTPUT:
        - Should contain "CutPattern" enumeration
        - CutPattern should include: Line, ZigZag, Circular, CircularZigZag, Spiral, Offset
        - These are the scan patterns available for SurfacePattern strategy
        """
        enums = PathSurface.ObjectSurface.propertyEnumerations()
        enum_dict = {name: values for name, values in enums}

        self.assertIn("CutPattern", enum_dict)
        patterns = enum_dict["CutPattern"]
        self.assertIn("Line", patterns)
        self.assertIn("ZigZag", patterns)
        self.assertIn("Circular", patterns)
        self.assertIn("Spiral", patterns)

    def test03(self):
        """
        Verifies that the CutPatternZLevel property has the correct enumeration values.

        INPUT:
        - Function: ObjectSurface.propertyEnumerations()
        - Parameters: dataType="data"
        - Input data: Class method call for enumeration data

        EXPECTED OUTPUT:
        - Should contain "CutPatternZLevel" enumeration
        - CutPatternZLevel should include: None, Line, ZigZag, Offset, Grid
        - These are the scan patterns available for Z-Level Hybrid strategy
        """
        enums = PathSurface.ObjectSurface.propertyEnumerations()
        enum_dict = {name: values for name, values in enums}

        self.assertIn("CutPatternZLevel", enum_dict)
        patterns = enum_dict["CutPatternZLevel"]
        self.assertIn("None", patterns)
        self.assertIn("Line", patterns)
        self.assertIn("ZigZag", patterns)
        self.assertIn("Offset", patterns)
        self.assertIn("Grid", patterns)

    def test04(self):
        """
        Verifies that opFeatures returns the expected feature flags.

        INPUT:
        - Function: ObjectSurface.opFeatures()
        - Parameters: obj (FreeCAD operation object)
        - Input data: A new Surface operation

        EXPECTED OUTPUT:
        - Should include FeatureTool, FeatureDepths, FeatureHeights
        - Should include FeatureStepDown, FeatureCoolant, FeatureBaseFaces
        - These flags control which base class behaviors are enabled
        """
        import Path.Op.Base as PathOp

        proxy = PathSurface.ObjectSurface.__new__(PathSurface.ObjectSurface)
        features = proxy.opFeatures(None)

        self.assertTrue(features & PathOp.FeatureTool)
        self.assertTrue(features & PathOp.FeatureDepths)
        self.assertTrue(features & PathOp.FeatureHeights)
        self.assertTrue(features & PathOp.FeatureStepDown)
        self.assertTrue(features & PathOp.FeatureCoolant)
        self.assertTrue(features & PathOp.FeatureBaseFaces)

    # -- SurfacePattern execution tests --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test10(self):
        """
        Executes the SurfacePattern strategy on a simple box and verifies G-code output.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfacePattern, CutPattern=Line on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Path should contain both G0 (rapid) and G1 (cut) moves
        - Verifies the full SurfacePattern pipeline works end-to-end
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "SurfacePattern"
        op.CutPattern = "Line"
        op.StepOver = 50.0
        op.SampleInterval = 5.0

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        # Execute the operation
        op.Proxy.execute(op)

        # Verify output
        self.assertTrue(
            len(op.Path.Commands) > 0,
            "SurfacePattern should produce G-code commands",
        )

        # Check for both rapid and cut moves
        cmd_names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", cmd_names, "Should contain rapid moves")
        self.assertIn("G1", cmd_names, "Should contain cutting moves")

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test11(self):
        """
        Executes the SurfacePattern (Adaptive) strategy on a simple box and verifies G-code output.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfacePattern, CutPattern=Line on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Path should contain both G0 (rapid) and G1 (cut) moves
        - Verifies the full SurfacePattern pipeline works end-to-end
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "SurfacePattern"
        op.CutPattern = "Line"
        op.StepOver = 50.0
        op.AdaptiveSampling = True
        op.SampleInterval = 5.0
        op.MinSampleInterval = 3.0

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        # Execute the operation
        op.Proxy.execute(op)

        # Verify output
        self.assertTrue(
            len(op.Path.Commands) > 0,
            "SurfacePattern should produce G-code commands",
        )

        # Check for both rapid and cut moves
        cmd_names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", cmd_names, "Should contain rapid moves")
        self.assertIn("G1", cmd_names, "Should contain cutting moves")

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test12(self):
        """
        Executes the SurfacePattern strategy with ZigZag pattern on a box.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfacePattern, CutPattern=ZigZag on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - ZigZag pattern alternates line direction for efficient machining
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "SurfacePattern"
        op.CutPattern = "ZigZag"
        op.StepOver = 50.0
        op.SampleInterval = 5.0

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        op.Proxy.execute(op)

        self.assertTrue(
            len(op.Path.Commands) > 0,
            "SurfacePattern ZigZag should produce G-code commands",
        )

    # -- Waterline execution tests --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test20(self):
        """
        Executes the Waterline strategy on a box and verifies G-code output.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=Waterline on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Waterline creates constant-Z contour paths around the model
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "Waterline"
        op.SampleInterval = 2.0

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        op.Proxy.execute(op)

        self.assertTrue(
            len(op.Path.Commands) > 0,
            "Waterline should produce G-code commands",
        )

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test21(self):
        """
        Executes the Waterline (Adaptive) strategy on a box.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=Waterline on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Waterline (Adaptive) refines sampling where contour changes rapidly
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "Waterline"
        op.AdaptiveSampling = True
        op.SampleInterval = 2.0
        op.MinSampleInterval = 1.0

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        op.Proxy.execute(op)

        self.assertTrue(
            len(op.Path.Commands) > 0,
            "AdaptiveWaterline should produce G-code commands",
        )

    # -- Z-Level Hybrid execution tests --

    def test30(self):
        """
        Executes the Z-Level Hybrid strategy on a sphere (no OCL required).

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=ZLevelHybrid on a 50mm radius sphere
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Z-Level Hybrid uses FreeCAD shape slicing, no OCL dependency
        """
        job = self._createJobWithSphere()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "ZLevelHybrid"

        # Set the Base geometry to the job's model
        op.Base = job.Model.Group

        job.Operations.addObject(op)
        self.doc.recompute()

        op.Proxy.execute(op)

        self.assertTrue(
            len(op.Path.Commands) > 0,
            "Z-Level Hybrid should produce G-code commands",
        )

    # -- Property visibility tests --

    def test40(self):
        """
        Verifies that SurfacePattern strategy shows scan pattern properties.

        INPUT:
        - Function: ObjectSurface.setEditorProperties()
        - Parameters: Strategy=SurfacePattern
        - Input data: Surface operation with SurfacePattern strategy selected

        EXPECTED OUTPUT:
        - CutPattern should be visible (editor mode 0)
        - ClearPlanarOnly, IgnoreOuter, SamplingAccuracy, CutPatternZLevel and StockToLeave
          should be hidden (editor mode 2) since it's Z-Level-specific
        - Ensures UI shows relevant properties for the selected strategy
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "SurfacePattern"
        proxy.setEditorProperties(op)

        self.assertEqual(op.getEditorMode("CutPattern"), [])  # visible
        self.assertEqual(op.getEditorMode("ClearPlanarOnly"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("IgnoreOuter"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("SamplingAccuracy"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("CutPatternZLevel"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("StockToLeave"), ["Hidden"])  # hidden

    def test41(self):
        """
        Verifies that Waterline strategy hides scan pattern properties.

        INPUT:
        - Function: ObjectSurface.setEditorProperties()
        - Parameters: Strategy=Waterline
        - Input data: Surface operation with Waterline strategy selected

        EXPECTED OUTPUT:
        - CutPattern should be hidden (editor mode 2) since waterline doesn't use scan patterns
        - ClearPlanarOnly, IgnoreOuter, SamplingAccuracy, CutPatternZLevel and StockToLeave
          should be hidden (editor mode 2) since it's Z-Level-specific
        - Ensures UI adapts to the selected strategy
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        op.Strategy = "Waterline"
        proxy.setEditorProperties(op)

        self.assertEqual(op.getEditorMode("CutPattern"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("ClearPlanarOnly"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("IgnoreOuter"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("SamplingAccuracy"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("CutPatternZLevel"), ["Hidden"])  # hidden
        self.assertEqual(op.getEditorMode("StockToLeave"), ["Hidden"])  # hidden

    # -- Accuracy Preset Tests --

    def test42(self):
        """
        Verifies that accuracy presets are properly defined and accessible.

        INPUT:
        - Function: ObjectSurface.ACCURACY_PRESETS
        - Parameters: None (static class attribute access)
        - Input data: The ACCURACY_PRESETS dictionary

        EXPECTED OUTPUT:
        - All levels 1-7 should exist in the presets
        - Each preset should have required keys: name, angular_deflection, linear_deflection,
          mesh_simplification, sample_interval, description
        - Confirms the preset system is properly structured
        """
        presets = PathSurface.ObjectSurface.ACCURACY_PRESETS

        # Check all levels 1-7 exist
        for level in range(1, 8):
            self.assertIn(level, presets)

        # Check required preset keys
        for level, preset in presets.items():
            self.assertIn("name", preset)
            self.assertIn("angular_deflection", preset)
            self.assertIn("linear_deflection", preset)
            self.assertIn("mesh_simplification", preset)
            self.assertIn("sample_interval", preset)
            self.assertIn("description", preset)

    def test43(self):
        """
        Verifies that apply_accuracy_preset function works correctly.

        INPUT:
        - Function: ObjectSurface.apply_accuracy_preset()
        - Parameters: level = 1 (Fastest)
        - Input data: Surface operation with default parameters

        EXPECTED OUTPUT:
        - All quality parameters should match preset values
        - AngularDeflection, LinearDeflection, MeshSimplification, SampleInterval
        - Confirms preset application works correctly
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        # Test Fastest (level 1)
        proxy.apply_accuracy_preset(op, 1)

        preset = PathSurface.ObjectSurface.ACCURACY_PRESETS[1]
        self.assertEqual(op.AngularDeflection.Value, preset["angular_deflection"])
        self.assertEqual(op.LinearDeflection.Value, preset["linear_deflection"])
        self.assertEqual(op.MeshSimplification, preset["mesh_simplification"])
        self.assertEqual(op.SampleInterval.Value, preset["sample_interval"])

    def test44(self):
        """
        Verifies that get_accuracy_level function correctly identifies preset matches.

        INPUT:
        - Function: ObjectSurface.get_accuracy_level()
        - Parameters: None (checks current operation state)
        - Input data: Surface operation with preset applied

        EXPECTED OUTPUT:
        - Should return the correct preset level when values match exactly
        - Should return None when values don't match any preset
        - Confirms preset detection works correctly
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Surface")
        proxy = PathSurface.ObjectSurface(op, "Surface")
        proxy.initOperation(op)

        # Apply a preset and verify detection
        proxy.apply_accuracy_preset(op, 3)  # Fast
        detected_level = proxy.get_accuracy_level(op)
        self.assertEqual(detected_level, 3)

        # Modify one parameter slightly and verify no match
        op.AngularDeflection = FreeCAD.Units.Quantity("0.21 mm")
        detected_level = proxy.get_accuracy_level(op)
        self.assertIsNone(detected_level)

    def test45(self):
        """
        Verifies that preset progression is logical from fast to high accuracy.

        INPUT:
        - Function: ACCURACY_PRESETS dictionary access
        - Parameters: None (static analysis)
        - Input data: The ACCURACY_PRESETS dictionary

        EXPECTED OUTPUT:
        - Higher accuracy levels should have smaller deflections
        - Mesh simplification should decrease with level (less reduction)
        - Sample interval should decrease with level (denser sampling)
        - Ensures logical parameter progression from 1=Fastest to 7=Ultra
        """
        presets = PathSurface.ObjectSurface.ACCURACY_PRESETS

        # Higher accuracy levels should have smaller deflections
        for level in range(1, 7):
            current = presets[level]
            next_level = presets[level + 1]

            # Angular deflection should decrease (higher accuracy = smaller value)
            self.assertGreaterEqual(current["angular_deflection"], next_level["angular_deflection"])

            # Linear deflection should decrease (higher accuracy = smaller value)
            self.assertGreaterEqual(current["linear_deflection"], next_level["linear_deflection"])

            # Mesh simplification should decrease (less reduction for higher accuracy)
            self.assertGreaterEqual(
                current["mesh_simplification"], next_level["mesh_simplification"]
            )

            # Sample interval should decrease (denser sampling for higher accuracy)
            self.assertGreaterEqual(current["sample_interval"], next_level["sample_interval"])

    def test46(self):
        """
        Verifies that extreme presets have appropriate values.

        INPUT:
        - Function: ACCURACY_PRESETS dictionary access
        - Parameters: None (static analysis of levels 1 and 7)
        - Input data: Fastest (level 1) and Ultra (level 7) presets

        EXPECTED OUTPUT:
        - Level 1 should have fastest settings (maximum simplification, coarsest deflections)
        - Level 7 should have highest quality settings (no simplification, finest deflections)
        - Confirms extreme presets are properly configured for inverted scale
        """
        presets = PathSurface.ObjectSurface.ACCURACY_PRESETS

        # Level 1 should be fastest
        fastest = presets[1]
        self.assertEqual(fastest["mesh_simplification"], 7)  # Max reduction
        self.assertEqual(fastest["angular_deflection"], 0.5)  # Coarsest
        self.assertEqual(fastest["linear_deflection"], 0.1)  # Least precise
        self.assertEqual(fastest["sample_interval"], 1.5)  # Coarsest
        self.assertEqual(fastest["min_sample_interval"], 0.3)  # Coarsest

        # Level 7 should be highest quality
        ultra = presets[7]
        self.assertEqual(ultra["mesh_simplification"], 1)  # No reduction
        self.assertEqual(ultra["angular_deflection"], 0.05)  # Finest
        self.assertEqual(ultra["linear_deflection"], 0.005)  # Most precise
        self.assertEqual(ultra["sample_interval"], 0.05)  # Densest
        self.assertEqual(ultra["min_sample_interval"], 0.05)  # Densest
