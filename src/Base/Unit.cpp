/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <cmath>
# include <limits>
# include <sstream>
#endif

#include "Unit.h"
#include "Exception.h"
#include "Quantity.h"


using namespace Base;

static inline void checkPow(UnitSignature sig, double exp)
{
    auto isInt = [](double value) {
        return std::fabs(std::round(value) - value) < std::numeric_limits<double>::epsilon();
    };
    if (!isInt(sig.Length * exp) ||
        !isInt(sig.Mass * exp) ||
        !isInt(sig.Time * exp) ||
        !isInt(sig.ElectricCurrent * exp) ||
        !isInt(sig.ThermodynamicTemperature * exp) ||
        !isInt(sig.AmountOfSubstance * exp) ||
        !isInt(sig.LuminousIntensity * exp) ||
        !isInt(sig.Angle * exp)) {
        throw Base::UnitsMismatchError("pow() of unit not possible");
    }
}

static inline void checkRange(const char * op, int length, int mass, int time, int electricCurrent,
                              int thermodynamicTemperature, int amountOfSubstance, int luminousIntensity, int angle)
{
    if ( ( length                   >=  (1 << (UnitSignatureLengthBits                   - 1)) ) ||
         ( mass                     >=  (1 << (UnitSignatureMassBits                     - 1)) ) ||
         ( time                     >=  (1 << (UnitSignatureTimeBits                     - 1)) ) ||
         ( electricCurrent          >=  (1 << (UnitSignatureElectricCurrentBits          - 1)) ) ||
         ( thermodynamicTemperature >=  (1 << (UnitSignatureThermodynamicTemperatureBits - 1)) ) ||
         ( amountOfSubstance        >=  (1 << (UnitSignatureAmountOfSubstanceBits        - 1)) ) ||
         ( luminousIntensity        >=  (1 << (UnitSignatureLuminousIntensityBits        - 1)) ) ||
         ( angle                    >=  (1 << (UnitSignatureAngleBits                    - 1)) ) )
        throw Base::OverflowError((std::string("Unit overflow in ") + std::string(op)).c_str());
    if ( ( length                   <  -(1 << (UnitSignatureLengthBits                   - 1)) ) ||
         ( mass                     <  -(1 << (UnitSignatureMassBits                     - 1)) ) ||
         ( time                     <  -(1 << (UnitSignatureTimeBits                     - 1)) ) ||
         ( electricCurrent          <  -(1 << (UnitSignatureElectricCurrentBits          - 1)) ) ||
         ( thermodynamicTemperature <  -(1 << (UnitSignatureThermodynamicTemperatureBits - 1)) ) ||
         ( amountOfSubstance        <  -(1 << (UnitSignatureAmountOfSubstanceBits        - 1)) ) ||
         ( luminousIntensity        <  -(1 << (UnitSignatureLuminousIntensityBits        - 1)) ) ||
         ( angle                    <  -(1 << (UnitSignatureAngleBits                    - 1)) ) )
        throw Base::UnderflowError((std::string("Unit underflow in ") + std::string(op)).c_str());
}

Unit::Unit(int8_t Length,
           int8_t Mass,
           int8_t Time,
           int8_t ElectricCurrent,
           int8_t ThermodynamicTemperature,
           int8_t AmountOfSubstance,
           int8_t LuminousIntensity,
           int8_t Angle)
{
    checkRange("unit",
               Length,
               Mass,
               Time,
               ElectricCurrent,
               ThermodynamicTemperature,
               AmountOfSubstance,
               LuminousIntensity,
               Angle);

    Sig.Length                   = Length;
    Sig.Mass                     = Mass;
    Sig.Time                     = Time;
    Sig.ElectricCurrent          = ElectricCurrent;
    Sig.ThermodynamicTemperature = ThermodynamicTemperature;
    Sig.AmountOfSubstance        = AmountOfSubstance;
    Sig.LuminousIntensity        = LuminousIntensity;
    Sig.Angle                    = Angle;
}


Unit::Unit()
{
    Sig.Length                   = 0;
    Sig.Mass                     = 0;
    Sig.Time                     = 0;
    Sig.ElectricCurrent          = 0;
    Sig.ThermodynamicTemperature = 0;
    Sig.AmountOfSubstance        = 0;
    Sig.LuminousIntensity        = 0;
    Sig.Angle                    = 0;
}

Unit::Unit(const Unit& that)
{
    this->Sig = that.Sig;
}

Unit::Unit(const QString& expr)
{
    try {
        *this = Quantity::parse(expr).getUnit();
    }
    catch (const Base::ParserError&) {
        Sig.Length                   = 0;
        Sig.Mass                     = 0;
        Sig.Time                     = 0;
        Sig.ElectricCurrent          = 0;
        Sig.ThermodynamicTemperature = 0;
        Sig.AmountOfSubstance        = 0;
        Sig.LuminousIntensity        = 0;
        Sig.Angle                    = 0;
    }
}

Unit Unit::pow(double exp) const
{
    checkPow(Sig, exp);
    checkRange("pow()",
               static_cast<int>(Sig.Length * exp),
               static_cast<int>(Sig.Mass * exp),
               static_cast<int>(Sig.Time * exp),
               static_cast<int>(Sig.ElectricCurrent * exp),
               static_cast<int>(Sig.ThermodynamicTemperature * exp),
               static_cast<int>(Sig.AmountOfSubstance * exp),
               static_cast<int>(Sig.LuminousIntensity * exp),
               static_cast<int>(Sig.Angle * exp));

    Unit result;
    result.Sig.Length                   = static_cast<int8_t>(Sig.Length                    * exp);
    result.Sig.Mass                     = static_cast<int8_t>(Sig.Mass                      * exp);
    result.Sig.Time                     = static_cast<int8_t>(Sig.Time                      * exp);
    result.Sig.ElectricCurrent          = static_cast<int8_t>(Sig.ElectricCurrent           * exp);
    result.Sig.ThermodynamicTemperature = static_cast<int8_t>(Sig.ThermodynamicTemperature  * exp);
    result.Sig.AmountOfSubstance        = static_cast<int8_t>(Sig.AmountOfSubstance         * exp);
    result.Sig.LuminousIntensity        = static_cast<int8_t>(Sig.LuminousIntensity         * exp);
    result.Sig.Angle                    = static_cast<int8_t>(Sig.Angle                     * exp);

    return result;
}

bool Unit::isEmpty()const
{
    return (this->Sig.Length == 0)
        && (this->Sig.Mass == 0)
        && (this->Sig.Time == 0)
        && (this->Sig.ElectricCurrent == 0)
        && (this->Sig.ThermodynamicTemperature == 0)
        && (this->Sig.AmountOfSubstance == 0)
        && (this->Sig.LuminousIntensity == 0)
        && (this->Sig.Angle == 0);
}

bool Unit::operator ==(const Unit& that) const
{
    return (this->Sig.Length == that.Sig.Length)
        && (this->Sig.Mass == that.Sig.Mass)
        && (this->Sig.Time == that.Sig.Time)
        && (this->Sig.ElectricCurrent == that.Sig.ElectricCurrent)
        && (this->Sig.ThermodynamicTemperature == that.Sig.ThermodynamicTemperature)
        && (this->Sig.AmountOfSubstance == that.Sig.AmountOfSubstance)
        && (this->Sig.LuminousIntensity == that.Sig.LuminousIntensity)
        && (this->Sig.Angle == that.Sig.Angle);
}


Unit Unit::operator *(const Unit &right) const
{
    checkRange("* operator",
               Sig.Length +right.Sig.Length,
               Sig.Mass + right.Sig.Mass,
               Sig.Time + right.Sig.Time,
               Sig.ElectricCurrent + right.Sig.ElectricCurrent,
               Sig.ThermodynamicTemperature + right.Sig.ThermodynamicTemperature,
               Sig.AmountOfSubstance + right.Sig.AmountOfSubstance,
               Sig.LuminousIntensity + right.Sig.LuminousIntensity,
               Sig.Angle + right.Sig.Angle);

    Unit result;
    result.Sig.Length                   = Sig.Length                    + right.Sig.Length;
    result.Sig.Mass                     = Sig.Mass                      + right.Sig.Mass;
    result.Sig.Time                     = Sig.Time                      + right.Sig.Time;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           + right.Sig.ElectricCurrent;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  + right.Sig.ThermodynamicTemperature;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         + right.Sig.AmountOfSubstance;
    result.Sig.LuminousIntensity        = Sig.LuminousIntensity         + right.Sig.LuminousIntensity;
    result.Sig.Angle                    = Sig.Angle                     + right.Sig.Angle;

    return result;
}

Unit Unit::operator /(const Unit &right) const
{
    checkRange("/ operator",
               Sig.Length - right.Sig.Length,
               Sig.Mass - right.Sig.Mass,
               Sig.Time - right.Sig.Time,
               Sig.ElectricCurrent - right.Sig.ElectricCurrent,
               Sig.ThermodynamicTemperature - right.Sig.ThermodynamicTemperature,
               Sig.AmountOfSubstance - right.Sig.AmountOfSubstance,
               Sig.LuminousIntensity - right.Sig.LuminousIntensity,
               Sig.Angle - right.Sig.Angle);

    Unit result;
    result.Sig.Length                   = Sig.Length                    - right.Sig.Length;
    result.Sig.Mass                     = Sig.Mass                      - right.Sig.Mass;
    result.Sig.Time                     = Sig.Time                      - right.Sig.Time;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           - right.Sig.ElectricCurrent;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  - right.Sig.ThermodynamicTemperature;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         - right.Sig.AmountOfSubstance;
    result.Sig.LuminousIntensity        = Sig.LuminousIntensity         - right.Sig.LuminousIntensity;
    result.Sig.Angle                    = Sig.Angle                     - right.Sig.Angle;

    return result;
}

Unit& Unit::operator = (const Unit &New)
{
    Sig.Length                   = New.Sig.Length;
    Sig.Mass                     = New.Sig.Mass;
    Sig.Time                     = New.Sig.Time;
    Sig.ElectricCurrent          = New.Sig.ElectricCurrent;
    Sig.ThermodynamicTemperature = New.Sig.ThermodynamicTemperature;
    Sig.AmountOfSubstance        = New.Sig.AmountOfSubstance;
    Sig.LuminousIntensity        = New.Sig.LuminousIntensity;
    Sig.Angle                    = New.Sig.Angle;

    return *this;
}

QString Unit::getString() const
{
    std::stringstream ret;

    if (isEmpty())
        return {};

    if (Sig.Length                  > 0 ||
        Sig.Mass                    > 0 ||
        Sig.Time                    > 0 ||
        Sig.ElectricCurrent         > 0 ||
        Sig.ThermodynamicTemperature> 0 ||
        Sig.AmountOfSubstance       > 0 ||
        Sig.LuminousIntensity       > 0 ||
        Sig.Angle                   > 0 ){

        bool mult = false;
        if (Sig.Length > 0) {
            mult = true;
            ret << "mm";
            if (Sig.Length > 1)
                ret << "^" << Sig.Length;
        }

        if (Sig.Mass > 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "kg";
            if (Sig.Mass > 1)
                ret << "^" << Sig.Mass;
        }

        if (Sig.Time > 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "s";
            if (Sig.Time > 1)
                ret << "^" << Sig.Time;
        }

        if (Sig.ElectricCurrent > 0) {
            if (mult) ret<<'*';
                mult = true;
            ret << "A";
            if (Sig.ElectricCurrent > 1)
                ret << "^" << Sig.ElectricCurrent;
        }

        if (Sig.ThermodynamicTemperature > 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "K";
            if (Sig.ThermodynamicTemperature > 1)
                ret << "^" << Sig.ThermodynamicTemperature;
        }

        if (Sig.AmountOfSubstance > 0){
            if (mult)
                ret<<'*';
            mult = true;
            ret << "mol";
            if (Sig.AmountOfSubstance > 1)
                ret << "^" << Sig.AmountOfSubstance;
        }

        if (Sig.LuminousIntensity > 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "cd";
            if (Sig.LuminousIntensity > 1)
                ret << "^" << Sig.LuminousIntensity;
        }

        if (Sig.Angle > 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "deg";
            if (Sig.Angle > 1)
                ret << "^" << Sig.Angle;
        }
    }
    else {
        ret << "1";
    }

    if (Sig.Length                  < 0 ||
        Sig.Mass                    < 0 ||
        Sig.Time                    < 0 ||
        Sig.ElectricCurrent         < 0 ||
        Sig.ThermodynamicTemperature< 0 ||
        Sig.AmountOfSubstance       < 0 ||
        Sig.LuminousIntensity       < 0 ||
        Sig.Angle                   < 0 ){
        ret << "/";

        int nnom = 0;
        nnom += Sig.Length<0?1:0;
        nnom += Sig.Mass<0?1:0;
        nnom += Sig.Time<0?1:0;
        nnom += Sig.ElectricCurrent<0?1:0;
        nnom += Sig.ThermodynamicTemperature<0?1:0;
        nnom += Sig.AmountOfSubstance<0?1:0;
        nnom += Sig.LuminousIntensity<0?1:0;
        nnom += Sig.Angle<0?1:0;

        if (nnom > 1)
            ret << '(';

        bool mult=false;
        if (Sig.Length < 0) {
            ret << "mm";
            mult = true;
            if (Sig.Length < -1)
                ret << "^" << abs(Sig.Length);
        }

        if (Sig.Mass < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "kg";
            if (Sig.Mass < -1)
                ret << "^" << abs(Sig.Mass);
        }

        if (Sig.Time < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "s";
            if (Sig.Time < -1)
                ret << "^" << abs(Sig.Time);
        }

        if (Sig.ElectricCurrent < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "A";
            if (Sig.ElectricCurrent < -1)
                ret << "^" << abs(Sig.ElectricCurrent);
        }

        if (Sig.ThermodynamicTemperature < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "K";
            if (Sig.ThermodynamicTemperature < -1)
                ret << "^" << abs(Sig.ThermodynamicTemperature);
        }

        if (Sig.AmountOfSubstance < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "mol";
            if (Sig.AmountOfSubstance < -1)
                ret << "^" << abs(Sig.AmountOfSubstance);
        }

        if (Sig.LuminousIntensity < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "cd";
            if (Sig.LuminousIntensity < -1)
                ret << "^" << abs(Sig.LuminousIntensity);
        }

        if (Sig.Angle < 0) {
            if (mult)
                ret<<'*';
            mult = true;
            ret << "deg";
            if (Sig.Angle < -1)
                ret << "^" << abs(Sig.Angle);
        }

        if (nnom > 1)
            ret << ')';
    }

    return QString::fromUtf8(ret.str().c_str());
}

QString Unit::getTypeString() const
{
    if (*this == Unit::Acceleration)
        return QString::fromLatin1("Acceleration");
    if (*this == Unit::AmountOfSubstance)
        return QString::fromLatin1("AmountOfSubstance");
    if (*this == Unit::Angle)
        return QString::fromLatin1("Angle");
    if (*this == Unit::AngleOfFriction)
        return QString::fromLatin1("AngleOfFriction");
    if (*this == Unit::Area)
        return QString::fromLatin1("Area");
    if (*this == Unit::CurrentDensity)
        return QString::fromLatin1("CurrentDensity");
    if (*this == Unit::Density)
        return QString::fromLatin1("Density");
    if (*this == Unit::DissipationRate)
        return QString::fromLatin1("DissipationRate");
    if (*this == Unit::DynamicViscosity)
        return QString::fromLatin1("DynamicViscosity");
    if (*this == Unit::ElectricalCapacitance)
        return QString::fromLatin1("ElectricalCapacitance");
    if (*this == Unit::ElectricalConductance)
        return QString::fromLatin1("ElectricalConductance");
    if (*this == Unit::ElectricalConductivity)
        return QString::fromLatin1("ElectricalConductivity");
    if (*this == Unit::ElectricalInductance)
        return QString::fromLatin1("ElectricalInductance");
    if (*this == Unit::ElectricalResistance)
        return QString::fromLatin1("ElectricalResistance");
    if (*this == Unit::ElectricCharge)
        return QString::fromLatin1("ElectricCharge");
    if (*this == Unit::ElectricCurrent)
        return QString::fromLatin1("ElectricCurrent");
    if (*this == Unit::ElectricPotential)
        return QString::fromLatin1("ElectricPotential");
    if (*this == Unit::Frequency)
        return QString::fromLatin1("Frequency");
    if (*this == Unit::Force)
        return QString::fromLatin1("Force");
    if (*this == Unit::HeatFlux)
        return QString::fromLatin1("HeatFlux");
    if (*this == Unit::InverseArea)
        return QString::fromLatin1("InverseArea");
    if (*this == Unit::InverseLength)
        return QString::fromLatin1("InverseLength");
    if (*this == Unit::InverseVolume)
        return QString::fromLatin1("InverseVolume");
    if (*this == Unit::KinematicViscosity)
        return QString::fromLatin1("KinematicViscosity");
    if (*this == Unit::Length)
        return QString::fromLatin1("Length");
    if (*this == Unit::LuminousIntensity)
        return QString::fromLatin1("LuminousIntensity");
    if (*this == Unit::MagneticFieldStrength)
        return QString::fromLatin1("MagneticFieldStrength");
    if (*this == Unit::MagneticFlux)
        return QString::fromLatin1("MagneticFlux");
    if (*this == Unit::MagneticFluxDensity)
        return QString::fromLatin1("MagneticFluxDensity");
    if (*this == Unit::Magnetization)
        return QString::fromLatin1("Magnetization");
    if (*this == Unit::Mass)
        return QString::fromLatin1("Mass");
    if (*this == Unit::Pressure)
        return QString::fromLatin1("Pressure");
    if (*this == Unit::Power)
        return QString::fromLatin1("Power");
    if (*this == Unit::ShearModulus)
        return QString::fromLatin1("ShearModulus");
    if (*this == Unit::SpecificEnergy)
        return QString::fromLatin1("SpecificEnergy");
    if (*this == Unit::SpecificHeat)
        return QString::fromLatin1("SpecificHeat");
    if (*this == Unit::Stiffness)
        return QString::fromLatin1("Stiffness");
    if (*this == Unit::Stress)
        return QString::fromLatin1("Stress");
    if (*this == Unit::Temperature)
        return QString::fromLatin1("Temperature");
    if (*this == Unit::ThermalConductivity)
        return QString::fromLatin1("ThermalConductivity");
    if (*this == Unit::ThermalExpansionCoefficient)
        return QString::fromLatin1("ThermalExpansionCoefficient");
    if (*this == Unit::ThermalTransferCoefficient)
        return QString::fromLatin1("ThermalTransferCoefficient");
    if (*this == Unit::TimeSpan)
        return QString::fromLatin1("TimeSpan");
    if (*this == Unit::UltimateTensileStrength)
        return QString::fromLatin1("UltimateTensileStrength");
    if (*this == Unit::VacuumPermittivity)
        return QString::fromLatin1("VacuumPermittivity");
    if (*this == Unit::Velocity)
        return QString::fromLatin1("Velocity");
    if (*this == Unit::Volume)
        return QString::fromLatin1("Volume");
    if (*this == Unit::VolumeFlowRate)
        return QString::fromLatin1("VolumeFlowRate");
    if (*this == Unit::VolumetricThermalExpansionCoefficient)
        return QString::fromLatin1("VolumetricThermalExpansionCoefficient");
    if (*this == Unit::Work)
        return QString::fromLatin1("Work");
    if (*this == Unit::YieldStrength)
        return QString::fromLatin1("YieldStrength");
    if (*this == Unit::YoungsModulus)
        return QString::fromLatin1("YoungsModulus");

    return {};
}

// SI base units
Unit Unit::AmountOfSubstance          (0, 0, 0, 0, 0, 1);
Unit Unit::ElectricCurrent            (0, 0, 0, 1);
Unit Unit::Length                     (1);
Unit Unit::LuminousIntensity          (0, 0, 0, 0, 0, 0, 1);
Unit Unit::Mass                       (0, 1);
Unit Unit::Temperature                (0, 0, 0, 0, 1);
Unit Unit::TimeSpan                   (0, 0, 1);

// all other units
Unit Unit::Acceleration               (1, 0, -2);
Unit Unit::Angle                      (0, 0, 0, 0, 0, 0, 0, 1);
Unit Unit::AngleOfFriction            (0, 0, 0, 0, 0, 0, 0, 1);
Unit Unit::Area                       (2);
Unit Unit::CompressiveStrength        (-1, 1, -2);
Unit Unit::CurrentDensity             (-2, 0, 0, 1);
Unit Unit::Density                    (-3, 1);
Unit Unit::DissipationRate   (2, 0, -3); // https://cfd-online.com/Wiki/Turbulence_dissipation_rate
Unit Unit::DynamicViscosity           (-1, 1, -1);
Unit Unit::ElectricalCapacitance      (-2, -1, 4, 2);
Unit Unit::ElectricalConductance      (-2, -1, 3, 2);
Unit Unit::ElectricalConductivity     (-3, -1, 3, 2);
Unit Unit::ElectricalInductance       (2, 1, -2, -2);
Unit Unit::ElectricalResistance       (2, 1, -3, -2);
Unit Unit::ElectricCharge             (0, 0, 1, 1);
Unit Unit::ElectricPotential          (2, 1, -3, -1);
Unit Unit::Force                      (1, 1, -2);
Unit Unit::Frequency                  (0, 0, -1);
Unit Unit::HeatFlux                   (0, 1, -3, 0, 0);
Unit Unit::InverseArea                (-2, 0, 0);
Unit Unit::InverseLength              (-1, 0, 0);
Unit Unit::InverseVolume              (-3, 0, 0);
Unit Unit::KinematicViscosity         (2, 0, -1);
Unit Unit::MagneticFieldStrength      (-1,0,0,1);
Unit Unit::MagneticFlux               (2,1,-2,-1);
Unit Unit::MagneticFluxDensity        (0,1,-2,-1);
Unit Unit::Magnetization              (-1,0,0,1);
Unit Unit::Pressure                   (-1,1,-2);
Unit Unit::Power                      (2, 1, -3);
Unit Unit::ShearModulus               (-1,1,-2);
Unit Unit::SpecificEnergy             (2, 0, -2);
Unit Unit::SpecificHeat               (2, 0, -2, 0, -1);
Unit Unit::Stiffness                  (0, 1, -2);
Unit Unit::Stress                     (-1,1,-2);
Unit Unit::ThermalConductivity        (1, 1, -3, 0, -1);
Unit Unit::ThermalExpansionCoefficient(0, 0, 0, 0, -1);
Unit Unit::ThermalTransferCoefficient (0, 1, -3, 0, -1);
Unit Unit::UltimateTensileStrength    (-1,1,-2);
Unit Unit::VacuumPermittivity         (-3, -1, 4,  2);
Unit Unit::Velocity                   (1, 0, -1);
Unit Unit::Volume                     (3);
Unit Unit::VolumeFlowRate             (3, 0, -1);
Unit Unit::VolumetricThermalExpansionCoefficient(0, 0, 0, 0, -1);
Unit Unit::Work                       (2, 1, -2);
Unit Unit::YieldStrength              (-1,1,-2);
Unit Unit::YoungsModulus              (-1,1,-2);
