// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeatureCompound.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeatureCompoundTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _compound = dynamic_cast<Part::Compound*>(_doc->addObject("Part::Compound"));
    }

    void TearDown() override
    {}

    Part::Compound* _compound;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureCompoundTest, testIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0],_boxes[1]});

    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 3.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeatureCompoundTest, testNonIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0],_boxes[2]});

    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 5.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

