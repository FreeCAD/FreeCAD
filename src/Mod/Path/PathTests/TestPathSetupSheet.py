# -*- coding: utf-8 -*-
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
import Path
import Path.Base.SetupSheet as PathSetupSheet
import json
import sys

# Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())

from PathTests.PathTestUtils import PathTestBase


def refstring(string):
    return string.replace(" u'", " '")


class SomeOp(object):
    def __init__(self, obj):
        Path.Log.track(obj, type(obj))
        obj.addProperty("App::PropertyPercent", "StepOver", "Base", "Some help you are")

    @classmethod
    def SetupProperties(cls):
        return ["StepOver"]

    @classmethod
    def Create(cls, name, obj=None, parentJob=None):
        Path.Log.track(name, obj)
        if obj is None:
            obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
        obj.Proxy = SomeOp(obj)
        return obj


class TestPathSetupSheet(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathSetupSheet")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        """Verify SetupSheet templateAttributes"""
        ss = PathSetupSheet.Create().Proxy
        self.doc.recompute()

        attrs = ss.templateAttributes(True, True)

        self.assertEqualLocale(attrs[PathSetupSheet.Template.HorizRapid], "0.00 mm/s")
        self.assertEqualLocale(attrs[PathSetupSheet.Template.VertRapid], "0.00 mm/s")
        self.assertEqualLocale(
            attrs[PathSetupSheet.Template.SafeHeightOffset], "3.00 mm"
        )
        self.assertEqual(
            attrs[PathSetupSheet.Template.SafeHeightExpression],
            "OpStockZMax+SetupSheet.SafeHeightOffset",
        )
        self.assertEqualLocale(
            attrs[PathSetupSheet.Template.ClearanceHeightOffset], "5.00 mm"
        )
        self.assertEqual(
            attrs[PathSetupSheet.Template.ClearanceHeightExpression],
            "OpStockZMax+SetupSheet.ClearanceHeightOffset",
        )

    def test01(self):
        """Verify SetupSheet template attributes roundtrip."""
        o1 = PathSetupSheet.Create()
        self.doc.recompute()
        o1.VertRapid = "10 mm/s"
        o1.HorizRapid = "22 mm/s"
        o1.SafeHeightOffset = "18 mm"
        o1.SafeHeightExpression = "Hugo+Olga"
        o1.ClearanceHeightOffset = "23 mm"
        o1.ClearanceHeightExpression = "Peter+Paul"
        o1.StartDepthExpression = "Alpha"
        o1.FinalDepthExpression = "Omega"
        o1.StepDownExpression = "1"

        o2 = PathSetupSheet.Create()
        self.doc.recompute()
        o2.Proxy.setFromTemplate(o1.Proxy.templateAttributes())
        self.doc.recompute()

        # Need to compare the UserString's due to rounding errors depending on the
        # user's unit settings - should have no impact on the validity

        self.assertEqual(o1.VertRapid.UserString, o2.VertRapid.UserString)
        self.assertEqual(o1.HorizRapid.UserString, o2.HorizRapid.UserString)
        self.assertEqual(o1.SafeHeightOffset.UserString, o2.SafeHeightOffset.UserString)
        self.assertEqual(o1.SafeHeightExpression, o2.SafeHeightExpression)
        self.assertEqual(
            o1.ClearanceHeightOffset.UserString, o2.ClearanceHeightOffset.UserString
        )
        self.assertEqual(o1.ClearanceHeightExpression, o2.ClearanceHeightExpression)
        self.assertEqual(o1.StartDepthExpression, o2.StartDepthExpression)
        self.assertEqual(o1.FinalDepthExpression, o2.FinalDepthExpression)
        self.assertEqual(o1.StepDownExpression, o2.StepDownExpression)

    def test02(self):
        """Verify default value detection logic."""
        obj = PathSetupSheet.Create()
        ss = obj.Proxy

        self.assertTrue(ss.hasDefaultToolRapids())
        self.assertTrue(ss.hasDefaultOperationHeights())
        self.assertTrue(ss.hasDefaultOperationDepths())

        obj.VertRapid = "1 mm/s"
        self.assertFalse(ss.hasDefaultToolRapids())
        obj.VertRapid = "0 mm/s"
        self.assertTrue(ss.hasDefaultToolRapids())
        obj.HorizRapid = "1 mm/s"
        self.assertFalse(ss.hasDefaultToolRapids())
        obj.HorizRapid = "0 mm/s"
        self.assertTrue(ss.hasDefaultToolRapids())

        obj.SafeHeightOffset = "0 mm"
        self.assertFalse(ss.hasDefaultOperationHeights())
        obj.SafeHeightOffset = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultSafeHeightOffset
        )
        self.assertTrue(ss.hasDefaultOperationHeights())
        obj.ClearanceHeightOffset = "0 mm"
        self.assertFalse(ss.hasDefaultOperationHeights())
        obj.ClearanceHeightOffset = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultClearanceHeightOffset
        )
        self.assertTrue(ss.hasDefaultOperationHeights())

        obj.SafeHeightExpression = "0 mm"
        self.assertFalse(ss.hasDefaultOperationHeights())
        obj.SafeHeightExpression = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultSafeHeightExpression
        )
        self.assertTrue(ss.hasDefaultOperationHeights())
        obj.ClearanceHeightExpression = "0 mm"
        self.assertFalse(ss.hasDefaultOperationHeights())
        obj.ClearanceHeightExpression = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultClearanceHeightExpression
        )
        self.assertTrue(ss.hasDefaultOperationHeights())

        obj.StartDepthExpression = ""
        self.assertFalse(ss.hasDefaultOperationDepths())
        obj.StartDepthExpression = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultStartDepthExpression
        )
        self.assertTrue(ss.hasDefaultOperationDepths())
        obj.FinalDepthExpression = ""
        self.assertFalse(ss.hasDefaultOperationDepths())
        obj.FinalDepthExpression = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultFinalDepthExpression
        )
        self.assertTrue(ss.hasDefaultOperationDepths())
        obj.StepDownExpression = ""
        self.assertFalse(ss.hasDefaultOperationDepths())
        obj.StepDownExpression = ss.decodeAttributeString(
            PathSetupSheet.SetupSheet.DefaultStepDownExpression
        )
        self.assertTrue(ss.hasDefaultOperationDepths())

    def test10(self):
        """Verify template attributes encoding/decoding of floats."""
        ss = PathSetupSheet.Create().Proxy

        self.assertEqual(ss.expressionReference(), "SetupSheet")

        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": 13.00})), "{'00': 13.0}"
        )
        self.assertEqual(
            str(ss.decodeTemplateAttributes({"00": 13.00})), "{'00': 13.0}"
        )

    def test11(self):
        """Verify template attributes encoding/decoding of strings."""
        ss = PathSetupSheet.Create().Proxy

        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": "hugo"})),
            refstring("{'00': u'hugo'}"),
        )
        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": "SetupSheet"})),
            refstring("{'00': u'${SetupSheet}'}"),
        )
        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": "SetupSheet.y"})),
            refstring("{'00': u'${SetupSheet}.y'}"),
        )
        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": "${hugo}"})),
            refstring("{'00': u'${hugo}'}"),
        )

        self.assertEqual(
            str(ss.decodeTemplateAttributes({"00": "hugo"})),
            refstring("{'00': u'hugo'}"),
        )
        self.assertEqual(
            str(ss.decodeTemplateAttributes({"00": "${SetupSheet}"})),
            refstring("{'00': u'SetupSheet'}"),
        )
        self.assertEqual(
            str(ss.decodeTemplateAttributes({"00": "${SetupSheet}.y"})),
            refstring("{'00': u'SetupSheet.y'}"),
        )
        self.assertEqual(
            str(
                ss.decodeTemplateAttributes({"00": "${SetupSheet}.y - ${SetupSheet}.z"})
            ),
            refstring("{'00': u'SetupSheet.y - SetupSheet.z'}"),
        )

    def test12(self):
        """Verify template attributes encoding/decoding of dictionaries."""
        ss = PathSetupSheet.Create().Proxy

        self.assertEqual(
            str(ss.encodeTemplateAttributes({"00": {"01": "hugo"}})),
            refstring("{'00': {'01': u'hugo'}}"),
        )
        self.assertEqual(
            str(
                ss.encodeTemplateAttributes(
                    {"00": {"01": "SetupSheet.y - SetupSheet.z"}}
                )
            ),
            refstring("{'00': {'01': u'${SetupSheet}.y - ${SetupSheet}.z'}}"),
        )

        self.assertEqual(
            str(ss.decodeTemplateAttributes({"00": {"01": "hugo"}})),
            refstring("{'00': {'01': u'hugo'}}"),
        )
        self.assertEqual(
            str(
                ss.decodeTemplateAttributes(
                    {"00": {"01": "${SetupSheet}.y - ${SetupSheet}.z"}}
                )
            ),
            refstring("{'00': {'01': u'SetupSheet.y - SetupSheet.z'}}"),
        )

    def test13(self):
        """Verify template attributes encoding/decoding of lists."""
        ss = PathSetupSheet.Create().Proxy

        attrs = {}
        attrs["00"] = "x.SetupSheet"
        attrs["01"] = [{"10": "SetupSheet", "11": "SetupSheet.y"}, {"20": "SetupSheet"}]
        attrs["02"] = [
            {
                "a": [{"b": "SetupSheet"}, {"c": "SetupSheet"}],
                "b": [{"b": "SetupSheet"}],
            }
        ]

        encoded = ss.encodeTemplateAttributes(attrs)
        self.assertEqual(encoded["00"], "x.${SetupSheet}")
        self.assertEqual(len(encoded["01"]), 2)
        self.assertEqual(encoded["01"][0]["10"], "${SetupSheet}")
        self.assertEqual(encoded["01"][0]["11"], "${SetupSheet}.y")
        self.assertEqual(str(encoded["01"][1]), refstring("{'20': u'${SetupSheet}'}"))
        self.assertEqual(len(encoded["02"]), 1)
        self.assertEqual(len(encoded["02"][0]["a"]), 2)
        self.assertEqual(
            str(encoded["02"][0]["a"][0]), refstring("{'b': u'${SetupSheet}'}")
        )
        self.assertEqual(
            str(encoded["02"][0]["a"][1]), refstring("{'c': u'${SetupSheet}'}")
        )
        self.assertEqual(len(encoded["02"][0]["b"]), 1)
        self.assertEqual(
            str(encoded["02"][0]["b"][0]), refstring("{'b': u'${SetupSheet}'}")
        )

        decoded = ss.decodeTemplateAttributes(encoded)
        self.assertEqual(len(decoded), len(attrs))
        self.assertEqual(decoded["00"], attrs["00"])
        self.assertEqual(len(decoded["01"]), len(attrs["01"]))
        self.assertEqual(decoded["01"][0]["10"], attrs["01"][0]["10"])
        self.assertEqual(decoded["01"][0]["11"], attrs["01"][0]["11"])
        self.assertEqual(decoded["01"][1]["20"], attrs["01"][1]["20"])
        self.assertEqual(len(decoded["02"]), len(attrs["02"]))
        self.assertEqual(len(decoded["02"][0]["a"]), len(attrs["02"][0]["a"]))
        self.assertEqual(decoded["02"][0]["a"][0]["b"], attrs["02"][0]["a"][0]["b"])
        self.assertEqual(decoded["02"][0]["a"][1]["c"], attrs["02"][0]["a"][1]["c"])
        self.assertEqual(len(decoded["02"][0]["b"]), len(attrs["02"][0]["b"]))
        self.assertEqual(decoded["02"][0]["b"][0]["b"], attrs["02"][0]["b"][0]["b"])

        # just to be safe ...
        s2 = PathSetupSheet.Create().Proxy
        self.doc.recompute()
        s2.setFromTemplate(ss.templateAttributes())
        self.assertEqual(s2.expressionReference(), "SetupSheet001")
        dec = s2.decodeTemplateAttributes(encoded)
        # pick one
        self.assertEqual(dec["01"][0]["11"], "SetupSheet001.y")

    def test20(self):
        """Verify SetupSheet template op attributes roundtrip."""

        opname = "whoop"

        o1 = PathSetupSheet.Create()

        PathSetupSheet.RegisterOperation(opname, SomeOp.Create, SomeOp.SetupProperties)
        ptt = PathSetupSheet._RegisteredOps[opname].prototype("whoopsy")
        pptt = ptt.getProperty("StepOver")
        pptt.setupProperty(
            o1,
            PathSetupSheet.OpPropertyName(opname, pptt.name),
            PathSetupSheet.OpPropertyGroup(opname),
            75,
        )

        # save setup sheet in json "file"
        attrs = o1.Proxy.templateAttributes(False, False, False, False, [opname])
        encdd = o1.Proxy.encodeTemplateAttributes(attrs)
        j1 = json.dumps({"SetupSheet": encdd}, sort_keys=True, indent=2)

        # restore setup sheet from json "file"
        j2 = json.loads(j1)

        o2 = PathSetupSheet.Create()
        o2.Proxy.setFromTemplate(j2["SetupSheet"])

        op = SomeOp.Create(opname)
        self.assertEqual(op.StepOver, 0)
        o2.Proxy.setOperationProperties(op, opname)
        self.assertEqual(op.StepOver, 75)
