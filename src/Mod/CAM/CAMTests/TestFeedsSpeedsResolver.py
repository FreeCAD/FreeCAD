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
Pure-function tests for the FeedsSpeeds resolver. No FreeCAD doc objects;
the contexts are constructed directly.
"""

import math
import unittest

from Path.Tool.FeedsSpeeds import (
    MachineContext,
    MaterialContext,
    OpContext,
    ToolContext,
    derive_preset_label,
    make_preset,
    resolve,
)
from Path.Tool.FeedsSpeeds.providers import (
    MachinabilityProvider,
    ToolDefaultsProvider,
    ToolPresetProvider,
    _score_preset,
)

ALU_UUID = "11111111-2222-3333-4444-555555555555"
STEEL_UUID = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"


def _alu_profile_preset():
    return make_preset(
        material_uuid=ALU_UUID,
        material_name="Aluminum 6061",
        op_type="profile",
        surface_speed=400.0,
        chipload=0.05,
        vert_feed_ratio=0.33,
    )


def _generic_drill_preset():
    return make_preset(
        op_type="drill",
        surface_speed=200.0,
        chipload=0.025,
    )


class TestFeedsSpeedsResolver(unittest.TestCase):

    def setUp(self):
        self.tool = ToolContext(
            diameter=6.35,  # 1/4 inch in mm
            flutes=2,
            presets=(_alu_profile_preset(),),
            shape_id="endmill",
            tool_material="Carbide",
        )
        self.alu = MaterialContext(uuid=ALU_UUID, name="Aluminum 6061")
        self.steel = MaterialContext(uuid=STEEL_UUID, name="Mild Steel")
        self.profile = OpContext(op_type="profile")
        self.pocket = OpContext(op_type="pocket")

    def test_no_presets_returns_empty_result(self):
        tool = ToolContext(diameter=6.35, flutes=2, presets=(), shape_id="endmill")
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.source, "")
        self.assertEqual(r.confidence, 0.0)
        self.assertIsNone(r.horiz_feed)

    def test_matching_preset_produces_derived_values(self):
        r = resolve(self.tool, self.alu, self.profile)
        self.assertNotEqual(r.source, "")
        # rpm = (surface_speed * 1000) / (pi * d) = (400 * 1000) / (pi * 6.35)
        expected_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertAlmostEqual(r.spindle_speed, expected_rpm, places=2)
        # horiz_feed = rpm * flutes * chipload
        expected_feed = expected_rpm * 2 * 0.05
        self.assertAlmostEqual(r.horiz_feed, expected_feed, places=2)
        # vert_feed = horiz * 0.33
        self.assertAlmostEqual(r.vert_feed, expected_feed * 0.33, places=2)
        self.assertEqual(r.surface_speed, 400.0)
        self.assertEqual(r.chipload, 0.05)

    def test_material_mismatch_returns_no_match(self):
        r = resolve(self.tool, self.steel, self.profile)
        self.assertEqual(r.source, "")

    def test_op_type_mismatch_returns_no_match(self):
        r = resolve(self.tool, self.alu, self.pocket)
        self.assertEqual(r.source, "")

    def test_specificity_picks_more_specific_preset(self):
        # Two presets: one material-only generic-op, one fully specific.
        # The fully specific one should win.
        material_only = make_preset(
            material_uuid=ALU_UUID,
            material_name="Aluminum 6061",
            op_type=None,
            surface_speed=300.0,
            chipload=0.04,
        )
        full = _alu_profile_preset()
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(material_only, full),
            shape_id="endmill",
        )
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.surface_speed, 400.0)
        self.assertEqual(r.chipload, 0.05)

    def test_generic_material_preset_matches_anything(self):
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(_generic_drill_preset(),),
            shape_id="drill",
        )
        r = resolve(tool, self.steel, OpContext(op_type="drill"))
        self.assertNotEqual(r.source, "")
        self.assertEqual(r.surface_speed, 200.0)

    def test_name_fallback_when_uuid_unknown(self):
        # Preset has UUID; query has only the same name.
        material_unknown_uuid = MaterialContext(uuid=None, name="Aluminum 6061")
        r = resolve(self.tool, material_unknown_uuid, self.profile)
        self.assertNotEqual(r.source, "")
        # Name-fallback should score lower than UUID-exact (~0.85 vs 0.95).
        self.assertLess(r.confidence, 0.96)

    def test_uuid_match_outscores_name_match(self):
        full_uuid = _alu_profile_preset()
        full_name_only = make_preset(
            material_uuid=None,
            material_name="Aluminum 6061",
            op_type="profile",
            surface_speed=999.0,
            chipload=0.05,
        )
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(full_name_only, full_uuid),
            shape_id="endmill",
        )
        r = resolve(tool, self.alu, self.profile)
        # UUID-matched preset wins
        self.assertEqual(r.surface_speed, 400.0)

    def test_preset_with_no_engineering_values_warns(self):
        # A preset with neither surface_speed nor chipload set is
        # effectively useless. The provider returns a result with no
        # spindle/feed and a warning.
        empty = make_preset(
            material_uuid=ALU_UUID, material_name="Aluminum 6061", op_type="profile"
        )
        tool = ToolContext(diameter=6.35, flutes=2, presets=(empty,), shape_id="endmill")
        r = resolve(tool, self.alu, self.profile)
        self.assertIsNone(r.spindle_speed)
        self.assertIsNone(r.horiz_feed)
        self.assertTrue(any("no usable" in w for w in r.warnings))

    def test_score_preset_returns_none_on_mismatch(self):
        score = _score_preset(
            _alu_profile_preset(),
            MaterialContext(uuid=STEEL_UUID, name="Mild Steel"),
            OpContext(op_type="profile"),
        )
        self.assertIsNone(score)

    def test_op_type_none_in_query_treats_op_hint_as_mismatch(self):
        # Query has op_type=None but preset specifies "profile". Per the
        # scoring rule, a specified hint that doesn't match the query is
        # treated as a mismatch.
        r = resolve(self.tool, self.alu, OpContext(op_type=None))
        self.assertEqual(r.source, "")

    def test_resolver_does_not_import_freecad(self):
        # Sanity check: the resolver module should be importable without
        # FreeCAD. We can't fully prove that here (we're inside FreeCAD), but
        # we can at least confirm the module's __dict__ has no FreeCAD ref.
        from Path.Tool.FeedsSpeeds import resolver, types, providers

        for mod in (resolver, types, providers):
            self.assertNotIn("FreeCAD", dir(mod), f"{mod.__name__} leaks FreeCAD")
            self.assertNotIn("Path", dir(mod), f"{mod.__name__} leaks Path")

    def test_make_preset_includes_name_field(self):
        preset = make_preset(name="Test", surface_speed=400)
        self.assertEqual(preset["name"], "Test")
        # Default is None when omitted
        self.assertIsNone(make_preset(surface_speed=400)["name"])

    def test_derive_preset_label_prefers_user_name(self):
        preset = make_preset(name="My fav", material_name="Aluminum 6061", op_type="profile")
        self.assertEqual(derive_preset_label(preset), "My fav")

    def test_derive_preset_label_falls_back_to_material_op(self):
        preset = make_preset(material_name="Aluminum 6061", op_type="profile")
        self.assertEqual(derive_preset_label(preset), "Aluminum 6061 / profile")

    def test_derive_preset_label_default_for_fully_generic(self):
        preset = make_preset(surface_speed=400)
        self.assertEqual(derive_preset_label(preset), "(unnamed)")

    # --- ToolDefaultsProvider (legacy ToolBit.Chipload fallback) -----------

    def test_tool_defaults_returns_chipload_when_no_preset_matches(self):
        # Material/op don't match any preset, but the tool has a default
        # chipload set on it. Resolver chain should still return the chipload.
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(),
            shape_id="endmill",
            chipload_default=0.04,
        )
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.chipload, 0.04)
        self.assertEqual(r.source, "tool_defaults:chipload")
        self.assertLess(r.confidence, 0.2)
        # No surface_speed provided by this provider alone.
        self.assertIsNone(r.surface_speed)
        self.assertIsNone(r.spindle_speed)

    def test_preset_match_outscores_tool_defaults(self):
        # Preset matches AND tool has a default chipload — preset wins.
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(_alu_profile_preset(),),
            shape_id="endmill",
            chipload_default=0.99,  # would be obviously wrong if used
        )
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.chipload, 0.05)  # from preset
        self.assertTrue(r.source.startswith("preset:"))

    def test_tool_defaults_silent_when_chipload_default_is_zero(self):
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(),
            shape_id="endmill",
            chipload_default=0.0,
        )
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.source, "")  # no provider fired
        self.assertIsNone(r.chipload)

    def test_tool_defaults_silent_when_chipload_default_is_none(self):
        # Default ToolContext has chipload_default=None.
        tool = ToolContext(diameter=6.35, flutes=2, presets=(), shape_id="endmill")
        r = resolve(tool, self.alu, self.profile)
        self.assertEqual(r.source, "")

    # --- MachinabilityProvider (stock-material surface-speed lookup) -------

    def _tool_no_presets(self, tool_material=None, chipload_default=None):
        return ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(),
            shape_id="endmill",
            tool_material=tool_material,
            chipload_default=chipload_default,
        )

    def _alu_machinability(self, hss=30.0, carbide=400.0):
        return MaterialContext(
            uuid=ALU_UUID,
            name="Aluminum 6061",
            surface_speed_hss=hss,
            surface_speed_carbide=carbide,
        )

    def test_machinability_carbide_branch(self):
        tool = self._tool_no_presets(tool_material="Carbide")
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertEqual(r.surface_speed, 400.0)
        self.assertTrue(r.source.startswith("machinability:"))
        self.assertIn("/carbide", r.source)

    def test_machinability_hss_branch(self):
        tool = self._tool_no_presets(tool_material="HSS")
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertEqual(r.surface_speed, 30.0)
        self.assertIn("/hss", r.source)

    def test_machinability_abstains_when_tool_material_missing(self):
        tool = self._tool_no_presets(tool_material=None)
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertEqual(r.source, "")

    def test_machinability_abstains_on_unknown_tool_material(self):
        tool = self._tool_no_presets(tool_material="Diamond")
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertEqual(r.source, "")

    def test_machinability_abstains_when_branch_value_missing(self):
        # HSS tool, but material only has Carbide surface speed.
        tool = self._tool_no_presets(tool_material="HSS")
        material = MaterialContext(
            uuid=ALU_UUID,
            name="Aluminum 6061",
            surface_speed_hss=None,
            surface_speed_carbide=400.0,
        )
        r = resolve(tool, material, self.profile)
        self.assertEqual(r.source, "")

    def test_machinability_plus_defaults_derives_full_result(self):
        # Stock material gives surface_speed, tool's default Chipload gives
        # chipload; resolver finalization derives spindle and feed.
        tool = self._tool_no_presets(tool_material="Carbide", chipload_default=0.05)
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertEqual(r.surface_speed, 400.0)
        self.assertEqual(r.chipload, 0.05)
        expected_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertAlmostEqual(r.spindle_speed, expected_rpm, places=2)
        self.assertAlmostEqual(r.horiz_feed, expected_rpm * 2 * 0.05, places=2)
        # Default vert_feed_ratio in finalization is 0.33.
        self.assertAlmostEqual(r.vert_feed, expected_rpm * 2 * 0.05 * 0.33, places=2)

    def test_preset_match_outscores_machinability(self):
        # Curated preset and machinability both available — preset wins.
        tool = ToolContext(
            diameter=6.35,
            flutes=2,
            presets=(_alu_profile_preset(),),
            shape_id="endmill",
            tool_material="Carbide",
        )
        # Material's carbide speed is intentionally different from preset's.
        material = self._alu_machinability(carbide=999.0)
        r = resolve(tool, material, self.profile)
        self.assertEqual(r.surface_speed, 400.0)  # preset's value
        self.assertTrue(r.source.startswith("preset:"))

    def test_machinability_confidence_between_preset_and_defaults(self):
        tool = self._tool_no_presets(tool_material="Carbide")
        r = resolve(tool, self._alu_machinability(), self.profile)
        self.assertGreater(r.confidence, 0.10)
        self.assertLess(r.confidence, 0.55)

    # --- Machine clamping (spindle min/max from Machine definition) -------

    def test_machine_no_clamp_when_within_limits(self):
        # Preset yields ~20053 rpm at d=6.35; machine max 30000 well above.
        machine = MachineContext(min_rpm=0, max_rpm=30000)
        r = resolve(self.tool, self.alu, self.profile, machine=machine)
        expected_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertAlmostEqual(r.spindle_speed, expected_rpm, places=2)
        self.assertFalse(any("clamped" in w.lower() for w in r.warnings))

    def test_machine_clamps_above_max(self):
        # Preset yields ~20053 rpm; cap at 10000.
        machine = MachineContext(min_rpm=0, max_rpm=10000)
        r = resolve(self.tool, self.alu, self.profile, machine=machine)
        original_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertEqual(r.spindle_speed, 10000)
        # Feeds scaled by ratio (chipload preserved).
        ratio = 10000 / original_rpm
        original_horiz = original_rpm * 2 * 0.05
        self.assertAlmostEqual(r.horiz_feed, original_horiz * ratio, places=2)
        self.assertEqual(r.chipload, 0.05)
        # Recommended surface_speed unchanged (intent, not achieved).
        self.assertEqual(r.surface_speed, 400.0)
        self.assertTrue(any("max" in w.lower() and "clamped" in w.lower() for w in r.warnings))

    def test_machine_clamps_below_min(self):
        # Preset yields ~20053 rpm; force min at 25000 so it has to climb.
        machine = MachineContext(min_rpm=25000, max_rpm=40000)
        r = resolve(self.tool, self.alu, self.profile, machine=machine)
        self.assertEqual(r.spindle_speed, 25000)
        self.assertTrue(any("min" in w.lower() and "clamped" in w.lower() for w in r.warnings))

    def test_machine_zero_max_means_no_limit(self):
        # max_rpm=0 (Machine model's default) disables the upper clamp.
        machine = MachineContext(min_rpm=0, max_rpm=0)
        r = resolve(self.tool, self.alu, self.profile, machine=machine)
        expected_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertAlmostEqual(r.spindle_speed, expected_rpm, places=2)
        self.assertFalse(any("clamped" in w.lower() for w in r.warnings))

    def test_machine_none_no_clamping(self):
        # No machine context = no clamping path runs.
        r = resolve(self.tool, self.alu, self.profile, machine=None)
        expected_rpm = (400.0 * 1000.0) / (math.pi * 6.35)
        self.assertAlmostEqual(r.spindle_speed, expected_rpm, places=2)

    def test_machine_clamps_machinability_derived_spindle(self):
        # Machinability + defaults path also runs through finalization
        # and so is subject to the same clamp.
        tool = self._tool_no_presets(tool_material="Carbide", chipload_default=0.05)
        machine = MachineContext(min_rpm=0, max_rpm=10000)
        r = resolve(tool, self._alu_machinability(), self.profile, machine=machine)
        self.assertEqual(r.spindle_speed, 10000)
        self.assertTrue(any("clamped" in w.lower() for w in r.warnings))
