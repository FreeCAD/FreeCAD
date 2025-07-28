#include <gtest/gtest.h>

#include "Gui/Assistant.h"

#include <src/App/InitApplication.h>


class Assistant: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(Assistant, first)
{}
