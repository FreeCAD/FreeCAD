# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
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


from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine
import FreeCAD
import Path
import Path.Post.Command as PathCommand
import Path.Post.Utils as PostUtils
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import os
import unittest

from .FilePathTestUtils import assertFilePathsEqual

PathCommand.LOG_MODULE = Path.Log.thisModule()
Path.Log.setLevel(Path.Log.Level.INFO, PathCommand.LOG_MODULE)


class TestFileNameGenerator(unittest.TestCase):
    r"""
    String substitution allows the following:
    %D ... directory of the active document
    %d ... name of the active document (with extension)
    %M ... user macro directory
    %j ... name of the active Job object


    The Following can be used if output is being split. If Output is not split
    these will be ignored.

    %S ... Sequence Number (default)

    Either:
    %T ... Tool Number
    %t ... Tool Controller label

    %W ... Work Coordinate System
    %O ... Operation Label

    |split on| use | Ignore |
    |-----------|-------|--------|
    |fixture | %W | %O %T %t |
    |Operation| %O | %T %t %W |
    |Tool| **Either %T or %t** | %O %W |

    The confusing bit is that for split on tool,  it will use EITHER the tool number or the tool label.
    If you include both, the second one overrides the first.
    And for split on operation, where including the tool should be possible, it ignores it altogether.

        self.job.Fixtures = ["G54"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Fixture"

    Assume:
    active document: self.assertTrue(filename, f"{home}/testdoc.fcstd
    user macro: ~/.local/share/FreeCAD/Macro
    Job:  MainJob
    Operations:
        OutsideProfile
        DrillAllHoles
    TC: 7/16" two flute  (5)
    TC: Drill (2)
    Fixtures: (G54, G55)

    Strings should be sanitized like this to ensure valid filenames
    # import re
    # filename="TC: 7/16" two flute"
    # >>> re.sub(r"[^\w\d-]","_",filename)
    # "TC__7_16__two_flute"

    """

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("TestFileNaming")
        cls.testfilename = cls.doc.Name
        cls.testfilepath = os.getcwd()
        cls.macro = FreeCAD.getUserMacroDir()

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessor = "linuxcnc"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]

        # Create a tool controller for testing tool-related substitutions
        from Path.Tool.toolbit import ToolBit

        tool_attrs = {
            "name": "TestTool",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 6.0},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=cls.doc)
        tool.Label = "6mm_Endmill"

        tc = PathToolController.Create("TC_Test_Tool", tool, 5)
        tc.Label = "TC: 6mm Endmill"
        cls.job.addObject(tc)

        # Create a simple mock operation for testing operation-related substitutions
        profile_op = cls.doc.addObject("Path::FeaturePython", "TestProfile")
        profile_op.Label = "OutsideProfile"
        # Path::FeaturePython objects already have a Path property
        profile_op.Path = Path.Path()
        cls.job.Operations.addObject(profile_op)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def test000(self):
        # Test basic name generation with empty string
        FreeCAD.setActiveDocument(self.doc.Label)
        teststring = ""
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)
        Path.Log.debug(filename)
        assertFilePathsEqual(
            self, filename, os.path.join(self.testfilepath, f"{self.testfilename}.nc")
        )

    def test010(self):
        # Substitute current file path
        teststring = "%D/testfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        print(os.path.normpath(filename))
        assertFilePathsEqual(self, filename, f"{self.testfilepath}/testfile.nc")

    def test015(self):
        # Test basic string substitution without splitting
        teststring = "~/Desktop/%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, "~/Desktop/MainJob.nc")

    def test020(self):
        teststring = "%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        expected = os.path.join(self.testfilepath, f"{self.testfilename}.nc")

        assertFilePathsEqual(self, filename, expected)

    def test030(self):
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.macro}outfile.nc")

    def test040(self):
        # unused substitution strings should be ignored
        teststring = "%d%T%t%W%O/testdoc.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.testfilename}/testdoc.nc")

    def test045(self):
        """Testing the sequence number substitution"""
        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        expected_filenames = [f"TestFileNaming{os.sep}testdoc.nc"] + [
            f"TestFileNaming{os.sep}testdoc-{i}.nc" for i in range(1, 5)
        ]
        for expected_filename in expected_filenames:
            filename = next(filename_generator)
            assertFilePathsEqual(self, filename, expected_filename)

    def test046(self):
        """Testing the sequence number substitution"""
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        expected_filenames = [
            os.path.join(self.testfilepath, f"{i}-TestFileNaming.nc") for i in range(5)
        ]
        for expected_filename in expected_filenames:
            filename = next(filename_generator)
            assertFilePathsEqual(self, filename, expected_filename)

    def test050(self):
        # explicitly using the sequence number should include it where indicated.
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "0-TestFileNaming.nc"))

    def test060(self):
        """Test subpart naming"""
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("Tool")
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.macro}outfile-Tool.nc")

    def test070(self):
        """Test %T substitution (tool number) with actual tool controller"""
        teststring = "%T.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("5")  # Tool number from our test tool controller
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "5.nc"))

    def test071(self):
        """Test %t substitution (tool description) with actual tool controller"""
        teststring = "%t.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("TC__6mm_Endmill")  # Sanitized tool label
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "TC__6mm_Endmill.nc"))

    def test072(self):
        """Test %W substitution (work coordinate system/fixture)"""
        teststring = "%W.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("G54")  # First fixture from our job setup
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "G54.nc"))

    def test073(self):
        """Test %O substitution (operation label)"""
        teststring = "%O.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("OutsideProfile")  # Operation label from our test setup
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "OutsideProfile.nc"))

    def test075(self):
        """Test path and filename substitutions together"""
        teststring = "%D/%j_%S.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        # %D should resolve to document directory (empty since doc has no filename)
        # %j should resolve to job name "MainJob"
        # %S should resolve to sequence number "0"
        assertFilePathsEqual(self, filename, os.path.join(".", "MainJob_0.nc"))

    def test076(self):
        """Test invalid substitution characters are ignored"""
        teststring = "%X%Y%Z/invalid_%Q.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        # Invalid substitutions should be removed, leaving "invalid_.nc"
        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "invalid_.nc"))

    def test080_file_extension_override(self):
        """
        Test that a file_extension override replaces the default .nc extension.

        Expected:
            file_extension = "gcode"
            PostProcessorOutputFile = ""
            -> output file ends with .gcode instead of .nc
        """
        FreeCAD.setActiveDocument(self.doc.Label)
        self.job.PostProcessorOutputFile = ""
        Path.Preferences.setOutputFileDefaults("", "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job, file_extension="gcode")
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        self.assertTrue(filename.endswith(".gcode"), f"Expected .gcode extension, got: {filename}")

    def test081_file_extension_overrides_job_extension(self):
        """
        Test that file_extension override always wins over the extension
        from PostProcessorOutputFile, since the postprocessor is the
        authoritative source for the output format.

        Expected:
            file_extension = "gcode"
            PostProcessorOutputFile = "output.tap"
            -> output file ends with .gcode (postprocessor wins)
        """
        FreeCAD.setActiveDocument(self.doc.Label)
        teststring = "output.tap"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job, file_extension="gcode")
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        self.assertTrue(
            filename.endswith(".gcode"),
            f"Expected .gcode extension (postprocessor override), got: {filename}",
        )

    def test082_no_machine_falls_back_to_nc(self):
        """
        Test that .nc is used when no machine is provided and no extension
        is specified by the user.

        Expected:
            No machine, PostProcessorOutputFile = ""
            -> output file ends with .nc
        """
        FreeCAD.setActiveDocument(self.doc.Label)
        self.job.PostProcessorOutputFile = ""
        Path.Preferences.setOutputFileDefaults("", "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        self.assertTrue(
            filename.endswith(".nc"), f"Expected .nc fallback extension, got: {filename}"
        )


class TestExport2Integration(unittest.TestCase):
    """Integration tests for the export2() function."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.newDocument("export2_test")

        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        cls.job = PathJob.Create("Export2TestJob", [box], None)
        cls.job.PostProcessor = "generic"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54"]

        # Machine property now exists by default
        cls.job.Machine = "Millstone"

        from Path.Tool.toolbit import ToolBit

        tool_attrs = {
            "name": "TestTool",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 6.0},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=cls.doc)
        tool.Label = "6mm_Endmill"

        tc = PathToolController.Create("TC_Test_Tool", tool, 1)
        tc.Label = "TC: 6mm Endmill"
        cls.job.addObject(tc)

        profile_op = cls.doc.addObject("Path::FeaturePython", "TestProfile")
        profile_op.Label = "TestProfile"
        cls._default_path = Path.Path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 100.0, "Y": 0.0, "Z": -5.0, "F": 100.0}),
                Path.Command("G1", {"X": 100.0, "Y": 100.0, "Z": -5.0}),
                Path.Command("G1", {"X": 0.0, "Y": 100.0, "Z": -5.0}),
                Path.Command("G1", {"X": 0.0, "Y": 0.0, "Z": -5.0}),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        )
        profile_op.Path = cls._default_path
        cls.job.Operations.addObject(profile_op)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def _create_machine(self, **output_options):
        """Helper to create a machine with specified output options."""
        from Machine.models.machine import Machine, OutputUnits, Toolhead, ToolheadType

        machine = Machine.create_3axis_config()
        machine.name = "TestMachine"

        # Add a default toolhead since post processor expects toolhead index 0
        default_toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            id="toolhead1",
            max_power_kw=2.2,
            max_rpm=24000,
            min_rpm=6000,
            tool_change="manual",
        )
        machine.toolheads = [default_toolhead]

        for key, value in output_options.items():
            # Handle nested output structure
            if key in ["header", "comments", "formatting", "precision", "duplicates"]:
                # Set nested attribute
                setattr(getattr(machine.output, key), key, value)
            elif key in [
                "include_date",
                "include_description",
                "include_document_name",
                "include_machine_name",
                "include_project_file",
                "include_units",
                "include_tool_list",
                "include_fixture_list",
            ]:
                # Set header nested attribute
                setattr(machine.output.header, key, value)
            elif key in [
                "enabled",
                "symbol",
                "include_operation_labels",
                "include_blank_lines",
                "output_bcnc_comments",
            ]:
                # Set comments nested attribute
                setattr(machine.output.comments, key, value)
            elif key == "comment_symbol":
                # Handle legacy test parameter name
                setattr(machine.output.comments, "symbol", value)
            elif key == "comments_enabled":
                # Handle legacy test parameter name
                setattr(machine.output.comments, "enabled", value)
            elif key in [
                "line_numbers",
                "line_number_start",
                "line_number_prefix",
                "line_increment",
                "command_space",
                "end_of_line_chars",
            ]:
                # Set formatting nested attribute
                setattr(machine.output.formatting, key, value)
            elif key in ["axis", "feed", "spindle"]:
                # Set precision nested attribute
                setattr(machine.output.precision, key, value)
            elif key == "axis_precision":
                # Handle legacy test parameter name
                setattr(machine.output.precision, "axis", value)
            elif key == "feed_precision":
                # Handle legacy test parameter name
                setattr(machine.output.precision, "feed", value)
            elif key == "spindle_precision":
                # Handle legacy test parameter name
                setattr(machine.output.precision, "spindle", value)
            elif key in ["commands", "parameters"]:
                # Set duplicates nested attribute
                setattr(machine.output.duplicates, key, value)
            elif key == "output_duplicate_commands":
                # Handle legacy test parameter name
                setattr(machine.output.duplicates, "commands", value)
            elif key == "output_duplicate_parameters":
                # Handle legacy test parameter name
                setattr(machine.output.duplicates, "parameters", value)
            elif key == "output_units":
                # Convert string output_units to enum
                if value == "metric":
                    machine.output.units = OutputUnits.METRIC
                elif value == "imperial":
                    machine.output.units = OutputUnits.IMPERIAL
            elif key == "output_header":
                # Set top-level output_header attribute
                setattr(machine.output, key, value)
            else:
                # Set top-level attribute
                setattr(machine.output, key, value)
        return machine

    def _create_postprocessor(self, machine=None, job=None):
        """Helper to create a PostProcessor with optional machine config."""
        from Path.Post.Processor import PostProcessor

        if job is None:
            job = self.job
        post = PostProcessor(job, "", "", "mm")
        if machine:
            post._machine = machine
        return post

    def _run_export2(self, machine=None, job=None):
        """Helper to run export2 and return results."""
        post = self._create_postprocessor(machine, job)
        return post.export2()

    def _get_first_section_gcode(self, results):
        """Helper to extract first section's G-code from results."""
        if results and len(results) > 0:
            return results[0][1]
        return ""

    def _get_all_gcode(self, results):
        """Helper to combine all sections into single G-code string."""
        all_output = ""
        for section_name, gcode in results:
            all_output += f"\n--- {section_name} ---\n{gcode}"
        return all_output

    def _modify_operation_path(self, commands):
        """Context manager to temporarily modify operation path."""

        class PathModifier:
            def __init__(modifier_self, test_self, commands):
                modifier_self.test_self = test_self
                modifier_self.commands = commands
                modifier_self.profile_op = None
                modifier_self.original_path = None

            def __enter__(modifier_self):
                modifier_self.profile_op = modifier_self.test_self.doc.getObject("TestProfile")
                modifier_self.original_path = modifier_self.profile_op.Path
                modifier_self.profile_op.Path = Path.Path(modifier_self.commands)
                return modifier_self

            def __exit__(modifier_self, exc_type, exc_val, exc_tb):
                modifier_self.profile_op.Path = modifier_self.original_path

        return PathModifier(self, commands)

    @staticmethod
    def _get_full_machine_config():
        """Helper to get the complete machine config used in multiple tests."""
        return {
            "freecad_version": "1.2.0",
            "machine": {
                "axes": {
                    "A": {
                        "joint": [[0, 0, 0], [0.0, 0.0, 1.0]],
                        "max": 180,
                        "max_velocity": 36000,
                        "min": -180,
                        "prefer_positive": True,
                        "sequence": 0,
                        "type": "angular",
                    },
                    "C": {
                        "joint": [[0, 0, 0], [0.0, 0.0, 1.0]],
                        "max": 180,
                        "max_velocity": 36000,
                        "min": -180,
                        "prefer_positive": True,
                        "sequence": 0,
                        "type": "angular",
                    },
                    "X": {
                        "joint": [[1.0, 0.0, 0.0], [0, 0, 0]],
                        "max": 500.0,
                        "max_velocity": 10000,
                        "min": 50.0,
                        "sequence": 0,
                        "type": "linear",
                    },
                    "Y": {
                        "joint": [[0.0, 1.0, 0.0], [0, 0, 0]],
                        "max": 1000,
                        "max_velocity": 10000,
                        "min": 0,
                        "sequence": 0,
                        "type": "linear",
                    },
                    "Z": {
                        "joint": [[0.0, 0.0, 1.0], [0, 0, 0]],
                        "max": 1000,
                        "max_velocity": 10000,
                        "min": 0,
                        "sequence": 0,
                        "type": "linear",
                    },
                },
                "description": "My linuxcnc mill",
                "manufacturer": "Supermax",
                "name": "MillStone",
                "spindles": [
                    {
                        "id": "spindle1",
                        "max_power_kw": 3.0,
                        "max_rpm": 24000,
                        "min_rpm": 6000,
                        "name": "Spindle 1",
                        "tool_change": "manual",
                        "coolant_flood": False,
                        "coolant_mist": False,
                        "coolant_delay": 0.0,
                        "spindle_wait": 0.0,
                    }
                ],
                "units": "metric",
            },
            "output": {
                "axis_precision": 3,
                "blank_lines": True,
                "command_space": " ",
                "comment_symbol": ";",
                "comments": False,
                "end_of_line_chars": "\n",
                "feed_precision": 3,
                "header": False,
                "line_increment": 10,
                "line_number_start": 100,
                "line_numbers": False,
                "list_tools_in_preamble": False,
                "machine_name": False,
                "output_bcnc_comments": True,
                "output_duplicate_parameters": True,
                "output_units": "metric",
                "path_labels": False,
                "show_operation_labels": True,
                "spindle_decimals": 0,
            },
            "postprocessor": {
                "file_name": "",
                "properties": {
                    "supports_tool_radius_compensation": False,
                    "supported_commands": "",
                    "drill_cycles_to_translate": "",
                    "preamble": "(preamble)",
                    "postamble": "(postamble)",
                    "safetyblock": "(safety)",
                    "pre_operation": "(preoperation)",
                    "post_operation": "(postoperation)",
                    "pre_tool_change": "(pretoolchange)",
                    "post_tool_change": "(posttoolchange)",
                    "pre_job": "(prejob)",
                    "post_job": "(postjob)",
                    "pre_fixture_change": "(prefixture)",
                    "post_fixture_change": "(postfixture)",
                    "pre_rotary_move": "(prerotary)",
                    "post_rotary_move": "(Postrotary)",
                    "tool_return": "(toolreturn)",
                },
            },
            "processing": {
                "early_tool_prep": False,
                "filter_inefficient_moves": False,
                "split_arcs": False,
                "tool_change": True,
                "translate_rapid_moves": False,
            },
            "version": 1,
        }

    # ===== 010-019: Basic smoke tests =====

    def test010_export2_returns_gcode_sections(self):
        """Test that export2() returns a non-empty list of (name, gcode) tuples."""
        from Path.Post.Processor import PostProcessor

        post = PostProcessor(self.job, "", "", "mm")
        results = post.export2()

        self.assertIsNotNone(results, "export2 should return results")
        self.assertIsInstance(results, list)
        self.assertGreater(len(results), 0, "Should have at least one section")

        section_name, gcode = results[0]
        self.assertIsInstance(section_name, str)
        self.assertGreater(len(gcode), 0, "First section should contain G-code")

    # ===== 020-039: _build_header tests =====

    def test020_header_enabled_shows_machine_name(self):
        """
        Test that _build_header includes machine name when header and include_machine_name are True.

        Expected:
            Output contains a comment line with the machine name.
        """
        machine = self._create_machine(
            output_header=True, include_machine_name=True, line_numbers=False
        )

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)

        header_comments = [
            line for line in gcode.split("\n") if line.startswith("(") and "Machine" in line
        ]
        self.assertGreater(len(header_comments), 0, "Header should contain machine name comment")

    def test021_header_disabled_suppresses_all_header(self):
        """
        Test that _build_header produces no header when output_header is False.

        Expected:
            No machine name, no tool list, no date in output header area.
        """
        machine = self._create_machine(
            output_header=False, include_machine_name=True, line_numbers=False
        )

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)

        header_comments = [
            line for line in gcode.split("\n") if line.startswith("(") and "Machine" in line
        ]
        self.assertEqual(
            len(header_comments), 0, "Header comments should be suppressed when header:false"
        )

    def test022_header_true_comments_false_suppresses_inline(self):
        """
        Test that header:true + comments:false shows header but suppresses inline comments.

        Expected:
            Header comments present, but inline (Test comment) is suppressed.
        """
        machine = self._create_machine(
            output_header=True, comments_enabled=False, line_numbers=False
        )

        with self._modify_operation_path(
            [
                Path.Command("(Test inline comment)"),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            self.assertNotIn(
                "Test inline comment",
                gcode,
                "Inline comments should be suppressed when comments:false",
            )

    def test023_header_false_comments_true_shows_inline(self):
        """
        Test that header:false + comments:true suppresses header but shows inline comments.

        Expected:
            No header comments, but inline (Test inline comment) is present.
        """
        machine = self._create_machine(
            output_header=False, comments_enabled=True, line_numbers=False
        )

        with self._modify_operation_path(
            [
                Path.Command("(Test inline comment)"),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)

            header_comments = [
                line for line in gcode.split("\n") if line.startswith("(") and "Machine" in line
            ]
            self.assertEqual(
                len(header_comments), 0, "Header should be suppressed when header:false"
            )
            self.assertIn(
                "Test inline comment", gcode, "Inline comments should be present when comments:true"
            )

    # ===== 040-049: _expand_canned_cycles tests =====

    def test040_canned_cycle_termination(self):
        """
        Test that _expand_canned_cycles adds G80 termination after drill cycle sequences.

        Expected:
            BEFORE: G81 X10 Y20 Z-10 R1 F100
                    G81 X10 Y30 Z-10 R1 F100

            AFTER:  G81 X10 Y20 Z-10 R1 F100
                    G81 X10 Y30 Z-10 R1 F100
                    G80
        """
        config = self._get_full_machine_config()
        machine = Machine.from_dict(config)

        cmd1 = Path.Command("G81", {"X": 10.0, "Y": 20.0, "Z": -10.0, "R": 1.0, "F": 100.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G81", {"X": 10.0, "Y": 30.0, "Z": -10.0, "R": 1.0, "F": 100.0})
        cmd2.Annotations = {"RetractMode": "G98"}
        cmd3 = Path.Command("G81", {"X": 20.0, "Y": 30.0, "Z": -10.0, "R": 1.0, "F": 100.0})
        cmd3.Annotations = {"RetractMode": "G98"}

        with self._modify_operation_path(
            [
                Path.Command("G0", {"Z": 20.0}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0}),
                Path.Command("G0", {"Z": 1.0}),
                cmd1,
                cmd2,
                cmd3,
                Path.Command("G0", {"Z": 20.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g81_lines = [line for line in lines if line.startswith("G81")]
            self.assertEqual(len(g81_lines), 3, "Should have 3 G81 drill commands")

            for g81_line in g81_lines:
                self.assertIn("Z", g81_line, f"G81 should have Z parameter: {g81_line}")
                self.assertIn("R", g81_line, f"G81 should have R parameter: {g81_line}")
                self.assertIn("F", g81_line, f"G81 should have F parameter: {g81_line}")

            g80_count = sum(1 for line in lines if line.startswith("G80"))
            self.assertEqual(g80_count, 1, "Should have exactly one G80 termination")

            g81_indices = [i for i, line in enumerate(lines) if line.startswith("G81")]
            g80_indices = [i for i, line in enumerate(lines) if line.startswith("G80")]
            self.assertGreater(
                min(g80_indices), max(g81_indices), "G80 should come after all G81 commands"
            )

    # ===== 050-059: _expand_split_arcs tests =====

    def test050_split_arcs(self):
        """
        Test that _expand_split_arcs splits arc moves into linear segments.

        Expected when split_arcs=True:
            BEFORE: G2 X10 Y0 I5 J0 F100

            AFTER:  G1 X... Y... F100  (multiple linear segments)
        """
        config = self._get_full_machine_config()
        config["processing"]["split_arcs"] = True
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 1.0}),
                Path.Command("G1", {"Z": -5.0, "F": 100.0}),
                Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "F": 100.0}),
                Path.Command("G0", {"Z": 20.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g1_lines = [line for line in lines if line.startswith("G1")]
            self.assertGreater(
                len(g1_lines), 1, "Should have multiple G1 commands from arc splitting"
            )

    # ===== 060-069: _expand_spindle_wait tests =====

    def test060_spindle_wait_injects_dwell(self):
        """
        Test that _expand_spindle_wait injects G4 pause after M3/M4 spindle start.

        Expected when spindle_wait=2.5:
            BEFORE: M3 S1000
                    G0 X10 Y20

            AFTER:  M3 S1000
                    G4 P2.5
                    G0 X10 Y20
        """
        config = self._get_full_machine_config()
        config["machine"]["spindles"][0]["spindle_wait"] = 2.5
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M3", {"S": 1000.0}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0}),
                Path.Command("G1", {"Z": -5.0, "F": 100.0}),
                Path.Command("M4", {"S": 1500.0}),
                Path.Command("G1", {"X": 20.0, "Y": 30.0, "F": 100.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g4_lines = [line for line in lines if line.startswith("G4")]
            self.assertGreaterEqual(
                len(g4_lines), 2, "Should have at least 2 G4 commands (after M3 and M4)"
            )

            for g4_line in g4_lines:
                self.assertIn("P2.5", g4_line, f"G4 command should have P2.5 parameter: {g4_line}")

            # Verify G4 appears immediately after M3 and M4
            m3_idx = next((i for i, l in enumerate(lines) if l.startswith("M3")), None)
            m4_idx = next((i for i, l in enumerate(lines) if l.startswith("M4")), None)
            g4_indices = [i for i, l in enumerate(lines) if l.startswith("G4")]

            if m3_idx is not None:
                self.assertIn(m3_idx + 1, g4_indices, "G4 should appear immediately after M3")
            if m4_idx is not None:
                self.assertIn(m4_idx + 1, g4_indices, "G4 should appear immediately after M4")

    def test061_spindle_wait_zero_no_dwell(self):
        """
        Test that no G4 is injected when spindle_wait is 0.

        Expected:
            M3 S1000
            G0 X10 Y20   (no G4 between)
        """
        config = self._get_full_machine_config()
        config["machine"]["spindles"][0]["spindle_wait"] = 0.0
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M3", {"S": 1000.0}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g4_lines = [line for line in lines if line.startswith("G4")]
            self.assertEqual(len(g4_lines), 0, "Should have no G4 commands when spindle_wait is 0")

    # ===== 070-079: _expand_coolant_delay tests =====

    def test070_coolant_delay_injects_dwell(self):
        """
        Test that _expand_coolant_delay injects G4 pause after coolant on commands.

        Expected when coolant_delay=1.5:
            BEFORE: M8
                    G1 X10 F100

            AFTER:  M8
                    G4 P1.5
                    G1 X10 F100
        """
        config = self._get_full_machine_config()
        config["machine"]["spindles"][0]["coolant_delay"] = 1.5
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M8"),
                Path.Command("G1", {"X": 10.0, "F": 100.0}),
                Path.Command("M7"),
                Path.Command("G1", {"X": 20.0, "F": 100.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g4_lines = [line for line in lines if line.startswith("G4")]
            self.assertGreater(len(g4_lines), 0, "Should have G4 pause after coolant on command")

            for g4_line in g4_lines:
                self.assertIn("P1.5", g4_line, f"G4 should have P1.5 parameter: {g4_line}")

    # ===== 080-089: _expand_translate_rapids tests =====

    def test080_translate_rapid_moves(self):
        """
        Test that _expand_translate_rapids converts G0 to G1 when enabled.

        Expected when translate_rapid_moves=True:
            BEFORE: G0 X10 Y10
                    G1 X20 Y10 F100
                    G0 Z5

            AFTER:  G1 X10 Y10
                    G1 X20 Y10 F100
                    G1 Z5
        """
        config = self._get_full_machine_config()
        config["processing"]["translate_rapid_moves"] = True
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G0", {"X": 10.0, "Y": 10.0}),
                Path.Command("G1", {"X": 20.0, "Y": 10.0, "F": 100.0}),
                Path.Command("G0", {"Z": 10.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [
                line.strip()
                for line in gcode.split("\n")
                if line.strip() and not line.strip().startswith("(")
            ]

            g0_count = sum(1 for line in lines if line.startswith("G0"))
            g1_count = sum(1 for line in lines if line.startswith("G1"))

            self.assertEqual(
                g0_count, 0, "Should have no G0 commands when rapid move translation is enabled"
            )
            self.assertGreaterEqual(
                g1_count, 4, f"Should have at least 4 G1 commands, found {g1_count}"
            )

    # ===== 090-099: _expand_xy_before_z tests =====

    def test090_xy_before_z_after_tool_change(self):
        """
        Test that _expand_xy_before_z decomposes first move after tool change.

        When xy_before_z_after_tool_change=True, a combined XYZ move after an
        M6 tool change is split into XY first, then Z, to prevent plunging
        before positioning.

        Expected:
            BEFORE: M6 T1
                    G0 X10 Y20 Z-5

            AFTER:  M6 T1
                    G0 X10 Y20
                    G0 Z-5
        """
        config = self._get_full_machine_config()
        config["processing"]["xy_before_z_after_tool_change"] = True
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M6", {"T": 1}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0, "Z": -5.0}),
                Path.Command("G1", {"X": 20.0, "Y": 30.0, "Z": -5.0, "F": 100.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [
                line.strip()
                for line in gcode.split("\n")
                if line.strip() and not line.strip().startswith("(")
            ]

            # Find G0 lines that are part of the decomposed move
            g0_lines = [line for line in lines if line.startswith("G0")]

            # Find a G0 with X or Y but no Z (the XY-only move)
            xy_only = [l for l in g0_lines if ("X" in l or "Y" in l) and "Z" not in l]
            z_only = [l for l in g0_lines if "Z" in l and "X" not in l and "Y" not in l]

            self.assertGreater(len(xy_only), 0, "Should have an XY-only G0 move")
            self.assertGreater(len(z_only), 0, "Should have a Z-only G0 move")

    # ===== 100-109: _expand_bcnc_commands tests =====

    def test100_bcnc_comments_enabled(self):
        """
        Test that _expand_bcnc_commands injects bCNC block annotations when enabled.

        Expected when output_bcnc_comments=True:
            (Block-name: TestProfile)
            (Block-expand: 0)
            (Block-enable: 1)
            G0 X0 Y0 Z5
            ...
            (Block-name: post_amble)
            (Block-expand: 0)
            (Block-enable: 1)
        """
        machine = self._create_machine(output_bcnc_comments=True, output_header=True)

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)

        self.assertIn("(Block-name:", gcode, "Should contain bCNC block name comments")
        self.assertIn("(Block-expand: 0)", gcode, "Should contain bCNC block expand comments")
        self.assertIn("(Block-enable: 1)", gcode, "Should contain bCNC block enable comments")
        self.assertIn("(Block-name: post_amble)", gcode, "Should contain bCNC postamble block")

    def test101_bcnc_comments_disabled(self):
        """
        Test that _expand_bcnc_commands removes bCNC annotations when disabled.

        Expected when output_bcnc_comments=False:
            No (Block-name:, (Block-expand:, or (Block-enable: lines.
        """
        machine = self._create_machine(output_bcnc_comments=False, output_header=True)

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)

        self.assertNotIn("(Block-name:", gcode, "Should not contain bCNC block name comments")
        self.assertNotIn("(Block-expand: 0)", gcode, "Should not contain bCNC expand comments")
        self.assertNotIn("(Block-enable: 1)", gcode, "Should not contain bCNC enable comments")

    def test102_bcnc_with_regular_comments_disabled(self):
        """
        Test that bCNC comments appear even when regular comments are disabled.

        bCNC block annotations are structural, not user comments, so they should
        be output regardless of the comments.enabled setting.
        """
        machine = self._create_machine(
            output_bcnc_comments=True, comments_enabled=False, output_header=False
        )

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)

        self.assertIn(
            "(Block-name:", gcode, "bCNC blocks should appear even when comments disabled"
        )
        self.assertIn("(Block-name: post_amble)", gcode, "bCNC postamble should appear")

    def test103_bcnc_block_structure(self):
        """
        Test that each bCNC block has the correct 3-line structure:
            (Block-name: <label>)
            (Block-expand: 0)
            (Block-enable: 1)
        """
        machine = self._create_machine(output_bcnc_comments=True, comments_enabled=True)

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)
        lines = gcode.split("\n")

        for i, line in enumerate(lines):
            if line.startswith("(Block-name:"):
                if i + 2 < len(lines):
                    self.assertEqual(
                        lines[i + 1], "(Block-expand: 0)", "Block should be followed by expand: 0"
                    )
                    self.assertEqual(
                        lines[i + 2], "(Block-enable: 1)", "Block should be followed by enable: 1"
                    )

    # ===== 110-119: _expand_tool_length_offset tests =====

    def test110_tool_length_offset_enabled(self):
        """
        Test that _expand_tool_length_offset adds G43 after M6 when enabled.

        When output_tool_length_offset=True and the operation path contains
        an M6 tool change, G43 is injected immediately after it.

        Expected:
            BEFORE: M6 T1
                    G0 X10 Y20 Z5

            AFTER:  M6 T1
                    G43 H1
                    G0 X10 Y20 Z5
        """
        config = self._get_full_machine_config()
        config["output"]["output_tool_length_offset"] = True
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M6", {"T": 1}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0, "Z": 5.0}),
                Path.Command("G1", {"X": 20.0, "Y": 30.0, "Z": -5.0, "F": 100.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g43_lines = [line for line in lines if line.startswith("G43")]
            self.assertGreater(len(g43_lines), 0, "Should have G43 tool length offset command")

            for g43_line in g43_lines:
                self.assertIn("H", g43_line, f"G43 should have H parameter: {g43_line}")

    def test111_tool_length_offset_disabled(self):
        """
        Test that no G43 commands appear when tool length offset is disabled.

        When output_tool_length_offset=False, M6 in the operation path should
        NOT be followed by G43.

        Expected:
            BEFORE: M6 T1
                    G0 X10 Y20 Z5

            AFTER:  M6 T1
                    G0 X10 Y20 Z5
                    (no G43)
        """
        config = self._get_full_machine_config()
        config["output"]["output_tool_length_offset"] = False
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("M6", {"T": 1}),
                Path.Command("G0", {"X": 10.0, "Y": 20.0, "Z": 5.0}),
                Path.Command("G1", {"X": 20.0, "Y": 30.0, "Z": -5.0, "F": 100.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [line.strip() for line in gcode.split("\n") if line.strip()]

            g43_lines = [line for line in lines if line.startswith("G43")]
            self.assertEqual(
                len(g43_lines), 0, "Should have no G43 commands when tool length offset is disabled"
            )

    # ===== 120-139: Output formatting tests =====

    def test120_line_numbers_exclude_header(self):
        """
        Test that line numbers are applied to G-code body but not header comments.

        Expected:
            (Machine: TestMachine)     <- no line number
            N100 G21                   <- numbered
            N110 G0 X0 Y0 Z5          <- numbered
        """
        machine = self._create_machine(
            output_header=True,
            comments_enabled=False,
            line_numbers=True,
            line_number_start=100,
            line_increment=10,
        )

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)
        lines = gcode.split("\n")

        for line in lines:
            if line.strip().startswith("("):
                self.assertFalse(
                    line.strip().startswith("N"),
                    f"Header comment should not have line number: {line}",
                )

        numbered_lines = [line for line in lines if line.strip().startswith("N")]
        self.assertGreater(len(numbered_lines), 0, "G-code lines should have line numbers")
        self.assertTrue(
            numbered_lines[0].strip().startswith("N100"),
            f"First line number should be N100, got: {numbered_lines[0].strip()}",
        )

    def test121_line_numbers_start_and_increment(self):
        """
        Test that line_number_start and line_increment are respected.

        Expected with start=50, increment=5:
            N50 ...
            N55 ...
            N60 ...
        """
        machine = self._create_machine(line_numbers=True, line_number_start=50, line_increment=5)

        results = self._run_export2(machine)
        gcode = self._get_first_section_gcode(results)
        numbered_lines = [
            line.strip() for line in gcode.split("\n") if line.strip().startswith("N")
        ]

        self.assertGreaterEqual(len(numbered_lines), 2, "Should have at least 2 numbered lines")
        self.assertTrue(
            numbered_lines[0].startswith("N50"),
            f"First line should be N50, got: {numbered_lines[0]}",
        )
        self.assertTrue(
            numbered_lines[1].startswith("N55"),
            f"Second line should be N55, got: {numbered_lines[1]}",
        )

    def test122_precision_from_config(self):
        """
        Test that axis_precision and feed_precision control decimal places.

        Expected with axis_precision=4, feed_precision=1:
            G0 X10.1235 Y20.9876 Z5.5000
            G1 X100.1235 ... F6007.4
        """
        machine = self._create_machine(axis_precision=4, feed_precision=1, line_numbers=False)

        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 10.12345, "Y": 20.98765, "Z": 5.5}),
                Path.Command("G1", {"X": 100.123456, "Y": 0.0, "Z": -5.0, "F": 100.123}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)

            self.assertIn("X10.1235", gcode, "X should have 4 decimal places (rounded)")
            self.assertIn("Y20.9876", gcode, "Y should have 4 decimal places (rounded)")
            self.assertIn("Z5.5000", gcode, "Z should have 4 decimal places")
            self.assertIn("F6007.4", gcode, "Feed should have 1 decimal place")

    def test123_spindle_decimals(self):
        """
        Test that spindle_decimals controls decimal places for spindle speed.

        Expected with spindle_decimals=0:
            M3 S1235   (rounded, no decimals)
        """
        machine = self._create_machine(spindle_decimals=0, output_header=False, line_numbers=False)

        with self._modify_operation_path(
            [
                Path.Command("M3", {"S": 1234.567}),
                Path.Command("G0", {"X": 10.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            self.assertIn("S1235", gcode, "Should have 0 decimal places for spindle speed")

    def test124_comment_symbol_formatting(self):
        """
        Test that comment_symbol controls comment format.

        Expected with comment_symbol='(':  (Test comment)
        Expected with comment_symbol=';':  ; Test comment
        """
        with self._modify_operation_path(
            [
                Path.Command("(Test comment)"),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        ):
            machine_paren = self._create_machine(
                comment_symbol="(", comments_enabled=True, output_header=False, line_numbers=False
            )
            results1 = self._run_export2(machine_paren)
            gcode1 = self._get_first_section_gcode(results1)
            self.assertIn(
                "(Test comment)", gcode1, "Comments should use parentheses when comment_symbol='('"
            )

            machine_semi = self._create_machine(
                comment_symbol=";", comments_enabled=True, output_header=False, line_numbers=False
            )
            results2 = self._run_export2(machine_semi)
            gcode2 = self._get_first_section_gcode(results2)
            self.assertIn(
                "; Test comment", gcode2, "Comments should use semicolon when comment_symbol=';'"
            )

    def test125_command_space_option(self):
        """
        Test that command_space controls spacing between command and parameters.

        Expected with command_space=" ":  G0 X10.000
        Expected with command_space="":   G0X10.000
        """
        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 10.0, "Y": 20.0}),
            ]
        ):
            machine_space = self._create_machine(
                command_space=" ", output_header=False, line_numbers=False
            )
            gcode_space = self._get_first_section_gcode(self._run_export2(machine_space))
            self.assertIn("G0 X", gcode_space, "Should have space between command and parameter")

            machine_no_space = self._create_machine(
                command_space="", output_header=False, line_numbers=False
            )
            gcode_no_space = self._get_first_section_gcode(self._run_export2(machine_no_space))
            self.assertIn(
                "G0X", gcode_no_space, "Should have no space between command and parameter"
            )

    def test126_end_of_line_chars(self):
        """
        Test that end_of_line_chars controls line ending characters.

        Expected with "\\r\\n": CRLF line endings
        Expected with "\\n": LF-only line endings
        """
        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 10.0}),
                Path.Command("G1", {"Y": 20.0}),
            ]
        ):
            machine_crlf = self._create_machine(
                end_of_line_chars="\r\n", output_header=False, line_numbers=False
            )
            gcode_crlf = self._get_first_section_gcode(self._run_export2(machine_crlf))
            self.assertIn("\r\n", gcode_crlf, "Should have CRLF line endings")

            machine_lf = self._create_machine(
                end_of_line_chars="\n", output_header=False, line_numbers=False
            )
            gcode_lf = self._get_first_section_gcode(self._run_export2(machine_lf))
            self.assertNotIn("\r", gcode_lf, "Should have LF-only line endings")

    def test127_output_units(self):
        """
        Test that output_units inserts G20 (imperial) or G21 (metric).

        Expected: G21 for metric, G20 for imperial.
        """
        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 25.4, "Y": 50.8}),
            ]
        ):
            machine_metric = self._create_machine(
                output_units="metric", output_header=False, line_numbers=False
            )
            gcode_metric = self._get_first_section_gcode(self._run_export2(machine_metric))
            self.assertIn("G21", gcode_metric, "Metric output should contain G21")

            machine_imperial = self._create_machine(
                output_units="imperial", output_header=False, line_numbers=False
            )
            gcode_imperial = self._get_first_section_gcode(self._run_export2(machine_imperial))
            self.assertIn("G20", gcode_imperial, "Imperial output should contain G20")

    def test128_duplicate_parameters_suppressed(self):
        """
        Test that duplicate parameters are suppressed when duplicates.parameters=False.

        Expected:
            BEFORE: G0 X10 Y10 Z5 F60000
                    G1 X20 Y10 Z5 F60000

            AFTER:  G0 X10 Y10 Z5 F60000
                    G1 X20              (Y, Z, F suppressed — unchanged)
        """
        machine = self._create_machine(
            parameters=False, line_numbers=False, comments_enabled=False, output_header=False
        )

        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 10.0, "Y": 10.0, "Z": 5.0, "F": 1000.0}),
                Path.Command("G1", {"X": 20.0, "Y": 10.0, "Z": 5.0, "F": 1000.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_first_section_gcode(results)
            lines = [
                l.strip() for l in gcode.split("\n") if l.strip() and not l.strip().startswith("(")
            ]

            second_commands = [l for l in lines if "G1" in l and "X20" in l]
            self.assertGreater(len(second_commands), 0, "Should have G1 with X20")
            self.assertNotIn("Y", second_commands[0], "Y should be suppressed (unchanged)")
            self.assertNotIn("Z", second_commands[0], "Z should be suppressed (unchanged)")

    def test129_duplicate_commands_suppressed(self):
        """
        Test that duplicate G-code commands are suppressed when duplicates.commands=False.

        Expected:
            BEFORE: G1 X10 Y10
                    G1 X20 Y10
                    G1 X20 Y20

            AFTER:  G1 X10 Y10
                    X20 Y10      (G1 suppressed — same as previous)
                    X20 Y20
        """
        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 10.0, "Y": 10.0, "Z": -5.0, "F": 100.0}),
                Path.Command("G1", {"X": 20.0, "Y": 10.0, "Z": -5.0}),
                Path.Command("G1", {"X": 20.0, "Y": 20.0, "Z": -5.0}),
                Path.Command("G1", {"X": 10.0, "Y": 20.0, "Z": -5.0}),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        ):
            machine = self._create_machine(commands=False, parameters=True)
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [
                l.strip() for l in gcode.split("\n") if l.strip() and not l.strip().startswith("(")
            ]

            g1_count = sum(1 for l in lines if l.startswith("G1"))
            suppressed_count = sum(
                1 for l in lines if l and l[0] in "XYZ" and not l.startswith("G")
            )

            self.assertEqual(
                g1_count,
                1,
                f"Should have only 1 G1 command when duplicates suppressed, found {g1_count}",
            )
            self.assertEqual(
                suppressed_count, 3, f"Should have 3 suppressed G1 moves, found {suppressed_count}"
            )

    # ===== 140-149: G-code blocks insertion tests =====

    def test140_gcode_blocks_insertion(self):
        """
        Test that all G-code blocks from machine config are properly inserted.

        Expected: safety, preamble, prejob, preoperation, postoperation,
                  postjob, and postamble all appear in output.
        """
        config = self._get_full_machine_config()
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("G0", {"X": 10.0, "Y": 10.0, "Z": 5.0, "F": 1000.0}),
                Path.Command("G1", {"X": 20.0, "Y": 10.0, "Z": 5.0, "F": 1000.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)

            self.assertIn("(safety)", gcode, "Safetyblock should appear in output")
            self.assertIn("(preamble)", gcode, "Preamble should appear in output")
            self.assertIn("(prejob)", gcode, "Pre-job should appear in output")
            self.assertIn("(preoperation)", gcode, "Pre-operation should appear in output")
            self.assertIn("(postoperation)", gcode, "Post-operation should appear in output")
            self.assertIn("(postjob)", gcode, "Post-job should appear in output")
            self.assertIn("(postamble)", gcode, "Postamble should appear in output")

    def test141_rotary_blocks_insertion(self):
        """
        Test that pre/post rotary blocks are inserted around rotary axis moves.

        Expected: 2 rotary groups → 2 pre-rotary + 2 post-rotary blocks.
        """
        config = self._get_full_machine_config()
        machine = Machine.from_dict(config)

        with self._modify_operation_path(
            [
                Path.Command("G1", {"X": 10.0}),
                Path.Command("G0", {"A": 45.0}),
                Path.Command("G0", {"C": 90.0, "Y": 11.0}),
                Path.Command("G1", {"Y": 10.0}),
                Path.Command("G0", {"B": 30.0}),
                Path.Command("G1", {"X": 20.0}),
            ]
        ):
            results = self._run_export2(machine)
            gcode = "\n".join(g for _, g in results)

            self.assertIn("(prerotary)", gcode, "Pre-rotary block should appear")
            self.assertIn("(Postrotary)", gcode, "Post-rotary block should appear")

            self.assertEqual(
                gcode.count("(prerotary)"),
                2,
                "Should have 2 pre-rotary blocks (one per rotary group)",
            )
            self.assertEqual(
                gcode.count("(Postrotary)"),
                2,
                "Should have 2 post-rotary blocks (one per rotary group)",
            )

    def test142_fixture_change_blocks_insertion(self):
        """
        Test that pre/post fixture change blocks are inserted when fixtures change.

        Expected with fixtures=[G54, G55]:
            (prefixture) before each fixture, (postfixture) after.
        """
        config = self._get_full_machine_config()
        machine = Machine.from_dict(config)

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = False

        try:
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)

            self.assertIn("(prefixture)", gcode, "Pre-fixture block should appear")
            self.assertIn("(postfixture)", gcode, "Post-fixture block should appear")

            self.assertGreaterEqual(
                gcode.count("(prefixture)"), 1, "Should have at least 1 pre-fixture block"
            )
            self.assertGreaterEqual(
                gcode.count("(postfixture)"), 1, "Should have at least 1 post-fixture block"
            )
        finally:
            self.job.Fixtures = ["G54"]

    # ===== 150-159: Postprocessor properties and misc tests =====

    def test150_tool_radius_compensation_property(self):
        """
        Test that postprocessors declare support for G41/G42 tool radius compensation.

        Expected: LinuxCNC schema has supports_tool_radius_compensation=True.
        """
        linuxcnc_post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        schema = linuxcnc_post.get_full_property_schema()

        trc_property = next(
            (p for p in schema if p.get("name") == "supports_tool_radius_compensation"), None
        )
        self.assertIsNotNone(trc_property, "Property should exist in schema")
        self.assertEqual(trc_property.get("type"), "bool", "Should be boolean type")
        self.assertTrue(trc_property.get("default"), "LinuxCNC should support G41/G42 by default")

    def test151_supported_commands_property(self):
        """
        Test that postprocessors declare their supported G-code command list.

        Expected: Generic post has a substantial list including G0, G1, G2, G3, M3, M6.
        """
        generic_post = PostProcessorFactory.get_post_processor(self.job, "generic")
        schema = generic_post.get_full_property_schema()

        commands_property = next((p for p in schema if p.get("name") == "supported_commands"), None)
        self.assertIsNotNone(commands_property, "Property should exist in schema")
        self.assertEqual(commands_property.get("type"), "text", "Should be text type")

        command_list = [
            c.strip() for c in commands_property.get("default", "").split("\n") if c.strip()
        ]
        for cmd in ["G0", "G1", "G2", "G3", "M3", "M6"]:
            self.assertIn(cmd, command_list, f"Should include {cmd}")
        self.assertGreater(len(command_list), 20, "Should have substantial command list")

    def test152_nested_parentheses_in_comments(self):
        """
        Test that nested parentheses in comments are sanitized.

        Path.Command truncates at first ) after (, so nested parens get mangled.
        The post processor must ensure output has balanced parentheses.

        Expected:
            INPUT:  (Begin operation: Pocket (fast))
            OUTPUT: (Begin operation: Pocket fast])  <- balanced, ] replaces nested )
        """
        with self._modify_operation_path(
            [
                Path.Command("(Begin operation: Test Operation (fast))"),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("(Tool: 5mm Endmill (HSS))"),
                Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 100.0}),
            ]
        ):
            machine = self._create_machine(
                comments_enabled=True, output_header=False, line_numbers=False
            )
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [l.strip() for l in gcode.split("\n") if l.strip()]

            for line in lines:
                if line.startswith("("):
                    self.assertEqual(
                        line.count("("), 1, f"Should have exactly 1 opening paren in: {line}"
                    )
                    self.assertEqual(
                        line.count(")"), 1, f"Should have exactly 1 closing paren in: {line}"
                    )

    def test153_drill_cycles_to_translate_property(self):
        """
        Test that drill_cycles_to_translate postprocessor property is stored correctly.

        Expected: Property value round-trips through Machine.from_dict.
        """
        config = self._get_full_machine_config()
        config["postprocessor"]["properties"]["drill_cycles_to_translate"] = "G81\nG82\nG83"
        machine = Machine.from_dict(config)

        self.assertIn("drill_cycles_to_translate", machine.postprocessor_properties)
        self.assertEqual(
            machine.postprocessor_properties["drill_cycles_to_translate"], "G81\nG82\nG83"
        )
