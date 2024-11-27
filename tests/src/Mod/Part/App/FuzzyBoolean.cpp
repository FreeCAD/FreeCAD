// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Part/App/FuzzyHelper.h"
#include "Mod/Part/App/FeaturePartImportBrep.h"
#include <src/App/InitApplication.h>
#include <BRepBuilderAPI_Copy.hxx>
#include <BOPAlgo_ArgumentAnalyzer.hxx>
#include "PartTestHelpers.h"


class FuzzyBooleanTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        std::string testPath = App::Application::getHomePath() + "/tests/brepfiles/";
        _fuse = dynamic_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
        _cylinder1 = dynamic_cast<Part::ImportBrep*>(_doc->addObject("Part::ImportBrep"));
        _cylinder1->FileName.setValue(testPath + "cylinder1.brep");
        _helix1 = dynamic_cast<Part::ImportBrep*>(_doc->addObject("Part::ImportBrep"));
        _helix1->FileName.setValue(testPath + "helix1.brep");

        // Load
        _cylinder1->execute();
        _helix1->execute();

        // Arrange
        _fuse->Base.setValue(_cylinder1);
        _fuse->Tool.setValue(_helix1);
    }

    void TearDown() override
    {}

    Part::Fuse* _fuse = nullptr;             // NOLINT Can't be private in a test framework
    Part::ImportBrep* _cylinder1 = nullptr;  // NOLINT Can't be private in a test framework
    Part::ImportBrep* _helix1 = nullptr;     // NOLINT Can't be private in a test framework
};

TEST_F(FuzzyBooleanTest, testLoadedCorrectly)
{

    EXPECT_NEAR(PartTestHelpers::getVolume(_cylinder1->Shape.getValue()), 125.6, 1.0);
    EXPECT_NEAR(PartTestHelpers::getVolume(_helix1->Shape.getValue()), 33.32, 1.0);
}

TEST_F(FuzzyBooleanTest, testDefaultFuzzy)
{

    // Act
    _fuse->execute();

    // Verify
    EXPECT_NEAR(PartTestHelpers::getVolume(_fuse->Shape.getValue()),
                PartTestHelpers::getVolume(_helix1->Shape.getValue())
                    + PartTestHelpers::getVolume(_cylinder1->Shape.getValue()),
                0.1);

    // Analyse
    Part::TopoShape ts = _fuse->Shape.getValue();
    ASSERT_FALSE(ts.isNull());
    TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(ts.getShape()).Shape();
    BOPAlgo_ArgumentAnalyzer BOPCheck;
    BOPCheck.SetShape1(BOPCopy);
    BOPCheck.SelfInterMode() = Standard_True;

    BOPCheck.Perform();
    // Assert
    EXPECT_FALSE(BOPCheck.HasFaulty());
}

TEST_F(FuzzyBooleanTest, testGoodFuzzy)
{

    // Act
    double oldFuzzy = Part::FuzzyHelper::getBooleanFuzzy();
    Part::FuzzyHelper::setBooleanFuzzy(10.0);
    _fuse->execute();
    EXPECT_FLOAT_EQ(Part::FuzzyHelper::getBooleanFuzzy(), 10.0);
    Part::FuzzyHelper::setBooleanFuzzy(oldFuzzy);

    // Verify
    EXPECT_NEAR(PartTestHelpers::getVolume(_fuse->Shape.getValue()),
                PartTestHelpers::getVolume(_helix1->Shape.getValue())
                    + PartTestHelpers::getVolume(_cylinder1->Shape.getValue()),
                0.1);

    // Analyse
    Part::TopoShape ts = _fuse->Shape.getValue();
    ASSERT_FALSE(ts.isNull());
    TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(ts.getShape()).Shape();
    BOPAlgo_ArgumentAnalyzer BOPCheck;
    BOPCheck.SetShape1(BOPCopy);
    BOPCheck.SelfInterMode() = Standard_True;

    BOPCheck.Perform();
    // Assert
    int result = BOPCheck.HasFaulty();
    EXPECT_FALSE(result);
}

TEST_F(FuzzyBooleanTest, testFailsTooSmallFuzzy)
{

    // Act
    double oldFuzzy = Part::FuzzyHelper::getBooleanFuzzy();
    Part::FuzzyHelper::setBooleanFuzzy(0.01);
    _fuse->execute();
    EXPECT_FLOAT_EQ(Part::FuzzyHelper::getBooleanFuzzy(), 0.01);
    Part::FuzzyHelper::setBooleanFuzzy(oldFuzzy);

    // Verify
    // EXPECT_NEAR(PartTestHelpers::getVolume(_fuse->Shape.getValue()),PartTestHelpers::getVolume(_helix1->Shape.getValue())+PartTestHelpers::getVolume(_cylinder1->Shape.getValue()),0.1);
    // // fails on OCCT 7.3 - ignore

    // Analyse
    Part::TopoShape ts = _fuse->Shape.getValue();
    ASSERT_FALSE(ts.isNull());
    TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(ts.getShape()).Shape();
    BOPAlgo_ArgumentAnalyzer BOPCheck;
    BOPCheck.SetShape1(BOPCopy);
    BOPCheck.SelfInterMode() = Standard_True;

    BOPCheck.Perform();
    // Assert
    int result = BOPCheck.HasFaulty();
    EXPECT_TRUE(result);
}

TEST_F(FuzzyBooleanTest, testCompletelyFailsTooBigFuzzy)
{

    // Act
    double oldFuzzy = Part::FuzzyHelper::getBooleanFuzzy();
    Part::FuzzyHelper::setBooleanFuzzy(1e10);
    int failed = 0;
    Part::TopoShape ts;
    try {
        _fuse->execute();
        ts = _fuse->Shape.getValue();
        if (ts.isNull()) {
            failed = 1;
        }
        else {
            TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(ts.getShape()).Shape();
            BOPAlgo_ArgumentAnalyzer BOPCheck;
            BOPCheck.SetShape1(BOPCopy);
            BOPCheck.SelfInterMode() = Standard_True;

            BOPCheck.Perform();
            // Assert
            if (BOPCheck.HasFaulty()) {
                failed = 1;
            }
        }
    }
    catch (...) {
        failed = 1;
    }
    Part::FuzzyHelper::setBooleanFuzzy(oldFuzzy);
    EXPECT_EQ(failed, 1);
}
