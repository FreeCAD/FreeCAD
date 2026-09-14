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
declares the plane in the control's own command and leaves the path alone.
These tests pin the emitted lines for each dialect, the sequencing between
operations, tool changes and the return to the table-parallel pose, and the
refusals for a machine that declares nothing this post can emit.
"""

import math
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
from Path.Post.CAMErrors import CAMValueError
from Path.Post.PostList import Postable
from Machine.models.machine import (
    Machine,
    RotaryAxis,
    AxisRole,
    RotationStrategy,
    PlaneCommand,
)

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


def _machineCA(strategy, plane_command=PlaneCommand.G68_2, control_positions=True, retract=0.0):
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
    m.kinematics.plane_command = plane_command
    m.kinematics.control_positions_rotaries = control_positions
    m.kinematics.index_retract_z = retract
    return m


def _tiltedAboutX(degrees=45):
    """The tool axis of a plane tilted about X: Rotation(X, degrees) applied to Z."""
    a = math.radians(degrees)
    return Vector(0, -math.sin(a), math.cos(a))


PATH = [
    Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
    Path.Command("G1", {"X": 10, "Y": 0, "Z": -2}),
    Path.Command("G2", {"X": 20, "Y": 10, "Z": -2, "I": 10, "J": 0}),
]


def _gcode(path):
    return [c.toGCode() for c in path.Commands]


def _text(item):
    """The line(s) of a str postable."""
    return item.data["str"]


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

    def _processor(self, machine=None):
        from Path.Post.Processor import PostProcessor

        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = self.machine if machine is None else machine
        if processor._machine is not None:
            processor._merge_machine_config()
        return processor

    def _plane(self, axis=None, origin=Vector(30, 10, 5), x=None):
        axis = _tiltedAboutX() if axis is None else axis
        plane = PathWorkplane.createWorkplaneFromToolAxis(self.job, axis, origin=origin)
        if x is not None:
            placement = plane.Placement
            placement.Rotation = Rotation(
                x, placement.Rotation.multVec(Z).cross(x), placement.Rotation.multVec(Z)
            )
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
                out.append(_text(item))
            elif item.item_type == "rotation":
                out.append("rotation:" + item.path.Commands[0].toGCode())
            else:
                out.append(item.item_type)
        return out

    # the dialects

    def test_fanucDeclaresThePlaneAndLeavesThePathInPlaneCoordinates(self):
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)])
        self.assertEqual(
            self._shape(items),
            [
                "G53 G0 Z0.000",
                "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G69",
            ],
        )
        self.assertEqual(_gcode(items[3].path), _gcode(op.Path), "path emitted as stored")

    def test_haasDialect(self):
        self.machine = _machineCA(RotationStrategy.TWP, PlaneCommand.G268)
        op = self._op("Tilted", self._plane())
        self.assertEqual(
            self._shape(self._expand([self._item(op)])),
            [
                "G53 G0 Z0.000",
                "G268 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G269",
            ],
        )

    def test_heidenhainDialect(self):
        self.machine = _machineCA(RotationStrategy.TWP, PlaneCommand.PLANE_SPATIAL, retract=-1.5)
        op = self._op("Tilted", self._plane())
        self.assertEqual(
            self._shape(self._expand([self._item(op)])),
            [
                "L Z-1.500 R0 FMAX M91",
                "PLANE SPATIAL SPA45.000 SPB0.000 SPC0.000 TURN FMAX",
                "operation",
                "PLANE RESET STAY",
            ],
        )

    def test_retractHeightAndOriginFollowTheOutputUnits(self):
        from Machine.models.machine import OutputUnits

        self.machine = _machineCA(RotationStrategy.TWP, retract=25.4)
        self.machine.output.units = OutputUnits.IMPERIAL
        op = self._op("Tilted", self._plane(origin=Vector(25.4, 50.8, 0)))
        processor = self._processor()
        processor.values["AXIS_PRECISION"] = 4
        shape = self._shape(self._expand([self._item(op)], processor))
        self.assertEqual(shape[0], "G53 G0 Z1.0000")
        self.assertEqual(shape[1], "G68.2 X1.0000 Y2.0000 Z0.0000 I0.0000 J45.0000 K0.0000")

    def test_theMachinePropertiesReplaceAnyLine(self):
        op = self._op("Tilted", self._plane())
        processor = self._processor()
        processor.values["TWP_DECLARE"] = "G68.2 P0 X{x} Y{y} Z{z} I{a1} J{a2} K{a3}"
        processor.values["TWP_ALIGN"] = ""  # empty: the dialect's own line
        processor.values["INDEX_RETRACT"] = "G53 G0 Z{z} (home)"
        self.assertEqual(
            self._shape(self._expand([self._item(op)], processor)),
            [
                "G53 G0 Z0.000 (home)",
                "G68.2 P0 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G69",
            ],
        )

    def test_theProgramPositionsTheRotariesWhenTheControlDoesNot(self):
        self.machine = _machineCA(RotationStrategy.TWP, control_positions=False)
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)])
        shape = self._shape(items)
        self.assertEqual(shape[0], "G53 G0 Z0.000")
        self.assertTrue(shape[1].startswith("rotation:G0"), shape)
        self.assertTrue(shape[2].startswith("G68.2"), shape)
        self.assertEqual(shape[3:], ["operation", "G69"])
        positions = {k: float(v) for k, v in dict(op.RotaryPositions).items()}
        rotary = items[1].path.Commands[0]
        for axis, angle in positions.items():
            self.assertAlmostEqual(rotary.Parameters[axis], angle, places=6)

    # sequencing

    def test_operationsOnTheSamePlaneShareOneDeclaration(self):
        plane = self._plane()
        a, b = self._op("A", plane), self._op("B", plane)
        self.assertEqual(
            self._shape(self._expand([self._item(a), self._item(b)])),
            [
                "G53 G0 Z0.000",
                "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "operation",
                "G69",
            ],
        )

    def test_aToolChangeCancelsThePlaneAndItIsDeclaredAgainAfter(self):
        plane = self._plane()
        a, b = self._op("A", plane), self._op("B", plane)
        self.assertEqual(
            self._shape(self._expand([self._item(a), self._tool_change(), self._item(b)])),
            [
                "G53 G0 Z0.000",
                "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G69",
                "tool_controller",
                "G53 G0 Z0.000",
                "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000",
                "G53.1",
                "operation",
                "G69",
            ],
        )

    def test_returningToTheTableCancelsThePlaneAndCommandsTheRotariesHome(self):
        tilted = self._op("Tilted", self._plane())
        plain = self._op("Plain", None)
        self.assertTrue(dict(plain.RotaryPositions), "a rotary machine records zeros too")
        shape = self._shape(self._expand([self._item(tilted), self._item(plain)]))
        self.assertEqual(shape[3], "operation")
        self.assertEqual(shape[4], "G53 G0 Z0.000")
        self.assertEqual(shape[5], "G69")
        self.assertTrue(shape[6].startswith("rotation:G0"), shape)
        self.assertEqual(shape[7], "operation")
        self.assertEqual(len(shape), 8, shape)
        rotary = self._expand([self._item(tilted), self._item(plain)])[6].path.Commands[0]
        for angle in rotary.Parameters.values():
            self.assertAlmostEqual(angle, 0.0, places=6)

    def test_aTurnedXOnTheSameTiltRedeclaresWithoutRetracting(self):
        axis = _tiltedAboutX()
        first = self._plane(axis)
        turned = self._plane(axis, x=Vector(-1, 0, 0))
        a, b = self._op("A", first), self._op("B", turned)
        self.assertEqual(
            dict(a.RotaryPositions), dict(b.RotaryPositions), "same tool axis, same angles"
        )
        shape = self._shape(self._expand([self._item(a), self._item(b)]))
        self.assertEqual(shape[3], "operation")
        self.assertEqual(shape[4], "G69", "the first plane is cancelled")
        self.assertTrue(shape[5].startswith("G68.2"), "the turned plane is declared")
        self.assertEqual(shape[6:], ["G53.1", "operation", "G69"])
        self.assertNotIn("G53 G0 Z0.000", shape[4:], "the rotaries do not move")

    # the other strategies

    def test_dwoRetractsThenCommandsTheRotariesAndRotatesThePath(self):
        self.machine = _machineCA(RotationStrategy.DWO)
        op = self._op("Tilted", self._plane())
        items = self._expand([self._item(op)])
        shape = self._shape(items)
        self.assertEqual(shape[0], "G53 G0 Z0.000")
        self.assertTrue(shape[1].startswith("rotation:G0"), shape)
        self.assertEqual(shape[2:], ["operation"])
        positions = {k: float(v) for k, v in dict(op.RotaryPositions).items()}
        chain = rotation.build_kinematic_chain(self.machine)
        R = rotation.compute_rotation_matrix(chain, positions)
        world = PathUtils.getPathWithPlacement(op)
        expected = PathUtils.applyPlacementToPath(FreeCAD.Placement(Vector(), R), world)
        self.assertEqual(_gcode(items[2].path), _gcode(expected))

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

    # end to end

    def test_aRealPostEmitsTheProgram(self):
        """Through export2 with a shipped post: the plane lines land in the
        program, the cut is in plane coordinates, and the rotary move that
        PRE_ROTARY_MOVE wraps does not trip on a missing value."""
        from Path.Post.Processor import PostProcessorFactory

        self.machine = _machineCA(RotationStrategy.TWP, control_positions=False)
        self.machine.output.comments.enabled = False
        self.machine.output.output_header = False
        op = self._op("Tilted", self._plane())
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        post.reinitialize()
        post._machine = self.machine
        post._merge_machine_config()
        sections = post.export2()
        gcode = "\n".join(g for _, g in sections)
        lines = [line.strip() for line in gcode.splitlines()]
        self.assertIn("G53 G0 Z0.000", lines)
        self.assertIn("G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000", lines)
        self.assertIn("G69", lines)
        self.assertTrue(any(line.startswith("G0 ") and "A" in line for line in lines), lines)
        self.assertIn("G1 X10.000 Y0.000 Z-2.000", lines)


class TestMachineRotationStrategy(unittest.TestCase):
    def test_defaultsDeclareNothing(self):
        k = Machine().kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.NONE)
        self.assertEqual(k.plane_command, PlaneCommand.G68_2)
        self.assertTrue(k.control_positions_rotaries)
        self.assertEqual(k.index_retract_z, 0.0)
        self.assertFalse(k.dwo_supported)

    def test_roundTrip(self):
        m = _machineCA(RotationStrategy.TWP, PlaneCommand.PLANE_SPATIAL, False, -12.5)
        k = Machine.from_dict(m.to_dict()).kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.TWP)
        self.assertEqual(k.plane_command, PlaneCommand.PLANE_SPATIAL)
        self.assertFalse(k.control_positions_rotaries)
        self.assertEqual(k.index_retract_z, -12.5)

    def test_anOlderFileWithDwoSupportedSelectsDwo(self):
        data = Machine(name="old").to_dict()
        kin = data["machine"]["kinematics"]
        for key in (
            "rotation_strategy",
            "plane_command",
            "control_positions_rotaries",
            "index_retract_z",
        ):
            del kin[key]
        kin["dwo_supported"] = True
        k = Machine.from_dict(data).kinematics
        self.assertEqual(k.rotation_strategy, RotationStrategy.DWO)
        self.assertTrue(k.dwo_supported)
        kin["dwo_supported"] = False
        self.assertEqual(
            Machine.from_dict(data).kinematics.rotation_strategy, RotationStrategy.NONE
        )

    def test_dwoSupportedIsWrittenForOlderReaders(self):
        m = _machineCA(RotationStrategy.DWO)
        self.assertTrue(m.to_dict()["machine"]["kinematics"]["dwo_supported"])
        self.assertFalse(
            _machineCA(RotationStrategy.TWP).to_dict()["machine"]["kinematics"]["dwo_supported"]
        )
