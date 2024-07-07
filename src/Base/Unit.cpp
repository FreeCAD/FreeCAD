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
#include <cmath>
#include <limits>
#include <sstream>
#endif

#include "Unit.h"
#include "Exception.h"
#include "Quantity.h"


using namespace Base;

// clang-format off
constexpr int UnitSignatureLengthBits                   = 4;
constexpr int UnitSignatureMassBits                     = 4;
constexpr int UnitSignatureTimeBits                     = 4;
constexpr int UnitSignatureElectricCurrentBits          = 4;
constexpr int UnitSignatureThermodynamicTemperatureBits = 4;
constexpr int UnitSignatureAmountOfSubstanceBits        = 4;
constexpr int UnitSignatureLuminousIntensityBits        = 4;
constexpr int UnitSignatureAngleBits                    = 4;

struct UnitSignature {
    int32_t Length: UnitSignatureLengthBits;
    int32_t Mass: UnitSignatureMassBits;
    int32_t Time: UnitSignatureTimeBits;
    int32_t ElectricCurrent: UnitSignatureElectricCurrentBits;
    int32_t ThermodynamicTemperature: UnitSignatureThermodynamicTemperatureBits;
    int32_t AmountOfSubstance: UnitSignatureAmountOfSubstanceBits;
    int32_t LuminousIntensity: UnitSignatureLuminousIntensityBits;
    int32_t Angle: UnitSignatureAngleBits;
};

static inline uint32_t sigVal(const std::string &op,
                              int length, int mass, int time, int electricCurrent,
                              int thermodynamicTemperature, int amountOfSubstance, int luminousIntensity, int angle)
{
    if ( ( length                   >=  (1 << (UnitSignatureLengthBits                   - 1)) ) ||
         ( mass                     >=  (1 << (UnitSignatureMassBits                     - 1)) ) ||
         ( time                     >=  (1 << (UnitSignatureTimeBits                     - 1)) ) ||
         ( electricCurrent          >=  (1 << (UnitSignatureElectricCurrentBits          - 1)) ) ||
         ( thermodynamicTemperature >=  (1 << (UnitSignatureThermodynamicTemperatureBits - 1)) ) ||
         ( amountOfSubstance        >=  (1 << (UnitSignatureAmountOfSubstanceBits        - 1)) ) ||
         ( luminousIntensity        >=  (1 << (UnitSignatureLuminousIntensityBits        - 1)) ) ||
         ( angle                    >=  (1 << (UnitSignatureAngleBits                    - 1)) ) ) {
        throw Base::OverflowError(("Unit overflow in " + op).c_str());
    }
    if ( ( length                   <  -(1 << (UnitSignatureLengthBits                   - 1)) ) ||
         ( mass                     <  -(1 << (UnitSignatureMassBits                     - 1)) ) ||
         ( time                     <  -(1 << (UnitSignatureTimeBits                     - 1)) ) ||
         ( electricCurrent          <  -(1 << (UnitSignatureElectricCurrentBits          - 1)) ) ||
         ( thermodynamicTemperature <  -(1 << (UnitSignatureThermodynamicTemperatureBits - 1)) ) ||
         ( amountOfSubstance        <  -(1 << (UnitSignatureAmountOfSubstanceBits        - 1)) ) ||
         ( luminousIntensity        <  -(1 << (UnitSignatureLuminousIntensityBits        - 1)) ) ||
         ( angle                    <  -(1 << (UnitSignatureAngleBits                    - 1)) ) ) {
        throw Base::UnderflowError(("Unit underflow in " + op).c_str());
    }

    UnitSignature Sig;
    Sig.Length                   = length;
    Sig.Mass                     = mass;
    Sig.Time                     = time;
    Sig.ElectricCurrent          = electricCurrent;
    Sig.ThermodynamicTemperature = thermodynamicTemperature;
    Sig.AmountOfSubstance        = amountOfSubstance;
    Sig.LuminousIntensity        = luminousIntensity;
    Sig.Angle                    = angle;

    uint32_t ret;
    memcpy(&ret, &Sig, sizeof(ret));
    return ret;
}


Unit::Unit(int8_t Length, //NOLINT
           int8_t Mass,
           int8_t Time,
           int8_t ElectricCurrent,
           int8_t ThermodynamicTemperature,
           int8_t AmountOfSubstance,
           int8_t LuminousIntensity,
           int8_t Angle)
{
    Val = sigVal("unit",
                 Length,
                 Mass,
                 Time,
                 ElectricCurrent,
                 ThermodynamicTemperature,
                 AmountOfSubstance,
                 LuminousIntensity,
                 Angle);
}


Unit::Unit() //NOLINT
{
    Val = 0;
}

Unit::Unit(const QString& expr)  // NOLINT
{
    try {
        *this = Quantity::parse(expr).getUnit();
    }
    catch (const Base::ParserError&) {
        Val = 0;
    }
}

Unit Unit::pow(double exp) const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    auto isInt = [](double value) {
        return std::fabs(std::round(value) - value) < std::numeric_limits<double>::epsilon();
    };
    if (!isInt(sig.Length                   * exp) ||
        !isInt(sig.Mass                     * exp) ||
        !isInt(sig.Time                     * exp) ||
        !isInt(sig.ElectricCurrent          * exp) ||
        !isInt(sig.ThermodynamicTemperature * exp) ||
        !isInt(sig.AmountOfSubstance        * exp) ||
        !isInt(sig.LuminousIntensity        * exp) ||
        !isInt(sig.Angle                    * exp))
        throw Base::UnitsMismatchError("pow() of unit not possible");

    Unit result;
    result.Val = sigVal("pow()",
               sig.Length                   * exp,
               sig.Mass                     * exp,
               sig.Time                     * exp,
               sig.ElectricCurrent          * exp,
               sig.ThermodynamicTemperature * exp,
               sig.AmountOfSubstance        * exp,
               sig.LuminousIntensity        * exp,
               sig.Angle                    * exp);

    return result;
}

Unit Unit::sqrt() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    // All components of unit must be either zero or dividable by 2
    if (!((sig.Length                   % 2) == 0) &&
         ((sig.Mass                     % 2) == 0) &&
         ((sig.Time                     % 2) == 0) &&
         ((sig.ElectricCurrent          % 2) == 0) &&
         ((sig.ThermodynamicTemperature % 2) == 0) &&
         ((sig.AmountOfSubstance        % 2) == 0) &&
         ((sig.LuminousIntensity        % 2) == 0) &&
         ((sig.Angle                    % 2) == 0))
        throw Base::UnitsMismatchError("sqrt() needs even dimensions");

    Unit result;
    result.Val = sigVal("sqrt()",
          sig.Length                   >> 1,
          sig.Mass                     >> 1,
          sig.Time                     >> 1,
          sig.ElectricCurrent          >> 1,
          sig.ThermodynamicTemperature >> 1,
          sig.AmountOfSubstance        >> 1,
          sig.LuminousIntensity        >> 1,
          sig.Angle                    >> 1);

    return result;
}

Unit Unit::cbrt() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    // All components of unit must be either zero or dividable by 3
    if (!((sig.Length                   % 3) == 0) &&
         ((sig.Mass                     % 3) == 0) &&
         ((sig.Time                     % 3) == 0) &&
         ((sig.ElectricCurrent          % 3) == 0) &&
         ((sig.ThermodynamicTemperature % 3) == 0) &&
         ((sig.AmountOfSubstance        % 3) == 0) &&
         ((sig.LuminousIntensity        % 3) == 0) &&
         ((sig.Angle                    % 3) == 0))
        throw Base::UnitsMismatchError("cbrt() needs dimensions to be multiples of 3");

    Unit result;
    result.Val = sigVal("cbrt()",
          sig.Length                   / 3,
          sig.Mass                     / 3,
          sig.Time                     / 3,
          sig.ElectricCurrent          / 3,
          sig.ThermodynamicTemperature / 3,
          sig.AmountOfSubstance        / 3,
          sig.LuminousIntensity        / 3,
          sig.Angle                    / 3);

    return result;
}

int Unit::length() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.Length;
}

int Unit::mass() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.Mass;
}


int Unit::time() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.Time;
}

int Unit::electricCurrent() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.ElectricCurrent;
}

int Unit::thermodynamicTemperature() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.ThermodynamicTemperature;
}

int Unit::amountOfSubstance() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.AmountOfSubstance;
}

int Unit::luminousIntensity() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.LuminousIntensity;
}

int Unit::angle() const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));
    return sig.Angle;
}

bool Unit::isEmpty() const
{
    return Val == 0;
}

int Unit::operator [](int index) const
{
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));

    switch (index) {
       case 0:
          return sig.Length;
       case 1:
          return sig.Mass;
       case 2:
          return sig.Time;
       case 3:
          return sig.ElectricCurrent;
       case 4:
          return sig.ThermodynamicTemperature;
       case 5:
          return sig.AmountOfSubstance;
       case 6:
          return sig.LuminousIntensity;
       case 7:
          return sig.Angle;
       default:
          throw Base::IndexError("Unknown Unit element");
    }
}

bool Unit::operator ==(const Unit& that) const
{
    return Val == that.Val;
}

Unit Unit::operator *(const Unit &right) const
{
    Unit result;
    UnitSignature sig, rsig;

    memcpy(&sig, &Val, sizeof(Val));
    memcpy(&rsig, &right.Val, sizeof(right.Val));
    result.Val = sigVal("* operator",
                        sig.Length                   + rsig.Length,
                        sig.Mass                     + rsig.Mass,
                        sig.Time                     + rsig.Time,
                        sig.ElectricCurrent          + rsig.ElectricCurrent,
                        sig.ThermodynamicTemperature + rsig.ThermodynamicTemperature,
                        sig.AmountOfSubstance        + rsig.AmountOfSubstance,
                        sig.LuminousIntensity        + rsig.LuminousIntensity,
                        sig.Angle                    + rsig.Angle);

    return result;
}

Unit Unit::operator /(const Unit &right) const
{
    Unit result;
    UnitSignature sig, rsig;

    memcpy(&sig, &Val, sizeof(Val));
    memcpy(&rsig, &right.Val, sizeof(right.Val));
    result.Val = sigVal("/ operator",
                        sig.Length                   - rsig.Length,
                        sig.Mass                     - rsig.Mass,
                        sig.Time                     - rsig.Time,
                        sig.ElectricCurrent          - rsig.ElectricCurrent,
                        sig.ThermodynamicTemperature - rsig.ThermodynamicTemperature,
                        sig.AmountOfSubstance        - rsig.AmountOfSubstance,
                        sig.LuminousIntensity        - rsig.LuminousIntensity,
                        sig.Angle                    - rsig.Angle);

    return result;
}

QString Unit::getString() const
{
    if (isEmpty()) {
        return {};
    }

    std::stringstream ret;
    UnitSignature sig;
    memcpy(&sig, &Val, sizeof(Val));

    if (sig.Length                   > 0 ||
        sig.Mass                     > 0 ||
        sig.Time                     > 0 ||
        sig.ElectricCurrent          > 0 ||
        sig.ThermodynamicTemperature > 0 ||
        sig.AmountOfSubstance        > 0 ||
        sig.LuminousIntensity        > 0 ||
        sig.Angle                    > 0 ) {

        bool mult = false;
        if (sig.Length > 0) {
            mult = true;
            ret << "mm";
            if (sig.Length > 1) {
                ret << "^" << sig.Length;
            }
        }

        if (sig.Mass > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "kg";
            if (sig.Mass > 1) {
                ret << "^" << sig.Mass;
            }
        }

        if (sig.Time > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "s";
            if (sig.Time > 1) {
                ret << "^" << sig.Time;
            }
        }

        if (sig.ElectricCurrent > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "A";
            if (sig.ElectricCurrent > 1) {
                ret << "^" << sig.ElectricCurrent;
            }
        }

        if (sig.ThermodynamicTemperature > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "K";
            if (sig.ThermodynamicTemperature > 1) {
                ret << "^" << sig.ThermodynamicTemperature;
            }
        }

        if (sig.AmountOfSubstance > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "mol";
            if (sig.AmountOfSubstance > 1) {
                ret << "^" << sig.AmountOfSubstance;
            }
        }

        if (sig.LuminousIntensity > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "cd";
            if (sig.LuminousIntensity > 1) {
                ret << "^" << sig.LuminousIntensity;
            }
        }

        if (sig.Angle > 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "deg";
            if (sig.Angle > 1) {
                ret << "^" << sig.Angle;
            }
        }
    }
    else {
        ret << "1";
    }

    if (sig.Length                   < 0 ||
        sig.Mass                     < 0 ||
        sig.Time                     < 0 ||
        sig.ElectricCurrent          < 0 ||
        sig.ThermodynamicTemperature < 0 ||
        sig.AmountOfSubstance        < 0 ||
        sig.LuminousIntensity        < 0 ||
        sig.Angle                    < 0 ) {
        ret << "/";

        int nnom = 0;
        nnom += sig.Length                   < 0 ? 1 : 0;
        nnom += sig.Mass                     < 0 ? 1 : 0;
        nnom += sig.Time                     < 0 ? 1 : 0;
        nnom += sig.ElectricCurrent          < 0 ? 1 : 0;
        nnom += sig.ThermodynamicTemperature < 0 ? 1 : 0;
        nnom += sig.AmountOfSubstance        < 0 ? 1 : 0;
        nnom += sig.LuminousIntensity        < 0 ? 1 : 0;
        nnom += sig.Angle                    < 0 ? 1 : 0;

        if (nnom > 1) {
            ret << '(';
        }

        bool mult = false;
        if (sig.Length < 0) {
            ret << "mm";
            mult = true;
            if (sig.Length < -1) {
                ret << "^" << abs(sig.Length);
            }
        }

        if (sig.Mass < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "kg";
            if (sig.Mass < -1) {
                ret << "^" << abs(sig.Mass);
            }
        }

        if (sig.Time < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "s";
            if (sig.Time < -1) {
                ret << "^" << abs(sig.Time);
            }
        }

        if (sig.ElectricCurrent < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "A";
            if (sig.ElectricCurrent < -1) {
                ret << "^" << abs(sig.ElectricCurrent);
            }
        }

        if (sig.ThermodynamicTemperature < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "K";
            if (sig.ThermodynamicTemperature < -1) {
                ret << "^" << abs(sig.ThermodynamicTemperature);
            }
        }

        if (sig.AmountOfSubstance < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "mol";
            if (sig.AmountOfSubstance < -1) {
                ret << "^" << abs(sig.AmountOfSubstance);
            }
        }

        if (sig.LuminousIntensity < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "cd";
            if (sig.LuminousIntensity < -1) {
                ret << "^" << abs(sig.LuminousIntensity);
            }
        }

        if (sig.Angle < 0) {
            if (mult) {
                ret << '*';
            }
            mult = true;
            ret << "deg";
            if (sig.Angle < -1) {
                ret << "^" << abs(sig.Angle);
            }
        }

        if (nnom > 1) {
            ret << ')';
        }
    }

    return QString::fromUtf8(ret.str().c_str());
}

QString Unit::getTypeString() const
{
    if (*this == Unit::Acceleration) {
        return QString::fromLatin1("Acceleration");
    }
    if (*this == Unit::AmountOfSubstance) {
        return QString::fromLatin1("AmountOfSubstance");
    }
    if (*this == Unit::Angle) {
        return QString::fromLatin1("Angle");
    }
    if (*this == Unit::AngleOfFriction) {
        return QString::fromLatin1("AngleOfFriction");
    }
    if (*this == Unit::Area) {
        return QString::fromLatin1("Area");
    }
    if (*this == Unit::CurrentDensity) {
        return QString::fromLatin1("CurrentDensity");
    }
    if (*this == Unit::Density) {
        return QString::fromLatin1("Density");
    }
    if (*this == Unit::DissipationRate) {
        return QString::fromLatin1("DissipationRate");
    }
    if (*this == Unit::DynamicViscosity) {
        return QString::fromLatin1("DynamicViscosity");
    }
    if (*this == Unit::ElectricalCapacitance) {
        return QString::fromLatin1("ElectricalCapacitance");
    }
    if (*this == Unit::ElectricalConductance) {
        return QString::fromLatin1("ElectricalConductance");
    }
    if (*this == Unit::ElectricalConductivity) {
        return QString::fromLatin1("ElectricalConductivity");
    }
    if (*this == Unit::ElectricalInductance) {
        return QString::fromLatin1("ElectricalInductance");
    }
    if (*this == Unit::ElectricalResistance) {
        return QString::fromLatin1("ElectricalResistance");
    }
    if (*this == Unit::ElectricCharge) {
        return QString::fromLatin1("ElectricCharge");
    }
    if (*this == Unit::ElectricCurrent) {
        return QString::fromLatin1("ElectricCurrent");
    }
    if (*this == Unit::ElectricPotential) {
        return QString::fromLatin1("ElectricPotential");
    }
    if (*this == Unit::ElectromagneticPotential) {
        return QString::fromLatin1("ElectromagneticPotential");
    }
    if (*this == Unit::Frequency) {
        return QString::fromLatin1("Frequency");
    }
    if (*this == Unit::Force) {
        return QString::fromLatin1("Force");
    }
    if (*this == Unit::HeatFlux) {
        return QString::fromLatin1("HeatFlux");
    }
    if (*this == Unit::InverseArea) {
        return QString::fromLatin1("InverseArea");
    }
    if (*this == Unit::InverseLength) {
        return QString::fromLatin1("InverseLength");
    }
    if (*this == Unit::InverseVolume) {
        return QString::fromLatin1("InverseVolume");
    }
    if (*this == Unit::KinematicViscosity) {
        return QString::fromLatin1("KinematicViscosity");
    }
    if (*this == Unit::Length) {
        return QString::fromLatin1("Length");
    }
    if (*this == Unit::LuminousIntensity) {
        return QString::fromLatin1("LuminousIntensity");
    }
    if (*this == Unit::MagneticFieldStrength) {
        return QString::fromLatin1("MagneticFieldStrength");
    }
    if (*this == Unit::MagneticFlux) {
        return QString::fromLatin1("MagneticFlux");
    }
    if (*this == Unit::MagneticFluxDensity) {
        return QString::fromLatin1("MagneticFluxDensity");
    }
    if (*this == Unit::Magnetization) {
        return QString::fromLatin1("Magnetization");
    }
    if (*this == Unit::Mass) {
        return QString::fromLatin1("Mass");
    }
    if (*this == Unit::Pressure) {
        return QString::fromLatin1("Pressure");
    }
    if (*this == Unit::Power) {
        return QString::fromLatin1("Power");
    }
    if (*this == Unit::ShearModulus) {
        return QString::fromLatin1("ShearModulus");
    }
    if (*this == Unit::SpecificEnergy) {
        return QString::fromLatin1("SpecificEnergy");
    }
    if (*this == Unit::SpecificHeat) {
        return QString::fromLatin1("SpecificHeat");
    }
    if (*this == Unit::Stiffness) {
        return QString::fromLatin1("Stiffness");
    }
    if (*this == Unit::StiffnessDensity) {
        return QString::fromLatin1("StiffnessDensity");
    }
    if (*this == Unit::Stress) {
        return QString::fromLatin1("Stress");
    }
    if (*this == Unit::Temperature) {
        return QString::fromLatin1("Temperature");
    }
    if (*this == Unit::ThermalConductivity) {
        return QString::fromLatin1("ThermalConductivity");
    }
    if (*this == Unit::ThermalExpansionCoefficient) {
        return QString::fromLatin1("ThermalExpansionCoefficient");
    }
    if (*this == Unit::ThermalTransferCoefficient) {
        return QString::fromLatin1("ThermalTransferCoefficient");
    }
    if (*this == Unit::TimeSpan) {
        return QString::fromLatin1("TimeSpan");
    }
    if (*this == Unit::UltimateTensileStrength) {
        return QString::fromLatin1("UltimateTensileStrength");
    }
    if (*this == Unit::VacuumPermittivity) {
        return QString::fromLatin1("VacuumPermittivity");
    }
    if (*this == Unit::Velocity) {
        return QString::fromLatin1("Velocity");
    }
    if (*this == Unit::Volume) {
        return QString::fromLatin1("Volume");
    }
    if (*this == Unit::VolumeFlowRate) {
        return QString::fromLatin1("VolumeFlowRate");
    }
    if (*this == Unit::VolumetricThermalExpansionCoefficient) {
        return QString::fromLatin1("VolumetricThermalExpansionCoefficient");
    }
    if (*this == Unit::Work) {
        return QString::fromLatin1("Work");
    }
    if (*this == Unit::YieldStrength) {
        return QString::fromLatin1("YieldStrength");
    }
    if (*this == Unit::YoungsModulus) {
        return QString::fromLatin1("YoungsModulus");
    }
    if (*this == Unit::Moment) {
        return QString::fromLatin1("Moment");
    }

    return {};
}

// SI base units
const Unit Unit::AmountOfSubstance          (0, 0, 0, 0, 0, 1);
const Unit Unit::ElectricCurrent            (0, 0, 0, 1);
const Unit Unit::Length                     (1);
const Unit Unit::LuminousIntensity          (0, 0, 0, 0, 0, 0, 1);
const Unit Unit::Mass                       (0, 1);
const Unit Unit::Temperature                (0, 0, 0, 0, 1);
const Unit Unit::TimeSpan                   (0, 0, 1);

// all other units
const Unit Unit::Acceleration               (1, 0, -2);
const Unit Unit::Angle                      (0, 0, 0, 0, 0, 0, 0, 1);
const Unit Unit::AngleOfFriction            (0, 0, 0, 0, 0, 0, 0, 1);
const Unit Unit::Area                       (2);
const Unit Unit::CompressiveStrength        (-1, 1, -2);
const Unit Unit::CurrentDensity             (-2, 0, 0, 1);
const Unit Unit::Density                    (-3, 1);
const Unit Unit::DissipationRate   (2, 0, -3); // https://cfd-online.com/Wiki/Turbulence_dissipation_rate
const Unit Unit::DynamicViscosity           (-1, 1, -1);
const Unit Unit::ElectricalCapacitance      (-2, -1, 4, 2);
const Unit Unit::ElectricalConductance      (-2, -1, 3, 2);
const Unit Unit::ElectricalConductivity     (-3, -1, 3, 2);
const Unit Unit::ElectricalInductance       (2, 1, -2, -2);
const Unit Unit::ElectricalResistance       (2, 1, -3, -2);
const Unit Unit::ElectricCharge             (0, 0, 1, 1);
const Unit Unit::ElectricPotential          (2, 1, -3, -1);
const Unit Unit::ElectromagneticPotential   (1, 1, -2, -1);
const Unit Unit::Force                      (1, 1, -2);
const Unit Unit::Frequency                  (0, 0, -1);
const Unit Unit::HeatFlux                   (0, 1, -3, 0, 0);
const Unit Unit::InverseArea                (-2, 0, 0);
const Unit Unit::InverseLength              (-1, 0, 0);
const Unit Unit::InverseVolume              (-3, 0, 0);
const Unit Unit::KinematicViscosity         (2, 0, -1);
const Unit Unit::MagneticFieldStrength      (-1,0,0,1);
const Unit Unit::MagneticFlux               (2,1,-2,-1);
const Unit Unit::MagneticFluxDensity        (0,1,-2,-1);
const Unit Unit::Magnetization              (-1,0,0,1);
const Unit Unit::Moment                     (2, 1, -2);
const Unit Unit::Pressure                   (-1,1,-2);
const Unit Unit::Power                      (2, 1, -3);
const Unit Unit::ShearModulus               (-1,1,-2);
const Unit Unit::SpecificEnergy             (2, 0, -2);
const Unit Unit::SpecificHeat               (2, 0, -2, 0, -1);
const Unit Unit::Stiffness                  (0, 1, -2);
const Unit Unit::StiffnessDensity           (-2, 1, -2);
const Unit Unit::Stress                     (-1,1,-2);
const Unit Unit::ThermalConductivity        (1, 1, -3, 0, -1);
const Unit Unit::ThermalExpansionCoefficient(0, 0, 0, 0, -1);
const Unit Unit::ThermalTransferCoefficient (0, 1, -3, 0, -1);
const Unit Unit::UltimateTensileStrength    (-1,1,-2);
const Unit Unit::VacuumPermittivity         (-3, -1, 4,  2);
const Unit Unit::Velocity                   (1, 0, -1);
const Unit Unit::Volume                     (3);
const Unit Unit::VolumeFlowRate             (3, 0, -1);
const Unit Unit::VolumetricThermalExpansionCoefficient(0, 0, 0, 0, -1);
const Unit Unit::Work                       (2, 1, -2);
const Unit Unit::YieldStrength              (-1,1,-2);
const Unit Unit::YoungsModulus              (-1,1,-2);
// clang-format on
