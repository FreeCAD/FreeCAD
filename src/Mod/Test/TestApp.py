#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
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
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/

import FreeCAD
import sys
import unittest


#---------------------------------------------------------------------------
# define the functions to test the FreeCAD base code
#---------------------------------------------------------------------------

def tryLoadingTest(testName):
    "Loads and returns testName, or a failing TestCase if unsuccessful."

    try:
        return unittest.defaultTestLoader.loadTestsFromName(testName)

    except ImportError:
        class LoadFailed(unittest.TestCase):
            def __init__(self, testName):
                # setattr() first, because TestCase ctor checks for methodName.
                setattr(self, "failed_to_load_" + testName, self._runTest)
                super(LoadFailed, self).__init__("failed_to_load_" + testName)
                self.testName = testName

            def __name__(self):
                return "Loading " + self.testName

            def _runTest(self):
                self.fail("Couldn't load " + self.testName)

        return LoadFailed(testName)

def All():
    # Base system tests
    tests = [ "UnicodeTests",
              "Document",
              "UnitTests",
              "BaseTests" ]

    # Base system gui test
    if (FreeCAD.GuiUp == 1):
        tests += [ "Workbench",
                   "Menu" ]

    # add the module tests
    tests += [ "TestFem",
               "MeshTestsApp",
               "TestSketcherApp",
               "TestPartApp",
               "TestPartDesignApp",
               "TestSpreadsheet",
               "TestTechDrawApp" ]

    # gui tests of modules
    if (FreeCAD.GuiUp == 1):
        tests += [ "TestSketcherGui",
                   "TestPartGui",
                   "TestPartDesignGui",
                   "TestDraft",
                   "TestArch" ]

    suite = unittest.TestSuite()

    for test in tests:
        suite.addTest(tryLoadingTest(test))

    return suite


def TestText(s):
    s = unittest.defaultTestLoader.loadTestsFromName(s)
    r = unittest.TextTestRunner(stream=sys.stdout, verbosity=2)
    return r.run(s)


def Test(s):
    TestText(s)


def testAll():
    r = unittest.TextTestRunner(stream=sys.stdout, verbosity=2)
    return r.run(All())


def testUnit():
    TestText(unittest.TestLoader().loadTestsFromName("UnitTests"))


def testDocument():
    suite = unittest.TestSuite()
    suite.addTest(unittest.defaultTestLoader.loadTestsFromName("Document"))
    TestText(suite)
