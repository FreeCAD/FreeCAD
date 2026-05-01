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
from unittest.mock import patch, Mock
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
        self.job.PostProcessor = "linuxcnc_legacy"
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "linuxcnc_legacy")

    def test020(self):
        # Test if post is invalid
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=False):
            with self.assertRaises(ValueError):
                PathCommand._resolve_post_processor_name(self.job)

    def test030(self):
        # Test if post is defined in prefs
        self.job.PostProcessor = ""
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "grbl_legacy")

        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "grbl_legacy")

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
        cls.job.PostProcessor = "linuxcnc_legacy"
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
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        self.assertIsNotNone(post)
        self.assertTrue(hasattr(post, "export"))
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test030(self):
        # test wrapping of old school postprocessor scripts
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        self.assertIsNotNone(post)
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test040(self):
        """Test that the __name__ of the postprocessor is correct."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        # Refactored post processors don't have script_module, they are the module
        if hasattr(post, "script_module"):
            self.assertEqual(post.script_module.__name__, "linuxcnc_legacy_post")
        else:
            # For refactored posts, check the class module name
            self.assertEqual(post.__class__.__module__, "linuxcnc_legacy_post")


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


class TestPostProcessorClassification(unittest.TestCase):
    """Test the POST_TYPE-based postprocessor classification system."""

    def setUp(self):
        # Clear the classification cache before each test
        import Path.Preferences

        Path.Preferences._post_type_cache = {}
        Path.Preferences._post_type_cache_keys = None

    def test010_classify_machine_post(self):
        """New-style posts with POST_TYPE = 'machine' are classified as 'machine'."""
        import Path.Preferences

        machine_posts = [
            "generic",
            "linuxcnc",
            "grbl",
            "centroid",
            "mach3_mach4",
            "opensbp",
            "generic_plasma",
            "smoothie",
            "masso_g3",
        ]
        for post in machine_posts:
            result = Path.Preferences.classifyPostProcessor(post)
            self.assertEqual(result, "machine", f"Expected 'machine' for {post}, got '{result}'")

    def test020_classify_legacy_post(self):
        """Legacy posts without POST_TYPE are classified as 'legacy'."""
        import Path.Preferences

        legacy_posts = ["linuxcnc_legacy", "grbl_legacy", "test"]
        available = Path.Preferences.allAvailablePostProcessors()
        for post in legacy_posts:
            if post in available:
                result = Path.Preferences.classifyPostProcessor(post)
                self.assertEqual(result, "legacy", f"Expected 'legacy' for {post}, got '{result}'")

    def test030_classify_nonexistent_post(self):
        """A nonexistent postprocessor is classified as 'unknown'."""
        import Path.Preferences

        result = Path.Preferences.classifyPostProcessor("nonexistent_xyz_post_that_does_not_exist")
        self.assertEqual(result, "unknown")

    def test040_legacy_list_excludes_machine(self):
        """allAvailableLegacyPostProcessors excludes machine-type posts."""
        import Path.Preferences

        legacy = Path.Preferences.allAvailableLegacyPostProcessors()
        machine = Path.Preferences.allAvailableMachinePostProcessors()

        # No overlap
        overlap = set(legacy) & set(machine)
        self.assertEqual(overlap, set(), f"Unexpected overlap: {overlap}")

        # Machine posts should not appear in legacy list
        for post in ["generic", "linuxcnc", "grbl"]:
            self.assertNotIn(post, legacy, f"Machine post '{post}' found in legacy list")

    def test050_machine_list_excludes_legacy(self):
        """allAvailableMachinePostProcessors excludes legacy-type posts."""
        import Path.Preferences

        machine = Path.Preferences.allAvailableMachinePostProcessors()

        # Legacy posts should not appear in machine list
        available = Path.Preferences.allAvailablePostProcessors()
        for post in available:
            if "_legacy" in post:
                self.assertNotIn(post, machine, f"Legacy post '{post}' found in machine list")

    def test060_all_posts_accounted_for(self):
        """Every available post is classified as either 'machine', 'legacy', or 'unknown'."""
        import Path.Preferences

        all_posts = Path.Preferences.allAvailablePostProcessors()
        for post in all_posts:
            result = Path.Preferences.classifyPostProcessor(post)
            self.assertIn(
                result,
                ["machine", "legacy", "unknown"],
                f"Unexpected classification '{result}' for {post}",
            )

    def test070_cache_invalidation(self):
        """Cache invalidates when available post list changes."""
        import Path.Preferences

        # Prime the cache
        Path.Preferences.classifyPostProcessor("generic")
        self.assertIn("generic", Path.Preferences._post_type_cache)

        # Simulate a change in available posts by modifying the cache key
        Path.Preferences._post_type_cache_keys = ("fake_post",)

        # Next call should rebuild the cache
        result = Path.Preferences.classifyPostProcessor("generic")
        self.assertEqual(result, "machine")

    def test080_postprocessor_sanity_checks_hook(self):
        """Test PostProcessor.get_sanity_checks() hook method."""
        from Path.Post.Processor import PostProcessor

        # Create a test postprocessor instance
        class TestPostProcessor(PostProcessor):
            def __init__(self):
                # Don't call super().__init__ to avoid complex setup
                self.values = {}

            def get_sanity_checks(self, job):
                return [
                    self._create_squawk("WARNING", "Test warning"),
                    self._create_squawk("NOTE", "Test note"),
                ]

        processor = TestPostProcessor()

        # Test the hook method
        mock_job = Mock()
        squawks = processor.get_sanity_checks(mock_job)

        self.assertEqual(len(squawks), 2)
        self.assertEqual(squawks[0]["squawkType"], "WARNING")
        self.assertEqual(squawks[0]["Note"], "Test warning")
        self.assertEqual(squawks[0]["Operator"], "TestPostProcessor")
        self.assertEqual(squawks[1]["squawkType"], "NOTE")
        self.assertEqual(squawks[1]["Note"], "Test note")

    def test081_postprocessor_create_squawk_helper(self):
        """Test PostProcessor._create_squawk() helper method."""
        from Path.Post.Processor import PostProcessor

        class TestPostProcessor(PostProcessor):
            def __init__(self):
                pass

        processor = TestPostProcessor()

        # Test squawk creation
        squawk = processor._create_squawk("WARNING", "Test message")

        # Verify structure
        self.assertIn("Date", squawk)
        self.assertIn("Operator", squawk)
        self.assertIn("Note", squawk)
        self.assertIn("squawkType", squawk)
        self.assertIn("squawkIcon", squawk)

        # Verify values
        self.assertEqual(squawk["squawkType"], "WARNING")
        self.assertEqual(squawk["Note"], "Test message")
        self.assertEqual(squawk["Operator"], "TestPostProcessor")
        self.assertTrue(squawk["squawkIcon"].endswith(".svg"))

        # Test different squawk types
        for squawk_type in ["NOTE", "WARNING", "CAUTION", "TIP"]:
            squawk = processor._create_squawk(squawk_type, f"Test {squawk_type}")
            self.assertEqual(squawk["squawkType"], squawk_type)

    def test082_postprocessor_default_sanity_checks(self):
        """Test PostProcessor default get_sanity_checks() returns empty list."""
        from Path.Post.Processor import PostProcessor

        class TestPostProcessor(PostProcessor):
            def __init__(self):
                pass

        processor = TestPostProcessor()
        mock_job = Mock()

        # Default implementation should return empty list
        squawks = processor.get_sanity_checks(mock_job)
        self.assertEqual(squawks, [])


class TestConfigurationBundle(unittest.TestCase):
    """Tests for build_configuration_bundle() and apply_configuration_bundle().

    build_configuration_bundle() is a pure function that returns a flat dict.
    apply_configuration_bundle() applies the bundle to self.values as UPPERCASE.
    """

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _make_postprocessor(self, machine_props=None, job_overrides=None, schema=None):
        """Create a minimal PostProcessor with controllable inputs.

        Args:
            machine_props: dict for machine.postprocessor_properties
            job_overrides: dict serialised as JSON on the mock job
            schema: list of schema dicts; if None a small default is used
        """
        from Path.Post.Processor import PostProcessor

        test_schema = schema

        class BundleTestPP(PostProcessor):
            def __init__(self):
                # Skip super().__init__() — we only need the bundle methods
                self.values = {}
                self._machine = None
                self._job = None

            @classmethod
            def get_property_schema(cls):
                if test_schema is not None:
                    return test_schema
                return [
                    {"name": "blend_mode", "type": "choice", "default": "BLEND"},
                    {"name": "blend_tolerance", "type": "float", "default": 0.0},
                ]

        pp = BundleTestPP()

        # Mock machine
        if machine_props is not None:
            pp._machine = Mock()
            pp._machine.postprocessor_properties = dict(machine_props)
            pp._machine.output = None  # no output config

        # Mock job
        if job_overrides is not None:
            import json

            pp._job = Mock()
            pp._job.PostProcessorPropertyOverrides = json.dumps(job_overrides)
        else:
            pp._job = Mock()
            pp._job.PostProcessorPropertyOverrides = "{}"

        return pp

    # ------------------------------------------------------------------
    # build_configuration_bundle — pure function tests
    # ------------------------------------------------------------------

    def test100_build_bundle_no_machine(self):
        """build_configuration_bundle with no machine returns schema defaults."""
        pp = self._make_postprocessor()
        bundle = pp.build_configuration_bundle()

        # Common schema keys (from base class) + our two specific keys
        self.assertIn("blend_mode", bundle)
        self.assertIn("blend_tolerance", bundle)
        self.assertEqual(bundle["blend_mode"], "BLEND")
        self.assertEqual(bundle["blend_tolerance"], 0.0)
        # Common property from base schema
        self.assertIn("preamble", bundle)

    def test110_build_bundle_machine_props_take_priority_over_schema(self):
        """Machine postprocessor_properties override schema defaults."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "EXACT_PATH"}
        )
        bundle = pp.build_configuration_bundle()

        self.assertEqual(bundle["blend_tolerance"], 0.05)
        self.assertEqual(bundle["blend_mode"], "EXACT_PATH")
        # Schema-only key still present via defaults
        self.assertIn("preamble", bundle)

    def test120_build_bundle_schema_fills_missing_keys(self):
        """Schema defaults fill keys absent from machine props."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.02}
            # blend_mode NOT in machine props
        )
        bundle = pp.build_configuration_bundle()

        self.assertEqual(bundle["blend_tolerance"], 0.02)  # from machine
        self.assertEqual(bundle["blend_mode"], "BLEND")  # from schema default

    def test130_build_bundle_job_overrides_win(self):
        """Job overrides take priority over machine props and schema defaults."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
            job_overrides={"blend_tolerance": 0.018},
        )
        bundle = pp.build_configuration_bundle()

        self.assertEqual(bundle["blend_tolerance"], 0.018)  # job override wins
        self.assertEqual(bundle["blend_mode"], "BLEND")  # untouched

    def test140_build_bundle_explicit_overrides_win_over_job(self):
        """Explicit overrides dict beats job-stored overrides."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
            job_overrides={"blend_tolerance": 0.018},
        )
        dialog_overrides = {"blend_tolerance": 0.1}
        bundle = pp.build_configuration_bundle(overrides=dialog_overrides)

        # Explicit override wins — job overrides are never read
        self.assertEqual(bundle["blend_tolerance"], 0.1)

    def test150_build_bundle_unknown_override_key_ignored(self):
        """Override keys not present in the bundle are silently ignored."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05},
        )
        bundle = pp.build_configuration_bundle(overrides={"nonexistent_key": 42})

        self.assertNotIn("nonexistent_key", bundle)
        self.assertEqual(bundle["blend_tolerance"], 0.05)

    def test160_build_bundle_empty_overrides(self):
        """Empty overrides dict changes nothing."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05},
        )
        bundle = pp.build_configuration_bundle(overrides={})

        self.assertEqual(bundle["blend_tolerance"], 0.05)

    def test170_build_bundle_is_pure(self):
        """build_configuration_bundle has no side effects on self.values or machine."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
        )
        original_values = dict(pp.values)
        original_props = dict(pp._machine.postprocessor_properties)

        pp.build_configuration_bundle(overrides={"blend_tolerance": 999.0})

        # self.values unchanged
        self.assertEqual(pp.values, original_values)
        # machine.postprocessor_properties unchanged
        self.assertEqual(pp._machine.postprocessor_properties, original_props)

    # ------------------------------------------------------------------
    # apply_configuration_bundle — side-effecting tests
    # ------------------------------------------------------------------

    def test200_apply_bundle_syncs_uppercase_keys(self):
        """apply_configuration_bundle writes bundle keys as UPPERCASE into self.values."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
        )
        pp.apply_configuration_bundle()

        self.assertEqual(pp.values["BLEND_TOLERANCE"], 0.05)
        self.assertEqual(pp.values["BLEND_MODE"], "BLEND")

    def test210_apply_bundle_with_overrides(self):
        """apply_configuration_bundle(overrides) writes overridden values."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
        )
        pp.apply_configuration_bundle(overrides={"blend_tolerance": 0.018})

        self.assertEqual(pp.values["BLEND_TOLERANCE"], 0.018)
        self.assertEqual(pp.values["BLEND_MODE"], "BLEND")

    def test220_apply_bundle_updates_machine_properties(self):
        """apply_configuration_bundle writes bundle back to machine.postprocessor_properties."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05},
        )
        pp.apply_configuration_bundle(overrides={"blend_tolerance": 0.018})

        # Machine props updated
        self.assertEqual(pp._machine.postprocessor_properties["blend_tolerance"], 0.018)
        # Schema-default keys also backfilled
        self.assertIn("blend_mode", pp._machine.postprocessor_properties)

    def test230_apply_bundle_idempotent(self):
        """Calling apply_configuration_bundle twice produces the same result."""
        pp = self._make_postprocessor(
            machine_props={"blend_tolerance": 0.05, "blend_mode": "BLEND"},
            job_overrides={"blend_tolerance": 0.018},
        )
        pp.apply_configuration_bundle()
        first = dict(pp.values)

        pp.apply_configuration_bundle()
        second = dict(pp.values)

        self.assertEqual(first, second)

    # ------------------------------------------------------------------
    # _read_job_overrides — parsing tests
    # ------------------------------------------------------------------

    def test300_read_job_overrides_valid_json(self):
        """Valid JSON string is parsed correctly."""
        pp = self._make_postprocessor(job_overrides={"blend_tolerance": 0.018})
        result = pp._read_job_overrides()
        self.assertEqual(result, {"blend_tolerance": 0.018})

    def test310_read_job_overrides_empty(self):
        """Empty JSON returns empty dict."""
        pp = self._make_postprocessor()
        result = pp._read_job_overrides()
        self.assertEqual(result, {})

    def test320_read_job_overrides_no_job(self):
        """No job returns empty dict."""
        pp = self._make_postprocessor()
        pp._job = None
        result = pp._read_job_overrides()
        self.assertEqual(result, {})

    def test330_read_job_overrides_invalid_json(self):
        """Invalid JSON returns empty dict without raising."""
        pp = self._make_postprocessor()
        pp._job.PostProcessorPropertyOverrides = "not valid json {"
        result = pp._read_job_overrides()
        self.assertEqual(result, {})

    def test340_read_job_overrides_non_dict_json(self):
        """JSON that parses to non-dict returns empty dict."""
        pp = self._make_postprocessor()
        pp._job.PostProcessorPropertyOverrides = "[1, 2, 3]"
        result = pp._read_job_overrides()
        self.assertEqual(result, {})
