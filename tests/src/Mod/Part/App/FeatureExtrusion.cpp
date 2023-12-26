// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeatureExtrusion.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeatureExtrusionTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
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
        _extrusion->Base.setValue(_boxes[0]);
        _extrusion->LengthFwd.setValue(10);
    }

    void TearDown() override
    {}

    Part::Extrusion* _extrusion;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureExtrusionTest, testExecute)
{

    // Need to cover the permutations of these values:
    // App::PropertyVector Dir;
    // App::PropertyEnumeration DirMode;
    // App::PropertyLinkSub DirLink;
    // App::PropertyDistance LengthFwd;
    // App::PropertyDistance LengthRev;
    // App::PropertyBool Solid;
    // App::PropertyBool Reversed;
    // App::PropertyBool Symmetric;
    // App::PropertyAngle TaperAngle;
    // App::PropertyAngle TaperAngleRev;
    // App::PropertyString FaceMakerClass;
}

TEST_F(FeatureExtrusionTest, testMustExecute)
{
    // // Assert
    // EXPECT_TRUE(_fillet->mustExecute());
    // // Act
    // _fillet->Base.setValue(_boxes[0]);
    // // Assert
    // EXPECT_TRUE(_fillet->mustExecute());
    // // Act
    // _fillet->Edges.setValues(PartTestHelpers::_getFilletEdges({1}, 0.5, 0.5));
    // // Assert
    // EXPECT_TRUE(_fillet->mustExecute());
    // // Act
    // _doc->recompute();
    // // Assert
    // EXPECT_FALSE(_fillet->mustExecute());
}

TEST_F(FeatureExtrusionTest, testGetProviderName)
{
    // Act
    _extrusion->execute();
    const char* name = _extrusion->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderExtrusion");
}

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
