# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import PathScripts
import PathScripts.PathDressupDogbone as Dogbone
import math
import unittest

from FreeCAD import Vector
from PathTests.PathTestUtils import PathTestBase

class TestProfile:

    def __init__(self, side, direction, path):
        self.Side = side
        self.Direction = direction
        self.Path = Path.Path(path)
        self.ToolController = None # default tool 5mm

class TestFeature:
    def __init__(self):
        self.Path = Path.Path()

    def addProperty(self, typ, nam, category, tip):
        setattr(self, nam, None)

    def setEditorMode(self, prop, mode):
        pass

class TestDogbone(PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def test00(self):
        path = []
        base = TestProfile('Inside', 'CW', 'G0 X10 Y10 Z10\nG1 Z0\nG1 Y100\nG1 X12\nG1 Y10\nG1 X10\nG1 Z10')
        obj = TestFeature()
        db = Dogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        for bone in db.bones:
            print("%d: (%.2f, %.2f)" % (bone[0], bone[1][0], bone[1][1]))

    def test01(self):
        path = []
        base = TestProfile('Inside', 'CW', 'G0 X10 Y10 Z10\nG1 Z0\nG1 Y100\nG1 X12\nG1 Y10\nG1 X10\nG0 Z10')
        obj = TestFeature()
        db = Dogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        for bone in db.bones:
            print("%d: (%.2f, %.2f)" % (bone[0], bone[1][0], bone[1][1]))

