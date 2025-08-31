# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022-2025 Larry Woestman <LarryWoestman2@gmail.com>     *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from os import linesep, path, remove
import tempfile
from unittest.mock import mock_open, patch

import FreeCAD

import Path
import CAMTests.PathTestUtils as PathTestUtils
from Path.Post.Command import CommandPathPost
from Path.Post.Processor import PostProcessorFactory
from Path.Post.Utils import FilenameGenerator

from PySide.QtCore import QT_TRANSLATE_NOOP  # type: ignore

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestRefactoredTestPost(PathTestUtils.PathTestBase):
    """Test the refactored_test_post.py postprocessor command line arguments."""

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
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "refactored_test")
        # locate the operation named "Profile"
        for op in cls.job.Operations.Group:
            if op.Label == "Profile":
                # remember the "Profile" operation
                cls.profile_op = op
                return

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        #
        # reinitialize the postprocessor data structures between tests
        #
        self.post.reinitialize()

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def single_compare(self, test_path, expected, args, debug=False):
        """Perform a test with a single line of gcode comparison."""
        nl = "\n"
        self.job.PostProcessorArgs = args
        # replace the original path (that came with the job and operation) with our path
        self.profile_op.Path = Path.Path(test_path)
        # the gcode is in the first section for this particular job and operation
        gcode = self.post.export()[0][1]
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        # there are 3 lines of "other stuff" before the line we are interested in
        self.assertEqual(gcode.splitlines()[3], expected)

    def multi_compare(self, test_path, expected, args, debug=False):
        """Perform a test with multiple lines of gcode comparison."""
        nl = "\n"

        self.job.PostProcessorArgs = args
        # replace the original path (that came with the job and operation) with our path
        self.profile_op.Path = Path.Path(test_path)
        # the gcode is in the first section for this particular job and operation
        gcode = self.post.export()[0][1]
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    #############################################################################
    #
    # The tests are organized into groups:
    #
    #   00000 - 00099  tests that don't fit any other category
    #   00100 - 09999  tests for all of the various arguments/options
    #   10000 - 18999  tests for the various G codes at 10000 + 10 * g_code_value
    #   19000 - 19999  tests for the A, B, and C axis outputs
    #   20000 - 29999  tests for the various M codes at 20000 + 10 * m_code_value
    #
    #############################################################################

    def test00100(self):
        """Test axis modal.

        Suppress the axis coordinate if the same as previous
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        self.profile_op.Path = Path.Path([c, c1])

        self.job.PostProcessorArgs = "--axis-modal"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[4], "G0 Y30.000")

        self.job.PostProcessorArgs = "--no-axis-modal"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[4], "G0 X10.000 Y30.000 Z30.000")

    #############################################################################

    def test00110(self):
        """Test axis-precision."""
        self.single_compare("G0 X10 Y20 Z30", "G0 X10.00 Y20.00 Z30.00", "--axis-precision=2")

    #############################################################################

    def test00120(self):
        """Test bcnc."""
        self.multi_compare(
            [],
            """G90
G21
(Block-name: Fixture)
(Block-expand: 0)
(Block-enable: 1)
G54
(Block-name: TC: Default Tool)
(Block-expand: 0)
(Block-enable: 1)
(Block-name: Profile)
(Block-expand: 0)
(Block-enable: 1)
(Block-name: post_amble)
(Block-expand: 0)
(Block-enable: 1)
""",
            "--bcnc",
        )
        self.multi_compare(
            [],
            """G90
G21
G54
""",
            "--no-bcnc",
        )

    #############################################################################

    def test00125(self) -> None:
        """Test chipbreaking amount."""
        test_path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G73 X1 Y2 Z0 F123 Q1.5 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        # check the default chipbreaking amount
        self.multi_compare(
            test_path,
            """G90
G21
G54
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z3.750
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z2.250
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z0.750
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
G90
""",
            "--translate_drill",
        )
        # check for a metric chipbreaking amount
        self.multi_compare(
            test_path,
            """G90
G21
G54
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z4.735
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z3.235
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z1.735
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
G90
""",
            "--translate_drill --chipbreaking_amount='1.23456 mm'",
        )
        # check for an inch/imperial chipbreaking amount
        test_path = [
            Path.Command("G0 X25.4 Y50.8"),
            Path.Command("G0 Z203.2"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G73 X25.4 Y50.8 Z0 F123 Q38.1 R127"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.multi_compare(
            test_path,
            """G90
G20
G54
G0 X1.0000 Y2.0000
G0 Z8.0000
G90
G0 X1.0000 Y2.0000
G1 Z5.0000 F290.5512
G1 Z3.5000 F290.5512
G0 Z3.7500
G0 Z3.5750
G1 Z2.0000 F290.5512
G0 Z2.2500
G0 Z2.0750
G1 Z0.5000 F290.5512
G0 Z0.7500
G0 Z0.5750
G1 Z0.0000 F290.5512
G0 Z5.0000
G90
""",
            "--translate_drill --chipbreaking_amount='0.25 in' --inches",
        )

    #############################################################################

    def test00126(self) -> None:
        """Test command space."""
        self.single_compare("G0 X10 Y20 Z30", "G0 X10.000 Y20.000 Z30.000", "")
        self.single_compare("G0 X10 Y20 Z30", "G0X10.000Y20.000Z30.000", "--command_space=''")
        self.single_compare("G0 X10 Y20 Z30", "G0_X10.000_Y20.000_Z30.000", "--command_space='_'")
        test_path = [Path.Command("(comment with spaces)")]
        self.multi_compare(
            test_path,
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(comment with spaces)
(Finish operation)
(Begin postamble)
""",
            "--command_space=' ' --comments",
        )
        self.multi_compare(
            test_path,
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6T1)
(Finish operation)
(Begin operation)
(comment with spaces)
(Finish operation)
(Begin postamble)
""",
            "--command_space='' --comments",
        )

    #############################################################################

    def test00127(self) -> None:
        """Test comment symbol."""
        test_path = [Path.Command("(comment with spaces)")]
        self.multi_compare(
            test_path,
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(comment with spaces)
(Finish operation)
(Begin postamble)
""",
            "--comments",
        )
        self.multi_compare(
            test_path,
            """;Begin preamble
G90
G21
;Begin operation
G54
;Finish operation
;Begin operation
;TC: Default Tool
;Begin toolchange
;M6 T1
;Finish operation
;Begin operation
;comment with spaces
;Finish operation
;Begin postamble
""",
            "--comment_symbol=';' --comments",
        )
        self.multi_compare(
            test_path,
            """!Begin preamble
G90
G21
!Begin operation
G54
!Finish operation
!Begin operation
!TC: Default Tool
!Begin toolchange
!M6 T1
!Finish operation
!Begin operation
!comment with spaces
!Finish operation
!Begin postamble
""",
            "--comment_symbol='!' --comments",
        )

    #############################################################################

    def test00130(self):
        """Test comments."""
        nl = "\n"

        c = Path.Command("(comment)")
        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--comments"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[12], "(comment)")

    #############################################################################

    def test00135(self) -> None:
        """Test enabling and disabling coolant."""
        args: str
        expected: str
        gcode: str
        nl = "\n"
        save_CoolantMode = self.profile_op.CoolantMode

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])

        # Test Flood coolant enabled
        self.profile_op.CoolantMode = "Flood"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Coolant On: Flood)
M8
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Coolant Off: Flood)
M9
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--enable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test Mist coolant enabled
        self.profile_op.CoolantMode = "Mist"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Coolant On: Mist)
M7
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Coolant Off: Mist)
M9
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--enable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test None coolant enabled with CoolantMode property
        self.profile_op.CoolantMode = "None"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--enable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test Flood coolant disabled
        self.profile_op.CoolantMode = "Flood"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--disable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test Mist coolant disabled
        self.profile_op.CoolantMode = "Mist"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--disable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test None coolant disabled with CoolantMode property
        self.profile_op.CoolantMode = "None"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--disable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test Flood coolant configured but no coolant argument (default)
        self.profile_op.CoolantMode = "Flood"
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test coolant enabled without a CoolantMode property

        self.profile_op.removeProperty("CoolantMode")

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--enable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test coolant disabled without a CoolantMode property
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
G0 X10.000 Y20.000 Z30.000
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--disable_coolant --comments"
        gcode = self.post.export()[0][1]
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # re-create the original CoolantMode property
        self.profile_op.addProperty(
            "App::PropertyEnumeration",
            "CoolantMode",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Coolant option for this operation"),
        )
        self.profile_op.CoolantMode = ["None", "Flood", "Mist"]
        self.profile_op.CoolantMode = save_CoolantMode

    #############################################################################

    def test00137(self) -> None:
        """Test enabling/disabling machine specific commands."""

        # test with machine specific commands enabled
        self.multi_compare(
            "(MC_RUN_COMMAND: blah)",
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(MC_RUN_COMMAND: blah)
blah
(Finish operation)
(Begin postamble)
""",
            "--enable_machine_specific_commands --comments",
        )
        # test with machine specific commands disabled
        self.multi_compare(
            "(MC_RUN_COMMAND: blah)",
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(MC_RUN_COMMAND: blah)
(Finish operation)
(Begin postamble)
""",
            "--disable_machine_specific_commands --comments",
        )
        # test with machine specific commands default
        self.multi_compare(
            "(MC_RUN_COMMAND: blah)",
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(MC_RUN_COMMAND: blah)
(Finish operation)
(Begin postamble)
""",
            "--comments",
        )
        # test with odd characters and spaces in the machine specific command
        self.multi_compare(
            "(MC_RUN_COMMAND: These are odd characters:!@#$%^&*?/)",
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(MC_RUN_COMMAND: These are odd characters:!@#$%^&*?/)
These are odd characters:!@#$%^&*?/
(Finish operation)
(Begin postamble)
""",
            "--enable_machine_specific_commands --comments",
        )

    #############################################################################

    def test00138(self) -> None:
        """Test end of line characters."""

        class MockWriter:
            def __init__(self):
                self.contents = ""

            def write(self, data):
                self.contents = data

        writer = MockWriter()
        opener = mock_open()
        opener.return_value.write = writer.write

        output_file_pattern = path.join(tempfile.gettempdir(), "test_postprocessor_write.nc")
        self.job.PostProcessorOutputFile = output_file_pattern
        Path.Preferences.setOutputFileDefaults(output_file_pattern, "Append Unique ID on conflict")
        policy = Path.Preferences.defaultOutputPolicy()
        generator = FilenameGenerator(job=self.job)
        generated_filename = generator.generate_filenames()
        generator.set_subpartname("")
        fname = next(generated_filename)

        self.profile_op.Path = Path.Path([])

        # Test with whatever end-of-line characters the system running the test happens to use
        expected = """G90
G21
G54
"""
        self.job.PostProcessorArgs = ""
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)
        # also test what is written to a mock file
        with patch("builtins.open", opener) as m:
            CommandPathPost._write_file(self, fname, gcode, policy)
        if m.call_args.kwargs["newline"] is None:
            mocked_output = writer.contents.replace("\n", linesep)
        else:
            mocked_output = writer.contents
        expected = expected.replace("\n", linesep)
        self.assertEqual(expected, mocked_output)

        # Test with a new line
        expected = "\n\nG90\nG21\nG54\n"
        self.job.PostProcessorArgs = "--end_of_line_characters='\n'"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)
        # also test what is written to a mock file
        with patch("builtins.open", opener) as m:
            CommandPathPost._write_file(self, fname, gcode, policy)
        if m.call_args.kwargs["newline"] is None:
            mocked_output = writer.contents.replace("\n", linesep)
        else:
            mocked_output = writer.contents
        expected = expected[2:]
        self.assertEqual(expected, mocked_output)

        # Test with a carriage return followed by a new line
        expected = "G90\r\nG21\r\nG54\r\n"
        self.job.PostProcessorArgs = "--end_of_line_characters='\r\n'"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)
        # also test what is written to a mock file
        with patch("builtins.open", opener) as m:
            CommandPathPost._write_file(self, fname, gcode, policy)
        if m.call_args.kwargs["newline"] is None:
            mocked_output = writer.contents.replace("\n", linesep)
        else:
            mocked_output = writer.contents
        self.assertEqual(expected, mocked_output)

        # Test with a carriage return
        expected = "G90\rG21\rG54\r"
        self.job.PostProcessorArgs = "--end_of_line_characters='\r'"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)
        # also test what is written to a mock file
        with patch("builtins.open", opener) as m:
            CommandPathPost._write_file(self, fname, gcode, policy)
        if m.call_args.kwargs["newline"] is None:
            mocked_output = writer.contents.replace("\n", linesep)
        else:
            mocked_output = writer.contents
        self.assertEqual(expected, mocked_output)

        # Test writing a mock file with a zero-length string for gcode
        expected = ""
        gcode = ""
        with patch("builtins.open", opener) as m:
            CommandPathPost._write_file(self, fname, gcode, policy)
        if m.call_args.kwargs["newline"] is None:
            mocked_output = writer.contents.replace("\n", linesep)
        else:
            mocked_output = writer.contents
        self.assertEqual(expected, mocked_output)

    #############################################################################

    def test00140(self):
        """Test feed-precision."""
        nl = "\n"

        c = Path.Command("G1 X10 Y20 Z30 F123.123456")
        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = ""
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[3], "G1 X10.000 Y20.000 Z30.000 F7387.407")

        self.job.PostProcessorArgs = "--feed-precision=2"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[3], "G1 X10.000 Y20.000 Z30.000 F7387.41")

    #############################################################################

    def test00145(self) -> None:
        """Test the finish label argument."""
        # test the default finish label
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
""",
            "--comments",
        )

        # test a changed finish label
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(End operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(End operation)
(Begin operation)
(End operation)
(Begin postamble)
""",
            "--finish_label='End' --comments",
        )

    #############################################################################

    def test00150(self):
        """Test output with an empty path.

        Also tests the interactions between --comments and --header.
        """
        nl = "\n"

        self.profile_op.Path = Path.Path([])

        # Test generating with comments and header.
        self.job.PostProcessorArgs = "--comments --header"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        split_gcode = gcode.splitlines()
        self.assertEqual(split_gcode[0], "(Exported by FreeCAD)")
        self.assertEqual(split_gcode[1], "(Post Processor: refactored_test_post)")
        self.assertEqual(split_gcode[2], "(Cam File: boxtest.fcstd)")
        # The header contains a time stamp that messes up unit testing.
        # Only test the length of the line that contains the time.
        self.assertIn("(Output Time: ", split_gcode[3])
        self.assertTrue(len(split_gcode[3]) == 41)
        self.assertEqual(split_gcode[4], "(Begin preamble)")
        self.assertEqual(split_gcode[5], "G90")
        self.assertEqual(split_gcode[6], "G21")
        self.assertEqual(split_gcode[7], "(Begin operation)")
        self.assertEqual(split_gcode[8], "G54")
        self.assertEqual(split_gcode[9], "(Finish operation)")
        self.assertEqual(split_gcode[10], "(Begin operation)")
        self.assertEqual(split_gcode[11], "(TC: Default Tool)")
        self.assertEqual(split_gcode[12], "(Begin toolchange)")
        self.assertEqual(split_gcode[13], "(M6 T1)")
        self.assertEqual(split_gcode[14], "(Finish operation)")
        self.assertEqual(split_gcode[15], "(Begin operation)")
        self.assertEqual(split_gcode[16], "(Finish operation)")
        self.assertEqual(split_gcode[17], "(Begin postamble)")

        # Test with comments without header.
        expected = """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
"""
        self.job.PostProcessorArgs = "--comments --no-header"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

        # Test without comments with header.
        self.job.PostProcessorArgs = "--no-comments --header"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[0], "(Exported by FreeCAD)")
        self.assertEqual(split_gcode[1], "(Post Processor: refactored_test_post)")
        self.assertEqual(split_gcode[2], "(Cam File: boxtest.fcstd)")
        # The header contains a time stamp that messes up unit testing.
        # Only test the length of the line that contains the time.
        self.assertIn("(Output Time: ", split_gcode[3])
        self.assertTrue(len(split_gcode[3]) == 41)
        self.assertEqual(split_gcode[4], "G90")
        self.assertEqual(split_gcode[5], "G21")
        self.assertEqual(split_gcode[6], "G54")

        # Test without comments or header.
        expected = """G90
G21
G54
"""
        self.job.PostProcessorArgs = "--no-comments --no-header"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    #############################################################################

    def test00160(self):
        """Test Line Numbers."""
        self.single_compare("G0 X10 Y20 Z30", "N130 G0 X10.000 Y20.000 Z30.000", "--line-numbers")
        self.single_compare("G0 X10 Y20 Z30", "G0 X10.000 Y20.000 Z30.000", "--no-line-numbers")

    #############################################################################

    def test00165(self) -> None:
        """Test line number increment and line number start."""
        test_path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
        ]
        # check the default line number increment
        self.multi_compare(
            test_path,
            """N100 G90
N110 G21
N120 G54
N130 G0 X1.000 Y2.000
N140 G0 Z8.000
""",
            "--line-numbers",
        )

        # check a non-default line number increment
        self.multi_compare(
            test_path,
            """N150 G90
N153 G21
N156 G54
N159 G0 X1.000 Y2.000
N162 G0 Z8.000
""",
            "--line-numbers --line_number_increment=3",
        )

        # check a non-default starting line number
        self.multi_compare(
            test_path,
            """N123 G90
N126 G21
N129 G54
N132 G0 X1.000 Y2.000
N135 G0 Z8.000
""",
            "--line-numbers --line_number_increment=3 --line_number_start=123",
        )

    #############################################################################

    def test00166(self) -> None:
        """Test listing tools in the preamble."""
        test_path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
        ]
        # test listing tools in the preamble
        self.multi_compare(
            test_path,
            """(T1=TC__Default_Tool)
(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
M6 T1
(Finish operation)
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
(Finish operation)
(Begin postamble)
""",
            "--comments --tool_change --list_tools_in_preamble",
        )
        # test not listing tools in the preamble
        self.multi_compare(
            test_path,
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
M6 T1
(Finish operation)
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
(Finish operation)
(Begin postamble)
""",
            "--comments --tool_change --no-list_tools_in_preamble",
        )
        # test the default behavior for listing tools in the preamble
        self.multi_compare(
            test_path,
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
M6 T1
(Finish operation)
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
(Finish operation)
(Begin postamble)
""",
            "--comments --tool_change",
        )

    #############################################################################

    def test00170(self):
        """Test inches."""
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30 A10 B20 C30 U10 V20 W30")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--inches"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[1], "G20")
        self.assertEqual(
            gcode.splitlines()[3],
            "G0 X0.3937 Y0.7874 Z1.1811 A10.0000 B20.0000 C30.0000 U0.3937 V0.7874 W1.1811",
        )

    #############################################################################

    def test00180(self):
        """Test modal.

        Suppress the command name if the same as previous
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        self.profile_op.Path = Path.Path([c, c1])

        self.job.PostProcessorArgs = "--modal"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[4], "X10.000 Y30.000 Z30.000")
        self.job.PostProcessorArgs = "--no-modal"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[4], "G0 X10.000 Y30.000 Z30.000")

    #############################################################################

    def test00190(self):
        """Test Outputting all arguments.

        Empty path.  Outputs all arguments.
        """
        nl = "\n"

        expected = """Arguments that are commonly used:
  --metric              Convert output for Metric mode (G21) (default)
  --inches              Convert output for US imperial mode (G20)
  --axis-modal          Don't output axis values if they are the same as the
                        previous line
  --no-axis-modal       Output axis values even if they are the same as the
                        previous line (default)
  --axis-precision AXIS_PRECISION
                        Number of digits of precision for axis moves, default
                        is 3
  --bcnc                Add Job operations as bCNC block headers. Consider
                        suppressing comments by adding --no-comments
  --no-bcnc             Suppress bCNC block header output (default)
  --chipbreaking_amount CHIPBREAKING_AMOUNT
                        Amount to move for chipbreaking in a translated G73
                        command, default is 0.25 mm
  --command_space COMMAND_SPACE
                        The character to use between parts of a command,
                        default is a space, may also use a null string
  --comments            Output comments (default)
  --no-comments         Suppress comment output
  --comment_symbol COMMENT_SYMBOL
                        The character used to start a comment, default is "("
  --enable_coolant      Enable coolant
  --disable_coolant     Disable coolant (default)
  --enable_machine_specific_commands
                        Enable machine specific commands of the form
                        (MC_RUN_COMMAND: blah)
  --disable_machine_specific_commands
                        Disable machine specific commands (default)
  --end_of_line_characters END_OF_LINE_CHARACTERS
                        The character(s) to use at the end of each line in the
                        output file, default is whatever the system uses, may
                        also use '\\n', '\\r', or '\\r\\n'
  --feed-precision FEED_PRECISION
                        Number of digits of precision for feed rate, default
                        is 3
  --finish_label FINISH_LABEL
                        The characters to use in the 'Finish operation'
                        comment, default is "Finish"
  --header              Output headers (default)
  --no-header           Suppress header output
  --line_number_increment LINE_NUMBER_INCREMENT
                        Amount to increment the line numbers, default is 10
  --line_number_start LINE_NUMBER_START
                        The number the line numbers start at, default is 100
  --line-numbers        Prefix with line numbers
  --no-line-numbers     Don't prefix with line numbers (default)
  --list_tools_in_preamble
                        List the tools used in the operation in the preamble
  --no-list_tools_in_preamble
                        Don't list the tools used in the operation (default)
  --modal               Don't output the G-command name if it is the same as
                        the previous line
  --no-modal            Output the G-command name even if it is the same as
                        the previous line (default)
  --output_all_arguments
                        Output all of the available arguments
  --no-output_all_arguments
                        Don't output all of the available arguments (default)
  --output_machine_name
                        Output the machine name in the pre-operation
                        information
  --no-output_machine_name
                        Don't output the machine name in the pre-operation
                        information (default)
  --output_path_labels  Output Path labels at the beginning of each Path
  --no-output_path_labels
                        Don't output Path labels at the beginning of each Path
                        (default)
  --output_visible_arguments
                        Output all of the visible arguments
  --no-output_visible_arguments
                        Don't output the visible arguments (default)
  --postamble POSTAMBLE
                        Set commands to be issued after the last command,
                        default is ""
  --post_operation POST_OPERATION
                        Set commands to be issued after every operation,
                        default is ""
  --preamble PREAMBLE   Set commands to be issued before the first command,
                        default is ""
  --precision PRECISION
                        Number of digits of precision for both feed rate and
                        axis moves, default is 3 for metric or 4 for inches
  --return-to RETURN_TO
                        Move to the specified x,y,z coordinates at the end,
                        e.g. --return-to=0,0,0 (default is do not move)
  --show-editor         Pop up editor before writing output (default)
  --no-show-editor      Don't pop up editor before writing output
  --tlo                 Output tool length offset (G43) following tool changes
                        (default)
  --no-tlo              Suppress tool length offset (G43) following tool
                        changes
  --tool_change         Insert M6 and any other tool change G-code for all
                        tool changes (default)
  --no-tool_change      Convert M6 to a comment for all tool changes
  --translate_drill     Translate drill cycles G73, G81, G82 & G83 into G0/G1
                        movements
  --no-translate_drill  Don't translate drill cycles G73, G81, G82 & G83 into
                        G0/G1 movements (default)
  --wait-for-spindle WAIT_FOR_SPINDLE
                        Time to wait (in seconds) after M3, M4 (default = 0.0)
"""
        self.profile_op.Path = Path.Path([])

        self.job.PostProcessorArgs = "--output_all_arguments"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        # The argparse help routine turns out to be sensitive to the
        # number of columns in the terminal window that the tests
        # are run from.  This affects the indenting in the output.
        # The next couple of lines remove all of the white space.
        gcode = "".join(gcode.split())
        expected = "".join(expected.split())
        self.assertEqual(gcode, expected)

    #############################################################################

    def test00191(self):
        """Make sure postprocessor doesn't crash on blank lines"""

        path = [
            Path.Command("G0 X1"),
            Path.Command(""),
            Path.Command("G0 X2"),
        ]

        self.post.values["OUTPUT_BLANK_LINES"] = True
        self.multi_compare(
            path,
            """G90
G21
G54
G0 X1.000

G0 X2.000
""",
            "",
        )

        self.post.values["OUTPUT_BLANK_LINES"] = False
        self.multi_compare(
            path,
            """G90
G21
G54
G0 X1.000
G0 X2.000
""",
            "",
        )

    #############################################################################

    def test00200(self):
        """Test Outputting visible arguments.

        Empty path.  Outputs visible arguments.
        """
        self.profile_op.Path = Path.Path([])
        expected = ""
        self.job.PostProcessorArgs = "--output_visible_arguments"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

    #############################################################################

    def test00205(self) -> None:
        """Test output_machine_name argument."""
        # test the default behavior
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
""",
            "--comments",
        )

        # test outputting the machine name
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
(Machine: test, mm/min)
G54
(Finish operation)
(Begin operation)
(Machine: test, mm/min)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Machine: test, mm/min)
(Finish operation)
(Begin postamble)
""",
            "--output_machine_name --comments",
        )

        # test not outputting the machine name
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
""",
            "--no-output_machine_name --comments",
        )

    #############################################################################

    def test00206(self) -> None:
        """Test output_path_labels argument."""
        # test the default behavior
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
""",
            "--comments",
        )

        # test outputting the path labels
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
(Path: Fixture)
G54
(Finish operation)
(Begin operation)
(Path: TC: Default Tool)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Path: Profile)
(Finish operation)
(Begin postamble)
""",
            "--output_path_labels --comments",
        )

        # test not outputting the path labels
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
(Begin operation)
(Finish operation)
(Begin postamble)
""",
            "--no-output_path_labels --comments",
        )

    #############################################################################

    def test00210(self):
        """Test Post-amble."""
        nl = "\n"

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--postamble='G0 Z50\nM2'"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[-2], "G0 Z50")
        self.assertEqual(split_gcode[-1], "M2")

    #############################################################################

    def test00215(self) -> None:
        """Test the post_operation argument."""
        self.multi_compare(
            [],
            """(Begin preamble)
G90
G21
(Begin operation)
G54
(Finish operation)
G90 G80
G40 G49
(Begin operation)
(TC: Default Tool)
(Begin toolchange)
(M6 T1)
(Finish operation)
G90 G80
G40 G49
(Begin operation)
(Finish operation)
G90 G80
G40 G49
(Begin postamble)
""",
            "--comments --post_operation='G90 G80\nG40 G49'",
        )

    #############################################################################

    def test00220(self):
        """Test Pre-amble."""
        nl = "\n"

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--preamble='G18 G55'"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[0], "G18 G55")

    #############################################################################

    def test00230(self):
        """Test precision."""
        self.single_compare(
            "G1 X10 Y20 Z30 F100",
            "G1 X10.00 Y20.00 Z30.00 F6000.00",
            "--precision=2",
        )
        self.single_compare(
            "G1 X10 Y20 Z30 F100",
            "G1 X0.39 Y0.79 Z1.18 F236.22",
            "--inches --precision=2",
        )

    #############################################################################

    def test00240(self):
        """Test return-to."""
        self.single_compare("", "G0 X12 Y34 Z56", "--return-to='12,34,56'")

    #############################################################################

    def test00250(self):
        """Test tlo."""
        nl = "\n"

        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")

        self.profile_op.Path = Path.Path([c, c2])

        self.job.PostProcessorArgs = "--tlo --tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[5], "M6 T2")
        self.assertEqual(split_gcode[6], "G43 H2")
        self.assertEqual(split_gcode[7], "M3 S3000")

        # suppress TLO
        self.job.PostProcessorArgs = "--no-tlo --tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[4], "M6 T2")
        self.assertEqual(split_gcode[5], "M3 S3000")

    #############################################################################

    def test00260(self):
        """Test tool_change."""
        nl = "\n"

        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.profile_op.Path = Path.Path([c, c2])

        self.job.PostProcessorArgs = "--no-comments --no-tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[2], "G54")
        self.assertEqual(split_gcode[3], "M3 S3000")

        self.job.PostProcessorArgs = "--no-comments --tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[4], "M6 T2")

        self.job.PostProcessorArgs = "--comments --no-tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[13], "(M6 T2)")

        self.job.PostProcessorArgs = "--comments --tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[13], "M6 T2")

    #############################################################################

    def test00270(self):
        """Test wait-for-spindle."""
        nl = "\n"

        c = Path.Command("M3 S3000")
        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = ""
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[3], "M3 S3000")

        self.job.PostProcessorArgs = "--wait-for-spindle=1.23456"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[3], "M3 S3000")
        self.assertEqual(split_gcode[4], "G4 P1.23456")

        c = Path.Command("M4 S3000")
        self.profile_op.Path = Path.Path([c])

        # This also tests that the default for --wait-for-spindle
        # goes back to 0.0 (no wait)
        self.job.PostProcessorArgs = ""
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[3], "M4 S3000")

        self.job.PostProcessorArgs = "--wait-for-spindle=1.23456"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[3], "M4 S3000")
        self.assertEqual(split_gcode[4], "G4 P1.23456")
