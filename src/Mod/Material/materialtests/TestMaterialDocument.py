# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import unittest
import FreeCAD

class DocumentTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material tests that need a document
    """

    def setUp(self):
        self.doc = FreeCAD.newDocument()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testApplyDiffuseColorCheckShapeAppearance(self):
        """ Test that applying a DiffuseColor with transparency results in a correct ShapeAppearance """
        if "BUILD_PART" in FreeCAD.__cmake__:
            dif_col_1 = (1.0, 1.0, 0.0, 1.0)  # yellow 0% transparent
            dif_col_2 = (1.0, 0.0, 0.0, 0.5)  # red 50% transparent
            dif_col = [dif_col_1] + [dif_col_2] + 4 * [dif_col_1]

            obj = self.doc.addObject("Part::Box")
            vobj = obj.ViewObject
            vobj.DiffuseColor = dif_col

            self.assertEqual(
                [m.DiffuseColor[:3] + (1.0 - m.Transparency, ) for m in vobj.ShapeAppearance],
                vobj.DiffuseColor
            )

    def testApplyShapeAppearanceCheckDiffuseColor(self):
        """ Test that applying a ShapeAppearance with transparency results in a correct DiffuseColor """
        if "BUILD_PART" in FreeCAD.__cmake__:
            sapp_1 = FreeCAD.Material()
            sapp_1.DiffuseColor = (0.0, 1.0, 1.0, 0.0)  # cyan
            sapp_1.Transparency = 0.0                   # 0% transparent
            sapp_2 = FreeCAD.Material()
            sapp_2.DiffuseColor = (0.0, 1.0, 0.0, 0.0)  # green
            sapp_2.Transparency = 0.3                   # 30% transparent
            sapp = [sapp_1] + [sapp_2] + 4 * [sapp_1]

            obj = self.doc.addObject("Part::Box")
            vobj = obj.ViewObject
            vobj.ShapeAppearance = sapp

            self.assertEqual(
                [m.DiffuseColor[:3] + (1.0 - m.Transparency, ) for m in vobj.ShapeAppearance],
                vobj.DiffuseColor
            )
