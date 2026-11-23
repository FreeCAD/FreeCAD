# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Furgo
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

import FreeCAD
import Arch
import Draft
import Part
from bimtests import TestArchBase


class TestArchRebar(TestArchBase.TestArchBase):

    def _create_sketch(self, add_line=True, length=500.0):
        """Helper function to create a basic sketch."""
        sketch = self.document.addObject("Sketcher::SketchObject")
        if add_line:
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(length, 0, 0)), False
            )
        self.document.recompute()
        return sketch

    def _create_host_structure(self, length=1000.0, width=200.0, height=300.0):
        """Helper function to create a basic host structure."""
        host = Arch.makeStructure(length=length, width=width, height=height)
        self.document.recompute()
        return host

    def test_makeRebar_default_name(self):
        """Test makeRebar creates rebar with default 'Rebar' Label.

        See https://github.com/FreeCAD/FreeCAD/issues/21670
        """
        sketch = self._create_sketch()
        rebar = Arch.makeRebar(sketch=sketch, diameter=6)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar failed to create a rebar object.")
        self.assertEqual(rebar.Label, "Rebar", "Rebar default label should be 'Rebar'.")
        self.assertTrue(
            rebar.Name.startswith("Rebar"), "Rebar internal name should start with 'Rebar'."
        )

    def test_makeRebar_custom_name(self):
        """Test makeRebar with a custom name sets Label and Mark correctly."""
        sketch = self._create_sketch()
        custom_name = "MyCustomRebar"
        rebar = Arch.makeRebar(sketch=sketch, diameter=16, amount=5, name=custom_name)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar failed to create a rebar object.")
        self.assertEqual(rebar.Label, custom_name, "Rebar label is incorrect.")
        self.assertEqual(rebar.Mark, custom_name, "Rebar mark should match label by default.")

    def test_makeRebar_with_sketch_and_host(self):
        """Test makeRebar with both sketch and host object links them correctly."""
        sketch_len = 800.0
        host_len = 1000.0
        sketch = self._create_sketch(length=sketch_len)
        host = self._create_host_structure(length=host_len)

        rebar = Arch.makeRebar(baseobj=host, sketch=sketch, diameter=10, amount=3, offset=20)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar failed.")
        self.assertEqual(rebar.Host, host, "Rebar.Host is not set correctly.")
        self.assertEqual(rebar.Base, sketch, "Rebar.Base (sketch) is not set correctly.")
        self.assertAlmostEqual(rebar.Diameter.Value, 10.0, delta=1e-9)
        self.assertEqual(rebar.Amount, 3)
        self.assertAlmostEqual(rebar.OffsetStart.Value, 20.0, delta=1e-9)

        self.assertTrue(hasattr(rebar, "Shape"), "Rebar should have a Shape attribute.")
        self.assertIsNotNone(rebar.Shape, "Rebar.Shape should not be None after recompute.")
        self.assertTrue(rebar.Shape.isValid(), "Rebar.Shape should be valid.")
        self.assertGreater(rebar.Shape.Length, 0, "Rebar shape seems to have no length.")
        self.assertGreater(
            rebar.TotalLength.Value, 0, "Rebar total length should be greater than 0."
        )

    def test_makeRebar_with_sketch_only(self):
        """Test makeRebar with only sketch results in no host and valid geometry."""
        sketch = self._create_sketch()
        rebar = Arch.makeRebar(sketch=sketch, diameter=8)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar failed.")
        self.assertIsNone(rebar.Host, "Rebar.Host should be None.")
        self.assertEqual(rebar.Base, sketch, "Rebar.Base (sketch) is not set correctly.")
        self.assertAlmostEqual(rebar.Diameter.Value, 8.0, delta=1e-9)
        self.assertIsNotNone(rebar.Shape)
        self.assertTrue(rebar.Shape.isValid())
        self.assertGreater(rebar.Shape.Length, 0)

    def test_makeRebar_with_baseobj_only_shape_conversion(self):
        """Test makeRebar with baseobj only copies shape."""
        host_shape_provider = self._create_host_structure()
        rebar = Arch.makeRebar(baseobj=host_shape_provider)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar failed.")
        self.assertIsNone(rebar.Host, "Rebar.Host should be None in this mode.")
        self.assertIsNone(rebar.Base, "Rebar.Base should be None in this mode.")
        self.assertIsNotNone(rebar.Shape, "Rebar.Shape should be set from baseobj.")
        self.assertTrue(rebar.Shape.isValid())
        self.assertGreater(rebar.Shape.Volume, 0, "Converted rebar shape should have volume.")

    def test_makeRebar_properties_set_correctly(self):
        """Test makeRebar sets dimensional and calculated properties correctly."""
        sketch = self._create_sketch(length=900)
        host = self._create_host_structure(length=1200)

        rebar = Arch.makeRebar(baseobj=host, sketch=sketch, diameter=12.5, amount=7, offset=30.0)
        # Default label "Rebar" will be used, and Mark will also be "Rebar"
        self.document.recompute()

        self.assertEqual(rebar.Mark, "Rebar")
        self.assertAlmostEqual(rebar.Diameter.Value, 12.5, delta=1e-9)
        self.assertEqual(rebar.Amount, 7)
        self.assertAlmostEqual(rebar.OffsetStart.Value, 30.0, delta=1e-9)
        self.assertAlmostEqual(rebar.OffsetEnd.Value, 30.0, delta=1e-9)
        self.assertEqual(rebar.Host, host)
        self.assertEqual(rebar.Base, sketch)

        self.assertTrue(hasattr(rebar, "Length"), "Rebar should have Length property.")
        self.assertTrue(hasattr(rebar, "TotalLength"), "Rebar should have TotalLength property.")
        self.assertAlmostEqual(
            rebar.Length.Value,
            sketch.Shape.Length,
            delta=1e-9,
            msg="Single rebar length should match sketch length.",
        )
        expected_total_length = rebar.Length.Value * rebar.Amount
        self.assertAlmostEqual(
            rebar.TotalLength.Value,
            expected_total_length,
            delta=1e-9,
            msg="TotalLength calculation is incorrect.",
        )

    def test_makeRebar_no_sketch_no_baseobj(self):
        """Test makeRebar with no sketch/baseobj creates object with no meaningful shape."""
        rebar = Arch.makeRebar(diameter=10, amount=1)
        self.document.recompute()

        self.assertIsNotNone(rebar, "makeRebar should create an object even with minimal inputs.")
        self.assertIsNone(rebar.Base, "Rebar.Base should be None.")
        self.assertIsNone(rebar.Host, "Rebar.Host should be None.")
        self.assertAlmostEqual(rebar.Diameter.Value, 10.0, delta=1e-9)
        self.assertEqual(rebar.Amount, 1)

        # A Part::FeaturePython object always has a .Shape attribute.
        # For a rebar with no Base, its execute() returns early, so no geometry is made.
        # The Shape should be the default null/empty shape.
        self.assertIsNotNone(rebar.Shape, "Rebar object should always have a Shape attribute.")
        self.assertTrue(
            rebar.Shape.isNull(), "Rebar.Shape should be a null shape if no Base is provided."
        )

    def test_RebarMe(self):
        """
        Tests the creation of an Arch Rebar
        """
        import Sketcher

        self.printTestMessage("Checking Arch Rebar...\n")
        s = Arch.makeStructure(length=2, width=3, height=5)
        sk = self.document.addObject("Sketcher::SketchObject", "Sketch")
        sk.AttachmentSupport = (s, ["Face6"])
        sk.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-0.85, 1.25, 0), FreeCAD.Vector(0.75, 1.25, 0))
        )
        sk.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0.75, 1.25, 0), FreeCAD.Vector(0.75, -1.20, 0))
        )
        sk.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0.75, -1.20, 0), FreeCAD.Vector(-0.85, -1.20, 0))
        )
        sk.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-0.85, -1.20, 0), FreeCAD.Vector(-0.85, 1.25, 0))
        )
        sk.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.document.recompute()
        r = Arch.makeRebar(s, sk, diameter=0.1, amount=2)
        self.document.recompute()
        self.assertTrue(r, "Arch Rebar creation failed")
        self.assertFalse(r.Shape.isNull(), "Rebar shape is null")

    def test_custom_spacing_rotated_host(self):
        """Test CustomSpacing does not double-rotate bars when host is rotated.

        See https://github.com/FreeCAD/FreeCAD/issues/33459
        """
        host = self._create_host_structure(length=3000.0, width=200.0, height=400.0)
        host.Placement = FreeCAD.Placement(
            FreeCAD.Vector(500.0, 1000.0, 0.0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 90),
        )
        self.document.recompute()

        rot = host.Placement.Rotation
        p1 = host.Placement.multVec(FreeCAD.Vector(30.0, -70.0, -150.0))
        p2 = host.Placement.multVec(FreeCAD.Vector(30.0, 70.0, -150.0))
        wire = Draft.make_wire([p1, p2])
        self.document.recompute()

        direction = rot.multVec(FreeCAD.Vector(1, 0, 0))

        uniform = Arch.makeRebar(baseobj=host, sketch=wire, diameter=12, amount=4, offset=30)
        uniform.Direction = direction
        custom = Arch.makeRebar(baseobj=host, sketch=wire, diameter=12, amount=2, offset=30)
        custom.Direction = direction
        custom.CustomSpacing = "4@980"
        self.document.recompute()

        self.assertEqual(custom.Amount, 4)
        self.assertAlmostEqual(custom.Spacing.Value, 0.0, delta=1e-9)
        self.assertTrue(custom.Shape.isValid())
        self.assertEqual(len(uniform.PlacementList), 4)
        self.assertEqual(len(custom.PlacementList), 4)
        self.assertEqual(len(uniform.Shape.Solids), 4)
        self.assertEqual(len(custom.Shape.Solids), 4)
        self.assertEqual(len(uniform.Proxy.wires), 4)
        self.assertEqual(len(custom.Proxy.wires), 4)

        identity = FreeCAD.Rotation()
        for pl_custom, pl_uniform in zip(custom.PlacementList, uniform.PlacementList):
            self.assertTrue(pl_custom.Base.isEqual(pl_uniform.Base, 1e-6))
            self.assertTrue(pl_custom.Rotation.isSame(identity, 1e-6))
            self.assertTrue(pl_custom.Rotation.isSame(pl_uniform.Rotation, 1e-6))

        for w_custom, w_uniform in zip(custom.Proxy.wires, uniform.Proxy.wires):
            self.assertTrue(w_custom.CenterOfMass.isEqual(w_uniform.CenterOfMass, 1e-3))
            self.assertAlmostEqual(
                w_custom.BoundBox.XLength, w_uniform.BoundBox.XLength, delta=1e-2
            )
            self.assertAlmostEqual(
                w_custom.BoundBox.YLength, w_uniform.BoundBox.YLength, delta=1e-2
            )
            self.assertAlmostEqual(
                w_custom.BoundBox.ZLength, w_uniform.BoundBox.ZLength, delta=1e-2
            )

        for s_custom, s_uniform in zip(custom.Shape.Solids, uniform.Shape.Solids):
            self.assertTrue(s_custom.CenterOfMass.isEqual(s_uniform.CenterOfMass, 1e-3))
            self.assertAlmostEqual(
                s_custom.BoundBox.XLength, s_uniform.BoundBox.XLength, delta=1e-2
            )
            self.assertAlmostEqual(
                s_custom.BoundBox.YLength, s_uniform.BoundBox.YLength, delta=1e-2
            )
            self.assertAlmostEqual(
                s_custom.BoundBox.ZLength, s_uniform.BoundBox.ZLength, delta=1e-2
            )

    def test_custom_spacing_rotated_host_with_sketch(self):
        """Test CustomSpacing matches uniform placement for a sketch attached to a rotated host."""
        host = self._create_host_structure(length=3000.0, width=200.0, height=400.0)
        host.Placement = FreeCAD.Placement(
            FreeCAD.Vector(250.0, -150.0, 50.0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 45),
        )
        self.document.recompute()

        sketch = self.document.addObject("Sketcher::SketchObject", "RebarSketch")
        sketch.AttachmentSupport = [(host, "Face5")]
        sketch.MapMode = "FlatFace"
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-70.0, 0.0, 0.0), FreeCAD.Vector(70.0, 0.0, 0.0)),
            False,
        )
        self.document.recompute()

        direction = host.Placement.Rotation.multVec(FreeCAD.Vector(1, 0, 0))

        uniform = Arch.makeRebar(baseobj=host, sketch=sketch, diameter=12, amount=4, offset=30)
        uniform.Direction = direction
        custom = Arch.makeRebar(baseobj=host, sketch=sketch, diameter=12, amount=2, offset=30)
        custom.Direction = direction
        custom.CustomSpacing = "4@980"
        self.document.recompute()

        self.assertEqual(custom.Amount, 4)
        self.assertAlmostEqual(custom.Spacing.Value, 0.0, delta=1e-9)
        self.assertTrue(custom.Shape.isValid())
        self.assertEqual(len(uniform.PlacementList), 4)
        self.assertEqual(len(custom.PlacementList), 4)
        self.assertEqual(len(uniform.Shape.Solids), 4)
        self.assertEqual(len(custom.Shape.Solids), 4)

        identity = FreeCAD.Rotation()
        for pl_custom, pl_uniform in zip(custom.PlacementList, uniform.PlacementList):
            self.assertTrue(pl_custom.Base.isEqual(pl_uniform.Base, 1e-6))
            self.assertTrue(pl_custom.Rotation.isSame(identity, 1e-6))
            self.assertTrue(pl_custom.Rotation.isSame(pl_uniform.Rotation, 1e-6))

        for s_custom, s_uniform in zip(custom.Shape.Solids, uniform.Shape.Solids):
            self.assertTrue(s_custom.CenterOfMass.isEqual(s_uniform.CenterOfMass, 1e-3))
            self.assertAlmostEqual(
                s_custom.BoundBox.XLength, s_uniform.BoundBox.XLength, delta=1e-2
            )
            self.assertAlmostEqual(
                s_custom.BoundBox.YLength, s_uniform.BoundBox.YLength, delta=1e-2
            )
            self.assertAlmostEqual(
                s_custom.BoundBox.ZLength, s_uniform.BoundBox.ZLength, delta=1e-2
            )

    def test_custom_spacing_without_host(self):
        """Test CustomSpacing works on a standalone rebar without a host."""
        sketch = self._create_sketch(length=500.0)
        rebar = Arch.makeRebar(sketch=sketch, diameter=12, amount=2, offset=50)
        rebar.Direction = FreeCAD.Vector(0, 0, 1)
        rebar.Distance = 850.0
        rebar.CustomSpacing = "2@200+2@300"
        self.document.recompute()

        self.assertEqual(rebar.Amount, 4)
        self.assertAlmostEqual(rebar.Spacing.Value, 0.0, delta=1e-9)
        self.assertEqual(len(rebar.PlacementList), 4)
        self.assertEqual(len(rebar.Shape.Solids), 4)
        self.assertTrue(rebar.Shape.isValid())

        identity = FreeCAD.Rotation()
        expected_z = [50.0, 250.0, 500.0, 800.0]
        for pl, z in zip(rebar.PlacementList, expected_z):
            self.assertTrue(pl.Base.isEqual(FreeCAD.Vector(0.0, 0.0, z), 1e-6))
            self.assertTrue(pl.Rotation.isSame(identity, 1e-6))
