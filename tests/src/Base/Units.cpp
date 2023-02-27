#include "gtest/gtest.h"
#include <boost/core/ignore_unused.hpp>
#include <QLocale>
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Base/UnitsSchemaImperial1.h>

TEST(Unit, TestString)
{
    auto toString = [](const Base::Unit& unit) {
        return unit.getString().toStdString();
    };
    EXPECT_EQ(toString(Base::Unit(0,0,0,0,0,0,0,0)), "");
    EXPECT_EQ(toString(Base::Unit(1,0,0,0,0,0,0,0)), "mm");
    EXPECT_EQ(toString(Base::Unit(0,1,0,0,0,0,0,0)), "kg");
    EXPECT_EQ(toString(Base::Unit(0,0,1,0,0,0,0,0)), "s");
    EXPECT_EQ(toString(Base::Unit(0,0,0,1,0,0,0,0)), "A");
    EXPECT_EQ(toString(Base::Unit(0,0,0,0,1,0,0,0)), "K");
    EXPECT_EQ(toString(Base::Unit(0,0,0,0,0,1,0,0)), "mol");
    EXPECT_EQ(toString(Base::Unit(0,0,0,0,0,0,1,0)), "cd");
    EXPECT_EQ(toString(Base::Unit(0,0,0,0,0,0,0,1)), "deg");
    EXPECT_EQ(toString(Base::Unit(2,0,0,0,0,0,0,0)), "mm^2");
    EXPECT_EQ(toString(Base::Unit(1,1,-2,0,0,0,0,0)), "mm*kg/s^2");
}

TEST(Unit, TestTypeString)
{
    auto toString = [](const Base::Unit& unit) {
        return unit.getTypeString().toStdString();
    };
    EXPECT_EQ(toString(Base::Unit::Acceleration), "Acceleration");
    EXPECT_EQ(toString(Base::Unit::AmountOfSubstance), "AmountOfSubstance");
    EXPECT_EQ(toString(Base::Unit::Angle), "Angle");
    EXPECT_EQ(toString(Base::Unit::AngleOfFriction), "Angle"); // same unit as Angle
    EXPECT_EQ(toString(Base::Unit::Area), "Area");
    EXPECT_EQ(toString(Base::Unit::CurrentDensity), "CurrentDensity");
    EXPECT_EQ(toString(Base::Unit::Density), "Density");
    EXPECT_EQ(toString(Base::Unit::DissipationRate), "DissipationRate");
    EXPECT_EQ(toString(Base::Unit::DynamicViscosity), "DynamicViscosity");
    EXPECT_EQ(toString(Base::Unit::ElectricalCapacitance), "ElectricalCapacitance");
    EXPECT_EQ(toString(Base::Unit::ElectricalConductance), "ElectricalConductance");
    EXPECT_EQ(toString(Base::Unit::ElectricalConductivity), "ElectricalConductivity");
    EXPECT_EQ(toString(Base::Unit::ElectricalInductance), "ElectricalInductance");
    EXPECT_EQ(toString(Base::Unit::ElectricalResistance), "ElectricalResistance");
    EXPECT_EQ(toString(Base::Unit::ElectricCharge), "ElectricCharge");
    EXPECT_EQ(toString(Base::Unit::ElectricCurrent), "ElectricCurrent");
    EXPECT_EQ(toString(Base::Unit::ElectricPotential), "ElectricPotential");
    EXPECT_EQ(toString(Base::Unit::Frequency), "Frequency");
    EXPECT_EQ(toString(Base::Unit::Force), "Force");
    EXPECT_EQ(toString(Base::Unit::HeatFlux), "HeatFlux");
    EXPECT_EQ(toString(Base::Unit::InverseArea), "InverseArea");
    EXPECT_EQ(toString(Base::Unit::InverseLength), "InverseLength");
    EXPECT_EQ(toString(Base::Unit::InverseVolume), "InverseVolume");
    EXPECT_EQ(toString(Base::Unit::KinematicViscosity), "KinematicViscosity");
    EXPECT_EQ(toString(Base::Unit::Length), "Length");
    EXPECT_EQ(toString(Base::Unit::LuminousIntensity), "LuminousIntensity");
    EXPECT_EQ(toString(Base::Unit::MagneticFieldStrength), "MagneticFieldStrength");
    EXPECT_EQ(toString(Base::Unit::MagneticFlux), "MagneticFlux");
    EXPECT_EQ(toString(Base::Unit::MagneticFluxDensity), "MagneticFluxDensity");
    EXPECT_EQ(toString(Base::Unit::Magnetization), "MagneticFieldStrength"); // same as MagneticFieldStrength
    EXPECT_EQ(toString(Base::Unit::Mass), "Mass");
    EXPECT_EQ(toString(Base::Unit::Pressure), "Pressure");
    EXPECT_EQ(toString(Base::Unit::Power), "Power");
    EXPECT_EQ(toString(Base::Unit::ShearModulus), "Pressure"); // same as Pressure
    EXPECT_EQ(toString(Base::Unit::SpecificEnergy), "SpecificEnergy");
    EXPECT_EQ(toString(Base::Unit::SpecificHeat), "SpecificHeat");
    EXPECT_EQ(toString(Base::Unit::Stiffness), "Stiffness");
    EXPECT_EQ(toString(Base::Unit::Stress), "Pressure"); // same as Pressure
    EXPECT_EQ(toString(Base::Unit::Temperature), "Temperature");
    EXPECT_EQ(toString(Base::Unit::ThermalConductivity), "ThermalConductivity");
    EXPECT_EQ(toString(Base::Unit::ThermalExpansionCoefficient), "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Unit::ThermalTransferCoefficient), "ThermalTransferCoefficient");
    EXPECT_EQ(toString(Base::Unit::TimeSpan), "TimeSpan");
    EXPECT_EQ(toString(Base::Unit::UltimateTensileStrength), "Pressure"); // same as Pressure
    EXPECT_EQ(toString(Base::Unit::VacuumPermittivity), "VacuumPermittivity");
    EXPECT_EQ(toString(Base::Unit::Velocity), "Velocity");
    EXPECT_EQ(toString(Base::Unit::Volume), "Volume");
    EXPECT_EQ(toString(Base::Unit::VolumeFlowRate), "VolumeFlowRate");
    EXPECT_EQ(toString(Base::Unit::VolumetricThermalExpansionCoefficient), "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Unit::Work), "Work");
    EXPECT_EQ(toString(Base::Unit::YieldStrength), "Pressure"); // same as Pressure
    EXPECT_EQ(toString(Base::Unit::YoungsModulus), "Pressure"); // same unit as Pressure
}

TEST(Unit, TestEqual)
{
    Base::Unit unit{1};
    EXPECT_EQ(unit.pow(2) == Base::Unit{2}, true);
}

TEST(Unit, TestNotEqual)
{
    Base::Unit unit{1};
    EXPECT_EQ(unit.pow(2) != Base::Unit{1}, true);
}

TEST(Unit, TestMult)
{
    EXPECT_EQ(Base::Unit{} * Base::Unit{}, Base::Unit{});
    EXPECT_EQ(Base::Unit(0, 1) * Base::Unit(1, 0), Base::Unit(1, 1));
}

TEST(Unit, TestDiv)
{
    EXPECT_EQ(Base::Unit{} * Base::Unit{}, Base::Unit{});
    EXPECT_EQ(Base::Unit(0, 1) / Base::Unit(1, 0), Base::Unit(-1, 1));
}

TEST(Unit, TestPowNoDim)
{
    Base::Unit unit{};
    EXPECT_EQ(unit.pow(2), Base::Unit{0});
    EXPECT_EQ(unit.isEmpty(), true);
}

TEST(Unit, TestPowEQ1)
{
    Base::Unit unit{2};
    EXPECT_EQ(unit.pow(1), Base::Unit{2});
}

TEST(Unit, TestPowEQ0)
{
    Base::Unit unit{2};
    EXPECT_EQ(unit.pow(0), Base::Unit{0});
}

TEST(Unit, TestPowGT1)
{
    Base::Unit unit{2};
    EXPECT_EQ(unit.pow(2), Base::Unit{4});
}

TEST(Unit, TestPowLT1)
{
    Base::Unit unit{3};
    EXPECT_EQ(unit.pow(1.0/3.0), Base::Unit{1});
}

TEST(Unit, TestPow3DIV2)
{
    Base::Unit unit{3};
    EXPECT_THROW(unit.pow(3.0/2.0), Base::UnitsMismatchError);
}

TEST(Unit, TestOverflow)
{
    // this tests _that_ the expected exception is thrown
    EXPECT_THROW({
        try
        {
            Base::Unit unit{3};
            unit.pow(10000);
        }
        catch (const Base::OverflowError& e)
        {
            // and this tests that it has the correct message
            EXPECT_STREQ( "Unit overflow in pow()", e.what());
            throw;
        }
    }, Base::OverflowError);
}

TEST(Unit, TestUnderflow)
{
    // this tests _that_ the expected exception is thrown
    EXPECT_THROW({
        try
        {
            Base::Unit unit{3};
            unit.pow(-10000);
        }
        catch (const Base::UnderflowError& e)
        {
            // and this tests that it has the correct message
            EXPECT_STREQ( "Unit underflow in pow()", e.what());
            throw;
        }
    }, Base::UnderflowError);
}

TEST(BaseQuantity, TestValid)
{
    Base::Quantity q1{1.0, Base::Unit::Length};
    Base::Quantity q2{1.0, Base::Unit::Area};
    q2.setInvalid();

    EXPECT_EQ(q1.isValid(), true);
    EXPECT_EQ(q2.isValid(), false);
}

TEST(BaseQuantity, TestDim)
{
    Base::Quantity q1{0, Base::Unit::Area};

    EXPECT_EQ(q1.isQuantity(), true);
}

TEST(BaseQuantity, TestNoDim)
{
    Base::Quantity q1{};

    EXPECT_EQ(q1.isDimensionless(), true);
}

TEST(BaseQuantity, TestString)
{
    Base::Quantity q1{2, QString::fromLatin1("kg*m/s^2")};
    EXPECT_EQ(q1.getUnit(), Base::Unit::Force);

    Base::Quantity q2{2, QString::fromLatin1("kg*m^2/s^2")};
    EXPECT_EQ(q2.getUnit(), Base::Unit::Work);
}

TEST(BaseQuantity, TestCopy)
{
    Base::Quantity q1{1.0, Base::Unit::Length};

    EXPECT_EQ(Base::Quantity{q1}, q1);
}

TEST(BaseQuantity, TestEqual)
{
    Base::Quantity q1{1.0, Base::Unit::Force};
    Base::Quantity q2{1.0, QString::fromLatin1("kg*mm/s^2")};

    EXPECT_EQ(q1 == q1, true);
    EXPECT_EQ(q1 == q2, true);
}

TEST(BaseQuantity, TestNotEqual)
{
    Base::Quantity q1{1.0, Base::Unit::Force};
    Base::Quantity q2{2.0, QString::fromLatin1("kg*m/s^2")};
    Base::Quantity q3{1.0, Base::Unit::Work};

    EXPECT_EQ(q1 != q2, true);
    EXPECT_EQ(q1 != q3, true);
}

TEST(BaseQuantity, TestLessOrGreater)
{
    Base::Quantity q1{1.0, Base::Unit::Force};
    Base::Quantity q2{2.0, QString::fromLatin1("kg*m/s^2")};
    Base::Quantity q3{2.0, Base::Unit::Work};

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
    Base::Quantity q1{1.0, Base::Unit::Length};
    Base::Quantity q2{1.0, Base::Unit::Area};
    EXPECT_THROW(q1 + q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 += q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 + q1, Base::Quantity(2, Base::Unit::Length));
    EXPECT_EQ(q1 += q1, Base::Quantity(2, Base::Unit::Length));
}

TEST(BaseQuantity, TestSub)
{
    Base::Quantity q1{1.0, Base::Unit::Length};
    Base::Quantity q2{1.0, Base::Unit::Area};
    EXPECT_THROW(q1 - q2, Base::UnitsMismatchError);
    EXPECT_THROW(q1 -= q2, Base::UnitsMismatchError);
    EXPECT_EQ(q1 - q1, Base::Quantity(0, Base::Unit::Length));
    EXPECT_EQ(q1 -= q1, Base::Quantity(0, Base::Unit::Length));
}

TEST(BaseQuantity, TestNeg)
{
    Base::Quantity q1{1.0, Base::Unit::Length};
    EXPECT_EQ(-q1, Base::Quantity(-1.0, Base::Unit::Length));
}

TEST(BaseQuantity, TestMult)
{
    Base::Quantity q1{1.0, Base::Unit::Length};
    Base::Quantity q2{1.0, Base::Unit::Area};
    EXPECT_EQ(q1 * q2, Base::Quantity(1.0, Base::Unit::Volume));
    EXPECT_EQ(q1 * 2.0, Base::Quantity(2.0, Base::Unit::Length));
}

TEST(BaseQuantity, TestDiv)
{
    Base::Quantity q1{1.0, Base::Unit::Length};
    Base::Quantity q2{1.0, Base::Unit::Area};
    EXPECT_EQ(q1 / q2, Base::Quantity(1.0, Base::Unit::InverseLength));
    EXPECT_EQ(q1 / 2.0, Base::Quantity(0.5, Base::Unit::Length));
}

TEST(BaseQuantity, TestPow)
{
    Base::Quantity q1{2.0, Base::Unit::Length};
    Base::Quantity q2{2.0, Base::Unit::Area};
    Base::Quantity q3{0.0};
    EXPECT_EQ(q1.pow(q3), Base::Quantity{1});
    EXPECT_EQ(q1.pow(2.0), Base::Quantity(4, Base::Unit::Area));
    EXPECT_THROW(q1.pow(q2), Base::UnitsMismatchError);
}

class Quantity : public ::testing::Test {
protected:
    void SetUp() override {
        QLocale loc(QLocale::C);
        QLocale::setDefault(loc);
    }
    void TearDown() override {
    }
};

TEST_F(Quantity, TestSchemeImperialTwo)
{
    Base::Quantity quantity{1.0, Base::Unit::Length};

    double factor{};
    QString unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    QString result = scheme->schemaTranslate(quantity, factor, unitString);
    EXPECT_EQ(result.toStdString(), "0.04 in");
}

TEST_F(Quantity, TestSchemeImperialOne)
{
    Base::Quantity quantity{1.0, Base::Unit::Length};

    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    double factor{};
    QString unitString;
    auto scheme = Base::UnitsApi::createSchema(Base::UnitSystem::ImperialDecimal);
    QString result = scheme->schemaTranslate(quantity, factor, unitString);

    EXPECT_EQ(result.toStdString(), "0.0 in");
}

TEST_F(Quantity, TestSafeUserString)
{
    Base::UnitsApi::setSchema(Base::UnitSystem::ImperialDecimal);

    Base::Quantity quantity{1.0, Base::Unit::Length};
    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    QString result = quantity.getSafeUserString();

    EXPECT_EQ(result.toStdString(), "1 mm");
}
