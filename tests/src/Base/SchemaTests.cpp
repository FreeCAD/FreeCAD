// SPDX-FileNotice: Part of the FreeCAD project.

/************************************************************************
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

    static std::string set(const std::string& schemaName, const Unit unit, const double value)  // NOLINT
    {
        UnitsApi::setSchema(schemaName);
        const auto quantity = Quantity {value, unit};
        return quantity.getSafeUserString();
    }

    static std::string setWithPrecision(
        const std::string& name,
        const double value,
        const Unit unit,
        const int precision
    )
    {
        UnitsApi::setSchema(name);
        Quantity quantity {value, unit};
        QuantityFormat format = quantity.getFormat();
        format.setPrecision(precision);
        quantity.setFormat(format);
        return quantity.getSafeUserString();
    }

    static std::string setWithDenominator(
        const std::string& name,
        const double value,
        const Unit unit,
        const int denominator
    )
    {
        UnitsApi::setSchema(name);
        Quantity quantity {value, unit};
        QuantityFormat format = quantity.getFormat();
        format.setDenominator(denominator);
        quantity.setFormat(format);
        return quantity.getSafeUserString();
    }

    static void sweepCheck(std::initializer_list<std::initializer_list<const char*>> groups)
    {
        for (const auto& group : groups) {
            for (const char* str : group) {
                SCOPED_TRACE(str);
                auto q = Quantity::parse(str);
                QuantityFormat fmt(QuantityFormat::Default);
                q.setFormat(fmt);
                double factor {};
                std::string unitString;
                auto result = UnitsApi::schemaTranslate(q, factor, unitString);
                EXPECT_EQ(result, str);
            }
        }
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
    const std::string result
        = setWithPrecision("MeterDecimal", 123456000.0, Unit::ElectricPotential, 6);
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
    const auto expect {"0 ft"};

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_civil_0_mm_precision_1)
{
    const std::string result = setWithPrecision("ImperialCivil", 0.0, Unit::Length, 1);
    const auto expect {"0.0 ft"};

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
    const auto result = setWithPrecision("Imperial", val, Unit::Length, 2);
    const auto expect = Tools::escapeQuotesFromString("1.00'");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_more)
{
    constexpr auto val {310.0};
    const auto result = setWithPrecision("Imperial", val, Unit::Length, 2);
    const auto expect = Tools::escapeQuotesFromString("1.02'");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_less)
{
    constexpr auto val {300.0};
    const auto result = setWithPrecision("Imperial", val, Unit::Length, 2);
    const auto expect = Tools::escapeQuotesFromString("11.81\"");

    EXPECT_EQ(result, expect);
}

TEST_F(SchemaTest, imperial_safe_user_str_one_inch)
{
    constexpr auto val {25.4};
    const auto result = setWithPrecision("Imperial", val, Unit::Length, 2);
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
    const auto result = setWithDenominator("ImperialBuilding", val, Unit::Length, 8);
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
    const auto result = setWithDenominator("ImperialBuilding", val, Unit::Length, 8);
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
    const auto result = setWithDenominator("ImperialBuilding", val, Unit::Length, 8);
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

// Sweep round-trip tests: parse a string, translate it back, verify identical output.
// Each string is both the input and the expected result. Values are chosen to land
// cleanly in each threshold band so the unit selection is tested across the full range.

TEST_F(SchemaTest, sweep_internal)
{
    UnitsApi::setSchema("Internal");
    UnitsApi::setDecimals(6);
    sweepCheck({
        // Length
        {"1 nm",
         "10 nm",
         "100 nm",
         "1 \xC2\xB5m",
         "10 \xC2\xB5m",
         "1 mm",
         "10 mm",
         "100 mm",
         "1000 mm",
         "10 m",
         "100 m",
         "1000 m",
         "10 km",
         "100 km",
         "1000 km",
         /* default */ "1e+09 m"},
        // Mass
        {"1 \xC2\xB5g",
         "10 \xC2\xB5g",
         "100 \xC2\xB5g",
         "1 mg",
         "10 mg",
         "100 mg",
         "1 g",
         "10 g",
         "100 g",
         "1 kg",
         "10 kg",
         "100 kg",
         "1 t",
         "10 t",
         /* default */ "1e+06 t"},
        // Area
        {"1 mm^2",
         "10 mm^2",
         "1 cm^2",
         "10 cm^2",
         "100 cm^2",
         "1000 cm^2",
         "1 m^2",
         "10 m^2",
         "100 m^2",
         "1000 m^2",
         "1 km^2",
         /* default */ "1e+06 km^2"},
        // Volume
        {"1 mm^3",
         "10 mm^3",
         "100 mm^3",
         "1 ml",
         "10 ml",
         "100 ml",
         "1 l",
         "10 l",
         "100 l",
         "1 m^3",
         "10 m^3",
         /* default */ "1e+06 m^3"},
        // Pressure
        {"1 Pa",
         "10 Pa",
         "100 Pa",
         "1000 Pa",
         "10 kPa",
         "100 kPa",
         "1000 kPa",
         "10 MPa",
         "100 MPa",
         "1000 MPa",
         "10 GPa",
         "100 GPa",
         "1000 GPa",
         /* default */ "1e+15 Pa"},
        // Force
        {"1 mN",
         "10 mN",
         "100 mN",
         "1 N",
         "10 N",
         "100 N",
         "1 kN",
         "10 kN",
         "100 kN",
         "1 MN",
         "10 MN",
         /* default */ "1e+06 MN"},
        // Power
        {"1 mW",
         "10 mW",
         "100 mW",
         "1 W",
         "10 W",
         "100 W",
         "1 kW",
         "10 kW",
         /* default */ "1e+06 kW"},
        // ElectricPotential
        {"1 mV",
         "10 mV",
         "100 mV",
         "1 V",
         "10 V",
         "100 V",
         "1 kV",
         "10 kV",
         "100 kV",
         /* default */ "1e+07 V"},
        // Frequency
        {"1 Hz",
         "10 Hz",
         "100 Hz",
         "1 kHz",
         "10 kHz",
         "100 kHz",
         "1 MHz",
         "10 MHz",
         "100 MHz",
         "1 GHz",
         "10 GHz",
         "100 GHz",
         "1 THz",
         /* default */ "1e+06 THz"},
        // ThermalConductivity
        {"1 W/m/K",
         "10 W/m/K",
         "100 W/m/K",
         "1 W/mm/K",
         "10 W/mm/K",
         /* default */ "1e+06 W/mm/K"},
        // ElectricalConductivity
        {"1 mS/m",
         "10 mS/m",
         "100 mS/m",
         "1 S/m",
         "10 S/m",
         "100 S/m",
         "1 kS/m",
         "10 kS/m",
         "100 kS/m",
         "1 MS/m",
         /* default */ "1e+06 MS/m"},
        // SurfaceChargeDensity
        {"1 C/m^2",
         "10 C/m^2",
         "100 C/m^2",
         "1 C/cm^2",
         "10 C/cm^2",
         "1 C/mm^2",
         /* default */ "1e+06 C/mm^2"},
        // VolumeChargeDensity
        {"1 C/m^3",
         "10 C/m^3",
         "100 C/m^3",
         "1 C/cm^3",
         "10 C/cm^3",
         "100 C/cm^3",
         "1 C/mm^3",
         /* default */ "1e+06 C/mm^3"},
        // CurrentDensity
        {"1 A/m^2",
         "10 A/m^2",
         "100 A/m^2",
         "1 A/cm^2",
         "10 A/cm^2",
         "1 A/mm^2",
         /* default */ "1e+06 A/mm^2"},
        // ElectricalCapacitance
        {"1 pF",
         "10 pF",
         "100 pF",
         "1 nF",
         "10 nF",
         "100 nF",
         "1 \xC2\xB5"
         "F",
         "10 \xC2\xB5"
         "F",
         "100 \xC2\xB5"
         "F",
         "1 mF",
         "10 mF",
         "100 mF",
         "1 F",
         /* default */ "1e+06 F"},
        // ElectricalInductance
        {"1 nH",
         "10 nH",
         "100 nH",
         "1 \xC2\xB5H",
         "10 \xC2\xB5H",
         "100 \xC2\xB5H",
         "1 mH",
         "10 mH",
         "100 mH",
         "1 H",
         /* default */ "1e+06 H"},
        // ElectricalConductance
        {"1 \xC2\xB5S",
         "10 \xC2\xB5S",
         "100 \xC2\xB5S",
         "1 mS",
         "10 mS",
         "100 mS",
         "1 S",
         /* default */ "1e+06 S"},
        // ElectricalResistance
        {"1 Ohm",
         "10 Ohm",
         "100 Ohm",
         "1 kOhm",
         "10 kOhm",
         "100 kOhm",
         "1 MOhm",
         /* default */ "1e+06 MOhm"},
        // MagneticFluxDensity
        {"1 mT",
         "10 mT",
         "100 mT",
         "1 T",
         /* default */ "1e+06 T"},
        // Stiffness
        {"1 mN/m",
         "10 mN/m",
         "100 mN/m",
         "1 N/m",
         "10 N/m",
         "100 N/m",
         "1 kN/m",
         "10 kN/m",
         "100 kN/m",
         "1 MN/m",
         /* default */ "1e+06 MN/m"},
        // KinematicViscosity
        {"1 mm^2/s",
         "10 mm^2/s",
         "100 mm^2/s",
         "1 m^2/s",
         /* default */ "1e+06 m^2/s"},
        // VolumeFlowRate
        {"1 mm^3/s",
         "10 mm^3/s",
         "100 mm^3/s",
         "1 ml/s",
         "10 ml/s",
         "100 ml/s",
         "1 l/s",
         "10 l/s",
         "100 l/s",
         "1 m^3/s",
         /* default */ "1e+06 m^3/s"},
    });
}

TEST_F(SchemaTest, sweep_mks)
{
    UnitsApi::setSchema("MKS");
    UnitsApi::setDecimals(6);
    sweepCheck({
        // Length
        {"1 nm",
         "10 nm",
         "100 nm",
         "1 \xC2\xB5m",
         "10 \xC2\xB5m",
         "1 mm",
         "10 mm",
         "100 mm",
         "1000 mm",
         "10 m",
         "100 m",
         "1000 m",
         "10 km",
         "100 km",
         "1000 km",
         /* default */ "1e+09 m"},
        // Mass
        {"1 \xC2\xB5g",
         "10 \xC2\xB5g",
         "100 \xC2\xB5g",
         "1 mg",
         "10 mg",
         "100 mg",
         "1 g",
         "10 g",
         "100 g",
         "1 kg",
         "10 kg",
         "100 kg",
         "1 t",
         "10 t",
         /* default */ "1e+06 t"},
        // Area
        {"1 mm^2",
         "10 mm^2",
         "1 cm^2",
         "10 cm^2",
         "100 cm^2",
         "1000 cm^2",
         "1 m^2",
         "10 m^2",
         "100 m^2",
         "1000 m^2",
         "1 km^2",
         /* default */ "1e+06 km^2"},
        // Volume
        {"1 mm^3",
         "10 mm^3",
         "100 mm^3",
         "1 ml",
         "10 ml",
         "100 ml",
         "1 l",
         "10 l",
         "100 l",
         "1 m^3",
         "10 m^3",
         /* default */ "1e+06 m^3"},
        // Pressure
        {"1 Pa",
         "10 Pa",
         "100 Pa",
         "1000 Pa",
         "10 kPa",
         "100 kPa",
         "1000 kPa",
         "10 MPa",
         "100 MPa",
         "1000 MPa",
         "10 GPa",
         "100 GPa",
         "1000 GPa",
         /* default */ "1e+15 Pa"},
        // Force
        {"1 mN",
         "10 mN",
         "100 mN",
         "1 N",
         "10 N",
         "100 N",
         "1 kN",
         "10 kN",
         "100 kN",
         "1 MN",
         "10 MN",
         /* default */ "1e+06 MN"},
        // Power
        {"1 mW",
         "10 mW",
         "100 mW",
         "1 W",
         "10 W",
         "100 W",
         "1 kW",
         "10 kW",
         /* default */ "1e+06 kW"},
        // ElectricPotential
        {"1 mV",
         "10 mV",
         "100 mV",
         "1 V",
         "10 V",
         "100 V",
         "1 kV",
         "10 kV",
         "100 kV",
         /* default */ "1e+07 V"},
        // Frequency
        {"1 Hz",
         "10 Hz",
         "100 Hz",
         "1 kHz",
         "10 kHz",
         "100 kHz",
         "1 MHz",
         "10 MHz",
         "100 MHz",
         "1 GHz",
         "10 GHz",
         "100 GHz",
         "1 THz",
         /* default */ "1e+06 THz"},
        // ThermalConductivity
        {"1 W/m/K",
         "10 W/m/K",
         "100 W/m/K",
         "1 W/mm/K",
         "10 W/mm/K",
         /* default */ "1e+06 W/mm/K"},
        // ElectricalConductivity
        {"1 mS/m",
         "10 mS/m",
         "100 mS/m",
         "1 S/m",
         "10 S/m",
         "100 S/m",
         "1 kS/m",
         "10 kS/m",
         "100 kS/m",
         "1 MS/m",
         /* default */ "1e+06 MS/m"},
        // CurrentDensity
        {"1 A/m^2",
         "10 A/m^2",
         "1 A/mm^2",
         /* default */ "1e+06 A/mm^2"},
        // ElectricalInductance
        {"1 nH",
         "10 nH",
         "100 nH",
         "1 \xC2\xB5H",
         "10 \xC2\xB5H",
         "100 \xC2\xB5H",
         "1 mH",
         "10 mH",
         "100 mH",
         "1 H",
         /* default */ "1e+06 H"},
        // ElectricalCapacitance
        {"1 pF",
         "10 pF",
         "100 pF",
         "1 nF",
         "10 nF",
         "100 nF",
         "1 \xC2\xB5"
         "F",
         "10 \xC2\xB5"
         "F",
         "100 \xC2\xB5"
         "F",
         "1 mF",
         "10 mF",
         "100 mF",
         "1 F",
         /* default */ "1e+06 F"},
    });
}

TEST_F(SchemaTest, sweep_imperial)
{
    UnitsApi::setSchema("Imperial");
    UnitsApi::setDecimals(6);
    sweepCheck({
        // Length
        {"1 thou",
         "10 thou",
         "1\"",
         "10\"",
         "1'",
         "2'",
         "1 yd",
         "10 yd",
         "100 yd",
         "1 mi",
         /* default */ "1e+09 in"},
        // Pressure
        {"1 psi",
         "10 psi",
         "100 psi",
         "1 ksi",
         /* default */ "1e+06 psi"},
    });
}

TEST_F(SchemaTest, sweep_imperial_decimal)
{
    UnitsApi::setSchema("ImperialDecimal");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 in", "10 in", "100 in"},
        {"1 in^2", "10 in^2", "100 in^2"},
        {"1 in^3", "10 in^3"},
        {"1 lb", "10 lb", "100 lb"},
        {"1 psi", "10 psi", "100 psi"},
    });
}

TEST_F(SchemaTest, sweep_imperial_building)
{
    UnitsApi::setSchema("ImperialBuilding");
    UnitsApi::setDecimals(6);
    sweepCheck({
        // Length (toFractional)
        {"1/8\"", "1/4\"", "3/8\"", "1/2\"", "5/8\"", "3/4\"", "7/8\"", "1\"", "6\"", "1'"},
        // Area, Volume
        {"1 sqft", "10 sqft", "100 sqft"},
        {"1 cft", "10 cft", "100 cft"},
    });
}

TEST_F(SchemaTest, sweep_imperial_civil)
{
    UnitsApi::setSchema("ImperialCivil");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 ft", "10 ft", "100 ft"},
        {"1 ft^2", "10 ft^2", "100 ft^2"},
        {"1 ft^3", "10 ft^3"},
        {"1 lb", "10 lb", "100 lb"},
        {"1 psi", "10 psi", "100 psi"},
        {"1 mph", "10 mph", "100 mph"},
        // Angle (toDMS)
        {"1°", "1°30′", "10°", "10°6′36″", "45°", "45°30′", "90°", "180°", "360°"},
    });
}

TEST_F(SchemaTest, sweep_centimeter)
{
    UnitsApi::setSchema("Centimeter");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 cm", "10 cm", "100 cm", "1000 cm"},
        {"1 m^2", "10 m^2", "100 m^2"},
        {"1 m^3", "10 m^3"},
        {"1 W", "10 W", "100 W"},
        {"1 V", "10 V", "100 V"},
    });
}

TEST_F(SchemaTest, sweep_fem)
{
    UnitsApi::setSchema("FEM");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 mm", "10 mm", "100 mm", "1000 mm"},
        {"1 t", "10 t", "100 t"},
    });
}

TEST_F(SchemaTest, sweep_mmmin)
{
    UnitsApi::setSchema("MmMin");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 mm", "10 mm", "100 mm", "1000 mm"},
        {"1 mm/min", "10 mm/min", "100 mm/min"},
    });
}

TEST_F(SchemaTest, sweep_meter_decimal)
{
    UnitsApi::setSchema("MeterDecimal");
    UnitsApi::setDecimals(6);
    sweepCheck({
        {"1 m", "10 m", "100 m", "1000 m"},
        {"1 m^2", "10 m^2", "100 m^2"},
        {"1 m^3", "10 m^3"},
        {"1 W", "10 W", "100 W"},
        {"1 V", "10 V", "100 V"},
        {"1 m/s", "10 m/s", "100 m/s"},
    });
}
