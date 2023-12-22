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
        createTestFile();
        _cut = static_cast<Part::Cut*>(_doc->addObject("Part::Cut"));
    }

    void TearDown() override
    {}


    Part::Cut* _cut;
};

TEST_F(FeaturePartCutTest, testIntersecting)
{
    // Arrange
    _cut->Base.setValue(_box1obj);
    _cut->Tool.setValue(_box2obj);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume,3.0);
}

TEST_F(FeaturePartCutTest, testNonIntersecting)
{
    // Arrange
    _cut->Base.setValue(_box1obj);
    _cut->Tool.setValue(_box3obj);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume,6.0);
}

TEST_F(FeaturePartCutTest, testTouching)
{
    // Arrange
    _cut->Base.setValue(_box1obj);
    _cut->Tool.setValue(_box4obj);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());

    // Assert
    EXPECT_DOUBLE_EQ(volume,6.0);
}

TEST_F(FeaturePartCutTest, testAlmostTouching)
{
    // Arrange
    _cut->Base.setValue(_box1obj);
    _cut->Tool.setValue(_box5obj);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume,6.0);
}

TEST_F(FeaturePartCutTest, testBarelyIntersecting)
{
    // Arrange
    _cut->Base.setValue(_box1obj);
    _cut->Tool.setValue(_box6obj);

    // Act
    _cut->execute();
    Part::TopoShape ts = _cut->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double target = 6 - Base::Precision::Confusion() * 3 * 1000;
    EXPECT_FLOAT_EQ(volume,target);
}

TEST_F(FeaturePartCutTest, testMustExecute)
{
    // Act
    short mE = _cut->mustExecute();
    // Assert
    EXPECT_FALSE(mE);
    _cut->Base.setValue(_box1obj);
    // Assert
    mE = _cut->mustExecute();
    EXPECT_FALSE(mE);
    // Act
    _cut->Tool.setValue(_box2obj);
    // Assert
    mE = _cut->mustExecute();
    EXPECT_TRUE(mE);
    _doc->recompute();
    mE = _cut->mustExecute();
    EXPECT_FALSE(mE);
}

TEST_F(FeaturePartCutTest, testGetProviderName)
{
    // Act
    _cut->execute();
    const char* name = _cut->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}
