#include "Base/Unit.h"
#include "Base/Units.h"
#include <gtest/gtest.h>
#include <Base/Exception.h>

using namespace Base;
using namespace Units;

TEST(Unit, string_nullUnit)
{
    EXPECT_EQ(NullUnit.getString(), "");
}

TEST(Unit, string_simple_numerator_no_denominator)
{
    EXPECT_EQ(Length.getString(), "mm");
}

TEST(Unit, string_complex_numerator_no_denominator)
{
    EXPECT_EQ(Area.getString(), "mm^2");
}

TEST(Unit, string_complex_single_denominator)
{
    EXPECT_EQ(DissipationRate.getString(), "mm^2/s^3");
}

TEST(Unit, string_no_numerator)
{
    EXPECT_EQ(InverseArea.getString(), "1/mm^2");
}

TEST(Unit, string_complex_multi_denominator)
{
    EXPECT_EQ(MagneticFlux.getString(), "mm^2*kg/(s^2*A)");
}

TEST(Unit, type_string_null)
{
    EXPECT_EQ(NullUnit.getTypeString(), "NullUnit");
}

TEST(Unit, type_string_not_null)
{
    EXPECT_EQ(MagneticFlux.getTypeString(), "MagneticFlux");
}

TEST(Unit, TestEqual)
{
    EXPECT_TRUE(Length == Length);
}

TEST(Unit, TestNotEqual)
{
    EXPECT_TRUE(Length != Area);
}

TEST(Unit, multipy_nullUnits_is_nullUnit)
{
    EXPECT_EQ(NullUnit * NullUnit, NullUnit);
}

TEST(Unit, TestMult)
{
    constexpr std::array<int8_t, 8> arr {1, 1, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(Mass * Length, Unit {arr});
}

TEST(Unit, mult_nullUnit_is_nullUnit)
{
    EXPECT_EQ(NullUnit * NullUnit, NullUnit);
}

TEST(Unit, div)
{
    EXPECT_EQ(Area / Length, Length);
}

TEST(Unit, div_by_nullUnit_does_nothing)
{
    EXPECT_EQ(Area / NullUnit, Area);
}

TEST(Unit, pow_0_is_null_unit)
{
    EXPECT_EQ(Area.pow(0), NullUnit);
}

TEST(Unit, pow_1_leaves_unit_unchanged)
{
    EXPECT_EQ(Area.pow(1), Area);
}

TEST(Unit, pow_2_is_squared)
{
    EXPECT_EQ(Length.pow(2), Area);
}

TEST(Unit, pow_3_is_cubed)
{
    EXPECT_EQ(Length.pow(3), Volume);
}

TEST(Unit, pow_less_then_one)
{
    EXPECT_EQ(Volume.pow(1.0 / 3.0), Length);
}

TEST(Unit, null_unit_still_null_after_pow)
{
    EXPECT_EQ(NullUnit.pow(2), NullUnit);
}

TEST(Unit, square_root)
{
    EXPECT_EQ(Area.root(2), Length);
}

TEST(Unit, cube_root)
{
    EXPECT_EQ(Volume.root(3), Length);
}

TEST(Unit, zero_root)
{
    EXPECT_THROW([[maybe_unused]] auto res = Area.root(0), UnitsMismatchError);
}

TEST(Unit, one_root)
{
    EXPECT_EQ(Area.root(1), Area);
}

TEST(Unit, TestPow3DIV2)
{
    EXPECT_THROW([[maybe_unused]] auto res = Volume.pow(3.0 / 2.0), UnitsMismatchError);
}

TEST(Unit, overflow)
{
    constexpr UnitVals arr {99, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(Unit {arr}, NullUnit);
}

TEST(Unit, underflow)
{
    constexpr UnitVals arr {-99, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(Unit {arr}, NullUnit);
}

TEST(Unit, representation_simple)
{
    const std::string expect {"Unit: mm (1,0,0,0,0,0,0,0) [Length]"};
    const auto actual = Length.representation();
    EXPECT_EQ(actual, expect);
}

TEST(Unit, representation_complex)
{
    const std::string expect {"Unit: mm^2*kg/(s^2*A) (2,1,-2,-1,0,0,0,0) [MagneticFlux]"};
    const auto actual = MagneticFlux.representation();
    EXPECT_EQ(actual, expect);
}

TEST(Unit, representation_no_name)
{
    constexpr Unit unit {{1, 1}};
    const std::string expect {"Unit: mm*kg (1,1,0,0,0,0,0,0)"};
    const auto actual = unit.representation();
    EXPECT_EQ(actual, expect);
}
