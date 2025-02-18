import FreeCAD as App
import Part

import unittest


class TopoShapeAssertions:

    def assertAttrEqual(self, toposhape, attr_value_list, msg=None):
        for attr, value in attr_value_list:
            result = toposhape.__getattribute__(
                attr
            )  # Look up each attribute by string name
            if result.__str__() != value.__str__():
                if msg == None:
                    msg = f"TopoShape {attr} is incorrect:  {result} should be {value}",
                raise AssertionError(msg)

    def assertAttrAlmostEqual(self, toposhape, attr_value_list, places=5, msg=None):
        range = 1 / 10 ** places
        for attr, value in attr_value_list:
            result = toposhape.__getattribute__(
                attr
            )  # Look up each attribute by string name
            if abs(result - value) > range:
                if msg == None:
                    msg = f"TopoShape {attr} is incorrect:  {result} should be {value}"
                raise AssertionError(msg)

    def assertAttrCount(self, toposhape, attr_value_list, msg=None):
        for attr, value in attr_value_list:
            result = toposhape.__getattribute__(
                attr
            )  # Look up each attribute by string name
            if len(result) != value:
                if msg == None:
                    msg = f"TopoShape {attr} is incorrect:  {result} should have {value} elements"
                raise AssertionError(msg)

    def assertKeysInMap(self, map, keys, msg=None):
        for key in keys:
            if not key in map:
                if msg == None:
                    msg = f"Key {key} not found in map:  {map}"
                raise AssertionError(msg)

    def assertBounds(self, shape, bounds, msg=None, precision=App.Base.Precision.confusion() * 100):
        shape_bounds = shape.BoundBox
        shape_bounds_max = App.BoundBox(shape_bounds)
        shape_bounds_max.enlarge(precision)
        bounds_max = App.BoundBox(bounds)
        bounds_max.enlarge(precision)
        if not (shape_bounds_max.isInside(bounds) and bounds_max.isInside(shape_bounds)):
            if msg == None:
                msg = f"Bounds {shape_bounds} doesn't match {bounds}"
            raise AssertionError(msg)


class TopoShapeTest(unittest.TestCase, TopoShapeAssertions):
    def setUp(self):
        """Create a document and some TopoShapes of various types"""
        self.doc = App.newDocument("TopoShape")
        # self.box = Part.makeBox(1, 2, 2)
        # Part.show(self.box, "Box1")
        # self.box2 = Part.makeBox(2, 1, 2)
        # Part.show(self.box2, "Box2")
        # Even on LS3 these boxes have no element maps.
        self.doc.addObject("Part::Box", "Box1")
        self.doc.Box1.Length = 1
        self.doc.Box1.Width = 2
        self.doc.Box1.Height = 2
        self.doc.addObject("Part::Box", "Box2")
        self.doc.Box2.Length = 2
        self.doc.Box2.Width = 1
        self.doc.Box2.Height = 2
        self.doc.addObject("Part::Cylinder", "Cylinder1")
        self.doc.Cylinder1.Radius = 0.5
        self.doc.Cylinder1.Height = 2
        self.doc.Cylinder1.Angle = 360
        self.doc.addObject("Part::Compound", "Compound1")
        self.doc.Compound1.Links = [ self.doc.Box1, self.doc.Box2 ]

        self.doc.recompute()
        self.box = self.doc.Box1.Shape
        self.box2 = self.doc.Box2.Shape

    def tearDown(self):
        App.closeDocument("TopoShape")

    def testTopoShapeBox(self):
        # Arrange our test TopoShape
        box2_toposhape = self.doc.Box2.Shape
        # Arrange list of attributes and values to string match
        attr_value_list = [
            ["BoundBox", App.BoundBox(0, 0, 0, 2, 1, 2)],
            ["CenterOfGravity", App.Vector(1, 0.5, 1)],
            ["CenterOfMass", App.Vector(1, 0.5, 1)],
            ["CompSolids", []],
            ["Compounds", []],
            ["Content", "<ElementMap/>\n"], # Our element map is empty, or there would be more here.
            ["ElementMap", {}],
            ["ElementReverseMap", {}],
            ['Hasher', None],
            [
                "MatrixOfInertia",
                App.Matrix(
                    1.66667, 0, 0, 0, 0, 2.66667, 0, 0, 0, 0, 1.66667, 0, 0, 0, 0, 1
                ),
            ],
            ["Module", "Part"],
            ["Orientation", "Forward"],
            # ['OuterShell', {}],    # Todo: Could verify that a Shell Object is returned
            ["Placement", App.Placement()],
            [
                "PrincipalProperties",
                {
                    "SymmetryAxis": True,
                    "SymmetryPoint": False,
                    "Moments": (
                        2.666666666666666,
                        1.666666666666667,
                        1.666666666666667,
                    ),
                    "FirstAxisOfInertia": App.Vector(0.0, 1.0, 0.0),
                    "SecondAxisOfInertia": App.Vector(0.0, 0.0, 1.0),
                    "ThirdAxisOfInertia": App.Vector(1.0, 0.0, 0.0),
                    "RadiusOfGyration": (
                        0.816496580927726,
                        0.6454972243679029,
                        0.6454972243679029,
                    ),
                },
            ],
            ["ShapeType", "Solid"],
            [
                "StaticMoments",
                (3.999999999999999, 1.9999999999999996, 3.999999999999999),
            ],
            # ['Tag', 0],    # Gonna vary, so can't really assert, except maybe != 0?
            ["TypeId", "Part::TopoShape"],
        ]
        # Assert all the expected values match when converted to strings.
        self.assertAttrEqual(box2_toposhape, attr_value_list)

        # Arrange list of attributes and values to match within 5 decimal places
        attr_value_list = [
            ["Area", 16.0],
            ["ElementMapSize", 0],
            # ['ElementMapVersion', 4 ], # Todo: Not until TNP on.
            ["Length", 40.0],  # Sum of all edges of each face, so some redundancy.
            ["Mass", 4.0],
            # ['MemSize', 13824],  # Platform variations in this size.
            ["Volume", 4.0],
        ]
        # Assert all the expected values match
        self.assertAttrAlmostEqual(box2_toposhape, attr_value_list, 5)

        # Arrange list of attributes to check list lengths
        attr_value_list = [
            ["Edges", 12],
            ["Faces", 6],
            ["Shells", 1],
            ["Solids", 1],
            ["SubShapes", 1],
            ["Vertexes", 8],
            ["Wires", 6],
        ]
        # Assert all the expected values match
        self.assertAttrCount(box2_toposhape, attr_value_list)

    def testTopoShapeElementMap(self):
        """Tests TopoShape elementMap"""
        # Arrange
        # Act No elementMaps exist in base shapes until we perform an operation.
        compound1 = Part.Compound(
            [self.doc.Objects[-1].Shape, self.doc.Objects[-2].Shape]
        )
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound2 = self.doc.Compound.Shape
        # Assert elementMap
        # This flag indicates that ElementMaps are supported under the current C++ build:
        if compound1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            # 52 is 2 cubes of 26 each: 6 Faces, 12 Edges, 8 Vertexes
            # Todo: This should contain something as soon as the Python interface
            #  for Part.Compound TNP exists
            # self.assertEqual(len(compound1.ElementMap), 52,
            #                  "ElementMap is Incorrect:  {0}".format(compound1.ElementMap))
            self.assertEqual(
                compound2.ElementMapSize,
                52,
                "ElementMap is Incorrect:  {0}".format(compound2.ElementMap),
            )
        # Assert Shape
        self.assertBounds(compound2, App.BoundBox(0, 0, 0, 2, 2, 2))

    def testPartCommon(self):
        # Arrange
        self.doc.addObject("Part::MultiCommon", "Common")
        self.doc.Common.Shapes = [self.doc.Box1, self.doc.Box2]
        # Act
        self.doc.recompute()
        common1 = self.doc.Common.Shape
        # Assert elementMap
        if common1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(common1.ElementReverseMap,
                                 [
                                     "Edge1",
                                     "Edge2",
                                     "Edge3",
                                     "Edge4",
                                     "Edge5",
                                     "Edge6",
                                     "Edge7",
                                     "Edge8",
                                     "Edge9",
                                     "Edge10",
                                     "Edge11",
                                     "Edge12",
                                     "Face1",
                                     "Face2",
                                     "Face3",
                                     "Face4",
                                     "Face5",
                                     "Face6",
                                     "Vertex1",
                                     "Vertex2",
                                     "Vertex3",
                                     "Vertex4",
                                     "Vertex5",
                                     "Vertex6",
                                     "Vertex7",
                                     "Vertex8",
                                 ],
                                 )
        # Assert Shape
        self.assertBounds(common1, App.BoundBox(0, 0, 0, 1, 1, 2))

    def testPartCut(self):
        # Arrange
        self.doc.addObject("Part::Cut", "Cut")
        self.doc.Cut.Base = self.doc.Box1
        self.doc.Cut.Tool = self.doc.Box2
        # Act
        self.doc.recompute()
        cut1 = self.doc.Cut.Shape
        # Assert elementMap
        if cut1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(cut1.ElementReverseMap,
                                 [
                                     "Edge1",
                                     "Edge2",
                                     "Edge3",
                                     "Edge4",
                                     "Edge5",
                                     "Edge6",
                                     "Edge7",
                                     "Edge8",
                                     "Edge9",
                                     "Edge10",
                                     "Edge11",
                                     "Edge12",
                                     "Face1",
                                     "Face2",
                                     "Face3",
                                     "Face4",
                                     "Face5",
                                     "Face6",
                                     "Vertex1",
                                     "Vertex2",
                                     "Vertex3",
                                     "Vertex4",
                                     "Vertex5",
                                     "Vertex6",
                                     "Vertex7",
                                     "Vertex8",
                                 ],
                                 )
        # Assert Shape
        self.assertBounds(cut1, App.BoundBox(0, 1, 0, 1, 2, 2))

    def testPartFuse(self):
        # Arrange
        self.doc.addObject("Part::Fuse", "Fuse")
        self.doc.Fuse.Refine = False
        self.doc.Fuse.Base = self.doc.Box1
        self.doc.Fuse.Tool = self.doc.Box2
        # Act
        self.doc.recompute()
        fuse1 = self.doc.Fuse.Shape
        # Assert elementMap
        if fuse1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(fuse1.ElementMapSize, 58)
            self.doc.Fuse.Refine = True
            self.doc.recompute()
            self.assertEqual(fuse1.ElementMapSize, 58)
        # Shape is an extruded L, with 8 Faces, 12 Vertexes, 18 Edges
        # Assert Shape
        self.assertBounds(fuse1, App.BoundBox(0, 0, 0, 2, 2, 2))

    def testAppPartMakeCompound(self):
        # This doesn't do element maps.
        # compound1 = Part.Compound([self.doc.Box1.Shape, self.doc.Box2.Shape])
        # Act
        compound1 = Part.makeCompound([self.doc.Box1.Shape, self.doc.Box2.Shape])
        # Assert elementMap
        if compound1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound1.ElementMapSize, 52)
        # Assert Shape
        self.assertBounds(compound1, App.BoundBox(0, 0, 0, 2, 2, 2))

    def testAppPartMakeShell(self):
        # Act
        shell1 = Part.makeShell(self.doc.Box1.Shape.Faces)
        # Assert elementMap
        if shell1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(shell1.ElementMapSize, 26)
        # Assert Shape
        self.assertBounds(shell1, App.BoundBox(0, 0, 0, 1, 2, 2))

    def testAppPartMakeFace(self):
        # Act
        face1 = Part.makeFace(self.doc.Box1.Shape.Faces[0], "Part::FaceMakerCheese")
        # Assert elementMap
        if face1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(face1.ElementMapSize, 10)
        # Assert Shape
        self.assertBounds(face1, App.BoundBox(0, 0, 0, 0, 2, 2))

    def testAppPartmakeFilledFace(self):
        face1 = Part.makeFilledFace(self.doc.Box1.Shape.Faces[3].Edges)
        # Assert elementMap
        if face1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(face1.ElementMapSize, 9)
        # Assert Shape
        self.assertBounds(face1, App.BoundBox(-0.05, 2, -0.1, 1.05, 2, 2.1))

    def testAppPartMakeSolid(self):
        # Act
        solid1 = Part.makeSolid(self.doc.Box1.Shape.Shells[0])
        # Assert elementMap
        if solid1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(solid1.ElementMapSize, 26)
        # Assert Shape
        self.assertBounds(solid1, App.BoundBox(0, 0, 0, 1, 2, 2))

    def testAppPartMakeRuled(self):
        # Act
        surface1 = Part.makeRuledSurface(*self.doc.Box1.Shape.Edges[3:5])
        # Assert elementMap
        if surface1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(surface1.ElementMapSize, 9)
        # Assert Shape
        self.assertBounds(surface1, App.BoundBox(0, 0, 0, 1, 2, 2))

    def testAppPartMakeShellFromWires(self):
        # Arrange
        wire1 = self.doc.Box1.Shape.Wires[0]  # .copy() Todo: prints 2 gen/mod warn because
        wire2 = self.doc.Box1.Shape.Wires[1]  # Todo: copy() isn't TNP yet.  Fix when it is.
        # Act
        shell1 = Part.makeShellFromWires([wire1, wire2])
        # Assert elementMap
        if shell1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(shell1.ElementMapSize, 24)
        # Assert Shape
        self.assertBounds(shell1, App.BoundBox(0, 0, 0, 1, 2, 2))

    def testAppPartMakeSweepSurface(self):
        # Arrange
        circle = Part.makeCircle(5, App.Vector(0, 0, 0))
        path = Part.makeLine(App.Vector(), App.Vector(0, 0, 10))
        Part.show(circle, "Circle")  # Trigger the elementMapping
        Part.show(path, "Path")  # Trigger the elementMapping
        del circle
        # Act
        surface1 = Part.makeSweepSurface(self.doc.Path.Shape, self.doc.Circle.Shape, 0.001, 0)
        Part.show(surface1, "Sweep")
        self.doc.recompute()
        # Assert elementMap
        if surface1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(surface1.ElementMapSize, 6)
            self.assertBounds(surface1, App.BoundBox(-5, -5, 0, 5, 5, 10), precision=2)
        else:
            # Todo: WHY is the actual sweep different?  That's BAD.  However, the "New" approach
            #       above, which uses BRepOffsetAPI_MakePipe appears to be correct over the older
            #       code which uses Geom_Curve.  This is done ostensibly because Geom_Curve is so
            #       old that it doesn't even support history, which toponaming needs, but also,
            #       the result is just wrong:  If you look at the resulting shape after Sweeping
            #       a circle along a line, you do not get a circular pipe:  you get a circular
            #       pipe with About a third of it removed.  More specifically, an angle of
            #       math.radians(math.degrees(360)%180) * 2 appears to have been applied, which
            #       looks suspiciously like a substantial bug in OCCT.
            # Assert Shape
            self.assertBounds(surface1, App.BoundBox(-5, -2.72011, 0, 5, 5, 6.28319), precision=2)
        del surface1

    def testAppPartMakeLoft(self):
        # Act
        solid1 = Part.makeLoft(self.doc.Box1.Shape.Wires[0:2])
        # Assert elementMap
        if solid1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(solid1.ElementMapSize, 24)
        # Assert Shape
        self.assertBounds(solid1, App.BoundBox(0, 0, 0, 1, 2, 2))

    def testAppPartMakeSplitShape(self):
        # Todo: Refine this test after all TNP code in place to eliminate warnings.
        # Arrange
        edge1 = self.doc.Box1.Shape.Faces[0].Edges[0].translated(App.Vector(0, 0.5, 0))
        face1 = self.doc.Box1.Shape.Faces[0]
        # Act
        solids1 = Part.makeSplitShape(face1, [(edge1, face1)])
        # Assert elementMap
        self.assertEqual(len(solids1), 2)
        self.assertEqual(len(solids1[0]), 1)
        if solids1[0][0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(solids1[0][0].ElementMapSize, 9)
            self.assertEqual(solids1[1][0].ElementMapSize, 9)
        # Assert Shape
        self.assertBounds(solids1[0][0], App.BoundBox(0, 0.5, 0, 0, 2, 2))
        self.assertBounds(solids1[1][0], App.BoundBox(0, 0.5, 0, 0, 2, 2))

    def testTopoShapePyInit(self):
        # Arrange
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound = self.doc.Compound.Shape
        # Act
        new_toposhape = Part.Shape(compound)
        new_empty_toposhape = Part.Shape()
        # Assert elementMap
        if compound.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound.ElementMapSize, 52)
            self.assertEqual(new_toposhape.ElementMapSize, 52)

    def testTopoShapeCopy(self):
        # Arrange
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound = self.doc.Compound.Shape
        # Act
        compound_copy = compound.copy()
        # Assert elementMap
        if compound.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound.ElementMapSize, 52)
            self.assertEqual(compound_copy.ElementMapSize, 52)

    def testTopoShapeCleaned(self):
        # Arrange
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound = self.doc.Compound.Shape
        # Act
        compound_cleaned = compound.cleaned()
        # Assert elementMap
        if compound.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound.ElementMapSize, 52)
            self.assertEqual(compound_cleaned.ElementMapSize, 52)

    def testTopoShapeReplaceShape(self):
        # Arrange
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound = self.doc.Compound.Shape
        # Act
        compound_replaced = compound.replaceShape([(App.activeDocument().Box2.Shape,
                                                    App.activeDocument().Box1.Shape)])
        # Assert elementMap
        if compound.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound.ElementMapSize, 52)
            self.assertEqual(compound_replaced.ElementMapSize, 52)

    def testTopoShapeRemoveShape(self):
        # Arrange
        self.doc.addObject("Part::Compound", "Compound")
        self.doc.Compound.Links = [
            App.activeDocument().Box1,
            App.activeDocument().Box2,
        ]
        self.doc.recompute()
        compound = self.doc.Compound.Shape
        # Act
        compound_removed = compound.removeShape([App.ActiveDocument.Box2.Shape])
        # Assert elementMap
        if compound.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compound.ElementMapSize, 52)
            self.assertEqual(compound_removed.ElementMapSize, 52)

    def testTopoShapeExtrude(self):
        # Arrange
        face = self.doc.Box1.Shape.Faces[0]
        # Act
        extrude = face.extrude(App.Vector(2, 0, 0))
        self.doc.recompute()
        # Assert elementMap
        if extrude.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(extrude.ElementMapSize, 26)

    def testTopoShapeRevolve(self):
        # Arrange
        face = self.doc.Box1.Shape.Faces[0]
        # Act
        face.revolve(App.Vector(), App.Vector(1, 0, 0), 45)
        self.doc.recompute()
        # Assert elementMap
        if face.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(face.ElementMapSize, 9)

    def testTopoShapeFuse(self):
        # Act
        fused = self.doc.Box1.Shape.fuse(self.doc.Box2.Shape)
        self.doc.recompute()
        # Assert elementMap
        if fused.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(fused.ElementMapSize, 58)

    def testTopoShapeMultiFuse(self):
        # Act
        fused = self.doc.Box1.Shape.multiFuse([self.doc.Box2.Shape])
        self.doc.recompute()
        # Assert elementMap
        if fused.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(fused.ElementMapSize, 58)

    def testTopoShapeCommon(self):
        # Act
        common = self.doc.Box1.Shape.common(self.doc.Box2.Shape)
        self.doc.recompute()
        # Assert elementMap
        if common.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(common.ElementMapSize, 26)

    def testTopoShapeSection(self):
        # Act
        section = self.doc.Box1.Shape.Faces[0].section(self.doc.Box2.Shape.Faces[3])
        self.doc.recompute()
        # Assert elementMap
        if section.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(section.ElementMapSize, 3)

    def testTopoShapeSlice(self):
        # Act
        slice = self.doc.Box1.Shape.slice(App.Vector(10, 10, 0), 1)
        self.doc.recompute()
        # Assert elementMap
        self.assertEqual(len(slice), 1)
        if slice[0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(slice[0].ElementMapSize, 8)

    def testTopoShapeSlices(self):
        # Act
        slices = self.doc.Box1.Shape.Faces[0].slices(App.Vector(10, 10, 0), [1, 2])
        self.doc.recompute()
        # Assert elementMap
        if slices.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(slices.ElementMapSize, 6)

    def testTopoShapeCut(self):
        # Act
        cut = self.doc.Box1.Shape.cut(self.doc.Box2.Shape)
        self.doc.recompute()
        # Assert elementMap
        if cut.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(cut.ElementMapSize, 26)

    def testTopoShapeGeneralFuse(self):
        # Act
        fuse = self.doc.Box1.Shape.generalFuse([self.doc.Box2.Shape])
        self.doc.recompute()
        # Assert elementMap
        self.assertEqual(len(fuse), 2)
        if fuse[0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(fuse[0].ElementMapSize, 60)

    def testTopoShapeChildShapes(self):
        # Act
        childShapes = self.doc.Box1.Shape.childShapes()
        self.doc.recompute()
        # Assert elementMap
        self.assertEqual(len(childShapes), 1)
        if childShapes[0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(childShapes[0].ElementMapSize, 26)

    def testTopoShapeMirror(self):
        # Act
        mirror = self.doc.Box1.Shape.mirror(App.Vector(), App.Vector(1, 0, 0))
        self.doc.recompute()
        # Assert elementMap
        if mirror.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(mirror.ElementMapSize, 26)

    def testTopoShapeScale(self):
        # Act
        scale = self.doc.Box1.Shape.scaled(2)
        self.doc.recompute()
        # Assert elementMap
        if scale.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(scale.ElementMapSize, 26)

    def testTopoShapeMakeFillet(self):
        # Act
        fillet = self.doc.Box1.Shape.makeFillet(0.1, self.doc.Box1.Shape.Faces[0].Edges)
        self.doc.recompute()
        # Assert elementMap
        if fillet.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(fillet.ElementMapSize, 42)

    def testTopoShapeMakeChamfer(self):
        # Act
        chamfer = self.doc.Box1.Shape.makeChamfer(0.1, self.doc.Box1.Shape.Faces[0].Edges)
        self.doc.recompute()
        # Assert elementMap
        if chamfer.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(chamfer.ElementMapSize, 42)

    def testTopoShapeMakeThickness(self):
        # Act
        thickness = self.doc.Box1.Shape.makeThickness(self.doc.Box1.Shape.Faces[0:2], 0.1, 0.0001)
        self.doc.recompute()
        # Assert elementMap
        if thickness.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(thickness.ElementMapSize, 74)

    def testTopoShapeMakeOffsetShape(self):
        # Act
        offset = self.doc.Box1.Shape.Faces[0].makeOffset(1)
        self.doc.recompute()
        # Assert elementMap
        if offset.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(offset.ElementMapSize, 17)

    def testTopoShapeOffset2D(self):
        # Act
        offset = self.doc.Box1.Shape.Faces[0].makeOffset2D(1)
        self.doc.recompute()
        # Assert elementMap
        if offset.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(offset.ElementMapSize, 17)

    def testTopoShapeRemoveSplitter(self):
        # Act
        fused = self.doc.Box1.Shape.fuse(self.doc.Box2.Shape)
        removed = fused.removeSplitter()
        self.doc.recompute()
        # Assert elementMap
        if removed.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(removed.ElementMapSize, 38)

    def testTopoShapeCompSolid(self):
        # Act
        compSolid = Part.CompSolid([self.doc.Box1.Shape, self.doc.Box2.Shape])  # list of subobjects
        box1ts = self.doc.Box1.Shape
        compSolid.add(box1ts.Solids[0])
        # Assert elementMap
        if compSolid.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(compSolid.ElementMapSize, 78)

    def testTopoShapeFaceOffset(self):
        # Arrange
        box_toposhape = self.doc.Box1.Shape
        # Act
        offset = box_toposhape.Faces[0].makeOffset(2.0)
        # Assert elementMap
        if box_toposhape.Faces[0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(box_toposhape.Faces[0].ElementMapSize, 9)  # 1 Face, 4 Edges, 4 Vertexes
            self.assertEqual(offset.ElementMapSize, 17)  # 1 Face, 8 Edges, 8 Vertexes

    # Todo:  makeEvolved doesn't work right, probably due to missing c++ code.
    # def testTopoShapeFaceEvolve(self):
    #     # Arrange
    #     box_toposhape = self.doc.Box1.Shape
    #     # Act
    #     evolved = box_toposhape.Faces[0].makeEvolved(self.doc.Box1.Shape.Wires[1])  # 2,3,4,5 bad
    #     # Assert elementMap
    #     if box_toposhape.Faces[0].ElementMapVersion != "":  # Should be '4' as of Mar 2023.
    #         self.assertEqual(box_toposhape.Faces[0].ElementMapSize, 9)  # 1 Face, 4 Edges, 4 Vertexes
    #         self.assertEqual(evolved.ElementMapSize, 0)  # Todo: This can't be correct.

    def testTopoShapePart(self):
        # Arrange
        box1ts = self.doc.Box1.Shape
        face1 = box1ts.Faces[0]
        box1ts2 = box1ts.copy()
        # Act
        face2 = box1ts.getElement("Face2")
        indexed_name = box1ts.findSubShape(face1)
        faces1 = box1ts.findSubShapesWithSharedVertex(face2)
        subshapes1 = box1ts.getChildShapes("Solid1")
        # box1ts.clearCache()   # Todo: no apparent way to see a result at this level
        # Assert
        self.assertTrue(face2.isSame(box1ts.Faces[1]))
        self.assertEqual(indexed_name[0], "Face")
        self.assertEqual(indexed_name[1], 1)
        self.assertEqual(len(faces1), 1)
        self.assertTrue(faces1[0].isSame(box1ts.Faces[1]))
        self.assertEqual(len(subshapes1), 1)
        self.assertTrue(subshapes1[0].isSame(box1ts.Solids[0]))

    def testTopoShapeMapSubElement(self):
        # Arrange
        box = Part.makeBox(1,2,3)
        # face = box.Faces[0]   # Do not do this.  Need the subelement call each usage.
        # Assert everything empty
        self.assertEqual(box.ElementMapSize,0)
        self.assertEqual(box.Faces[0].ElementMapSize,0)
        # Act
        box.mapSubElement(box.Faces[0])
        # Assert elementMaps created
        if box.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(box.ElementMapSize,9)  # 1 Face, 4 Edges, 4 Vertexes
            self.assertEqual(box.Faces[0].ElementMapSize,9)

    def testTopoShapeGetElementHistory(self):
        self.doc.addObject("Part::Fuse", "Fuse")
        self.doc.Fuse.Base = self.doc.Box1
        self.doc.Fuse.Tool = self.doc.Box2
        # Act
        self.doc.recompute()
        fuse1 = self.doc.Fuse.Shape
        if fuse1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            history1 = fuse1.getElementHistory(fuse1.ElementReverseMap["Vertex1"])
            # Assert
            self.assertEqual(len(history1),3)   # Just the Fuse operation

    # Todo:  Still broken, still can't find parms that consistently work to test this.
    #           However, the results with an empty elementMap are consistent with making the
    #           same calls on LS3.  So what this method is supposed to do remains a mystery;
    #           So far, it just wipes out the elementMap and returns the Toposhape.
    # def testTopoShapeMapShapes(self):
    #     self.doc.addObject("Part::Fuse", "Fuse")
    #     self.doc.Fuse.Base = self.doc.Box1
    #     self.doc.Fuse.Tool = self.doc.Box2
    #     # Act
    #     self.doc.recompute()
    #     fuse1 = self.doc.Fuse.Shape
    #     res = fuse1.copy()  # Make it mutable
    #     self.assertEqual(res.ElementMapSize,58)
    #     result = res.mapShapes([(fuse1, fuse1.Faces[0])], []) #[(res, res.Vertexes[0])])
    #     self.assertEqual(res.ElementMapSize,9)
    #     # result2 = fuse1.copy().mapShapes([],[(fuse1, fuse1.Edges[0]),(fuse1, fuse1.Edges[1])])
    #     self.assertEqual(fuse1.ElementMapSize,58) #
    #     self.assertEqual(fuse1.Faces[0].ElementMapSize,9)
    #     self.assertEqual(result.ElementMapSize,9)
    #     self.assertEqual(result.Faces[0].ElementMapSize,0)
    #     self.assertEqual(result2.ElementMapSize,9)
    #     self.assertEqual(result2.Faces[0].ElementMapSize,0)

    def testPartCompoundCut1(self):
        # Arrange
        self.doc.addObject("Part::Cut", "Cut")
        self.doc.Cut.Base = self.doc.Cylinder1
        self.doc.Cut.Tool = self.doc.Compound1
        # Act
        self.doc.recompute()
        cut1 = self.doc.Cut.Shape
        # Assert elementMap
        refkeys = [
            'Vertex6', 'Vertex5', 'Edge7', 'Edge8', 'Edge9', 'Edge5', 'Edge6', 'Face4', 'Face2',
            'Edge1', 'Vertex4', 'Edge4', 'Vertex3', 'Edge2', 'Edge3', 'Face1', 'Face5', 'Face3',
            'Vertex1', 'Vertex2'
        ]
        if cut1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(cut1.ElementReverseMap, refkeys )
        self.assertEqual(len(cut1.ElementReverseMap.keys()),len(refkeys))
        # Assert Volume
        self.assertAlmostEqual(cut1.Volume, self.doc.Cylinder1.Shape.Volume * (3/4))

    def testPartCompoundCut2(self):
        # Arrange
        self.doc.addObject("Part::Cut", "Cut")
        self.doc.Cut.Base = self.doc.Compound1
        self.doc.Cut.Tool = self.doc.Cylinder1
        # Act
        self.doc.recompute()
        cut1 = self.doc.Cut.Shape
        # Assert elementMap
        refkeys = [
            'Vertex3', 'Vertex4', 'Vertex8', 'Vertex10', 'Vertex7', 'Vertex9', 'Vertex13',
            'Vertex14', 'Vertex18', 'Vertex20', 'Vertex17', 'Vertex19', 'Edge3', 'Edge15', 'Edge9',
            'Edge12', 'Edge13', 'Edge11', 'Edge8', 'Edge17', 'Edge18', 'Edge19', 'Edge30', 'Edge24',
            'Edge27', 'Edge28', 'Edge29', 'Edge25', 'Edge26', 'Edge23', 'Face7', 'Face4', 'Face8',
            'Face14', 'Face13', 'Face11', 'Face12', 'Face10', 'Edge22', 'Vertex12', 'Edge20',
            'Vertex11', 'Edge21', 'Edge16', 'Face9', 'Vertex15', 'Vertex16'
        ]
        if cut1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(cut1.ElementReverseMap, refkeys )
        self.assertEqual(len(cut1.ElementReverseMap.keys()),len(refkeys))
