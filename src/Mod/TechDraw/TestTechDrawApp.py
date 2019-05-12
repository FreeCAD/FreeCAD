#   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************

import FreeCAD, os, sys, unittest, Part
import Measure
import TechDraw
import time
App = FreeCAD

from TDTest.DHatchTest         import DHatchTest
from TDTest.DProjGroupTest     import DProjGroupTest
from TDTest.DVAnnoSymImageTest import DVAnnoSymImageTest
from TDTest.DVDimensionTest    import DVDimensionTest
from TDTest.DVPartTest         import DVPartTest
from TDTest.DVSectionTest      import DVSectionTest
from TDTest.DVBalloonTest      import DVBalloonTest

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD TechDraw module
#---------------------------------------------------------------------------


class TechDrawTestCases(unittest.TestCase):
    def testViewPartCase(self):
        print("starting TD DrawViewPart test")
        rc = DVPartTest()
        if rc:
            print("TD DrawViewPart test passed")
        else:
            print("TD DrawViewPart test failed")

    def testHatchCase(self):
        print("starting TD DrawHatch test")
        rc = DHatchTest()
        if rc:
            print("TD DrawHatch test passed")
        else:
            print("TD DrawHatch test failed")

    def testAnnoSymImageCase(self):
        print("starting TD DrawAnno/Sym/Image test")
        rc = DVAnnoSymImageTest()
        if rc:
            print("TD DrawAnno/Sym/Image test passed")
        else:
            print("TD DrawAnno/Sym/Image test failed")

    def testProjGroupCase(self):
        print("starting TD DrawProjGroup test")
        rc = DProjGroupTest()
        if rc:
            print("TD DrawProjGroup test passed")
        else:
            print("TD DrawProjGroup test failed")

    def testDimensionCase(self):
        print("starting TD DrawViewDimension test")
        rc = DVDimensionTest()
        if rc:
            print("TD DrawViewDimension test passed")
        else:
            print("TD DrawViewDimension test failed")

    def testSectionCase(self):
        print("starting TD DrawViewSection test")
        rc = DVSectionTest()
        if rc:
            print("TD DrawViewSection test passed")
        else:
            print("TD DrawViewSection test failed")

    def testBalloonCase(self):
        print("starting TD DrawViewBalloon test")
        rc = DVBalloonTest()
        if rc:
            print("TD DrawViewBalloon test passed")
        else:
            print("TD DrawViewBalloon test failed")
