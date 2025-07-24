# *************************************************************************
#   Copyright (c) 2021 Emmanuel O'Brien                                   *
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
import FreeCAD
import Part
import Sketcher
from FreeCAD import Vector

App = FreeCAD


class TestSketchNote(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("SketchNoteTest")

    def test_note_creation(self):
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketcher")
        pos = FreeCAD.Vector(10, 20, 30)
        text = "Note test"

        note_geom = Part.Note(pos, text)
        idx = sketch.addGeometry(note_geom, False)

        note = sketch.Geometry[idx]

        self.assertEqual(str(note_geom).strip(), str(note).strip())

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
