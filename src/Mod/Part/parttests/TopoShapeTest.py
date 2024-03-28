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
class TopoShapeTest(unittest.TestCase, TopoShapeAssertions):
    def setUp(self):
        """Create a document and some TopoShapes of various types"""
        self.doc = App.newDocument("TopoShape")
        self.box = Part.makeBox(1, 2, 2)
        Part.show(self.box, "Box1")
        self.box2 = Part.makeBox(2, 1, 2)
        Part.show(self.box2, "Box2")

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
            ["Content", ""],
            ["ElementMap", {}],
            ["ElementReverseMap", {}],
            # ['Hasher', {}],    # Todo:  Should this exist?  Different implementation?
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
        # Assert
        # This is a flag value to indicate that ElementMaps are supported under the current C++ build:
        if compound1.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            # 52 is 2 cubes of 26 each: 6 Faces, 12 Edges, 8 Vertexes
            # Todo: This should contain something as soon as the Python interface for Part.Compound TNP exists
            # self.assertEqual(len(compound1.ElementMap), 52, "ElementMap is Incorrect:  {0}".format(compound1.ElementMap))
            self.assertEqual(
                len(compound2.ElementReverseMap),
                52,
                "ElementMap is Incorrect:  {0}".format(compound2.ElementMap),
            )

    # def testTopoShapeOperations(self):
    #     compound1 = Part.Compound([self.doc.Box1.Shape, self.doc.Box2.Shape])
    #     box1ts = self.doc.Box1.Shape
    #     box2ts = self.doc.Box2.Shape
    #     face1 = Part.Face(Part.Wire([Part.makeCircle(10)]))
    #     cut1 = box1ts.cut(box2ts)
    #     common1 = box1ts.common(box2ts)
    #     fuse1 = box1ts.fuse(box2ts)
    #     fuse2 = box1ts.generalFuse([box2ts])
    #     fuse3 = box1ts.multiFuse([box2ts])
    #     mirror1 = box1ts.mirror(App.Vector(0, 0, 0), App.Vector(1, 0, 0))
    #     clean1 = box1ts.cleaned()
    #     # complement1 = box1ts.complement()
    #     # fix1 = box1ts.fix()
    #     rotate1 = box1ts.rotated(App.Vector(0, 0, 0), App.Vector(1, 0, 0), 45)
    #     scale1 = box1ts.scaled(2)
    #     translate1 = box1ts.translated((2, 0, 0))
    #     section1 = box1ts.section(face1)
    #     slice1 = box1ts.slice(App.Vector(1, 0, 0), 2)
    #     slice2 = box1ts.slices(App.Vector(1, 0, 0), [2, 3])
    #     # clean, complement, fix, reverse, scale,

    def testPartCommon(self):
        self.doc.addObject("Part::MultiCommon", "Common")
        self.doc.Common.Shapes = [self.doc.Box1, self.doc.Box2]
        self.doc.recompute()
        names = list(self.doc.Common.Shape.ElementReverseMap.keys())
        names.sort()
        if self.doc.Common.Shape.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(self.doc.Common.Shape.ElementReverseMap,
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

    def testPartCut(self):
        self.doc.addObject("Part::Cut", "Cut")
        self.doc.Cut.Base = self.doc.Box1
        self.doc.Cut.Tool = self.doc.Box2
        self.doc.recompute()
        if self.doc.Cut.Shape.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertKeysInMap(self.doc.Cut.Shape.ElementReverseMap,
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

    def testPartFuse(self):
        self.doc.addObject("Part::Fuse", "Fuse")
        self.doc.Fuse.Base = self.doc.Box1
        self.doc.Fuse.Tool = self.doc.Box2
        self.doc.recompute()
        if self.doc.Fuse.Shape.ElementMapVersion != "":  # Should be '4' as of Mar 2023.
            self.assertEqual(len(self.doc.Fuse.Shape.ElementReverseMap), 58)
            self.doc.Fuse.Refine = True
            self.doc.recompute()
            self.assertEqual(len(self.doc.Fuse.Shape.ElementReverseMap), 38)
        # Shape is an extruded L, with 8 Faces, 12 Vertexes, 18 Edges


# TODO: Consider the following possible test objects:
# Part::AttachExtension       ::init();
# Part::AttachExtensionPython ::init();
# Part::PrismExtension        ::init();
# Part::Feature               ::init();
# Part::FeatureExt            ::init();
# Part::BodyBase              ::init();
# Part::FeaturePython         ::init();
# Part::FeatureGeometrySet    ::init();
# Part::CustomFeature         ::init();
# Part::CustomFeaturePython   ::init();
# Part::Boolean               ::init();
# Part::Common                ::init();
# Part::MultiCommon           ::init();
# Part::Cut                   ::init();
# Part::Fuse                  ::init();
# Part::MultiFuse             ::init();
# Part::Section               ::init();
# Part::FilletBase            ::init();
# Part::Fillet                ::init();
# Part::Chamfer               ::init();
# Part::Compound              ::init();
# Part::Compound2             ::init();
# Part::Extrusion             ::init();
# Part::Scale                 ::init();
# Part::Revolution            ::init();
# Part::Mirroring             ::init();
#  TopoShape calls to be consider testing
#  'add',
#  'ancestorsOfType',
#  'applyRotation',
#  'applyTranslation',
#  'check',
#  'childShapes',
#  'cleaned',
#  'common',
#  'complement',
#  'connectEdgesToWires',
#  'copy',
#  'countElement',
#  'countSubElements',
#  'cut',
#  'defeaturing',
#  'distToShape',
#  'dumpContent',
#  'dumpToString',
#  'dumps',
#  'exportBinary',
#  'exportBrep',
#  'exportBrepToString',
#  'exportIges',
#  'exportStep',
#  'exportStl',
#  'extrude',
#  'findPlane',
#  'fix',
#  'fixTolerance',
#  'fuse',
#  'generalFuse',
#  'getAllDerivedFrom',
#  'getElement',
#  'getElementTypes',
#  'getFaces',
#  'getFacesFromSubElement',
#  'getLines',
#  'getLinesFromSubElement',
#  'getPoints',
#  'getTolerance',
#  'globalTolerance',
#  'hashCode',
#  'importBinary',
#  'importBrep',
#  'importBrepFromString',
#  'inTolerance',
#  'isClosed',
#  'isCoplanar',
#  'isDerivedFrom',
#  'isEqual',
#  'isInfinite',
#  'isInside',
#  'isNull',
#  'isPartner',
#  'isSame',
#  'isValid',
#  'limitTolerance',
#  'loads',
#  'makeChamfer',
#  'makeFillet',
#  'makeOffset2D',
#  'makeOffsetShape',
#  'makeParallelProjection',
#  'makePerspectiveProjection',
#  'makeShapeFromMesh',
#  'makeThickness',
#  'makeWires',
#  'mirror',
#  'multiFuse',
#  'nullify',
#  'oldFuse',
#  'optimalBoundingBox',
#  'overTolerance',
#  'project',
#  'proximity',
#  'read',
#  'reflectLines',
#  'removeInternalWires',
#  'removeShape',
#  'removeSplitter',
#  'replaceShape',
#  'restoreContent',
#  'reverse',
#  'reversed',
#  'revolve',
#  'rotate',
#  'rotated',
#  'scale',
#  'scaled',
#  'section',
#  'setFaces',
#  'sewShape',
#  'slice',
#  'slices',
#  'tessellate',
#  'toNurbs',
#  'transformGeometry',
#  'transformShape',
#  'transformed',
#  'translate',
#  'translated',
#  'writeInventor'
