#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-non-const-global-variables"
#pragma ide diagnostic ignored "bugprone-easily-swappable-parameters"
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

#include "Exception.h"
#include "Unit.h"
#include "Quantity.h"


using namespace Base;

void Unit::checkUnitRange() const
{
    if (length >= shifted || mass >= shifted || time >= shifted || electricCurrent >= shifted
        || thermodynamicTemperature >= shifted || amountOfSubstance >= shifted
        || luminousIntensity >= shifted || angle >= shifted) {
        throw Base::OverflowError("Unit overflow");
    }
    if (length < -shifted || mass < -shifted || time < -shifted || electricCurrent < -shifted
        || thermodynamicTemperature < -shifted || amountOfSubstance < -shifted
        || luminousIntensity < -shifted || angle < -shifted) {
        throw Base::UnderflowError("Unit underflow");
    }
}

Unit::Unit(int length, int mass, int time, int electricCurrent, int thermodynamicTemperature,
           int amountOfSubstance, int luminousIntensity, int angle)
    : length {length},
      mass {mass},
      time {time},
      electricCurrent {electricCurrent},
      thermodynamicTemperature {thermodynamicTemperature},
      amountOfSubstance {amountOfSubstance},
      luminousIntensity {luminousIntensity},
      angle {angle}
{
    checkUnitRange();
}

Unit::Unit() = default;

Unit::Unit(const Unit& that)
    : length {that.length},
      mass {that.mass},
      time {that.time},
      electricCurrent {that.electricCurrent},
      thermodynamicTemperature {that.thermodynamicTemperature},
      amountOfSubstance {that.amountOfSubstance},
      luminousIntensity {that.luminousIntensity},
      angle {that.angle}
{}

// TODO move this dependency to Quantity
Unit::Unit(const QString& expr)
{
    try {
        *this = Quantity::parse(expr).getUnit();
    }
    catch (const Base::ParserError&) {
        reset();
    }
}

Unit Unit::pow(double exp) const
{
    auto isInt = [](double value) {
        return std::fabs(std::round(value) - value) < std::numeric_limits<double>::epsilon();
    };
    if (!(isInt(length * exp) && isInt(mass * exp) && isInt(time * exp)
          && isInt(electricCurrent * exp) && isInt(thermodynamicTemperature * exp)
          && isInt(amountOfSubstance * exp) && isInt(luminousIntensity * exp)
          && isInt(angle * exp))) {
        throw Base::UnitsMismatchError("pow() of unit not possible");
    }
    Unit unit {static_cast<int>(length * exp),
               static_cast<int>(mass * exp),
               static_cast<int>(time * exp),
               static_cast<int>(electricCurrent * exp),
               static_cast<int>(thermodynamicTemperature * exp),
               static_cast<int>(amountOfSubstance * exp),
               static_cast<int>(luminousIntensity * exp),
               static_cast<int>(angle * exp)};
    return unit;
}

bool Unit::isEmpty() const
{
    return (this->length == 0) && (this->mass == 0) && (this->time == 0)
        && (this->electricCurrent == 0) && (this->thermodynamicTemperature == 0)
        && (this->amountOfSubstance == 0) && (this->luminousIntensity == 0) && (this->angle == 0);
}

bool Unit::operator==(const Unit& that) const
{
    return (this->length == that.length) && (this->mass == that.mass) && (this->time == that.time)
        && (this->electricCurrent == that.electricCurrent)
        && (this->thermodynamicTemperature == that.thermodynamicTemperature)
        && (this->amountOfSubstance == that.amountOfSubstance)
        && (this->luminousIntensity == that.luminousIntensity) && (this->angle == that.angle);
}


Unit Unit::operator*(const Unit& right) const
{
    Unit unit {length + right.length,
               mass + right.mass,
               time + right.time,
               electricCurrent + right.electricCurrent,
               thermodynamicTemperature + right.thermodynamicTemperature,
               amountOfSubstance + right.amountOfSubstance,
               luminousIntensity + right.luminousIntensity,
               angle + right.angle};
    return unit;
}

Unit Unit::operator/(const Unit& right) const
{
    Unit unit {
        length - right.length,
        mass - right.mass,
        time - right.time,
        electricCurrent - right.electricCurrent,
        thermodynamicTemperature - right.thermodynamicTemperature,
        amountOfSubstance - right.amountOfSubstance,
        luminousIntensity - right.luminousIntensity,
        angle - right.angle,
    };
    return unit;
}

Unit& Unit::operator=(const Unit& right)
{
    length = right.length;
    mass = right.mass;
    time = right.time;
    electricCurrent = right.electricCurrent;
    thermodynamicTemperature = right.thermodynamicTemperature;
    amountOfSubstance = right.amountOfSubstance;
    luminousIntensity = right.luminousIntensity;
    angle = right.angle;

    return *this;
}

bool Unit::allZeroOrDivisibleBy(int div) const
{
    return length % div == 0 && mass % div == 0 && time % div == 0 && electricCurrent % div == 0
        && thermodynamicTemperature % div == 0 && amountOfSubstance % div == 0
        && luminousIntensity % div == 0 && angle % div == 0;
}

void Unit::divideAllBy(int div)
{
    length /= div;
    mass /= div;
    time /= div;
    electricCurrent /= div;
    thermodynamicTemperature /= div;
    amountOfSubstance /= div;
    luminousIntensity /= div;
    // angle /= div;
}

void Unit::reset()
{
    length = 0;
    mass = 0;
    time = 0;
    electricCurrent = 0;
    thermodynamicTemperature = 0;
    amountOfSubstance = 0;
    luminousIntensity = 0;
    angle = 0;
}

std::string Unit::representation() const
{
    std::stringstream ret;
    ret << "Unit: ";
    ret << getString().toUtf8().constData() << " (";
    ret << length << ",";
    ret << mass << ",";
    ret << time << ",";
    ret << electricCurrent << ",";
    ret << thermodynamicTemperature << ",";
    ret << amountOfSubstance << ",";
    ret << luminousIntensity << ",";
    ret << angle << ")";
    std::string type = getTypeString().toUtf8().constData();
    if (!type.empty()) {
        ret << " [" << type << "]";
    }
    return ret.str();
}

int Unit::getLength() const
{
    return length;
}
int Unit::getMass() const
{
    return mass;
}
int Unit::getTime() const
{
    return time;
}
int Unit::getElectricCurrent() const
{
    return electricCurrent;
}
int Unit::getThermodynamicTemperature() const
{
    return thermodynamicTemperature;
}
int Unit::getAmountOfSubstance() const
{
    return amountOfSubstance;
}
int Unit::getLuminousIntensity() const
{
    return luminousIntensity;
}
int Unit::getAngle() const
{
    return angle;
}

QString Unit::getString() const
{
    auto calcString = [&]() -> std::string {
        {
            ValsArray vals {length,
                            mass,
                            time,
                            electricCurrent,
                            thermodynamicTemperature,
                            amountOfSubstance,
                            luminousIntensity,
                            angle};
            std::stringstream ret;

            auto draw = [&](auto lambda1, auto lambda2) {
                bool mult {false};
                bool notFirst {true};
                auto drawValue = [&](int posn) {
                    int value = vals.at(posn);
                    if (!lambda1(value)) {
                        return;
                    }
                    const char* defStr = baseDefStrs.at(posn);
                    if (notFirst && mult) {
                        ret << "*";
                    }
                    mult = true;
                    notFirst = true;
                    ret << defStr;
                    if (lambda2(value)) {
                        ret << "^" << abs(value);
                    }
                };
                for (int i = 0; i < numBaseItems; i++) {
                    drawValue(i);
                }
            };

            auto drawPos = [&]() {
                draw(
                    [](int value) {
                        return value > 0;
                    },
                    [](int value) {
                        return value > 1;
                    });
            };

            auto drawNeg = [&]() {
                draw(
                    [](int value) {
                        return value < 0;
                    },
                    [](int value) {
                        return value < -1;
                    });
            };

            if (isEmpty()) {
                return "";
            }
            bool hasMoreThanZero = std::any_of(vals.cbegin(), vals.cend(), [](int val) {
                return val > 0;
            });
            if (hasMoreThanZero) {
                drawPos();
            }
            else {
                ret << "1";
            }

            auto negCount = static_cast<int>(std::count_if(vals.begin(), vals.end(), [](int val) {
                return val < 0;
            }));
            if (negCount == 0) {
                return ret.str();
            }
            ret << '/';
            if (negCount == 1) {
                drawNeg();
            }
            else {
                ret << '(';
                drawNeg();
                ret << ')';
            }

            return ret.str();
        }
    };
    return QString::fromStdString(calcString());
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

    return {};
}

// SI base units
Unit Unit::AmountOfSubstance(0, 0, 0, 0, 0, 1);
Unit Unit::ElectricCurrent(0, 0, 0, 1);
Unit Unit::Length(1);
Unit Unit::LuminousIntensity(0, 0, 0, 0, 0, 0, 1);
Unit Unit::Mass(0, 1);
Unit Unit::Temperature(0, 0, 0, 0, 1);
Unit Unit::TimeSpan(0, 0, 1);

// all other units
Unit Unit::Acceleration(1, 0, -2);
Unit Unit::Angle(0, 0, 0, 0, 0, 0, 0, 1);
Unit Unit::AngleOfFriction(0, 0, 0, 0, 0, 0, 0, 1);
Unit Unit::Area(2);
Unit Unit::CompressiveStrength(-1, 1, -2);
Unit Unit::CurrentDensity(-2, 0, 0, 1);
Unit Unit::Density(-3, 1);
Unit Unit::DissipationRate(2, 0, -3);// https://cfd-online.com/Wiki/Turbulence_dissipation_rate
Unit Unit::DynamicViscosity(-1, 1, -1);
Unit Unit::ElectricalCapacitance(-2, -1, 4, 2);
Unit Unit::ElectricalConductance(-2, -1, 3, 2);
Unit Unit::ElectricalConductivity(-3, -1, 3, 2);
Unit Unit::ElectricalInductance(2, 1, -2, -2);
Unit Unit::ElectricalResistance(2, 1, -3, -2);
Unit Unit::ElectricCharge(0, 0, 1, 1);
Unit Unit::ElectricPotential(2, 1, -3, -1);
Unit Unit::Force(1, 1, -2);
Unit Unit::Frequency(0, 0, -1);
Unit Unit::HeatFlux(0, 1, -3, 0, 0);
Unit Unit::InverseArea(-2, 0, 0);
Unit Unit::InverseLength(-1, 0, 0);
Unit Unit::InverseVolume(-3, 0, 0);
Unit Unit::KinematicViscosity(2, 0, -1);
Unit Unit::MagneticFieldStrength(-1, 0, 0, 1);
Unit Unit::MagneticFlux(2, 1, -2, -1);
Unit Unit::MagneticFluxDensity(0, 1, -2, -1);
Unit Unit::Magnetization(-1, 0, 0, 1);
Unit Unit::Pressure(-1, 1, -2);
Unit Unit::Power(2, 1, -3);
Unit Unit::ShearModulus(-1, 1, -2);
Unit Unit::SpecificEnergy(2, 0, -2);
Unit Unit::SpecificHeat(2, 0, -2, 0, -1);
Unit Unit::Stiffness(0, 1, -2);
Unit Unit::Stress(-1, 1, -2);
Unit Unit::ThermalConductivity(1, 1, -3, 0, -1);
Unit Unit::ThermalExpansionCoefficient(0, 0, 0, 0, -1);
Unit Unit::ThermalTransferCoefficient(0, 1, -3, 0, -1);
Unit Unit::UltimateTensileStrength(-1, 1, -2);
Unit Unit::VacuumPermittivity(-3, -1, 4, 2);
Unit Unit::Velocity(1, 0, -1);
Unit Unit::Volume(3);
Unit Unit::VolumeFlowRate(3, 0, -1);
Unit Unit::VolumetricThermalExpansionCoefficient(0, 0, 0, 0, -1);
Unit Unit::Work(2, 1, -2);
Unit Unit::YieldStrength(-1, 1, -2);
Unit Unit::YoungsModulus(-1, 1, -2);

#pragma clang diagnostic pop
