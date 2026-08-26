// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Mod/CadX/App/NativePrimitiveOperations.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace
{

TEST(CadXNativePrimitiveParser, ParsesCenteredBoxAndDefaultsIdentityRotation)
{
    const auto result = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"box",
        "operation_id":"box-1",
        "expected_graph_revision":"",
        "label":"Plate",
        "center_mm":{"x":10,"y":20,"z":30},
        "length_mm":10,
        "width_mm":20,
        "height_mm":30
    })json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.request.operation, "box");
    EXPECT_DOUBLE_EQ(result.request.rotation.axis.x, 0.0);
    EXPECT_DOUBLE_EQ(result.request.rotation.axis.y, 0.0);
    EXPECT_DOUBLE_EQ(result.request.rotation.axis.z, 1.0);
    EXPECT_DOUBLE_EQ(result.request.rotation.angleDegrees, 0.0);

    const auto origin = CadX::NativePrimitiveOperations::expectedOrigin(result.request);
    EXPECT_DOUBLE_EQ(origin.x, 5.0);
    EXPECT_DOUBLE_EQ(origin.y, 10.0);
    EXPECT_DOUBLE_EQ(origin.z, 15.0);
}

TEST(CadXNativePrimitiveParser, NormalizesCylinderAxisAndDefaultsFullSweep)
{
    const auto result = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"cylinder",
        "operation_id":"cylinder-1",
        "expected_graph_revision":"sha256:base",
        "label":"Pin",
        "center_mm":{"x":0,"y":0,"z":10},
        "rotation":{"axis":{"x":0,"y":0,"z":2},"angle_degrees":90},
        "radius_mm":5,
        "height_mm":20
    })json");

    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result.request.sweepDegrees, 360.0);
    EXPECT_DOUBLE_EQ(result.request.rotation.axis.z, 1.0);
    const auto origin = CadX::NativePrimitiveOperations::expectedOrigin(result.request);
    EXPECT_NEAR(origin.x, 0.0, 1.0e-12);
    EXPECT_NEAR(origin.y, 0.0, 1.0e-12);
    EXPECT_NEAR(origin.z, 0.0, 1.0e-12);
}

TEST(CadXNativePrimitiveParser, RejectsUnknownRootAndNestedFields)
{
    const auto unknownRoot = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"box", "operation_id":"x", "expected_graph_revision":"",
        "label":"x", "center_mm":{"x":0,"y":0,"z":0},
        "length_mm":1, "width_mm":1, "height_mm":1, "unexpected":true
    })json");
    EXPECT_FALSE(unknownRoot);
    EXPECT_NE(unknownRoot.diagnostic.find("unknown field"), std::string::npos);

    const auto unknownNested = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"box", "operation_id":"x", "expected_graph_revision":"",
        "label":"x", "center_mm":{"x":0,"y":0,"z":0,"w":0},
        "length_mm":1, "width_mm":1, "height_mm":1
    })json");
    EXPECT_FALSE(unknownNested);
    EXPECT_NE(unknownNested.diagnostic.find("unknown field"), std::string::npos);
}

TEST(CadXNativePrimitiveParser, RejectsInvalidBoundsAndNonFinitePreflightValues)
{
    const auto zeroDimension = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"box", "operation_id":"x", "expected_graph_revision":"",
        "label":"x", "center_mm":{"x":0,"y":0,"z":0},
        "length_mm":0, "width_mm":1, "height_mm":1
    })json");
    EXPECT_FALSE(zeroDimension);

    const auto outOfRangeCenter = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"cylinder", "operation_id":"x", "expected_graph_revision":"",
        "label":"x", "center_mm":{"x":1000001,"y":0,"z":0},
        "radius_mm":1, "height_mm":1
    })json");
    EXPECT_FALSE(outOfRangeCenter);

    CadX::PrimitiveRequest direct;
    direct.operation = "box";
    direct.operationId = "x";
    direct.label = "x";
    direct.center = {0.0, 0.0, 0.0};
    direct.rotation.axis = {std::numeric_limits<double>::infinity(), 0.0, 0.0};
    direct.lengthMm = direct.widthMm = direct.heightMm = 1.0;
    EXPECT_FALSE(CadX::NativePrimitiveOperations::preflight(direct));
}

TEST(CadXNativePrimitiveParser, EnforcesLabelAndRequiredFields)
{
    const auto missingLabel = CadX::NativePrimitiveOperations::parseRequest(R"json({
        "operation":"box", "operation_id":"x", "expected_graph_revision":"",
        "center_mm":{"x":0,"y":0,"z":0},
        "length_mm":1, "width_mm":1, "height_mm":1
    })json");
    EXPECT_FALSE(missingLabel);

    std::string longLabel(161, 'a');
    const auto tooLong = CadX::NativePrimitiveOperations::parseRequest(
        std::string(R"json({"operation":"box","operation_id":"x","expected_graph_revision":"","label":")json")
        + longLabel
        + R"json(","center_mm":{"x":0,"y":0,"z":0},"length_mm":1,"width_mm":1,"height_mm":1})json");
    EXPECT_FALSE(tooLong);
}

}  // namespace
