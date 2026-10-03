# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2023 Larry Woestman <LarryWoestman2@gmail.com>
# SPDX-FileCopyrightText: 2026 Petter Reinholdtsen <pere@hungry.com>
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

import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks

from Path.Post.Processor import PostProcessorFactory

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# The mock job posts the tool controller, the fixture and the operation in that
# order. With --no-header this is the line the operation's first command lands on.
FIRST_OP_LINE = 20

# Lines the mock tool change produces before the operation, without comments.
TOOL_CHANGE = ["M05", "M6T1", "M3S1000", "L1.0", "E01"]


class TestDynapath4060Post(PathTestUtils.PathTestBase):
    """Test suite for the Dynapath Delta 40/50/60 legacy postprocessor."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code
        and objects here that are needed for the duration of the test() methods
        in this class.  In other words, set up the 'global' test environment
        here; use the `setUp()` method to set up a 'local' test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        # Create mock job with default operation and tool controller
        self.job, self.profile_op, self.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        # Canned cycles read the operation's clearance height for the O word.
        self.profile_op.ClearanceHeight = type("obj", (object,), {"Value": 18.0})()

        # Create postprocessor using the mock job
        self.post = PostProcessorFactory.get_post_processor(self.job, "dynapath_4060_legacy")

        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        # reinitialize the postprocessor data structures between tests
        self.post.reinitialize()

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """

    def _op_lines(self, commands, args="--no-header --no-comments --no-show-editor"):
        """Post `commands` as the operation and return the lines it produced,
        between the tool change and the postamble."""
        # Arguments such as --inches persist in the script's globals; start each
        # export from a freshly loaded script, as the GUI does.
        self.post.load_script()
        self.profile_op.Path = Path.Path(commands)
        self.job.PostProcessorArgs = args
        lines = self.post.export()[0][1].splitlines()
        start = lines.index("E01") + 1
        end = len(lines) - lines[::-1].index("M05") - 1
        return lines[start:end]

    def test_empty_path(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-show-editor"

        # Test generating with header
        # Header contains a time stamp that messes up unit testing.
        # Only test length of result.
        gcode = self.post.export()[0][1]
        self.assertEqual(33, len(gcode.splitlines()))
        # Test without header
        expected = """(T)BEGIN PREAMBLE$
G17
G90
G80
G40
G71
(T)BEGIN OPERATION: TC: DEFAULT TOOL$
(T)MACHINE UNITS: MM/MIN$
(T)BEGIN TOOLCHANGE$
M05
M6T1
M3S1000
L1.0
(T)FINISH OPERATION: TC: DEFAULT TOOL$
(T)BEGIN OPERATION: FIXTURE$
(T)MACHINE UNITS: MM/MIN$
E01
(T)FINISH OPERATION: FIXTURE$
(T)BEGIN OPERATION: PROFILE$
(T)MACHINE UNITS: MM/MIN$
(T)FINISH OPERATION: PROFILE$
(T)BEGIN POSTAMBLE$
M05
G80
G40
G17
G90
M30
E
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"

        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """G17
G90
G80
G40
G71
M05
M6T1
M3S1000
L1.0
E01
M05
G80
G40
G17
G90
M30
E
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-header --no-comments --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

    def test_precision(self):
        """Test command Generation.
        Test Precision
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[FIRST_OP_LINE]
        expected = "G0X10.000Y20.000Z30.000"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[FIRST_OP_LINE]
        expected = "G0X10.00Y20.00Z30.00"
        self.assertEqual(result, expected)

    def test_line_numbers(self):
        """
        Test Line Numbers
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --line-numbers --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[FIRST_OP_LINE]
        expected = "N0021G0X10.000Y20.000Z30.000"
        self.assertEqual(result, expected)

    def test_pre_amble(self):
        """
        Test Pre-amble
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --preamble='G18\nG55' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        lines = gcode.splitlines()
        self.assertEqual(lines[0], "G18")
        self.assertEqual(lines[1], "G55")

    def test_post_amble(self):
        """
        Test Post-amble
        """
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --postamble='G0 Z50\nM30' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[-3], "G0 Z50")
        self.assertEqual(gcode.splitlines()[-2], "M30")
        self.assertEqual(gcode.splitlines()[-1], "E")

    def test_inches(self):
        """
        Test inches
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --inches --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[5], "G70")

        result = gcode.splitlines()[FIRST_OP_LINE]
        expected = "G0X0.394Y0.787Z1.181"

        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --inches --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[FIRST_OP_LINE]
        expected = "G0X0.39Y0.79Z1.18"
        self.assertEqual(result, expected)

    def test_tool_change(self):
        """Dynapath stops the spindle before M6 and needs an XYZ move after it.
        The first rapid after the tool change gets X0 Y0; later retracts don't."""
        lines = self._op_lines(
            [
                Path.Command("G0 Z18"),
                Path.Command("G0 X5 Y5"),
                Path.Command("G1 Z-1 F100"),
                Path.Command("G0 Z18"),
            ]
        )
        self.assertEqual(
            lines, ["G0X0.000Y0.000Z18.000", "G0X5.000Y5.000", "G1Z-1.000F6000.000", "G0Z18.000"]
        )

        gcode = self.post.export()[0][1].splitlines()
        self.assertEqual(gcode[5:10], TOOL_CHANGE)

        # A first rapid that already carries XY is left alone.
        lines = self._op_lines([Path.Command("G0 X5 Y5 Z18"), Path.Command("G0 Z18")])
        self.assertEqual(lines, ["G0X5.000Y5.000Z18.000", "G0Z18.000"])

        # The inserted XY follows the output units.
        lines = self._op_lines(
            [Path.Command("G0 Z25.4")], "--no-header --no-comments --no-show-editor --inches"
        )
        self.assertEqual(lines, ["G0X0.000Y0.000Z1.000"])

    def test_canned_cycle(self):
        """Every hole of a canned cycle is posted, with O and R reference planes,
        and the cycle is closed with G80 before the next plain command."""
        holes = [
            Path.Command("G81 X2 Y2 Z0 R16 F0.8"),
            Path.Command("G81 X18 Y18 Z0 R16 F0.8"),
        ]
        lines = self._op_lines(holes + [Path.Command("G0 Z18")])
        self.assertEqual(
            lines,
            [
                "G81X2.000Y2.000Z0.000F48.000O18.000R16.000",
                "G81X18.000Y18.000Z0.000F48.000O18.000R16.000",
                "G80",
                "G0X0.000Y0.000Z18.000",
            ],
        )

        # A cycle still open at the end of the operation is closed there.
        lines = self._op_lines(holes)
        self.assertEqual(lines[-1], "G80")

        # A G80 the operation emits itself is not doubled.
        lines = self._op_lines(holes + [Path.Command("G80"), Path.Command("G0 Z18")])
        self.assertEqual(lines[2:], ["G80", "G0X0.000Y0.000Z18.000"])

        # G98/G99 have no meaning on Dynapath; the retract planes are R and O.
        lines = self._op_lines([Path.Command("G98")] + holes[:1] + [Path.Command("G99")])
        self.assertEqual(lines, ["G81X2.000Y2.000Z0.000F48.000O18.000R16.000", "G80"])

        # Holes keep their XY even when identical axis values are suppressed.
        lines = self._op_lines(
            [Path.Command("G81 X2 Y2 Z0 R16 F0.8"), Path.Command("G81 X2 Y9 Z0 R16 F0.8")],
            "--no-header --no-comments --no-show-editor --axis-modal",
        )
        self.assertEqual(
            lines,
            [
                "G81X2.000Y2.000Z0.000F48.000O18.000R16.000",
                "G81X2.000Y9.000O18.000R16.000",
                "G80",
            ],
        )

    def test_tapping(self):
        """A tapping cycle's F is the thread pitch. Dynapath wants a feed rate,
        so the post emits pitch times spindle speed in the output units."""
        taps = [
            Path.Command("G84 X5 Y15 Z2 R10 S500 F1.25"),
            Path.Command("G74 X15 Y5 Z2 R10 S500 F1.25"),
        ]
        lines = self._op_lines(taps + [Path.Command("G0 Z18")])
        self.assertEqual(
            lines,
            [
                "G84F625.000X5.000Y15.000Z2.000O18.000R10.000",
                "G74F625.000X15.000Y5.000Z2.000O18.000R10.000",
                "G80",
                "G0X0.000Y0.000Z18.000",
            ],
        )

        lines = self._op_lines(taps[:1], "--no-header --no-comments --no-show-editor --inches")
        self.assertEqual(lines[0], "G84F24.606X0.197Y0.591Z0.079O0.709R0.394")

    def test_arc_centers(self):
        """Dynapath arcs take absolute centers, in the output units."""
        arc = [
            Path.Command("G0 X10 Y0 Z18"),
            Path.Command("G2 X0 Y10 Z18 I-10 J0 F100"),
        ]
        lines = self._op_lines(arc)
        self.assertEqual(lines[1], "G2X0.000Y10.000Z18.000I0.000J0.000F6000.000")

        lines = self._op_lines(arc, "--no-header --no-comments --no-show-editor --inches")
        self.assertEqual(lines[1], "G2X0.000Y0.394Z0.709I0.000J0.000F236.220")

        arc = [
            Path.Command("G0 X10 Y5 Z18"),
            Path.Command("G3 X20 Y15 Z18 I10 J0 F100"),
        ]
        lines = self._op_lines(arc)
        self.assertEqual(lines[1], "G3X20.000Y15.000Z18.000I20.000J5.000F6000.000")
