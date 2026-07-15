// SPDX-License-Identifier: LGPL-2.1-or-later

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
    EXPECT_EQ(stats.cumulativeDiagnoseMatrixSize, 2*N);
}
