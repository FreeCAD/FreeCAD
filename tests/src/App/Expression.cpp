#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include "App/ExpressionParser.h"
#include "App/ExpressionTokenizer.h"


class Expression: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

// clang-format off
TEST_F(Expression, tokenize)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8(""), 10), QString());
    // 0.0000 deg-
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-"), 10), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-s"), 11), QString::fromLatin1("s"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-ss"), 12), QString::fromLatin1("ss"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 5), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 11), QString::fromLatin1("deg"));
}

TEST_F(Expression, tokenizeCompletion)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("My Cube"), 7), QString::fromUtf8("MyCube"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("My Cube0"), 8), QString::fromUtf8("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("My Cube 0"), 9), QString::fromUtf8("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("My Cube1"), 8), QString::fromUtf8("MyCube1"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("My Cube 1"), 9), QString::fromUtf8("MyCube1"));
}

TEST_F(Expression, tokenizeQuantity)
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

TEST_F(Expression, tokenizeFunc)
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

TEST_F(Expression, tokenizeOne)
{
    auto result = App::ExpressionParser::tokenize("1");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::ONE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "1");
}

TEST_F(Expression, tokenizeNum)
{
    auto result = App::ExpressionParser::tokenize("1.2341");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::NUM);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "1.2341");
}

TEST_F(Expression, tokenizeID)
{
    auto result = App::ExpressionParser::tokenize("Something");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::IDENTIFIER);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "Something");
}

TEST_F(Expression, tokenizeUnit)
{
    auto result = App::ExpressionParser::tokenize("km");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::UNIT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "km");
}

TEST_F(Expression, tokenizeUSUnit)
{
    auto result = App::ExpressionParser::tokenize("\"");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::USUNIT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "\"");
}

TEST_F(Expression, tokenizeInt)
{
    auto result = App::ExpressionParser::tokenize("123456");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::INTEGER);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "123456");
}

TEST_F(Expression, tokenizePi)
{
    auto result = App::ExpressionParser::tokenize("pi");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "pi");
}

TEST_F(Expression, tokenizeE)
{
    auto result = App::ExpressionParser::tokenize("e");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CONSTANT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "e");
}

TEST_F(Expression, tokenizeConstant)
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

TEST_F(Expression, tokenizeEqual)
{
    auto result = App::ExpressionParser::tokenize("==");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::EQ);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "==");
}

TEST_F(Expression, tokenizeNotEqual)
{
    auto result = App::ExpressionParser::tokenize("!=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::NEQ);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "!=");
}

TEST_F(Expression, tokenizeLessThan)
{
    auto result = App::ExpressionParser::tokenize("<");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::LT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<");
}

TEST_F(Expression, tokenizeLessThanEqual)
{
    auto result = App::ExpressionParser::tokenize("<=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::LTE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<=");
}

TEST_F(Expression, tokenizeGreaterThan)
{
    auto result = App::ExpressionParser::tokenize(">");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::GT);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), ">");
}

TEST_F(Expression, tokenizeGreaterThanEqual)
{
    auto result = App::ExpressionParser::tokenize(">=");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::GTE);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), ">=");
}

TEST_F(Expression, tokenizeMinus)
{
    auto result = App::ExpressionParser::tokenize("1-1");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<0>(result[1]), App::ExpressionParser::MINUSSIGN);
    EXPECT_EQ(std::get<1>(result[1]), 1);
    EXPECT_EQ(std::get<2>(result[1]), "-");
}

TEST_F(Expression, tokenizeCell1)
{
    auto result = App::ExpressionParser::tokenize("$A$12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "$A$12");
}

TEST_F(Expression, tokenizeCell2)
{
    auto result = App::ExpressionParser::tokenize("A$12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "A$12");
}

TEST_F(Expression, tokenizeCell3)
{
    auto result = App::ExpressionParser::tokenize("$A12");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::CELLADDRESS);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "$A12");
}

TEST_F(Expression, tokenizeString)
{
    auto result = App::ExpressionParser::tokenize("<<Test>>");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<0>(result[0]), App::ExpressionParser::STRING);
    EXPECT_EQ(std::get<1>(result[0]), 0);
    EXPECT_EQ(std::get<2>(result[0]), "<<Test>>");
}

TEST_F(Expression, tokenizeExponent)
{
    // TODO
}

TEST_F(Expression, tokenizeNumAndUnit)
{
    // TODO
}

TEST_F(Expression, tokenizePos)
{
    // TODO
}

TEST_F(Expression, tokenizeNeg)
{
    // TODO
}

TEST_F(Expression, tokenizePi_rad)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("p"), 1), QString::fromLatin1("p"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi"), 2), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi "), 3), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi r"), 4), QString::fromLatin1("r"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi ra"), 5), QString::fromLatin1("ra"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi rad"), 6), QString::fromLatin1("rad"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi rad"), 2), QString());
}

TEST_F(Expression, toString)
{
    App::UnitExpression expr{nullptr, Base::Quantity{}, "pi rad"};
    EXPECT_EQ(expr.toString(), "pi rad");
}

TEST_F(Expression, test_pi_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "pi");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "pi rad");
    op.release();
}

TEST_F(Expression, test_e_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "e");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "e rad");
    op.release();
}
// clang-format on
