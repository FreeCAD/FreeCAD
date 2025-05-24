# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Unit tests for the ArchComponent module

import Arch
import Draft
import FreeCAD as App
from bimtests import TestArchBase
from draftutils.messages import _msg

from math import pi, cos, sin, radians

class TestArchComponent(TestArchBase.TestArchBase):

    def testBsplineSlabAreas(self):
        """Test the HorizontalArea and VerticalArea properties of a Bspline-based slab.

        See https://github.com/FreeCAD/FreeCAD/issues/20989.
        """

        operation = "Checking Bspline slab area calculation..."
        self.printTestMessage(operation)

        doc = App.ActiveDocument

        # Parameters
        radius = 10000  # 10 meters in mm
        extrusionLength = 100  # 10 cm in mm
        numPoints = 50  # Number of points for B-spline
        startAngle = 0   # Start at 0 degrees (right)
        endAngle = 180   # End at 180 degrees (left)

        # Create points for semicircle
        points = []
        angleStep = (endAngle - startAngle) / (numPoints - 1)
        for i in range(numPoints):
            angleDeg = startAngle + i * angleStep
            angleRad = radians(angleDeg)
            x = radius * cos(angleRad)
            y = radius * sin(angleRad)
            points.append(App.Vector(x, y, 0))

        # Create Draft objects
        bspline = Draft.makeBSpline(points, closed=False)
        closingLine = Draft.makeLine(points[-1], points[0])
        doc.recompute()

        # Create sketch
        # We do this because Draft.make_wires does not support B-splines
        # and we need a closed wire for the slab
        sketch = Draft.makeSketch([bspline, closingLine],
                               autoconstraints=True,
                               delete=True)
        if sketch is None:
            self.fail("Sketch creation failed")
        sketch.recompute()

        # Create slab
        slab = Arch.makeStructure(sketch, length=extrusionLength, name="Slab")
        slab.recompute()

        # Calculate theoretical areas
        radiusMeters = radius / 1000
        heightMeters = extrusionLength / 1000
        theoreticalHorizontalArea = (pi * radiusMeters**2) / 2
        theoreticalVerticalArea = (pi * radiusMeters + 2 * radiusMeters) * heightMeters

        # Get actual areas
        actualHorizontalArea = slab.HorizontalArea.getValueAs("m^2").Value
        actualVerticalArea = slab.VerticalArea.getValueAs("m^2").Value

        # Optimally wrapped assertions
        self.assertAlmostEqual(
            actualHorizontalArea, theoreticalHorizontalArea, places=3,
            msg=(
                "Horizontal area > 0.1% tolerance | "
                f"Exp: {theoreticalHorizontalArea:.3f} m² | "
                f"Got: {actualHorizontalArea:.3f} m²"
            )
        )

        self.assertAlmostEqual(
            actualVerticalArea, theoreticalVerticalArea, places=3,
            msg=(
                "Vertical area > 0.1% tolerance | "
                f"Exp: {theoreticalVerticalArea:.3f} m² | "
                f"Got: {actualVerticalArea:.3f} m²"
            )
        )

    def testHouseSpaceAreas(self):
        """Test the HorizontalArea and VerticalArea properties of a house-like space.

        See https://github.com/FreeCAD/FreeCAD/issues/14687.
        """

        operation = "Checking house space area calculation..."
        self.printTestMessage(operation)

        doc = App.ActiveDocument

        # Dimensional parameters (all in mm)
        baseLength = 5000  # 5m along X-axis
        baseWidth = 5000   # 5m along Y-axis (extrusion depth)
        rectangleHeight = 2500  # 2.5m lower rectangular portion
        triangleHeight = 2500   # 2.5m upper triangular portion
        totalHeight = rectangleHeight + triangleHeight  # 5m total height

        # Create envelope profile points (XZ plane)
        points = [
            App.Vector(0, 0, 0),
            App.Vector(baseLength, 0, 0),
            App.Vector(baseLength, 0, rectangleHeight),
            App.Vector(baseLength/2, 0, totalHeight),
            App.Vector(0, 0, rectangleHeight)
        ]

        # Create wire with automatic face creation
        wire = Draft.makeWire(points, closed=True, face=True)
        if not wire:
            self.fail(f"Wire creation failed with points: {points}\n")
        doc.recompute()

        # Extrude the wire
        extrudedObj = Draft.extrude(wire, App.Vector(0, baseWidth, 0), solid=True)
        if not extrudedObj:
            self.fail("Extrusion failed - no object created\n")
        extrudedObj.Label = "Extruded house"
        doc.recompute()

        # Create Arch Space from the extrusion
        space = Arch.makeSpace(extrudedObj)
        space.Label = "House space"
        doc.recompute()

        # Calculate theoretical areas
        # Horizontal area (only bottom face on XY plane)
        theoreticalHorizontalArea = (baseLength * baseWidth) / 1e6  # 25 m²

        # Vertical areas
        # Side faces (YZ plane) - two rectangles
        sideFaceArea = (rectangleHeight * baseWidth) / 1e6  # 12.5 m² each
        totalSides = sideFaceArea * 2  # 25 m²

        # Front/back faces (XZ plane)
        rectangularPart = (baseLength * rectangleHeight) / 1e6  # 12.5 m²
        triangularPart = (baseLength * triangleHeight / 2) / 1e6  # 6.25 m²
        totalFrontBack = (rectangularPart + triangularPart) * 2  # 37.5 m²

        theoreticalVerticalArea = totalSides + totalFrontBack  # 62.5 m²

        # Get actual areas from space
        actualHorizontalArea = space.HorizontalArea.getValueAs("m^2").Value
        actualVerticalArea = space.VerticalArea.getValueAs("m^2").Value

        self.assertAlmostEqual(
            actualHorizontalArea, theoreticalHorizontalArea, places=3,
            msg=f"Horizontal area > 0.1% | Exp: {theoreticalHorizontalArea:.3f} | "
                f"Got: {actualHorizontalArea:.3f}"
        )

        self.assertAlmostEqual(
            actualVerticalArea, theoreticalVerticalArea, places=3,
            msg=f"Vertical area > 0.1% | Exp: {theoreticalVerticalArea:.3f} | "
                f"Got: {actualVerticalArea:.3f}"
        )
