// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartCut.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeaturePartCutTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _cut = static_cast<Part::Cut*>(_doc->addObject("Part::Cut"));
    }

    void TearDown() override
    {}

    Part::Cut* _cut;
};

TEST_F(FeaturePartCutTest, testIntersecting)
{
    // Arrange
    _cut->Base.setValue(_boxes[0]);
    _cut->Tool.setValue(_boxes[1]);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 3.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartCutTest, testNonIntersecting)
{
    // Arrange
    _cut->Base.setValue(_boxes[0]);
    _cut->Tool.setValue(_boxes[2]);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 6.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 2.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartCutTest, testTouching)
{
    // Arrange
    _cut->Base.setValue(_boxes[0]);
    _cut->Tool.setValue(_boxes[3]);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 6.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 2.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartCutTest, testAlmostTouching)
{
    // Arrange
    _cut->Base.setValue(_boxes[0]);
    _cut->Tool.setValue(_boxes[4]);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 6.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 2.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartCutTest, testBarelyIntersecting)
{
    // Arrange
    _cut->Base.setValue(_boxes[0]);
    _cut->Tool.setValue(_boxes[5]);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double target = 6 - Base::Precision::Confusion() * 3 * 1000;    // Reduce precision to 1e04 over 3 dimensions
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    // Using FLOAT, not DOUBLE here so test library comparison is of reasonable precision 1e07 rather than 1e15
    // See https://google.github.io/googletest/reference/assertions.html#floating-point
    EXPECT_FLOAT_EQ(volume, target);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 1.9999);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartCutTest, testMustExecute)
{
    // Assert initially we don't need to execute
    EXPECT_FALSE(_cut->mustExecute());
    // Act to change one property
    _cut->Base.setValue(_boxes[0]);
    // Assert we still can't execute
    EXPECT_FALSE(_cut->mustExecute());
    // Act to complete the properties we need
    _cut->Tool.setValue(_boxes[1]);
    // Assert that we now must execute
    EXPECT_TRUE(_cut->mustExecute());
    // Act to execute
    _doc->recompute();
    // Assert we don't need to execute anymore
    EXPECT_FALSE(_cut->mustExecute());
}

TEST_F(FeaturePartCutTest, testGetProviderName)
{
    // Act
    _cut->execute();
    const char* name = _cut->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

// See FeaturePartCommon.cpp for a history test.  It would be exactly the same and redundant here.