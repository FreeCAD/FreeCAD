#include <gtest/gtest.h>
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <QLocale>

using Base::ParserError;
using Base::Quantity;
using Base::Unit;
using Base::UnitsMismatchError;


TEST(BaseQuantity, TestValid)
{
    const Quantity q1 {1.0, Unit::Length};
    Quantity q2 {1.0, Unit::Area};
    q2.setInvalid();

    EXPECT_EQ(q1.isValid(), true);
    EXPECT_EQ(q2.isValid(), false);
}

TEST(BaseQuantity, TestParse)
{
    const Quantity q1 = Quantity::parse("1,234 kg");
    constexpr auto val {1.2340};
    EXPECT_EQ(q1, Quantity(val, Unit::Mass));
    EXPECT_THROW(auto rew [[maybe_unused]] = Quantity::parse("1,234,500.12 kg"), ParserError);
}

TEST(BaseQuantity, TestNoDim)
{
    const Quantity q1 {};

    EXPECT_EQ(q1.pow(2), Quantity {0});
    EXPECT_EQ(q1.isDimensionless(), true);
}

TEST(BaseQuantity, TestPowEQ1)
{
    const Quantity q1 {2, Unit::Area};
    const auto expect = Quantity {2, Unit::Area};
    EXPECT_EQ(q1.pow(1), expect);
}

TEST(BaseQuantity, TestPowEQ0)
{
    const Quantity q1 {2, Unit::Area};
    EXPECT_EQ(q1.pow(0), Quantity {1});
}

TEST(BaseQuantity, TestPowGT1)
{
    constexpr auto v2 {2};
    constexpr auto v4 {4};
    const Quantity q1 {v2, Unit::Length};
    EXPECT_EQ(q1.pow(v2), Quantity(v4, Unit::Area));
}

TEST(BaseQuantity, TestPowLT1)
{
    constexpr auto v8 {8};
    constexpr auto v2 {2};
    constexpr auto v3 {3.0};
    const Quantity q1 {v8, Unit::Volume};
    EXPECT_EQ(q1.pow(1.0 / v3), Quantity(v2, Unit::Length));
}

TEST(BaseQuantity, TestPow3DIV2)
{
    constexpr auto v2 {2.0};
    constexpr auto v3 {3.0};
    constexpr auto v8 {8};
    const Quantity unit {v8, Unit::Volume};
    EXPECT_THROW(unit.pow(v3 / v2), UnitsMismatchError);
}

TEST(BaseQuantity, TestString)
{
    constexpr auto v2 {2};
    const Quantity q1 {v2, "kg*m/s^2"};
    EXPECT_EQ(q1.getUnit(), Unit::Force);

    const Quantity q2 {v2, "kg*m^2/s^2"};
    EXPECT_EQ(q2.getUnit(), Unit::Work);
}

TEST(BaseQuantity, TestCopy)
{
    const Quantity q1 {1.0, Unit::Length};

    EXPECT_EQ(Quantity {q1}, q1);
}

TEST(BaseQuantity, TestEqual)
{
    const Quantity q1 {1.0, Unit::Force};
    const Quantity q2 {1.0, "kg*mm/s^2"};

    EXPECT_EQ(q1 == q1, true);
    EXPECT_EQ(q1 == q2, true);
}

TEST(BaseQuantity, TestNotEqual)
{
    constexpr auto v2 {2.0};
    const Quantity q1 {1.0, Unit::Force};
    const Quantity q2 {v2, "kg*m/s^2"};
    const Quantity q3 {1.0, Unit::Work};

    EXPECT_EQ(q1 != q2, true);
    EXPECT_EQ(q1 != q3, true);
}

TEST(BaseQuantity, TestLessOrGreater)
{
    constexpr auto v2 {2.0};
    Quantity q1 {1.0, Unit::Force};
    Quantity q2 {v2, "kg*m/s^2"};
    Quantity q3 {v2, Unit::Work};

    EXPECT_EQ(q1 < q2, true);
    EXPECT_EQ(q1 > q2, false);
    EXPECT_EQ(q1 <= q1, true);
    EXPECT_EQ(q1 >= q1, true);
    EXPECT_THROW(auto res [[maybe_unused]] = (q1 < q3), UnitsMismatchError);
    EXPECT_THROW(auto res [[maybe_unused]] = (q1 > q3), UnitsMismatchError);
    EXPECT_THROW(auto res [[maybe_unused]] = (q1 <= q3), UnitsMismatchError);
    EXPECT_THROW(auto res [[maybe_unused]] = (q1 >= q3), UnitsMismatchError);
}

TEST(BaseQuantity, TestAdd)
{
    Quantity q1 {1.0, Unit::Length};
    Quantity q2 {1.0, Unit::Area};
    EXPECT_THROW(q1 + q2, UnitsMismatchError);
    EXPECT_THROW(q1 += q2, UnitsMismatchError);
    EXPECT_EQ(q1 + q1, Quantity(2, Unit::Length));
    EXPECT_EQ(q1 += q1, Quantity(2, Unit::Length));
}

TEST(BaseQuantity, TestSub)
{
    Quantity q1 {1.0, Unit::Length};
    Quantity q2 {1.0, Unit::Area};
    EXPECT_THROW(q1 - q2, UnitsMismatchError);
    EXPECT_THROW(q1 -= q2, UnitsMismatchError);
    EXPECT_EQ(q1 - q1, Quantity(0, Unit::Length));
    EXPECT_EQ(q1 -= q1, Quantity(0, Unit::Length));
}

TEST(BaseQuantity, TestNeg)
{
    const Quantity q1 {1.0, Unit::Length};
    EXPECT_EQ(-q1, Quantity(-1.0, Unit::Length));
}

TEST(BaseQuantity, TestMult)
{
    const Quantity q1 {1.0, Unit::Length};
    const Quantity q2 {1.0, Unit::Area};
    EXPECT_EQ(q1 * q2, Quantity(1.0, Unit::Volume));
    EXPECT_EQ(q1 * 2.0, Quantity(2.0, Unit::Length));
}

TEST(BaseQuantity, TestDiv)
{
    const Quantity q1 {1.0, Unit::Length};
    const Quantity q2 {1.0, Unit::Area};
    EXPECT_EQ(q1 / q2, Quantity(1.0, Unit::InverseLength));
    EXPECT_EQ(q1 / 2.0, Quantity(0.5, Unit::Length));
}

TEST(BaseQuantity, TestPow)
{
    constexpr auto v2 {2.0};
    constexpr auto v4 {4};

    Quantity q1 {v2, Unit::Length};
    Quantity q2 {v2, Unit::Area};
    Quantity q3 {0.0};
    EXPECT_EQ(q1.pow(q3), Quantity {1});
    EXPECT_EQ(q1.pow(v2), Quantity(v4, Unit::Area));
    EXPECT_THROW(q1.pow(q2), UnitsMismatchError);
}

class BaseQuantityLoc: public ::testing::Test
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

TEST_F(BaseQuantityLoc, psi_parse_spaced)
{
    const auto qParsed = Quantity::parse("1 psi");
    EXPECT_EQ(qParsed.getValue(), 6.8947448254939996);
}

TEST_F(BaseQuantityLoc, psi_parse_no_space)
{
    const auto qParsed = Quantity::parse("1psi");
    EXPECT_EQ(qParsed.getValue(), 6.8947448254939996);
}

TEST_F(BaseQuantityLoc, psi_parse_user_str)
{
    const auto qParsed = Quantity::parse("1 psi");
    EXPECT_EQ(qParsed.getUserString(), "6894.74 Pa");
}

TEST_F(BaseQuantityLoc, psi_parse_safe_user_str)
{
    const auto qParsed = Quantity::parse("1 psi");
    EXPECT_EQ(qParsed.getSafeUserString(), "6894.74 Pa");
}

TEST_F(BaseQuantityLoc, psi_parse_unit_type)
{
    const auto qParsed = Quantity::parse("1 psi");
    EXPECT_EQ(qParsed.getUnit().getTypeString(), "Pressure");
}

TEST_F(BaseQuantityLoc, psi_to_Pa)
{
    const auto result = Quantity::parse("1 psi").getValueAs(Quantity::Pascal);
    const auto expect = 6894.7448254939991;

    EXPECT_EQ(result, expect);
}

TEST_F(BaseQuantityLoc, psi_to_KPa)
{
    const auto result = Quantity::parse("1 psi").getValueAs(Quantity::KiloPascal);
    const auto expect = 6.8947448254939996;

    EXPECT_EQ(result, expect);
}

TEST_F(BaseQuantityLoc, psi_to_MPa)
{
    const auto result = Quantity::parse("1 psi").getValueAs(Quantity::MegaPascal);
    const auto expect = 0.0068947448254939999;

    EXPECT_EQ(result, expect);
}

TEST_F(BaseQuantityLoc, voltage_unit)
{
    const auto qq = Quantity::parse("1e20 V");

    EXPECT_EQ(qq.getUnit(), Unit::ElectricPotential);
}

TEST_F(BaseQuantityLoc, voltage_val)
{
    const auto qq = Quantity::parse("1e20 V");

    EXPECT_EQ(qq.getValue(), 1e+26);
}

TEST_F(BaseQuantityLoc, voltage_val_smaller)
{
    const auto qq = Quantity::parse("1e3 V");

    EXPECT_EQ(qq.getValue(), 1e+9);
}
