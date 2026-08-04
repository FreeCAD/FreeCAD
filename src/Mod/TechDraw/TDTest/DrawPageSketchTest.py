# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD <hello@astocad.com>                         *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import unittest

import FreeCAD

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawPageSketchTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDPageSketch")
        self.document = FreeCAD.ActiveDocument
        self.page = createPageWithSVGTemplate(self.document)
        self.sketch = self.document.addObject("Sketcher::SketchObject", "Sketch")

    def tearDown(self):
        FreeCAD.closeDocument("TDPageSketch")

    def testSketchCanBeAddedAndRemoved(self):
        self.page.addView(self.sketch)

        self.assertIn(self.sketch, self.page.Views)
        self.assertNotIn(self.sketch, self.page.getViews())

        self.page.removeView(self.sketch)
        self.assertNotIn(self.sketch, self.page.Views)
        self.assertIsNotNone(self.document.getObject("Sketch"))

    def testSketchCanBelongToViewPart(self):
        view = self.document.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(view)
        self.page.addView(self.sketch)

        view.Sketches = [self.sketch]

        self.assertIn(self.sketch, self.page.Views)
        self.assertIn(self.sketch, view.Sketches)

        view.Sketches = []
        self.assertIn(self.sketch, self.page.Views)
        self.assertNotIn(self.sketch, view.Sketches)


if __name__ == "__main__":
    unittest.main()
