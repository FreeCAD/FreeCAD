/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel                                     *
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
# include <sstream>
# include <stdlib.h>
#endif

#include "Unit.h"
#include "Quantity.h"
#include "Exception.h"

using namespace Base;

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
        throw Base::OverflowError((std::string("Unit underflow in ") + std::string(op)).c_str());
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
               (int32_t)Length,
               (int32_t)Mass,
               (int32_t)Time,
               (int32_t)ElectricCurrent,
               (int32_t)ThermodynamicTemperature,
               (int32_t)AmountOfSubstance,
               (int32_t)LuminousIntensity,
               (int32_t)Angle);

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
    catch (...) {
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

Unit Unit::pow(signed char exp) const
{
    checkRange("pow()",
               (int32_t)Sig.Length * (int32_t)exp,
               (int32_t)Sig.Mass * (int32_t)exp,
               (int32_t)Sig.Time * (int32_t)exp,
               (int32_t)Sig.ElectricCurrent * (int32_t)exp,
               (int32_t)Sig.ThermodynamicTemperature * (int32_t)exp,
               (int32_t)Sig.AmountOfSubstance * (int32_t)exp,
               (int32_t)Sig.LuminousIntensity * (int32_t)exp,
               (int32_t)Sig.Angle * (int32_t)exp);

    Unit result;
    result.Sig.Length                   = Sig.Length                    * exp;
    result.Sig.Mass                     = Sig.Mass                      * exp;
    result.Sig.Time                     = Sig.Time                      * exp;
    result.Sig.ElectricCurrent          = Sig.ElectricCurrent           * exp;
    result.Sig.ThermodynamicTemperature = Sig.ThermodynamicTemperature  * exp;
    result.Sig.AmountOfSubstance        = Sig.AmountOfSubstance         * exp;
    result.Sig.LuminousIntensity        = Sig.LuminousIntensity         * exp;
    result.Sig.Angle                    = Sig.Angle                     * exp;

    return result;
}

bool Unit::isEmpty(void)const
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
               (int32_t)Sig.Length + (int32_t)right.Sig.Length,
               (int32_t)Sig.Mass + (int32_t)right.Sig.Mass,
               (int32_t)Sig.Time + (int32_t)right.Sig.Time,
               (int32_t)Sig.ElectricCurrent + (int32_t)right.Sig.ElectricCurrent,
               (int32_t)Sig.ThermodynamicTemperature + (int32_t)right.Sig.ThermodynamicTemperature,
               (int32_t)Sig.AmountOfSubstance + (int32_t)right.Sig.AmountOfSubstance,
               (int32_t)Sig.LuminousIntensity + (int32_t)right.Sig.LuminousIntensity,
               (int32_t)Sig.Angle + (int32_t)right.Sig.Angle);

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
               (int32_t)Sig.Length - (int32_t)right.Sig.Length,
               (int32_t)Sig.Mass - (int32_t)right.Sig.Mass,
               (int32_t)Sig.Time - (int32_t)right.Sig.Time,
               (int32_t)Sig.ElectricCurrent - (int32_t)right.Sig.ElectricCurrent,
               (int32_t)Sig.ThermodynamicTemperature - (int32_t)right.Sig.ThermodynamicTemperature,
               (int32_t)Sig.AmountOfSubstance - (int32_t)right.Sig.AmountOfSubstance,
               (int32_t)Sig.LuminousIntensity - (int32_t)right.Sig.LuminousIntensity,
               (int32_t)Sig.Angle - (int32_t)right.Sig.Angle);

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

QString Unit::getString(void) const
{
    std::stringstream ret;

    if (isEmpty())
        return QString();

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
            if(mult)
                ret<<'*';
            mult = true;
            ret << "kg";
            if (Sig.Mass < -1)
                ret << "^" << abs(Sig.Mass);
        }

        if (Sig.Time < 0) {
            if(mult)
                ret<<'*';
            mult = true;
            ret << "s";
            if (Sig.Time < -1)
                ret << "^" << abs(Sig.Time);
        }

        if (Sig.ElectricCurrent < 0) {
            if(mult)
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

QString Unit::getTypeString(void) const
{
    if(*this == Unit::Length                      )       return QString::fromLatin1("Length"); else
    if(*this == Unit::Area                        )       return QString::fromLatin1("Area"); else
    if(*this == Unit::Volume                      )       return QString::fromLatin1("Volume"); else
    if(*this == Unit::Mass                        )       return QString::fromLatin1("Mass"); else
    if(*this == Unit::Angle                       )       return QString::fromLatin1("Angle"); else
    if(*this == Unit::Density                     )       return QString::fromLatin1("Density"); else
    if(*this == Unit::TimeSpan                    )       return QString::fromLatin1("TimeSpan"); else
    if(*this == Unit::Velocity                    )       return QString::fromLatin1("Velocity"); else
    if(*this == Unit::Acceleration                )       return QString::fromLatin1("Acceleration"); else
    if(*this == Unit::Temperature                 )       return QString::fromLatin1("Temperature"); else
    if(*this == Unit::ElectricCurrent             )       return QString::fromLatin1("ElectricCurrent"); else
    if(*this == Unit::ElectricPotential           )       return QString::fromLatin1("ElectricPotential"); else
    if(*this == Unit::AmountOfSubstance           )       return QString::fromLatin1("AmountOfSubstance"); else
    if(*this == Unit::LuminousIntensity           )       return QString::fromLatin1("LuminousIntensity"); else
    if(*this == Unit::Pressure                    )       return QString::fromLatin1("Pressure"); else
    if(*this == Unit::Force                       )       return QString::fromLatin1("Force"); else
    if(*this == Unit::Work                        )       return QString::fromLatin1("Work"); else
    if(*this == Unit::Power                       )       return QString::fromLatin1("Power"); else
    if(*this == Unit::SpecificEnergy              )       return QString::fromLatin1("SpecificEnergy"); else
    if(*this == Unit::ThermalConductivity         )       return QString::fromLatin1("ThermalConductivity"); else
    if(*this == Unit::ThermalExpansionCoefficient )       return QString::fromLatin1("ThermalExpansionCoefficient"); else
    if(*this == Unit::SpecificHeat                )       return QString::fromLatin1("SpecificHeat"); else
    if(*this == Unit::ThermalTransferCoefficient  )       return QString::fromLatin1("ThermalTransferCoefficient"); else
    if(*this == Unit::HeatFlux                    )       return QString::fromLatin1("HeatFlux"); else
    if(*this == Unit::DynamicViscosity            )       return QString::fromLatin1("DynamicViscosity"); else
    if(*this == Unit::KinematicViscosity          )       return QString::fromLatin1("KinematicViscosity"); else

    return QString();

}

Unit Unit::Length(1);
Unit Unit::Area(2);
Unit Unit::Volume(3);
Unit Unit::Mass(0,1);
Unit Unit::Angle(0,0,0,0,0,0,0,1);
Unit Unit::Density(-3,1);

Unit Unit::TimeSpan(0,0,1);
Unit Unit::Velocity(1,0,-1);
Unit Unit::Acceleration(1,0,-2);
Unit Unit::Temperature(0,0,0,0,1);

Unit Unit::ElectricCurrent(0,0,0,1);
Unit Unit::ElectricPotential(2,1,-3,-1);
Unit Unit::AmountOfSubstance(0,0,0,0,0,1);
Unit Unit::LuminousIntensity(0,0,0,0,0,0,1);

// Pressure, kg/m*s^2 or N/m^2 or PSI or MPa
Unit Unit::CompressiveStrength     (-1,1,-2);
Unit Unit::Pressure                (-1,1,-2);
Unit Unit::ShearModulus            (-1,1,-2);
Unit Unit::Stress                  (-1,1,-2);
Unit Unit::UltimateTensileStrength (-1,1,-2);
Unit Unit::YieldStrength           (-1,1,-2);
Unit Unit::YoungsModulus           (-1,1,-2);

Unit Unit::Force   (1,1,-2);
Unit Unit::Work    (2,1,-2);
Unit Unit::Power   (2,1,-3);

Unit Unit::SpecificEnergy              (2,0,-2);
Unit Unit::ThermalConductivity         (1,1,-3,0,-1);
Unit Unit::ThermalExpansionCoefficient (0,0,0,0,-1);
Unit Unit::SpecificHeat                (2,0,-2,0,-1);
Unit Unit::ThermalTransferCoefficient  (0,1,-3,0,-1);
Unit Unit::HeatFlux                    (0,1,-3,0,0);
Unit Unit::DynamicViscosity            (-1,1,-1);  // SI unit: kg/m/s
Unit Unit::KinematicViscosity          (2,0,-1);  // SI unit: m^2/s, https://en.wikipedia.org/wiki/Viscosity#Kinematic_viscosity
