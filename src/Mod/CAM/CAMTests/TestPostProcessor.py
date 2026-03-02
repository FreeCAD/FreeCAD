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
from unittest.mock import patch
import FreeCAD
import Path
import Path.Post.Command as PathCommand
import Path.Main.Job as PathJob
import unittest
from Path.Post.Processor import _HeaderBuilder

PathCommand.LOG_MODULE = Path.Log.thisModule()
Path.Log.setLevel(Path.Log.Level.INFO, PathCommand.LOG_MODULE)


class TestResolvingPostProcessorName(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("boxtest")

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "")

    def tearDown(self):
        pass

    def test010(self):
        # Test if post is defined in job
        self.job.PostProcessor = "linuxcnc"
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "linuxcnc")

    def test020(self):
        # Test if post is invalid
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=False):
            with self.assertRaises(ValueError):
                PathCommand._resolve_post_processor_name(self.job)

    def test030(self):
        # Test if post is defined in prefs
        self.job.PostProcessor = ""
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "grbl")

        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "grbl")

    def test040(self):
        # Test if user interaction is correctly handled
        if FreeCAD.GuiUp:
            with patch("Path.Post.Command.DlgSelectPostProcessor") as mock_dlg, patch(
                "Path.Post.Processor.PostProcessor.exists", return_value=True
            ):
                mock_dlg.return_value.exec_.return_value = "generic"
                postname = PathCommand._resolve_post_processor_name(self.job)
                self.assertEqual(postname, "generic")
        else:
            with patch.object(self.job, "PostProcessor", ""):
                with self.assertRaises(ValueError):
                    PathCommand._resolve_post_processor_name(self.job)


class TestPostProcessorFactory(unittest.TestCase):
    """Test creation of postprocessor objects."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("boxtest")

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

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test020(self):
        # test creation of postprocessor object
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        self.assertIsNotNone(post)
        self.assertTrue(hasattr(post, "export"))
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test030(self):
        # test wrapping of old school postprocessor scripts
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        self.assertIsNotNone(post)
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test040(self):
        """Test that the __name__ of the postprocessor is correct."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        # Refactored post processors don't have script_module, they are the module
        if hasattr(post, "script_module"):
            self.assertEqual(post.script_module.__name__, "linuxcnc_post")
        else:
            # For refactored posts, check the class module name
            self.assertEqual(post.__class__.__module__, "linuxcnc_post")


class TestHeaderBuilder(unittest.TestCase):
    """Test the HeaderBuilder class."""

    def test010_initialization(self):
        """Test that HeaderBuilder initializes with empty data structures."""

        builder = _HeaderBuilder()

        # Check initial state
        self.assertIsNone(builder._exporter)
        self.assertIsNone(builder._post_processor)
        self.assertIsNone(builder._cam_file)
        self.assertIsNone(builder._project_file)
        self.assertIsNone(builder._output_units)
        self.assertIsNone(builder._document_name)
        self.assertIsNone(builder._description)
        self.assertIsNone(builder._author)
        self.assertIsNone(builder._output_time)
        self.assertEqual(builder._tools, [])
        self.assertEqual(builder._fixtures, [])
        self.assertEqual(builder._notes, [])

    def test020_add_methods(self):
        """Test adding header elements."""

        builder = _HeaderBuilder()

        # Add various elements
        builder.add_exporter_info("TestExporter")
        builder.add_machine_info("TestMachine")
        builder.add_post_processor("test_post")
        builder.add_cam_file("test.fcstd")
        builder.add_project_file("/path/to/project.FCStd")
        builder.add_output_units("Metric - mm")
        builder.add_document_name("TestDocument")
        builder.add_description("Test job description")
        builder.add_author("Test Author")
        builder.add_output_time("2024-12-24 10:00:00")
        builder.add_tool(1, "End Mill")
        builder.add_tool(2, "Drill Bit")
        builder.add_fixture("G54")
        builder.add_fixture("G55")
        builder.add_note("This is a test note")

        # Verify elements were added
        self.assertEqual(builder._exporter, "TestExporter")
        self.assertEqual(builder._machine, "TestMachine")
        self.assertEqual(builder._post_processor, "test_post")
        self.assertEqual(builder._cam_file, "test.fcstd")
        self.assertEqual(builder._project_file, "/path/to/project.FCStd")
        self.assertEqual(builder._output_units, "Metric - mm")
        self.assertEqual(builder._document_name, "TestDocument")
        self.assertEqual(builder._description, "Test job description")
        self.assertEqual(builder._author, "Test Author")
        self.assertEqual(builder._output_time, "2024-12-24 10:00:00")
        self.assertEqual(builder._tools, [(1, "End Mill"), (2, "Drill Bit")])
        self.assertEqual(builder._fixtures, ["G54", "G55"])
        self.assertEqual(builder._notes, ["This is a test note"])

    def test030_path_property_empty(self):
        """Test Path property with no data returns empty Path."""

        builder = _HeaderBuilder()
        path = builder.Path

        self.assertIsInstance(path, Path.Path)
        self.assertEqual(len(path.Commands), 0)

    def test040_path_property_complete(self):
        """Test Path property generates correct comment commands."""

        builder = _HeaderBuilder()

        # Add complete header data
        builder.add_exporter_info("FreeCAD")
        builder.add_machine_info("CNC Router")
        builder.add_post_processor("linuxcnc")
        builder.add_cam_file("project.fcstd")
        builder.add_project_file("/home/user/myproject.FCStd")
        builder.add_output_units("Metric - mm")
        builder.add_document_name("MyProject")
        builder.add_description("CNC milling project")
        builder.add_author("John Doe")
        builder.add_output_time("2024-12-24 10:00:00")
        builder.add_tool(1, '1/4" End Mill')
        builder.add_fixture("G54")
        builder.add_note("Test operation")

        path = builder.Path

        # Verify it's a Path object
        self.assertIsInstance(path, Path.Path)

        # Check expected number of commands
        expected_commands = [
            "(Exported by FreeCAD)",
            "(Machine: CNC Router)",
            "(Post Processor: linuxcnc)",
            "(Cam File: project.fcstd)",
            "(Project File: /home/user/myproject.FCStd)",
            "(Output Units: Metric - mm)",
            "(Document: MyProject)",
            "(Description: CNC milling project)",
            "(Author: John Doe)",
            "(Output Time: 2024-12-24 10:00:00)",
            '(T1=1/4" End Mill)',
            "(Fixture: G54)",
            "(Note: Test operation)",
        ]

        self.assertEqual(len(path.Commands), len(expected_commands))

        # Verify each command
        for i, expected_comment in enumerate(expected_commands):
            self.assertIsInstance(path.Commands[i], Path.Command)
            self.assertEqual(path.Commands[i].Name, expected_comment)

    def test050_path_property_partial(self):
        """Test Path property with partial data."""

        builder = _HeaderBuilder()

        # Add only some elements
        builder.add_exporter_info()
        builder.add_tool(5, "Drill")
        builder.add_note("Partial test")

        path = builder.Path

        expected_commands = ["(Exported by FreeCAD)", "(T5=Drill)", "(Note: Partial test)"]

        self.assertEqual(len(path.Commands), len(expected_commands))
        for i, expected_comment in enumerate(expected_commands):
            self.assertEqual(path.Commands[i].Name, expected_comment)

        # converted
        expected_gcode = "(Exported by FreeCAD)\n(T5=Drill)\n(Note: Partial test)\n"
        gcode = path.toGCode()
        self.assertEqual(gcode, expected_gcode)

    def test060_multiple_tools_fixtures_notes(self):
        """Test adding multiple tools, fixtures, and notes."""

        builder = _HeaderBuilder()

        # Add multiple items
        builder.add_tool(1, "Tool A")
        builder.add_tool(2, "Tool B")
        builder.add_tool(3, "Tool C")

        builder.add_fixture("G54")
        builder.add_fixture("G55")
        builder.add_fixture("G56")

        builder.add_note("Note 1")
        builder.add_note("Note 2")

        path = builder.Path

        # Should have 8 commands (3 tools + 3 fixtures + 2 notes)
        self.assertEqual(len(path.Commands), 8)

        # Check tool commands
        self.assertEqual(path.Commands[0].Name, "(T1=Tool A)")
        self.assertEqual(path.Commands[1].Name, "(T2=Tool B)")
        self.assertEqual(path.Commands[2].Name, "(T3=Tool C)")

        # Check fixture commands
        self.assertEqual(path.Commands[3].Name, "(Fixture: G54)")
        self.assertEqual(path.Commands[4].Name, "(Fixture: G55)")
        self.assertEqual(path.Commands[5].Name, "(Fixture: G56)")

        # Check note commands
        self.assertEqual(path.Commands[6].Name, "(Note: Note 1)")
        self.assertEqual(path.Commands[7].Name, "(Note: Note 2)")
