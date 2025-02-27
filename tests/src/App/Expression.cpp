#include <gtest/gtest.h>

#include "App/ExpressionParser.h"
#include "App/ExpressionTokenizer.h"

// clang-format off
TEST(Expression, tokenize)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral(""), 10), QString());
    // 0.0000 deg-
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-"), 10), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-s"), 11), QStringLiteral("s"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-ss"), 12), QStringLiteral("ss"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("0.00000 deg"), 5), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("0.00000 deg"), 11), QStringLiteral("deg"));
}

TEST(Expression, tokenizeCompletion)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube"), 7), QStringLiteral("MyCube"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube0"), 8), QStringLiteral("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube 0"), 9), QStringLiteral("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube1"), 8), QStringLiteral("MyCube1"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube 1"), 9), QStringLiteral("MyCube1"));
}

TEST(Expression, tokenizeQuantity)
{
    auto result = App::ExpressionParser::tokenize("0.00000 deg");
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::NUM);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "0.00000");
    EXPECT_EQ(std::get<0>(result[1]), App::ExpressionParser::UNIT);
    EXPECT_EQ(std::get<1>(result[1]), 8);
    EXPECT_EQ(std::get<2>(result[1]), "deg");
}

TEST(Expression, tokenizeFunc)
{
    auto result = App::ExpressionParser::tokenize("sin(0.00000)");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::FUNC);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "sin(");
    EXPECT_EQ(std::get<0>(result[1]), App::ExpressionParser::NUM);
    EXPECT_EQ(std::get<1>(result[1]), 4);
    EXPECT_EQ(std::get<2>(result[1]), "0.00000");
    EXPECT_EQ(std::get<1>(result[2]), 11);
    EXPECT_EQ(std::get<2>(result[2]), ")");
}

TEST(Expression, tokenizeOne)
{
    auto result = App::ExpressionParser::tokenize("1");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::ONE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "1");
}

TEST(Expression, tokenizeNum)
{
    auto result = App::ExpressionParser::tokenize("1.2341");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::NUM);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "1.2341");
}

TEST(Expression, tokenizeID)
{
    auto result = App::ExpressionParser::tokenize("Something");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::IDENTIFIER);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "Something");
}

TEST(Expression, tokenizeUnit)
{
    auto result = App::ExpressionParser::tokenize("km");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::UNIT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "km");
}

TEST(Expression, tokenizeUSUnit)
{
    auto result = App::ExpressionParser::tokenize("\"");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::USUNIT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "\"");
}

TEST(Expression, tokenizeInt)
{
    auto result = App::ExpressionParser::tokenize("123456");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::INTEGER);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "123456");
}

TEST(Expression, tokenizePi)
{
    auto result = App::ExpressionParser::tokenize("pi");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "pi");
}

TEST(Expression, tokenizeE)
{
    auto result = App::ExpressionParser::tokenize("e");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "e");
}

TEST(Expression, tokenizeConstant)
{
    auto result = App::ExpressionParser::tokenize("True False true false None");
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "True");
    EXPECT_EQ(std::get<0>(result[1]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<0>(result[2]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<0>(result[3]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<0>(result[4]), App::ExpressionParser::CONSTANT);
}

TEST(Expression, tokenizeEqual)
{
    auto result = App::ExpressionParser::tokenize("==");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::EQ);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "==");
}

TEST(Expression, tokenizeNotEqual)
{
    auto result = App::ExpressionParser::tokenize("!=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::NEQ);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "!=");
}

TEST(Expression, tokenizeLessThan)
{
    auto result = App::ExpressionParser::tokenize("<");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::LT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<");
}

TEST(Expression, tokenizeLessThanEqual)
{
    auto result = App::ExpressionParser::tokenize("<=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::LTE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<=");
}

TEST(Expression, tokenizeGreaterThan)
{
    auto result = App::ExpressionParser::tokenize(">");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::GT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), ">");
}

TEST(Expression, tokenizeGreaterThanEqual)
{
    auto result = App::ExpressionParser::tokenize(">=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::GTE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), ">=");
}

TEST(Expression, tokenizeMinus)
{
    auto result = App::ExpressionParser::tokenize("1-1");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<0>(result[1]), App::ExpressionParser::MINUSSIGN);
    EXPECT_EQ(std::get<1>(result[1]), 1);
    EXPECT_EQ(std::get<2>(result[1]), "-");
}

TEST(Expression, tokenizeCell1)
{
    auto result = App::ExpressionParser::tokenize("$A$12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "$A$12");
}

TEST(Expression, tokenizeCell2)
{
    auto result = App::ExpressionParser::tokenize("A$12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "A$12");
}

TEST(Expression, tokenizeCell3)
{
    auto result = App::ExpressionParser::tokenize("$A12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "$A12");
}

TEST(Expression, tokenizeString)
{
    auto result = App::ExpressionParser::tokenize("<<Test>>");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::STRING);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<<Test>>");
}

TEST(Expression, tokenizeExponent)
{
    // TODO
}

TEST(Expression, tokenizeNumAndUnit)
{
    // TODO
}

TEST(Expression, tokenizePos)
{
    // TODO
}

TEST(Expression, tokenizeNeg)
{
    // TODO
}

TEST(Expression, tokenizePi_rad)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("p"), 1), QStringLiteral("p"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi"), 2), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi "), 3), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi r"), 4), QStringLiteral("r"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi ra"), 5), QStringLiteral("ra"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi rad"), 6), QStringLiteral("rad"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi rad"), 2), QString());
}

TEST(Expression, toString)
{
    App::UnitExpression expr{nullptr, Base::Quantity{}, "pi rad"};
    EXPECT_EQ(expr.toString(), "pi rad");
}

TEST(Expression, test_pi_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "pi");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "pi rad");
    op.release();
}

TEST(Expression, test_e_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "e");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "e rad");
    op.release();
}
// clang-format on
