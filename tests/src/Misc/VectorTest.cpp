#include <stdio.h>
#include <vector>

#include "gtest/gtest.h"

class VectorTest: public testing::Test
{
public:
    std::vector<int> m_vector;

    virtual void SetUp()
    {
        m_vector.push_back(1);
        m_vector.push_back(2);
    }

    virtual void TearDown()
    {}
};

TEST_F(VectorTest, testElementZeroIsOne)
{
    EXPECT_EQ(1, m_vector[0]);
}

TEST_F(VectorTest, testElementOneIsTwo)
{
    EXPECT_EQ(2, m_vector[1]);
}
