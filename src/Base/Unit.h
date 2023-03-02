#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-non-const-global-variables"
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

#include <FCGlobal.h>
#include <QString>
#include <array>
#include <cstdint>

namespace Base
{

constexpr int bitsLength {4};                 // bit field legacy
constexpr int shifted {1 << (bitsLength - 1)};// bit field legacy
constexpr int numBaseItems {8};
using ValsArray = std::array<int, numBaseItems>;
using StrsArray = std::array<const char*, numBaseItems>;
constexpr StrsArray baseDefStrs {"mm", "kg", "s", "A", "K", "mol", "cd", "deg"};

/**
 * The Unit class.
 */
class BaseExport Unit
{
public:
    /// default constructor
    explicit Unit(int length, int mass = 0, int time = 0, int electricCurrent = 0,
                  int thermodynamicTemperature = 0, int amountOfSubstance = 0,
                  int luminousIntensity = 0, int angle = 0);
    Unit();
    Unit(const Unit&);
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
    Unit& operator=(const Unit&);
    Unit pow(double exp) const;

    //@}
    bool isEmpty() const;

    // TODO replace all QString with std::string

    QString getString() const;
    /// get the type as an string such as "Area", "Length" or "Pressure".
    QString getTypeString() const;
    bool allZeroOrDivisibleBy(int) const;
    void divideAllBy(int);
    std::string representation() const;

    int getLength() const;
    int getMass() const;
    int getTime() const;
    int getElectricCurrent() const;
    int getThermodynamicTemperature() const;
    int getAmountOfSubstance() const;
    int getLuminousIntensity() const;
    int getAngle() const;

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
    void reset();
    void checkUnitRange() const;

    int length {};
    int mass {};
    int time {};
    int electricCurrent {};
    int thermodynamicTemperature {};
    int amountOfSubstance {};
    int luminousIntensity {};
    int angle {};
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

}// namespace Base

#endif// BASE_Unit_H

#pragma clang diagnostic pop
