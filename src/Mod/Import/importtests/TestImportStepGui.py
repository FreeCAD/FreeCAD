# SPDX-License-Identifier: LGPL-2.1-or-later

# **************************************************************************
#   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

"""GUI-based STEP import/export tests.

This file contains the STEP color-per-face test previously located at the
module root, adapted to inherit from `TestImportBaseGui`.
"""

import os
import tempfile
import ImportGui
from pivy import coin
from importtests.TestImportBaseGui import TestImportBaseGui
import FreeCAD as App


class TestImportStepGui(TestImportBaseGui):
    def setUp(self):
        super().setUp()
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "ColorPerFaceTest.step"
        # self.doc is created by the base class and exposed as self.document
        # keep API compatibility with old tests by also setting self.doc
        self.doc = self.document

    def tearDown(self):
        # base class tearDown will close document
        super().tearDown()

    def test_save_load_step_file(self):
        """
        Create a STEP file with color per face
        """
        part = self.doc.addObject("App::Part", "Part")
        box = part.newObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
        ]

        ImportGui.export([part], self.fileName)

        self.doc.clearDocument()
        ImportGui.insert(name=self.fileName, docName=self.doc.Name, merge=False, useLinkGroup=True)

        part_features = list(filter(lambda x: x.isDerivedFrom("Part::Feature"), self.doc.Objects))
        self.assertEqual(len(part_features), 1)
        feature = part_features[0]

        self.assertEqual(len(feature.ViewObject.DiffuseColor), 6)
        self.assertEqual(feature.ViewObject.DiffuseColor[0], (1.0, 0.0, 0.0, 1.0))
        self.assertEqual(feature.ViewObject.DiffuseColor[1], (1.0, 0.0, 0.0, 1.0))
        self.assertEqual(feature.ViewObject.DiffuseColor[2], (1.0, 0.0, 0.0, 1.0))
        self.assertEqual(feature.ViewObject.DiffuseColor[3], (1.0, 0.0, 0.0, 1.0))
        self.assertEqual(feature.ViewObject.DiffuseColor[4], (1.0, 1.0, 0.0, 1.0))
        self.assertEqual(feature.ViewObject.DiffuseColor[5], (1.0, 1.0, 0.0, 1.0))

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)
