// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"


class DXFImportTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(DXFImportTest, testDXFImportCPPIssue20195)
{
    EXPECT_EQ(1, 42);
}
