// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "Mod/Sketcher/App/planegcs/GCS.h"

class SystemTest: public GCS::System
{
public:
    size_t getNumberOfConstraints(int tagID = -1)
    {
        return _getNumberOfConstraints(tagID);
    }

    void setStats(GCS::SystemStats* stats)
    {
        return _setStats(stats);
    }
};

class GCSTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _system = std::make_unique<SystemTest>();
    }

    void TearDown() override
    {
        _system.reset();
    }

    SystemTest* System()
    {
        return _system.get();
    }

private:
    std::unique_ptr<SystemTest> _system;
};

TEST_F(GCSTest, clearConstraints)  // NOLINT
{
    // Arrange
    const size_t numConstraints {100};
    for (size_t i = 0; i < numConstraints; ++i) {
        System()->addConstraint(new GCS::Constraint());
    }
    ASSERT_EQ(numConstraints, System()->getNumberOfConstraints());

    // Act
    System()->clear();

    // Assert
    EXPECT_EQ(0, System()->getNumberOfConstraints());
}

TEST_F(GCSTest, diagnoseRedundancyAcrossComponentsPrefersOlderConstraint)  // NOLINT
{
    // Arrange
    std::vector<double> values = {0.0, 0.0, 5.0, 5.0};  // x1, y1, x2, y2
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point p2(params[2], params[3]);

    constexpr int vertical_tag = 1;
    constexpr int coincident_tag = 2;
    System()->addConstraintVertical(p1, p2, vertical_tag);
    System()->addConstraintP2PCoincident(p1, p2, coincident_tag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    ASSERT_EQ(redundant.size(), 1u);
    EXPECT_EQ(redundant[0], vertical_tag);
}

TEST_F(GCSTest, diagnoseRedundancyAcrossComponentsIndependentOfAdditionOrder)  // NOLINT
{
    // Arrange
    std::vector<double> values = {0.0, 0.0, 5.0, 5.0};  // x1, y1, x2, y2
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point p2(params[2], params[3]);

    constexpr int coincident_tag = 1;
    constexpr int vertical_tag = 2;
    System()->addConstraintP2PCoincident(p1, p2, coincident_tag);
    System()->addConstraintVertical(p1, p2, vertical_tag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    ASSERT_EQ(redundant.size(), 1u);
    EXPECT_EQ(redundant[0], vertical_tag);
}

TEST_F(GCSTest, diagnoseAccumulatesRedundantTagsAcrossUnrelatedComponents)  // NOLINT
{
    // Arrange
    std::vector<double> values = {
        0.0,
        0.0,
        5.0,
        5.0,  // pair 1: x1, y1, x2, y2
        1.0,
        1.0,
        6.0,
        6.0,  // pair 2: x3, y3, x4, y4
    };
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point p2(params[2], params[3]);
    GCS::Point p3(params[4], params[5]);
    GCS::Point p4(params[6], params[7]);

    constexpr int vertical_tag_1 = 1;
    constexpr int coincident_tag_1 = 2;
    constexpr int vertical_tag_2 = 3;
    constexpr int coincident_tag_2 = 4;
    System()->addConstraintVertical(p1, p2, vertical_tag_1);
    System()->addConstraintP2PCoincident(p1, p2, coincident_tag_1);
    System()->addConstraintVertical(p3, p4, vertical_tag_2);
    System()->addConstraintP2PCoincident(p3, p4, coincident_tag_2);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    std::sort(redundant.begin(), redundant.end());
    EXPECT_EQ(redundant, (GCS::VEC_I {vertical_tag_1, vertical_tag_2}));
}

TEST_F(GCSTest, diagnoseResetsDependentParamsEachCall)  // NOLINT
{
    // Arrange
    std::vector<double> values = {0.0, 5.0, 10.0};
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);
    System()->addConstraintEqual(params[0], params[1], 1);
    System()->diagnose();

    // Act
    System()->addConstraintEqual(params[1], params[2], 2);
    System()->diagnose();

    // Assert
    GCS::VEC_pD dependent;
    System()->getDependentParams(dependent);
    EXPECT_EQ(dependent.size(), 3u);
}

TEST_F(GCSTest, diagnoseReportsFreeParamsOfUnconstrainedComponent)  // NOLINT
{
    // Arrange
    double fixedX = 0.0, fixedY = 0.0;
    std::vector<double> values = {
        0.0,
        0.0,  // p1
        1.0,
        1.0,  // p2
    };
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point fixed(&fixedX, &fixedY);
    System()->addConstraintP2PCoincident(p1, fixed, 1);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_pD dependent;
    System()->getDependentParams(dependent);
    ASSERT_EQ(dependent.size(), 2u);
    EXPECT_NE(std::ranges::find(dependent, params[2]), dependent.end());
    EXPECT_NE(std::ranges::find(dependent, params[3]), dependent.end());
}

TEST_F(GCSTest, diagnoseFlagsConflictingConstraintEntirelyOnExternalGeometry)  // NOLINT
{
    // Arrange
    double extX1 = 0.0, extY1 = 0.0, extX2 = 3.0, extY2 = 0.0;
    double wrongDistance = 10.0;
    std::vector<double> values = {1.0, 1.0};  // p1
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point pExt1(&extX1, &extY1);
    GCS::Point pExt2(&extX2, &extY2);

    constexpr int conflictTag = 1;
    System()->addConstraintP2PDistance(pExt1, pExt2, &wrongDistance, conflictTag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I conflicting;
    System()->getConflicting(conflicting);
    ASSERT_EQ(conflicting.size(), 1u);
    EXPECT_EQ(conflicting[0], conflictTag);
}

TEST_F(GCSTest, diagnoseTreatsSatisfiedConstraintOnExternalGeometryAsRedundant)  // NOLINT
{
    // Arrange
    double extX1 = 0.0, extY1 = 0.0, extX2 = 3.0, extY2 = 0.0;
    double correctDistance = 3.0;
    std::vector<double> values = {1.0, 1.0};  // p1
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point pExt1(&extX1, &extY1);
    GCS::Point pExt2(&extX2, &extY2);

    constexpr int redundantTag = 1;
    System()->addConstraintP2PDistance(pExt1, pExt2, &correctDistance, redundantTag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    ASSERT_EQ(redundant.size(), 1u);
    EXPECT_EQ(redundant[0], redundantTag);
}

TEST_F(GCSTest, diagnoseAccumulatesAcrossMixedConstrainedComponents)  // NOLINT
{
    // Arrange
    double constX1 = 0.0, constX2 = 5.0, constY = 0.0, zero = 0.0;
    std::vector<double> values = {
        0.0,
        0.0,  // p1
        1.0,
        1.0,  // p2
        2.0,
        1.0,  // p3
    };
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point p2(params[2], params[3]);
    GCS::Point p3(params[4], params[5]);

    constexpr int conflictingTag1 = 1;
    constexpr int conflictingTag2 = 2;
    constexpr int anchorYTag = 3;
    System()->addConstraintCoordinateX(p1, &constX1, conflictingTag1);
    System()->addConstraintCoordinateX(p1, &constX2, conflictingTag2);
    System()->addConstraintCoordinateY(p1, &constY, anchorYTag);

    constexpr int horizontalTag = 4;
    constexpr int redundantTag = 5;
    System()->addConstraintHorizontal(p2, p3, horizontalTag);
    System()->addConstraintDifference(p2.y, p3.y, &zero, redundantTag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    ASSERT_EQ(redundant.size(), 1u);
    EXPECT_EQ(redundant[0], redundantTag);
    EXPECT_EQ(System()->dofsNumber(), 3);

    GCS::VEC_I conflicting;
    System()->getConflicting(conflicting);
    std::sort(conflicting.begin(), conflicting.end());
    EXPECT_EQ(conflicting, (GCS::VEC_I {conflictingTag1, conflictingTag2}));
}

TEST_F(GCSTest, diagnoseCoincidentBetweenAlreadyCoincidentExternalPointsIsFullyRedundant)  // NOLINT
{
    // Arrange
    double extX1 = 3.0, extY1 = 4.0, extX2 = 3.0, extY2 = 4.0;  // already coincident
    std::vector<double> values = {1.0, 1.0};  // p1, unrelated unknown so hasUnknowns is true
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point pExt1(&extX1, &extY1);
    GCS::Point pExt2(&extX2, &extY2);

    constexpr int coincidentTag = 1;
    System()->addConstraintP2PCoincident(pExt1, pExt2, coincidentTag);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    EXPECT_EQ(redundant, (GCS::VEC_I {coincidentTag}));

    GCS::VEC_I partiallyRedundant;
    System()->getPartiallyRedundant(partiallyRedundant);
    EXPECT_TRUE(partiallyRedundant.empty());

    GCS::VEC_I conflicting;
    System()->getConflicting(conflicting);
    EXPECT_TRUE(conflicting.empty());
}

TEST_F(GCSTest, diagnoseDuplicateCoincidentOnUnlinkedPointsIsFullyRedundant)  // NOLINT
{
    // Arrange
    std::vector<double> values = {0.0, 0.0, 5.0, 5.0};  // x1, y1, x2, y2
    GCS::VEC_pD params;
    for (auto& v : values) {
        params.push_back(&v);
    }
    System()->declareUnknowns(params);

    GCS::Point p1(params[0], params[1]);
    GCS::Point p2(params[2], params[3]);

    constexpr int coincidentTag1 = 1;
    constexpr int coincidentTag2 = 2;
    System()->addConstraintP2PCoincident(p1, p2, coincidentTag1);
    System()->addConstraintP2PCoincident(p1, p2, coincidentTag2);

    // Act
    System()->diagnose();

    // Assert
    GCS::VEC_I redundant;
    System()->getRedundant(redundant);
    EXPECT_EQ(redundant, (GCS::VEC_I {coincidentTag2}));

    GCS::VEC_I partiallyRedundant;
    System()->getPartiallyRedundant(partiallyRedundant);
    EXPECT_TRUE(partiallyRedundant.empty());
}

TEST_F(GCSTest, diagnoseManyIndependentComponentsPerf)  // NOLINT
{
    // Arrange
    constexpr int N = 6000;
    std::vector<double> values(2 * N, 0.0);
    GCS::VEC_pD params;
    params.reserve(values.size());
    for (auto& v : values) {
        params.push_back(&v);
    }

    System()->declareUnknowns(params);
    for (int i = 0; i < N; ++i) {
        System()->addConstraintEqual(params[2 * i], params[2 * i + 1]);
    }

    // Act
    GCS::SystemStats stats;
    System()->setStats(&stats);
    System()->diagnose();

    // Assert
    EXPECT_EQ(stats.cumulativeDiagnoseMatrixSize, 2 * N);
}
