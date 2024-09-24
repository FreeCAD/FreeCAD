#***************************************************************************
#*   Copyright (c) 2024 David Carter <dcarter@davidcarter.ca>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD

class TestMaterial(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestMaterial")

    def compareColors(self, c1, c2):
        # Colors are in the form of tuples
        self.assertEqual(len(c1), 4)
        self.assertEqual(len(c2), 4)
        self.assertAlmostEqual(c1[0], c2[0])
        self.assertAlmostEqual(c1[1], c2[1])
        self.assertAlmostEqual(c1[2], c2[2])
        self.assertAlmostEqual(c1[3], c2[3])

    def testDiffuseColor(self):
        print("PartDesignTestMaterial.testDiffuseColor()")
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()

        self.Body.ViewObject.DiffuseColor = [(1.,0.,0.,0.),
                                             (.9,0.,0.,0.),
                                             (.8,0.,0.,0.),
                                             (.7,0.,0.,0.),
                                             (.6,1.,0.,0.),
                                             (.5,1.,0.,0.)]
        self.Doc.recompute()
        self.assertEqual(len(self.Body.ViewObject.DiffuseColor), 6)
        colors = self.Body.ViewObject.DiffuseColor
        self.compareColors(colors[0], (1.,0.,0.,0.))
        self.compareColors(colors[1], (.9,0.,0.,0.))
        self.compareColors(colors[2], (.8,0.,0.,0.))
        self.compareColors(colors[3], (.7,0.,0.,0.))
        self.compareColors(colors[4], (.6,1.,0.,0.))
        self.compareColors(colors[5], (.5,1.,0.,0.))

        self.Body.ViewObject.DiffuseColor = [(1.,0.,0.,0.)]
        self.Doc.recompute()
        self.assertEqual(len(self.Body.ViewObject.DiffuseColor), 1)
        colors = self.Body.ViewObject.DiffuseColor
        self.compareColors(colors[0], (1.,0.,0.,0.))

        self.Box.ViewObject.DiffuseColor = [(1.,0.,0.,0.),
                                             (.9,0.,0.,0.),
                                             (.8,0.,0.,0.),
                                             (.7,0.,0.,0.),
                                             (.6,1.,0.,0.),
                                             (.5,1.,0.,0.)]
        self.Doc.recompute()
        self.assertEqual(len(self.Box.ViewObject.DiffuseColor), 6)
        colors = self.Box.ViewObject.DiffuseColor
        self.compareColors(colors[0], (1.,0.,0.,0.))
        self.compareColors(colors[1], (.9,0.,0.,0.))
        self.compareColors(colors[2], (.8,0.,0.,0.))
        self.compareColors(colors[3], (.7,0.,0.,0.))
        self.compareColors(colors[4], (.6,1.,0.,0.))
        self.compareColors(colors[5], (.5,1.,0.,0.))

        self.Box.ViewObject.DiffuseColor = [(1.,0.,0.,0.)]
        self.Doc.recompute()
        self.assertEqual(len(self.Box.ViewObject.DiffuseColor), 1)
        colors = self.Box.ViewObject.DiffuseColor
        self.compareColors(colors[0], (1.,0.,0.,0.))

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestMaterial")
        # print ("omit closing document for debugging")
