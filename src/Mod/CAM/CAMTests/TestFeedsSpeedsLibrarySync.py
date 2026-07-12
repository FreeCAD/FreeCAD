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
Tests for Path.Tool.FeedsSpeeds.library_sync: merging presets from a library
ToolBit asset into a Job-embedded copy, and matching embedded tools back to
their library source by ToolBitID only.
"""

import unittest
from typing import cast

import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.toolbit import ToolBitEndmill
from Path.Tool.FeedsSpeeds import get_presets, make_preset, set_presets
from Path.Tool.FeedsSpeeds.library_sync import (
    preset_key,
    merge_presets,
    resolve_library_source,
    sync_job_tools,
    sync_tool_presets,
)


class TestMergePresets(unittest.TestCase):
    """Pure-function tests; no FreeCAD document needed."""

    def test_empty_embedded_and_library_presets_yield_empty(self):
        self.assertEqual(merge_presets([], []), [])

    def test_empty_embedded_copies_library_straight_in(self):
        library = [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        self.assertEqual(merge_presets([], library), library)

    def test_local_only_preset_is_preserved(self):
        local_only = make_preset(name="My Special", surface_speed=250, chipload=0.03)
        merged = merge_presets([local_only], [])
        self.assertEqual(merged, [local_only])

    def test_same_key_in_both_library_wins(self):
        embedded = [make_preset(name="Roughing", surface_speed=200, chipload=0.02)]
        library = [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        merged = merge_presets(embedded, library)
        self.assertEqual(len(merged), 1)
        self.assertEqual(merged[0]["surface_speed"], 300)

    def test_disjoint_named_presets_are_both_kept(self):
        embedded = [make_preset(name="A", surface_speed=100, chipload=0.01)]
        library = [make_preset(name="B", surface_speed=200, chipload=0.02)]
        merged = merge_presets(embedded, library)
        self.assertEqual({p["name"] for p in merged}, {"A", "B"})

    def test_key_derivation_includes_material_and_op(self):
        named = make_preset(name="Roughing", surface_speed=1, chipload=1)
        unnamed = make_preset(
            material_uuid="uuid-1",
            material_name="Aluminum",
            op_type="pocket",
            surface_speed=1,
            chipload=1,
        )
        self.assertEqual(preset_key(named), ("Roughing", None, None))
        self.assertEqual(preset_key(unnamed), (None, "uuid-1", "pocket"))

    def test_same_name_different_material_is_not_a_collision(self):
        embedded = [
            make_preset(name="Default", material_uuid="hard", surface_speed=100, chipload=0.01)
        ]
        library = [
            make_preset(name="Default", material_uuid="soft", surface_speed=200, chipload=0.02)
        ]
        merged = merge_presets(embedded, library)
        self.assertEqual(len(merged), 2)

    def test_two_unnamed_presets_with_same_hint_collide(self):
        # Same material/op hint, different numbers - an inherent limitation
        # of hint-based keys for anonymous presets; library wins as usual.
        embedded = [
            make_preset(material_uuid="uuid-1", op_type="pocket", surface_speed=100, chipload=0.01)
        ]
        library = [
            make_preset(material_uuid="uuid-1", op_type="pocket", surface_speed=200, chipload=0.02)
        ]
        merged = merge_presets(embedded, library)
        self.assertEqual(len(merged), 1)
        self.assertEqual(merged[0]["surface_speed"], 200)


class TestSyncToolPresets(PathTestWithAssets):

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("LibrarySyncPresetsTest")

    def tearDown(self):
        try:
            FreeCAD.closeDocument(self.doc.Name)
        except Exception:
            pass  # already closed by the test itself, or never fully created
        super().tearDown()

    def _make_obj(self):
        return self.doc.addObject("App::FeaturePython", "Obj")

    def test_noop_when_merge_produces_no_change(self):
        embedded = self._make_obj()
        library = self._make_obj()
        preset = make_preset(name="Roughing", surface_speed=300, chipload=0.05)
        set_presets(embedded, [preset])
        set_presets(library, [preset])
        self.assertFalse(sync_tool_presets(embedded, library))

    def test_changed_when_library_has_new_preset(self):
        embedded = self._make_obj()
        library = self._make_obj()
        set_presets(library, [make_preset(name="Roughing", surface_speed=300, chipload=0.05)])
        self.assertTrue(sync_tool_presets(embedded, library))
        self.assertEqual(len(get_presets(embedded)), 1)


class TestResolveLibrarySource(PathTestWithAssets):
    """Uses the isolated in-memory AssetManager (self.assets) from
    PathTestWithAssets, which registers a "local" store - the only store
    this module ever searches (see library_sync.py's module docstring).
    Matching is ToolBitID only - no fallback, no guessing."""

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


class TestSyncJobTools(PathTestWithAssets):

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("LibrarySyncJobTest")

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
        return job

    def test_updates_embedded_tool_presets_from_library(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job = self._job_with_tool(embedded_obj)

        # Simulate a library curation edit: fetch the tool, add a preset,
        # and persist it back to the store (what the library editor does on
        # accept - see presets.py's docstring). resolve_library_source
        # re-fetches from the store, so mutating an in-memory ToolBit alone
        # would not be visible to it.
        library_tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        set_presets(
            library_tool.obj, [make_preset(name="Roughing", surface_speed=300, chipload=0.05)]
        )
        self.assets.add(library_tool, store="local")
        # Simulate the embedded copy having come from the library (same id).
        self.assertEqual(embedded_obj.ToolBitID, library_tool.obj.ToolBitID)

        report = sync_job_tools(job, self.assets)

        self.assertEqual(len(report.updated), 1)
        self.assertEqual(len(report.unchanged), 0)
        self.assertEqual(len(report.orphaned), 0)
        self.assertEqual(len(get_presets(embedded_obj)), 1)

    def test_second_sync_with_no_library_changes_is_a_noop(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        job = self._job_with_tool(embedded_obj)

        report1 = sync_job_tools(job, self.assets)
        self.assertEqual(len(report1.updated), 0)
        self.assertEqual(len(report1.unchanged), 1)

    def test_tool_with_no_library_match_is_reported_orphaned(self):
        toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        embedded_obj = toolbit.attach_to_doc(self.doc)
        embedded_obj.ToolBitID = "no-such-id-in-library"
        embedded_obj.Label = "Some Unrelated Custom Tool"
        job = self._job_with_tool(embedded_obj)

        report = sync_job_tools(job, self.assets)

        self.assertEqual(len(report.orphaned), 1)
        self.assertEqual(len(report.updated), 0)
        self.assertEqual(len(report.unchanged), 0)


if __name__ == "__main__":
    unittest.main()
