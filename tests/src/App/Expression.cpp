#include "gtest/gtest.h"

#include "App/ExpressionTokenizer.h"

TEST(Expression, tokenize)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8(""), 10), QString());
    // 0.0000 deg-
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-"), 10), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-s"), 11), QString("s"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-ss"), 12), QString("ss"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 5), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 11), QString("deg"));
}

TEST(Expression, tokenizePi)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("p"), 1), QString("p"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi"), 2), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi "), 3), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi r"), 4), QString("r"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi ra"), 5), QString("ra"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi rad"), 6), QString("rad"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString("pi rad"), 2), QString());
}
