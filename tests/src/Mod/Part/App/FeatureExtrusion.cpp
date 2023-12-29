// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>

#include "gtest/gtest.h"

#include "Mod/Part/App/FeatureExtrusion.h"
#include <src/App/InitApplication.h>

#include "BRepBuilderAPI_MakeEdge.hxx"

#include "PartTestHelpers.h"

struct extrusionParms
{
    char* name;
    // Parms for each permutation:
    char dirMode;
    double lengthFwd;
    double lengthRev;
    bool solid;
    bool reversed;
    bool symmetric;
    double taperAngleFwd;
    double taperAngleRev;
    char* faceMakerClass;
    // Expected result values:
    double volume;
    Base::BoundBox3d box;
};

const double len = 3;
const double wid = 4;

class FeatureExtrusionTest: public ::testing::TestWithParam<extrusionParms>,
                            public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _extrusion = dynamic_cast<Part::Extrusion*>(_doc->addObject("Part::Extrusion"));
        PartTestHelpers::rectangle(len, wid, "Rect1");
        _extrusion->Base.setValue(_doc->getObjects().back());
        _extrusion->LengthFwd.setValue(ext1);
    }

    void TearDown() override
    {}

    Part::Extrusion* _extrusion;  // NOLINT Can't be private in a test framework
};


// TEST_P(FeatureExtrusionTest, testExecute)
// {
//     // Arrange
//     auto test = GetParam();
//     _extrusion->Dir.setValue(Base::Vector3d(0, 0, 1));
//     _extrusion->DirMode.setValue(test.dirMode);
//     BRepBuilderAPI_MakeEdge e1(gp_Pnt(0, 0, 0), gp_Pnt(10, 10, 10));
//     auto edge = static_cast<Part::Feature*>(_doc->addObject("Part::Feature", "Edge"));
//     edge->Shape.setValue(e1);
//     _extrusion->DirLink.setValue(edge);
//     _extrusion->LengthFwd.setValue(test.lengthFwd);
//     _extrusion->LengthRev.setValue(test.lengthRev);
//     _extrusion->Solid.setValue(test.solid);
//     _extrusion->Reversed.setValue(test.reversed);
//     _extrusion->Symmetric.setValue(test.symmetric);
//     _extrusion->TaperAngle.setValue(test.taperAngleFwd);
//     _extrusion->TaperAngleRev.setValue(test.taperAngleRev);
//     _extrusion->FaceMakerClass.setValue(test.faceMakerClass);
//     // Act
//     _extrusion->execute();
//     Part::TopoShape ts = _extrusion->Shape.getValue();
//     double volume = PartTestHelpers::getVolume(ts.getShape());
//     Base::BoundBox3d bb = ts.getBoundBox();
//     // Assert
//     // Opencascade volume calculations aren't precisely the same as ours.  Hmmm.
//     EXPECT_NEAR(volume, test.volume, 1.2) << "SubTest " << test.name;
//     EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, test.box)) << "SubTest " << test.name;
// }

TEST_F(FeatureExtrusionTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
    // Act
    _extrusion->Base.setValue(_extrusion->Base.getValue());
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
    // Act
    _extrusion->Solid.setValue(Standard_True);
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
}

TEST_F(FeatureExtrusionTest, testGetProviderName)
{
    // Act
    _extrusion->execute();
    const char* name = _extrusion->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderExtrusion");
}

// Not clear if there is test value in this one.

TEST_F(FeatureExtrusionTest, testFetchAxisLink)
{
    // static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
    //                           Base::Vector3d& basepoint,
    //                           Base::Vector3d& dir);
}

// Filling in these next two tests seems very redundant, since they are used in execute()
// and thus tested by the results there.  In the event that ever went funny, then maybe
// implementation here would make sense.

TEST_F(FeatureExtrusionTest, testExtrudeShape)
{
    // static TopoShape extrudeShape(const TopoShape& source, const ExtrusionParameters& params);
}

TEST_F(FeatureExtrusionTest, testComputeFinalParameters)
{
    // ExtrusionParameters computeFinalParameters();
}

// Arbtitrary constants for testing.  Named here for clarity.
const double ext1 = 10;
const double ext2 = 9;
const double ang = 30;

const double tangent = tan(ang / 180.0 * M_PI);

// Volume of a truncated trapezoidal rectangular pyramid (V”) =(1/3)[A’+A”+√(A’*A”)] X H’
const double a = len * wid;                                                 // Area of the base
const double aa = (len + ext1 * tangent * 2) * (wid + ext1 * tangent * 2);  // Area of the top
const double sym_aa =
    (len + ext1 * tangent * 2 / 2) * (wid + ext1 * tangent * 2 / 2);  // Area of the top
const double pyramidVol = ext1 * (a + aa + sqrt(a * aa)) / 3;
const double symPyramidVol = ext1 / 2 * (a + sym_aa + sqrt(a * sym_aa)) / 3;

const double sin45 = sin(45 / 180.0 * M_PI);

TEST_F(FeatureExtrusionTest, testExecuteSimple)
{
    // Arrange
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteSimpleRev)
{
    // Arrange
    _extrusion->LengthFwd.setValue(0);
    _extrusion->LengthRev.setValue(ext2);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext2);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext2, len, wid, 0)));
}

TEST_F(FeatureExtrusionTest, testExecuteSolid)
{
    // Arrange
    _extrusion->Solid.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteReverse)
{
    // Arrange
    _extrusion->Reversed.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext1, len, wid, 0)));
}

TEST_F(FeatureExtrusionTest, testExecuteSymmetric)
{
    // Arrange
    _extrusion->Symmetric.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext1 / 2, len, wid, ext1 / 2)));
}

TEST_F(FeatureExtrusionTest, testExecuteAngled)
{
    // Arrange
    _extrusion->Solid.setValue(true);
    _extrusion->TaperAngle.setValue(ang);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, pyramidVol);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-ext1 * tangent, -ext1 * tangent, 0,
                                    len + ext1 * tangent, wid + ext1 * tangent, ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteAngledRev)
{
    // Arrange
    _extrusion->Solid.setValue(true);
    _extrusion->TaperAngleRev.setValue(ang);
    // _extrusion->FaceMakerClass.setValue(test.faceMakerClass);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, symPyramidVol + len * wid * ext1 / 2);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-ext1 * tangent / 2, -ext1 * tangent / 2, -ext1 / 2,
                                    len + ext1 * tangent / 2, wid + ext1 * tangent / 2, ext1 / 2)));
}

TEST_F(FeatureExtrusionTest, testExecuteEdge)
{
    // Arrange
    BRepBuilderAPI_MakeEdge e1(gp_Pnt(0, 0, 0), gp_Pnt(10, 10, 10));
    auto edge = static_cast<Part::Feature*>(_doc->addObject("Part::Feature", "Edge"));
    edge->Shape.setValue(e1);
    _extrusion->DirLink.setValue(edge);
    _extrusion->DirMode.setValue(1);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1 * tangent);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(0, 0, 0, len + ext1 * tangent, wid + ext1 * tangent, ext1 * tangent)));
}

TEST_F(FeatureExtrusionTest, testExecuteDir)
{
    // Arrange
    _extrusion->Dir.setValue(Base::Vector3d(0, 1, 1));
    _extrusion->DirMode.setValue((long)0);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1 * sin45);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(0, 0, 0, len, wid + ext1 * sin45, ext1 * sin45)));
}

// clang-format off
// extrusionParms testslist[] = {
    // {"Simple Extrusion", 2, ext1, 0, 
    //                      false, false, false, 0, 0, "",
    //  /* Results: */ len* wid* ext1, Base::BoundBox3d(0, 0, 0, len, wid, ext1)},

    // {"Reverse Simple Extrusion", 2, 0, ext2,
    //                      false, false, false, 0, 0, "",
    //  /* Results: */ len* wid* ext2, Base::BoundBox3d(0, 0, -ext2, len, wid, 0)},

    // {"Solid Extrusion", 2, ext1, 0,
    //                      true, false, false, 0, 0, "",
    //  /* Results: */ len* wid* ext1, Base::BoundBox3d(0, 0, 0, len, wid, ext1)},

    // {"Reverse Flag Extrusion", 2, ext1, 0,
    //                      false, true, false, 0, 0, "",
    //  /* Results: */ len* wid* ext1, Base::BoundBox3d(0, 0, -ext1, len, wid, 0)},

    // {"Symmetric Extrusion", 2, ext1, 0,
    //                      false, false, true, 0, 0, "",
    //  /* Results: */ len* wid* ext1, Base::BoundBox3d(0, 0, -ext1 / 2, len, wid, ext1 / 2)},

    // Note:  Angled volumes appear to be wrong unless we have a solid.  Flag must be true
    // {"Angled Extrusion", 2, ext1, 0,
    //                      true, false, false, ang, 0, "",
    //  /* Results: */ pyramidVol, 
    //                 Base::BoundBox3d(-ext1 * tangent, -ext1 * tangent, 0,
    //                                 len + ext1 * tangent, wid + ext1 * tangent, ext1)},

    // {"Reverse Angled Extrusion", 2, ext1, 0,
    //                      true, false, true, 0, ang, "",
    //  /* Results: */ symPyramidVol + len * wid * ext1 / 2, 
    //                 Base::BoundBox3d(-ext1 * tangent / 2, -ext1 * tangent / 2, -ext1 / 2,
    //                                 len + ext1 * tangent / 2, wid + ext1 * tangent / 2, ext1 / 2)},

    // {"Simple Extrusion Vector1", 1, ext1, 0, 
    //                      false, false, false, 0, 0, "",
    //  /* Results: */ len* wid* ext1 * tangent, Base::BoundBox3d(0, 0, 0,
    //                                 len + ext1 * tangent, wid + ext1 * tangent, ext1 * tangent)},

    // Set the dir!
    // {"Simple Extrusion Vector0", 0, ext1, 0, 
    //                      false, false, false, 0, 0, "",
    //  /* Results: */ len* wid* ext1 * sin45, 
    //                 Base::BoundBox3d(0, 0, 0, len, wid + ext1 * sin45, ext1 * sin45)},

    // {"Simple Extrusion Cheese", 2, ext1, 0, 
    //                      false, false, false, 0, 0, "Part::FaceMakerCheese",
    //  /* Results: */ len* wid* ext1, Base::BoundBox3d(0, 0, 0, len, wid, ext1)},

// };
// clang-format on

// INSTANTIATE_TEST_SUITE_P(ExecuteTests, FeatureExtrusionTest, ::testing::ValuesIn(testslist));
