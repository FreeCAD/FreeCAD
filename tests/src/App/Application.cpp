#include "gtest/gtest.h"
#define FC_OS_MACOSX 1
#include "App/ProgramOptionsUtilities.h"

using namespace App::Util;

using Spr = std::pair<std::string, std::string>;

TEST(ApplicationTest, fCustomSyntaxLookup)
{
    Spr res {customSyntax("-display")};
    Spr exp {"display", "null"};
    EXPECT_EQ(res, exp);
};
TEST(ApplicationTest, fCustomSyntaxMac)
{
    Spr res {customSyntax("-psn_stuff")};
    Spr exp {"psn", "stuff"};
    EXPECT_EQ(res, exp);
};
TEST(ApplicationTest, fCustomSyntaxWidgetCount)
{
    Spr res {customSyntax("-widgetcount")};
    Spr exp {"widgetcount", ""};
    EXPECT_EQ(res, exp);
}
TEST(ApplicationTest, fCustomSyntaxNotFound)
{
    Spr res {customSyntax("-displayx")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};
TEST(ApplicationTest, fCustomSyntaxAmpersand)
{
    Spr res {customSyntax("@freddie")};
    Spr exp {"response-file", "freddie"};
    EXPECT_EQ(res, exp);
};
TEST(ApplicationTest, fCustomSyntaxEmptyIn)
{
    Spr res {customSyntax("")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};
