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


#pragma once

#include "Unit.h"
#include <cstdint>
#include <optional>
#include <string>

namespace Base
{
class UnitsSchema;

struct BaseExport QuantityFormat
{
    // Base-owned number formatting flags.
    //
    // Note: values intentionally match Qt's QLocale::NumberOptions bit assignments for historical
    // compatibility (e.g. persisted config values), but this type is independent of Qt.
    using NumberOptions = std::uint32_t;
    static constexpr NumberOptions None = 0x00;
    static constexpr NumberOptions OmitGroupSeparator = 0x01;
    static constexpr NumberOptions RejectGroupSeparator = 0x02;
    // Reserved for future use (aligns with Qt values if/when needed):
    static constexpr NumberOptions OmitLeadingZeroInExponent = 0x04;
    static constexpr NumberOptions IncludeTrailingZeroesAfterDot = 0x08;
    enum NumberFormat
    {
        Default = 0,
        Fixed = 1,
        Scientific = 2
    };

    NumberOptions option;
    NumberFormat format;

    int getPrecision() const;
    inline void setPrecision(int precision)
    {
        _precision = precision;
    }

    int getDenominator() const;
    inline void setDenominator(int denominator)
    {
        _denominator = denominator;
    }

    QuantityFormat();
    explicit QuantityFormat(NumberFormat format, int decimals = -1);
    inline char toFormat() const
    {
        switch (format) {
            case Fixed:
                return 'f';
            case Scientific:
                return 'e';
            default:
                return 'g';
        }
    }
    static inline NumberFormat toFormat(char ch, bool* ok = nullptr)
    {
        if (ok) {
            *ok = true;
        }
        switch (ch) {
            case 'f':
                return Fixed;
            case 'e':
                return Scientific;
            case 'g':
                return Default;
            default:
                if (ok) {
                    *ok = false;
                }
                return Default;
        }
    }

private:
    int _precision, _denominator;
};

/**
 * The Quantity class.
 */
class BaseExport Quantity
{
public:
    /// default constructor
    Quantity();
    Quantity(const Quantity&) = default;
    Quantity(Quantity&&) = default;
    explicit Quantity(double value, const Unit& unit = Unit());
    explicit Quantity(double value, const std::string& unit);
    /// Destruction
    ~Quantity() = default;

    /** Operators. */
    //@{
    Quantity operator*(const Quantity& other) const;
    Quantity operator*(double factor) const;
    Quantity operator+(const Quantity& other) const;
    Quantity& operator+=(const Quantity& other);
    Quantity operator-(const Quantity& other) const;
    Quantity& operator-=(const Quantity& other);
    Quantity operator-() const;
    Quantity operator/(const Quantity& other) const;
    Quantity operator/(double factor) const;
    bool operator==(const Quantity&) const;
    bool operator!=(const Quantity&) const;
    bool operator<(const Quantity&) const;
    bool operator>(const Quantity&) const;
    bool operator<=(const Quantity&) const;
    bool operator>=(const Quantity&) const;
    Quantity& operator=(const Quantity&) = default;
    Quantity& operator=(Quantity&&) = default;
    Quantity pow(const Quantity&) const;
    Quantity pow(double) const;
    //@}

    const QuantityFormat& getFormat() const
    {
        return myFormat;
    }
    void setFormat(const QuantityFormat& fmt)
    {
        myFormat = fmt;
    }

    std::string toString(const QuantityFormat& format = QuantityFormat(QuantityFormat::Default)) const;

    std::string toNumber(const QuantityFormat& format = QuantityFormat(QuantityFormat::Default)) const;

    std::string getUserString() const;
    /// transfer to user preferred unit/potence
    std::string getUserString(double& factor, std::string& unitString) const;
    std::string getUserString(UnitsSchema* schema, double& factor, std::string& unitString) const;
    std::string getSafeUserString() const;

    static Quantity parse(const std::string& string);

    /// Return the quantity represented by an exact unit symbol, if known.
    static std::optional<Quantity> lookupUnit(const std::string& symbol);

    /// returns the unit of the quantity
    const Unit& getUnit() const
    {
        return myUnit;
    }
    /// set the unit of the quantity
    void setUnit(const Unit& un)
    {
        myUnit = un;
    }
    /// get the Value of the quantity
    double getValue() const
    {
        return myValue;
    }
    /// set the value of the quantity
    void setValue(double val)
    {
        myValue = val;
    }
    /** get the Value in a special unit given as quantity.
     * One can use one of the predifeined quantity units in this class
     */
    double getValueAs(const Quantity&) const;


    /// true if it has no unit
    bool isDimensionless() const;
    /// true if it has a specific unit or no dimension.
    bool isDimensionlessOrUnit(const Unit& unit) const;
    /// true if it has a number with or without a unit
    bool isValid() const;
    /// sets the quantity invalid
    void setInvalid();

    /** Return a registered unit or throw ParserError for an unknown symbol. */
    static Quantity unit(std::string_view symbol);

    /** Predefined units for compiler-checked C++ access. */
    //@{
#define QUANTITY_UNIT(name, symbol) static const Quantity name;
#include "QuantityUnits.inc"
#undef QUANTITY_UNIT
    //@}

private:
    double myValue;
    Unit myUnit;
    QuantityFormat myFormat;
};

}  // namespace Base
