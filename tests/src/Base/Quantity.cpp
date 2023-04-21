#include "gtest/gtest.h"
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Base/UnitsSchemaImperial1.h>
#include <QLocale>
#include <boost/core/ignore_unused.hpp>

// NOLINTBEGIN
TEST(BaseQuantity, TestValid)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};
    Base::Quantity q2 {1.0, Base::Unit::Area};
    q2.setInvalid();

    EXPECT_EQ(q1.isValid(), true);
    EXPECT_EQ(q2.isValid(), false);
}

TEST(BaseQuantity, TestParse)
{
    Base::Quantity q1 = Base::Quantity::parse(QString::fromLatin1("1,234 kg"));

    EXPECT_EQ(q1, Base::Quantity(1.2340, Base::Unit::Mass));
    EXPECT_THROW(
        boost::ignore_unused(Base::Quantity::parse(QString::fromLatin1("1,234,500.12 kg"))),
        Base::ParserError);
}

TEST(BaseQuantity, TestDim)
{
    Base::Quantity q1 {0, Base::Unit::Area};

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
    Base::Quantity q1 {2, Base::Unit::Area};
    EXPECT_EQ(q1.pow(1), Base::Quantity(2, Base::Unit::Area));
}

TEST(BaseQuantity, TestPowEQ0)
{
    Base::Quantity q1 {2, Base::Unit::Area};
    EXPECT_EQ(q1.pow(0), Base::Quantity {1});
}

TEST(BaseQuantity, TestPowGT1)
{
    Base::Quantity q1 {2, Base::Unit::Length};
    EXPECT_EQ(q1.pow(2), Base::Quantity(4, Base::Unit::Area));
}

TEST(BaseQuantity, TestPowLT1)
{
    Base::Quantity q1 {8, Base::Unit::Volume};
    EXPECT_EQ(q1.pow(1.0 / 3.0), Base::Quantity(2, Base::Unit::Length));
}

TEST(BaseQuantity, TestPow3DIV2)
{
    Base::Quantity unit {8, Base::Unit::Volume};
    EXPECT_THROW(unit.pow(3.0 / 2.0), Base::UnitsMismatchError);
}

TEST(BaseQuantity, TestString)
{
    Base::Quantity q1 {2, QString::fromLatin1("kg*m/s^2")};
    EXPECT_EQ(q1.getUnit(), Base::Unit::Force);

    Base::Quantity q2 {2, QString::fromLatin1("kg*m^2/s^2")};
    EXPECT_EQ(q2.getUnit(), Base::Unit::Work);
}

TEST(BaseQuantity, TestCopy)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};

    EXPECT_EQ(Base::Quantity {q1}, q1);
}

TEST(BaseQuantity, TestEqual)
{
    Base::Quantity q1 {1.0, Base::Unit::Force};
    Base::Quantity q2 {1.0, QString::fromLatin1("kg*mm/s^2")};

    EXPECT_EQ(q1 == q1, true);
    EXPECT_EQ(q1 == q2, true);
}

TEST(BaseQuantity, TestNotEqual)
{
    Base::Quantity q1 {1.0, Base::Unit::Force};
    Base::Quantity q2 {2.0, QString::fromLatin1("kg*m/s^2")};
    Base::Quantity q3 {1.0, Base::Unit::Work};

    EXPECT_EQ(q1 != q2, true);
    EXPECT_EQ(q1 != q3, true);
}

TEST(BaseQuantity, TestLessOrGreater)
{
    Base::Quantity q1 {1.0, Base::Unit::Force};
    Base::Quantity q2 {2.0, QString::fromLatin1("kg*m/s^2")};
    Base::Quantity q3 {2.0, Base::Unit::Work};

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
    Base::Quantity q1 {1.0, Base::Unit::Length};
    Base::Quantity q2 {1.0, Base::Unit::Area};
    EXPECT_THROW(q1 + q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 += q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 + q1, Base::Quantity(2, Base::Unit::Length));
    EXPECT_EQ(q1 += q1, Base::Quantity(2, Base::Unit::Length));
}

TEST(BaseQuantity, TestSub)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};
    Base::Quantity q2 {1.0, Base::Unit::Area};
    EXPECT_THROW(q1 - q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 -= q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 - q1, Base::Quantity(0, Base::Unit::Length));
    EXPECT_EQ(q1 -= q1, Base::Quantity(0, Base::Unit::Length));
}

TEST(BaseQuantity, TestNeg)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};
    EXPECT_EQ(-q1, Base::Quantity(-1.0, Base::Unit::Length));
}

TEST(BaseQuantity, TestMult)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};
    Base::Quantity q2 {1.0, Base::Unit::Area};
    EXPECT_EQ(q1 * q2, Base::Quantity(1.0, Base::Unit::Volume));
    EXPECT_EQ(q1 * 2.0, Base::Quantity(2.0, Base::Unit::Length));
}

TEST(BaseQuantity, TestDiv)
{
    Base::Quantity q1 {1.0, Base::Unit::Length};
    Base::Quantity q2 {1.0, Base::Unit::Area};
    EXPECT_EQ(q1 / q2, Base::Quantity(1.0, Base::Unit::InverseLength));
    EXPECT_EQ(q1 / 2.0, Base::Quantity(0.5, Base::Unit::Length));
}

TEST(BaseQuantity, TestPow)
{
    Base::Quantity q1 {2.0, Base::Unit::Length};
    Base::Quantity q2 {2.0, Base::Unit::Area};
    Base::Quantity q3 {0.0};
    EXPECT_EQ(q1.pow(q3), Base::Quantity {1});
    EXPECT_EQ(q1.pow(2.0), Base::Quantity(4, Base::Unit::Area));
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
    Base::Quantity quantity {1.0, Base::Unit::Length};

    double factor {};
    QString unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    QString result = scheme->schemaTranslate(quantity, factor, unitString);
    EXPECT_EQ(result.toStdString(), "0.04 in");
}

TEST_F(Quantity, TestSchemeImperialOne)
{
    Base::Quantity quantity {1.0, Base::Unit::Length};

    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    double factor {};
    QString unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    QString result = scheme->schemaTranslate(quantity, factor, unitString);

    EXPECT_EQ(result.toStdString(), "0.0 in");
}

TEST_F(Quantity, TestSafeUserString)
{
    Base::UnitsApi::setSchema(Base::UnitSystem::ImperialDecimal);

    Base::Quantity quantity {1.0, Base::Unit::Length};
    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    QString result = quantity.getSafeUserString();

    EXPECT_EQ(result.toStdString(), "1 mm");
}
// NOLINTEND
