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
import Part
import FreeCAD as App
from bimtests import TestArchBase
from draftutils.messages import _msg

from math import pi, cos, sin, radians


class TestArchComponent(TestArchBase.TestArchBase):

    def testAdd(self):
        App.Console.PrintLog("Checking Arch Add...\n")
        l = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2, 0, 0))
        w = Arch.makeWall(l, width=0.2, height=2)
        sb = Part.makeBox(1, 1, 1)
        b = App.ActiveDocument.addObject("Part::Feature", "Box")
        b.Shape = sb
        App.ActiveDocument.recompute()
        Arch.addComponents(b, w)
        App.ActiveDocument.recompute()
        r = w.Shape.Volume > 1.5
        self.assertTrue(r, "Arch Add failed")

    def testRemove(self):
        App.Console.PrintLog("Checking Arch Remove...\n")
        l = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2, 0, 0))
        w = Arch.makeWall(l, width=0.2, height=2, align="Right")
        sb = Part.makeBox(1, 1, 1)
        b = App.ActiveDocument.addObject("Part::Feature", "Box")
        b.Shape = sb
        App.ActiveDocument.recompute()
        Arch.removeComponents(b, w)
        App.ActiveDocument.recompute()
        r = w.Shape.Volume < 0.75
        self.assertTrue(r, "Arch Remove failed")

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
        startAngle = 0  # Start at 0 degrees (right)
        endAngle = 180  # End at 180 degrees (left)

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
        sketch = Draft.makeSketch([bspline, closingLine], autoconstraints=True, delete=True)
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
            actualHorizontalArea,
            theoreticalHorizontalArea,
            places=3,
            msg=(
                "Horizontal area > 0.1% tolerance | "
                f"Exp: {theoreticalHorizontalArea:.3f} m² | "
                f"Got: {actualHorizontalArea:.3f} m²"
            ),
        )

        self.assertAlmostEqual(
            actualVerticalArea,
            theoreticalVerticalArea,
            places=3,
            msg=(
                "Vertical area > 0.1% tolerance | "
                f"Exp: {theoreticalVerticalArea:.3f} m² | "
                f"Got: {actualVerticalArea:.3f} m²"
            ),
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
        baseWidth = 5000  # 5m along Y-axis (extrusion depth)
        rectangleHeight = 2500  # 2.5m lower rectangular portion
        triangleHeight = 2500  # 2.5m upper triangular portion
        totalHeight = rectangleHeight + triangleHeight  # 5m total height

        # Create envelope profile points (XZ plane)
        points = [
            App.Vector(0, 0, 0),
            App.Vector(baseLength, 0, 0),
            App.Vector(baseLength, 0, rectangleHeight),
            App.Vector(baseLength / 2, 0, totalHeight),
            App.Vector(0, 0, rectangleHeight),
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
            actualHorizontalArea,
            theoreticalHorizontalArea,
            places=3,
            msg=f"Horizontal area > 0.1% | Exp: {theoreticalHorizontalArea:.3f} | "
            f"Got: {actualHorizontalArea:.3f}",
        )

        self.assertAlmostEqual(
            actualVerticalArea,
            theoreticalVerticalArea,
            places=3,
            msg=f"Vertical area > 0.1% | Exp: {theoreticalVerticalArea:.3f} | "
            f"Got: {actualVerticalArea:.3f}",
        )

    def test_remove_single_window_from_wall_host_is_none(self):
        """
        Tests that a window is removed from its wall's host list when
        Arch.removeComponents is called with host=None (single window selection scenario).

        See https://github.com/FreeCAD/FreeCAD/issues/21551
        """

        # Create a basic wall
        wall_base = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(3000, 0, 0))
        wall = Arch.makeWall(wall_base, width=200, height=2500)

        # Create a window to be added to the wall
        window_width = 1000.0
        window_height = 1200.0

        # Create a Draft Rectangle for the window's Base. Arch.makeWindow can work with
        # either a Wire or a Face.
        window_base = Draft.makeRectangle(length=window_width, height=window_height)

        window = Arch.makeWindow(baseobj=window_base)
        window.Width = window_width  # Manually set as makeWindow(base) doesn't
        window.Height = window_height  # Manually set

        self.document.recompute()

        # Add the window to the wall.
        Arch.addComponents(window, wall)
        self.document.recompute()

        # Pre-condition check: ensure the wall is indeed a host for the window.
        self.assertIn(wall, window.Hosts, "Wall should be in window.Hosts before removal.")
        self.assertEqual(len(window.Hosts), 1, "Window should have 1 host before removal.")

        # Simulate the Arch_Remove command with the scenario where only the window
        # is selected and "Remove" is used.
        Arch.removeComponents([window], host=None)
        self.document.recompute()  # Important for Arch objects to update their state.

        # Assert: the wall should no longer be in the window's Hosts list.
        self.assertNotIn(wall, window.Hosts, "Wall should not be in window.Hosts after removal.")
        self.assertEqual(len(window.Hosts), 0, "Window.Hosts list should be empty after removal.")

    def test_if_face_vertical(self):
        """
        Test the ArchComponent.AreaCalculator.isFaceVertical method directly.

        Verifies classification of standard walls, periodic surfaces (cylinders),
        extruded surfaces (B-Splines), and sloped faces (tapered wedge) using
        white-box testing on the internal calculator.
        """
        import math
        import ArchComponent

        tolerance = Part.Precision.confusion()

        def get_normal_z(face):
            return abs(face.normalAt(0, 0).z)

        def is_vertical(normal_z):
            return math.isclose(normal_z, 0.0, abs_tol=tolerance)

        def is_horizontal(normal_z):
            return math.isclose(normal_z, 1.0, abs_tol=tolerance)

        def is_sloped(normal_z):
            return not (is_vertical(normal_z) or is_horizontal(normal_z))

        with self.subTest(case="Standard Wall"):
            line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
            wall = Arch.makeWall(line, width=2, height=10)
            self.document.recompute()

            calc = ArchComponent.AreaCalculator(wall)
            vertical_count = 0

            for face in wall.Shape.Faces:
                is_vert = calc.isFaceVertical(face)
                normal_z = get_normal_z(face)

                if is_vertical(normal_z):
                    self.assertTrue(is_vert, "Side face should be vertical")
                    vertical_count += 1
                elif is_horizontal(normal_z):
                    self.assertFalse(is_vert, "Top/Bottom face should not be vertical")

            self.assertEqual(
                vertical_count, 4, f"Expected 4 vertical sides on wall, found {vertical_count}"
            )

        with self.subTest(case="Closed Extrusion"):
            points = [App.Vector(0, 0, 0), App.Vector(10, 0, 0), App.Vector(5, 5, 0)]
            bspline = Draft.makeBSpline(points, closed=True)
            structure = Arch.makeStructure(bspline, height=10)
            self.document.recompute()

            calc_bspline = ArchComponent.AreaCalculator(structure)
            extrusion_vert_count = 0

            for face in structure.Shape.Faces:
                is_vert = calc_bspline.isFaceVertical(face)

                if "SurfaceOfExtrusion" in face.Surface.TypeId:
                    self.assertTrue(is_vert, "Extruded B-Spline surface should be vertical")
                    extrusion_vert_count += 1

            self.assertGreater(
                extrusion_vert_count,
                0,
                f"Expected at least one vertical extruded face, found {extrusion_vert_count}",
            )

        with self.subTest(case="Cylinder"):
            circle = Draft.makeCircle(radius=5)
            struct = Arch.makeStructure(circle, height=20)
            self.document.recompute()

            calc_struct = ArchComponent.AreaCalculator(struct)
            cyl_vertical_count = 0

            for face in struct.Shape.Faces:
                is_vert = calc_struct.isFaceVertical(face)

                if "Cylinder" in face.Surface.TypeId:
                    self.assertTrue(is_vert, "Cylindrical face should be vertical")
                    cyl_vertical_count += 1
                else:
                    self.assertFalse(is_vert, "Caps of cylinder should not be vertical")

            self.assertEqual(
                cyl_vertical_count,
                1,
                f"Expected exactly 1 vertical face on cylinder, found {cyl_vertical_count}",
            )

        with self.subTest(case="Generic Vertical Surface"):
            # Create two B-spline curves, vertically aligned
            points1 = [App.Vector(0, 0, 0), App.Vector(5, 5, 0), App.Vector(10, 0, 0)]
            bspline1 = Draft.makeBSpline(points1, closed=False)

            points2 = [App.Vector(0, 0, 20), App.Vector(5, 5, 20), App.Vector(10, 0, 20)]
            bspline2 = Draft.makeBSpline(points2, closed=False)

            # Create a ruled surface (Loft)
            loft = self.document.addObject("Part::Loft", "GenericVerticalLoft")
            loft.Sections = [bspline1, bspline2]
            loft.Solid = False
            loft.Ruled = True
            self.document.recompute()

            comp = Arch.makeComponent(loft)
            calc_loft = ArchComponent.AreaCalculator(comp)

            generic_vertical_count = 0
            for face in comp.Shape.Faces:
                if calc_loft.isFaceVertical(face):
                    generic_vertical_count += 1

            self.assertEqual(
                generic_vertical_count,
                1,
                f"Expected generic vertical surface to be detected as vertical, found {generic_vertical_count}",
            )

        with self.subTest(case="Tapered Wedge"):
            wedge = self.document.addObject("Part::Wedge", "Wedge")
            wedge.Ymin = 0
            wedge.Zmin = 0
            wedge.Xmin = 0
            wedge.Ymax = 10
            wedge.Zmax = 10
            wedge.Xmax = 10
            # Taper top to create slopes
            wedge.X2min = 2
            wedge.X2max = 8
            wedge.Z2min = 2
            wedge.Z2max = 8
            self.document.recompute()

            comp = Arch.makeComponent(wedge)
            calc_wedge = ArchComponent.AreaCalculator(comp)

            tapered_vertical_count = 0
            for face in comp.Shape.Faces:
                normal_z = get_normal_z(face)

                if is_vertical(normal_z):
                    self.assertTrue(
                        calc_wedge.isFaceVertical(face), "X-Tapered face should be vertical"
                    )
                    tapered_vertical_count += 1
                elif is_sloped(normal_z):
                    self.assertFalse(
                        calc_wedge.isFaceVertical(face), "Sloped face must not be vertical"
                    )

            self.assertGreater(
                tapered_vertical_count,
                0,
                f"Expected at least one vertical face on tapered wedge, found {tapered_vertical_count}",
            )

    def test_vertical_area_update_to_zero(self):
        """
        Verify that VerticalArea property updates correctly even when the result is zero.
        """

        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        wall = Arch.makeWall(line, width=2, height=10)
        self.document.recompute()

        initial_area = wall.VerticalArea.Value
        self.assertGreater(
            initial_area, 0, f"Setup error: Wall should have vertical area, found {initial_area}"
        )

        # Get the footprint to simulate a flat wall (valid shape, but 0 height)
        footprint_faces = wall.Proxy.getFootprint(wall)

        # Exactly one face is expected for this particular footprint (straight wall)
        self.assertEqual(len(footprint_faces), 1, "Setup error: Expected exactly 1 footprint face")

        # Manually assign the single footprint face as the wall shape. We do this as an alternative
        # to setting height=0, because ArchWall.applyShape protects against null shapes (which
        # height=0 produces), preventing area updates
        wall.Shape = footprint_faces[0]

        wall.Proxy.computeAreas(wall)

        final_area = wall.VerticalArea.Value
        self.assertEqual(final_area, 0.0, f"VerticalArea must update to zero, found {final_area}")

    def test_complex_composite_area(self):
        """
        Integration test: verify that AreaCalculator correctly sums areas from
        mixed geometry types (planar, cylindrical, and generic) within a single object,
        while correctly ignoring horizontal faces.
        """

        # Create planar geometry (Box)
        # 10x10x10 box.
        # 4 Vertical faces = 10 * 10 * 4 = 400.
        # 2 Horizontal faces (Top/Bottom) should be ignored.
        box = Part.makeBox(10, 10, 10)
        box.translate(App.Vector(0, 0, 0))
        expected_box_v_area = 400.0

        # Create cylindrical geometry (Cylinder)
        # Radius 5, Height 10.
        # 1 Vertical Face = 2 * pi * r * h = 100 * pi.
        # 2 Horizontal faces (caps) should be ignored.
        cyl = Part.makeCylinder(5, 10)
        cyl.translate(App.Vector(20, 0, 0))
        expected_cyl_v_area = 100 * pi

        # Create generic geometry (Ruled Surface / Loft)
        # Reuse the B-Spline logic that triggers the fallback projection path
        points1 = [App.Vector(40, 0, 0), App.Vector(45, 5, 0), App.Vector(50, 0, 0)]
        bspline1 = Draft.makeBSpline(points1, closed=False)
        points2 = [App.Vector(40, 0, 10), App.Vector(45, 5, 10), App.Vector(50, 0, 10)]
        bspline2 = Draft.makeBSpline(points2, closed=False)

        loft = self.document.addObject("Part::Loft", "IntegrationLoft")
        loft.Sections = [bspline1, bspline2]
        loft.Solid = False
        loft.Ruled = True
        self.document.recompute()

        # The entire loft is a vertical surface, so we take its total area.
        expected_loft_v_area = loft.Shape.Area

        # Combine into an Arch Component using a compound to simulate a complex single object
        compound_shape = Part.makeCompound([box, cyl, loft.Shape])

        complex_obj = Arch.makeComponent(compound_shape, name="ComplexStructure")

        # Execute calculation
        complex_obj.Proxy.computeAreas(complex_obj)

        # Verify
        total_expected = expected_box_v_area + expected_cyl_v_area + expected_loft_v_area

        self.assertAlmostEqual(
            complex_obj.VerticalArea.Value,
            total_expected,
            places=3,
            msg=f"Failed to aggregate vertical areas of mixed types. Expected {total_expected}, got {complex_obj.VerticalArea.Value}",
        )

    def test_horizontal_area_tilted_cylinders(self):
        """
        Verify that the HorizontalArea of tilted cylinders is correct.
        The cylinders are rotated around the X-axis and the Y-axis.
        """

        # The created cylinders are very tall to also check for potential
        # 'crazy edge' issues related to the use of TechDraw code. Edges
        # longer than ca. 10m are considered 'crazy'.
        angle = 30  # in degrees
        radius = 100  # in mm
        height = 50000  # in mm

        # To calculate the horizontal area, the shape to be projected can be
        # reduced to a rectangular face through the center of the cylinder
        # and two semi-circular faces for the top and bottom.
        area_rect = 2 * radius * height * cos(radians(90 - angle))
        area_circ = pi * radius**2 * cos(radians(angle))
        area_expected = (area_rect + area_circ) / 1e6  # in m^2

        for rot_vec in (App.Vector(1, 0, 0), App.Vector(0, 1, 0)):
            cyl = Part.show(Part.makeCylinder(radius, height))
            cyl.Placement.Rotation = App.Rotation(rot_vec, 30)
            obj = Arch.makeStructure(cyl)
            obj.recompute()
            area_actual = obj.HorizontalArea.getValueAs("m^2").Value

            self.assertAlmostEqual(
                area_expected,
                area_actual,
                places=3,
                msg=(
                    "Horizontal area > 0.1% tolerance | "
                    f"Exp: {area_expected:.3f} m² | "
                    f"Got: {area_actual:.3f} m²"
                ),
            )

    def test_rotated_component_area(self):
        """Verify AreaCalculator respects Placement for generic ArchComponents."""
        self.printTestMessage("ArchComponent rotated area calculation...")

        # Create a horizontal slab (1 m x 1 m x 0.01 m)
        # Area of one large face = 1.0 m2
        box = self.document.addObject("Part::Box", "HorizontalSlab")
        box.Length = 1000.0
        box.Width = 1000.0
        box.Height = 10.0
        self.document.recompute()

        # Wrap in a generic Component and rotate 90 degrees around the X axis to turn it into a
        # vertical panel.
        comp = Arch.makeComponent(box)
        comp.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        self.document.recompute()

        # Check the VerticalArea property.
        # FreeCAD sums all vertical faces (front + back + vertical side edges).
        # Expected: (1.0 * 1.0)*2 + (1.0 * 0.01)*2 = 2.02 m2
        expected_v_area = 2.02
        actual_v_area = comp.VerticalArea.getValueAs("m^2").Value

        self.assertAlmostEqual(
            actual_v_area,
            expected_v_area,
            places=2,
            msg=f"Vertical area calculation failed for rotated component. Got {actual_v_area}",
        )

    def test_moved_component_subtraction(self):
        """Verify subtractions align correctly for moved/rotated components."""
        self.printTestMessage("moved component boolean subtraction...")

        # Create a base component and move it 5 meters away
        base_box = self.document.addObject("Part::Box", "BaseBox")
        base_box.Length = base_box.Width = base_box.Height = 100.0
        comp = Arch.makeComponent(base_box)
        comp.Placement.Base = App.Vector(5000.0, 0, 0)
        self.document.recompute()

        initial_volume = comp.Shape.Volume  # Should be 1,000,000 mm^3

        # Create a "Cutter" box at the same global 5-meter position
        cutter = self.document.addObject("Part::Box", "CutterBox")
        cutter.Length = cutter.Width = cutter.Height = 100.0
        cutter.Placement.Base = App.Vector(5000.0, 0, 0)
        self.document.recompute()

        # Add the cutter to subtractions.
        # processSubShapes must inverse-transform the cutter into the component's local space.
        comp.Subtractions = [cutter]
        self.document.recompute()

        # Assert: If the fix works, the volumes overlap perfectly and result is 0.
        # If the fix fails, the cutter is ignored because it looks for the box at the origin.
        final_volume = comp.Shape.Volume
        self.assertLess(
            final_volume,
            initial_volume,
            "Subtraction failed. The global cutter did not intersect the moved local shape.",
        )
        self.assertAlmostEqual(final_volume, 0.0, places=5)

    def test_apply_shape_spread(self):
        """Ensure generic components handle spreading (automatic arraying) via the Axis property."""
        self.printTestMessage("applyShape spread logic (generic component)...")

        # Create base geometry at identity (0,0,0)
        box = self.document.addObject("Part::Box", "SpreadBase")
        box.Length = box.Width = box.Height = 100.0
        self.document.recompute()

        # Create a generic Arch Component
        comp = Arch.makeComponent(box)

        # Create an Axis system with 2 points (at 0 and 2000mm)
        axis = Arch.makeAxis(num=2, size=2000)
        self.document.recompute()

        # Link the axis to the component
        comp.Axis = axis
        self.document.recompute()

        # Verify that the resulting shape contains 2 instances (solids)
        # This confirms that the execute() loop correctly processes the Axis property.
        self.assertEqual(
            len(comp.Shape.Solids),
            2,
            "Generic Arch Component failed to spread geometry to Axis points.",
        )

    def test_component_double_transformation(self):
        """Test that Arch Components do not suffer from double-transformation."""
        self.printTestMessage("ArchComponent placement and coordinate integrity...")

        # Scenario 1: translation and vertex check
        with self.subTest(case="Translation Only"):
            base_box = self.document.addObject("Part::Box", "BaseBoxTrans")
            base_box.Length = base_box.Width = base_box.Height = 1000.0

            # Move the box 10 meters away. Raw vertices are at 0, Shape.Placement is at 10 m.
            base_box.Placement.Base = App.Vector(10000, 0, 0)
            self.document.recompute()

            comp = Arch.makeComponent(base_box, name="TestTrans")
            self.document.recompute()

            # The component object should match the base object's placement
            self.assertEqual(comp.Placement.Base.x, 10000.0)

            # Verification of localization:
            # Visual Center = Object.Placement (10000) + Shape.Center (500) = 10500.
            # If the bug were present (double transform), it would be 20500.
            actual_center_x = comp.Shape.BoundBox.Center.x
            self.assertAlmostEqual(
                actual_center_x,
                10500.0,
                places=3,
                msg="Double transformation detected! Object is offset twice.",
            )

        # Scenario 2: CSG alignment (Additions)
        with self.subTest(case="CSG Alignment"):
            # Base box (1m cube) moved to 5m
            base_box_csg = self.document.addObject("Part::Box", "BaseBoxCSG")
            base_box_csg.Length = base_box_csg.Width = base_box_csg.Height = 1000.0
            base_box_csg.Placement.Base = App.Vector(5000, 0, 0)

            comp_csg = Arch.makeComponent(base_box_csg, name="TestCSG")
            self.document.recompute()

            # Addition box (identical 1m cube) at exactly the same global location (5m)
            # They should overlap perfectly.
            add_box = self.document.addObject("Part::Box", "AdditionBox")
            add_box.Length = add_box.Width = add_box.Height = 1000.0
            add_box.Placement.Base = App.Vector(5000, 0, 0)
            if App.GuiUp:
                add_box.ViewObject.hide()

            comp_csg.Additions = [add_box]
            self.document.recompute()

            # If sanitized, they overlap perfectly: Total Volume = 1,000,000,000 mm3
            # If not sanitized, they would be 5m apart: Volume = 2,000,000,000 mm3
            self.assertAlmostEqual(
                comp_csg.Shape.Volume,
                1000000000.0,
                delta=100.0,
                msg="CSG pieces did not align. Base shape likely retained the offset.",
            )

    def test_component_base_removal_cleanup(self):
        """Test that removing the Base property clears the component shape."""
        self.printTestMessage("ArchComponent Base Removal Cleanup...")

        box = self.document.addObject("Part::Box", "StaleTestBox")
        comp = Arch.makeComponent(box)
        self.document.recompute()

        self.assertFalse(comp.Shape.isNull(), "Component should have a shape initially.")

        # Trigger the 'else' block
        comp.Base = None
        self.document.recompute()

        self.assertTrue(
            comp.Shape.isNull(), "Component retained a stale shape after its Base was removed."
        )
