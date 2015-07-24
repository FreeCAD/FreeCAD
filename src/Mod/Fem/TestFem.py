# Unit test for the FEM module

#***************************************************************************
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author: Przemo Firszt <przemo@firszt.eu>                              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

import FreeCAD
import unittest


class FemTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")

    def test_new_analysis(self):
        FreeCAD.Console.PrintMessage('\nChecking FEM new analysis...\n')
        import MechanicalAnalysis
        analysis = MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')
        self.failUnless(analysis, "FemTest of new analysis failed")

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass
