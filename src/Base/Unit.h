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

namespace Base
{

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
struct UnitSignature
{
    int32_t Length: UnitSignatureLengthBits;
    int32_t Mass: UnitSignatureMassBits;
    int32_t Time: UnitSignatureTimeBits;
    int32_t ElectricCurrent: UnitSignatureElectricCurrentBits;
    int32_t ThermodynamicTemperature: UnitSignatureThermodynamicTemperatureBits;
    int32_t AmountOfSubstance: UnitSignatureAmountOfSubstanceBits;
    int32_t LuminousIntensity: UnitSignatureLuminousIntensityBits;
    int32_t Angle: UnitSignatureAngleBits;
};
/**
 * The Unit class.
 */
class BaseExport Unit
{
public:
    /// default constructor
    explicit Unit(int8_t Length,
                  int8_t Mass = 0,
                  int8_t Time = 0,
                  int8_t ElectricCurrent = 0,
                  int8_t ThermodynamicTemperature = 0,
                  int8_t AmountOfSubstance = 0,
                  int8_t LuminousIntensity = 0,
                  int8_t Angle = 0);
    Unit();
    Unit(const Unit&) = default;
    Unit(Unit&&) = default;
    explicit Unit(const QString& expr);
    /// Destruction
    ~Unit() = default;


    /** Operators. */
    //@{
    inline Unit& operator*=(const Unit& that);
    inline Unit& operator/=(const Unit& that);
    Unit operator*(const Unit&) const;
    Unit operator/(const Unit&) const;
    bool operator==(const Unit&) const;
    bool operator!=(const Unit& that) const
    {
        return !(*this == that);
    }
    Unit& operator=(const Unit&) = default;
    Unit& operator=(Unit&&) = default;
    Unit pow(double exp) const;
    //@}
    /// get the unit signature
    const UnitSignature& getSignature() const
    {
        return Sig;
    }
    bool isEmpty() const;

    QString getString() const;
    /// get the type as an string such as "Area", "Length" or "Pressure".
    QString getTypeString() const;

    /** Predefined Unit types. */
    //@{
    /// Length unit
    static const Unit Length;
    /// Mass unit
    static const Unit Mass;

    /// Angle
    static const Unit Angle;
    static const Unit AngleOfFriction;

    static const Unit Density;

    static const Unit Area;
    static const Unit Volume;
    static const Unit TimeSpan;
    static const Unit Frequency;
    static const Unit Velocity;
    static const Unit Acceleration;
    static const Unit Temperature;

    static const Unit CurrentDensity;
    static const Unit ElectricCurrent;
    static const Unit ElectricPotential;
    static const Unit ElectricCharge;
    static const Unit MagneticFieldStrength;
    static const Unit MagneticFlux;
    static const Unit MagneticFluxDensity;
    static const Unit Magnetization;
    static const Unit ElectricalCapacitance;
    static const Unit ElectricalInductance;
    static const Unit ElectricalConductance;
    static const Unit ElectricalResistance;
    static const Unit ElectricalConductivity;
    static const Unit AmountOfSubstance;
    static const Unit LuminousIntensity;

    // Pressure
    static const Unit CompressiveStrength;
    static const Unit Pressure;
    static const Unit ShearModulus;
    static const Unit Stress;
    static const Unit UltimateTensileStrength;
    static const Unit YieldStrength;
    static const Unit YoungsModulus;

    static const Unit Stiffness;
    static const Unit StiffnessDensity;

    static const Unit Force;
    static const Unit Work;
    static const Unit Power;
    static const Unit Moment;

    static const Unit SpecificEnergy;
    static const Unit ThermalConductivity;
    static const Unit ThermalExpansionCoefficient;
    static const Unit VolumetricThermalExpansionCoefficient;
    static const Unit SpecificHeat;
    static const Unit ThermalTransferCoefficient;
    static const Unit HeatFlux;
    static const Unit DynamicViscosity;
    static const Unit KinematicViscosity;
    static const Unit VacuumPermittivity;
    static const Unit VolumeFlowRate;
    static const Unit DissipationRate;

    static const Unit InverseLength;
    static const Unit InverseArea;
    static const Unit InverseVolume;

    //@}
private:
    UnitSignature Sig;
};

inline Unit& Unit::operator*=(const Unit& that)
{
    *this = *this * that;
    return *this;
}

inline Unit& Unit::operator/=(const Unit& that)
{
    *this = *this / that;
    return *this;
}

}  // namespace Base

#endif  // BASE_Unit_H
