# SPDX-License-Identifier: LGPL-2.1-or-later
# ****************************************************************************
# *                                                                          *
# *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
# *                                                                          *
# *   This file is part of FreeCAD.                                          *
# *                                                                          *
# *   FreeCAD is free software: you can redistribute it and/or modify it     *
# *   under the terms of the GNU Lesser General Public License as            *
# *   published by the Free Software Foundation, either version 2.1 of the   *
# *   License, or (at your option) any later version.                        *
# *                                                                          *
# *   FreeCAD is distributed in the hope that it will be useful, but         *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
# *   Lesser General Public License for more details.                        *
# *                                                                          *
# *   You should have received a copy of the GNU Lesser General Public       *
# *   License along with FreeCAD. If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                       *
# *                                                                          *
# ***************************************************************************/

import unittest
import functools

import FreeCAD
import Part
import Sketcher


def finegrained(variants):
    """Method decorator: mark a test to be duplicated for each variant.

    Accepts either a single bool or an iterable of bools, for example:
      @finegrained((True, False))
      @finegrained(True)
    """

    # Normalize to a tuple of booleans
    if isinstance(variants, bool):
        variants = (variants,)
    else:
        variants = tuple(variants)

    def decorate(func):
        setattr(func, "_fg_variants", variants)
        return func

    return decorate


def parameterize():
    """Class decorator: create wrapper test methods for each @finegrained variant.

    For every test method named test* that has a _fg_variants attribute, this
    will create additional methods:
      <orig>_fineGrained_on   (for True)
      <orig>_fineGrained_off  (for False)
    """

    def decorate(cls):
        def make_wrapper(func, fg):
            """Capture func and fg at creation time."""

            @functools.wraps(func)
            def wrapper(self, *args, **kwargs):
                return func(self, *args, **kwargs)

            wrapper._fineGrained = fg
            wrapper._generated = True
            return wrapper

        for name, func in list(cls.__dict__.items()):
            if not name.startswith("test"):
                continue

            if getattr(func, "_generated", False):
                continue

            variants = getattr(func, "_fg_variants", None)
            if variants is None:
                continue

            for fg in variants:
                suffix = "on" if fg else "off"
                new_name = f"{name}_fineGrained_{suffix}"
                if hasattr(cls, new_name):
                    continue

                wrapper = make_wrapper(func, fg)
                wrapper.__name__ = new_name
                setattr(cls, new_name, wrapper)

            try:
                # Remove the original test function.
                delattr(cls, name)
            except AttributeError:
                # The attribute may have been removed.  Not a problem.
                pass
        return cls

    return decorate


class DoubleLengthCube:
    # A cube that has an input length and an output double length.
    def __init__(self, obj):
        self.Object = obj
        obj.Proxy = self
        obj.addProperty("App::PropertyLength", "Length", "Cube")
        obj.addProperty("App::PropertyLength", "DoubleLength", "Output")
        obj.setPropertyStatus("DoubleLength", "Output")
        obj.Length = "10 mm"

    def execute(self, obj):
        length = float(obj.Length)
        obj.Shape = Part.makeBox(length, length, length)
        obj.DoubleLength = f"{length * 2} mm"

    def onChanged(self, obj, prop):
        if prop == "Length":
            self.execute(obj)


@parameterize()
class FineGrainedRecomputeCases(unittest.TestCase):
    # Most tests work both with fine-grained recompute on and off.  Some tests
    # only work with either fine-grained on or off and those tests have an
    # explicit comment that explains why fine-grained recompute needs to be on
    # or off.

    def setUp(self):
        self.Doc = FreeCAD.newDocument(f"RecomputeTests_{self._testMethodName}")

        method = getattr(self, self._testMethodName)
        self.fineGrained = getattr(method, "_fineGrained", None)
        FreeCAD.setFineGrainedRecompute(self.fineGrained)

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def assertDep(self, depEdges, depEdgeTuples):
        """Helper to assert that a list of dependency edges matches expected.
        depEdges: list of DepEdge
        depEdgeTuples: list of tuples (fromObj, fromProp, toObj, toProp)
        """
        actual = {str(de) for de in depEdges}
        expected = {
            f"{fromObj.FullName}.{fromProp} --> {toObj.FullName}.{toProp}"
            for fromObj, fromProp, toObj, toProp in depEdgeTuples
        }
        for actualDepEdge in actual:
            self.assertIn(
                actualDepEdge, expected, f"{actualDepEdge} not in the expected dependency edges"
            )
        for expectedDepEdge in expected:
            self.assertIn(
                expectedDepEdge, actual, f"{expectedDepEdge} not in the actual dependency edges"
            )

    @finegrained((True, False))
    def testSimple(self):
        # arrange
        varSet = self.Doc.addObject("App::VarSet", "VarSet")
        varSet.addProperty("App::PropertyLength", "Length", "Params")
        varSet.Length = "10.0 mm"

        cube = self.Doc.addObject("Part::Box", "Cube")
        cube.setExpression("Length", "VarSet.Length")
        self.Doc.recompute()

        # activate
        varSet.Length = "20.0 mm"
        self.Doc.recompute([varSet])

        # assert
        self.assertIn("Touched", cube.State)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(varSet.Name)

    @finegrained((True,))
    def testIndependent(self):
        # This is the crucial test that will fail if fine-grained recompute is
        # disabled because without fine-grained recomputation, both cube1 and
        # cube2 will be touched.

        # arrange
        varSet = self.Doc.addObject("App::VarSet", "VarSet")

        varSet.addProperty("App::PropertyLength", "Length1", "Params")
        varSet.Length1 = "10.0 mm"

        varSet.addProperty("App::PropertyLength", "Length2", "Params")
        varSet.Length2 = "10.0 mm"

        cube1 = self.Doc.addObject("Part::Box", "Cube1")
        cube1.setExpression("Length", "VarSet.Length1")

        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "VarSet.Length2")
        self.Doc.recompute()

        # activate
        varSet.Length1 = "20.0 mm"
        self.Doc.recompute([varSet])

        # assert
        self.assertIn("Touched", cube1.State)
        self.assertNotIn("Touched", cube2.State)

        # teardown
        self.Doc.removeObject(cube1.Name)
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(varSet.Name)

    @finegrained((True, False))
    def testChaining(self):
        # arrange
        varSet1 = self.Doc.addObject("App::VarSet", "VarSet1")
        varSet1.addProperty("App::PropertyLength", "Length", "Params")
        varSet1.Length = "10.0 mm"

        varSet2 = self.Doc.addObject("App::VarSet", "VarSet2")
        varSet2.addProperty("App::PropertyLength", "Length", "Params")
        varSet2.setExpression("Length", "VarSet1.Length")

        cube = self.Doc.addObject("Part::Box", "Cube")
        cube.setExpression("Length", "VarSet2.Length")
        self.Doc.recompute()

        # activate
        varSet1.Length = "20.0 mm"
        self.Doc.recompute([varSet1, varSet2])

        # assert
        self.assertIn("Touched", cube.State)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(varSet2.Name)
        self.Doc.removeObject(varSet1.Name)

    @finegrained((True, False))
    def testOutputThroughExpression(self):
        # arrange
        varSet = self.Doc.addObject("App::VarSet", "VarSet")

        varSet.addProperty("App::PropertyLength", "Length", "Params")
        varSet.Length = "10.0 mm"

        cube1 = self.Doc.addObject("Part::Box", "Cube1")
        cube1.addProperty("App::PropertyLength", "OutputLength", "Output")
        cube1.setPropertyStatus("OutputLength", "Output")
        cube1.setExpression("Length", "VarSet.Length")
        cube1.setExpression("OutputLength", "Length + Width")

        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "Cube1.OutputLength")
        self.Doc.recompute()

        # activate
        varSet.Length = "20.0 mm"
        self.Doc.recompute([varSet, cube1])

        # assert
        self.assertIn("Touched", cube2.State)

        # teardown
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(cube1.Name)
        self.Doc.removeObject(varSet.Name)

    @finegrained((True, False))
    def testOutputInternal(self):
        # arrange
        cube1 = self.Doc.addObject("Part::FeaturePython", "Cube1")
        DoubleLengthCube(cube1)

        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "Cube1.DoubleLength")
        self.Doc.recompute()

        # activate
        cube1.Length = "20.0 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(cube1.DoubleLength.Value, 40.0)
        self.assertEqual(cube1.DoubleLength.Unit, FreeCAD.Units.Length)
        self.assertEqual(cube2.Length.Value, 40.0)
        self.assertEqual(cube2.Length.Unit, FreeCAD.Units.Length)

        # teardown
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(cube1.Name)

    @finegrained((True, False))
    def testRelabel(self):
        # arrange
        varSet = self.Doc.addObject("App::VarSet", "VarSet")
        varSet.addProperty("App::PropertyLength", "Length", "Params")
        varSet.Length = "10.0 mm"

        cube = self.Doc.addObject("Part::Box", "Cube")
        cube.setExpression("Length", "VarSet.Length")
        self.Doc.recompute()

        # activate
        varSet.Label = "Params"

        # assert
        self.assertNotIn("Touched", varSet.State)
        self.assertNotIn("Touched", cube.State)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(varSet.Name)

    @finegrained((True, False))
    def testChangeVisibility(self):
        # arrange
        cube1 = self.Doc.addObject("Part::Box", "Cube1")
        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "Cube1.Length")
        self.Doc.recompute()

        # activate
        cube1.Visibility = False

        # assert
        self.assertNotIn("Touched", cube1.State)
        self.assertNotIn("Touched", cube2.State)

        # teardown
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(cube1.Name)

    @finegrained((True, False))
    def testExpression(self):
        # arrange
        cube1 = self.Doc.addObject("Part::Box", "Cube1")
        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "Cube1.Length")

        # activate
        self.Doc.recompute()

        # assert
        depEdges = [(cube2, "ExpressionEngine", cube1, "Length")]
        self.assertDep(cube1.OutListProp, [])
        self.assertDep(cube2.OutListProp, depEdges)
        self.assertDep(cube1.InListProp, depEdges)
        self.assertDep(cube2.InListProp, [])

        # teardown
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(cube1.Name)

    @finegrained((True, False))
    def testRemoveExpression(self):
        # arrange
        cube1 = self.Doc.addObject("Part::Box", "Cube1")
        cube2 = self.Doc.addObject("Part::Box", "Cube2")
        cube2.setExpression("Length", "Cube1.Length")

        # activate
        self.Doc.recompute()

        # assert
        depEdges = [(cube2, "ExpressionEngine", cube1, "Length")]
        self.assertDep(cube1.OutListProp, [])
        self.assertDep(cube2.OutListProp, depEdges)
        self.assertDep(cube1.InListProp, depEdges)
        self.assertDep(cube2.InListProp, [])

        # activate
        cube2.setExpression("Length", None)
        self.Doc.recompute()

        # assert
        self.assertDep(cube1.OutListProp, [])
        self.assertDep(cube2.OutListProp, [])
        self.assertDep(cube1.InListProp, [])
        self.assertDep(cube2.InListProp, [])

        # teardown
        self.Doc.removeObject(cube2.Name)
        self.Doc.removeObject(cube1.Name)

    @finegrained((True,))
    def testInputProperty(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.

        # arrange
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)
        part.addProperty("App::PropertyLength", "Length", "Params")
        part.Length = "10 mm"
        part.setPropertyStatus("Length", ["Input"])
        cube.setExpression("Length", "Part.Length")
        self.Doc.recompute()

        # activate
        part.Length = "20 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(cube.Length.Value, 20.0)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)

    @finegrained((True,))
    def testInputPropertyDeps(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.

        # arrange
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)
        part.addProperty("App::PropertyLength", "Length", "Params")
        part.Length = "10 mm"
        part.setPropertyStatus("Length", ["Input"])
        self.Doc.recompute()

        # activate
        cube.setExpression("Length", "Part.Length")

        # assert
        origin = self.Doc.Origin
        inListPart = [(cube, "ExpressionEngine", part, "Length")]
        inListCube = [(part, "Group", cube, "HEAD")]
        outListPart = [(part, "Origin", origin, "HEAD")] + inListCube
        outListCube = inListPart

        self.assertDep(part.OutListProp, outListPart)
        self.assertDep(part.InListProp, inListPart)
        self.assertDep(cube.OutListProp, outListCube)
        self.assertDep(cube.InListProp, inListCube)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)

    @finegrained((False,))
    def testInputPropertyCoarse(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode, so should raise an error here.

        # arrange
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)
        part.addProperty("App::PropertyLength", "Length", "Params")
        part.Length = "10 mm"
        part.setPropertyStatus("Length", ["Input"])

        # activate / assert
        with self.assertRaises(RuntimeError):
            cube.setExpression("Length", "Part.Length")

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)

    @finegrained((True, False))
    def testCyclicDependency(self):
        # arrange
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)
        part.addProperty("App::PropertyLength", "Length", "Params")
        part.Length = "10 mm"

        # activate/assert
        with self.assertRaises(RuntimeError):
            cube.setExpression("Length", "Part.Length")

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)

    @finegrained((True,))
    def testInputPropertyChain(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.

        # arrange
        varSet = self.Doc.addObject("App::VarSet", "VarSet")
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)

        varSet.addProperty("App::PropertyLength", "Length", "Params")
        varSet.Length = "10 mm"
        part.addProperty("App::PropertyLength", "Length", "Params")
        part.Length = "10 mm"
        part.setPropertyStatus("Length", ["Input"])
        part.setExpression("Length", "VarSet.Length")
        cube.setExpression("Length", "Part.Length")
        self.Doc.recompute()

        # activate
        varSet.Length = "20 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(cube.Length.Value, 20.0)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)
        self.Doc.removeObject(varSet.Name)

    @finegrained((True,))
    def testInputLink(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.

        # arrange
        varSet1 = self.Doc.addObject("App::VarSet", "VarSet")
        varSet2 = self.Doc.addObject("App::VarSet", "VarSet")
        part = self.Doc.addObject("App::Part", "Part")
        cube = self.Doc.addObject("Part::Box", "Cube")
        part.addObject(cube)

        varSet1.addProperty("App::PropertyLength", "Length", "Params")
        varSet1.Length = "10 mm"

        varSet2.addProperty("App::PropertyLength", "Length", "Params")
        varSet2.Length = "20 mm"

        part.addProperty("App::PropertyLink", "Params", "Params")
        part.setPropertyStatus("Params", ["Input"])
        part.Params = varSet1

        cube.setExpression("Length", "Part.Params.Length")
        self.Doc.recompute()

        # activate
        varSet1.Length = "15 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(cube.Length.Value, 15.0)

        # activate
        part.Params = varSet2
        self.Doc.recompute()

        # assert
        self.assertEqual(cube.Length.Value, 20.0)

        # teardown
        self.Doc.removeObject(cube.Name)
        self.Doc.removeObject(part.Name)
        self.Doc.removeObject(varSet2.Name)
        self.Doc.removeObject(varSet1.Name)

    def createPartDesignObject(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (self.Doc.Origin, ["XY_Plane"])
        sketch.MapMode = "FlatFace"

        def lsegment(x1, y1, x2, y2):
            return Part.LineSegment(FreeCAD.Vector(x1, y1, 0), FreeCAD.Vector(x2, y2, 0))

        geoList = []
        geoList.append(lsegment(-16, 16, -16, -16))
        geoList.append(lsegment(-16, -16, 16, -16))
        geoList.append(lsegment(16, -16, 16, 16))
        geoList.append(lsegment(16, 16, -16, 16))
        sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 0))
        constraintList.append(Sketcher.Constraint("Vertical", 2))
        constraintList.append(Sketcher.Constraint("Horizontal", 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 3))
        constraintList.append(Sketcher.Constraint("Symmetric", 2, 2, 0, 2, -1, 1))
        constraintList.append(Sketcher.Constraint("Equal", 3, 2))
        constraintList.append(Sketcher.Constraint("Distance", 3, 1, 3, 2, 32.0))
        sketch.addConstraint(constraintList)
        del constraintList

        sketch.setGeometryIds([(0, 1), (1, 2), (2, 3), (3, 4)])
        sketch.setDatum(10, FreeCAD.Units.Quantity("32.0 mm"))

        self.Doc.recompute()

        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = "10 mm"
        pad.ReferenceAxis = (sketch, "N_Axis")

    @finegrained((True,))
    def testPadWithInputProperty(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.
        self.createPartDesignObject()

        body = self.Doc.Body
        body.addProperty("App::PropertyLength", "Height", "Params")
        body.Height = "20 mm"
        body.setPropertyStatus("Height", ["Input"])

        pad = self.Doc.Pad
        pad.setExpression("Length", "Body.Height")

        # activate
        body.Height = "30 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(pad.Length.Value, 30.0)

    @finegrained((True,))
    def testSketchWithInputProperty(self):
        # References to input properties of dependent objects only works in
        # fine-grained mode.
        self.createPartDesignObject()

        body = self.Doc.Body
        body.addProperty("App::PropertyLength", "Length", "Params")
        body.Length = "20 mm"
        body.setPropertyStatus("Length", ["Input"])

        sketch = self.Doc.Sketch
        sketch.setExpression("Constraints[10]", "Body.Length")

        # activate
        body.Length = "30 mm"
        self.Doc.recompute()

        # assert
        self.assertEqual(body.Shape.BoundBox.XLength, 30.0)
