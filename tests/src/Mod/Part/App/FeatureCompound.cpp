// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

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
        _compound = _doc->addObject<Part::Compound>();
    }

    void TearDown() override
    {}

    Part::Compound* _compound = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureCompoundTest, testIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0], _boxes[1]});
    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 3.0, 3.0)));
    EXPECT_EQ(ts.countSubShapes(TopAbs_SHAPE), 2);
}

TEST_F(FeatureCompoundTest, testNonIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0], _boxes[2]});
    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 5.0, 3.0)));
    EXPECT_EQ(ts.countSubShapes(TopAbs_SHAPE), 2);
}
