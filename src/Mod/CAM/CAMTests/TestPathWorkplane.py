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

"""Tests for the Workplane property and its accessors.

The Workplane property began as an App::PropertyVector holding a tool axis,
became an App::PropertyPlacement holding a full frame, and is now an
App::PropertyLink to a named work plane held by the Job. These tests pin the
things that must not drift:

  * the convention used to build a placement from a tool axis, because document
    migration reproduces it and a change would rotate existing operations in
    plane;
  * the migration itself;
  * the fact that only the tool axis is consumed today - the origin and the
    in-plane X are recorded, and the frame-equality predicate compares axes
    only. When origins are consumed these expectations change together, which
    is why they are written down;
  * that an operation's Placement is left alone, because the path viewer
    already compensates for the rotary words in a rotated path.
"""

import math

import FreeCAD
import Part

import Path.Base.Util as PathUtil
import Path.Op.Custom as PathCustom
import Path.Op.MillFacing as PathMillFacing
import Path.Main.Job as PathJob
import Path.Main.Workplane as PathWorkplane
import CAMTests.PathTestUtils as PathTestUtils

from FreeCAD import Vector


class TestWorkplaneAccessors(PathTestUtils.PathTestBase):
    """placementFromToolAxis / workplaneForOp / toolAxisForOp / sameWorkplane."""

    def test_identityAxisGivesIdentityRotation(self):
        """A +Z tool axis is the default frame and must not rotate anything."""
        pla = PathUtil.placementFromToolAxis(Vector(0, 0, 1))
        self.assertTrue(pla.Rotation.isIdentity(1e-9), "Z-up axis must give an identity rotation")
        self.assertTrue(pla.Base.isEqual(Vector(0, 0, 0), 1e-9))

    def test_placementLocalZIsToolAxis(self):
        """The placement's local +Z is the axis it was built from."""
        for axis in (Vector(1, 0, 0), Vector(0, 1, 0), Vector(1, 1, 1), Vector(0, -1, 0)):
            expected = Vector(axis)
            expected.normalize()
            pla = PathUtil.placementFromToolAxis(axis)
            got = pla.Rotation.multVec(Vector(0, 0, 1))
            self.assertTrue(
                got.isEqual(expected, 1e-9),
                "Local +Z of the placement must be the tool axis: %s != %s" % (got, expected),
            )

    def test_originIsRecorded(self):
        """An origin passed in is stored on the placement."""
        pla = PathUtil.placementFromToolAxis(Vector(0, 0, 1), Vector(10, 20, 30))
        self.assertTrue(pla.Base.isEqual(Vector(10, 20, 30), 1e-9))

    def test_degenerateAxisFallsBackToZUp(self):
        """A zero-length axis must not produce a broken rotation."""
        pla = PathUtil.placementFromToolAxis(Vector(0, 0, 0))
        self.assertTrue(pla.Rotation.isIdentity(1e-9))

    def test_sameWorkplaneIgnoresOrigin(self):
        """Two frames with one tool axis are the same frame today.

        This is correct only while a Workplane's origin is recorded and not
        consumed. When origins reach the generated path this assertion must be
        inverted, and every caller that reuses geometry between operations -
        rest machining in particular - has to change with it."""
        a = PathUtil.placementFromToolAxis(Vector(0, 0, 1), Vector(0, 0, 0))
        b = PathUtil.placementFromToolAxis(Vector(0, 0, 1), Vector(50, 50, 0))
        self.assertTrue(PathUtil.sameWorkplane(a, b))

    def test_sameWorkplaneDistinguishesAxes(self):
        a = PathUtil.placementFromToolAxis(Vector(0, 0, 1))
        b = PathUtil.placementFromToolAxis(Vector(1, 0, 0))
        self.assertFalse(PathUtil.sameWorkplane(a, b))


class TestWorkplaneOnOperation(PathTestUtils.PathTestBase):
    """The property on a real operation: default, migration, derived Placement."""

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathWorkplane")
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 100
        box.Width = 100
        box.Height = 100
        self.doc.recompute()
        self.job = PathJob.Create("Job", [box], None)
        self.job.GeometryTolerance.Value = 0.001
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _makeOp(self, name="Op"):
        op = PathCustom.Create(name, parentJob=self.job)
        op.ToolController.Tool.Diameter = 5.0
        self.doc.recompute()
        return op

    def _attachMachine(self):
        """Attach a C-A table-table machine to the job via getMachine."""
        from Machine.models.machine import Machine, RotaryAxis, AxisRole

        machine = Machine(name="Test CA Machine")
        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=Vector(0, 0, 1),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=Vector(1, 0, 0),
            min_limit=-120,
            max_limit=120,
            role=AxisRole.TABLE_ROTARY,
            parent="C",
            sequence=1,
        )
        self.job.Proxy.getMachine = lambda: machine

    def test_defaultIsAnEmptyLink(self):
        """A new operation links to no work plane: the Job's own XY."""
        op = self._makeOp()
        self.assertEqual("App::PropertyLink", op.getTypeIdOfProperty("Workplane"))
        self.assertIsNone(op.Workplane)
        self.assertTrue(PathUtil.workplaneForOp(op).isIdentity(1e-9))
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(0, 0, 1), 1e-9))

    def test_missingPropertyReadsAsIdentity(self):
        """An operation predating the property is ordinary Z-up milling."""
        op = self._makeOp()
        op.removeProperty("Workplane")
        self.assertTrue(PathUtil.workplaneForOp(op).isIdentity(1e-9))

    def test_linkedPlaneIsTheFrame(self):
        self._attachMachine()  # tilted planes need rotary axes
        op = self._makeOp()
        op.Workplane = PathWorkplane.createWorkplaneFromToolAxis(self.job, Vector(1, 0, 0))
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(1, 0, 0), 1e-9))

    def test_migrationFromVectorProperty(self):
        """A document written with the vector property migrates to a linked plane.

        The migrated frame's local +Z must be the axis the vector held, or the
        operation would generate its path in a different frame after an
        upgrade. A non-identity frame becomes a work plane object of its own,
        filed under the Job."""
        op = self._makeOp()
        axis = Vector(0, 1, 0)
        op.removeProperty("Workplane")
        op.addProperty("App::PropertyVector", "Workplane", "Path", "legacy")
        op.Workplane = axis

        op.Proxy.onDocumentRestored(op)

        self.assertEqual("App::PropertyLink", op.getTypeIdOfProperty("Workplane"))
        self.assertIsNotNone(op.Workplane, "a tilted frame must become a linked plane")
        self.assertIn(op.Workplane, self.job.Workplanes.Group)
        self.assertTrue(
            PathUtil.toolAxisForOp(op).isEqual(axis, 1e-9),
            "Migrated tool axis must equal the vector the document held",
        )

    def test_migrationFromPlacementProperty(self):
        """A document written with the placement form migrates the same way,
        origin included."""
        op = self._makeOp()
        frame = PathUtil.placementFromToolAxis(Vector(1, 0, 0), Vector(5, 6, 7))
        op.removeProperty("Workplane")
        op.addProperty("App::PropertyPlacement", "Workplane", "Path", "legacy")
        op.Workplane = frame

        op.Proxy.onDocumentRestored(op)

        self.assertEqual("App::PropertyLink", op.getTypeIdOfProperty("Workplane"))
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(1, 0, 0), 1e-9))
        self.assertTrue(op.Workplane.Placement.Base.isEqual(Vector(5, 6, 7), 1e-9))

    def test_identityLegacyFrameMigratesToNoLink(self):
        """A Z-up legacy frame must not litter the Job with a redundant plane."""
        op = self._makeOp()
        op.removeProperty("Workplane")
        op.addProperty("App::PropertyVector", "Workplane", "Path", "legacy")
        op.Workplane = Vector(0, 0, 1)

        op.Proxy.onDocumentRestored(op)

        self.assertIsNone(op.Workplane)
        self.assertEqual([], self.job.Workplanes.Group)

    def test_migrationFromPrototypeLink(self):
        """A prototype document with placement plus WorkplaneLink keeps the link."""
        self._attachMachine()  # tilted planes need rotary axes
        op = self._makeOp()
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, Vector(0, 1, 0))
        op.removeProperty("Workplane")
        op.addProperty("App::PropertyPlacement", "Workplane", "Path", "legacy")
        op.Workplane = PathUtil.placementFromToolAxis(Vector(1, 0, 0))
        op.addProperty("App::PropertyLink", "WorkplaneLink", "Path", "legacy")
        op.WorkplaneLink = plane

        op.Proxy.onDocumentRestored(op)

        self.assertFalse(hasattr(op, "WorkplaneLink"))
        self.assertIs(op.Workplane, plane, "the prototype link must win over the placement")

    def test_migrationIsIdempotent(self):
        self._attachMachine()  # tilted planes need rotary axes
        op = self._makeOp()
        plane = PathWorkplane.createWorkplaneFromToolAxis(
            self.job, Vector(1, 0, 0), Vector(5, 6, 7)
        )
        op.Workplane = plane
        op.Proxy.onDocumentRestored(op)
        self.assertIs(op.Workplane, plane)
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(1, 0, 0), 1e-9))

    def test_vectorPropertyReadableBeforeMigration(self):
        """workplaneForOp copes with a legacy property not yet migrated."""
        op = self._makeOp()
        op.removeProperty("Workplane")
        op.addProperty("App::PropertyVector", "Workplane", "Path", "legacy")
        op.Workplane = Vector(1, 0, 0)
        self.assertTrue(PathUtil.toolAxisForOp(op).isEqual(Vector(1, 0, 0), 1e-9))

    def test_zUpOpKeepsIdentityPlacement(self):
        """A three-axis operation must not acquire a placement."""
        op = self._makeOp()
        self.doc.recompute()
        self.assertTrue(op.Placement.isIdentity(1e-9))

    def test_rotatedOpKeepsIdentityPlacement(self):
        """A rotated operation must not acquire a placement either.

        A path generated in a rotated workplane carries the rotary A/B/C words
        that produced it, and Path::PathSegmentWalker applies
        compensateRotation() to every point when drawing, mapping those
        rotated-frame coordinates back to world. The toolpath is therefore
        already drawn on the part. Deriving a Placement from the workplane to
        "put the path back" applies that compensation a second time and draws
        the path off the part - so an operation's Placement must stay whatever
        the user left it, and nothing in execute() may write it."""
        self._attachMachine()
        op = self._makeOp("Side")
        op.Workplane = PathWorkplane.createWorkplaneFromToolAxis(self.job, Vector(0, 1, 0))
        self.doc.recompute()

        self.assertTrue(
            op.Placement.isIdentity(1e-9),
            "execute() must not write Placement: the path viewer already "
            "compensates for the rotary words in the path",
        )


class TestDepthFromSelection(PathTestUtils.PathTestBase):
    """PathUtil.depthOfFeature - the value the Start/Final helper buttons set.

    A depth is a coordinate in the frame the operation generates in, so a
    selected face names a depth when it lies in a plane perpendicular to the
    tool axis - the default work plane's +Z for three-axis work, the work
    plane's own +Z otherwise."""

    Z_UP = Vector(0, 0, 1)

    @staticmethod
    def _tilted():
        a = math.radians(45)
        return Vector(0, -math.sin(a), math.cos(a))

    @staticmethod
    def _planeFacing(axis, distance):
        """A square plane perpendicular to axis, distance along it from origin."""
        face = Part.makePlane(10, 10)
        face.Placement = PathUtil.placementFromToolAxis(axis, Vector(axis).multiply(distance))
        return face

    def test_zUpFaceUnchanged(self):
        """A horizontal face with the default work plane reads its Z, as before."""
        face = self._planeFacing(self.Z_UP, 7.0)
        self.assertAlmostEqual(PathUtil.depthOfFeature(face, self.Z_UP), 7.0, places=6)

    def test_zUpRejectsVerticalFace(self):
        """A face that is not horizontal names no depth with the default plane."""
        face = self._planeFacing(Vector(1, 0, 0), 7.0)
        self.assertIsNone(PathUtil.depthOfFeature(face, self.Z_UP))

    def test_vertexAlwaysNamesADepth(self):
        """A vertex projects onto the tool axis in every frame."""
        vertex = Part.Vertex(FreeCAD.Vector(3, 4, 5))
        self.assertAlmostEqual(PathUtil.depthOfFeature(vertex, self.Z_UP), 5.0, places=6)
        axis = self._tilted()
        self.assertAlmostEqual(
            PathUtil.depthOfFeature(vertex, axis),
            axis.dot(FreeCAD.Vector(3, 4, 5)),
            places=6,
        )

    def test_faceParallelToWorkplaneNamesADepth(self):
        """A face parallel to a tilted work plane reads its distance along the axis.

        This is the case the Start/Final helper buttons used to refuse: the
        face is not parallel to XY, but it is parallel to the work plane, and
        its depth is perfectly well defined."""
        axis = self._tilted()
        face = self._planeFacing(axis, 7.0)
        self.assertAlmostEqual(PathUtil.depthOfFeature(face, axis), 7.0, places=6)

    def test_faceNotParallelToWorkplaneNamesNoDepth(self):
        """A horizontal face names no depth once the work plane is tilted."""
        axis = self._tilted()
        face = self._planeFacing(self.Z_UP, 7.0)
        self.assertIsNone(PathUtil.depthOfFeature(face, axis))

    def test_edgeInWorkplaneNamesADepth(self):
        axis = self._tilted()
        face = self._planeFacing(axis, 4.0)
        edge = face.Edges[0]
        self.assertTrue(PathUtil.liesInPlanePerpendicularTo(edge, axis))
        self.assertAlmostEqual(PathUtil.depthOfFeature(edge, axis), 4.0, places=6)

    def test_edgeCrossingDepthsNamesNoDepth(self):
        axis = self._tilted()
        edge = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 10))
        self.assertFalse(PathUtil.liesInPlanePerpendicularTo(edge, axis))
        self.assertIsNone(PathUtil.depthOfFeature(edge, axis))


class TestDepthDefaultsAcrossWorkplaneChange(PathTestUtils.PathTestBase):
    """Heights and depths when a work plane is assigned after an operation exists.

    The values an operation generates from are bound to the computed Op values
    by SetupSheet expressions. Editing any of those fields clears its
    expression, after which it tracks nothing - so assigning a work plane moves
    the Op values into the new frame and leaves the generating values behind,
    in a frame that no longer exists."""

    DEPTH_PROPS = ["StartDepth", "FinalDepth", "ClearanceHeight", "SafeHeight"]

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathWorkplaneDepths")
        box = self.doc.addObject("Part::Box", "Box")
        box.Length, box.Width, box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [box], None)
        self.doc.recompute()

        from Machine.models.machine import Machine, RotaryAxis, AxisRole

        machine = Machine(name="Test CA Machine")
        machine.rotary_axes["C"] = RotaryAxis(
            name="C", rotation_vector=Vector(0, 0, 1), role=AxisRole.TABLE_ROTARY, sequence=0
        )
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=Vector(1, 0, 0),
            min_limit=-120,
            max_limit=120,
            role=AxisRole.TABLE_ROTARY,
            parent="C",
            sequence=1,
        )
        self.job.Proxy.getMachine = lambda: machine

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _tiltedAxis(self):
        a = math.radians(45)
        return Vector(0, -math.sin(a), math.cos(a))

    def _facing(self, name="Facing"):
        op = PathMillFacing.Create(name)
        self.doc.recompute()
        return op

    def _breakExpressions(self, op):
        """What the task panel does when the user types into these fields."""
        for prop in self.DEPTH_PROPS:
            op.setExpression(prop, None)
            setattr(op, prop, getattr(op, prop).Value)
        self.doc.recompute()

    def _tilt(self, op):
        op.Workplane = PathWorkplane.createWorkplaneFromToolAxis(self.job, self._tiltedAxis())
        self.doc.recompute()

    def test_intactExpressionsFollowTheWorkplane(self):
        """Untouched fields re-derive themselves when the frame changes."""
        op = self._facing()
        before = {p: getattr(op, p).Value for p in self.DEPTH_PROPS}
        self._tilt(op)
        for prop in self.DEPTH_PROPS:
            self.assertNotAlmostEqual(
                getattr(op, prop).Value,
                before[prop],
                places=3,
                msg="%s should re-derive in the rotated frame" % prop,
            )

    def test_editedFieldsDoNotFollowTheWorkplane(self):
        """Once edited, the fields stay in the frame they were computed in.

        Documenting the behaviour, not endorsing it: this is why the Heights
        page carries a Reset to defaults button, and it is what makes depths
        measured as coordinates rather than as distances along the tool axis
        an awkward representation."""
        op = self._facing()
        self._breakExpressions(op)
        before = {p: getattr(op, p).Value for p in self.DEPTH_PROPS}
        self._tilt(op)

        self.assertNotAlmostEqual(op.OpFinalDepth.Value, before["FinalDepth"], places=3)
        for prop in self.DEPTH_PROPS:
            self.assertAlmostEqual(getattr(op, prop).Value, before[prop], places=6)

    def test_staleDepthsFailLegibly(self):
        """An operation whose depths predate the work plane refuses to generate.

        Before this check the same state produced 'No shape found at final
        depth' from inside a generator, or a plausible path in the wrong
        place."""
        op = self._facing()
        self._breakExpressions(op)
        self._tilt(op)

        self.assertTrue(op.Path.Commands, "the operation should emit a marker, not nothing")
        self.assertIn(
            "work plane",
            op.Path.Commands[0].Name,
            "a stale-depth operation must say so in its path",
        )

    def test_resetRestoresExpressionsNotJustValues(self):
        """Reset to defaults repairs the link, so later changes track again."""
        op = self._facing()
        self._breakExpressions(op)
        self._tilt(op)

        self.assertTrue(op.Proxy.resetDepthDefaults(op))
        self.doc.recompute()

        expressions = dict(op.ExpressionEngine)
        for prop in ["StartDepth", "FinalDepth", "ClearanceHeight", "SafeHeight"]:
            self.assertIn(
                prop,
                expressions,
                "%s must get its expression back, or it goes stale again "
                "on the next change" % prop,
            )

        self.assertAlmostEqual(op.StartDepth.Value, op.OpStartDepth.Value, places=6)
        self.assertAlmostEqual(op.FinalDepth.Value, op.OpFinalDepth.Value, places=6)
        self.assertTrue(len(op.Path.Commands) > 1, "the operation should generate again")
