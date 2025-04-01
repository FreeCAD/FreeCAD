// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Part/App/FeatureRevolution.h"
#include <src/App/InitApplication.h>

#include "Base/Interpreter.h"

#include "BRepBuilderAPI_MakeEdge.hxx"

#include "TopoDS_Iterator.hxx"

#include "PartTestHelpers.h"

class FeatureRevolutionTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _revolution = _doc->addObject<Part::Revolution>();
        PartTestHelpers::rectangle(len, wid, "Rect1");
        _revolution->Source.setValue(_doc->getObjects().back());
        _revolution->Axis.setValue(0, 1, 0);
    }

    void TearDown() override
    {}

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    Part::Revolution* _revolution = nullptr;
    // Arbtitrary constants for testing.  Named here for clarity.
    const double len = 3;
    const double wid = 4;
    const double ext1 = 10;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

TEST_F(FeatureRevolutionTest, testExecute)
{
    // Arrange
    double puckVolume = len * len * std::numbers::pi * wid;  // Area is PIr2; apply height
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, puckVolume);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-len, 0, -len, len, wid, len)));
}

TEST_F(FeatureRevolutionTest, testExecuteBase)
{
    // Arrange
    double rad = len + 1.0;
    double rad2 = 1.0;
    double outerPuckVolume = rad * rad * std::numbers::pi * wid;    // Area is PIr2; apply height
    double innerPuckVolume = rad2 * rad2 * std::numbers::pi * wid;  // Area is PIr2; apply height
    _revolution->Base.setValue(Base::Vector3d(len + 1, 0, 0));
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, outerPuckVolume - innerPuckVolume);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -wid, wid * 2, wid, wid)));
}


TEST_F(FeatureRevolutionTest, testAxis)
{
    // Arrange
    double puckVolume = wid * wid * std::numbers::pi * len;  // Area is PIr2 times height
    _revolution->Axis.setValue(Base::Vector3d(1, 0, 0));
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, puckVolume);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, -wid, -wid, len, wid, wid)));
}

TEST_F(FeatureRevolutionTest, testAxisLink)
{
    // Arrange
    BRepBuilderAPI_MakeEdge e1(gp_Pnt(0, 0, 0), gp_Pnt(0, 0, ext1));
    auto edge = _doc->addObject<Part::Feature>("Edge");
    edge->Shape.setValue(e1);
    _revolution->AxisLink.setValue(edge);
    // double puckVolume = wid * wid * std::numbers::pi * len;  // Area is PIr2; apply height
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    double puckVolume = 0;  // Someday make this test use a more interesting edge angle
    EXPECT_FLOAT_EQ(volume, puckVolume);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(-ext1 / 2, -ext1 / 2, 0, ext1 / 2, ext1 / 2, 0)));
}

TEST_F(FeatureRevolutionTest, testSymmetric)
{
    // Arrange
    double puckVolume = len * len * std::numbers::pi * wid;  // Area is PIr2 times height
    _revolution->Symmetric.setValue(true);
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, puckVolume);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-len, 0, -len, len, wid, len)));
}

TEST_F(FeatureRevolutionTest, testAngle)
{
    // Arrange
    double puckVolume = len * len * std::numbers::pi * wid;  // Area is PIr2 times height
    _revolution->Angle.setValue(90);                         // NOLINT magic number
    // Act
    _revolution->execute();
    Part::TopoShape ts = _revolution->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, puckVolume / 4);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -len, len, wid, 0)));
}

TEST_F(FeatureRevolutionTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_revolution->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_revolution->mustExecute());
    // Act
    _revolution->Base.setValue(_revolution->Base.getValue());
    // Assert
    EXPECT_TRUE(_revolution->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_revolution->mustExecute());
    // Act
    _revolution->Solid.setValue(Standard_True);
    // Assert
    EXPECT_TRUE(_revolution->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_revolution->mustExecute());
}

// TEST_F(FeatureRevolutionTest, testOnChanged)
// {
//     // void onChanged(const App::Property* prop) override;
// }

TEST_F(FeatureRevolutionTest, testGetProviderName)
{
    // Act
    _revolution->execute();
    const char* name = _revolution->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderRevolution");
}

// Tested by execute above
// TEST_F(FeatureRevolutionTest, testFetchAxisLink)
// {
//     // static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
//     //                           Base::Vector3d &center,
//     //                           Base::Vector3d &dir,
//     //                           double &angle);
// }
