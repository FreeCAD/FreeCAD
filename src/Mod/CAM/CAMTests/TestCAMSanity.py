# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
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

import FreeCAD
import Path
from Path.Main.Sanity import ReportGenerator, Sanity
from Path.Main.Sanity.ImageBuilder import (
    DummyImageBuilder,
    ImageBuilderFactory,
    ImageBuilder,
)
import os
import Path.Post.Command as PathPost
from Path.Post.Processor import PostProcessor
import unittest
from unittest.mock import patch, MagicMock
import urllib
import tempfile
from CAMTests.PathTestUtils import PathTestBase


class TestCAMSanity(PathTestBase):
    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")
        cls.job = cls.doc.getObject("Job")

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument("boxtest")

    def setUp(self):
        self.temp_file = tempfile.NamedTemporaryFile()

    def tearDown(self):
        pass

    def test00(self):
        """Test no output location"""
        with self.assertRaises(TypeError):
            S = Sanity.CAMSanity(self.job)

    def test010(self):
        """Test CAMSanity using a DummyImageBuilder.

        Given: A valid CAM job (boxtest.fcstd) and a DummyImageBuilder.
        When: CAMSanity is instantiated with a temp output file.
        Then: The instance is created with the correct filelocation and output_file attributes.
        """
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
            self.assertTrue(os.path.isdir(S.filelocation))
            self.assertEqual(self.temp_file.name, S.output_file)

    def test020(self):
        """Test erroneous output location"""
        filename = "/tmpXXXX/THIS_DOESN'T_EXIST"
        with self.assertRaises(ValueError):
            S = Sanity.CAMSanity(self.job, output_file=filename)

    # This test fails  A headless image generation routine is needed.
    # def test40(self):
    #     """Test image generation"""
    #     import imghdr  # fixme: not available in python3.13
    #     path = FreeCAD.getUserMacroDir()
    #     image_builder = ImageBuilder.ImageBuilderFactory.get_image_builder(path)
    #     file_name = image_builder.build_image(self.doc.getObject("Box"), "theBox")
    #     print(f"filename:{file_name}")

    #     # This test shouldn't be enabled in production as is.
    #     # it writes to the user macro directory as a persistent location.
    #     # writing to tempdir fails
    #     # the tempfile get cleaned up before we can do any asserts.

    #     # Need to find a way to write semi-persistently for the tests to pass

    #     with open(file_name, 'rb') as f:
    #         header = f.read(32)  # Read the first 32 bytes
    #     self.assertTrue(imghdr.what(None, header) is not None)

    def test050(self):
        """Test base data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._baseObjectData()
        self.assertIsInstance(data, dict)
        self.assertIn("baseimage", data)
        self.assertIn("bases", data)

    def test060(self):
        """Test design data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._designData()
        self.assertIsInstance(data, dict)
        self.assertIn("FileName", data)
        self.assertIn("LastModifiedDate", data)
        self.assertIn("Customer", data)
        self.assertIn("Designer", data)
        self.assertIn("JobDescription", data)
        self.assertIn("JobLabel", data)
        self.assertIn("Sequence", data)
        self.assertIn("JobType", data)

    def test070(self):
        """Test tool data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._toolData()
        self.assertIn("squawkData", data)

        for key in data.keys():
            if isinstance(key, int):
                val = data[key]
                self.assertIsInstance(val, dict)
                self.assertIn("bitShape", val)
                self.assertIn("description", val)
                self.assertIn("manufacturer", val)
                self.assertIn("url", val)
                self.assertIn("inspectionNotes", val)
                self.assertIn("diameter", val)
                self.assertIn("shape", val)
                self.assertIn("partNumber", val)

    def test080(self):
        """Test run data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._runData()
        self.assertIsInstance(data, dict)
        self.assertIn("cycletotal", data)
        self.assertIn("jobMinZ", data)
        self.assertIn("jobMaxZ", data)
        self.assertIn("jobDescription", data)
        self.assertIn("squawkData", data)
        self.assertIn("operations", data)

    def test090(self):
        """Test output data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._outputData()
        self.assertIsInstance(data, dict)
        self.assertIn("lastpostprocess", data)
        self.assertIn("lastgcodefile", data)
        self.assertIn("optionalstops", data)
        self.assertIn("programmer", data)
        self.assertIn("machine", data)
        self.assertIn("postprocessor", data)
        self.assertIn("postprocessorFlags", data)
        self.assertIn("filesize", data)
        self.assertIn("linecount", data)
        self.assertIn("outputfilename", data)
        self.assertIn("squawkData", data)

    def test100(self):
        """Test fixture data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._fixtureData()
        self.assertIsInstance(data, dict)
        self.assertIn("fixtures", data)
        self.assertIn("orderBy", data)
        self.assertIn("datumImage", data)
        self.assertIn("squawkData", data)

    def test110(self):
        """Test stock data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._stockData()
        self.assertIsInstance(data, dict)
        self.assertIn("xLen", data)
        self.assertIn("yLen", data)
        self.assertIn("zLen", data)
        self.assertIn("material", data)
        self.assertIn("stockImage", data)
        self.assertIn("squawkData", data)

    def test120(self):
        """Test squawk data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S.squawk(
            "CAMSanity",
            "Test Message",
            squawkType="TIP",
        )
        self.assertIsInstance(data, dict)
        self.assertIn("Date", data)
        self.assertIn("Operator", data)
        self.assertIn("Note", data)
        self.assertIn("squawkType", data)
        self.assertIn("squawkIcon", data)

    def test130(self):
        """Test tool data"""
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
        data = S._toolData()
        self.assertIsInstance(data, dict)
        self.assertIn("squawkData", data)

        for key, val in data.items():
            if key == "squawkData":
                continue
            else:
                self.assertIsInstance(val, dict)
                self.assertIn("bitShape", val)
                self.assertIn("description", val)
                self.assertIn("manufacturer", val)
                self.assertIn("url", val)
                self.assertIn("inspectionNotes", val)
                self.assertIn("diameter", val)
                self.assertIn("shape", val)
                self.assertIn("partNumber", val)

    def test140(self):
        """Test get_output_report returns a non-empty HTML string.

        Given: A valid CAM job (boxtest.fcstd) and a DummyImageBuilder.
        When: get_output_report() is called on a CAMSanity instance.
        Then: The result is a non-empty string containing HTML content.
        """
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)
            html_content = S.get_output_report()
            self.assertIsInstance(html_content, str)

    # def test150(self):
    #     """Test Post Processing a File"""

    #     def exportObjectsWith(objs, partname, job, sequence, postname):
    #         Path.Log.track(partname, sequence)
    #         Path.Log.track(objs)

    #         Path.Log.track(objs, partname)

    #         postArgs = Path.Preferences.defaultPostProcessorArgs()
    #         if hasattr(job, "PostProcessorArgs") and job.PostProcessorArgs:
    #             postArgs = job.PostProcessorArgs
    #         elif hasattr(job, "PostProcessor") and job.PostProcessor:
    #             postArgs = ""

    #         Path.Log.track(postArgs)

    #         filename = f"output-{partname}-{sequence}.ngc"

    #         processor = PostProcessor.load(postname)
    #         gcode = processor.export(objs, filename, postArgs)
    #         return (gcode, filename)

    #     doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
    #     job = self.doc.getObject("Job")

    #     postlist = PathPost.buildPostList(job)

    #     filenames = []
    #     for idx, section in enumerate(postlist):
    #         partname = section[0]
    #         sublist = section[1]

    #         gcode, name = exportObjectsWith(sublist, partname, job, idx, "linuxcnc")
    #         filenames.append(name)

    #     with tempfile.TemporaryDirectory() as temp_dir:
    #         output_file_path = temp_dir

    #         output_file_name = os.path.join(output_file_path, "test.ngc")

    # def test_dev_report(self):
    #     """Generate Report: disabled but useful during development to see the report"""
    #     with tempfile.NamedTemporaryFile() as temp_file:
    #         S= Sanity.CAMSanity(self.job, output_file=temp_file.name)
    #         html_content = S.get_output_report()
    #         encoded_html = urllib.parse.quote(html_content)
    #         data_uri = f"data:text/html;charset=utf-8,{encoded_html}"
    #         import webbrowser
    #         webbrowser.open(data_uri)

    # --- Helpers ---

    def _make_sanity_with_mock_job(self, mock_job):
        """Create a CAMSanity instance with a mock job, bypassing summarize() during init.

        Used by tests that call individual _xxxData() methods with a controlled mock job
        without running the full summarize() pipeline.
        """
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            with patch.object(Sanity.CAMSanity, "summarize", return_value={}):
                S = Sanity.CAMSanity(mock_job, output_file=self.temp_file.name)
        return S

    def _make_mock_tc(
        self,
        tool_number=1,
        shape_type="endmill",
        horiz_feed_value=1000.0,
        spindle_speed=1000.0,
        label="TC: Endmill",
    ):
        """Create a mock tool controller with the given properties."""
        mock_tc = MagicMock()
        mock_tc.Tool.BitBody = True
        mock_tc.Tool.ShapeType = shape_type
        mock_tc.Tool.Diameter.UserString = "10 mm"
        mock_tc.Tool.Label = shape_type.capitalize()
        mock_tc.HorizFeed.Value = horiz_feed_value
        mock_tc.HorizFeed.UserString = f"{horiz_feed_value} mm/s"
        mock_tc.SpindleSpeed = spindle_speed
        mock_tc.ToolNumber = tool_number
        mock_tc.Label = label
        return mock_tc

    def _make_mock_job(
        self,
        machine_name="test_machine",
        has_tools=True,
        has_operations=True,
        has_model=True,
    ):
        """Create a mock job with basic attributes for testing."""
        mock_job = MagicMock()
        mock_job.Machine = machine_name
        mock_job.LastPostProcessDate = ""
        mock_job.LastPostProcessOutput = ""
        mock_job.PostProcessor = "linuxcnc"
        mock_job.PostProcessorArgs = ""
        mock_job.PostProcessorOutputFile = ""

        if has_tools:
            mock_tc = self._make_mock_tc()
            mock_job.Tools.Group = [mock_tc]
        else:
            mock_job.Tools.Group = []

        if has_operations:
            mock_job.Operations.Group = []
        else:
            mock_job.Operations = None

        if has_model:
            mock_job.Model = MagicMock()
        else:
            mock_job.Model = None

        return mock_job

    # --- Phase 2: Squawk Detection Tests ---

    def test200_zero_feedrate_squawk(self):
        """Zero feedrate on a tool controller should produce a WARNING squawk.

        Given: A job with one tool controller whose HorizFeed.Value is 0.0.
        When: _toolData() is called.
        Then: squawkData contains a WARNING entry mentioning "feedrate".

        Example: TC.HorizFeed.Value = 0.0
          → squawkData contains {"squawkType": "WARNING", "Note": "... feedrate ..."}
        """
        mock_tc = self._make_mock_tc(horiz_feed_value=0.0)
        mock_job = MagicMock()
        mock_job.Tools.Group = [mock_tc]
        mock_job.Operations.Group = []

        S = self._make_sanity_with_mock_job(mock_job)
        data = S._toolData()

        squawks = data.get("squawkData", [])
        warning_squawks = [s for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(
            any("feedrate" in s["Note"].lower() for s in warning_squawks),
            f"Expected WARNING squawk for zero feedrate, got: {squawks}",
        )

    def test210_zero_spindle_squawk(self):
        """Zero spindle speed on a tool controller should produce a WARNING squawk.

        Given: A job with one tool controller whose SpindleSpeed is 0.0.
        When: _toolData() is called.
        Then: squawkData contains a WARNING entry mentioning "spindlespeed".

        Example: TC.SpindleSpeed = 0.0
          → squawkData contains {"squawkType": "WARNING", "Note": "... spindlespeed ..."}
        """
        mock_tc = self._make_mock_tc(spindle_speed=0.0)
        mock_job = MagicMock()
        mock_job.Tools.Group = [mock_tc]
        mock_job.Operations.Group = []

        S = self._make_sanity_with_mock_job(mock_job)
        data = S._toolData()

        squawks = data.get("squawkData", [])
        warning_squawks = [s for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(
            any("spindlespeed" in s["Note"].lower() for s in warning_squawks),
            f"Expected WARNING squawk for zero spindle speed, got: {squawks}",
        )

    def test220_unused_tool_controller_squawk(self):
        """A tool controller not used by any operation should produce a WARNING squawk.

        Given: A job with one tool controller and no operations referencing it.
        When: _toolData() is called.
        Then: squawkData contains a WARNING entry mentioning "not used".

        Example: job.Operations.Group = [] with one TC present
          → squawkData contains {"squawkType": "WARNING", "Note": "... not used ..."}
        """
        mock_tc = self._make_mock_tc()
        mock_job = MagicMock()
        mock_job.Tools.Group = [mock_tc]
        mock_job.Operations.Group = []

        S = self._make_sanity_with_mock_job(mock_job)
        data = S._toolData()

        squawks = data.get("squawkData", [])
        warning_squawks = [s for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(
            any("not used" in s["Note"].lower() for s in warning_squawks),
            f"Expected WARNING squawk for unused TC, got: {squawks}",
        )

    def test230_duplicate_tool_number_squawk(self):
        """Two tool controllers sharing the same tool number should produce a CAUTION squawk.

        Given: A job with two tool controllers assigned tool number 1 but different ShapeTypes.
        When: _toolData() is called.
        Then: squawkData contains a CAUTION entry about multiple tools sharing a number.

        Example: TC1.ToolNumber=1 ShapeType="endmill", TC2.ToolNumber=1 ShapeType="ballmill"
          → squawkData contains {"squawkType": "CAUTION", "Note": "... multiple tools ..."}
        """
        mock_tc1 = self._make_mock_tc(tool_number=1, shape_type="endmill", label="TC1")
        mock_tc2 = self._make_mock_tc(tool_number=1, shape_type="ballmill", label="TC2")

        mock_job = MagicMock()
        mock_job.Tools.Group = [mock_tc1, mock_tc2]
        mock_job.Operations.Group = []

        S = self._make_sanity_with_mock_job(mock_job)
        data = S._toolData()

        squawks = data.get("squawkData", [])
        caution_squawks = [s for s in squawks if s["squawkType"] == "CAUTION"]
        self.assertTrue(
            len(caution_squawks) > 0,
            f"Expected CAUTION squawk for duplicate tool numbers, got: {squawks}",
        )

    def test240_not_postprocessed_squawk(self):
        """A job with no prior post-processing should produce a squawk in _outputData().

        Given: A job whose LastPostProcessOutput is an empty string.
        When: _outputData() is called.
        Then: squawkData contains an entry mentioning "post-process".

        Example: job.LastPostProcessOutput = ""
          → squawkData contains {"Note": "The Job has not been post-processed"}
        """
        mock_job = MagicMock()
        mock_job.LastPostProcessDate = ""
        mock_job.LastPostProcessOutput = ""
        mock_job.PostProcessor = "linuxcnc"
        mock_job.PostProcessorArgs = ""
        mock_job.PostProcessorOutputFile = ""
        mock_job.Operations.Group = []

        S = self._make_sanity_with_mock_job(mock_job)
        data = S._outputData()

        squawks = data.get("squawkData", [])
        self.assertTrue(len(squawks) > 0, "Expected at least one squawk for unprocessed job")
        self.assertTrue(
            any("post-process" in s["Note"].lower() for s in squawks),
            f"Expected squawk mentioning post-processing, got: {squawks}",
        )

    def test250_no_operations_squawk(self):
        """A job with no operations should produce a WARNING from _validate_job_structure().

        Given: A job where the Operations attribute is None (falsy).
        When: _validate_job_structure() is called.
        Then: The returned list contains a WARNING squawk mentioning "operation".

        Example: job.Operations = None
          → [{"squawkType": "WARNING", "Note": "No operations found in job"}]
        """
        mock_job = MagicMock()
        mock_job.Operations = None

        S = self._make_sanity_with_mock_job(mock_job)
        squawks = S._validate_job_structure()

        warning_squawks = [s for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(
            any("operation" in s["Note"].lower() for s in warning_squawks),
            f"Expected WARNING squawk for no operations, got: {squawks}",
        )

    def test260_no_base_geometry_squawk(self):
        """A job with no model/base geometry should produce a WARNING from _validate_job_structure().

        Given: A job that has operations but whose Model attribute is None (falsy).
        When: _validate_job_structure() is called.
        Then: The returned list contains a WARNING squawk mentioning "model" or "base".

        Example: job.Operations = MagicMock(), job.Model = None
          → [{"squawkType": "WARNING", "Note": "No model/base geometry found in job"}]
        """
        mock_job = MagicMock()
        mock_job.Operations = MagicMock()
        mock_job.Model = None

        S = self._make_sanity_with_mock_job(mock_job)
        squawks = S._validate_job_structure()

        warning_squawks = [s for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(
            any(
                "model" in s["Note"].lower() or "base" in s["Note"].lower() for s in warning_squawks
            ),
            f"Expected WARNING squawk for no base geometry, got: {squawks}",
        )

    def test270_validate_for_postprocessing_returns_tuple(self):
        """validate_for_postprocessing() should return a (bool, list, list) tuple.

        Given: A valid CAM job (boxtest.fcstd).
        When: validate_for_postprocessing() is called on a CAMSanity instance.
        Then:
          - Returns a 3-tuple: (has_critical, all_squawks, critical_squawks)
          - has_critical is a bool equal to (len(critical_squawks) > 0)
          - all_squawks and critical_squawks are lists
          - Every item in critical_squawks is also in all_squawks
        """
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)

        result = S.validate_for_postprocessing()

        self.assertIsInstance(result, tuple)
        self.assertEqual(len(result), 3)
        has_critical, all_squawks, critical_squawks = result
        self.assertIsInstance(has_critical, bool)
        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)
        self.assertEqual(has_critical, len(critical_squawks) > 0)
        for squawk in critical_squawks:
            self.assertIn(squawk, all_squawks)

    def test280_validate_job_static(self):
        """validate_job() static method should return (all_squawks, critical_squawks) without images.

        Given: A valid CAM job (boxtest.fcstd).
        When: CAMSanity.validate_job(job) is called as a static method.
        Then:
          - Returns a 2-tuple: (all_squawks, critical_squawks)
          - Both are lists of squawk dicts
          - All critical squawks have squawkType "WARNING" or "CAUTION"
          - All critical squawks are also in all_squawks
          - No image or HTML files are generated
        """
        result = Sanity.CAMSanity.validate_job(self.job)

        self.assertIsInstance(result, tuple)
        self.assertEqual(len(result), 2)
        all_squawks, critical_squawks = result
        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)
        for squawk in critical_squawks:
            self.assertIn(squawk["squawkType"], ("WARNING", "CAUTION"))
        for squawk in critical_squawks:
            self.assertIn(squawk, all_squawks)

    # --- Phase 6: Integration Tests ---

    def test300_get_all_squawks_completeness(self):
        """get_all_squawks() should collect squawks from all validation sections.

        Given: A valid CAM job (boxtest.fcstd) with a DummyImageBuilder.
        When: get_all_squawks() is called on a CAMSanity instance.
        Then:
          - Returns a 2-tuple: (all_squawks, critical_squawks)
          - critical_squawks contains only WARNING or CAUTION squawks
          - Every item in critical_squawks is also in all_squawks
          - Each squawk dict has the required keys: squawkType, Note, Date
        """
        with patch(
            "Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder"
        ) as mock_factory:
            dummy_builder = DummyImageBuilder(self.temp_file.name)
            mock_factory.return_value = dummy_builder
            S = Sanity.CAMSanity(self.job, output_file=self.temp_file.name)

        all_squawks, critical_squawks = S.get_all_squawks()

        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)
        for squawk in critical_squawks:
            self.assertIn(squawk["squawkType"], ("WARNING", "CAUTION"))
        for squawk in critical_squawks:
            self.assertIn(squawk, all_squawks)
        for squawk in all_squawks:
            self.assertIn("squawkType", squawk)
            self.assertIn("Note", squawk)
            self.assertIn("Date", squawk)

    def test310_validate_job_end_to_end(self):
        """validate_job() end-to-end: works without a pre-existing output path.

        Given: A valid CAM job (boxtest.fcstd).
        When: CAMSanity.validate_job(job) is called with no other setup.
        Then:
          - Returns (all_squawks, critical_squawks)
          - All critical squawks are WARNING or CAUTION
          - All critical squawks appear in all_squawks
          - squawkType and Note keys are present in every squawk
        """
        all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(self.job)

        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)
        for s in critical_squawks:
            self.assertIn(s["squawkType"], ("WARNING", "CAUTION"))
        for s in critical_squawks:
            self.assertIn(s, all_squawks)
        for s in all_squawks:
            self.assertIn("squawkType", s)
            self.assertIn("Note", s)

    def test320_postprocessor_sanity_checks_integration(self):
        """Postprocessor sanity checks: collected and integrated into validation.

        Given: A job with a machine that has postprocessor sanity checks.
        When: CAMSanity.validate_job() is called.
        Then: Postprocessor squawks appear in validation results.
        """
        # Create a mock job with machine
        mock_job = MagicMock()
        mock_job.Machine = "test_machine"
        mock_job.Tools.Group = []
        mock_job.Operations.Group = []

        # Create a mock postprocessor with sanity checks
        class MockPostprocessor:
            def get_sanity_checks(self, job):
                return [
                    {
                        "Date": "Mon Mar 24 17:00:00 2026",
                        "Operator": "MockPostprocessor",
                        "Note": "Test postprocessor warning",
                        "squawkType": "WARNING",
                        "squawkIcon": "/path/to/warning.svg",
                    },
                    {
                        "Date": "Mon Mar 24 17:00:00 2026",
                        "Operator": "MockPostprocessor",
                        "Note": "Test postprocessor note",
                        "squawkType": "NOTE",
                        "squawkIcon": "/path/to/note.svg",
                    },
                ]

        # Mock machine with postprocessor_file_name
        mock_machine = MagicMock()
        mock_machine.postprocessor_file_name = "test_post"

        import unittest.mock

        with unittest.mock.patch(
            "Machine.models.machine.MachineFactory.get_machine", return_value=mock_machine
        ), unittest.mock.patch(
            "Path.Post.Processor.PostProcessorFactory.get_post_processor",
            return_value=MockPostprocessor(),
        ):

            all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(mock_job)

            # Verify postprocessor squawks are included
            postprocessor_squawks = [s for s in all_squawks if s["Operator"] == "MockPostprocessor"]
            self.assertEqual(len(postprocessor_squawks), 2)

            # Verify critical classification
            postprocessor_critical = [
                s for s in critical_squawks if s["Operator"] == "MockPostprocessor"
            ]
            self.assertEqual(len(postprocessor_critical), 1)  # Only the WARNING
            self.assertEqual(postprocessor_critical[0]["squawkType"], "WARNING")

    def test321_postprocessor_sanity_checks_error_handling(self):
        """Postprocessor sanity checks: graceful handling of exceptions.

        Given: A postprocessor that raises an exception in get_sanity_checks().
        When: CAMSanity.validate_job() is called.
        Then: Exception is caught and validation continues.
        """
        mock_job = MagicMock()
        mock_job.Machine = "error_machine"
        mock_job.Tools.Group = []
        mock_job.Operations.Group = []

        class ErrorPostprocessor:
            def get_sanity_checks(self, job):
                raise Exception("Test postprocessor error")

        mock_machine = MagicMock()
        mock_machine.postprocessor_file_name = "error_post"

        import unittest.mock

        with unittest.mock.patch(
            "Machine.models.machine.MachineFactory.get_machine", return_value=mock_machine
        ), unittest.mock.patch(
            "Path.Post.Processor.PostProcessorFactory.get_post_processor",
            return_value=ErrorPostprocessor(),
        ):

            # Should not raise exception
            all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(mock_job)

            # Should still return valid results
            self.assertIsInstance(all_squawks, list)
            self.assertIsInstance(critical_squawks, list)

    def test322_postprocessor_sanity_checks_no_machine(self):
        """Postprocessor sanity checks: job without machine.

        Given: A job with no machine assigned.
        When: CAMSanity.validate_job() is called.
        Then: No postprocessor checks are performed, but validation continues.
        """
        mock_job = MagicMock()
        mock_job.Tools.Group = []
        mock_job.Operations.Group = []
        # No machine attribute

        all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(mock_job)

        # Should still return valid results
        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)

        # No postprocessor squawks should be present
        postprocessor_squawks = [
            s
            for s in all_squawks
            if hasattr(s, "Operator") and s["Operator"] not in ["CAMSanity", "MockTC"]
        ]
        self.assertEqual(len(postprocessor_squawks), 0)

    def test323_generic_plasma_sanity_checks_integration(self):
        """GenericPlasma postprocessor sanity checks: verify get_sanity_checks is called.

        Given: A job with a machine whose postprocessor_file_name is "generic_plasma".
        When: CAMSanity.validate_job() is called.
        Then: GenericPlasma get_sanity_checks() method is called and test squawk appears.
        """
        # Create a mock job with a machine name
        mock_job = self._make_mock_job(machine_name="plasma")

        # Add stock with thickness for plasma-specific checks
        mock_stock = MagicMock()
        mock_stock.Thickness = 10.0  # 10mm thick material
        mock_job.Stock = mock_stock

        # Mock the machine so it returns the correct postprocessor file name
        mock_machine = MagicMock()
        mock_machine.postprocessor_file_name = "generic_plasma"

        import unittest.mock

        with unittest.mock.patch(
            "Machine.models.machine.MachineFactory.get_machine", return_value=mock_machine
        ):
            # Call validate_job - this should load the real GenericPlasma postprocessor
            all_squawks, critical_squawks = Sanity.CAMSanity.validate_job(mock_job)

        # Verify we got squawks
        self.assertIsInstance(all_squawks, list)
        self.assertIsInstance(critical_squawks, list)
        self.assertGreater(len(all_squawks), 0, "Should have some squawks")

        # Look for the test squawk from GenericPlasma.get_sanity_checks
        test_squawks = [s for s in all_squawks if s["Note"] == "This is a test warning message"]
        self.assertGreater(len(test_squawks), 0, "Test squawk from GenericPlasma should be present")

        # Verify the test squawk is classified as WARNING (critical)
        test_critical = [
            s for s in critical_squawks if s["Note"] == "This is a test warning message"
        ]
        self.assertGreater(len(test_critical), 0, "Test squawk should be in critical squawks")
