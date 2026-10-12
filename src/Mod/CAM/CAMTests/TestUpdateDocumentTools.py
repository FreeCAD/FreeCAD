# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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
Tests for Path.Tool.UpdateDocumentTools: matching a Job-embedded ToolBit
back to its library source, diffing its dimensional properties against
that source, deciding whether a tool is stale at all (presets and/or
geometry), and fully replacing a stale tool with a fresh library copy.
"""

import unittest
from typing import cast

import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.toolbit import ToolBitEndmill
from Path.Tool.FeedsSpeeds import get_presets, make_preset, set_presets
from Path.Tool.UpdateDocumentTools import (
    diff_tool_geometry,
    geometry_properties,
    job_stale_tools,
    replace_tool_from_library,
    resolve_library_source,
)


class TestResolveLibrarySource(PathTestWithAssets):
    """Uses the isolated in-memory AssetManager (self.assets) from
    PathTestWithAssets, which registers a "local" store - the only store
    a depth=0 lookup ever searches (see resolve_library_source's
    docstring). Matching is ToolBitID only - no fallback, no guessing."""

    def test_direct_id_match(self):
        found = resolve_library_source(self.assets, "5mm_Endmill")
        self.assertIsNotNone(found)
        self.assertEqual(found.get_id(), "5mm_Endmill")

    def test_empty_id_returns_none(self):
        found = resolve_library_source(self.assets, "")
        self.assertIsNone(found)

    def test_unknown_id_returns_none(self):
        found = resolve_library_source(self.assets, "not-a-real-id")
        self.assertIsNone(found)


class TestGeometryProperties(PathTestWithAssets):

    def test_includes_known_dimensional_properties(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        props = geometry_properties(toolbit.obj)
        for name in ("Diameter", "CuttingEdgeHeight", "Length", "ShankDiameter"):
            self.assertIn(name, props)

    def test_excludes_non_shape_properties(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        props = geometry_properties(toolbit.obj)
        self.assertNotIn("Label", props)
        self.assertNotIn("ToolBitID", props)


class TestDiffToolGeometry(PathTestWithAssets):

    def test_no_differences_when_identical(self):
        embedded = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        self.assertEqual(diff_tool_geometry(embedded.obj, library.obj), [])

    def test_finds_changed_diameter(self):
        embedded = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library.obj.Diameter = 4.8

        changes = diff_tool_geometry(embedded.obj, library.obj)

        self.assertEqual(len(changes), 1)
        self.assertEqual(changes[0].name, "Diameter")
        self.assertAlmostEqual(float(changes[0].old_value), 5.0)
        self.assertAlmostEqual(float(changes[0].new_value), 4.8)

    def test_finds_multiple_changes(self):
        embedded = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library.obj.CuttingEdgeHeight = 25.0
        library.obj.Length = 45.0

        changes = {c.name for c in diff_tool_geometry(embedded.obj, library.obj)}

        self.assertEqual(changes, {"CuttingEdgeHeight", "Length"})

    def test_shape_type_mismatch_is_not_diffed(self):
        embedded = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library.obj.Diameter = 4.8  # a real difference, but ShapeType wins first
        library.obj.ShapeType = "Bullnose"

        self.assertEqual(diff_tool_geometry(embedded.obj, library.obj), [])


class TestUpdateToolFromLibrary(PathTestWithAssets):

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("UpdateToolFromLibraryTest")

    def tearDown(self):
        try:
            FreeCAD.closeDocument(self.doc.Name)
        except Exception:
            pass  # already closed by the test itself, or never fully created
        super().tearDown()

    def _job_with_tool(self, embedded_tool_obj):
        # embedded_tool_obj must be a real App.DocumentObject, already
        # passed through attach_to_doc() - a ToolController's Tool is a
        # real link property, unlike the DetachedDocumentObject a library
        # fetch returns.
        tc = self.doc.addObject("App::FeaturePython", "TC")
        tc.addExtension("App::GroupExtensionPython")
        tc.addProperty("App::PropertyLink", "Tool", "Path", "")
        tc.Tool = embedded_tool_obj
        tools = self.doc.addObject("App::DocumentObjectGroupPython", "Tools")
        tools.addObject(tc)
        job = self.doc.addObject("App::FeaturePython", "Job")
        job.addProperty("App::PropertyLink", "Tools", "Path", "")
        job.Tools = tools
        return job, tc

    def test_geometry_only_difference_is_detected(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, _tc = self._job_with_tool(embedded_obj)

        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library_tool.obj.CuttingEdgeHeight = 25.0
        self.assets.add(library_tool, store="local")

        infos = job_stale_tools(job, self.assets)

        self.assertEqual(len(infos), 1)
        self.assertFalse(infos[0].presets_differ)
        self.assertEqual([c.name for c in infos[0].geometry_changes], ["CuttingEdgeHeight"])

    def test_presets_only_difference_is_detected(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, _tc = self._job_with_tool(embedded_obj)

        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        set_presets(
            library_tool.obj, [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        )
        self.assets.add(library_tool, store="local")

        infos = job_stale_tools(job, self.assets)

        self.assertEqual(len(infos), 1)
        self.assertTrue(infos[0].presets_differ)
        self.assertEqual(infos[0].geometry_changes, [])

    def test_both_presets_and_geometry_differing_is_one_row(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, _tc = self._job_with_tool(embedded_obj)

        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library_tool.obj.Diameter = 4.8
        set_presets(
            library_tool.obj, [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        )
        self.assets.add(library_tool, store="local")

        infos = job_stale_tools(job, self.assets)

        self.assertEqual(len(infos), 1)
        self.assertTrue(infos[0].presets_differ)
        self.assertEqual([c.name for c in infos[0].geometry_changes], ["Diameter"])

    def test_unchanged_tool_is_absent_from_results(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, _tc = self._job_with_tool(embedded_obj)

        self.assertEqual(job_stale_tools(job, self.assets), [])

    def test_tool_with_no_library_match_is_absent_from_results(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        embedded_obj.ToolBitID = "no-such-id-in-library"
        job, _tc = self._job_with_tool(embedded_obj)

        self.assertEqual(job_stale_tools(job, self.assets), [])

    def test_replace_relinks_tc_to_a_fresh_tool_with_library_values(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, tc = self._job_with_tool(embedded_obj)

        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library_tool.obj.Diameter = 4.8
        set_presets(
            library_tool.obj, [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        )
        self.assets.add(library_tool, store="local")

        replaced = replace_tool_from_library(tc, self.assets)

        self.assertTrue(replaced)
        self.assertNotEqual(tc.Tool, embedded_obj)
        self.assertAlmostEqual(float(tc.Tool.Diameter), 4.8)
        self.assertEqual(len(get_presets(tc.Tool)), 1)

    def test_replace_removes_the_old_tool_object(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job, tc = self._job_with_tool(embedded_obj)
        old_name = embedded_obj.Name

        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        library_tool.obj.Diameter = 4.8
        self.assets.add(library_tool, store="local")

        replace_tool_from_library(tc, self.assets)

        self.assertIsNone(self.doc.getObject(old_name))

    def test_replace_returns_false_when_no_library_match(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        embedded_obj.ToolBitID = "no-such-id-in-library"
        job, tc = self._job_with_tool(embedded_obj)

        replaced = replace_tool_from_library(tc, self.assets)

        self.assertFalse(replaced)
        self.assertEqual(tc.Tool, embedded_obj)


if __name__ == "__main__":
    unittest.main()
