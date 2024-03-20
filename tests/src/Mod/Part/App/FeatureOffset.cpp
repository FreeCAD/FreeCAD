// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"
#include "Mod/Part/App/FeatureOffset.h"

using namespace PartTestHelpers;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
class FeatureOffsetTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _offset = dynamic_cast<Part::Offset*>(_doc->addObject("Part::Offset"));
        _offset->Source.setValue(_boxes[0]);
        _offset->Value.setValue(1);
        _offset->Join.setValue((int)JoinType::intersection);
        _offset->execute();
    }

    void TearDown() override
    {}

    Part::Offset* _offset = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureOffsetTest, testOffset3D)
{
    // Arrange
    Base::BoundBox3d bb = _offset->Shape.getShape().getBoundBox();
    // Assert size and position
    // a 1x2x3 box 3doffset by 1 becomes a 3x4x5 box, so volume is 60.
    EXPECT_EQ(getVolume(_offset->Shape.getShape().getShape()), 60);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(-1, -1, -1, 2, 3, 4)));
    // Assert correct element Map
#ifdef FC_USE_TNP_FIX
    EXPECT_TRUE(allElementsMatch(
        _offset->Shape.getShape(),
        {
            "Edge10;:G;OFS;:H47b:7,E",  "Edge11;:G;OFS;:H47b:7,E",  "Edge12;:G;OFS;:H47b:7,E",
            "Edge1;:G;OFS;:H47b:7,E",   "Edge2;:G;OFS;:H47b:7,E",   "Edge3;:G;OFS;:H47b:7,E",
            "Edge4;:G;OFS;:H47b:7,E",   "Edge5;:G;OFS;:H47b:7,E",   "Edge6;:G;OFS;:H47b:7,E",
            "Edge7;:G;OFS;:H47b:7,E",   "Edge8;:G;OFS;:H47b:7,E",   "Edge9;:G;OFS;:H47b:7,E",
            "Face1;:G;OFS;:H47b:7,F",   "Face2;:G;OFS;:H47b:7,F",   "Face3;:G;OFS;:H47b:7,F",
            "Face4;:G;OFS;:H47b:7,F",   "Face5;:G;OFS;:H47b:7,F",   "Face6;:G;OFS;:H47b:7,F",
            "Vertex1;:G;OFS;:H47b:7,V", "Vertex2;:G;OFS;:H47b:7,V", "Vertex3;:G;OFS;:H47b:7,V",
            "Vertex4;:G;OFS;:H47b:7,V", "Vertex5;:G;OFS;:H47b:7,V", "Vertex6;:G;OFS;:H47b:7,V",
            "Vertex7;:G;OFS;:H47b:7,V", "Vertex8;:G;OFS;:H47b:7,V",
        }));
#else
    EXPECT_EQ(_offset->Shape.getShape().getElementMapSize(), 0);
#endif
}

TEST_F(FeatureOffsetTest, testOffset3DWithExistingElementMap)
{
    // Arrange
    Part::Fuse* _fuse = nullptr;  // NOLINT Can't be private in a test framework
    _fuse = dynamic_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[1]);
    _fuse->Refine.setValue(true);
    // Act
    _fuse->execute();
    _offset->Source.setValue(_fuse);
    _offset->Value.setValue(2);
    _offset->execute();
    Base::BoundBox3d bb = _offset->Shape.getShape().getBoundBox();
    // Assert size and position
    // A 1x3x3 box 3doffset by 2 becomes a 5x7x7 box with volume of 245
    EXPECT_EQ(getVolume(_fuse->Shape.getShape().getShape()), 9);
    EXPECT_EQ(getVolume(_offset->Shape.getShape().getShape()), 245);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(-2, -2, -2, 3, 5, 5)));
    // Assert correct element Map
#ifdef FC_USE_TNP_FIX
    EXPECT_TRUE(elementsMatch(
        _offset->Shape.getShape(),
        {
            "Edge2;:M2(Edge2;:H366,E);FUS;:H365:17,E;:G(Edge2;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge2;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:G;OFS;:H36c:7,E;SLD;:H36c:4,E",
            "Edge2;:M2(Edge2;:H366,E);FUS;:H365:17,E;:G(Edge2;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge2;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U2;RFI;:H365:8,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge2;:M2(Edge2;:H366,E);FUS;:H365:17,E;:G(Edge2;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge2;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U;RFI;:H365:7,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge4;:M2(Edge4;:H366,E);FUS;:H365:17,E;:G(Edge4;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge4;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:G;OFS;:H36c:7,E;SLD;:H36c:4,E",
            "Edge4;:M2(Edge4;:H366,E);FUS;:H365:17,E;:G(Edge4;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge4;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U2;RFI;:H365:8,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge4;:M2(Edge4;:H366,E);FUS;:H365:17,E;:G(Edge4;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge4;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U;RFI;:H365:7,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge6;:M2(Edge6;:H366,E);FUS;:H365:17,E;:G(Edge6;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge6;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:G;OFS;:H36c:7,E;SLD;:H36c:4,E",
            "Edge6;:M2(Edge6;:H366,E);FUS;:H365:17,E;:G(Edge6;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge6;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U2;RFI;:H365:8,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge6;:M2(Edge6;:H366,E);FUS;:H365:17,E;:G(Edge6;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge6;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U;RFI;:H365:7,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge8;:M2(Edge8;:H366,E);FUS;:H365:17,E;:G(Edge8;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge8;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:G;OFS;:H36c:7,E;SLD;:H36c:4,E",
            "Edge8;:M2(Edge8;:H366,E);FUS;:H365:17,E;:G(Edge8;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge8;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U2;RFI;:H365:8,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            "Edge8;:M2(Edge8;:H366,E);FUS;:H365:17,E;:G(Edge8;:M2;FUS;:H366:8,E;K-1;:H366:4,E|"
            "Edge8;:M;FUS;:H365:7,E;K-1;:H365:4,E);RFI;:H365:53,E;:U;RFI;:H365:7,V;:G;OFS;:H36c:7,"
            "V;SLD;:H36c:4,V",
            // TODO:  Testing the Faces here was non-deterministic from run to run.  Is that okay?
        }));
#else
    EXPECT_EQ(_offset->Shape.getShape().getElementMapSize(), 0);
#endif
}

TEST_F(FeatureOffsetTest, testOffset2D)
{
    // Arrange
    Part::Offset2D* _offset2 = dynamic_cast<Part::Offset2D*>(_doc->addObject("Part::Offset2D"));
    Part::Plane* _pln = dynamic_cast<Part::Plane*>(_doc->addObject("Part::Plane"));
    _pln->Length.setValue(2);
    _pln->Width.setValue(3);
    _offset2->Source.setValue(_pln);
    _offset2->Value.setValue(1);
    _offset2->Join.setValue((int)JoinType::intersection);
    // Act
    _offset2->execute();
    Base::BoundBox3d bb = _offset2->Shape.getShape().getBoundBox();
    // Assert size and position
    // a 2x3 face 2doffset by 1 becomes a 4x5 face, so area is 20.
    EXPECT_EQ(getArea(_offset2->Shape.getShape().getShape()), 20);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(-1, -1, 0, 3, 4, 0)));
    // Assert correct element Map
    EXPECT_EQ(_offset2->Shape.getShape().getElementMapSize(), 0);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
