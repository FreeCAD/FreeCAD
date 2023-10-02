# **************************************************************************
#   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

# Broken-out test modules
from SketcherTests.TestSketcherSolver import TestSketcherSolver
from SketcherTests.TestSketchFillet import TestSketchFillet
from SketcherTests.TestSketchExpression import TestSketchExpression

# Path and PartDesign tests use these functions that used to live here
# but moved to SketcherTests/TestSketcherSolver.py
from SketcherTests.TestSketcherSolver import CreateCircleSketch
from SketcherTests.TestSketcherSolver import CreateRectangleSketch
from SketcherTests.TestSketcherSolver import CreateSlotPlateSet
