// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartFuse.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"


class FeaturePartFuseTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestFile();
        _fuse = static_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
    }

    void TearDown() override
    {}

    Part::Fuse* _fuse;
};

TEST_F(FeaturePartFuseTest, testIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box2obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 9.0);
}

TEST_F(FeaturePartFuseTest, testNonIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box3obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 12.0);
}

TEST_F(FeaturePartFuseTest, testTouching)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box4obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 12.0);
}

TEST_F(FeaturePartFuseTest, testAlmostTouching)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box5obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_FLOAT_EQ(volume, 12.0);
}

TEST_F(FeaturePartFuseTest, testBarelyIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box6obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double target = 12 - Base::Precision::Confusion() * 3 * 1000;
    EXPECT_FLOAT_EQ(volume, target);
}

TEST_F(FeaturePartFuseTest, testMustExecute)
{
    // Act
    short mE = _fuse->mustExecute();
    // Assert
    EXPECT_FALSE(mE);
    _fuse->Base.setValue(_box1obj);
    // Assert
    mE = _fuse->mustExecute();
    EXPECT_FALSE(mE);
    // Act
    _fuse->Tool.setValue(_box2obj);
    // Assert
    mE = _fuse->mustExecute();
    EXPECT_TRUE(mE);
    _doc->recompute();
    mE = _fuse->mustExecute();
    EXPECT_FALSE(mE);
}

TEST_F(FeaturePartFuseTest, testGetProviderName)
{
    // Act
    _fuse->execute();
    const char* name = _fuse->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

TEST_F(FeaturePartFuseTest, testRefine)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box2obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    // what's the wire or face count here?
    std::vector<Part::TopoShape> subs = ts.getSubTopoShapes(TopAbs_FACE);  // WIRE
    EXPECT_EQ(subs.size(), 14);
    // Assert
    _fuse->Refine.setValue(true);
    _fuse->execute();
    ts = _fuse->Shape.getValue();
    subs = ts.getSubTopoShapes(TopAbs_FACE);  // WIRE
    EXPECT_EQ(subs.size(), 6);
}
