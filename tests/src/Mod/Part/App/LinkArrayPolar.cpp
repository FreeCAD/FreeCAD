// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <gp_Dir.hxx>
#include <Precision.hxx>

#include <Mod/Part/App/LinkArrayPolar.h>
#include <src/App/InitApplication.h>

class LinkArrayPolarTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Part::PolarPatternExtension::init();
        Part::LinkArray::init();
        Part::LinkArrayPolar::init();
    }
};

TEST_F(LinkArrayPolarTest, objectAxes)
{
    Part::LinkArrayPolar array;

    const auto checkAxis = [&array](const char* role, const gp_Dir& expected) {
        array.Axis.setValue(nullptr, std::vector<std::string> {role});
        EXPECT_TRUE(array.getRotation().Direction().IsEqual(expected, Precision::Angular()));
    };

    checkAxis("X_Axis", gp_Dir(1.0, 0.0, 0.0));
    checkAxis("Y_Axis", gp_Dir(0.0, 1.0, 0.0));
    checkAxis("Z_Axis", gp_Dir(0.0, 0.0, 1.0));
}
