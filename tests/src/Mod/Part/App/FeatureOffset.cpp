// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

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
        _offset = _doc->addObject<Part::Offset>();
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
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Vertex"), 8);
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Edge"), 12);
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Face"), 6);
}

TEST_F(FeatureOffsetTest, testOffset3DWithExistingElementMap)
{
    // Arrange
    Part::Fuse* _fuse = nullptr;  // NOLINT Can't be private in a test framework
    _fuse = _doc->addObject<Part::Fuse>();
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
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Vertex"), 8);
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Edge"), 12);
    EXPECT_EQ(_offset->Shape.getShape().countSubElements("Face"), 6);
}

TEST_F(FeatureOffsetTest, testOffset2D)
{
    // Arrange
    Part::Offset2D* _offset2 = _doc->addObject<Part::Offset2D>();
    Part::Plane* _pln = _doc->addObject<Part::Plane>();
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
    EXPECT_EQ(_offset2->Shape.getShape().getElementMapSize(), 9);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
