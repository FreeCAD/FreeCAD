// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeatureRevolution.h"
#include <src/App/InitApplication.h>

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
        _revolution = dynamic_cast<Part::Revolution*>(_doc->addObject("Part::Revolution"));
    }

    void TearDown() override
    {}

    Part::Revolution* _revolution;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureRevolutionTest, testExecute)
{
    // Test perumtations of these settings:

    // App::PropertyLink Source;
    // App::PropertyVector Base;
    // App::PropertyVector Axis;
    // App::PropertyLinkSub AxisLink;
    // App::PropertyFloatConstraint Angle;
    // App::PropertyBool Symmetric; //like "Midplane" in PartDesign
    // App::PropertyBool Solid;
    // App::PropertyString FaceMakerClass;
}

TEST_F(FeatureRevolutionTest, testMustExecute)
{}

TEST_F(FeatureRevolutionTest, testOnChanged)
{
    // void onChanged(const App::Property* prop) override;
}

TEST_F(FeatureRevolutionTest, testGetProviderName)
{
    // Act
    _revolution->execute();
    const char* name = _revolution->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderRevolution");
}

TEST_F(FeatureRevolutionTest, testFetchAxisLink)
{
    // /**
    //  * @brief fetchAxisLink: read AxisLink to obtain the axis parameters and
    //  * angle span. Note: this routine is re-used in Revolve dialog, hence it
    //  * is static.
    //  * @param axisLink (input): the link
    //  * @param center (output): base point of axis
    //  * @param dir (output): direction of axis
    //  * @param angle (output): if edge is an arc of circle, this argument is
    //  * used to return the angle span of the arc.
    //  * @return true if link was fetched. false if link was empty. Throws if the
    //  * link is wrong.
    //  */
    // static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
    //                           Base::Vector3d &center,
    //                           Base::Vector3d &dir,
    //                           double &angle);
}
