// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * This is a set of examples to help clarify both how the parts module works and how
 * to write tests for it.  This file is not really a test; everything tested should be redundant.
 * It also breaks the rules of good tests by stringing many different setup and assertions
 * together.  Finally, it has everything commented; real tests should ideally be so obvious
 * that minimal commenting is required for the reader to understand them.
 *
 * AND THE FILENAME IS OUTRAGEOUS AND MEANT JUST TO GET YOUR ATTENTION
 */

/**
 * Part tree of objects:
 *
 * DocumentObject in the document, either from an addObject, a python Part.show(), or a UI creation.
 * DocumentObject->Shape is a PropertyPartShape
 * DocumentObject->Shape.getShape() is a TopoShape.  Lowest level FC Object
 * DocumentObject->Shape.getValue() is a TopoShape.  Same as the PropertyPartShape getShape().
 * DocumentObject->Shape.getShape().getShape() is a TopoDS_Shape.  Highest level OpenCascade object.
 * DocumentObject->Shape.getShape().getShape().TShape() is a TopoDS_TShape.  Actual OpenCascade
 * Shape.
 *
 * TopoDS_Shape is an abstract with one of many possible concrete types:
 * https://dev.opencascade.org/doc/refman/html/class_topo_d_s___shape.html
 * TopoDS_TShape is an abstract with one of many possible concrete types:
 * https://dev.opencascade.org/doc/refman/html/class_topo_d_s___t_shape.html
 *
 * Toponaming applies a MappedName ( reference name based on the history of operations on a
 * TopoShape ) to each TopoShape that includes other shapes; containers, boolean operations,
 * touchups and the like.
 *
 */

/**
 * We'll retrieve the same values in many different ways to illustrate the different ways of using
 * the APIs.  In the real tests, these are scattered over multiple tests and files.
 */

#include "gtest/gtest.h"  // The testing framework.  Always required.

#include <src/App/InitApplication.h>  // FreeCAD setup for testing.

#include "PartTestHelpers.h"  // Useful helper classes and functions.

#include "Mod/Part/App/FeaturePartCommon.h"  // An actual object under test.
#include <BRepBuilderAPI_MakeVertex.hxx>     // An OpenCascade API declaration.

// Tests tend to be full of magic numbers, so let's make lint go away for them
// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

using namespace Part;             // Shorten the names of all the Part:: objects below.
using namespace PartTestHelpers;  // Shorten the names of all the PartTestHelper:: items.

/**
 * Enables the following use of TEST_F tests as opposed to TEST below, with each TEST_F
 * associated with this class which can do common setup and teardown tasks as well as
 * provide initialized instance variables for use.
 *
 * All test classes must inherit from ::testint::Test.
 * Inheriting from PublicTestHelperClass brings in some already initialized test objects.
 *
 * See https://google.github.io/googletest/primer.html for more information on using test framework.
 */
class FeatureTutorialTestClass: public ::testing::Test, public PartTestHelperClass
{
protected:  // Private members of this class will not be available to TEST_F methods,
    // as each one appears to become a subclass of this class.  Don't use them
    // unless they are only used in the methods in this class and not the tests.
    /**
     * https://google.github.io/googletest/advanced.html#sharing-resources-between-tests-in-the-same-test-suite
     */
    static void SetUpTestSuite()
    {
        tests::initApplication();  // Required for FreeCAD testing.
    }

    /**
     * https://google.github.io/googletest/reference/testing.html#classes-and-types
     */
    void SetUp() override
    {
        createTestDoc();  // PartTestHelper method that creates a document and test cubes
        _common = dynamic_cast<Common*>(_doc->addObject("Part::Common"));  // DocumentObject
    }

    void TearDown() override
    {}

    /**
     * Instance variable used in tests.
     */
    Common* _common = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureTutorialTestClass, testTutorial)
{
    // Arrange
    _boxes[0]->Shape.getShape().Tag = 1L;
    _boxes[1]->Shape.getShape().Tag = 2L;
    _common->Base.setValue(_boxes[0]);  // Setup the subelements of the common operation to
    _common->Tool.setValue(_boxes[1]);  // be the first two boxes created in PartTestHelperClass
    _common->Shape.getShape().Tag = 3L;

    // Act
    _common->execute();  // Create the common
    // Common is now referring to a single rectangular prism, 1 x 1 x 3 that represents the
    // intersection between two 1 x 2 x 3 boxes, one at the origin, and one at 0,1,0.
    // See PartTestHelpers.cpp for details.  That means 8 Vertexes, 12 Edges, 6 Wires,
    // 6 Faces, 1 Solid, 1 Shell, 1 Shape, 1 Compound, no CompSolids.

    // Two different ways of getting the TopoShape associated with the DocumentObject / Property
    TopoShape topoShape =
        _common->Shape.getValue();  // Get the TopoShape associated with the common
    TopoShape topoShape1 =
        _common->Shape.getShape();  // Get the TopoShape associated with the common
    TopoDS_Shape topoDSshape =
        topoShape.getShape();  // Down a level, get the OpenCascade TopoDS_Shape
    const TopoDS_Shape topoDSshape1 =
        topoShape1.getShape();  // Down a level, get the OpenCascade TopoDS_Shape
    EXPECT_STREQ(topoShape.shapeName().c_str(), topoShape1.shapeName().c_str());  // Same Name?
    EXPECT_TRUE(topoDSshape.IsEqual(topoDSshape1));                               // Same shape?

    // ASSERT Basic topology tests to prove the operation worked correctly.
    EXPECT_DOUBLE_EQ(getVolume(topoDSshape), 3.0);    // Correct shape volume?
    Base::BoundBox3d bbox = topoShape.getBoundBox();  // right bounding box?
    EXPECT_DOUBLE_EQ(bbox.MinX, 0.0);                 // Check bound
    EXPECT_DOUBLE_EQ(bbox.MinY, 1.0);                 // Check bound
    EXPECT_DOUBLE_EQ(bbox.MinZ, 0.0);                 // Check bound
    EXPECT_DOUBLE_EQ(bbox.MaxX, 1.0);                 // Check bound
    EXPECT_DOUBLE_EQ(bbox.MaxY, 2.0);                 // Check bound
    EXPECT_DOUBLE_EQ(bbox.MaxZ, 3.0);                 // Check bound

    // Test the TopoNaming enhancements
    TopoShape topoShape2 = _boxes[0]->Shape.getValue();  // or getShape()
    topoShape2.resetElementMap();  // required to avoid segfault from null elementMap.
    topoShape2.setElementComboName(
        Data::IndexedName("Arbitrary", 5),        // Real name like "Face5"
        {Data::MappedName("SampleMappedText")});  // Force a name in the map
    EXPECT_EQ(elementMap(topoShape2).size(), 1);  // We better just have the one entry we set.
    EXPECT_STREQ(topoShape2.getElementName("Arbitrary5").name.toString().c_str(),
                 "SampleMappedText;");  // Note the semicolon at the end from the mapping

    // Demonstrate the name parsing utilities.  Some are static methods as you can see.
    auto typeAndIndex = TopoShape::shapeTypeAndIndex("Face5");  // Parse name into two pieces
    auto eType = topoShape2.elementType("Face5");  // Type is the first character of name
    auto sType = TopoShape::shapeType(eType);      // Type converted to an enum
    EXPECT_EQ(eType, 'F');                         // There's the first character
    EXPECT_EQ(typeAndIndex.first, sType);          // Matching the type enum
    EXPECT_EQ(typeAndIndex.second, 5);             // And the index value

    // Accessing the subShapes
    EXPECT_EQ(topoShape.countSubShapes("Face"), 6);                  // Confirm expected
    EXPECT_EQ(topoShape.countSubElements("Face"), 6);                // Confirm expected
    std::vector<TopoDS_Shape> subShapes = topoShape.getSubShapes();  // Defaults to TopAbs_SHAPE
    EXPECT_EQ(subShapes.size(), 1);                                  // One Shape
    subShapes = topoShape.getSubShapes(TopAbs_COMPSOLID);
    EXPECT_EQ(subShapes.size(), 0);  // No CompSolids
    subShapes = topoShape.getSubShapes(TopAbs_COMPOUND);
    EXPECT_EQ(subShapes.size(), 1);  // One Compound
    subShapes = topoShape.getSubShapes(TopAbs_SHAPE);
    EXPECT_EQ(subShapes.size(), 1);  // One Shape
    subShapes = topoShape.getSubShapes(TopAbs_SOLID);
    EXPECT_EQ(subShapes.size(), 1);  // One Solid
    subShapes = topoShape.getSubShapes(TopAbs_SHELL);
    EXPECT_EQ(subShapes.size(), 1);  // One Shell
    subShapes = topoShape.getSubShapes(TopAbs_FACE);
    EXPECT_EQ(subShapes.size(), 6);  // Six Faces
    subShapes = topoShape.getSubShapes(TopAbs_WIRE);
    EXPECT_EQ(subShapes.size(), 6);  // 6 Wires, one per face
    subShapes = topoShape.getSubShapes(TopAbs_EDGE);
    EXPECT_EQ(subShapes.size(), 12);  // 12 Edges on a rectangular prism.
    subShapes = topoShape.getSubShapes(TopAbs_VERTEX);
    EXPECT_EQ(subShapes.size(), 8);  // 8 Vertices

    // Accessing TopoShapes containing the TopoDS_Shapes.  1-1 correspondence.
    std::vector<TopoShape> subTopoShapes = topoShape.getSubTopoShapes();
    EXPECT_EQ(subTopoShapes.size(), 1);  // Yep, one of them.
    subTopoShapes = topoShape.getSubTopoShapes(TopAbs_FACE);
    EXPECT_EQ(subTopoShapes.size(), 6);  // Same deal as above.
    // COMPSOLID, COMPOUNDS, SHAPE, SOLID, SHELL, WIRE, EDGE, VERTEX all as expected.

    // Confirm no element map
    EXPECT_EQ(elementMap(topoShape).size(), 0);  // Nothing in the element Map
    EXPECT_EQ(topoShape.Tag, 0);                 // We haven't set a Tag value
    // The call to subElement can throw exceptions.
    auto face1 = topoShape.getSubElement("Face", 1);
    EXPECT_STREQ(face1->getName().c_str(), "");  // No names at this level.

    auto ss1 = topoShape.getSubShape("Face1");
    EXPECT_FALSE(ss1.IsNull());

    // topoShape.reTagElementMap(2,nullptr,nullptr);

    // Create a single Vertex
    BRepBuilderAPI_MakeVertex vertexMaker = BRepBuilderAPI_MakeVertex(gp_Pnt(10, 10, 10));
    TopoShape topoShape3(vertexMaker.Vertex(),
                         1L);  // Nonzero Tag is required to create MappedNames
    EXPECT_EQ(topoShape3.getElementMap().size(), 0);  // No map yet.

    // Create the shape from that single vertex and test the elementMap
    // Note the reference used.  If you don't, a default constructed TopoShape will be created and
    // then initialized, and you'll loose the tag and the element map.
    TopoShape& result = topoShape3.makeElementShape(vertexMaker, topoShape3);  // create the shape
    EXPECT_EQ(result.getShape(),
              topoShape3.getShape());                 // Same topoShape is returned. Can use either.
    EXPECT_EQ(topoShape3.getElementMap().size(), 1);  // Now we have an elementMap
    auto elements = elementMap(result);               // Helper Method makes a map out of the vector
    EXPECT_EQ(elements.size(), 1);                    // One entry in the map
    EXPECT_EQ(elements.count(IndexedName("Vertex", 1)), 1);  // and it is "Vertex1"
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 0);    // definitely not "Face1"
    EXPECT_EQ(elements[IndexedName("Vertex", 1)],
              MappedName("Vertex1;MAK;:H:4,V"));  // A correct name exists
    EXPECT_EQ(getArea(result.getShape()), 0);     // Area of a single Vertex better be nothing
    auto mn1 = result.getElementName("Vertex1");  // access it another way.
    auto vecmap = result.getElementMap();         // get the raw Vector
    EXPECT_EQ(mn1, vecmap.front());  // and make sure it contains the same Vertex MappedElement
    EXPECT_STREQ(mn1.index.toString().c_str(), "Vertex1");  // and that the mapped element IndexName
    EXPECT_STREQ(mn1.name.toString().c_str(), "Vertex1;MAK;:H:4,V");  // Leads to the MappedName
    auto vec1 = result.getMappedChildElements();                      // access yet another way.
    EXPECT_EQ(vec1.size(), 1);          // still better be only one of them.
    auto child_element = vec1.front();  // get it
    EXPECT_STREQ(child_element.indexedName.toString().c_str(), "Vertex1");     // confirm the name
    EXPECT_STREQ(child_element.postfix.toStdString().c_str(), ";MAK;:H:4,V");  // the mapped value
    EXPECT_EQ(child_element.count, 1);                                         // the number of them
    EXPECT_EQ(child_element.offset, 0);        // where it starts in the name string
    EXPECT_EQ(child_element.tag, 0);           // the associated tag
    EXPECT_EQ(child_element.sids.count(), 0);  // any strings along the way

    auto fs = result.findShape("Vertex2");  // Locate a non existent sub shape
    EXPECT_TRUE(fs.IsNull());               // prove it.
    fs = result.findShape("Vertex1");       // locate the subshape that is there.
    EXPECT_FALSE(fs.IsNull());              // prove it

    // Now set up and test with tags and real mapped names ( as code becomes available )
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
