# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2011 Juergen Riegel
# SPDX-FileCopyrightText: 2021 Emmanuel O'Brien
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

# Broken-out test modules
from SketcherTests.TestSketcherSolver import TestSketcherSolver
from SketcherTests.TestSketchFillet import TestSketchFillet
from SketcherTests.TestSketchExpression import TestSketchExpression
from SketcherTests.TestSketchValidateCoincidents import TestSketchValidateCoincidents
from SketcherTests.TestSketchCarbonCopyReverseMapping import TestSketchCarbonCopyReverseMapping
from SketcherTests.TestSketchInternalFaces import TestSketchInternalFaces
from SketcherTests.TestSketcherEllipse import TestSketcherEllipse

# Path and PartDesign tests use these functions that used to live here
# but moved to SketcherTests/TestSketcherSolver.py
from SketcherTests.TestSketcherSolver import CreateCircleSketch
from SketcherTests.TestSketcherSolver import CreateRectangleSketch
from SketcherTests.TestSketcherSolver import CreateSlotPlateSet
