// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <boost/core/ignore_unused.hpp>
#include "Mod/Part/App/Geometry.h"
#include <src/App/InitApplication.h>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include "PartTestHelpers.h"
#include "App/MappedElement.h"

// using namespace Part;
// using namespace PartTestHelpers;

class GeometryTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
    }

    void TearDown() override
    {}
};

TEST_F(GeometryTest, testTrimBSpline)
{
    // Arrange
    // create arbitrary B-splines periodic and non-periodic, with arbitrary knots
    // NOTE: Avoid B-spline with typical knots like those ranging from [0,1] or [-1,1]
    int degree = 3;
    std::vector<Base::Vector3d> poles;
    poles.emplace_back(1, 0, 0);
    poles.emplace_back(1, 1, 0);
    poles.emplace_back(1, 0.5, 0);
    poles.emplace_back(0, 1, 0);
    poles.emplace_back(0, 0, 0);
    std::vector<double> weights(5, 1.0);
    std::vector<double> knotsNonPeriodic = {0.0, 1.0, 2.0};
    std::vector<int> multiplicitiesNonPeriodic = {degree + 1, 1, degree + 1};
    Part::GeomBSplineCurve nonPeriodicBSpline1(poles,
                                               weights,
                                               knotsNonPeriodic,
                                               multiplicitiesNonPeriodic,
                                               degree,
                                               false);
    Part::GeomBSplineCurve nonPeriodicBSpline2(poles,
                                               weights,
                                               knotsNonPeriodic,
                                               multiplicitiesNonPeriodic,
                                               degree,
                                               false);
    std::vector<double> knotsPeriodic = {0.0, 0.3, 1.0, 1.5, 1.8, 2.0};
    double period = knotsPeriodic.back() - knotsPeriodic.front();
    std::vector<int> multiplicitiesPeriodic(6, 1);
    Part::GeomBSplineCurve periodicBSpline1(poles,
                                            weights,
                                            knotsPeriodic,
                                            multiplicitiesPeriodic,
                                            degree,
                                            true);
    Part::GeomBSplineCurve periodicBSpline2(poles,
                                            weights,
                                            knotsPeriodic,
                                            multiplicitiesPeriodic,
                                            degree,
                                            true);
    // NOTE: These should be within the knot range, with param1 < param2
    double param1 = 0.5, param2 = 1.4;
    // TODO: Decide what to do if params are outside the range

    // Act
    periodicBSpline1.Trim(param1, param2);
    periodicBSpline2.Trim(param2, param1);
    nonPeriodicBSpline1.Trim(param1, param2);
    // TODO: What happens when a non-periodic B-spline is trimmed this way?
    nonPeriodicBSpline2.Trim(param2, param1);

    // Assert
    EXPECT_DOUBLE_EQ(periodicBSpline1.getFirstParameter(), param1);
    EXPECT_DOUBLE_EQ(periodicBSpline1.getLastParameter(), param2);
    EXPECT_DOUBLE_EQ(periodicBSpline2.getFirstParameter(), param2);
    EXPECT_DOUBLE_EQ(periodicBSpline2.getLastParameter(), param1 + period);
    EXPECT_DOUBLE_EQ(nonPeriodicBSpline1.getFirstParameter(), param1);
    EXPECT_DOUBLE_EQ(nonPeriodicBSpline1.getLastParameter(), param2);
}
