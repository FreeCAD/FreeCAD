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

"""Work planes shared between operations.

A shared work plane is a plain ``Part::LocalCoordinateSystem`` held in a group
on the Job. These tests cover the three things a prototype has to get right:
the resolution order an operation uses to find its frame, that a shared frame
really is shared - editing it once moves every operation using it - and that a
work plane derived from a face follows that face when the model changes.
"""

import FreeCAD

import Path.Base.Util as PathUtil
import Path.Main.Job as PathJob
import Path.Main.Workplane as PathWorkplane
import Path.Op.Custom as PathCustom
import Path.Op.Profile as PathProfile
import CAMTests.PathTestUtils as PathTestUtils

from FreeCAD import Vector


class TestSharedWorkplane(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathSharedWorkplane")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()
        # Most of these tests use tilted faces, which need rotary axes.
        self._rotary()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _model(self):
        return self.job.Model.Group[0]

    def _faceNamed(self, obj, normal):
        for i, face in enumerate(obj.Shape.Faces):
            if (face.normalAt(0, 0) - normal).Length < 1e-6:
                return "Face%d" % (i + 1)
        return None

    def _op(self, name="Op"):
        op = PathCustom.Create(name, parentJob=self.job)
        op.ToolController.Tool.Diameter = 5.0
        self.doc.recompute()
        return op

    def _threeAxis(self):
        from Machine.models.machine import Machine

        self.job.Proxy.getMachine = lambda: Machine(name="3 axis")

    def _rotary(self):
        from Machine.models.machine import Machine, RotaryAxis, AxisRole

        machine = Machine(name="CA")
        machine.rotary_axes["C"] = RotaryAxis(
            name="C", rotation_vector=Vector(0, 0, 1), role=AxisRole.TABLE_ROTARY, sequence=0
        )
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=Vector(1, 0, 0),
            role=AxisRole.TABLE_ROTARY,
            parent="C",
            sequence=1,
        )
        self.job.Proxy.getMachine = lambda: machine

    # --- what a plane may be without rotary axes ---------------------------

    def test_threeAxisJobAcceptsAPlaneParallelToTheTable(self):
        self._threeAxis()
        top = self._faceNamed(self.box, Vector(0, 0, 1))
        plane = PathWorkplane.createWorkplane(self.job, self.box, top)
        self.assertIn(plane, PathWorkplane.workplanesOf(self.job))
        self.assertAlmostEqual(plane.Placement.Base.z, 50, places=6)

    def test_threeAxisJobAcceptsATurnedX(self):
        self._threeAxis()
        turned = FreeCAD.Placement(Vector(10, 20, 0), FreeCAD.Rotation(Vector(0, 0, 1), 30))
        plane = PathWorkplane.createWorkplane(self.job, placement=turned)
        self.assertIn(plane, PathWorkplane.workplanesOf(self.job))

    def test_threeAxisJobAcceptsATiltedFace(self):
        """Creation is not gated on the machine: whether a tilted plane can
        be reached is the operation's and the post's question, asked when
        it matters. Here the operation records no rotary positions."""
        self._threeAxis()
        side = self._faceNamed(self.box, Vector(1, 0, 0))
        plane = PathWorkplane.createWorkplane(self.job, self.box, side)
        self.assertIn(plane, PathWorkplane.workplanesOf(self.job))
        op = self._op()
        op.Workplane = plane
        self.doc.recompute()
        z = op.Placement.Rotation.multVec(Vector(0, 0, 1))
        self.assertTrue(z.isEqual(Vector(1, 0, 0), 1e-6), "the op generates in the plane")
        self.assertNotIn("unavailable", op.Path.toGCode())

    # --- the plane's fixture ---------------------------------------------

    def test_aNewWorkplaneNamesNoFixture(self):
        top = self._faceNamed(self.box, Vector(0, 0, 1))
        plane = PathWorkplane.createWorkplane(self.job, self.box, top)
        self.assertEqual(plane.Fixture, "")
        self.assertIsNone(PathWorkplane.fixtureOf(plane))

    def test_fixtureOfIsTheTrimmedWordOrNone(self):
        plane = PathWorkplane.createWorkplane(self.job)
        plane.Fixture = "  G55 "
        self.assertEqual(PathWorkplane.fixtureOf(plane), "G55")
        plane.Fixture = "   "
        self.assertIsNone(PathWorkplane.fixtureOf(plane))
        self.assertIsNone(PathWorkplane.fixtureOf(None))

    def test_aPlaneFromAnOlderDocumentGainsTheFixturePropertyOnRestore(self):
        older = self.doc.addObject("Part::LocalCoordinateSystem", "Workplane")
        self.job.Workplanes.addObject(older)
        self.assertFalse(hasattr(older, "Fixture"))
        self.job.Proxy.onDocumentRestored(self.job)
        self.assertTrue(hasattr(older, "Fixture"))
        self.assertEqual(older.Fixture, "")

    def test_rotaryJobAcceptsATiltedFace(self):
        self._rotary()
        side = self._faceNamed(self.box, Vector(1, 0, 0))
        plane = PathWorkplane.createWorkplane(self.job, self.box, side)
        self.assertIn(plane, PathWorkplane.workplanesOf(self.job))

    # --- the group on the Job ----------------------------------------------

    def test_deletingTheJobTakesItsWorkplanesWithIt(self):
        top = self._faceNamed(self.box, Vector(0, 0, 1))
        plane = PathWorkplane.createWorkplane(self.job, self.box, top)
        names = [plane.Name, self.job.Workplanes.Name]
        self.assertTrue(all(self.doc.getObject(n) is not None for n in names))
        # What the view provider calls when the Job is deleted.
        self.job.Proxy.onDelete(self.job, None)
        self.doc.recompute()
        for name in names:
            self.assertIsNone(self.doc.getObject(name), "%s left behind" % name)

    def test_jobHasAWorkplanesGroup(self):
        """Every Job gets somewhere to keep named work planes."""
        self.assertTrue(hasattr(self.job, "Workplanes"))
        self.assertIsNotNone(self.job.Workplanes)
        self.assertEqual([], self.job.Workplanes.Group)

    def test_createdWorkplaneIsFiledUnderTheJob(self):
        workplane = PathWorkplane.createWorkplane(self.job)
        self.assertIn(workplane, self.job.Workplanes.Group)
        self.assertEqual("Part::LocalCoordinateSystem", workplane.TypeId)

    # --- resolution ------------------------------------------------------

    def test_noLinkIsTheJobXY(self):
        op = self._op()
        self.assertIsNone(op.Workplane)
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(0, 0, 1), 1e-9))

    def test_linkIsTheFrame(self):
        op = self._op()
        model = self._model()
        op.Workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(1, 0, 0))
        )
        self.doc.recompute()
        self.assertTrue(
            PathUtil.toolAxisForOp(op).isEqual(Vector(1, 0, 0), 1e-6),
            "the linked frame is the operation's frame, got %s" % PathUtil.toolAxisForOp(op),
        )

    def test_clearingTheLinkReturnsToTheJobXY(self):
        op = self._op()
        model = self._model()
        op.Workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(1, 0, 0))
        )
        self.doc.recompute()
        op.Workplane = None
        self.doc.recompute()
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(0, 0, 1), 1e-9))

    def test_attachingToTheOriginalResolvesToTheClone(self):
        """A plane picked on the user's object attaches to the Job's clone.

        Operations reference the clone; a plane attached to the original would
        follow an object they never look at."""
        clone = self._model()
        sub = self._faceNamed(self.box, Vector(0, 0, 1))
        workplane = PathWorkplane.createWorkplane(self.job, self.box, sub)
        support = workplane.AttachmentSupport
        self.assertTrue(support, "the plane must be attached")
        self.assertIs(support[0][0], clone, "attachment must target the Job's clone")

    # --- sharing ------------------------------------------------------------

    def test_oneEditMovesEveryOperationUsingIt(self):
        """The point of a shared frame: edit once, every user follows."""
        model = self._model()
        workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(0, 0, 1))
        )
        ops = [self._op("Op%d" % i) for i in range(3)]
        for op in ops:
            op.Workplane = workplane
        self.doc.recompute()

        for op in ops:
            self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(0, 0, 1), 1e-6))

        # Tilt the frame by 30 degrees about its own X, via the attachment
        # offset - the property the attachment editor exposes.
        workplane.AttachmentOffset = FreeCAD.Placement(
            Vector(0, 0, 0), FreeCAD.Rotation(Vector(1, 0, 0), 30)
        )
        self.doc.recompute()

        expected = FreeCAD.Rotation(Vector(1, 0, 0), 30).multVec(Vector(0, 0, 1))
        for op in ops:
            self.assertTrue(
                PathUtil.toolAxisForOp(op).isEqual(expected, 1e-6),
                "every operation sharing the frame must follow it",
            )

    def test_workplaneFollowsTheModelItWasDerivedFrom(self):
        """Attachment is why the frame is not a detached snapshot.

        The work plane attaches to the Job's Resource Clone, which is what the
        operations reference, not to the user's original object. A change to
        the original that reaches the clone's shape therefore moves the frame -
        which is the case that matters, because a detached placement would go
        quietly wrong instead."""
        model = self._model()
        workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(0, 0, 1))
        )
        op = self._op()
        op.Workplane = workplane
        self.doc.recompute()
        self.assertAlmostEqual(workplane.Placement.Base.z, 50.0, places=6)

        self.box.Height = 80
        self.doc.recompute()

        self.assertAlmostEqual(
            workplane.Placement.Base.z,
            80.0,
            places=6,
            msg="the frame must follow the face it was derived from",
        )
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(0, 0, 1), 1e-6))

    def test_workplaneFollowsTheModelOrientation(self):
        """Rotating the clone the operations reference reorients the frame."""
        model = self._model()
        workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(0, 0, 1))
        )
        op = self._op()
        op.Workplane = workplane
        self.doc.recompute()

        model.Placement = FreeCAD.Placement(Vector(0, 0, 0), FreeCAD.Rotation(Vector(1, 0, 0), 30))
        self.doc.recompute()

        expected = FreeCAD.Rotation(Vector(1, 0, 0), 30).multVec(Vector(0, 0, 1))
        self.assertTrue(
            PathUtil.toolAxisForOp(op).isEqual(expected, 1e-6),
            "the frame must follow the model, got %s" % PathUtil.toolAxisForOp(op),
        )

    # --- defaulting ---------------------------------------------------------

    def test_newOperationInheritsThePreviousOnesWorkplane(self):
        """A job on one tilted face should not need the frame set per op.

        The same rule the tool controller already follows."""
        model = self._model()
        workplane = PathWorkplane.createWorkplane(
            self.job, model, self._faceNamed(model, Vector(1, 0, 0))
        )
        first = PathProfile.Create("Profile")
        first.Workplane = workplane
        self.doc.recompute()

        second = PathProfile.Create("Profile2")
        self.doc.recompute()
        self.assertIs(
            second.Workplane,
            workplane,
            "a new operation should adopt the previous operation's work plane",
        )
