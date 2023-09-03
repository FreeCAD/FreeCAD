# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2007 Juergen Riegel <juergen.riegel@web.de>             *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# Open and edit only in UTF-8 !!!!!!

import FreeCAD, os, unittest, tempfile


# ---------------------------------------------------------------------------
# define the functions to test the FreeCAD Document code
# ---------------------------------------------------------------------------


class UnicodeBasicCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("CreateTest")

    def testUnicodeLabel(self):
        L1 = self.Doc.addObject("App::FeatureTest", "Label_1")
        L1.Label = "हिन्दी"
        self.failUnless(L1.Label == "हिन्दी")

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("CreateTest")


class DocumentSaveRestoreCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("SaveRestoreTests")
        L1 = self.Doc.addObject("App::FeatureTest", "Label_1")
        L1.Label = "हिन्दी"
        self.TempPath = tempfile.gettempdir()
        FreeCAD.Console.PrintLog("  Using temp path: " + self.TempPath + "\n")

    def testSaveAndRestore(self):
        # saving and restoring
        SaveName = self.TempPath + os.sep + "UnicodeTest.FCStd"
        self.Doc.saveAs(SaveName)
        FreeCAD.closeDocument("SaveRestoreTests")
        self.Doc = FreeCAD.open(SaveName)
        self.failUnless(self.Doc.Label_1.Label == "हिन्दी")
        FreeCAD.closeDocument("UnicodeTest")
        FreeCAD.newDocument("SaveRestoreTests")

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("SaveRestoreTests")
