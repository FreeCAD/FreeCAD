# SPDX-License-Identifier: LGPL-2.1-or-later

# **************************************************************************
#                                                                         *
#   This file is part of FreeCAD.                                         *
#                                                                         *
#   FreeCAD is free software: you can redistribute it and/or modify it    *
#   under the terms of the GNU Lesser General Public License as           *
#   published by the Free Software Foundation, either version 2.1 of the  *
#   License, or (at your option) any later version.                       *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful, but        *
#   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#   Lesser General Public License for more details.                       *
#                                                                         *
#   You should have received a copy of the GNU Lesser General Public      *
#   License along with FreeCAD. If not, see                               *
#   <https://www.gnu.org/licenses/>.                                      *
#                                                                         *
# **************************************************************************

import os
import re
import tempfile
import unittest
import FreeCAD as App
import Import


class StepExportColorTest(unittest.TestCase):
    def setUp(self):
        self.fileName = os.path.join(tempfile.gettempdir(), "ImportStepExportColorTest.step")
        self.doc = App.newDocument()

    def tearDown(self):
        App.closeDocument(self.doc.Name)
        if os.path.exists(self.fileName):
            os.remove(self.fileName)

    def testOutOfRangeAlphaIsClamped(self):
        """An out-of-range alpha must not reach OCCT unchecked (#33758)"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        # Alpha of 1 - 60 (a transparency given in percent instead of 0..1)
        Import.export([(box, [(0.6, 0.62, 0.7, -59.0)] * 6)], self.fileName)

        with open(self.fileName, encoding="utf-8") as f:
            content = f.read()
        values = [float(v) for v in re.findall(r"SURFACE_STYLE_TRANSPARENT\(([^)]*)\)", content)]
        self.assertTrue(values)
        for value in values:
            self.assertGreaterEqual(value, 0.0)
            self.assertLessEqual(value, 1.0)
