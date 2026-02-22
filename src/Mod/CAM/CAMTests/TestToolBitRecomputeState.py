# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2026 Chris Jones github.com/ipatch                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# ***************************************************************************

import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.toolbit import ToolBitEndmill


class TestToolBitRecomputeState(PathTestWithAssets):
    """Test toolbit recompute state handling (issue #26652)"""

    def setUp(self):
        """Set up test fixtures"""
        super().setUp()
        self.doc = FreeCAD.newDocument("TestToolbitState")

    def tearDown(self):
        """Clean up after tests"""
        FreeCAD.closeDocument(self.doc.Name)
        super().tearDown()

    def testToolbitExpressionRecomputeState(self):
        """Test that toolbit with unitless expression doesn't stay touched after recompute"""
        # create an endmill shape and toolbit
        shape = self.assets.get("toolbitshape://endmill")
        toolbit = ToolBitEndmill(shape, id="test_endmill")

        # attach to document
        obj = toolbit.attach_to_doc(self.doc, label="TestEndmill")

        # set initial diameter
        obj.Diameter = FreeCAD.Units.Quantity("5 mm")
        self.doc.recompute()

        # now set expression without unit - this triggers issue #26652
        obj.setExpression("Diameter", "5")
        self.doc.recompute()

        # after recompute, toolbit should not be in touched state
        self.assertFalse(
            "Touched" in obj.State,
            "Toolbit with unitless expression should not be touched after recompute",
        )
