// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <cmath>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include "Exception.h"
#include "Quantity.h"
#include "QuantityParser.h"
#include "Tools.h"
#include "UnitRegistry.h"
#include "UnitsApi.h"
#include "UnitsConvData.h"
#include "UnitsSchema.h"

/** \defgroup Units Units system
    \ingroup BASE
    \brief The quantities and units system enables FreeCAD to work transparently with many different
   units
*/

using Base::ParserError;
using Base::Quantity;
using Base::QuantityFormat;
using Base::UnitsSchema;

QuantityFormat::QuantityFormat()
    : option(OmitGroupSeparator | RejectGroupSeparator)
    , format(Fixed)
    , _precision(-1)
    , _denominator(-1)
{}

QuantityFormat::QuantityFormat(QuantityFormat::NumberFormat format, int decimals)
    : option(OmitGroupSeparator | RejectGroupSeparator)
    , format(format)
    , _precision(decimals)
    , _denominator(-1)
{}

int QuantityFormat::getPrecision() const
{
    return _precision < 0 ? UnitsApi::getDecimals() : _precision;
}

int QuantityFormat::getDenominator() const
{
    return _denominator < 0 ? UnitsApi::getDenominator() : _denominator;
}

// ----------------------------------------------------------------------------

Quantity::Quantity()
    : myValue {0.0}
{}

Quantity::Quantity(double value, const Unit& unit)
    : myValue {value}
    , myUnit {unit}
{}

Quantity::Quantity(double value, const std::string& unit)
{
    if (unit.empty()) {
        myValue = value;
        myUnit = Unit();
        return;
    }

    try {
        auto tmpQty = parse(unit);
        myValue = value * tmpQty.getValue();
        myUnit = tmpQty.getUnit();
    }
    catch (const Base::ParserError&) {
        myValue = 0.0;
        myUnit = Unit();
    }
}

double Quantity::getValueAs(const Quantity& other) const
{
    return myValue / other.getValue();
}

bool Quantity::operator==(const Quantity& that) const
{
    return (myValue == that.myValue) && (myUnit == that.myUnit);
}

bool Quantity::operator!=(const Quantity& that) const
{
    return !(*this == that);
}

bool Quantity::operator<(const Quantity& that) const
{
    if (myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator <(): quantities need to have same unit to compare"
        );
    }

    return (myValue < that.myValue);
}

bool Quantity::operator>(const Quantity& that) const
{
    if (myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator >(): quantities need to have same unit to compare"
        );
    }

    return (myValue > that.myValue);
}

bool Quantity::operator<=(const Quantity& that) const
{
    if (myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator <=(): quantities need to have same unit to compare"
        );
    }

    return (myValue <= that.myValue);
}

bool Quantity::operator>=(const Quantity& that) const
{
    if (myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator >=(): quantities need to have same unit to compare"
        );
    }

    return (myValue >= that.myValue);
}

Quantity Quantity::operator*(const Quantity& other) const
{
    return Quantity(myValue * other.myValue, myUnit * other.myUnit);
}

Quantity Quantity::operator*(double factor) const
{
    return Quantity(myValue * factor, myUnit);
}

Quantity Quantity::operator/(const Quantity& other) const
{
    return Quantity(myValue / other.myValue, myUnit / other.myUnit);
}

Quantity Quantity::operator/(double factor) const
{
    return Quantity(myValue / factor, myUnit);
}

Quantity Quantity::pow(const Quantity& other) const
{
    if (!other.isDimensionless()) {
        throw Base::UnitsMismatchError("Quantity::pow(): exponent must not have a unit");
    }

    return Quantity(
        std::pow(myValue, other.myValue),
        myUnit.pow(static_cast<signed char>(other.myValue))
    );
}

Quantity Quantity::pow(double exp) const
{
    return Quantity(std::pow(myValue, exp), myUnit.pow(exp));
}

Quantity Quantity::operator+(const Quantity& other) const
{
    if (myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator +(): Unit mismatch in plus operation");
    }

    return Quantity(myValue + other.myValue, myUnit);
}

Quantity& Quantity::operator+=(const Quantity& other)
{
    if (myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator +=(): Unit mismatch in plus operation");
    }

    myValue += other.myValue;

    return *this;
}

Quantity Quantity::operator-(const Quantity& other) const
{
    if (myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator -(): Unit mismatch in minus operation");
    }

    return Quantity(myValue - other.myValue, myUnit);
}

Quantity& Quantity::operator-=(const Quantity& other)
{
    if (myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator -=(): Unit mismatch in minus operation");
    }

    myValue -= other.myValue;

    return *this;
}

Quantity Quantity::operator-() const
{
    return Quantity(-myValue, myUnit);
}

std::string Quantity::toString(const QuantityFormat& format) const
{
    return fmt::format("'{} {}'", toNumber(format), myUnit.getString());
}

std::string Quantity::toNumber(const QuantityFormat& format) const
{
    std::stringstream ss;

    switch (format.format) {
        case QuantityFormat::Fixed:
            ss << std::fixed;
            break;
        case QuantityFormat::Scientific:
            ss << std::scientific;
            break;
        default:
            break;
    }
    ss << std::setprecision(format.getPrecision()) << myValue;

    return ss.str();
}

std::string Quantity::getUserString() const
{
    double dummy1 {};  // to satisfy GCC
    std::string dummy2 {};
    return getUserString(dummy1, dummy2);
}

std::string Quantity::getUserString(double& factor, std::string& unitString) const
{
    return Base::UnitsApi::schemaTranslate(*this, factor, unitString);
}

std::string Quantity::getUserString(UnitsSchema* schema, double& factor, std::string& unitString) const
{
    return schema->translate(*this, factor, unitString);
}

std::string Quantity::getSafeUserString() const
{
    auto userStr = getUserString();
    if (myValue != 0.0) {
        bool useFallback {false};
        try {
            useFallback = (parse(userStr).getValue() == 0);
        }
        catch (const Base::ParserError&) {
            useFallback = true;
        }
        if (useFallback) {
            auto unitStr = getUnit().getString();
            userStr = fmt::format("{}{}{}", myValue, unitStr.empty() ? "" : " ", unitStr);
        }
    }

    return Tools::escapeQuotesFromString(userStr);
}

/// true if unit equals to 1, therefore quantity has no dimension
bool Quantity::isDimensionless() const
{
    return myUnit == Unit::One;
}

/// true if it has a specific unit or no dimension.
bool Quantity::isDimensionlessOrUnit(const Unit& unit) const
{
    return isDimensionless() || myUnit == unit;
}

// true if it has a number with or without a unit
bool Quantity::isValid() const
{
    return !std::isnan(myValue);
}

void Quantity::setInvalid()
{
    myValue = std::numeric_limits<double>::quiet_NaN();
}

// === Predefined types =====================================================
// clang-format off
using namespace Base::UnitsConvData;

#define QUANTITY_UNIT(name, symbol) const Quantity Quantity::name = UnitRegistry::require(symbol);
#include "QuantityUnits.inc"
#undef QUANTITY_UNIT

Quantity Quantity::parse(const std::string& string)
{
    return QuantityParserSupport::parse(string);
}

std::optional<Quantity> Quantity::lookupUnit(const std::string& symbol)
{
    return UnitRegistry::lookup(symbol);
}

Quantity Quantity::unit(std::string_view symbol)
{
    return UnitRegistry::require(symbol);
}
