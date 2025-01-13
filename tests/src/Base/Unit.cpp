#include "Base/Unit.h"
#include <gtest/gtest.h>
#include <Base/Exception.h>

// NOLINTBEGIN
TEST(Unit, TestString)
{
    auto toString = [](const Base::Unit& unit) {
        return unit.getString();
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
        return unit.getTypeString();
    };
    EXPECT_EQ(toString(Base::Units::Acceleration), "Acceleration");
    EXPECT_EQ(toString(Base::Units::AmountOfSubstance), "AmountOfSubstance");
    EXPECT_EQ(toString(Base::Units::Angle), "Angle");
    EXPECT_EQ(toString(Base::Units::AngleOfFriction), "Angle");  // same unit as Angle
    EXPECT_EQ(toString(Base::Units::Area), "Area");
    EXPECT_EQ(toString(Base::Units::CurrentDensity), "CurrentDensity");
    EXPECT_EQ(toString(Base::Units::Density), "Density");
    EXPECT_EQ(toString(Base::Units::DissipationRate), "DissipationRate");
    EXPECT_EQ(toString(Base::Units::DynamicViscosity), "DynamicViscosity");
    EXPECT_EQ(toString(Base::Units::ElectricalCapacitance), "ElectricalCapacitance");
    EXPECT_EQ(toString(Base::Units::ElectricalConductance), "ElectricalConductance");
    EXPECT_EQ(toString(Base::Units::ElectricalConductivity), "ElectricalConductivity");
    EXPECT_EQ(toString(Base::Units::ElectricalInductance), "ElectricalInductance");
    EXPECT_EQ(toString(Base::Units::ElectricalResistance), "ElectricalResistance");
    EXPECT_EQ(toString(Base::Units::ElectricCharge), "ElectricCharge");
    EXPECT_EQ(toString(Base::Units::ElectricCurrent), "ElectricCurrent");
    EXPECT_EQ(toString(Base::Units::ElectricPotential), "ElectricPotential");
    EXPECT_EQ(toString(Base::Units::Frequency), "Frequency");
    EXPECT_EQ(toString(Base::Units::Force), "Force");
    EXPECT_EQ(toString(Base::Units::HeatFlux), "HeatFlux");
    EXPECT_EQ(toString(Base::Units::InverseArea), "InverseArea");
    EXPECT_EQ(toString(Base::Units::InverseLength), "InverseLength");
    EXPECT_EQ(toString(Base::Units::InverseVolume), "InverseVolume");
    EXPECT_EQ(toString(Base::Units::KinematicViscosity), "KinematicViscosity");
    EXPECT_EQ(toString(Base::Units::Length), "Length");
    EXPECT_EQ(toString(Base::Units::LuminousIntensity), "LuminousIntensity");
    EXPECT_EQ(toString(Base::Units::MagneticFieldStrength), "MagneticFieldStrength");
    EXPECT_EQ(toString(Base::Units::MagneticFlux), "MagneticFlux");
    EXPECT_EQ(toString(Base::Units::MagneticFluxDensity), "MagneticFluxDensity");
    EXPECT_EQ(toString(Base::Units::Magnetization),
              "MagneticFieldStrength");  // same as MagneticFieldStrength
    EXPECT_EQ(toString(Base::Units::Mass), "Mass");
    EXPECT_EQ(toString(Base::Units::Pressure), "Pressure");
    EXPECT_EQ(toString(Base::Units::Power), "Power");
    EXPECT_EQ(toString(Base::Units::ShearModulus), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Units::SpecificEnergy), "SpecificEnergy");
    EXPECT_EQ(toString(Base::Units::SpecificHeat), "SpecificHeat");
    EXPECT_EQ(toString(Base::Units::Stiffness), "Stiffness");
    EXPECT_EQ(toString(Base::Units::Stress), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Units::Temperature), "Temperature");
    EXPECT_EQ(toString(Base::Units::ThermalConductivity), "ThermalConductivity");
    EXPECT_EQ(toString(Base::Units::ThermalExpansionCoefficient), "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Units::ThermalTransferCoefficient), "ThermalTransferCoefficient");
    EXPECT_EQ(toString(Base::Units::TimeSpan), "TimeSpan");
    EXPECT_EQ(toString(Base::Units::UltimateTensileStrength), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Units::VacuumPermittivity), "VacuumPermittivity");
    EXPECT_EQ(toString(Base::Units::Velocity), "Velocity");
    EXPECT_EQ(toString(Base::Units::Volume), "Volume");
    EXPECT_EQ(toString(Base::Units::VolumeFlowRate), "VolumeFlowRate");
    EXPECT_EQ(toString(Base::Units::VolumetricThermalExpansionCoefficient),
              "ThermalExpansionCoefficient");
    EXPECT_EQ(toString(Base::Units::Work), "Work");
    EXPECT_EQ(toString(Base::Units::YieldStrength), "Pressure");  // same as Pressure
    EXPECT_EQ(toString(Base::Units::YoungsModulus), "Pressure");  // same unit as Pressure
}
TEST(Unit, strings)
{
    EXPECT_STREQ(Base::Units::Acceleration.getString().c_str(), "mm/s^2");
    EXPECT_STREQ(Base::Units::AmountOfSubstance.getString().c_str(), "mol");
    EXPECT_STREQ(Base::Units::Angle.getString().c_str(), "deg");
    EXPECT_STREQ(Base::Units::AngleOfFriction.getString().c_str(), "deg");
    EXPECT_STREQ(Base::Units::Area.getString().c_str(), "mm^2");
    EXPECT_STREQ(Base::Units::CurrentDensity.getString().c_str(), "A/mm^2");
    EXPECT_STREQ(Base::Units::Density.getString().c_str(), "kg/mm^3");
    EXPECT_STREQ(Base::Units::DissipationRate.getString().c_str(), "mm^2/s^3");
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
