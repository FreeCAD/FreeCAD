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


"""Integration tests for the Surface3D operation."""

import unittest

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Surface3D as PathSurface3D
from CAMTests.PathTestUtils import PathTestWithAssets

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


_ocl_available = False
try:
    try:
        import ocl  # noqa: F401

        _ocl_available = True
    except ImportError:
        import opencamlib as ocl  # noqa: F401

        _ocl_available = True
except ImportError:
    pass


class TestSurface3DOp(PathTestWithAssets):
    """Integration tests for ObjectSurface3D.

    These tests build a FreeCAD document with a Job, model geometry,
    and tool controller, then exercise the Surface3D operation to verify
    end-to-end behaviour: property defaults, strategy dispatch, depth
    inference, AccuracyLevel preset, and toolpath generation.
    """

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestSurface3D")

    def tearDown(self):
        super().tearDown()
        FreeCAD.closeDocument(self.doc.Name)

    # -- Helpers ---------------------------------------------------------

    def _createJobWithBox(self, length=20, width=20, height=5):
        box = self.doc.addObject("Part::Feature", "TestBox")
        box.Shape = Part.makeBox(length, width, height)

        job = PathJob.Create("Job", [box])

        toolbit = self.assets.get("toolbit://5mm_Endmill")
        loaded_tool = toolbit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = loaded_tool

        self.doc.recompute()
        return job

    def _createJobWithSteppedBlock(self):
        """Stepped block: 20×20×5 base with a 6×6×3 step on top."""
        base = Part.makeBox(20, 20, 5)
        step = Part.makeBox(6, 6, 3, FreeCAD.Vector(7, 7, 5))
        body = self.doc.addObject("Part::Feature", "Stepped")
        body.Shape = base.fuse(step)

        job = PathJob.Create("Job", [body])

        toolbit = self.assets.get("toolbit://5mm_Endmill")
        loaded_tool = toolbit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = loaded_tool

        self.doc.recompute()
        return job

    # -- Property definition tests --------------------------------------

    def test00_create(self):
        """Surface3D op creates with the expected feature flags and default Strategy."""
        job = self._createJobWithBox()
        op = PathSurface3D.Create("Surface3D", parentJob=job)
        self.assertEqual(op.Strategy, "SurfacePattern")
        self.assertEqual(op.CutPattern, "ZigZag")
        self.assertEqual(op.CutMode, "Climb")

    def test01_strategy_enum(self):
        """Strategy enum exposes the three v1 values."""
        enums = PathSurface3D.ObjectSurface3D.propertyEnumerations()
        enum_dict = dict(enums)
        self.assertIn("Strategy", enum_dict)
        for s in ("SurfacePattern", "Waterline", "ZLevelHybrid"):
            self.assertIn(s, enum_dict["Strategy"])

    def test02_cutpattern_enum(self):
        """CutPattern enum exposes the four 2D-projection patterns."""
        enums = PathSurface3D.ObjectSurface3D.propertyEnumerations()
        enum_dict = dict(enums)
        self.assertIn("CutPattern", enum_dict)
        for p in ("Line", "ZigZag", "Circular", "Spiral"):
            self.assertIn(p, enum_dict["CutPattern"])

    def test03_zlevel_enum(self):
        """CutPatternZLevel enum exposes the three ZLevel floor patterns."""
        enums = PathSurface3D.ObjectSurface3D.propertyEnumerations()
        enum_dict = dict(enums)
        self.assertIn("CutPatternZLevel", enum_dict)
        for p in ("None", "Line", "Spiral"):
            self.assertIn(p, enum_dict["CutPatternZLevel"])

    def test04_op_features(self):
        """opFeatures advertises the standard feature set used by Surface3D."""
        import Path.Op.Base as PathOp

        proxy = PathSurface3D.ObjectSurface3D.__new__(PathSurface3D.ObjectSurface3D)
        # Need an object handle to satisfy opFeatures(obj); a stub works because
        # the implementation doesn't read it.
        features = proxy.opFeatures(None)
        for flag in (
            PathOp.FeatureTool,
            PathOp.FeatureDepths,
            PathOp.FeatureHeights,
            PathOp.FeatureStepDown,
            PathOp.FeatureCoolant,
            PathOp.FeatureBaseFaces,
        ):
            self.assertTrue(features & flag, "Surface3D should advertise {}".format(flag))

    # -- Depth inference -------------------------------------------------

    def test10_default_finaldepth_is_model_floor(self):
        """opUpdateDepths sets OpFinalDepth to model ZMin when no Base is selected.

        The base ObjectOp.updateDepths would otherwise set FinalDepth to model
        ZMax, which produces an empty pre-clip volume in surface_mesh.
        """
        job = self._createJobWithBox()
        op = PathSurface3D.Create("Surface3D", parentJob=job)
        self.doc.recompute()
        # Box is 20×20×5 at origin, so model floor is 0.
        self.assertAlmostEqual(op.OpFinalDepth.Value, 0.0, places=3)

    # -- AccuracyLevel preset --------------------------------------------

    def test20_accuracy_preset_drives_dependent_props(self):
        """Setting AccuracyLevel writes the linked deflection/sample-interval set."""
        job = self._createJobWithBox()
        op = PathSurface3D.Create("Surface3D", parentJob=job)

        op.AccuracyLevel = 7
        self.assertAlmostEqual(op.LinearDeflection.Value, 0.005, places=4)
        self.assertAlmostEqual(op.SampleInterval.Value, 0.05, places=4)
        self.assertEqual(op.MeshSimplification, 1)

        op.AccuracyLevel = 1
        self.assertAlmostEqual(op.LinearDeflection.Value, 0.1, places=4)
        self.assertAlmostEqual(op.SampleInterval.Value, 1.5, places=4)
        self.assertEqual(op.MeshSimplification, 7)

    # -- Strategy execution ---------------------------------------------

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test30_surfacepattern_emits_g1(self):
        """SurfacePattern strategy produces G1 cutting moves on a stepped block."""
        job = self._createJobWithSteppedBlock()
        op = PathSurface3D.Create("Surface3D_Pattern", parentJob=job)
        op.Strategy = "SurfacePattern"
        op.CutPattern = "ZigZag"
        op.StepDown = 1.0
        self.doc.recompute()

        self.assertIsNotNone(op.Path)
        names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", names, "Should contain rapid moves")
        self.assertIn("G1", names, "Should contain cutting moves")

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test33_surfacepattern_multipass_emits_intermediate_layers(self):
        """SurfacePattern with StepDown < depth-range emits multiple Z-layers.

        Regression for the apply_multipass wiring.  Stepped-block: step
        top at Z=8, base top at Z=5.  With StartDepth=8, FinalDepth=0,
        StepDown=2 we expect cuts at Z=8 (step roughed at start),
        Z=6 (step layer + air-roughing over base), and Z=5 (base
        riding the surface), at minimum.
        """
        job = self._createJobWithSteppedBlock()
        op = PathSurface3D.Create("Surface3D_Multi", parentJob=job)
        op.Strategy = "SurfacePattern"
        op.CutPattern = "ZigZag"
        op.setExpression("StartDepth", None)
        op.setExpression("FinalDepth", None)
        op.setExpression("StepDown", None)
        op.StartDepth = 8.0
        op.FinalDepth = 0.0
        op.StepDown = 2.0
        self.doc.recompute()

        self.assertIsNotNone(op.Path)
        cut_zs = {
            round(c.Parameters["Z"], 2)
            for c in op.Path.Commands
            if c.Name in ("G1", "G2", "G3") and "Z" in c.Parameters
        }
        # The exact set depends on drop-cutter sampling, but multi-pass
        # MUST produce more than just the surface Zs (5, 8).
        self.assertGreaterEqual(
            len(cut_zs),
            3,
            "Multi-pass should emit cuts at >=3 distinct Z-levels, got {}".format(cut_zs),
        )

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test31_waterline_emits_g1(self):
        """Waterline strategy produces multi-Z contour cuts.

        Per the v1 W1.1 redirect, the 'Waterline' strategy is implemented
        via AdaptiveWaterline with min_sampling=sampling, so this also
        verifies that path is wired.
        """
        job = self._createJobWithSteppedBlock()
        op = PathSurface3D.Create("Surface3D_Waterline", parentJob=job)
        op.Strategy = "Waterline"
        op.StepDown = 1.0
        self.doc.recompute()

        self.assertIsNotNone(op.Path)
        names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", names)
        self.assertIn("G1", names)

    def test32_zlevelhybrid_no_ocl_required(self):
        """ZLevelHybrid produces toolpath via Path.Area without OCL."""
        job = self._createJobWithSteppedBlock()
        op = PathSurface3D.Create("Surface3D_ZLevel", parentJob=job)
        op.Strategy = "ZLevelHybrid"
        op.StepDown = 1.0
        self.doc.recompute()

        self.assertIsNotNone(op.Path)
        names = [c.Name for c in op.Path.Commands]
        self.assertIn("G0", names)
        # ZLevelHybrid emits G1 verticals plus G2/G3 arcs around the step.
        self.assertTrue(
            any(n in ("G1", "G2", "G3") for n in names),
            "ZLevelHybrid should emit at least one feed/arc command",
        )


def setup_test():
    return TestSurface3DOp


if __name__ == "__main__":
    unittest.main()
