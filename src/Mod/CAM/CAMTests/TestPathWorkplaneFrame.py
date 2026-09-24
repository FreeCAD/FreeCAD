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

"""Operations generate in the work plane's own frame.

The plane's origin is (0, 0, 0) of the frame an operation generates in, its X
is the plane's X, and Z is the tool axis. The path is stored in that frame with
no rotary words; the operation's Placement positions it, and the operation
records the rotary positions it was solved for. The post-processor commands
those positions and transforms the path into the frame the machine reaches.

These tests pin the four things that have to be true for that to hold:

  * the stored path is plane-relative and Placement puts it on the part -
    checked geometrically, by asking where the placed path lies in the world;
  * a depth entered relative to the plane keeps meaning the same thing when
    the plane moves;
  * the post turns a plane-frame path plus recorded positions into what a
    dynamic-work-offset machine runs: a rotary move, then coordinates in the
    frame the machine reaches;
  * rest machining still sees an operation on a parallel plane at a different
    depth.
"""

import math

import FreeCAD
import Part

import Path
import Path.Base.Generator.rotation as rotation
import Path.Base.Util as PathUtil
import Path.Main.Job as PathJob
import Path.Main.Workplane as PathWorkplane
import Path.Op.Custom as PathCustom
import Path.Op.Engrave as PathEngrave
import Path.Op.MillFacing as PathMillFacing
import Path.Op.Profile as PathProfile
import Path.Op.Util as PathOpUtil
import CAMTests.PathTestUtils as PathTestUtils
import PathScripts.PathUtils as PathUtils

from FreeCAD import Vector


def _machineCA():
    from Machine.models.machine import Machine, RotaryAxis, AxisRole

    m = Machine(name="Test CA Machine")
    m.rotary_axes["C"] = RotaryAxis(
        name="C", rotation_vector=Vector(0, 0, 1), role=AxisRole.TABLE_ROTARY, sequence=0
    )
    m.rotary_axes["A"] = RotaryAxis(
        name="A",
        rotation_vector=Vector(1, 0, 0),
        min_limit=-120,
        max_limit=120,
        role=AxisRole.TABLE_ROTARY,
        parent="C",
        sequence=1,
    )
    return m


def _tilted():
    a = math.radians(45)
    return Vector(0, -math.sin(a), math.cos(a))


def _cutPoints(path):
    """Points of the cutting moves, tracking modal X/Y/Z."""
    cur = [None, None, None]
    pts = []
    for c in path.Commands:
        if c.Name not in ("G0", "G00", "G1", "G01", "G2", "G02", "G3", "G03"):
            continue
        for i, k in enumerate("XYZ"):
            if k in c.Parameters:
                cur[i] = c.Parameters[k]
        if c.Name in ("G1", "G01", "G2", "G02", "G3", "G03") and None not in cur:
            pts.append(Vector(*cur))
    return pts


def _leadInOut(doc, job, op):
    """A Lead In/Out dressup on op, set up as the command does."""
    import Path.Dressup.Gui.LeadInOut as PathLeadInOut

    dressup = doc.addObject("Path::FeaturePython", "LeadInOut")
    proxy = PathLeadInOut.ObjectDressup(dressup, op)
    job.Proxy.addOperation(dressup, op)
    proxy.setup(dressup)
    doc.recompute()
    return dressup


class TestGenerateInPlaneFrame(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathWorkplaneFrame")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()
        self.machine = _machineCA()
        self.job.Proxy.getMachine = lambda: self.machine

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _facing(self, name, plane, start, final, clearance, safe):
        op = PathMillFacing.Create(name)
        op.Workplane = plane
        self.doc.recompute()
        for prop, val in (
            ("StartDepth", start),
            ("FinalDepth", final),
            ("ClearanceHeight", clearance),
            ("SafeHeight", safe),
        ):
            op.setExpression(prop, None)
            setattr(op, prop, val)
        self.doc.recompute()
        return op

    def test_storedPathIsPlaneRelativeAndPlacementPutsItOnThePart(self):
        """The claim of the change, checked in the world.

        A facing pass at FinalDepth 0 on a plane through the box's top face,
        tilted 45 degrees: every stored cutting point has Z 0, the op's
        Placement is the plane's, and every *placed* cutting point lies on the
        plane in world coordinates."""
        n = _tilted()
        origin = Vector(50, 50, 50)
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, n, origin=origin)
        op = self._facing("F", plane, 2.0, 0.0, 7.0, 5.0)

        stored = _cutPoints(op.Path)
        self.assertTrue(len(stored) > 4, "the facing pass should cut")
        for p in stored:
            self.assertAlmostEqual(p.z, 0.0, places=6, msg="stored path is plane-relative")
        self.assertEqual(self._rotaryWords(op.Path), [], "no rotary words in the op's path")
        self.assertTrue(op.Placement.isSame(plane.Placement, 1e-9))

        placed = _cutPoints(PathUtils.getPathWithPlacement(op))
        for p in placed:
            self.assertAlmostEqual(
                n.dot(p - origin), 0.0, places=5, msg="placed path lies on the plane in the world"
            )

    @staticmethod
    def _rotaryWords(path):
        return [c for c in path.Commands if any(k in c.Parameters for k in "ABC")]

    def test_depthsStayInThePlaneFrameWhenAPropertyChanges(self):
        """execute() computes an operation's depths in its work plane's frame.
        Adding base geometry recomputes them from onChanged(), outside
        execute(), and used to do it in world coordinates: a new Helix that
        kept its inherited plane showed the world stock top as its Start
        Depth, above its Safe Height, and generated nothing."""
        import Path.Op.Helix as PathHelix

        drilled = self.doc.addObject("Part::Feature", "Drilled")
        drilled.Shape = Part.makeBox(100, 100, 50).cut(Part.makeCylinder(4, 30, Vector(50, 50, 20)))
        self.doc.recompute()
        job = PathJob.Create("JobDrilled", [drilled], None)
        self.doc.recompute()
        clone = job.Model.Group[0]
        hole = [
            "Face%d" % i
            for i, f in enumerate(clone.Shape.Faces, 1)
            if isinstance(f.Surface, Part.Cylinder)
        ]
        top = PathWorkplane.createWorkplaneFromToolAxis(
            job, Vector(0, 0, 1), origin=Vector(0, 0, 50)
        )
        op = PathHelix.Create("H", parentJob=job)
        op.Workplane = top
        self.doc.recompute()
        in_plane = job.Stock.Shape.BoundBox.ZMax - 50.0
        self.assertAlmostEqual(op.OpStartDepth.Value, in_plane, places=5)

        op.Base = [(clone, hole)]  # onChanged -> updateDepths, outside execute()
        self.assertAlmostEqual(op.OpStartDepth.Value, in_plane, places=5)

    def test_zUpPlaneOnTheTopFaceMeasuresFromIt(self):
        """A plane with no rotation but an origin still defines the frame: the
        stock top reads as its height above the face, the model top as 0, and
        the placed path sits at the real height."""
        top = PathWorkplane.createWorkplaneFromToolAxis(
            self.job, Vector(0, 0, 1), origin=Vector(0, 0, 50)
        )
        op = PathMillFacing.Create("F")
        op.Workplane = top
        self.doc.recompute()
        stock_top = self.job.Stock.Shape.BoundBox.ZMax
        self.assertAlmostEqual(op.OpStockZMax.Value, stock_top - 50.0, places=5)
        self.assertAlmostEqual(op.OpFinalDepth.Value, 0.0, places=6)

        for prop in ("StartDepth", "FinalDepth", "ClearanceHeight", "SafeHeight"):
            op.setExpression(prop, None)
        op.StartDepth, op.FinalDepth, op.ClearanceHeight, op.SafeHeight = 3.0, 0.0, 8.0, 6.0
        self.doc.recompute()
        self.assertAlmostEqual(min(p.z for p in _cutPoints(op.Path)), 0.0, places=6)
        self.assertAlmostEqual(
            min(p.z for p in _cutPoints(PathUtils.getPathWithPlacement(op))), 50.0, places=6
        )

    def test_depthEnteredRelativeToThePlaneFollowsThePlane(self):
        """3 below the plane stays 3 below the plane when the plane moves."""
        n = _tilted()
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, n, origin=n * 20.0)
        op = self._facing("F", plane, 2.0, -3.0, 7.0, 5.0)
        stored_before = min(p.z for p in _cutPoints(op.Path))
        world_before = min(n.dot(p) for p in _cutPoints(PathUtils.getPathWithPlacement(op)))

        plane.Placement = FreeCAD.Placement(n * 25.0, plane.Placement.Rotation)
        self.doc.recompute()
        self.assertAlmostEqual(op.FinalDepth.Value, -3.0, msg="the entered depth is untouched")
        self.assertAlmostEqual(min(p.z for p in _cutPoints(op.Path)), stored_before, places=6)
        world_after = min(n.dot(p) for p in _cutPoints(PathUtils.getPathWithPlacement(op)))
        self.assertAlmostEqual(
            world_after - world_before, 5.0, places=5, msg="the cut moved with the plane"
        )

    def test_theOperationRecordsNothingAboutTheMachine(self):
        """The rotary positions are the post's to solve from the Placement;
        the operation carries its plane and nothing else."""
        n = _tilted()
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, n)
        op = self._facing("F", plane, 2.0, 0.0, 7.0, 5.0)
        self.assertFalse(hasattr(op, "RotaryPositions"))
        z = op.Placement.Rotation.multVec(Vector(0, 0, 1))
        self.assertTrue(z.isEqual(n, 1e-6))

    def test_placingAPathOnATurnedOverPlaneKeepsEveryArcInPlace(self):
        """A G2/G3 is an arc as seen from +Z. Turning the path over reverses
        that sense, so the placed command swaps its direction word; the
        placed arc then lies where the frame puts the stored one. Checked at
        the midpoint, which is where a wrong direction shows."""
        turned_over = FreeCAD.Placement(Vector(5, 7, 0), FreeCAD.Rotation(Vector(1, 0, 0), 180))
        stored = Path.Path(
            [
                Path.Command("G0", {"X": 0, "Y": 0, "Z": 1}),
                Path.Command("G1", {"X": 10, "Y": 0, "Z": -1}),
                Path.Command("G3", {"X": 10, "Y": 10, "Z": -1, "I": 0, "J": 5}),
                Path.Command("G2", {"X": 0, "Y": 10, "Z": -1, "I": -5, "J": 0}),
            ]
        )
        placed = PathUtils.applyPlacementToPath(turned_over, stored)
        self.assertEqual([c.Name for c in placed.Commands], ["G0", "G1", "G2", "G3"])
        pos_s, pos_p = Vector(0, 0, 0), Vector(0, 0, 0)
        for c, pc in zip(stored.Commands, placed.Commands):
            if c.Name in ("G2", "G3"):
                e_s = Path.Geom.edgeForCmd(c, pos_s)
                e_p = Path.Geom.edgeForCmd(pc, pos_p)
                mid_s = turned_over.multVec(
                    e_s.valueAt((e_s.FirstParameter + e_s.LastParameter) / 2)
                )
                mid_p = e_p.valueAt((e_p.FirstParameter + e_p.LastParameter) / 2)
                self.assertTrue(mid_s.isEqual(mid_p, 1e-6), "%s != %s" % (mid_s, mid_p))
            pos_s = Vector(c.x, c.y, c.z)
            pos_p = Vector(pc.x, pc.y, pc.z)

    def test_placingACannedCycleMovesItsRetractPlaneWithIt(self):
        """R is a height like Z: a drill cycle on a plane 50 above the Job's
        zero must retract 50 higher too, or it rapids into the part."""
        raised = FreeCAD.Placement(Vector(0, 0, 50), FreeCAD.Rotation())
        stored = Path.Path([Path.Command("G81", {"X": 10, "Y": 5, "Z": -20, "R": 5})])
        placed = PathUtils.applyPlacementToPath(raised, stored).Commands[0]
        self.assertRoughly(placed.Parameters["Z"], 30)
        self.assertRoughly(placed.Parameters["R"], 55)

    def test_modelTopInATiltedFrameComesFromTheShapeNotItsBoundingBox(self):
        """A facing operation's final depth is the top of the model along the
        tool axis. A 30 degree facet cut off the end of a block leaves the
        block's old corner in its bounding box, 15 above the facet in the
        plane's frame; the model has nothing there, so the top is the facet."""
        drop = 30 * math.tan(math.radians(30))
        wedge = Part.Face(
            Part.makePolygon(
                [
                    Vector(90, 0, 50),
                    Vector(120, 0, 50),
                    Vector(120, 0, 50 - drop),
                    Vector(90, 0, 50),
                ]
            )
        ).extrude(Vector(0, 80, 0))
        model = self.doc.addObject("Part::Feature", "Faceted")
        model.Shape = Part.makeBox(120, 80, 50).cut(wedge)
        self.doc.recompute()
        job = PathJob.Create("JobFaceted", [model], None)
        self.doc.recompute()
        plane = PathWorkplane.createWorkplane(
            job,
            placement=FreeCAD.Placement(
                Vector(105, 40, 50 - drop / 2), FreeCAD.Rotation(Vector(0, 1, 0), 30)
            ),
        )
        op = PathMillFacing.Create("FacetFacing", parentJob=job)
        op.Workplane = plane
        self.doc.recompute()
        self.assertRoughly(op.OpFinalDepth.Value, 0.0, 1e-6)

    def test_startPointIsTakenInTheWorld(self):
        """A start point is picked in the 3D view, in world coordinates. On a
        plane whose origin is off the Job's, the operation must carry it into
        its frame, or the profile starts where the untransformed point lands:
        here 50 mm away, on the other side of the part."""
        plane = PathWorkplane.createWorkplaneFromToolAxis(
            self.job, Vector(0, 0, 1), origin=Vector(50, 50, 50)
        )
        op = PathProfile.Create("P")
        op.Workplane = plane
        op.UseStartPoint = True
        op.StartPoint = Vector(100, 0, 50)
        self.doc.recompute()
        first = _cutPoints(PathUtils.getPathWithPlacement(op))[0]
        self.assertLess((Vector(first.x, first.y, 0) - Vector(100, 0, 0)).Length, 10.0)

    def test_dressupWorksInTheBaseOpsFrameAndCarriesItsPlacement(self):
        """A dressup reads its base op's stored path, in the op's plane frame,
        and stores its own path there: the op's heights, measured from the
        plane, are the dressup's heights too. Its Placement is the op's, so
        placed, the retracts land 50 above the Job's zero as the op's do."""
        plane = PathWorkplane.createWorkplaneFromToolAxis(
            self.job, Vector(0, 0, 1), origin=Vector(50, 50, 50)
        )
        op = PathProfile.Create("P")
        op.Workplane = plane
        self.doc.recompute()
        dressup = _leadInOut(self.doc, self.job, op)

        def rapids(path):
            return [
                c.Parameters["Z"] for c in path.Commands if c.Name == "G0" and "Z" in c.Parameters
            ]

        self.assertTrue(rapids(dressup.Path), "the dressup should retract")
        self.assertRoughly(max(rapids(dressup.Path)), op.ClearanceHeight.Value, 1e-6)
        self.assertTrue(dressup.Placement.isSame(op.Placement, 1e-9))
        placed = PathUtils.getPathWithPlacement(dressup)
        self.assertRoughly(max(rapids(placed)), op.ClearanceHeight.Value + 50.0, 1e-6)

    def test_dressupOnATiltedPlaneKeepsItsArcsInThePlane(self):
        """In the plane frame every arc is a flat XY arc, which is the only
        arc G2/G3 can say. A dressup that read the placed, tilted path saw
        arcs standing out of XY and rewrote them as chords."""
        plane = PathWorkplane.createWorkplane(
            self.job,
            placement=FreeCAD.Placement(Vector(50, 50, 40), FreeCAD.Rotation(Vector(0, 1, 0), 30)),
        )
        op = PathProfile.Create("P")
        op.Workplane = plane
        self.doc.recompute()
        dressup = _leadInOut(self.doc, self.job, op)
        self.assertTrue(dressup.Placement.isSame(op.Placement, 1e-9))
        self.assertFalse(dressup.Placement.isIdentity(1e-9))

        z = None
        arcs = 0
        for c in dressup.Path.Commands:
            if c.Name in ("G2", "G3", "G02", "G03"):
                arcs += 1
                self.assertRoughly(c.Parameters.get("K", 0.0), 0.0, 1e-6)
                if z is not None and "Z" in c.Parameters:
                    self.assertRoughly(c.Parameters["Z"], z, 1e-6)
            if "Z" in c.Parameters:
                z = c.Parameters["Z"]
        self.assertGreater(arcs, 0, "a lead-in is an arc")

    def test_legacyPostPlacesAnOperationOnce(self):
        """WrapperPost places each operation's path for legacy scripts, and
        most of those scripts call PathUtils.getPathWithPlacement() on the
        item as well. The item forwards Placement to its operation, so the
        plane's offset was applied twice."""
        from Path.Post.PostList import Postable
        from Path.Post.Processor import WrapperPost

        class Op:
            Placement = FreeCAD.Placement(Vector(0, 0, 50), FreeCAD.Rotation())

        item = Postable(
            item_type="operation",
            label="Op",
            path=Path.Path([Path.Command("G1", {"X": 1, "Y": 2, "Z": -5})]),
            source=Op(),
        )
        WrapperPost._place_operations([("all", [item])])
        self.assertRoughly(PathUtils.getPathWithPlacement(item).Commands[0].z, 45.0)

    def test_engraveBaseShapesAreReadInTheOperationsFrame(self):
        """Engrave takes whole shapes beside its Base. A line lying on a
        tilted plane is read in that plane's frame, as Base geometry is: its
        top is at depth 0, and the placed path runs along the line in the
        world. Read in world coordinates it engraved 50 mm below the table,
        90 mm off to the side."""
        plane = FreeCAD.Placement(Vector(60, 40, 50), FreeCAD.Rotation(Vector(0, 1, 0), 30))
        a, b = plane.multVec(Vector(-10, -20, 0)), plane.multVec(Vector(10, 20, 0))
        line = self.doc.addObject("Part::Feature", "Line")
        line.Shape = Part.makeLine(a, b)
        op = PathEngrave.Create("Engrave")
        op.Workplane = PathWorkplane.createWorkplane(self.job, placement=plane)
        op.BaseShapes = [line]
        self.doc.recompute()

        self.assertRoughly(op.OpFinalDepth.Value, 0.0, 1e-6)
        stored = _cutPoints(op.Path)
        self.assertTrue(stored)
        self.assertRoughly(min(p.z for p in stored), 0.0, 1e-6)
        for p in stored:
            self.assertTrue(-10 - 1e-6 <= p.x <= 10 + 1e-6, p)
            self.assertTrue(-20 - 1e-6 <= p.y <= 20 + 1e-6, p)
        world = Part.makeLine(a, b)
        for p in _cutPoints(PathUtils.getPathWithPlacement(op)):
            if Path.Geom.isRoughly(plane.inverse().multVec(p).z, 0.0):
                self.assertRoughly(world.distToShape(Part.Vertex(p))[0], 0.0, 1e-6)

    def test_placingAPathOnAnUprightPlaneKeepsTheArcWords(self):
        upright = FreeCAD.Placement(Vector(5, 7, 3), FreeCAD.Rotation(Vector(0, 0, 1), 90))
        stored = Path.Path([Path.Command("G3", {"X": 10, "Y": 10, "I": 0, "J": 5})])
        self.assertEqual(PathUtils.applyPlacementToPath(upright, stored).Commands[0].Name, "G3")

    def test_anUnreachablePlaneStillGeneratesItsPath(self):
        """A plane the machine cannot index to is the post's problem. The
        operation generates in the plane's frame regardless, so the same
        document posts on another machine, or by refixturing, unchanged."""
        n = Vector(0, 0, -1)  # the underside: A = 180 on a +-120 trunnion
        self.assertFalse(rotation.solve_orientation(self.machine, n).success)
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, n)
        op = self._facing("Under", plane, 2.0, 0.0, 7.0, 5.0)
        self.assertTrue(_cutPoints(op.Path), "a path was generated")
        self.assertNotIn("unavailable", op.Path.toGCode())

    def test_planeFromAFaceHasItsOriginOnTheFace(self):
        model = self.job.Model.Group[0]
        top = None
        for i, f in enumerate(model.Shape.Faces):
            if (f.normalAt(0, 0) - Vector(0, 0, 1)).Length < 1e-6:
                top = "Face%d" % (i + 1)
        plane = PathWorkplane.createWorkplane(self.job, model, top)
        self.assertTrue(
            plane.Placement.Base.isEqual(Vector(50, 50, 50), 1e-6),
            "origin should be the face centroid, got %s" % plane.Placement.Base,
        )

    def test_depthOfFeatureMeasuresFromTheOrigin(self):
        face = Part.makePlane(10, 10)
        face.Placement = FreeCAD.Placement(Vector(0, 0, 7), FreeCAD.Rotation())
        self.assertAlmostEqual(PathUtil.depthOfFeature(face, Vector(0, 0, 1)), 7.0, places=6)
        self.assertAlmostEqual(
            PathUtil.depthOfFeature(face, Vector(0, 0, 1), Vector(0, 0, 5)), 2.0, places=6
        )

    def test_parallelPlanesAtDifferentDepthsShareClearedArea(self):
        """Rest machining carries the query into the other operation's frame."""
        n = _tilted()
        lower = PathWorkplane.createWorkplaneFromToolAxis(self.job, n, origin=n * 20.0)
        upper = PathWorkplane.createWorkplaneFromToolAxis(self.job, n, origin=n * 25.0)

        def custom(name, plane, path):
            op = PathCustom.Create(name, parentJob=self.job)
            op.Workplane = plane
            op.ToolController.Tool.Diameter = 5.0
            op.Path = path
            return op

        rect = Path.Path(
            [
                Path.Command("G0", {"X": -20, "Y": -20, "Z": 5}),
                Path.Command("G1", {"Z": -1}),
                Path.Command("G1", {"X": 20}),
                Path.Command("G1", {"Y": 20}),
                Path.Command("G1", {"X": -20}),
                Path.Command("G1", {"Y": -20}),
            ]
        )
        custom("Prev", lower, rect)
        cur = custom("Cur", upper, rect)
        bb = FreeCAD.BoundBox()
        bb.add(Vector(-50, -50, -10))
        bb.add(Vector(50, 50, 10))
        self.assertEqual(len(PathOpUtil.getClearedAreas(cur, bb)), 1)


class TestPostWorkplaneFrames(PathTestUtils.PathTestBase):
    """The post turns a plane-frame path into what a DWO machine runs."""

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathWorkplaneFramePost")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()
        self.machine = _machineCA()
        self.job.Proxy.getMachine = lambda: self.machine

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _processor(self):
        from Path.Post.Processor import PostProcessor

        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = self.machine
        processor._merge_machine_config()
        return processor

    def _item(self, op):
        from Path.Post.PostList import Postable

        return Postable(
            item_type="operation",
            label=op.Label,
            path=Path.Path(op.Path.Commands),
            source=op,
        )

    def test_threeAxisOperationIsUntouched(self):
        op = PathCustom.Create("Plain", parentJob=self.job)
        op.Path = Path.Path([Path.Command("G0", {"X": 1, "Y": 2, "Z": 3})])
        processor = self._processor()
        processor._machine = None
        out = processor._expand_workplane_frames([("Job", [self._item(op)])])
        items = out[0][1]
        self.assertEqual(len(items), 1)
        self.assertEqual(items[0].path.Commands[0].toGCode(), op.Path.Commands[0].toGCode())

    def test_dressedTiltedOperationIsPosedLikeItsBase(self):
        """The post reads the pose from the item it is handed. A dressup
        carries its base op's Placement, so a dressed tilted op gets the
        rotary move its base would; it used to post as A0 C0 with the tilted
        path written out as three-axis moves."""
        from Machine.models.machine import RotationStrategy

        self.machine.kinematics.rotation_strategy = RotationStrategy.DWO
        plane = PathWorkplane.createWorkplane(
            self.job,
            placement=FreeCAD.Placement(Vector(50, 50, 40), FreeCAD.Rotation(Vector(0, 1, 0), 30)),
        )
        op = PathProfile.Create("P", parentJob=self.job)
        op.Workplane = plane
        self.doc.recompute()
        dressup = _leadInOut(self.doc, self.job, op)

        out = self._processor()._expand_workplane_frames([("Job", [self._item(dressup)])])
        self.assertEqual([i.item_type for i in out[0][1]], ["rotation", "operation"])
        rotary = out[0][1][0].path.Commands[0].Parameters
        self.assertFalse(all(Path.Geom.isRoughly(v, 0.0) for v in rotary.values()))

    def test_planeOperationGetsRotaryMoveAndMachineFrame(self):
        """Positions first, then the path in the frame the machine reaches.

        The expected coordinates are the world position of each point (the
        placed path) rotated by the machine's rotation for the recorded
        angles - which is what a DWO control expects after that rotary move."""
        from Machine.models.machine import RotationStrategy

        self.machine.kinematics.rotation_strategy = RotationStrategy.DWO
        n = _tilted()
        origin = Vector(30, 10, 5)
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, n, origin=origin)
        op = PathCustom.Create("Tilted", parentJob=self.job)
        op.Workplane = plane
        self.doc.recompute()
        # Assigned after the recompute: Custom rebuilds its path from Gcode on
        # execute, which would discard a path assigned before it.
        op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
                Path.Command("G1", {"X": 10, "Y": 0, "Z": -2}),
                Path.Command("G1", {"X": 10, "Y": 10, "Z": -2}),
            ]
        )

        out = self._processor()._expand_workplane_frames([("Job", [self._item(op)])])
        items = out[0][1]
        self.assertEqual([i.item_type for i in items], ["rotation", "operation"])

        positions = dict(rotation.solve_orientation(self.machine, n).angles)
        rotary = items[0].path.Commands[0]
        self.assertEqual(rotary.Name, "G0")
        for axis, angle in positions.items():
            self.assertAlmostEqual(rotary.Parameters[axis], angle, places=6)

        chain = rotation.build_kinematic_chain(self.machine)
        R = rotation.compute_rotation_matrix(chain, positions)
        world = _cutPoints(PathUtils.getPathWithPlacement(op))
        expected = [R.multVec(p) for p in world]
        got = _cutPoints(items[1].path)
        self.assertEqual(len(got), len(expected))
        for g, e in zip(got, expected):
            self.assertTrue(g.isEqual(e, 1e-5), "%s != %s" % (g, e))
        # and the machine frame is Z-up: a cut at plane depth -2 is level in Z
        self.assertAlmostEqual(got[0].z, got[1].z, places=6)
