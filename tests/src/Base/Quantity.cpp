#include <gtest/gtest.h>
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include "Base/Units.h"
#include <Base/UnitsSchemaImperial1.h>
#include <QLocale>
#include <boost/core/ignore_unused.hpp>

// NOLINTBEGIN
TEST(BaseQuantity, TestValid)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    Base::Quantity q2 {1.0, Base::Units::Area};
    q2.setInvalid();

    EXPECT_EQ(q1.isValid(), true);
    EXPECT_EQ(q2.isValid(), false);
}

TEST(BaseQuantity, TestParse)
{
    Base::Quantity q1 = Base::Quantity::parse("1,234 kg");

    EXPECT_EQ(q1, Base::Quantity(1.2340, Base::Units::Mass));
    EXPECT_THROW(boost::ignore_unused(Base::Quantity::parse("1,234,500.12 kg")), Base::ParserError);
}

TEST(BaseQuantity, TestDim)
{
    Base::Quantity q1 {0, Base::Units::Area};

    EXPECT_EQ(q1.isQuantity(), true);
}

TEST(BaseQuantity, TestNoDim)
{
    Base::Quantity q1 {};

    EXPECT_EQ(q1.pow(2), Base::Quantity {0});
    EXPECT_EQ(q1.isDimensionless(), true);
}

TEST(BaseQuantity, TestPowEQ1)
{
    Base::Quantity q1 {2, Base::Units::Area};
    EXPECT_EQ(q1.pow(1), Base::Quantity(2, Base::Units::Area));
}

TEST(BaseQuantity, TestPowEQ0)
{
    Base::Quantity q1 {2, Base::Units::Area};
    EXPECT_EQ(q1.pow(0), Base::Quantity {1});
}

TEST(BaseQuantity, TestPowGT1)
{
    Base::Quantity q1 {2, Base::Units::Length};
    EXPECT_EQ(q1.pow(2), Base::Quantity(4, Base::Units::Area));
}

TEST(BaseQuantity, TestPowLT1)
{
    Base::Quantity q1 {8, Base::Units::Volume};
    EXPECT_EQ(q1.pow(1.0 / 3.0), Base::Quantity(2, Base::Units::Length));
}

TEST(BaseQuantity, TestPow3DIV2)
{
    Base::Quantity unit {8, Base::Units::Volume};
    EXPECT_THROW(unit.pow(3.0 / 2.0), Base::UnitsMismatchError);
}

TEST(BaseQuantity, TestString)
{
    Base::Quantity q1 {2, "kg*m/s^2"};
    EXPECT_EQ(q1.getUnit(), Base::Units::Force);

    Base::Quantity q2 {2, "kg*m^2/s^2"};
    EXPECT_EQ(q2.getUnit(), Base::Units::Work);
}

TEST(BaseQuantity, TestCopy)
{
    Base::Quantity q1 {1.0, Base::Units::Length};

    EXPECT_EQ(Base::Quantity {q1}, q1);
}

TEST(BaseQuantity, TestEqual)
{
    Base::Quantity q1 {1.0, Base::Units::Force};
    Base::Quantity q2 {1.0, "kg*mm/s^2"};

    EXPECT_EQ(q1 == q1, true);
    EXPECT_EQ(q1 == q2, true);
}

TEST(BaseQuantity, TestNotEqual)
{
    Base::Quantity q1 {1.0, Base::Units::Force};
    Base::Quantity q2 {2.0, "kg*m/s^2"};
    Base::Quantity q3 {1.0, Base::Units::Work};

    EXPECT_EQ(q1 != q2, true);
    EXPECT_EQ(q1 != q3, true);
}

TEST(BaseQuantity, TestLessOrGreater)
{
    Base::Quantity q1 {1.0, Base::Units::Force};
    Base::Quantity q2 {2.0, "kg*m/s^2"};
    Base::Quantity q3 {2.0, Base::Units::Work};

    EXPECT_EQ(q1 < q2, true);
    EXPECT_EQ(q1 > q2, false);
    EXPECT_EQ(q1 <= q1, true);
    EXPECT_EQ(q1 >= q1, true);
    EXPECT_THROW(boost::ignore_unused(q1 < q3), Base::UnitsMismatchError);
    EXPECT_THROW(boost::ignore_unused(q1 > q3), Base::UnitsMismatchError);
    EXPECT_THROW(boost::ignore_unused(q1 <= q3), Base::UnitsMismatchError);
    EXPECT_THROW(boost::ignore_unused(q1 >= q3), Base::UnitsMismatchError);
}

TEST(BaseQuantity, TestAdd)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    Base::Quantity q2 {1.0, Base::Units::Area};
    EXPECT_THROW(q1 + q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 += q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 + q1, Base::Quantity(2, Base::Units::Length));
    EXPECT_EQ(q1 += q1, Base::Quantity(2, Base::Units::Length));
}

TEST(BaseQuantity, TestSub)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    Base::Quantity q2 {1.0, Base::Units::Area};
    EXPECT_THROW(q1 - q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 -= q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 - q1, Base::Quantity(0, Base::Units::Length));
    EXPECT_EQ(q1 -= q1, Base::Quantity(0, Base::Units::Length));
}

TEST(BaseQuantity, TestNeg)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    EXPECT_EQ(-q1, Base::Quantity(-1.0, Base::Units::Length));
}

TEST(BaseQuantity, TestMult)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    Base::Quantity q2 {1.0, Base::Units::Area};
    EXPECT_EQ(q1 * q2, Base::Quantity(1.0, Base::Units::Volume));
    EXPECT_EQ(q1 * 2.0, Base::Quantity(2.0, Base::Units::Length));
}

TEST(BaseQuantity, TestDiv)
{
    Base::Quantity q1 {1.0, Base::Units::Length};
    Base::Quantity q2 {1.0, Base::Units::Area};
    EXPECT_EQ(q1 / q2, Base::Quantity(1.0, Base::Units::InverseLength));
    EXPECT_EQ(q1 / 2.0, Base::Quantity(0.5, Base::Units::Length));
}

TEST(BaseQuantity, TestPow)
{
    Base::Quantity q1 {2.0, Base::Units::Length};
    Base::Quantity q2 {2.0, Base::Units::Area};
    Base::Quantity q3 {0.0};
    EXPECT_EQ(q1.pow(q3), Base::Quantity {1});
    EXPECT_EQ(q1.pow(2.0), Base::Quantity(4, Base::Units::Area));
    EXPECT_THROW(q1.pow(q2), Base::UnitsMismatchError);
}

class Quantity: public ::testing::Test
{
protected:
    void SetUp() override
    {
        QLocale loc(QLocale::C);
        QLocale::setDefault(loc);
    }
    void TearDown() override
    {}
};

TEST_F(Quantity, TestSchemeImperialTwo)
{
    Base::Quantity quantity {1.0, Base::Units::Length};

    double factor {};
    std::string unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    std::string result = scheme->schemaTranslate(quantity, factor, unitString);
    EXPECT_EQ(result, "0.04 in");
}

TEST_F(Quantity, TestSchemeImperialOne)
{
    Base::Quantity quantity {1.0, Base::Units::Length};

    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    double factor {};
    std::string unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    std::string result = scheme->schemaTranslate(quantity, factor, unitString);

    EXPECT_EQ(result, "0.0 in");
}

TEST_F(Quantity, TestSafeUserString)
{
    Base::UnitsApi::setSchema(Base::UnitSystem::ImperialDecimal);

    Base::Quantity quantity {1.0, Base::Units::Length};
    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    std::string result = quantity.getSafeUserString();

    EXPECT_EQ(result, "1 mm");

    Base::UnitsApi::setSchema(Base::UnitSystem::Imperial1);

    quantity = Base::Quantity {304.8, Base::Units::Length};
    quantity.setFormat(format);

    result = quantity.getSafeUserString();

    EXPECT_EQ(result, "1.0 \\'");

    quantity = Base::Quantity {25.4, Base::Units::Length};
    quantity.setFormat(format);

    result = quantity.getSafeUserString();

    EXPECT_EQ(result, "1.0 \\\"");
}
// NOLINTEND
