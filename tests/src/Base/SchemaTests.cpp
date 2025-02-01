/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#include <gtest/gtest.h>
#include "Base/Exception.h"
#include "Base/Tools.h"
#include "Base/Unit.h"
#include "Base/Quantity.h"
#include "Base/UnitsApi.h"
#include "Base/UnitsSchemasData.h"
#include "Base/UnitsSchemas.h"

#include <QLocale>
#include <array>
#include <string>

using Base::Quantity;
using Base::QuantityFormat;
using Base::RuntimeError;
using Base::Tools;
using Base::Unit;
using Base::UnitsApi;
using Base::UnitsSchema;
using Base::UnitsSchemas;

class SchemaTest: public testing::Test
{
protected:
    void SetUp() override
    {
        const QLocale loc(QLocale::C);
        QLocale::setDefault(loc);
    }

    void TearDown() override
    {}

    static std::string
    set(const std::string& schemaName, const Unit unit, const double value)  // NOLINT
    {
        UnitsApi::setSchema(schemaName);
        const auto quantity = Quantity {value, unit};
        return quantity.getSafeUserString();
    }

    static std::string setWithPrecision(const std::string& name,
                                        const double value,
                                        const Unit unit,
                                        const int precision)
    {
        UnitsApi::setSchema(name);
        Quantity quantity {value, unit};
        QuantityFormat format = quantity.getFormat();
        format.precision = precision;
        quantity.setFormat(format);
        return quantity.getSafeUserString();
    }

    std::unique_ptr<UnitsSchemas> schemas;  // NOLINT
};

TEST_F(SchemaTest, meter_decimal_1_mm_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 1.0, Unit::Length, 6);
    const auto expect {"0.001000 m"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_15_mm2_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 15.0, Unit::Area, 6);
    const auto expect {"0.000015 m^2"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_123456000_mm3_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 123456000.0, Unit::Volume, 6);
    const auto expect {"0.123456 m^3"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_123456000_W_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 123456000.0, Unit::Power, 6);
    const auto expect {"123.456000 W"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_123456000_V_precision_6)
{
    const std::string result =
        setWithPrecision("MeterDecimal", 123456000.0, Unit::ElectricPotential, 6);
    const auto expect {"123.456000 V"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_123456000_W_m2_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 123.456, Unit::HeatFlux, 6);
    const auto expect {"123.456000 W/m^2"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, meter_decimal_123456000_m_s_precision_6)
{
    const std::string result = setWithPrecision("MeterDecimal", 123.456, Unit::Velocity, 6);
    const auto expect {"0.123456 m/s"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_1_mm_precision_6)
{
    const std::string result = setWithPrecision("MKS", 1.0, Unit::Length, 6);
    const auto expect {"1.000000 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_15_mm2_precision_6)
{
    const std::string result = setWithPrecision("MKS", 15.0, Unit::Area, 6);
    const auto expect {"15.000000 mm^2"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_123456000_mm3_precision_6)
{
    const std::string result = setWithPrecision("MKS", 123456000.0, Unit::Volume, 6);
    const auto expect {"123.456000 l"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_123456000_W_precision_6)
{
    const std::string result = setWithPrecision("MKS", 123456000.0, Unit::Power, 6);
    const auto expect {"123.456000 W"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_123456000_V_precision_6)
{
    const std::string result = setWithPrecision("MKS", 123456000.0, Unit::ElectricPotential, 6);
    const auto expect {"123.456000 V"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_123456000_W_m2_precision_6)
{
    const std::string result = setWithPrecision("MKS", 123.456, Unit::HeatFlux, 6);
    const auto expect {"123.456000 W/m^2"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, mks_123456000_m_s_precision_6)
{
    const std::string result = setWithPrecision("MKS", 123.456, Unit::Velocity, 6);
    const auto expect {"0.123456 m/s"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_1_mm_default_precision)
{
    const std::string result = set("ImperialDecimal", Unit::Length, 1.0);
    const auto expect {"0.04 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, internal_1_mm_precision_0)
{
    const std::string result = setWithPrecision("Internal", 1.0, Unit::Length, 0);
    const auto expect {"1 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, internal_100_mm_precision_0)
{
    const std::string result = setWithPrecision("Internal", 100.0, Unit::Length, 0);
    const auto expect {"100 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, internal_100_mm_precision_1)
{
    const std::string result = setWithPrecision("Internal", 100.0, Unit::Length, 1);
    const auto expect {"100.0 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, internal_20000_mm_precision_2)
{
    const std::string result = setWithPrecision("Internal", 20000.0, Unit::Length, 2);
    const auto expect {"20.00 m"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_1_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialDecimal", 1.0, Unit::Length, 0);
    const auto expect {"1 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_10_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialDecimal", 10.0, Unit::Length, 0);
    const auto expect {"10 mm"};
    // https://github.com/FreeCAD/FreeCAD/commit/569154b73f818c6a88b010def687d5e684ce64c2

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_20_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialDecimal", 20.0, Unit::Length, 0);
    const auto expect {"1 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_1_mm_precision_0)
{
    const std::string result = setWithPrecision("Imperial", 1.0, Unit::Length, 0);
    const auto expect {"39 thou"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_0_mm_precision_0)
{
    const std::string result = setWithPrecision("Imperial", 0.0, Unit::Length, 0);
    const auto expect {"0 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_0_mm_precision_1)
{
    const std::string result = setWithPrecision("Imperial", 0.0, Unit::Length, 1);
    const auto expect {"0.0 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_0_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialDecimal", 0.0, Unit::Length, 0);
    const auto expect {"0 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_0_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialDecimal", 0.0, Unit::Length, 1);
    const auto expect {"0.0 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_0_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialCivil", 0.0, Unit::Length, 0);
    const auto expect {"0 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_0_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialCivil", 0.0, Unit::Length, 1);
    const auto expect {"0.0 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_0_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialBuilding", 0.0, Unit::Length, 0);
    const auto expect {"0"};  // don't know why

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_0_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialBuilding", 0.0, Unit::Length, 1);
    const auto expect {"0"};  // don't know why

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_1_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialDecimal", 1.0, Unit::Length, 1);
    const auto expect {"1 mm"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_100_mm_precision_0)
{
    const std::string result = setWithPrecision("ImperialDecimal", 100.0, Unit::Length, 0);
    const auto expect {"4 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_100_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialDecimal", 100.0, Unit::Length, 1);
    const auto expect {"3.9 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_100_mm_precision_2)
{
    const std::string result = setWithPrecision("ImperialDecimal", 100.0, Unit::Length, 2);
    const auto expect {"3.94 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_1_mm_precision_2)
{
    const std::string result = setWithPrecision("ImperialDecimal", 1.0, Unit::Length, 2);
    const auto expect {"0.04 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_decimal_1_mm_precision_4)
{
    const std::string result = setWithPrecision("ImperialDecimal", 1.0, Unit::Length, 4);
    const auto expect {"0.0394 in"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_same)
{
    constexpr auto val {304.8};
    const auto result = set("Imperial", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1.00'");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_more)
{
    constexpr auto val {310.0};
    const auto result = set("Imperial", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1.02'");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_less)
{
    constexpr auto val {300.0};
    const auto result = set("Imperial", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("11.81\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_one_inch)
{
    constexpr auto val {25.4};
    const auto result = set("Imperial", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1.00\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_length_inch)
{
    constexpr auto val {25.4};
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_length_foot)
{
    constexpr auto val {25.4 * 12};
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1'");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_zero_length)
{
    const auto result = set("ImperialBuilding", Unit::Length, 0.0);
    const auto expect = Tools::escapeQuotesFromString("0");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_length_negative_fraction_only)
{
    constexpr auto val {(-1.0 / 8.0) * 25.4};  // -1/8 inch in mm
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("-1/8\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_negative_inches_and_fraction)
{
    constexpr auto val {-2.5 * 25.4};  // -2.5 inches in mm
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("-2\" - 1/2\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_high_precision_rounding)
{
    constexpr auto val {25.396};  // Very close to exactly 1 inch
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_length)
{
    GTEST_SKIP() << "QuantityParser::yyparse() is crashing on the >>1' 2\" + 1/4\"<< input, "
                    "so disable this test";
    constexpr auto val {360.6};
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("1' 2\" + 1/4\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_building_special_function_length_neg)
{
    constexpr auto val {-360.6};
    const auto result = set("ImperialBuilding", Unit::Length, val);
    const auto expect = Tools::escapeQuotesFromString("-1' 2\" - 1/4\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_special_function_angle_degrees)
{
    constexpr auto val {180};
    const auto result = set("ImperialCivil", Unit::Angle, val);
    const auto expect {"180°"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_special_function_angle_minutes)
{
    constexpr auto val {180.5};
    const auto result = set("ImperialCivil", Unit::Angle, val);
    const auto expect {"180°30′"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_special_function_angle_seconds)
{
    constexpr auto val {180.11};
    const auto result = set("ImperialCivil", Unit::Angle, val);
    const auto expect {"180°6′36″"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_special_function_angle_no_degrees)
{
    constexpr auto val {0.11};
    const auto result = set("ImperialCivil", Unit::Angle, val);
    const auto expect {"0°6′36″"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, unknown_schema_name_throws)
{
    EXPECT_THROW(UnitsApi::setSchema("Unknown"), RuntimeError);
}

TEST_F(SchemaTest, round_trip_test)
{
    const auto units = std::to_array<Unit>({
        Unit::Length,
        Unit::Mass,
        Unit::Area,
        Unit::Density,
        Unit::Volume,
        Unit::TimeSpan,
        Unit::Frequency,
        Unit::Velocity,
        Unit::Acceleration,
        Unit::Temperature,
        Unit::CurrentDensity,
        Unit::ElectricCurrent,
        Unit::ElectricPotential,
        Unit::ElectricCharge,
        Unit::SurfaceChargeDensity,
        Unit::MagneticFieldStrength,
        Unit::MagneticFlux,
        Unit::MagneticFluxDensity,
        Unit::Magnetization,
        Unit::ElectricalCapacitance,
        Unit::ElectricalInductance,
        Unit::ElectricalConductance,
        Unit::ElectricalResistance,
        Unit::ElectricalConductivity,
        Unit::ElectromagneticPotential,
        Unit::AmountOfSubstance,
        Unit::LuminousIntensity,
        Unit::CompressiveStrength,
        Unit::Pressure,
        Unit::ShearModulus,
        Unit::Stress,
        Unit::UltimateTensileStrength,
        Unit::YieldStrength,
        Unit::YoungsModulus,
        Unit::Stiffness,
        Unit::StiffnessDensity,
        Unit::Force,
        Unit::Work,
        Unit::Power,
        Unit::Moment,
        Unit::SpecificEnergy,
        Unit::ThermalConductivity,
        Unit::ThermalExpansionCoefficient,
        Unit::VolumetricThermalExpansionCoefficient,
        Unit::SpecificHeat,
        Unit::ThermalTransferCoefficient,
        Unit::HeatFlux,
        Unit::DynamicViscosity,
        Unit::KinematicViscosity,
        Unit::VacuumPermittivity,
        Unit::VolumeFlowRate,
        Unit::DissipationRate,
        Unit::InverseLength,
        Unit::InverseArea,
        Unit::InverseVolume,
    });

    std::array values = {0.01, 0.1, 1.0, 10.0, 100.0};

    double factor {};
    std::string unitString;

    UnitsApi::setDecimals(16);

    UnitsApi::setSchema("Internal");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }

    UnitsApi::setSchema("MKS");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }

    UnitsApi::setSchema("Imperial");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_NEAR(q2.getValue(), value, 0.001);
        }
    }

    UnitsApi::setSchema("ImperialDecimal");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_NEAR(q2.getValue(), value, 0.001);
        }
    }

    UnitsApi::setSchema("Centimeter");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }

    UnitsApi::setSchema("MmMin");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }

    UnitsApi::setSchema("ImperialCivil");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_NEAR(q2.getValue(), value, 0.001);
        }
    }

    UnitsApi::setSchema("FEM");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }

    UnitsApi::setSchema("MeterDecimal");
    for (auto unit : units) {
        for (double value : values) {
            Quantity q1 {value, unit};
            std::string result = UnitsApi::schemaTranslate(q1, factor, unitString);
            Quantity q2 = Quantity::parse(result);
            EXPECT_DOUBLE_EQ(q2.getValue(), value);
        }
    }
}
