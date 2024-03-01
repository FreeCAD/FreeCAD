// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <BRepFilletAPI_MakeFillet.hxx>
#include "Mod/Part/App/FeaturePartCommon.h"
#include "Mod/Part/App/PropertyTopoShape.h"
#include <src/App/InitApplication.h>
#include "PartTestHelpers.h"
#include "Mod/Part/App/TopoShapeCompoundPy.h"

using namespace Part;
using namespace PartTestHelpers;

class PropertyTopoShapeTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _common = dynamic_cast<Common*>(_doc->addObject("Part::Common"));
        _common->Base.setValue(_boxes[0]);
        _common->Tool.setValue(_boxes[1]);
        _common->execute();  // We should now have an elementMap with 26 entries.
    }

    void TearDown() override
    {}

    Common* _common = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(PropertyTopoShapeTest, testPropertyPartShapeTopoShape)
{
    // Arrange
    auto partShape = PropertyPartShape();
    auto topoShapeIn = _common->Shape.getShape();
    auto topoDsShapeIn = _common->Shape.getShape().getShape();
    // Act
    partShape.setValue(topoShapeIn);
    auto topoShapeOut = partShape.getShape();
    auto topoDsShapeOut = partShape.getValue();
    // Assert
    EXPECT_TRUE(topoShapeOut.isSame(topoShapeIn));
    EXPECT_TRUE(topoDsShapeOut.IsSame(topoDsShapeIn));
    EXPECT_EQ(getVolume(topoDsShapeOut), 3);
#ifdef FC_USE_TNP_FIX
    EXPECT_EQ(topoShapeOut.getElementMapSize(), 26);
#else
    EXPECT_EQ(topoShapeOut.getElementMapSize(), 0);
#endif
}

TEST_F(PropertyTopoShapeTest, testPropertyPartShapeTopoDSShape)
{
    // Arrange
    auto property = _boxes[0]->addDynamicProperty("Part::PropertyPartShape", "test");
    auto partShape = dynamic_cast<PropertyPartShape*>(property);
    auto topoShapeIn = _common->Shape.getShape();
    auto topoDsShapeIn = _common->Shape.getShape().getShape();
    // Act
    // The second parameter of setValue, whether to resetElementMap is never called and irrelevant.
    // It appears to exist only to pass to the lower level setShape call.
    partShape->setValue(topoDsShapeIn);
    auto topoShapeOut = partShape->getShape();
    auto topoDsShapeOut = partShape->getValue();
    // Assert
    EXPECT_FALSE(topoShapeOut.isSame(topoShapeIn));
    EXPECT_TRUE(topoDsShapeOut.IsSame(topoDsShapeIn));
    EXPECT_EQ(getVolume(topoDsShapeOut), 3);
    EXPECT_EQ(topoShapeOut.getElementMapSize(), 0);  // We passed in a TopoDS_Shape so lost the map
}

TEST_F(PropertyTopoShapeTest, testPropertyPartShapeGetPyObject)
{
    // Arrange
    Py_Initialize();
    Base::PyGILStateLocker lock;
    auto partShape = PropertyPartShape();
    auto topoDsShapeIn = _common->Shape.getShape().getShape();
    auto topoDsShapeIn2 = _boxes[3]->Shape.getShape().getShape();
    // Act
    partShape.setValue(topoDsShapeIn);
    auto pyObj = partShape.getPyObject();
    // Assert
    EXPECT_TRUE(PyObject_TypeCheck(pyObj, &TopoShapeCompoundPy::Type));  // _common is a compound.
    // We can't build a TopoShapeCompoundPy directly ( protected destructor ), so we'll flip
    // the compound out and in to prove setPyObject works as expected.
    // Act
    partShape.setValue(topoDsShapeIn2);
    auto pyObj2 = partShape.getPyObject();
    // Assert the shape is no longer a compound
    EXPECT_FALSE(
        PyObject_TypeCheck(pyObj2, &TopoShapeCompoundPy::Type));  // _boxes[3] is not a compound.
    Py_XDECREF(pyObj2);
    // Act
    partShape.setPyObject(pyObj);
    auto pyObj3 = partShape.getPyObject();
    // Assert the shape returns to a compound if we setPyObject
    EXPECT_TRUE(PyObject_TypeCheck(pyObj3, &TopoShapeCompoundPy::Type));  // _common is a compound.
    Py_XDECREF(pyObj3);
    Py_XDECREF(pyObj);
}

// Possible future PropertyPartShape tests:
// Copy, Paste, getMemSize, beforeSave, Save. Restore


TEST_F(PropertyTopoShapeTest, testShapeHistory)
{
    // Arrange
    TopoDS_Shape baseShape = _boxes[0]->Shape.getShape().getShape();
    BRepFilletAPI_MakeFillet mkFillet(baseShape);
    auto edges = _boxes[0]->Shape.getShape().getSubTopoShapes(TopAbs_EDGE);
    mkFillet.Add(0.1, 0.1, TopoDS::Edge(edges[0].getShape()));
    TopoDS_Shape newShape = mkFillet.Shape();
    // Act to test the constructor
    auto hist = ShapeHistory(mkFillet, TopAbs_EDGE, newShape, baseShape);
    // Assert
    EXPECT_EQ(hist.type, TopAbs_EDGE);
    EXPECT_EQ(hist.shapeMap.size(), 11);  // We filleted away one of the cubes 12 Edges
    // Act to test the join operation
    hist.join(hist);
    // Assert
    EXPECT_EQ(hist.type, TopAbs_EDGE);
    EXPECT_EQ(hist.shapeMap.size(), 9);  // TODO: Is this correct?
    // Act to test the reset operation
    hist.reset(mkFillet, TopAbs_VERTEX, newShape, baseShape);
    // Assert
    EXPECT_EQ(hist.type, TopAbs_VERTEX);
    EXPECT_EQ(hist.shapeMap.size(), 8);  // Vertexes on a cube.
}

TEST_F(PropertyTopoShapeTest, testPropertyShapeHistory)
{
    // N/A nothing to really test
}

TEST_F(PropertyTopoShapeTest, testPropertyShapeCache)
{
    // Arrange
    PropertyShapeCache propertyShapeCache;
    TopoShape topoShapeIn {_boxes[0]->Shape.getShape()};  // Any TopoShape to test with
    TopoShape topoShapeOut;
    char* subName = "Face1";  // Cache key
    // Act
    auto gotShapeNotYet = propertyShapeCache.getShape(_boxes[0], topoShapeOut, subName);
    propertyShapeCache.setShape(_boxes[0], topoShapeIn, subName);
    auto gotShapeGood = propertyShapeCache.getShape(_boxes[0], topoShapeOut, subName);
    // Assert
    ASSERT_FALSE(gotShapeNotYet);
    ASSERT_TRUE(gotShapeGood);
    EXPECT_EQ(getVolume(topoShapeIn.getShape()), 6);
    EXPECT_EQ(getVolume(topoShapeOut.getShape()), 6);
    EXPECT_TRUE(topoShapeIn.isSame(topoShapeOut));
}

TEST_F(PropertyTopoShapeTest, testPropertyShapeCachePyObj)
{
    // Arrange
    Py_Initialize();
    Base::PyGILStateLocker lock;
    PropertyShapeCache catalyst;
    auto propertyShapeCache = catalyst.get(_boxes[1], true);
    auto faces = _boxes[1]->Shape.getShape().getSubTopoShapes(TopAbs_FACE);
    propertyShapeCache->setShape(_boxes[1], faces[0], "Face1");
    propertyShapeCache->setShape(_boxes[1], faces[1], "Face2");
    propertyShapeCache->setShape(_boxes[1], faces[2], "Face3");
    propertyShapeCache->setShape(_boxes[1], faces[3], "Face4");
    auto eraseList = PyList_New(2);
    PyList_SetItem(eraseList, 0, PyUnicode_FromString("Face2"));
    PyList_SetItem(eraseList, 1, PyUnicode_FromString("Face3"));
    // Act
    auto pyObjOut = propertyShapeCache->getPyObject();
    propertyShapeCache->setPyObject(eraseList);  // pyObjInRepr);
    auto pyObjOutErased = propertyShapeCache->getPyObject();
    // The faces that come back from python are in a random order compared to what was passed in
    // so testing them individually is difficult.  We'll just make sure the counts are right.
    EXPECT_EQ(PyList_Size(pyObjOut), 4);        // All four faces we added to the cache
    EXPECT_EQ(PyList_Size(pyObjOutErased), 2);  // setPyObject removed Face2 and Face3
    Py_XDECREF(pyObjOut);
    Py_XDECREF(pyObjOutErased);
}
