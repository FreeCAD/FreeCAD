#include "Base/Unit.h"
#include "gtest/gtest.h"
#include <Base/Exception.h>

// NOLINTBEGIN
TEST(Unit, TestString)
{
    auto toString = [](const Base::Unit& unit) {
        return unit.getString().toStdString();
    };
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 0, 0, 0, 0, 0)), "");
    EXPECT_EQ(toString(Base::Unit(1, 0, 0, 0, 0, 0, 0, 0)), "mm");
    EXPECT_EQ(toString(Base::Unit(0, 1, 0, 0, 0, 0, 0, 0)), "kg");
    EXPECT_EQ(toString(Base::Unit(0, 0, 1, 0, 0, 0, 0, 0)), "s");
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 1, 0, 0, 0, 0)), "A");
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 0, 1, 0, 0, 0)), "K");
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 0, 0, 1, 0, 0)), "mol");
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 0, 0, 0, 1, 0)), "cd");
    EXPECT_EQ(toString(Base::Unit(0, 0, 0, 0, 0, 0, 0, 1)), "deg");
    EXPECT_EQ(toString(Base::Unit(2, 0, 0, 0, 0, 0, 0, 0)), "mm^2");
    EXPECT_EQ(toString(Base::Unit(1, 1, -2, 0, 0, 0, 0, 0)), "mm*kg/s^2");
}

TEST(Unit, TestTypeString)
{
    auto toString = [](const Base::Unit& unit) {
        return unit.getTypeString().toStdString();
    };
    EXPECT_EQ(toString(Base::Unit::Acceleration), "Acceleration");
    EXPECT_EQ(toString(Base::Unit::AmountOfSubstance), "AmountOfSubstance");
    EXPECT_EQ(toString(Base::Unit::Angle), "Angle");
    EXPECT_EQ(toString(Base::Unit::AngleOfFriction), "Angle");  // same unit as Angle
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
    EXPECT_EQ(toString(Base::Unit::Magnetization),
              "MagneticFieldStrength");  // same as MagneticFieldStrength
    EXPECT_EQ(toString(Base::Unit::Mass), "Mass");
    EXPECT_EQ(toString(Base::Unit::Pressure), "Pressure");
    EXPECT_EQ(toString(Base::Unit::Power), "Power");
    EXPECT_EQ(toString(Base::Unit::ShearModulus), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Unit::SpecificEnergy), "SpecificEnergy");
    EXPECT_EQ(toString(Base::Unit::SpecificHeat), "SpecificHeat");
    EXPECT_EQ(toString(Base::Unit::Stiffness), "Stiffness");
    EXPECT_EQ(toString(Base::Unit::Stress), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Unit::Temperature), "Temperature");
    EXPECT_EQ(toString(Base::Unit::ThermalConductivity), "ThermalConductivity");
    EXPECT_EQ(toString(Base::Unit::ThermalExpansionCoefficient), "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Unit::ThermalTransferCoefficient), "ThermalTransferCoefficient");
    EXPECT_EQ(toString(Base::Unit::TimeSpan), "TimeSpan");
    EXPECT_EQ(toString(Base::Unit::UltimateTensileStrength), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Unit::VacuumPermittivity), "VacuumPermittivity");
    EXPECT_EQ(toString(Base::Unit::Velocity), "Velocity");
    EXPECT_EQ(toString(Base::Unit::Volume), "Volume");
    EXPECT_EQ(toString(Base::Unit::VolumeFlowRate), "VolumeFlowRate");
    EXPECT_EQ(toString(Base::Unit::VolumetricThermalExpansionCoefficient),
              "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Unit::Work), "Work");
    EXPECT_EQ(toString(Base::Unit::YieldStrength), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Unit::YoungsModulus), "Pressure");  // same unit as Pressure
}
TEST(Unit, strings)
{
    EXPECT_STREQ(Base::Unit::Acceleration.getString().toStdString().c_str(), "mm/s^2");
    EXPECT_STREQ(Base::Unit::AmountOfSubstance.getString().toStdString().c_str(), "mol");
    EXPECT_STREQ(Base::Unit::Angle.getString().toStdString().c_str(), "deg");
    EXPECT_STREQ(Base::Unit::AngleOfFriction.getString().toStdString().c_str(), "deg");
    EXPECT_STREQ(Base::Unit::Area.getString().toStdString().c_str(), "mm^2");
    EXPECT_STREQ(Base::Unit::CurrentDensity.getString().toStdString().c_str(), "A/mm^2");
    EXPECT_STREQ(Base::Unit::Density.getString().toStdString().c_str(), "kg/mm^3");
    EXPECT_STREQ(Base::Unit::DissipationRate.getString().toStdString().c_str(), "mm^2/s^3");
}

TEST(Unit, TestEqual)
{
    Base::Unit unit {1};
    EXPECT_EQ(unit.pow(2) == Base::Unit {2}, true);
}

TEST(Unit, TestNotEqual)
{
    Base::Unit unit {1};
    EXPECT_EQ(unit.pow(2) != Base::Unit {1}, true);
}

TEST(Unit, TestMult)
{
    EXPECT_EQ(Base::Unit {} * Base::Unit {}, Base::Unit {});
    EXPECT_EQ(Base::Unit(0, 1) * Base::Unit(1, 0), Base::Unit(1, 1));
}

TEST(Unit, TestDiv)
{
    EXPECT_EQ(Base::Unit {} * Base::Unit {}, Base::Unit {});
    EXPECT_EQ(Base::Unit(0, 1) / Base::Unit(1, 0), Base::Unit(-1, 1));
}

TEST(Unit, TestPowNoDim)
{
    Base::Unit unit {};
    EXPECT_EQ(unit.pow(2), Base::Unit {0});
    EXPECT_EQ(unit.isEmpty(), true);
}

TEST(Unit, TestPowEQ1)
{
    Base::Unit unit {2};
    EXPECT_EQ(unit.pow(1), Base::Unit {2});
}

TEST(Unit, TestPowEQ0)
{
    Base::Unit unit {2};
    EXPECT_EQ(unit.pow(0), Base::Unit {0});
}

TEST(Unit, TestPowGT1)
{
    Base::Unit unit {2};
    EXPECT_EQ(unit.pow(2), Base::Unit {4});
}

TEST(Unit, TestPowLT1)
{
    Base::Unit unit {3};
    EXPECT_EQ(unit.pow(1.0 / 3.0), Base::Unit {1});
}

TEST(Unit, TestPow3DIV2)
{
    Base::Unit unit {3};
    EXPECT_THROW(unit.pow(3.0 / 2.0), Base::UnitsMismatchError);
}

TEST(Unit, TestOverflow)
{
    // this tests _that_ the expected exception is thrown
    EXPECT_THROW(
        {
            try {
                Base::Unit unit {3};
                unit.pow(10000);
            }
            catch (const Base::OverflowError& e) {
                // and this tests that it has the correct message
                EXPECT_STREQ("Unit overflow in pow()", e.what());
                throw;
            }
        },
        Base::OverflowError);
}

TEST(Unit, TestUnderflow)
{
    // this tests _that_ the expected exception is thrown
    EXPECT_THROW(
        {
            try {
                Base::Unit unit {3};
                unit.pow(-10000);
            }
            catch (const Base::UnderflowError& e) {
                // and this tests that it has the correct message
                EXPECT_STREQ("Unit underflow in pow()", e.what());
                throw;
            }
        },
        Base::UnderflowError);
}

// NOLINTEND
