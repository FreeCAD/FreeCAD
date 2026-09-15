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

"""The post's tilted-work-plane output.

An operation on a work plane stores its path plane-relative. The machine's
rotation strategy decides what the post makes of it: DWO commands the
rotaries and rotates the path into the frame the machine reaches; TWP
declares the plane in the post's plane command and leaves the path alone.
These tests pin the emitted lines for each dialect, the sequencing between
operations, tool changes and the return to the table-parallel pose, the
pre- and post-rotary blocks around every rotary move, the refusals, and the
sanity warning for a machine whose block is empty.
"""

import random
import unittest

import FreeCAD
import Path
import Path.Main.Job as PathJob
import Path.Main.Workplane as PathWorkplane
import Path.Op.Custom as PathCustom
import Path.Base.Generator.rotation as rotation
import CAMTests.PathTestUtils as PathTestUtils
import PathScripts.PathUtils as PathUtils
from Path.Post import TiltedWorkPlane
from Path.Post.TiltedWorkPlane import PlaneCommand
from Path.Post.CAMErrors import CAMValueError
from Path.Post.PostList import Postable
from Machine.models.machine import Machine, RotaryAxis, AxisRole, RotationStrategy

from FreeCAD import Rotation, Vector

X = Vector(1, 0, 0)
Y = Vector(0, 1, 0)
Z = Vector(0, 0, 1)


class TestPlaneAngles(unittest.TestCase):
    """The angle conventions, checked against rotations composed by hand."""

    def test_eulerZXZ_readsBackAComposedIntrinsicRotation(self):
        # G68.2: about Z by I, then the new X by J, then the newest Z by K
        r = Rotation(Z, 30).multiply(Rotation(X, 45)).multiply(Rotation(Z, 10))
        i, j, k = TiltedWorkPlane.euler_zxz(r)
        self.assertAlmostEqual(i, 30, places=6)
        self.assertAlmostEqual(j, 45, places=6)
        self.assertAlmostEqual(k, 10, places=6)

    def test_spatialABC_readsBackAComposedFixedAxisRotation(self):
        # PLANE SPATIAL: about fixed X by A, then fixed Y by B, then fixed Z by C
        r = Rotation(Z, 30).multiply(Rotation(Y, 20)).multiply(Rotation(X, 45))
        a, b, c = TiltedWorkPlane.spatial_abc(r)
        self.assertAlmostEqual(a, 45, places=6)
        self.assertAlmostEqual(b, 20, places=6)
        self.assertAlmostEqual(c, 30, places=6)

    def test_aTiltAboutXIsTheMiddleAngleAlone(self):
        self.assertEqual(TiltedWorkPlane.euler_zxz(Rotation(X, 45)), (0.0, 45.0, 0.0))
        self.assertEqual(TiltedWorkPlane.spatial_abc(Rotation(X, 45)), (45.0, 0.0, 0.0))

    def test_tableParallelPlaneIsAllZeros(self):
        self.assertEqual(TiltedWorkPlane.euler_zxz(Rotation()), (0.0, 0.0, 0.0))
        self.assertEqual(TiltedWorkPlane.spatial_abc(Rotation()), (0.0, 0.0, 0.0))

    def test_inPlaneRotationOnlyIsASingleZAngle(self):
        # The singular case: a plane parallel to the table with a turned X.
        # Only the sum of the first and last angle is defined.
        i, j, k = TiltedWorkPlane.euler_zxz(Rotation(Z, 90))
        self.assertAlmostEqual(j, 0, places=6)
        self.assertAlmostEqual((i + k) % 360, 90, places=6)

    def test_everyDecompositionRebuildsItsRotation(self):
        rng = random.Random(7)
        for _ in range(200):
            axis = Vector(rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))
            if axis.Length < 1e-6:
                continue
            r = Rotation(axis, rng.uniform(-180, 180))
            zxz = Rotation()
            zxz.setEulerAngles("IZXZ", *TiltedWorkPlane.euler_zxz(r))
            self.assertTrue(zxz.isSame(r, 1e-9), "ZXZ rebuild differs for %s" % r)
            a, b, c = TiltedWorkPlane.spatial_abc(r)
            zyx = Rotation()
            zyx.setEulerAngles("IZYX", c, b, a)
            self.assertTrue(zyx.isSame(r, 1e-9), "spatial rebuild differs for %s" % r)


def _machineCA(strategy):
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
    m.kinematics.rotation_strategy = strategy
    return m


def _tiltedAboutX(degrees=45):
    """The tool axis of a plane tilted about X: Rotation(X, degrees) applied to Z."""
    return Rotation(X, degrees).multVec(Z)


PATH = [
    Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
    Path.Command("G1", {"X": 10, "Y": 0, "Z": -2}),
    Path.Command("G2", {"X": 20, "Y": 10, "Z": -2, "I": 10, "J": 0}),
]

DECLARE = "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000"
CLEAR = "G53 G0 Z0\nG53 G0 X-300 Y0"


def _gcode(path):
    return [c.toGCode() for c in path.Commands]


class TestTiltedWorkPlanePost(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathTiltedWorkPlane")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()
        self.machine = _machineCA(RotationStrategy.TWP)
        self.job.Proxy.getMachine = lambda: self.machine

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    # helpers

    def _processor(self, machine=None, plane_command=None, pre=None, post=None, **values):
        from Path.Post.Processor import PostProcessor

        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = self.machine if machine is None else machine
        if processor._machine is not None:
            processor._merge_machine_config()
        if plane_command is not None:
            processor.PLANE_COMMAND = plane_command
        if pre is not None:
            processor.values["PRE_ROTARY_MOVE"] = pre
        if post is not None:
            processor.values["POST_ROTARY_MOVE"] = post
        processor.values.update(values)
        return processor

    def _plane(self, axis=None, origin=Vector(30, 10, 5), x=None):
        axis = _tiltedAboutX() if axis is None else axis
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, axis, origin=origin)
        if x is not None:
            placement = plane.Placement
            normal = placement.Rotation.multVec(Z)
            placement.Rotation = Rotation(x, normal.cross(x), normal)
            plane.Placement = placement
        return plane

    def _op(self, label, plane=None):
        """A Custom op on the plane, with a fixed plane-relative path."""
        op = PathCustom.Create(label, parentJob=self.job)
        op.Workplane = plane
        self.doc.recompute()  # records the rotary positions
        # Custom rebuilds its path from Gcode on execute; assign after.
        op.Path = Path.Path(list(PATH))
        return op

    @staticmethod
    def _item(op):
        return Postable(
            item_type="operation", label=op.Label, path=Path.Path(op.Path.Commands), source=op
        )

    @staticmethod
    def _tool_change():
        return Postable(
            item_type="tool_controller",
            label="TC",
            path=Path.Path([Path.Command("M6", {"T": 1})]),
            source=None,
            data={"tool_number": 1},
        )

    def _expand(self, items, processor=None):
        processor = processor or self._processor()
        return processor._expand_workplane_frames([("Job", items)])[0][1]

    @staticmethod
    def _shape(items):
        """A readable trace: the text of str items, the type of the rest."""
        out = []
        for item in items:
            if item.item_type == "str":
                out.append(item.data["str"])
            elif item.item_type == "rotation":
                out.append("rotation:" + item.path.Commands[0].toGCode())
            else:
                out.append(item.item_type)
        return out

    # the dialects

    def test_fanucDeclaresThePlaneAndLeavesThePathInPlaneCoordinates(self):
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)])
        self.assertEqual(self._shape(items), [DECLARE, "G53.1", "operation", "G69"])
        self.assertEqual(_gcode(items[2].path), _gcode(op.Path), "path emitted as stored")

    def test_haasDialect(self):
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)], self._processor(plane_command=PlaneCommand.G268))
        self.assertEqual(
            self._shape(items),
            ["G268 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000", "G53.1", "operation", "G269"],
        )

    def test_heidenhainDialect(self):
        op = self._op("Tilted", self._plane())
        items = self._expand(
            [self._item(op)], self._processor(plane_command=PlaneCommand.PLANE_SPATIAL)
        )
        self.assertEqual(
            self._shape(items),
            [
                "PLANE SPATIAL SPA45.000 SPB0.000 SPC0.000 TURN FMAX",
                "operation",
                "PLANE RESET STAY",
            ],
        )

    def test_theOriginFollowsTheOutputUnits(self):
        from Machine.models.machine import OutputUnits

        self.machine.output.units = OutputUnits.IMPERIAL
        op = self._op("Tilted", self._plane(origin=Vector(25.4, 50.8, 0)))
        processor = self._processor(AXIS_PRECISION=4)
        shape = self._shape(self._expand([self._item(op)], processor))
        self.assertEqual(shape[0], "G68.2 X1.0000 Y2.0000 Z0.0000 I0.0000 J45.0000 K0.0000")

    def test_thePostPropertiesReplaceAnyLine(self):
        op = self._op("Tilted", self._plane())
        processor = self._processor(
            TWP_DECLARE="G68.2 P0 X{x} Y{y} Z{z} I{a1} J{a2} K{a3}",
            TWP_ALIGN="",  # empty: the dialect's own line
            TWP_CANCEL="G69 (plane off)",
        )
        self.assertEqual(
            self._shape(self._expand([self._item(op)], processor)),
            [
                "G68.2 P0 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G69 (plane off)",
            ],
        )

    def test_theProgramPositionsTheRotariesWhenTheControlDoesNot(self):
        op = self._op("Tilted", self._plane())
        items = self._expand(
            [self._item(op)], self._processor(TWP_CONTROL_POSITIONS_ROTARIES=False)
        )
        shape = self._shape(items)
        self.assertTrue(shape[0].startswith("rotation:G0"), shape)
        self.assertEqual(shape[1:], [DECLARE, "operation", "G69"])
        positions = {k: float(v) for k, v in dict(op.RotaryPositions).items()}
        rotary = items[0].path.Commands[0]
        for axis, angle in positions.items():
            self.assertAlmostEqual(rotary.Parameters[axis], angle, places=6)

    # the rotary blocks

    def test_theBlocksWrapTheWholePoseChange(self):
        op = self._op("Tilted", self._plane())
        shape = self._shape(self._expand([self._item(op)], self._processor(pre=CLEAR, post="M11")))
        self.assertEqual(shape, [CLEAR, DECLARE, "G53.1", "M11", "operation", "G69"])

    def test_theBlocksWrapAProgramCommandedRotaryMoveOnce(self):
        """_expand_rotary_move wraps rotary words it finds; the move emitted
        here is already wrapped and must not be wrapped again."""
        op = self._op("Tilted", self._plane())
        processor = self._processor(pre=CLEAR, post="M11", TWP_CONTROL_POSITIONS_ROTARIES=False)
        postables = processor._expand_workplane_frames([("Job", [self._item(op)])])
        processor._expand_rotary_move(postables)
        shape = self._shape(postables[0][1])
        self.assertEqual(shape.count(CLEAR), 1, shape)
        self.assertEqual(shape.count("M11"), 1, shape)
        self.assertEqual(shape[0], CLEAR)
        self.assertTrue(shape[1].startswith("rotation:G0"), shape)
        self.assertEqual(shape[2:], [DECLARE, "M11", "operation", "G69"])

    def test_rotaryWordsInsideAPathAreStillWrapped(self):
        """A wrapped fourth-axis path carries A on its own moves; those keep
        the blocks _expand_rotary_move always gave them."""
        op = PathCustom.Create("Wrapped", parentJob=self.job)
        self.doc.recompute()
        op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
                Path.Command("G1", {"X": 10, "A": 90}),
                Path.Command("G1", {"X": 20, "A": 180}),
            ]
        )
        op.RotaryPositions = {}
        processor = self._processor(machine=Machine(name="3 axis"), pre="M10", post="M11")
        postables = processor._expand_workplane_frames([("Job", [self._item(op)])])
        processor._expand_rotary_move(postables)
        kinds = [i.item_type for i in postables[0][1]]
        texts = [i.data.get("str") for i in postables[0][1] if i.item_type == "str"]
        self.assertEqual(texts, ["M10", "M11"])
        self.assertEqual(kinds, ["command", "str", "command", "str"])

    # sequencing

    def test_operationsOnTheSamePlaneShareOneDeclaration(self):
        plane = self._plane()
        a, b = self._op("A", plane), self._op("B", plane)
        self.assertEqual(
            self._shape(self._expand([self._item(a), self._item(b)])),
            [DECLARE, "G53.1", "operation", "operation", "G69"],
        )

    def test_aToolChangeCancelsThePlaneAndItIsDeclaredAgainAfter(self):
        plane = self._plane()
        a, b = self._op("A", plane), self._op("B", plane)
        shape = self._shape(
            self._expand(
                [self._item(a), self._tool_change(), self._item(b)], self._processor(pre=CLEAR)
            )
        )
        self.assertEqual(
            shape,
            [
                CLEAR,
                DECLARE,
                "G53.1",
                "operation",
                "G69",
                "tool_controller",
                CLEAR,
                DECLARE,
                "G53.1",
                "operation",
                "G69",
            ],
        )

    def test_returningToTheTableCancelsThePlaneAndCommandsTheRotariesHome(self):
        tilted = self._op("Tilted", self._plane())
        plain = self._op("Plain", None)
        self.assertTrue(dict(plain.RotaryPositions), "a rotary machine records zeros too")
        items = self._expand([self._item(tilted), self._item(plain)], self._processor(pre=CLEAR))
        shape = self._shape(items)
        self.assertEqual(shape[:5], [CLEAR, DECLARE, "G53.1", "operation", CLEAR])
        self.assertEqual(shape[5], "G69")
        self.assertTrue(shape[6].startswith("rotation:G0"), shape)
        self.assertEqual(shape[7:], ["operation"])
        for angle in items[6].path.Commands[0].Parameters.values():
            self.assertAlmostEqual(angle, 0.0, places=6)

    def test_aTurnedXOnTheSameTiltRedeclaresWithoutMovingTheRotaries(self):
        axis = _tiltedAboutX()
        first = self._plane(axis)
        turned = self._plane(axis, x=Vector(-1, 0, 0))
        a, b = self._op("A", first), self._op("B", turned)
        self.assertEqual(
            dict(a.RotaryPositions), dict(b.RotaryPositions), "same tool axis, same angles"
        )
        shape = self._shape(
            self._expand([self._item(a), self._item(b)], self._processor(pre=CLEAR))
        )
        self.assertEqual(shape[:4], [CLEAR, DECLARE, "G53.1", "operation"])
        self.assertEqual(shape[4], "G69", "the first plane is cancelled")
        self.assertTrue(shape[5].startswith("G68.2"), "the turned plane is declared")
        self.assertEqual(shape[6:], ["G53.1", "operation", "G69"])
        self.assertNotIn(CLEAR, shape[4:], "no rotary move, so no block")

    # the other strategies

    def test_dwoCommandsTheRotariesAndRotatesThePath(self):
        self.machine = _machineCA(RotationStrategy.DWO)
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)], self._processor(pre=CLEAR, post="M11"))
        shape = self._shape(items)
        self.assertEqual(shape[0], CLEAR)
        self.assertTrue(shape[1].startswith("rotation:G0"), shape)
        self.assertEqual(shape[2:], ["M11", "operation"])
        positions = {k: float(v) for k, v in dict(op.RotaryPositions).items()}
        chain = rotation.build_kinematic_chain(self.machine)
        R = rotation.compute_rotation_matrix(chain, positions)
        world = PathUtils.getPathWithPlacement(op)
        expected = PathUtils.applyPlacementToPath(FreeCAD.Placement(Vector(), R), world)
        self.assertEqual(_gcode(items[3].path), _gcode(expected))

    def test_aMachineWithNoStrategyRefusesATiltedOperation(self):
        self.machine = _machineCA(RotationStrategy.NONE)
        op = self._op("Tilted", self._plane())
        with self.assertRaises(CAMValueError) as raised:
            self._expand([self._item(op)])
        self.assertIn("Rotation strategy", str(raised.exception))

    def test_aMachineWithNoStrategyStillPostsAPlainOperation(self):
        self.machine = _machineCA(RotationStrategy.NONE)
        op = self._op("Plain", None)
        shape = self._shape(self._expand([self._item(op)]))
        self.assertTrue(shape[0].startswith("rotation:G0"), shape)
        self.assertEqual(shape[1:], ["operation"])

    def test_postTransformIsRefusedForNow(self):
        self.machine = _machineCA(RotationStrategy.POST_TRANSFORM)
        op = self._op("Tilted", self._plane())
        with self.assertRaises(CAMValueError):
            self._expand([self._item(op)])

    def test_aPostThatCannotEmitTheStrategyRefuses(self):
        op = self._op("Tilted", self._plane())
        processor = self._processor()
        processor.ROTATION_STRATEGIES = ("dwo",)
        with self.assertRaises(CAMValueError) as raised:
            self._expand([self._item(op)], processor)
        self.assertIn("TWP", str(raised.exception))

    def test_withoutARotaryMachineThePathIsPlacedIntoTheWorld(self):
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)], self._processor(machine=Machine(name="3 axis")))
        self.assertEqual(self._shape(items), ["operation"])
        self.assertEqual(_gcode(items[0].path), _gcode(PathUtils.getPathWithPlacement(op)))

    def test_aThreeAxisJobIsUntouched(self):
        op = PathCustom.Create("Plain", parentJob=self.job)
        self.doc.recompute()
        op.Path = Path.Path(list(PATH))
        op.RotaryPositions = {}
        items = self._expand([self._item(op)], self._processor(machine=Machine(name="3 axis")))
        self.assertEqual(self._shape(items), ["operation"])
        self.assertEqual(_gcode(items[0].path), _gcode(op.Path))

    # legacy posts

    def test_aLegacyPostRefusesATiltedOperation(self):
        from Path.Post.Processor import PostProcessorFactory

        self._op("Tilted", self._plane())
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        with self.assertRaises(CAMValueError) as raised:
            post.export()
        self.assertIn("Legacy post-processor", str(raised.exception))

    def test_aLegacyPostStillPostsAPlainJob(self):
        from Path.Post.Processor import PostProcessorFactory

        self._op("Plain", None)
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        sections = post.export()
        self.assertTrue(sections and sections[0][1])

    # end to end

    def test_aRealPostEmitsTheProgram(self):
        """Through export2 with a shipped post: the block, the plane lines and
        the cut in plane coordinates land in the program, with the post's
        properties read from the machine."""
        from Path.Post.Processor import PostProcessorFactory

        self.machine.output.comments.enabled = False
        self.machine.output.output_header = False
        self.machine.postprocessor_properties = {
            "pre_rotary_move": "G53 G0 Z0",
            "twp_control_positions_rotaries": False,
        }
        op = self._op("Tilted", self._plane())
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        post.reinitialize()
        post._machine = self.machine
        sections = post.export2()
        gcode = "\n".join(g for _, g in sections)
        lines = [line.strip() for line in gcode.splitlines()]
        self.assertIn("G53 G0 Z0", lines)
        self.assertIn(DECLARE, lines)
        self.assertIn("G69", lines)
        self.assertTrue(any(line.startswith("G0 ") and "A" in line for line in lines), lines)
        self.assertIn("G1 X10.000 Y0.000 Z-2.000", lines)
        self.assertLess(lines.index("G53 G0 Z0"), lines.index(DECLARE))

    # the sanity warning

    def _sanity_post(self, pre):
        from Path.Post.Processor import PostProcessorFactory

        self.machine.postprocessor_properties = {"pre_rotary_move": pre}
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        post._machine = self.machine
        post.apply_configuration_bundle()
        return post

    def test_sanityWarnsWhenTheRotariesMoveAndTheBlockIsEmpty(self):
        self._op("A", self._plane(_tiltedAboutX(45)))
        self._op("B", self._plane(_tiltedAboutX(-45)))
        squawks = self._sanity_post("").get_sanity_checks(self.job)
        self.assertEqual([s["squawkType"] for s in squawks], ["WARNING"])
        self.assertIn("Pre-Rotary Move", squawks[0]["Note"])

    def test_sanityIsQuietWithABlock(self):
        self._op("A", self._plane(_tiltedAboutX(45)))
        self._op("B", self._plane(_tiltedAboutX(-45)))
        self.assertEqual(self._sanity_post("G53 G0 Z0").get_sanity_checks(self.job), [])

    def test_sanityIsQuietWhenThePoseNeverChanges(self):
        plane = self._plane()
        self._op("A", plane)
        self._op("B", plane)
        self.assertEqual(self._sanity_post("").get_sanity_checks(self.job), [])

    def test_sanityIsQuietOnAThreeAxisMachine(self):
        self._op("A", None)
        self.machine = Machine(name="3 axis")
        self.assertEqual(self._sanity_post("").get_sanity_checks(self.job), [])


class TestMachineRotationStrategy(unittest.TestCase):
    def test_defaultsDeclareNothing(self):
        k = Machine().kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.NONE)
        self.assertFalse(k.dwo_supported)

    def test_roundTrip(self):
        k = Machine.from_dict(_machineCA(RotationStrategy.TWP).to_dict()).kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.TWP)

    def test_anOlderFileWithDwoSupportedSelectsDwo(self):
        data = Machine(name="old").to_dict()
        kin = data["machine"]["kinematics"]
        del kin["rotation_strategy"]
        kin["dwo_supported"] = True
        k = Machine.from_dict(data).kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.DWO)
        self.assertTrue(k.dwo_supported)
        kin["dwo_supported"] = False
        self.assertEqual(
            Machine.from_dict(data).kinematics.rotation_strategy, RotationStrategy.NONE
        )

    def test_dwoSupportedIsWrittenForOlderReaders(self):
        self.assertTrue(
            _machineCA(RotationStrategy.DWO).to_dict()["machine"]["kinematics"]["dwo_supported"]
        )
        self.assertFalse(
            _machineCA(RotationStrategy.TWP).to_dict()["machine"]["kinematics"]["dwo_supported"]
        )
