#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENSE text file.                                 *
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
import OpenSCAD
import importCSG

from os.path import join

__title__ = "ImportCSG OpenSCAD App unit tests"
__author__ = "Chris Hennes"
__url__ = "https://www.freecadweb.org"


class TestImportCSG(unittest.TestCase):

    MODULE = 'test_importCSG' # file name without extension


    def setUp(self):
        self.test_dir = join(FreeCAD.getHomePath(), "Mod", "OpenSCAD", "OpenSCADTest", "data")
        pass

    def test_open_scad(self):
        testfile = join(self.test_dir, "CSG.scad")
        doc = importCSG.open(testfile)

        # Doc should now contain three solids: a union, an intersection, and a difference
        union = doc.getObject("union")
        intersection = doc.getObject("intersection")
        difference = doc.getObject("difference")

        self.assertTrue (union is not None)
        self.assertTrue (intersection is not None)
        self.assertTrue (difference is not None)

        FreeCAD.closeDocument("CSG")

    def test_open_csg(self):
        testfile = join(self.test_dir, "CSG.csg")
        doc = importCSG.open(testfile)

        # Doc should now contain three solids: a union, an intersection, and a difference
        union = doc.getObject("union")
        intersection = doc.getObject("intersection")
        difference = doc.getObject("difference")

        self.assertTrue (union is not None)
        self.assertTrue (intersection is not None)
        self.assertTrue (difference is not None)

        FreeCAD.closeDocument("CSG")