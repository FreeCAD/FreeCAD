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

"""Deprecation notices on operations.

An op declares a notice by overriding opDeprecationNotice(obj). The base op
logs it when a document is restored and the task panel shows it as a banner.
The notice is worded for the DeprecationStage the op is in: Warning, Hidden or
Deleted. A deleted op restores onto RemovedOp.
"""

import os
import tempfile

from unittest.mock import patch

import FreeCAD
import Part

import Path
import Path.Main.Job as PathJob
import Path.Op.Base as PathOp
import Path.Op.MillFace as PathMillFace
import Path.Op.Tapping as PathTapping
from CAMTests.PathTestUtils import PathTestBase

try:
    import ocl  # noqa: F401
except ImportError:
    try:
        import opencamlib as ocl  # noqa: F401
    except ImportError:
        ocl = None

if ocl is not None:
    import Path.Op.Surface as PathSurface
    import Path.Op.Waterline as PathWaterline

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathOpDeprecation(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestOpDeprecation")
        box = self.doc.addObject("Part::Feature", "Box")
        box.Shape = Part.makeBox(20, 20, 10)
        self.job = PathJob.Create("Job", [box])
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test000_notice_wording(self):
        """deprecationNotice() names subject and replacement and announces the next stage."""
        warning = PathOp.deprecationNotice("The X", "Y")
        self.assertIn("The X is deprecated", warning)
        self.assertIn("replaced by Y", warning)
        self.assertIn("hidden in the next release", warning)
        self.assertNotIn("cannot be created", warning)
        self.assertEqual(
            warning, PathOp.deprecationNotice("The X", "Y", stage=PathOp.DeprecationStage.Warning)
        )

        hidden = PathOp.deprecationNotice("The X", "Y", stage=PathOp.DeprecationStage.Hidden)
        self.assertIn("The X is deprecated", hidden)
        self.assertIn("replaced by Y", hidden)
        self.assertIn("deleted in the next release", hidden)
        self.assertIn("new ones cannot be created", hidden)

        deleted = PathOp.deprecationNotice("The X", "Y", stage=PathOp.DeprecationStage.Deleted)
        self.assertIn("The X has been deleted", deleted)
        self.assertIn("no longer generates a toolpath", deleted)
        self.assertIn("replaced by Y", deleted)

    def test005_unknown_stage_rejected(self):
        """A stage outside DeprecationStage is a programming error, not a silent default."""
        with self.assertRaises(ValueError):
            PathOp.deprecationNotice("The X", "Y", stage="Unsupported")

    def test010_default_is_none(self):
        """An op that declares nothing reports no notice and logs nothing on restore."""
        import Path.Op.Profile as PathProfile

        op = PathProfile.Create("Profile", parentJob=self.job)
        self.assertIsNone(op.Proxy.opDeprecationNotice(op))
        with patch.object(Path.Log, "warning") as warn:
            op.Proxy.onDocumentRestored(op)
        self.assertFalse(
            any("deprecated" in str(c) for c in warn.call_args_list),
            f"Unexpected deprecation warning: {warn.call_args_list}",
        )

    def test020_tapping_deprecated(self):
        """Tapping is in the Hidden stage and points at Drilling."""
        op = PathTapping.Create("Tapping", parentJob=self.job)
        notice = op.Proxy.opDeprecationNotice(op)
        self.assertIn("Tapping operation is deprecated", notice)
        self.assertIn("Drilling", notice)
        self.assertIn("deleted in the next release", notice)
        self.assertIn("new ones cannot be created", notice)

    def test030_face_deprecated(self):
        """The old Face op is in the Hidden stage and points at Mill Facing."""
        op = PathMillFace.Create("Face", parentJob=self.job)
        notice = op.Proxy.opDeprecationNotice(op)
        self.assertIn("Face operation is deprecated", notice)
        self.assertIn("Mill Facing", notice)
        self.assertIn("deleted in the next release", notice)
        self.assertIn("new ones cannot be created", notice)

    def test040_restore_logs_warning(self):
        """onDocumentRestored() logs the notice, prefixed by the op label."""
        op = PathTapping.Create("Tapping", parentJob=self.job)
        with patch.object(Path.Log, "warning") as warn:
            op.Proxy.onDocumentRestored(op)
        messages = [str(c.args[0]) for c in warn.call_args_list if c.args]
        self.assertTrue(
            any(m.startswith(op.Label) and "deprecated" in m for m in messages),
            f"Expected a deprecation warning, got: {messages}",
        )

    def test045_sanity_squawks_deprecated_op(self):
        """validate_job() reports a deprecated op as a squawk, so the post-process dialog shows it."""
        from Path.Main.Sanity.Sanity import CAMSanity

        op = PathTapping.Create("Tapping", parentJob=self.job)
        self.doc.recompute()
        all_squawks, critical = CAMSanity.validate_job(self.job)
        notes = [s["Note"] for s in all_squawks if "deprecated" in s["Note"]]
        self.assertTrue(
            any(op.Label in n and "Drilling" in n for n in notes),
            f"Expected a deprecation squawk for {op.Label}, got: {all_squawks}",
        )
        self.assertFalse(any("deprecated" in s["Note"] for s in critical))

    def test050_surface_rotational_only(self):
        """3D Surface is in the Warning stage only while its ScanType is Rotational."""
        if ocl is None:
            self.skipTest("OpenCamLib not available")
        op = PathSurface.Create("Surface", parentJob=self.job)
        op.ScanType = "Planar"
        self.assertIsNone(op.Proxy.opDeprecationNotice(op))
        op.ScanType = "Rotational"
        notice = op.Proxy.opDeprecationNotice(op)
        self.assertIn("Rotational scan type is deprecated", notice)
        self.assertIn("Rotary Surface", notice)
        self.assertIn("hidden in the next release", notice)
        self.assertNotIn("cannot be created", notice)

    def test055_waterline_deprecated(self):
        """Waterline is in the Warning stage: superseded, still creatable."""
        if ocl is None:
            self.skipTest("OpenCamLib not available")
        op = PathWaterline.Create("Waterline", parentJob=self.job)
        notice = op.Proxy.opDeprecationNotice(op)
        self.assertIn("Waterline operation is deprecated", notice)
        self.assertIn("Planar Surface operation with Strategy set to 'Waterline'", notice)
        self.assertIn("hidden in the next release", notice)
        self.assertNotIn("cannot be created", notice)

    def test060_deleted_op_restores_onto_removed_op(self):
        """A document holding an op whose implementation was deleted restores onto RemovedOp.

        Given: a saved document with a Tapping op, and Path.Op.Tapping.ObjectTapping
               replaced by a RemovedOp subclass, as it is when the op is deleted.
        When: the document is opened.
        Then: the op loads with its properties, has no toolpath, the notice is logged as
              an error, and the Sanity Check reports it as critical.
        """
        from Path.Main.Sanity.Sanity import CAMSanity

        class ObjectTapping(PathOp.RemovedOp):
            def opDeprecationNotice(self, obj):
                return PathOp.deprecationNotice(
                    "The Tapping operation",
                    "the Drilling operation",
                    stage=PathOp.DeprecationStage.Deleted,
                )

        op = PathTapping.Create("Tapping", parentJob=self.job)
        op.Path = Path.Path([Path.Command("G0", {"Z": 5.0})])
        opName, jobName = op.Name, self.job.Name
        fd, filename = tempfile.mkstemp(suffix=".FCStd")
        os.close(fd)
        try:
            self.doc.saveAs(filename)
            FreeCAD.closeDocument(self.doc.Name)
            with (
                patch.object(PathTapping, "ObjectTapping", ObjectTapping),
                patch.object(Path.Log, "error") as error,
            ):
                self.doc = FreeCAD.openDocument(filename)
            restored = self.doc.getObject(opName)
            job = self.doc.getObject(jobName)

            self.assertIsInstance(restored.Proxy, PathOp.RemovedOp)
            self.assertTrue(hasattr(restored, "DwellTime"), "properties should survive")
            self.assertEqual(len(restored.Path.Commands), 0)
            messages = [str(c.args[0]) for c in error.call_args_list if c.args]
            self.assertTrue(
                any(m.startswith(restored.Label) and "has been deleted" in m for m in messages),
                f"Expected a deletion error, got: {messages}",
            )

            self.doc.recompute()
            self.assertEqual(len(restored.Path.Commands), 0)

            all_squawks, critical = CAMSanity.validate_job(job)
            self.assertTrue(
                any(
                    s["squawkType"] == "CAUTION"
                    and restored.Label in s["Note"]
                    and "has been deleted" in s["Note"]
                    for s in critical
                ),
                f"Expected a critical squawk for {restored.Label}, got: {all_squawks}",
            )
        finally:
            os.remove(filename)
