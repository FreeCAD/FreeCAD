// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

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
        _boxes[0]->Length.setValue(4);
        _boxes[0]->Width.setValue(5);
        _boxes[0]->Height.setValue(6);
        _boxes[0]->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, 6), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Length.setValue(1);
        _boxes[1]->Width.setValue(2);
        _boxes[1]->Height.setValue(3);
        _fused = dynamic_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
        _fused->Base.setValue(_boxes[0]);
        _fused->Tool.setValue(_boxes[1]);
        _fused->execute();
        _chamfer = dynamic_cast<Part::Chamfer*>(_doc->addObject("Part::Chamfer"));
    }

    void TearDown() override
    {}

    Part::Fuse* _fused;  // NOLINT Can't be private in a test framework
    Part::Chamfer* _chamfer;  // NOLINT Can't be private in a test framework

    std::vector<Part::FilletElement>
    _getFilletEdges(std::vector<int> edges, double startRadius, double endRadius)
    {
        std::vector<Part::FilletElement> filletElements;
        for (auto e : edges) {
            Part::FilletElement fe = {e, startRadius, endRadius};
            filletElements.push_back(fe);
        }
        return filletElements;
    }
};

// Unfortunately for these next two tests, there are upstream errors in OCCT
// at least until 7.5.2 that cause some chamfers that intersect each other to
// fail.  Until that's fixed, test subsets of the complete chamfer list.

TEST_F(FeatureChamferTest, testOther)
{
    // Arrange
    _chamfer->Base.setValue(_fused);
    Part::TopoShape ts = _fused->Shape.getValue();
    unsigned long sec = ts.countSubElements("Edge");
    double fusedVolume, chamferVolume;
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
    _chamfer->Edges.setValues(_getFilletEdges({1, 2}, 0.5, 0.5));
    fusedVolume = PartTestHelpers::getVolume(_fused->Shape.getValue());
    chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    // Assert
    EXPECT_DOUBLE_EQ(fusedVolume, 126.0);
    EXPECT_DOUBLE_EQ(chamferVolume, 0.0);
    // Act
    _chamfer->execute();
    chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    // Assert
    EXPECT_FLOAT_EQ(chamferVolume, 124.79166);
}

TEST_F(FeatureChamferTest, testMost)
{
    // Arrange
    _fused->Refine.setValue(true);
    _fused->execute();
    _chamfer->Base.setValue(_fused);
    _chamfer->Edges.setValues(_getFilletEdges({ 3, 4, 5, 6, 7, 8, 9, 
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}, 0.4, 0.4) );
    // Act
    _chamfer->execute();
    double chamferVolume = PartTestHelpers::getVolume(_chamfer->Shape.getValue());
    // Assert
    EXPECT_FLOAT_EQ(chamferVolume, 121.46667);
}

// Hmmmm...  FeaturePartCommon with insufficent parameters says MustExecute false,
// but FeatureChamfer says MustExecute true ...  Neither of these should really
// happen, though.

TEST_F(FeatureChamferTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_chamfer->mustExecute());
    // Act
    _chamfer->Base.setValue(_boxes[0]);
    // Assert
    EXPECT_TRUE(_chamfer->mustExecute());
    // Act
    _chamfer->Edges.setValues(_getFilletEdges({1}, 0.5, 0.5) );
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
