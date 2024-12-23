// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "App/ProgramOptionsUtilities.h"

using namespace App::Util;

using stringPair = std::pair<std::string, std::string>;

TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxLookup)
{
    stringPair res {customSyntax("-display")};
    stringPair exp {"display", "null"};
    EXPECT_EQ(res, exp);
};
TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxMac)
{
#if defined(FC_OS_MACOSX)
    stringPair res {customSyntax("-psn_stuff")};
    stringPair exp {"psn", "stuff"};
    EXPECT_EQ(res, exp);
#endif
};
TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxWidgetCount)
{
    stringPair res {customSyntax("-widgetcount")};
    stringPair exp {"widgetcount", ""};
    EXPECT_EQ(res, exp);
}
TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxNotFound)
{
    stringPair res {customSyntax("-displayx")};
    stringPair exp {"", ""};
    EXPECT_EQ(res, exp);
};
TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxAmpersand)
{
    stringPair res {customSyntax("@freddie")};
    stringPair exp {"response-file", "freddie"};
    EXPECT_EQ(res, exp);
};
TEST(ProgramOptionsUtilitiesTest, fCustomSyntaxEmptyIn)
{
    stringPair res {customSyntax("")};
    stringPair exp {"", ""};
    EXPECT_EQ(res, exp);
};
