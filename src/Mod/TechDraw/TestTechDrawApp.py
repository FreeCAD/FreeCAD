# **************************************************************************
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
# **************************************************************************

import unittest

from TDTest.DrawHatchTest import DrawHatchTest  # noqa: F401
from TDTest.DrawViewAnnotationTest import DrawViewAnnotationTest  # noqa: F401
from TDTest.DrawViewBalloonTest import DrawViewBalloonTest  # noqa: F401
from TDTest.DrawViewDimensionTest import DrawViewDimensionTest  # noqa: F401
from TDTest.DrawViewImageTest import DrawViewImageTest  # noqa: F401
from TDTest.DrawViewSectionTest import DrawViewSectionTest  # noqa: F401
from TDTest.DrawViewSymbolTest import DrawViewSymbolTest  # noqa: F401
from TDTest.DrawViewPartTest import DrawViewPartTest  # noqa: F401
from TDTest.DProjGroupTest import DProjGroupTest

# ---------------------------------------------------------------------------
# define the test cases to test the FreeCAD TechDraw module
# ---------------------------------------------------------------------------


class TechDrawTestCases(unittest.TestCase):
    def testProjGroupCase(self):
        print("starting TD DrawProjGroup test")
        rc = DProjGroupTest()
        if rc:
            print("TD DrawProjGroup test passed")
        else:
            print("TD DrawProjGroup test failed")
