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
        0.0, 0.0, 5.0, 5.0,  // pair 1: x1, y1, x2, y2
        1.0, 1.0, 6.0, 6.0,  // pair 2: x3, y3, x4, y4
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
