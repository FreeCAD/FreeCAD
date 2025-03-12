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

import os
import tempfile
import unittest
import FreeCAD as App
import ImportGui
from pivy import coin


class ExportImportTest(unittest.TestCase):
    def setUp(self):
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "ColorPerFaceTest.step"
        self.doc = App.newDocument()

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testSaveLoadStepFile(self):
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
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(2).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)
