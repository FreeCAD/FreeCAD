# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run with FreeCADCmd -t TestPropertyFlags."""

import unittest

import FreeCAD as App


class TestPropertyFlags(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestPropertyFlags")
        self.obj = self.doc.addObject("App::VarSet", "VarSet")
        self.obj.addProperty("App::PropertyBool", "Test")

    def tearDown(self):
        if self.doc is not None:
            App.closeDocument(self.doc.Name)

    def test(self):
        FlagList = [ "Touched", "Immutable", "ReadOnly", "Hidden", "Transient", "MaterialEdit", 
            "NoMaterialListEdit", "Output", "Input", "LockDynamic", "NoModify", "PartialTrigger", 
            "NoRecompute", "CopyOnChange", "UserEdit", "DisableNotify", "PropStaticBegin", "Busy",  
            "PropDynamic", "PropNoPersist", "PropNoRecompute", "PropReadOnly", "PropTransient", 
            "PropHidden", "PropOutput", "PropInput", "PropStaticEnd", "User1", "User2", "User3", ]

        SpecialFlags = [ "PropStaticBegin", "PropDynamic", "PropNoRecompute", "PropReadOnly", "PropTransient", 
            "PropOutput", "PropHidden", "PropNoPersist", "Busy", "User1",  ]

        SettableFlags = [ i for i in FlagList if i not in SpecialFlags ]

        negativeFlagList = [ '-'+i for i in FlagList]

        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic"])
        self.obj.Test = False

        self.assertEqual(self.obj.getPropertyStatus("Test"), ["Touched", "PropDynamic"])
        self.obj.setPropertyStatus("Test", ["-Touched"])
        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic"])

        self.obj.setPropertyStatus("Test", ["User3"])
        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic", "User3"])

        self.obj.setPropertyStatus("Test", ["-User3"])
        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic"])

        self.obj.setPropertyStatus("Test", [31])
        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic", "User3"])

        self.obj.setPropertyStatus("Test", [-31])
        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic"])

        with self.assertRaises(ValueError):
            self.obj.setPropertyStatus("Test", ["User4"])
            raise RuntimeError("Expected FreeCAD failure")

        with self.assertRaises(ValueError):
            self.obj.setPropertyStatus("Test", [33])
            raise RuntimeError("Expected FreeCAD failure")

        self.assertEqual(self.obj.getPropertyStatus("Test"), ["PropDynamic"])

        self.obj.setPropertyStatus("Test", FlagList)
        ret = self.obj.getPropertyStatus("Test")
        for i in SettableFlags:
            self.assertTrue( i in ret, msg=f"{i}")

        self.obj.setPropertyStatus("Test", negativeFlagList)

        ret = self.obj.getPropertyStatus("Test")
        for i in SettableFlags:
            self.assertTrue( i not in ret, msg=f"{i}")


        # try to set/clear forbidden bits
        ret = [ i for i in self.obj.getPropertyStatus("Test") if i in SpecialFlags ]
        self.obj.setPropertyStatus("Test", SpecialFlags)
        ret2 = [ i for i in self.obj.getPropertyStatus("Test") if i in SpecialFlags ]
        self.assertEqual(ret, ret2)

        negativeSpecialFlags = [ '-'+i for i in SpecialFlags ]
        ret = [ i for i in self.obj.getPropertyStatus("Test") if i in SpecialFlags ]
        self.obj.setPropertyStatus("Test", negativeSpecialFlags)
        ret2 = [ i for i in self.obj.getPropertyStatus("Test") if i in SpecialFlags ]
        self.assertEqual(ret, ret2)

