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
Round-trip test for ToolBit feeds & speeds presets through the FCTB
serializer. Confirms:
- Lazy-add: a tool with no presets serializes byte-identical to before.
- The "presets" key appears only when presets exist.
- A round-trip preserves all preset fields.
"""

import json
from typing import cast
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.toolbit import ToolBit, ToolBitEndmill
from Path.Tool.toolbit.serializers import FCTBSerializer
from Path.Tool.assets.uri import AssetUri
from Path.Tool.shape import ToolBitShapeEndmill
from Path.Tool.FeedsSpeeds import (
    PRESETS_PROPERTY,
    get_presets,
    make_preset,
    set_presets,
)


class TestFeedsSpeedsToolBitPresets(PathTestWithAssets):

    def setUp(self):
        super().setUp()
        self.tool = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        self.tool.label = "Preset Roundtrip Test"

    def test_no_presets_means_no_presets_key_in_serialized(self):
        data = FCTBSerializer.serialize(self.tool)
        parsed = json.loads(data.decode("utf-8"))
        self.assertNotIn("presets", parsed)

    def test_presets_property_lazy_added_on_first_set(self):
        self.assertFalse(hasattr(self.tool.obj, PRESETS_PROPERTY))
        set_presets(self.tool.obj, [make_preset(surface_speed=300, chipload=0.04)])
        self.assertTrue(hasattr(self.tool.obj, PRESETS_PROPERTY))

    def test_get_presets_empty_when_property_absent(self):
        # Fresh tool, never had presets stored.
        self.assertEqual(get_presets(self.tool.obj), [])

    def test_serialize_includes_presets_when_set(self):
        preset = make_preset(
            material_uuid="uuid-aluminum",
            material_name="Aluminum 6061",
            op_type="profile",
            surface_speed=400.0,
            chipload=0.05,
        )
        set_presets(self.tool.obj, [preset])

        data = FCTBSerializer.serialize(self.tool)
        parsed = json.loads(data.decode("utf-8"))
        self.assertIn("presets", parsed)
        self.assertEqual(len(parsed["presets"]), 1)
        self.assertEqual(parsed["presets"][0]["surface_speed"], 400.0)
        self.assertEqual(parsed["presets"][0]["op_type_hint"], "profile")
        self.assertEqual(parsed["presets"][0]["material_hint"]["uuid"], "uuid-aluminum")

    def test_roundtrip_preserves_all_fields(self):
        preset_a = make_preset(
            name="Aluminum aggressive",
            material_uuid="uuid-aluminum",
            material_name="Aluminum 6061",
            op_type="profile",
            surface_speed=400.0,
            chipload=0.05,
            vert_feed_ratio=0.4,
        )
        preset_b = make_preset(
            op_type="drill",
            surface_speed=120.0,
            chipload=0.03,
        )
        set_presets(self.tool.obj, [preset_a, preset_b])

        data = FCTBSerializer.serialize(self.tool)
        shape = ToolBitShapeEndmill("endmill")
        deps = {AssetUri.build("toolbitshape", "endmill"): shape}
        round_trip = FCTBSerializer.deserialize(data, id="rt_id", dependencies=deps)

        restored = get_presets(round_trip.obj)
        self.assertEqual(len(restored), 2)
        self.assertEqual(restored[0]["name"], "Aluminum aggressive")
        self.assertEqual(restored[0]["surface_speed"], 400.0)
        self.assertEqual(restored[0]["chipload"], 0.05)
        self.assertEqual(restored[0]["vert_feed_ratio"], 0.4)
        self.assertEqual(restored[0]["material_hint"]["uuid"], "uuid-aluminum")
        self.assertIsNone(restored[1]["name"])
        self.assertEqual(restored[1]["op_type_hint"], "drill")
        self.assertEqual(restored[1]["surface_speed"], 120.0)
        self.assertEqual(restored[1]["chipload"], 0.03)
        self.assertIsNone(restored[1]["material_hint"])
        # Engineering-only storage: raw_feed/raw_speed are not persisted.
        self.assertNotIn("raw_feed", restored[1])
        self.assertNotIn("raw_speed", restored[1])
