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
#include <string>
#include <FCGlobal.h>

namespace Base
{

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
    explicit Unit(const std::string& expr);
    /// Destruction
    ~Unit() = default;

    /** Operators. */
    //@{
    inline Unit& operator*=(const Unit& that);
    inline Unit& operator/=(const Unit& that);
    int operator[](int index) const;
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
    Unit sqrt() const;
    Unit cbrt() const;
    //@}
    int length() const;
    int mass() const;
    int time() const;
    int electricCurrent() const;
    int thermodynamicTemperature() const;
    int amountOfSubstance() const;
    int luminousIntensity() const;
    int angle() const;
    bool isEmpty() const;

    std::string getString() const;
    /// get the type as an string such as "Area", "Length" or "Pressure".
    std::string getTypeString() const;

private:
    uint32_t Val;
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

//--------------------------------------------------------------------------------------------------

namespace Units
{

// clang-format off
const Unit AmountOfSubstance          (0, 0, 0, 0, 0, 1);
const Unit ElectricCurrent            (0, 0, 0, 1);
const Unit Length                     (1);
const Unit LuminousIntensity          (0, 0, 0, 0, 0, 0, 1);
const Unit Mass                       (0, 1);
const Unit Temperature                (0, 0, 0, 0, 1);
const Unit TimeSpan                   (0, 0, 1);

// all other units
const Unit Acceleration               (1, 0, -2);
const Unit Angle                      (0, 0, 0, 0, 0, 0, 0, 1);
const Unit AngleOfFriction            (0, 0, 0, 0, 0, 0, 0, 1);
const Unit Area                       (2);
const Unit CompressiveStrength        (-1, 1, -2);
const Unit CurrentDensity             (-2, 0, 0, 1);
const Unit Density                    (-3, 1);
const Unit DissipationRate            (2, 0, -3); // https://cfd-online.com/Wiki/Turbulence_dissipation_rate
const Unit DynamicViscosity           (-1, 1, -1);
const Unit ElectricalCapacitance      (-2, -1, 4, 2);
const Unit ElectricalConductance      (-2, -1, 3, 2);
const Unit ElectricalConductivity     (-3, -1, 3, 2);
const Unit ElectricalInductance       (2, 1, -2, -2);
const Unit ElectricalResistance       (2, 1, -3, -2);
const Unit ElectricCharge             (0, 0, 1, 1);
const Unit ElectricPotential          (2, 1, -3, -1);
const Unit ElectromagneticPotential   (1, 1, -2, -1);
const Unit Force                      (1, 1, -2);
const Unit Frequency                  (0, 0, -1);
const Unit HeatFlux                   (0, 1, -3, 0, 0);
const Unit InverseArea                (-2, 0, 0);
const Unit InverseLength              (-1, 0, 0);
const Unit InverseVolume              (-3, 0, 0);
const Unit KinematicViscosity         (2, 0, -1);
const Unit MagneticFieldStrength      (-1,0,0,1);
const Unit MagneticFlux               (2,1,-2,-1);
const Unit MagneticFluxDensity        (0,1,-2,-1);
const Unit Magnetization              (-1,0,0,1);
const Unit Moment                     (2, 1, -2);
const Unit Pressure                   (-1,1,-2);
const Unit Power                      (2, 1, -3);
const Unit ShearModulus               (-1,1,-2);
const Unit SpecificEnergy             (2, 0, -2);
const Unit SpecificHeat               (2, 0, -2, 0, -1);
const Unit Stiffness                  (0, 1, -2);
const Unit StiffnessDensity           (-2, 1, -2);
const Unit Stress                     (-1,1,-2);
const Unit ThermalConductivity        (1, 1, -3, 0, -1);
const Unit ThermalExpansionCoefficient(0, 0, 0, 0, -1);
const Unit ThermalTransferCoefficient (0, 1, -3, 0, -1);
const Unit UltimateTensileStrength    (-1,1,-2);
const Unit VacuumPermittivity         (-3, -1, 4,  2);
const Unit Velocity                   (1, 0, -1);
const Unit Volume                     (3);
const Unit VolumeFlowRate             (3, 0, -1);
const Unit VolumetricThermalExpansionCoefficient(0, 0, 0, 0, -1);
const Unit Work                       (2, 1, -2);
const Unit YieldStrength              (-1,1,-2);
const Unit YoungsModulus              (-1,1,-2);
// clang-format on


}  // namespace Units
}  // namespace Base

#endif  // BASE_Unit_H
