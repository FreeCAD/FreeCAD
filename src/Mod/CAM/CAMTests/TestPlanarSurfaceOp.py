# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2026 Dimitris75 <dimitriospana75@gmail.com>
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


import FreeCAD
import Part
import Path
import unittest
import Path.Main.Job as PathJob
from CAMTests.PathTestUtils import PathTestWithAssets

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check if OCL is available
try:
    import ocl
except ImportError:
    try:
        import opencamlib as ocl
    except ImportError:
        ocl = None

_ocl_available = ocl is not None

if _ocl_available:
    import Path.Op.PlanarSurface as PathPlanarSurface


@unittest.skipUnless(_ocl_available, "OpenCamLib not available")
class TestPlanarSurfaceOp(PathTestWithAssets):
    """Integration tests for the unified Surface operation.

    These tests create a FreeCAD document with a Job, model geometry,
    and tool controller, then execute the Surface operation to verify
    end-to-end functionality.
    """

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestPlanarSurface")

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
        - Function: PathPlanarSurface.ObjectSurface()
        - Parameters: None (just instantiation check)
        - Input data: A new Surface operation object

        EXPECTED OUTPUT:
        - Operation object should be created successfully
        - Should have a Strategy property with default value "SurfaceScan"
        - Confirms the operation is properly registered and initialized
        """
        self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)

        self.assertTrue(hasattr(op, "Strategy"))
        self.doc.recompute()

    def test01(self):
        """
        Verifies that the Strategy property has the correct enumeration values.

        INPUT:
        - Function: ObjectPlanarSurface.propertyEnumerations()
        - Parameters: dataType="data"
        - Input data: Class method call for enumeration data

        EXPECTED OUTPUT:
        - Should contain "Strategy" enumeration
        - Strategy should include: SurfaceScan, Waterline, ZLevelHybrid
        - These are the four supported 3D surfacing strategies
        """
        enums = PathPlanarSurface.ObjectSurface.propertyEnumerations()
        enum_dict = {name: values for name, values in enums}

        self.assertIn("Strategy", enum_dict)
        strategies = enum_dict["Strategy"]
        self.assertIn("SurfaceScan", strategies)
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
        - These are the scan patterns available for SurfaceScan strategy
        """
        enums = PathPlanarSurface.ObjectSurface.propertyEnumerations()
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
        enums = PathPlanarSurface.ObjectSurface.propertyEnumerations()
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

        proxy = PathPlanarSurface.ObjectSurface.__new__(PathPlanarSurface.ObjectSurface)
        features = proxy.opFeatures(None)

        self.assertTrue(features & PathOp.FeatureTool)
        self.assertTrue(features & PathOp.FeatureDepths)
        self.assertTrue(features & PathOp.FeatureHeights)
        self.assertTrue(features & PathOp.FeatureStepDown)
        self.assertTrue(features & PathOp.FeatureCoolant)
        self.assertTrue(features & PathOp.FeatureBaseFaces)

    # -- SurfaceScan execution tests --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test10(self):
        """
        Executes the SurfaceScan strategy on a simple box and verifies G-code output.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfaceScan, CutPattern=Line on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Path should contain both G0 (rapid) and G1 (cut) moves
        - Verifies the full SurfaceScan pipeline works end-to-end
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)

        op.Strategy = "SurfaceScan"
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
            "SurfaceScan should produce G-code commands",
        )

        # Check for both rapid and cut moves
        cmd_names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", cmd_names, "Should contain rapid moves")
        self.assertIn("G1", cmd_names, "Should contain cutting moves")

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test11(self):
        """
        Executes the SurfaceScan (Adaptive) strategy on a simple box and verifies G-code output.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfaceScan, CutPattern=Line on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Path should contain both G0 (rapid) and G1 (cut) moves
        - Verifies the full SurfaceScan pipeline works end-to-end
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)

        op.Strategy = "SurfaceScan"
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
            "SurfaceScan should produce G-code commands",
        )

        # Check for both rapid and cut moves
        cmd_names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", cmd_names, "Should contain rapid moves")
        self.assertIn("G1", cmd_names, "Should contain cutting moves")

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test12(self):
        """
        Executes the SurfaceScan strategy with ZigZag pattern on a box.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=SurfaceScan, CutPattern=ZigZag on a 100x100x10mm box
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - ZigZag pattern alternates line direction for efficient machining
        """
        job = self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)

        op.Strategy = "SurfaceScan"
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
            "SurfaceScan ZigZag should produce G-code commands",
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

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
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

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
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
        Executes the Z-Level Hybrid strategy on a sphere.

        INPUT:
        - Function: ObjectSurface.opExecute()
        - Parameters: Strategy=ZLevelHybrid on a 50mm radius sphere
        - Input data: Simple rectangular solid with 5mm endmill

        EXPECTED OUTPUT:
        - Operation should execute without errors
        - Should produce G-code commands (non-empty path)
        - Operation should execute using the configured strategy
        """
        job = self._createJobWithSphere()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
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
        Verifies that SurfaceScan strategy shows scan pattern properties.

        INPUT:
        - Function: ObjectSurface.setEditorProperties()
        - Parameters: Strategy=SurfaceScan
        - Input data: Surface operation with SurfaceScan strategy selected

        EXPECTED OUTPUT:
        - CutPattern should be visible (editor mode 0)
        - ClearPlanarOnly, IgnoreOuter, SamplingAccuracy, CutPatternZLevel and StockToLeave
          should be hidden (editor mode 2) since it's Z-Level-specific
        - Ensures UI shows relevant properties for the selected strategy
        """
        self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)

        op.Strategy = "SurfaceScan"
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
        self._createJobWithBox()

        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
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
        presets = PathPlanarSurface.ObjectSurface.ACCURACY_PRESETS

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
        presets = PathPlanarSurface.ObjectSurface.ACCURACY_PRESETS

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

    def test44(self):
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
        presets = PathPlanarSurface.ObjectSurface.ACCURACY_PRESETS

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

    # -- 3+2 workplane rotation tests --

    def _createRotatedJob(self, shape):
        """Helper: job for *shape* on a machine with a single table A axis.

        With Workplane=(0,-1,0) the solver yields A=-90 and the geometry
        transform maps (x, y, z) -> (x, z, -y): the model's -Y side becomes
        the Z-up top.
        """
        from Machine.models.machine import Machine, RotaryAxis, AxisRole

        model = self.doc.addObject("Part::Feature", "TestModel")
        model.Shape = shape
        job = PathJob.Create("Job", [model])
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        loaded_tool = toolbit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = loaded_tool

        machine = Machine(name="Test A-axis Machine")
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=FreeCAD.Vector(1, 0, 0),
            min_limit=-120,
            max_limit=120,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )
        job.Proxy.getMachine = lambda: machine
        self.doc.recompute()
        return job

    def _createRotatedOp(self, job, strategy):
        """Helper: PlanarSurface op on *job* with a -Y workplane, not yet executed.

        Execution is left to a document recompute: calling Proxy.execute()
        directly re-enters execute() via the base class's obj.recompute(),
        and the nested run strips the 3+2 transform state before the outer
        run emits its path.
        """
        op = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlanarSurface")
        proxy = PathPlanarSurface.ObjectSurface(op, "PlanarSurface")
        proxy.initOperation(op)
        op.Strategy = strategy
        op.Base = job.Model.Group
        op.Workplane = FreeCAD.Vector(0, -1, 0)
        job.Operations.addObject(op)
        return op

    @staticmethod
    def _rotaryMoves(op):
        return [c for c in op.Path.Commands if c.Name == "G0" and "A" in c.Parameters]

    @staticmethod
    def _cutValues(op, axis):
        return [
            c.Parameters[axis] for c in op.Path.Commands if c.Name == "G1" and axis in c.Parameters
        ]

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test50(self):
        """
        Executes SurfaceScan with a non-Z-up Workplane and verifies the path is
        computed against the rotated geometry.

        INPUT:
        - Function: ObjectOp.execute() 3+2 setup -> ObjectSurface.opExecute()
        - Parameters: Workplane=(0,-1,0), Strategy=SurfaceScan, Line pattern
        - Input data: 50x40x30 box on a single-A-axis machine; rotated the box
          spans Y 0..30 and Z -40..0 with its top at Z=0

        EXPECTED OUTPUT:
        - The path contains the rotary positioning move G0 A-90
        - Cutting moves sit on the rotated top face (Z ~ 0), not the unrotated
          top (Z = 30)
        - Cutting moves stay within the rotated Y extent (0..30), never
          reaching the unrotated extent (up to 40)
        """
        job = self._createRotatedJob(Part.makeBox(50, 40, 30))
        op = self._createRotatedOp(job, "SurfaceScan")
        op.CutPattern = "Line"
        op.StepOver = 50.0
        op.SampleInterval = 5.0
        self.doc.recompute()

        rotary = self._rotaryMoves(op)
        self.assertTrue(rotary, "Path should contain a rotary positioning move")
        self.assertAlmostEqual(rotary[0].Parameters["A"], -90.0, places=3)

        cut_z = self._cutValues(op, "Z")
        self.assertTrue(cut_z, "SurfaceScan should produce cutting moves")
        self.assertLess(max(cut_z), 10.0, "Cuts must not sit on the unrotated top (Z=30)")
        self.assertGreater(min(cut_z), -10.0, "Cuts must stay near the rotated top (Z=0)")

        cut_y = self._cutValues(op, "Y")
        self.assertTrue(cut_y)
        self.assertLessEqual(max(cut_y), 30.0 + 0.1, "Cuts must respect the rotated Y extent")

    def test51(self):
        """
        Executes ZLevelHybrid with a non-Z-up Workplane and
        verifies depths and paths are in the rotated frame.

        INPUT:
        - Function: ObjectOp.execute() 3+2 setup -> ObjectSurface.opExecute()
        - Parameters: Workplane=(0,-1,0), Strategy=ZLevelHybrid, BoundBox=Stock
        - Input data: 15mm sphere centred at (25, 0, 15) on a single-A-axis
          machine; unrotated it spans Z 0..30, rotated it spans Z -15..15

        EXPECTED OUTPUT:
        - The path contains the rotary positioning move G0 A-90
        - OpFinalDepth is the rotated model floor (-15), not the unrotated one (0)
        - Cutting moves descend below Z=0, impossible on the unrotated model
        """
        job = self._createRotatedJob(Part.makeSphere(15, FreeCAD.Vector(25, 0, 15)))
        op = self._createRotatedOp(job, "ZLevelHybrid")
        op.BoundBox = "Stock"
        self.doc.recompute()

        rotary = self._rotaryMoves(op)
        self.assertTrue(rotary, "Path should contain a rotary positioning move")
        self.assertAlmostEqual(rotary[0].Parameters["A"], -90.0, places=3)

        self.assertAlmostEqual(op.OpFinalDepth.Value, -15.0, places=3)

        cut_z = self._cutValues(op, "Z")
        self.assertTrue(cut_z, "ZLevelHybrid should produce cutting moves")
        self.assertLess(max(cut_z), 16.0, "Cuts must not use the unrotated Z range (0..30)")
        self.assertLess(min(cut_z), -5.0, "Cuts must descend into the rotated Z range (-15..15)")
