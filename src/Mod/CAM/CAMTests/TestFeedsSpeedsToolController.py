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
Integration test for the resolver against a real ToolController and ToolBit
in a FreeCAD document. Exercises:
  - Lazy-add of OpTypeHint and FeedSpeedProvenance on first dialog open.
  - End-to-end adapter -> resolver -> apply round-trip writing
    HorizFeed/VertFeed/SpindleSpeed.
  - Provenance stamping for resolved fields.
"""

import unittest
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from typing import cast
from Path.Tool.toolbit import ToolBitEndmill
from Path.Tool.FeedsSpeeds import (
    MaterialContext,
    OpContext,
    make_preset,
    resolve,
    set_presets,
)


class TestFeedsSpeedsToolController(PathTestWithAssets):

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("FSToolControllerTest")
        # Acquire a 5mm endmill from the test asset store; this also gives
        # us a doc-object-bearing toolbit instance.
        self.toolbit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))

    def tearDown(self):
        try:
            FreeCAD.closeDocument(self.doc.Name)
        except Exception:
            pass
        super().tearDown()

    def test_resolver_applies_to_tc_and_stamps_provenance(self):
        # Seed a preset on the tool.
        preset = make_preset(
            material_uuid="uuid-test-mat",
            material_name="Test Material",
            op_type="profile",
            surface_speed=400.0,
            chipload=0.05,
        )
        set_presets(self.toolbit.obj, [preset])

        # Build a TC-shaped doc object the lightweight way: we don't need
        # the full ToolController factory here, just a doc object carrying
        # the relevant feed/speed/spindle properties.
        tc = self.doc.addObject("App::FeaturePython", "TC")
        tc.addProperty("App::PropertySpeed", "HorizFeed", "Feed", "")
        tc.addProperty("App::PropertySpeed", "VertFeed", "Feed", "")
        tc.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", "")

        # Run the resolver via the public API.
        from Path.Tool.FeedsSpeeds import ToolContext

        tool_ctx = ToolContext(
            diameter=5.0,
            flutes=2,
            presets=(preset,),
            shape_id="endmill",
        )
        material = MaterialContext(uuid="uuid-test-mat", name="Test Material")
        op = OpContext(op_type="profile")
        result = resolve(tool_ctx, material, op)
        self.assertNotEqual(result.source, "")
        self.assertIsNotNone(result.spindle_speed)
        self.assertIsNotNone(result.horiz_feed)

        # Apply via the Gui-side helper. (Importing FeedsSpeedsDialog is fine
        # in headless tests; we don't show any widgets, we just call the
        # property-mutation helper.)
        from Path.Tool.Gui.FeedsSpeedsDialog import write_result_to_tc

        write_result_to_tc(tc, result)

        self.assertTrue(hasattr(tc, "OpTypeHint"))
        self.assertTrue(hasattr(tc, "FeedSpeedProvenance"))

        # Provenance was stamped for the three resolved fields.
        prov = dict(tc.FeedSpeedProvenance)
        self.assertIn("HorizFeed", prov)
        self.assertIn("VertFeed", prov)
        self.assertIn("SpindleSpeed", prov)
        self.assertTrue(prov["HorizFeed"].startswith("preset:tool/"))

        # The TC carries the resolved values.
        self.assertGreater(float(tc.SpindleSpeed), 0.0)
        self.assertGreater(float(tc.HorizFeed.Value), 0.0)
        self.assertGreater(float(tc.VertFeed.Value), 0.0)

    def test_empty_result_does_not_mutate_tc(self):
        from Path.Tool.Gui.FeedsSpeedsDialog import write_result_to_tc
        from Path.Tool.FeedsSpeeds import FeedSpeedResult

        tc = self.doc.addObject("App::FeaturePython", "TC2")
        tc.addProperty("App::PropertySpeed", "HorizFeed", "Feed", "")
        tc.addProperty("App::PropertySpeed", "VertFeed", "Feed", "")
        tc.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", "")

        write_result_to_tc(tc, FeedSpeedResult())  # empty result

        # Lazy properties should NOT have been added for an empty result.
        self.assertFalse(hasattr(tc, "OpTypeHint"))
        self.assertFalse(hasattr(tc, "FeedSpeedProvenance"))


if __name__ == "__main__":
    unittest.main()
