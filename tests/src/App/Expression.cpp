#include "gtest/gtest.h"

#include "App/ExpressionParser.h"
#include "App/ExpressionTokenizer.h"

// clang-format off
TEST(Expression, tokenize)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8(""), 10), QString());
    // 0.0000 deg-
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-"), 10), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-s"), 11), QString::fromLatin1("s"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-ss"), 12), QString::fromLatin1("ss"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 5), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 deg"), 11), QString::fromLatin1("deg"));
}

TEST(Expression, tokenizePi)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("p"), 1), QString::fromLatin1("p"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi"), 2), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi "), 3), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi r"), 4), QString::fromLatin1("r"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi ra"), 5), QString::fromLatin1("ra"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi rad"), 6), QString::fromLatin1("rad"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromLatin1("pi rad"), 2), QString());
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

TEST(Expression, parseQuantityFromText)
{
    EXPECT_ANY_THROW(App::parseQuantityFromText("")) << "should not parse empty";
    EXPECT_ANY_THROW(App::parseQuantityFromText("mm")) << "should not parse missing value";
    EXPECT_NO_THROW(App::parseQuantityFromText("2")) << "ok to parse missing unit";
    EXPECT_NO_THROW(App::parseQuantityFromText("2mm"));
    EXPECT_NO_THROW(App::parseQuantityFromText("2 mm"));
    EXPECT_NO_THROW(App::parseQuantityFromText("\t \n .5e-3kg/m^3 \t"));
    EXPECT_NO_THROW(App::parseQuantityFromText("\n \t -6.7E3 \t A/m^2 \t"));
    EXPECT_EQ(App::parseQuantityFromText("2mm"), Base::Quantity(2.0, QString::fromStdString("mm"))); // exact ULP form
    EXPECT_EQ(App::parseQuantityFromText("2 mm"), Base::Quantity(2.0, QString::fromStdString("mm"))); // exact ULP form
    auto quant_one = App::parseQuantityFromText("\t \n.5e-3kg/m^3 \t");
    EXPECT_DOUBLE_EQ(quant_one.getValue(), 0.5e-3); // approximately equal, to within 4 ULPs
    EXPECT_EQ(quant_one.getUnit(), Base::Unit(QString::fromStdString("kg/m^3")));
    auto quant_two = App::parseQuantityFromText("\n \t -6.7E3 \t A/m^2 \t");
    EXPECT_DOUBLE_EQ(quant_two.getValue(), -6.7e+3); // approximately equal, to within 4 ULPs
    EXPECT_EQ(quant_two.getUnit(), Base::Unit(QString::fromStdString("A/m^2")));
}

TEST(Expression, anyToQuantity)
{
    EXPECT_EQ(App::anyToQuantity(Base::Quantity()), Base::Quantity());
    EXPECT_EQ(App::anyToQuantity(true), Base::Quantity(1.0));
    EXPECT_EQ(App::anyToQuantity(false), Base::Quantity(0.0));
    EXPECT_EQ(App::anyToQuantity(123), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity(123L), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity(123.0F), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity(123.0), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity("123"), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity(std::string("123")), Base::Quantity(123.0));
    EXPECT_EQ(App::anyToQuantity("123 mm"), Base::Quantity(123.0, QString::fromStdString("mm")));
    EXPECT_EQ(App::anyToQuantity(std::string("123 mm")), Base::Quantity(123.0, QString::fromStdString("mm")));
    EXPECT_ANY_THROW(App::anyToQuantity(""));
    EXPECT_ANY_THROW(App::anyToQuantity("mm"));
}

TEST(Expression, isAnyEqual)
{
    EXPECT_TRUE(App::isAnyEqual("123 mm", "123 mm"));
    EXPECT_TRUE(App::isAnyEqual("123 mm", Base::Quantity(123.0, QString::fromStdString("mm"))));
    EXPECT_TRUE(App::isAnyEqual(Base::Quantity(123.0, QString::fromStdString("mm")), "123 mm"));
}
// clang-format on
