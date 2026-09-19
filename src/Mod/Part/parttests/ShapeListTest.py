# -*- coding: utf-8 -*-
# Tests for Part.ShapeList, what Shape.Faces and its siblings answer with.
#
# The point of the type is that it hands out an element only when one is
# asked for, so most of what is checked here is that it still behaves like
# the list it replaced.

import operator
import unittest

import FreeCAD as App
import Part


class ShapeListTest(unittest.TestCase):
    def setUp(self):
        self.box = Part.makeBox(1, 1, 1)
        self.cyl = Part.makeCylinder(1, 2)

    def testIsAView(self):
        faces = self.box.Faces
        self.assertIsInstance(faces, Part.ShapeList)
        self.assertTrue(faces.IsView)
        self.assertEqual(faces.ElementType, "Face")
        self.assertEqual(faces.Count, 6)
        self.assertEqual(len(faces), 6)
        self.assertTrue(faces.Shape.isSame(self.box))

    def testCounts(self):
        self.assertEqual(len(self.box.Faces), 6)
        self.assertEqual(len(self.box.Edges), 12)
        self.assertEqual(len(self.box.Vertexes), 8)
        self.assertEqual(len(self.box.Wires), 6)
        self.assertEqual(len(self.box.Shells), 1)
        self.assertEqual(len(self.box.Solids), 1)
        self.assertEqual(len(self.box.CompSolids), 0)
        self.assertEqual(len(self.box.Compounds), 0)
        self.assertEqual(len(self.box.SubShapes), 1)

    def testElementsAreTheSameShapes(self):
        # every element must be the shape the indexed accessor answers with
        for i in range(len(self.box.Faces)):
            named = getattr(self.box, "Face%d" % (i + 1))
            self.assertTrue(self.box.Faces[i].isSame(named))
        self.assertEqual(self.box.Faces[0].ShapeType, "Face")
        self.assertEqual(self.box.Edges[0].ShapeType, "Edge")
        self.assertEqual(self.box.Vertexes[0].ShapeType, "Vertex")

    def testNegativeIndexAndOutOfRange(self):
        faces = self.box.Faces
        self.assertTrue(faces[-1].isSame(faces[5]))
        self.assertTrue(faces[-6].isSame(faces[0]))
        with self.assertRaises(IndexError):
            operator.getitem(faces, 6)
        with self.assertRaises(IndexError):
            operator.getitem(faces, -7)

    def testIteration(self):
        faces = self.box.Faces
        iterator = iter(faces)
        self.assertTrue(faces.IsView)
        self.assertIs(next(iterator), faces[0])
        walked = [f for f in faces]
        self.assertTrue(faces.IsView)
        self.assertEqual(len(walked), 6)
        for i, face in enumerate(walked):
            self.assertTrue(face.isSame(faces[i]))
        # a second walk gives the same shapes: nothing is consumed
        self.assertEqual(len([f for f in faces]), 6)

    def testSliceGivesAPlainList(self):
        faces = self.box.Faces
        part = faces[1:3]
        self.assertIsInstance(part, list)
        self.assertEqual(len(part), 2)
        self.assertTrue(part[0].isSame(faces[1]))
        self.assertEqual(len(faces[:]), 6)
        self.assertEqual(len(faces[::2]), 3)
        self.assertEqual(len(faces[::-1]), 6)
        self.assertTrue(faces[::-1][0].isSame(faces[5]))

    def testContainsAndIndex(self):
        faces = self.box.Faces
        self.assertIn(faces[3], faces)
        self.assertEqual(faces.index(faces[3]), 3)
        self.assertEqual(faces.index(faces[3], 2, 5), 3)
        self.assertEqual(faces.index(faces[3], -4, -1), 3)
        self.assertEqual(faces.index(faces[3], -(10**100), 10**100), 3)
        self.assertEqual(faces.count(faces[3]), 1)
        self.assertEqual(faces.count("Face1"), 0)
        self.assertNotIn(self.cyl.Faces[0], faces)
        with self.assertRaises(ValueError):
            faces.index(self.cyl.Faces[0])
        with self.assertRaises(ValueError):
            faces.index(faces[3], 4)
        with self.assertRaises(ValueError):
            faces.index(faces[3], 0, 3)
        with self.assertRaises(ValueError):
            faces.index("Face1")
        with self.assertRaises(ValueError):
            faces.copy().remove("Face1")
        with self.assertRaises(TypeError):
            faces.index(faces[0], "zero")
        # something that is not a shape at all is simply not in the list
        self.assertNotIn("Face1", faces)

    def testElementWrapperIdentityAndMutation(self):
        faces = self.box.Faces
        face = faces[0]
        self.assertIs(face, faces[0])

        before = face.CenterOfMass
        face.translate(App.Vector(10, 0, 0))
        self.assertAlmostEqual(faces[0].CenterOfMass.x, before.x + 10, 6)
        self.assertIs(list(faces)[0], face)

        # A later structural write must copy the changed wrapper, not recreate
        # the original face from the parent view.
        faces.append(self.cyl.Faces[0])
        self.assertAlmostEqual(faces[0].CenterOfMass.x, before.x + 10, 6)

        copied = faces.copy()
        self.assertIs(copied[0], faces[0])

    def testNullShapeEqualityMembershipAndSearch(self):
        null = Part.Shape()
        shapes = Part.ShapeList([null, null])
        self.assertEqual(shapes, [null, null])
        self.assertEqual(shapes, shapes)
        self.assertIn(null, shapes)
        self.assertEqual(shapes.count(null), 2)
        self.assertEqual(shapes.index(null), 0)

    def testInplaceRepeat(self):
        faces = Part.ShapeList([self.box.Faces[0], self.box.Faces[1]])
        identity = id(faces)
        faces *= 2
        self.assertEqual(id(faces), identity)
        self.assertEqual(len(faces), 4)
        self.assertIs(faces[0], faces[2])

    def testExtendedSlices(self):
        faces = Part.ShapeList(list(self.box.Faces))
        replacement = [self.cyl.Faces[0], self.cyl.Faces[1], self.cyl.Faces[2]]
        faces[::2] = replacement
        self.assertIs(faces[0], replacement[0])
        self.assertIs(faces[2], replacement[1])
        self.assertIs(faces[4], replacement[2])
        self.assertTrue(faces[0].isSame(replacement[0]))
        self.assertTrue(faces[2].isSame(replacement[1]))
        self.assertTrue(faces[4].isSame(replacement[2]))

        with self.assertRaises(ValueError):
            faces[::2] = [replacement[0]]

        del faces[1::2]
        self.assertEqual(len(faces), 3)
        self.assertTrue(faces[0].isSame(replacement[0]))
        self.assertTrue(faces[1].isSame(replacement[1]))
        self.assertTrue(faces[2].isSame(replacement[2]))

        reverse = Part.ShapeList(list(self.box.Faces))
        reverse[::-2] = replacement
        self.assertIs(reverse[5], replacement[0])
        self.assertIs(reverse[3], replacement[1])
        self.assertIs(reverse[1], replacement[2])
        self.assertTrue(reverse[5].isSame(replacement[0]))
        self.assertTrue(reverse[3].isSame(replacement[1]))
        self.assertTrue(reverse[1].isSame(replacement[2]))
        del reverse[::-2]
        self.assertEqual(len(reverse), 3)

    def testTruth(self):
        self.assertTrue(self.box.Faces)
        self.assertFalse(self.box.CompSolids)
        self.assertEqual(len(self.box.CompSolids), 0)

    def testConcatenation(self):
        both = self.box.Faces + self.cyl.Faces
        self.assertIsInstance(both, list)
        self.assertEqual(len(both), 9)
        # with a plain list, on either side
        with_list = self.box.Faces + [self.cyl.Faces[0]]
        self.assertIsInstance(with_list, list)
        self.assertEqual(len(with_list), 7)
        list_first = [self.cyl.Faces[0]] + self.box.Faces
        self.assertIsInstance(list_first, list)
        self.assertEqual(len(list_first), 7)
        self.assertTrue(list_first[0].isSame(self.cyl.Faces[0]))
        with self.assertRaises(TypeError):
            self.box.Faces + tuple(self.cyl.Faces)
        with self.assertRaises(TypeError):
            tuple(self.cyl.Faces) + self.box.Faces
        # and the chain the Draft code writes
        chain = self.box.Edges + [self.cyl.Edges[0]] + self.box.Edges
        self.assertEqual(len(chain), 25)

    def testRepeat(self):
        twice = self.box.Faces * 2
        self.assertIsInstance(twice, list)
        self.assertEqual(len(twice), 12)
        reverse = 2 * self.box.Faces
        self.assertIsInstance(reverse, list)
        self.assertEqual(len(reverse), 12)
        self.assertEqual(len(self.box.Faces * 0), 0)

    def testEquality(self):
        faces = self.box.Faces
        self.assertEqual(faces, faces)
        self.assertNotEqual(faces, self.cyl.Faces)
        # against the plain list this used to be
        self.assertEqual(faces, list(faces))
        self.assertEqual(list(faces), faces)
        self.assertNotEqual(faces, list(faces)[:-1])
        self.assertNotEqual(faces, tuple(faces))
        self.assertNotEqual(tuple(faces), faces)
        equivalent = faces.copy()
        for comparison, expected in (
            (operator.lt, False),
            (operator.le, True),
            (operator.gt, False),
            (operator.ge, True),
        ):
            with self.subTest(comparison=comparison.__name__):
                self.assertEqual(comparison(faces, equivalent), expected)
        with self.assertRaises(TypeError):
            self.box.Faces < tuple(self.box.Faces)

    def testConsumersTakeIt(self):
        # the calls that used to be handed a list
        wire = Part.Wire(self.box.Faces[0].Edges)
        self.assertEqual(wire.ShapeType, "Wire")
        shell = Part.makeShell(self.box.Faces)
        self.assertEqual(shell.ShapeType, "Shell")
        comp = Part.makeCompound(self.box.Faces)
        self.assertEqual(len(comp.Faces), 6)

        changed_faces = self.box.Faces
        changed = changed_faces[0]
        before = changed.CenterOfMass.x
        changed.translate(App.Vector(10, 0, 0))
        changed_compound = Part.makeCompound(changed_faces)
        self.assertAlmostEqual(changed_compound.Faces[0].CenterOfMass.x, before + 10, 6)

        structurally_changed = self.box.Faces
        structurally_changed.pop()
        self.assertEqual(len(Part.makeCompound(structurally_changed).Faces), 5)
        structurally_changed.append(self.cyl.Faces[0])
        self.assertEqual(len(Part.makeCompound(structurally_changed).Faces), 6)

        edges = Part.ShapeList(list(self.box.Faces[0].Edges))
        before = Part.Wire(edges).CenterOfMass.x
        for edge in edges:
            edge.translate(App.Vector(10, 0, 0))
        moved_wire = Part.Wire(edges)
        self.assertAlmostEqual(moved_wire.CenterOfMass.x, before + 10, 6)

    def testWritingDetaches(self):
        faces = self.box.Faces
        self.assertTrue(faces.IsView)
        faces.append(self.cyl.Faces[0])
        self.assertFalse(faces.IsView)
        self.assertIsNone(faces.Shape)
        self.assertEqual(len(faces), 7)
        # the shape is untouched by any of it
        self.assertEqual(len(self.box.Faces), 6)

        mixed = self.box.Faces
        mixed.append(self.box.Edges[0])
        self.assertEqual(mixed.ElementType, "Shape")

    def testElementTypeTracksEveryMixedWrite(self):
        assigned = self.box.Faces
        assigned[0] = self.box.Edges[0]
        self.assertEqual(assigned.ElementType, "Shape")

        extended = self.box.Faces
        extended.extend([self.box.Edges[0]])
        self.assertEqual(extended.ElementType, "Shape")

        sliced = self.box.Faces
        sliced[:1] = [self.box.Edges[0]]
        self.assertEqual(sliced.ElementType, "Shape")

        mutated = self.box.Faces
        changed = mutated[0]
        changed.loads(self.box.dumps())
        self.assertEqual(changed.ShapeType, "Solid")
        self.assertEqual(mutated.ElementType, "Shape")

    def testMutators(self):
        faces = self.box.Faces
        last = faces[5]
        popped = faces.pop()
        self.assertIs(popped, last)
        self.assertTrue(popped.isSame(last))
        self.assertEqual(len(faces), 5)

        faces.insert(0, popped)
        self.assertEqual(len(faces), 6)
        self.assertIs(faces[0], popped)
        self.assertTrue(faces[0].isSame(popped))

        faces.remove(popped)
        self.assertEqual(len(faces), 5)

        faces.extend([popped, popped])
        self.assertEqual(len(faces), 7)
        self.assertIs(faces[-1], popped)
        self.assertIs(faces[-2], popped)

        first = faces[0]
        faces.reverse()
        self.assertIs(faces[-1], first)
        self.assertTrue(faces[-1].isSame(first))

        faces += [popped]
        self.assertEqual(len(faces), 8)

        faces[0] = popped
        self.assertIs(faces[0], popped)
        self.assertTrue(faces[0].isSame(popped))

        del faces[0]
        self.assertEqual(len(faces), 7)

        faces.clear()
        self.assertEqual(len(faces), 0)
        self.assertFalse(faces)

    def testSort(self):
        faces = self.box.Faces
        faces.sort(key=lambda f: -f.Area)
        areas = [f.Area for f in faces]
        self.assertEqual(areas, sorted(areas, reverse=True))
        edges = self.cyl.Edges
        edges.sort(key=lambda e: e.Length)
        self.assertLessEqual(edges[0].Length, edges[-1].Length)

        first = self.box.Faces[0]
        second = self.box.Faces[0]
        self.assertIsNot(first, second)
        duplicate_shapes = Part.ShapeList([first, second])
        duplicate_shapes.sort(key=lambda face: 0)
        self.assertIs(duplicate_shapes[0], first)
        self.assertIs(duplicate_shapes[1], second)

    def testCopyShares(self):
        faces = self.box.Faces
        other = faces.copy()
        self.assertIsInstance(other, list)
        self.assertEqual(len(other), 6)
        # A derived list has normal list ownership, but retains wrappers that
        # were already handed out by the source view.
        self.assertIs(faces[0], other[0])
        self.assertEqual(len(faces), 6)

    def testConstruction(self):
        self.assertEqual(len(Part.ShapeList()), 0)
        self.assertEqual(len(Part.ShapeList(self.box.Faces)), 6)
        self.assertEqual(len(Part.ShapeList([self.box, self.cyl])), 2)
        face = self.box.Faces[0]
        constructed = Part.ShapeList([face])
        self.assertIs(constructed[0], face)
        source = self.box.Faces
        copied = Part.ShapeList(source)
        self.assertIs(copied[0], source[0])
        made = Part.ShapeList(self.box, "Edge")
        self.assertTrue(made.IsView)
        self.assertEqual(len(made), 12)
        self.assertEqual(made.ElementType, "Edge")

    def testAvoidedType(self):
        # an edge of the box belongs to a face, so avoiding faces leaves none
        self.assertEqual(len(self.box.getChildShapes("Edge", "Face")), 0)
        self.assertEqual(len(self.box.getChildShapes("Edge")), 12)
        wire = Part.Wire(self.box.Faces[0].Edges)
        self.assertEqual(len(wire.getChildShapes("Edge", "Face")), 4)

    def testAViewSurvivesTheShapeItCameFrom(self):
        shape = Part.makeBox(1, 1, 1)
        faces = shape.Faces
        shape.Placement = App.Placement(App.Vector(10, 0, 0), App.Rotation())
        # the list is of the shape as it was asked for, not of the name
        self.assertEqual(len(faces), 6)
        self.assertAlmostEqual(faces[0].CenterOfMass.x, shape.Faces[0].CenterOfMass.x - 10, 6)

    def testNullShape(self):
        self.assertEqual(len(Part.Shape().Faces), 0)
        self.assertFalse(Part.Shape().Faces)

    def testElementMapSurvives(self):
        # asking for the elements of a mapped shape must not cost the shape
        # its own element map, nor give a different answer the second time
        doc = App.newDocument("ShapeListElementMap")
        try:
            box = doc.addObject("Part::Box", "Box")
            cyl = doc.addObject("Part::Cylinder", "Cyl")
            cut = doc.addObject("Part::Cut", "Cut")
            cut.Base = box
            cut.Tool = cyl
            doc.recompute()
            shape = cut.Shape
            before = shape.ElementMapSize
            self.assertGreater(before, 0)
            first = [f.ElementMapSize for f in shape.Faces]
            second = [f.ElementMapSize for f in shape.Faces]
            self.assertEqual(first, second)
            self.assertEqual(shape.ElementMapSize, before)
            names = [shape.getElementName("Face%d" % (i + 1), 1) for i in range(len(shape.Faces))]
            self.assertEqual(len(set(names)), len(names))
        finally:
            App.closeDocument("ShapeListElementMap")
