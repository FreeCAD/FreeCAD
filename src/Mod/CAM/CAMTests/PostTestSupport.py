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

"""
Shared support for the post-processor tests.

The functions build the machines that the tests of the machine pipeline
(``export2()``) need. ``MachinePostTestBase`` drives one post on a mock job, and
``BoxtestJobMixin`` loads the ``boxtest.fcstd`` Job for the tests of the
legacy posts (``export()``). This module has no tests of its own.

See README.md in this directory.
"""

import os

import FreeCAD
import Constants
import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Machine.models.machine import (
    AxisRole,
    Machine,
    OutputUnits,
    RotaryAxis,
    Toolhead,
    ToolheadType,
)
from Path.Post.Processor import PostProcessor, PostProcessorFactory

EOL = "\n"

# Table-table machines for make_rotary_machine(): the axis on the machine base,
# its rotation vector and its limits. That axis carries C.
_ROTARY_KINDS = {
    "AC": ("A", FreeCAD.Vector(1, 0, 0), (-120, 120)),
    "BC": ("B", FreeCAD.Vector(0, 1, 0), (-35, 120)),
}


def make_toolhead(name="Default Toolhead", max_rpm=24000, max_power_kw=1.0, **fields):
    """A rotary toolhead. export2() needs a toolhead at index 0."""
    return Toolhead(
        name=name,
        toolhead_type=ToolheadType.ROTARY,
        min_rpm=fields.pop("min_rpm", 0),
        max_rpm=max_rpm,
        max_power_kw=max_power_kw,
        **fields,
    )


def make_manual_toolhead(**fields):
    """A router spindle: manual tool change, 6000 to 24000 rpm."""
    return make_toolhead(
        id="toolhead1", max_power_kw=2.2, min_rpm=6000, tool_change="manual", **fields
    )


def quiet_output(machine):
    """Reduce the output of a machine to the lines that the commands produce.

    No header, no comments and no tool change, with fixed precision and with
    each word on each line. A test then reads one output line for one command.
    """
    machine.output.units = OutputUnits.METRIC
    machine.output.output_header = False
    machine.output.comments.enabled = False
    machine.output.precision.axis = 3
    machine.output.precision.feed = 3
    machine.output.precision.spindle = 0
    machine.output.duplicates.commands = True
    machine.output.duplicates.parameters = True
    machine.processing.tool_change_format = Constants.ToolChangeFormat.NO_TOOL_CHANGE
    return machine


def make_machine(name="Test Machine", quiet=False, toolhead=None):
    """A three-axis machine with one toolhead.

    quiet=False gives the defaults of the machine model. quiet=True applies
    quiet_output().
    """
    machine = Machine.create_3axis_config()
    machine.name = name
    machine.toolheads = [toolhead or make_toolhead()]
    if quiet:
        quiet_output(machine)
    return machine


def make_rotary_machine(kind, strategy=None, name=None, quiet=True):
    """A table-table five-axis machine: "AC" or "BC". The first axis carries C."""
    base_name, vector, limits = _ROTARY_KINDS[kind]
    machine = make_machine(name or f"Test {kind} Machine")
    machine.rotary_axes["C"] = RotaryAxis(
        name="C",
        rotation_vector=FreeCAD.Vector(0, 0, 1),
        role=AxisRole.TABLE_ROTARY,
        parent=base_name,
        sequence=0,
    )
    machine.rotary_axes[base_name] = RotaryAxis(
        name=base_name,
        rotation_vector=vector,
        min_limit=limits[0],
        max_limit=limits[1],
        role=AxisRole.TABLE_ROTARY,
        parent=None,
        sequence=1,
    )
    if strategy is not None:
        machine.kinematics.rotation_strategy = strategy
    if quiet:
        quiet_output(machine)
    return machine


def load_post(job, post_name):
    """Get a machine-based post from the factory, and fail if it did not load."""
    post = PostProcessorFactory.get_post_processor(job, post_name)
    if not hasattr(post, "apply_configuration_bundle"):
        # The factory returns a CAMError or a script wrapper, not a post, when
        # it cannot load the file as a machine-based post.
        raise AssertionError(f"The '{post_name}' post did not load: {post}")
    return post


def run_export2(job, machine, post=None):
    """Run export2() for a job on a machine, and return the (name, gcode) sections.

    With no post, the base PostProcessor does the conversion.
    """
    if post is None:
        post = PostProcessor(job, "", "", "mm")
    post._machine = machine
    post.apply_configuration_bundle()
    return post.export2()


def machine_post_names():
    """The names of the shipped posts that declare POST_TYPE = "machine"."""
    scripts = os.path.join(os.path.dirname(os.path.abspath(Path.Post.__file__)), "scripts")
    names = []
    for filename in sorted(os.listdir(scripts)):
        if not filename.endswith("_post.py"):
            continue
        with open(os.path.join(scripts, filename), encoding="utf-8") as source:
            if any(line.replace(" ", "").startswith('POST_TYPE="machine"') for line in source):
                names.append(filename[: -len("_post.py")])
    return names


def as_commands(commands):
    """Path.Command objects from a list of command strings and Path.Command objects."""
    return [Path.Command(c) if isinstance(c, str) else c for c in commands]


def all_gcode(sections):
    """The text of all sections that export2() returned."""
    return EOL.join(gcode for _, gcode in sections)


class MachinePostTestBase(PathTestUtils.PathTestBase):
    """One machine-based post on a mock job, with a new machine for each test.

    A subclass sets POST_NAME. It can set MACHINE_NAME and QUIET, and can change
    the machine in configure_machine(). This class has no tests of its own.
    """

    POST_NAME = None
    MACHINE_NAME = "Test Machine"
    # True: quiet_output(), for tests that read single lines.
    # False: the defaults of the machine model, with header and comments.
    QUIET = True

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        # The mock job has no Machine property, so the post does not look for a
        # machine file of the user. setUp() gives the post its machine.
        cls.post = load_post(cls.job, cls.POST_NAME)

    def setUp(self):
        self.maxDiff = None
        self.post.reinitialize()
        machine = make_machine(self.MACHINE_NAME, quiet=self.QUIET)
        self.configure_machine(machine)
        # Each test sets the path that it needs.
        self.profile_op.Path = Path.Path([])
        self.post._machine = machine
        self.post.apply_configuration_bundle()

    def configure_machine(self, machine):
        """Change the machine before the post reads it. The default changes nothing."""

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def set_commands(self, commands):
        """Give the operation a path made of command strings or Path.Command objects."""
        self.profile_op.Path = Path.Path(as_commands(commands))

    def export(self, commands=None):
        """Run export2() and return the text of the first section."""
        if commands is not None:
            self.set_commands(commands)
        self.post.apply_configuration_bundle()
        return self.post.export2()[0][1]

    def lines(self, commands=None):
        return self.export(commands).split(EOL)

    def assert_line(self, expected, lines):
        self.assertIn(expected, lines, f"expected line {expected!r} in\n{EOL.join(lines)}")

    def assert_no_line(self, unexpected, lines):
        self.assertNotIn(unexpected, lines, f"unexpected line {unexpected!r} in\n{EOL.join(lines)}")


class BoxtestJobMixin:
    """Load the boxtest.fcstd Job once for a test class.

    Put the mixin before the TestCase base. It sets cls.doc, cls.job and
    cls.profile_op (the operation with the label "Profile"). A class that sets
    POST_NAME also gets cls.post from the factory.
    """

    POST_NAME = None

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")
        cls.profile_op = next(op for op in cls.job.Operations.Group if op.Label == "Profile")
        if cls.POST_NAME is not None:
            cls.post = PostProcessorFactory.get_post_processor(cls.job, cls.POST_NAME)

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")
        super().tearDownClass()


class LegacyPostTestBase(BoxtestJobMixin, PathTestUtils.PathTestBase):
    """A legacy post on the boxtest Job, run through export().

    A subclass sets POST_NAME. Each test gives the "Profile" operation a path
    and compares the output for a PostProcessorArgs string. This class has no
    tests of its own.
    """

    def setUp(self):
        self.maxDiff = None
        self.post.reinitialize()

    def export_with_args(self, path, args):
        """Give the operation the path, run export() with the arguments, return the text."""
        self.job.PostProcessorArgs = args
        self.profile_op.Path = Path.Path(path)
        # This Job and this operation give one section.
        return self.post.export()[0][1]

    def single_compare(self, path, expected, args, debug=False):
        """Compare the first output line after the three lines of the preamble."""
        gcode = self.export_with_args(path, args)
        if debug:
            print(f"--------{EOL}{gcode}--------{EOL}")
        self.assertEqual(gcode.splitlines()[3], expected)

    def multi_compare(self, path, expected, args, debug=False):
        """Compare the full output."""
        gcode = self.export_with_args(path, args)
        if debug:
            print(f"--------{EOL}{gcode}--------{EOL}")
        self.assertEqual(gcode, expected)
