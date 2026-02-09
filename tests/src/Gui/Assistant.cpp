// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

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
