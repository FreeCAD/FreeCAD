// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"
#include "Mod/Part/App/FeatureChamfer.h"

class FeatureChamferTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _boxes[0]->Length.setValue(length);
        _boxes[0]->Width.setValue(width);
        _boxes[0]->Height.setValue(height);
        _boxes[0]->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, height), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Length.setValue(1);
        _boxes[1]->Width.setValue(2);
        _boxes[1]->Height.setValue(3);
        _fused = _doc->addObject<Part::Fuse>();
        _fused->Base.setValue(_boxes[0]);
        _fused->Tool.setValue(_boxes[1]);
        _fused->execute();
        _chamfer = _doc->addObject<Part::Chamfer>();
    }

    void TearDown() override
    {}

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    const double length = 4.0;
    const double width = 5.0;
    const double height = 6.0;
    const double chamfer = 0.5;
    Part::Fuse* _fused = nullptr;
    Part::Chamfer* _chamfer = nullptr;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

// Unfortunately for these next two tests, there are upstream errors in OCCT
// at least until 7.5.2 that cause some chamfers that intersect each other to
// fail.  Until that's fixed, test subsets of the complete chamfer list.

TEST_F(FeatureChamferTest, testOther)
{
    const double baseVolume =
        _boxes[0]->Length.getValue() * _boxes[0]->Width.getValue() * _boxes[0]->Height.getValue()
        + _boxes[1]->Length.getValue() * _boxes[1]->Width.getValue() * _boxes[1]->Height.getValue();
    // Arrange
    _chamfer->Base.setValue(_fused);
    Part::TopoShape ts = _fused->Shape.getValue();
    unsigned long sec = ts.countSubElements("Edge");
    // Assert
    EXPECT_EQ(sec, 25);
    // Act
    _fused->Refine.setValue(true);
    _fused->execute();
    ts = _fused->Shape.getValue();
    sec = ts.countSubElements("Edge");
    // Assert
    EXPECT_EQ(sec, 24);
    // Act
    _chamfer->Edges.setValues(PartTestHelpers::_getFilletEdges({1, 2}, chamfer, chamfer));
    double fusedVolume = PartTestHelpers::getVolume(_fused->Shape.getValue());
    double chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    // Assert
    EXPECT_DOUBLE_EQ(fusedVolume, baseVolume);
    EXPECT_DOUBLE_EQ(chamferVolume, 0.0);
    // Act
    _chamfer->execute();
    chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    double cv = (_boxes[0]->Length.getValue()) * chamfer * chamfer / 2
        + (_boxes[0]->Height.getValue()) * chamfer * chamfer / 2 - chamfer * chamfer * chamfer / 3;
    // Assert
    EXPECT_FLOAT_EQ(chamferVolume, baseVolume - cv);
}

TEST_F(FeatureChamferTest, testMost)
{
    // Arrange
    _fused->Refine.setValue(true);
    _fused->execute();
    _chamfer->Base.setValue(_fused);
    _chamfer->Edges.setValues(PartTestHelpers::_getFilletEdges(
        {3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,  // NOLINT magic number
         15, 16, 17, 18, 19, 20, 21, 22, 23, 24},         // NOLINT magic number
        0.4,                                              // NOLINT magic number
        0.4));                                            // NOLINT magic number
    // Act
    _chamfer->execute();
    double chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    // Assert
    EXPECT_NEAR(chamferVolume, 121.46667, 1e-5);  // This is calculable, but painful.
}

// Worth noting that FeaturePartCommon with insufficient parameters says MustExecute false,
// but FeatureChamfer says MustExecute true.  Not a condition that should ever really be hit.

TEST_F(FeatureChamferTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_chamfer->mustExecute());
    // Act
    _chamfer->Base.setValue(_boxes[0]);
    // Assert
    EXPECT_TRUE(_chamfer->mustExecute());
    // Act
    _chamfer->Edges.setValues(PartTestHelpers::_getFilletEdges({1}, chamfer, chamfer));
    // Assert
    EXPECT_TRUE(_chamfer->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_chamfer->mustExecute());
}

TEST_F(FeatureChamferTest, testGetProviderName)
{
    // Act
    _chamfer->execute();
    const char* name = _chamfer->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderChamfer");
}
