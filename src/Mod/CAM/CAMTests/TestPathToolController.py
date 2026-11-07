# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
from Path.Tool.toolbit import ToolBit
import Path.Tool.Controller as PathToolController
from CAMTests.PathTestUtils import PathTestBase


class TestPathToolController(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathToolController")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def createTool(self, name="t1", diameter=1.75):
        attrs = {
            "name": name or "t1",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": diameter},
            "attribute": {},
        }
        print(f"Debug: attrs['attribute'] is {attrs['attribute']}")
        toolbit = ToolBit.from_dict(attrs)
        return toolbit.attach_to_doc(doc=FreeCAD.ActiveDocument)

    def test00(self):
        """Verify ToolController templateAttrs"""
        t = self.createTool("T1")
        tc = PathToolController.Create("TC0", t)

        tc.Label = "ToolController"
        tc.ToolNumber = 7
        tc.VertFeed = "3 in/s"
        tc.VertFeed = round(tc.VertFeed, 1)
        tc.HorizFeed = "10 mm/s"
        tc.VertRapid = 40
        tc.HorizRapid = 28
        tc.SpindleDir = "Reverse"
        tc.SpindleSpeed = 12000

        attrs = tc.Proxy.templateAttrs(tc)

        self.assertEqual(attrs["name"], "TC0")
        self.assertEqual(attrs["label"], "ToolController")
        self.assertEqual(attrs["nr"], 7)
        self.assertEqual(attrs["vfeed"], "76.2 mm/s")
        self.assertEqual(attrs["hfeed"], "10.0 mm/s")
        self.assertEqual(attrs["vrapid"], "40.0 mm/s")
        self.assertEqual(attrs["hrapid"], "28.0 mm/s")
        self.assertEqual(attrs["dir"], "Reverse")
        self.assertEqual(attrs["speed"], 12000)
        self.assertEqual(attrs["tool"], t.Proxy.to_dict())

        return tc

    def test01(self):
        """Verify ToolController template roundtrip."""

        tc0 = self.test00()
        tc1 = PathToolController.FromTemplate(tc0.Proxy.templateAttrs(tc0))

        self.assertNotEqual(tc0.Name, tc1.Name)
        self.assertNotEqual(tc0.Label, tc1.Label)
        self.assertEqual(tc0.ToolNumber, tc1.ToolNumber)
        self.assertRoughly(tc0.VertFeed, tc1.VertFeed)
        self.assertRoughly(tc0.HorizFeed, tc1.HorizFeed)
        self.assertRoughly(tc0.VertRapid, tc1.VertRapid)
        self.assertRoughly(tc0.HorizRapid, tc1.HorizRapid)
        self.assertEqual(tc0.SpindleDir, tc1.SpindleDir)
        self.assertRoughly(tc0.SpindleSpeed, tc1.SpindleSpeed)
        # These are not valid because the name & label get adjusted if there
        # is a conflict. No idea how this could work with the C implementation
        # self.assertEqual(tc0.Tool.Name, tc1.Tool.Name)
        # self.assertEqual(tc0.Tool.Label, tc1.Tool.Label)
        self.assertRoughly(tc0.Tool.Diameter, tc1.Tool.Diameter)

    def testRampFeedModes(self):
        tc = PathToolController.Create("TC", self.createTool())
        self.assertEqual(tc.RampFeedMode, "Automatic")
        self.assertNotIn("RampFeed", dict(tc.ExpressionEngine))
        for mode in ("Custom", "Automatic"):
            tc.RampFeedMode = mode
            tc.RampFeed = 7.0
            restored = PathToolController.FromTemplate(tc.Proxy.templateAttrs(tc))
            self.assertEqual(restored.RampFeedMode, mode)
            self.assertEqual(restored.RampFeed.Value, 7.0)

    def testLegacyRampFeedMigration(self):
        tc = PathToolController.Create("TC", self.createTool())
        tc.removeProperty("RampFeedMode")
        tc.RampFeed = 7.0
        tc.Proxy.onDocumentRestored(tc)
        self.assertEqual(tc.RampFeedMode, "Custom")
        self.assertEqual(tc.RampFeed.Value, 7.0)

        tc.removeProperty("RampFeedMode")
        tc.setExpression("RampFeed", "HorizFeed / 2")
        tc.Proxy.onDocumentRestored(tc)
        self.assertEqual(tc.RampFeedMode, "Custom")
        self.assertEqual(dict(tc.ExpressionEngine)["RampFeed"], "HorizFeed / 2")

        tc.removeProperty("RampFeedMode")
        tc.removeProperty("RampFeed")
        tc.Proxy.onDocumentRestored(tc)
        self.assertEqual(tc.RampFeedMode, "Automatic")
        self.assertNotIn("RampFeed", dict(tc.ExpressionEngine))

    def testLegacyRampFeedTemplate(self):
        tc = PathToolController.Create("TC", self.createTool())
        tc.RampFeed = 7.0
        template = tc.Proxy.templateAttrs(tc)
        del template[PathToolController.ToolControllerTemplate.RampFeedMode]
        restored = PathToolController.FromTemplate(template)
        self.assertEqual(restored.RampFeedMode, "Custom")
        self.assertEqual(restored.RampFeed.Value, 7.0)

        del template[PathToolController.ToolControllerTemplate.RampFeed]
        restored = PathToolController.FromTemplate(template)
        self.assertEqual(restored.RampFeedMode, "Automatic")
