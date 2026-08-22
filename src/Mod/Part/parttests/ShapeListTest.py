# -*- coding: utf-8 -*-
# Tests for Part.ShapeList, what Shape.Faces and its siblings answer with.
#
# The point of the type is that it hands out an element only when one is
# asked for, so most of what is checked here is that it still behaves like
# the list it replaced.

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
            faces[6]
        with self.assertRaises(IndexError):
            faces[-7]

    def testIteration(self):
        faces = self.box.Faces
        walked = [f for f in faces]
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
        self.assertTrue(faces[3] in faces)
        self.assertEqual(faces.index(faces[3]), 3)
        self.assertEqual(faces.count(faces[3]), 1)
        self.assertFalse(self.cyl.Faces[0] in faces)
        with self.assertRaises(ValueError):
            faces.index(self.cyl.Faces[0])
        # something that is not a shape at all is simply not in the list
        self.assertFalse("Face1" in faces)

    def testTruth(self):
        self.assertTrue(self.box.Faces)
        self.assertFalse(self.box.CompSolids)
        self.assertEqual(len(self.box.CompSolids), 0)

    def testConcatenation(self):
        both = self.box.Faces + self.cyl.Faces
        self.assertEqual(len(both), 9)
        # with a plain list, on either side
        with_list = self.box.Faces + [self.cyl.Faces[0]]
        self.assertEqual(len(with_list), 7)
        list_first = [self.cyl.Faces[0]] + self.box.Faces
        self.assertEqual(len(list_first), 7)
        self.assertTrue(list_first[0].isSame(self.cyl.Faces[0]))
        # and the chain the Draft code writes
        chain = self.box.Edges + [self.cyl.Edges[0]] + self.box.Edges
        self.assertEqual(len(chain), 25)

    def testRepeat(self):
        twice = self.box.Faces * 2
        self.assertEqual(len(twice), 12)
        self.assertEqual(len(2 * self.box.Faces), 12)
        self.assertEqual(len(self.box.Faces * 0), 0)

    def testEquality(self):
        self.assertEqual(self.box.Faces, self.box.Faces)
        self.assertNotEqual(self.box.Faces, self.cyl.Faces)
        # against the plain list this used to be
        self.assertEqual(self.box.Faces, list(self.box.Faces))
        self.assertNotEqual(self.box.Faces, list(self.box.Faces)[:-1])

    def testConsumersTakeIt(self):
        # the calls that used to be handed a list
        wire = Part.Wire(self.box.Faces[0].Edges)
        self.assertEqual(wire.ShapeType, "Wire")
        shell = Part.makeShell(self.box.Faces)
        self.assertEqual(shell.ShapeType, "Shell")
        comp = Part.makeCompound(self.box.Faces)
        self.assertEqual(len(comp.Faces), 6)

    def testWritingDetaches(self):
        faces = self.box.Faces
        self.assertTrue(faces.IsView)
        faces.append(self.cyl.Faces[0])
        self.assertFalse(faces.IsView)
        self.assertIsNone(faces.Shape)
        self.assertEqual(len(faces), 7)
        # the shape is untouched by any of it
        self.assertEqual(len(self.box.Faces), 6)

    def testMutators(self):
        faces = self.box.Faces
        last = faces[5]
        popped = faces.pop()
        self.assertTrue(popped.isSame(last))
        self.assertEqual(len(faces), 5)

        faces.insert(0, popped)
        self.assertEqual(len(faces), 6)
        self.assertTrue(faces[0].isSame(popped))

        faces.remove(popped)
        self.assertEqual(len(faces), 5)

        faces.extend([popped, popped])
        self.assertEqual(len(faces), 7)

        first = faces[0]
        faces.reverse()
        self.assertTrue(faces[-1].isSame(first))

        faces += [popped]
        self.assertEqual(len(faces), 8)

        faces[0] = popped
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

    def testCopyShares(self):
        faces = self.box.Faces
        other = faces.copy()
        self.assertEqual(len(other), 6)
        other.append(self.cyl.Faces[0])
        self.assertEqual(len(other), 7)
        self.assertEqual(len(faces), 6)

    def testConstruction(self):
        self.assertEqual(len(Part.ShapeList()), 0)
        self.assertEqual(len(Part.ShapeList(self.box.Faces)), 6)
        self.assertEqual(len(Part.ShapeList([self.box, self.cyl])), 2)
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
