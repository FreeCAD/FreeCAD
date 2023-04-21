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


#ifndef BASE_Unit_H
#define BASE_Unit_H

#include <cstdint>
#include <QString>
#include <FCGlobal.h>

namespace Base {

#define UnitSignatureLengthBits 4
#define UnitSignatureMassBits 4
#define UnitSignatureTimeBits 4
#define UnitSignatureElectricCurrentBits 4
#define UnitSignatureThermodynamicTemperatureBits 4
#define UnitSignatureAmountOfSubstanceBits 4
#define UnitSignatureLuminousIntensityBits 4
#define UnitSignatureAngleBits 4

// Hint:
// https://en.cppreference.com/w/cpp/language/bit_field
// https://stackoverflow.com/questions/33723631/signed-bit-field-in-c14
struct UnitSignature{
    int32_t Length:UnitSignatureLengthBits;
    int32_t Mass:UnitSignatureMassBits;
    int32_t Time:UnitSignatureTimeBits;
    int32_t ElectricCurrent:UnitSignatureElectricCurrentBits;
    int32_t ThermodynamicTemperature:UnitSignatureThermodynamicTemperatureBits;
    int32_t AmountOfSubstance:UnitSignatureAmountOfSubstanceBits;
    int32_t LuminousIntensity:UnitSignatureLuminousIntensityBits;
    int32_t Angle:UnitSignatureAngleBits;
};
/**
 * The Unit class.
 */
class BaseExport Unit
{
public:
    /// default constructor
    explicit Unit(int8_t Length,int8_t Mass=0,int8_t Time=0,int8_t ElectricCurrent=0,
                  int8_t ThermodynamicTemperature=0, int8_t AmountOfSubstance=0,
                  int8_t LuminousIntensity=0, int8_t Angle=0);
    Unit();
    Unit(const Unit&);
    explicit Unit(const QString& expr);
    /// Destruction
    ~Unit () = default;


    /** Operators. */
    //@{
    inline Unit& operator *=(const Unit& that);
    inline Unit& operator /=(const Unit& that);
    Unit operator *(const Unit&) const;
    Unit operator /(const Unit&) const;
    bool operator ==(const Unit&) const;
    bool operator !=(const Unit&that) const {return !(*this == that);}
    Unit& operator =(const Unit&);
    Unit pow(double exp)const;
    //@}
    /// get the unit signature
    const UnitSignature & getSignature()const {return Sig;}
    bool isEmpty()const;

    QString getString() const;
    /// get the type as an string such as "Area", "Length" or "Pressure".
    QString getTypeString() const;

    /** Predefined Unit types. */
    //@{
    /// Length unit
    static Unit Length;
    /// Mass unit
    static Unit Mass;

    /// Angle
    static Unit Angle;
    static Unit AngleOfFriction;

    static Unit Density;

    static Unit Area;
    static Unit Volume;
    static Unit TimeSpan;
    static Unit Frequency;
    static Unit Velocity;
    static Unit Acceleration;
    static Unit Temperature;

    static Unit CurrentDensity;
    static Unit ElectricCurrent;
    static Unit ElectricPotential;
    static Unit ElectricCharge;
    static Unit MagneticFieldStrength;
    static Unit MagneticFlux;
    static Unit MagneticFluxDensity;
    static Unit Magnetization;
    static Unit ElectricalCapacitance;
    static Unit ElectricalInductance;
    static Unit ElectricalConductance;
    static Unit ElectricalResistance;
    static Unit ElectricalConductivity;
    static Unit AmountOfSubstance;
    static Unit LuminousIntensity;

    // Pressure
    static Unit CompressiveStrength;
    static Unit Pressure;
    static Unit ShearModulus;
    static Unit Stress;
    static Unit UltimateTensileStrength;
    static Unit YieldStrength;
    static Unit YoungsModulus;

    static Unit Stiffness;

    static Unit Force;
    static Unit Work;
    static Unit Power;

    static Unit SpecificEnergy;
    static Unit ThermalConductivity;
    static Unit ThermalExpansionCoefficient;
    static Unit VolumetricThermalExpansionCoefficient;
    static Unit SpecificHeat;
    static Unit ThermalTransferCoefficient;
    static Unit HeatFlux;
    static Unit DynamicViscosity;
    static Unit KinematicViscosity;
    static Unit VacuumPermittivity;
    static Unit VolumeFlowRate;
    static Unit DissipationRate;

    static Unit InverseLength;
    static Unit InverseArea;
    static Unit InverseVolume;

    //@}
protected:
    UnitSignature Sig;
};

inline Unit& Unit::operator *=(const Unit& that)
{
    *this = *this * that;
    return *this;
}

inline Unit& Unit::operator /=(const Unit& that)
{
    *this = *this / that;
    return *this;
}

} // namespace Base

#endif // BASE_Unit_H
