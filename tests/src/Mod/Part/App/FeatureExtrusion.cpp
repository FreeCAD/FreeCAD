// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
// #include <math>

#include "gtest/gtest.h"

#include "Mod/Part/App/FeatureExtrusion.h"
#include <src/App/InitApplication.h>
#include "PartTestHelpers.h"

struct extrusionParms
{
    char* name;
    // Parms for each permutation:
    Base::Vector3d dir;
    char dirMode;
    App::DocumentObject* dirLink;
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
    }

    void TearDown() override
    {}

    Part::Extrusion* _extrusion;  // NOLINT Can't be private in a test framework
};


TEST_P(FeatureExtrusionTest, testExecute)
{
    // Arrange
    auto test = GetParam();
    _extrusion->Dir.setValue(test.dir);
    _extrusion->DirMode.setValue(test.dirMode);
    _extrusion->DirLink.setValue(test.dirLink);
    _extrusion->LengthFwd.setValue(test.lengthFwd);
    _extrusion->LengthRev.setValue(test.lengthRev);
    _extrusion->Solid.setValue(test.solid);
    _extrusion->Reversed.setValue(test.reversed);
    _extrusion->Symmetric.setValue(test.symmetric);
    _extrusion->TaperAngle.setValue(test.taperAngleFwd);
    _extrusion->TaperAngleRev.setValue(test.taperAngleRev);
    _extrusion->FaceMakerClass.setValue(test.faceMakerClass);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    // Opencascade volume calculations aren't precisely the same as ours.  Hmmm.
    EXPECT_NEAR(volume, test.volume, 1.2) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MinX, test.box.MinX) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MinY, test.box.MinY) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MinZ, test.box.MinZ) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MaxX, test.box.MaxX) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MaxY, test.box.MaxY) << "SubTest " << test.name;
    EXPECT_FLOAT_EQ(bb.MaxZ, test.box.MaxZ) << "SubTest " << test.name;
}

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
    // /**
    //  * @brief fetchAxisLink: read AxisLink to obtain the direction and
    //  * length. Note: this routine is re-used in Extrude dialog, hence it
    //  * is static.
    //  * @param axisLink (input): the link
    //  * @param basepoint (output): starting point of edge. Not used by extrude as of now.
    //  * @param dir (output): direction of axis, with magnitude (length)
    //  * @return true if link was fetched. false if link was empty. Throws if the
    //  * link is wrong.
    //  */
    // static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
    //                           Base::Vector3d& basepoint,
    //                           Base::Vector3d& dir);
}

// Filling in these next two tests seems very redundant, since they are used in execute()
// and thus tested by the results there.  In the event that ever went funny, then maybe
// implementation here would make sense.

TEST_F(FeatureExtrusionTest, testExtrudeShape)
{
    // /**
    //  * @brief The ExtrusionParameters struct is supposed to be filled with final
    //  * extrusion parameters, after resolving links, applying mode logic,
    //  * reversing, etc., and be passed to extrudeShape.
    //  */
    // struct ExtrusionParameters {
    //     gp_Dir dir;
    //     double lengthFwd{0};
    //     double lengthRev{0};
    //     bool solid{false};
    //     double taperAngleFwd{0}; //in radians
    //     double taperAngleRev{0};
    //     std::string faceMakerClass;
    // };

    // /**
    //  * @brief extrudeShape powers the extrusion feature.
    //  * @param source: the shape to be extruded
    //  * @param params: extrusion parameters
    //  * @return result of extrusion
    //  */
    // static TopoShape extrudeShape(const TopoShape& source, const ExtrusionParameters& params);
}

TEST_F(FeatureExtrusionTest, testComputeFinalParameters)
{
    // /**
    //  * @brief computeFinalParameters: applies mode logic and fetches links, to
    //  * compute the actual parameters of extrusion. Dir property is updated in
    //  * the process, hence the function is non-const.
    //  */
    // ExtrusionParameters computeFinalParameters();

    // static Base::Vector3d calculateShapeNormal(const App::PropertyLink& shapeLink);
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

// Need to cover the (reasonable) permutations of these parms:
// App::PropertyVector Dir;
// App::PropertyEnumeration DirMode;    // Normal, Edge, Custom
// App::PropertyLinkSub DirLink;    // the Edge, presumably
// DONE App::PropertyDistance LengthFwd;
// DONE App::PropertyDistance LengthRev;
// DONE App::PropertyBool Solid;  // createsolid
// DONE App::PropertyBool Reversed;
// DONE App::PropertyBool Symmetric;
// DONE App::PropertyAngle TaperAngle;
// DONE App::PropertyAngle TaperAngleRev;
// App::PropertyString FaceMakerClass;

extrusionParms testslist[] = {
    {"Simple Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     false,
     false,
     false,
     0,
     0,
     "",
     /* Results: */ len* wid* ext1,
     Base::BoundBox3d(0, 0, 0, len, wid, ext1)},
    {"Reverse Simple Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     0,
     ext2,
     false,
     false,
     false,
     0,
     0,
     "",
     /* Results: */ len* wid* ext2,
     Base::BoundBox3d(0, 0, -ext2, len, wid, 0)},
    {"Solid Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     true,
     false,
     false,
     0,
     0,
     "",
     /* Results: */ len* wid* ext1,
     Base::BoundBox3d(0, 0, 0, len, wid, ext1)},
    {"Reverse Flag Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     false,
     true,
     false,
     0,
     0,
     "",
     /* Results: */ len* wid* ext1,
     Base::BoundBox3d(0, 0, -ext1, len, wid, 0)},
    {"Symmetric Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     false,
     false,
     true,
     0,
     0,
     "",
     /* Results: */ len* wid* ext1,
     Base::BoundBox3d(0, 0, -ext1 / 2, len, wid, ext1 / 2)},
    // Note:  Angled volumes appear to be wrong unless we have a solid.  Flag must be true
    {"Angled Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     true,
     false,
     false,
     ang,
     0,
     "",
     /* Results: */ pyramidVol,
     Base::BoundBox3d(-ext1* tangent,
                      -ext1* tangent,
                      0,
                      len + ext1 * tangent,
                      wid + ext1 * tangent,
                      ext1)},
    {"Reverse Angled Extrusion",
     Base::Vector3d(1, 0, 0),
     2,
     nullptr,
     ext1,
     0,
     true,
     false,
     true,
     0,
     ang,
     "",
     /* Results: */ symPyramidVol + len* wid* ext1 / 2,
     Base::BoundBox3d(-ext1* tangent / 2,
                      -ext1* tangent / 2,
                      -ext1 / 2,
                      len + ext1 * tangent / 2,
                      wid + ext1 * tangent / 2,
                      ext1 / 2)},
};

INSTANTIATE_TEST_SUITE_P(ExecuteTests, FeatureExtrusionTest, ::testing::ValuesIn(testslist));

