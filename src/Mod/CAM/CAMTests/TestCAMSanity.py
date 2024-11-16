# -*- coding: utf-8 -*-
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
import imghdr
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
        # """Test CAMSanity using a DummyImageBuilder"""
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
                val = data["key"]
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

    # def test140(self):
    #     """Test Generate Report"""
    #     with patch('Path.Main.Sanity.ImageBuilder.ImageBuilderFactory.get_image_builder') as mock_factory:
    #         dummy_builder = DummyImageBuilder(self.temp_file.name)
    #         mock_factory.return_value = dummy_builder
    #         S = Sanity.CAMSanity(self.job, output_file= self.temp_file.name)
    #         html_content = S.get_output_report()
    #         self.assertIsInstance(html_content, str)

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

    # def test200(self):
    #     """Generate Report This test is disabled but useful during development to see the report"""
    #     with tempfile.NamedTemporaryFile() as temp_file:
    #         S= Sanity.CAMSanity(self.job, output_file=temp_file.name)
    #         shape = self.doc.getObject("Box")
    #         html_content = S.get_output_report()

    #         encoded_html = urllib.parse.quote(html_content)

    #         data_uri = f"data:text/html;charset=utf-8,{encoded_html}"
    #         import webbrowser
    #         webbrowser.open(data_uri)
