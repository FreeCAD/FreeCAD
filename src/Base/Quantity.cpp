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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#endif

#include <fmt/format.h>

#include "Exception.h"
#include "Quantity.h"
#include "Tools.h"
#include "UnitsApi.h"
#include "UnitsSchema.h"

/** \defgroup Units Units system
    \ingroup BASE
    \brief The quantities and units system enables FreeCAD to work transparently with many different
   units
*/

// suppress annoying warnings from generated source files
#ifdef _MSC_VER
#pragma warning(disable : 4003)
#pragma warning(disable : 4018)
#pragma warning(disable : 4065)
#pragma warning(disable : 4273)
#pragma warning(disable : 4335)  // disable MAC file format warning on VC
#endif

using Base::Quantity;
using Base::QuantityFormat;
using Base::UnitsSchema;

// ====== Static attributes =========================
// NOLINTNEXTLINE
int QuantityFormat::defaultDenominator = 8;  // for 1/8"


QuantityFormat::QuantityFormat()
    : option(OmitGroupSeparator | RejectGroupSeparator)
    , format(Fixed)
    , precision(static_cast<int>(UnitsApi::getDecimals()))
    , denominator(defaultDenominator)
{}

QuantityFormat::QuantityFormat(QuantityFormat::NumberFormat format, int decimals)
    : option(OmitGroupSeparator | RejectGroupSeparator)
    , format(format)
    , precision(decimals < 0 ? UnitsApi::getDecimals() : decimals)
    , denominator(defaultDenominator)
{}

// ----------------------------------------------------------------------------

Quantity::Quantity()
    : myValue {0.0}
{}

Quantity::Quantity(double value, const Unit& unit)
    : myValue {value}
    , myUnit {unit}
{}

Quantity::Quantity(double value, const std::string& unit)
    : myValue {0.0}
{
    if (unit.empty()) {
        this->myValue = value;
        this->myUnit = Unit();
        return;
    }

    try {
        auto tmpQty = parse(unit);
        this->myUnit = tmpQty.getUnit();
        this->myValue = value * tmpQty.getValue();
    }
    catch (const Base::ParserError&) {
        this->myValue = 0.0;
        this->myUnit = Unit();
    }
}

double Quantity::getValueAs(const Quantity& other) const
{
    return myValue / other.getValue();
}

bool Quantity::operator==(const Quantity& that) const
{
    return (this->myValue == that.myValue) && (this->myUnit == that.myUnit);
}

bool Quantity::operator!=(const Quantity& that) const
{
    return !(*this == that);
}

bool Quantity::operator<(const Quantity& that) const
{
    if (this->myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator <(): quantities need to have same unit to compare");
    }

    return (this->myValue < that.myValue);
}

bool Quantity::operator>(const Quantity& that) const
{
    if (this->myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator >(): quantities need to have same unit to compare");
    }

    return (this->myValue > that.myValue);
}

bool Quantity::operator<=(const Quantity& that) const
{
    if (this->myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator <=(): quantities need to have same unit to compare");
    }

    return (this->myValue <= that.myValue);
}

bool Quantity::operator>=(const Quantity& that) const
{
    if (this->myUnit != that.myUnit) {
        throw Base::UnitsMismatchError(
            "Quantity::operator >=(): quantities need to have same unit to compare");
    }

    return (this->myValue >= that.myValue);
}

Quantity Quantity::operator*(const Quantity& other) const
{
    return Quantity(this->myValue * other.myValue, this->myUnit * other.myUnit);
}

Quantity Quantity::operator*(double factor) const
{
    return Quantity(this->myValue * factor, this->myUnit);
}

Quantity Quantity::operator/(const Quantity& other) const
{
    return Quantity(this->myValue / other.myValue, this->myUnit / other.myUnit);
}

Quantity Quantity::operator/(double factor) const
{
    return Quantity(this->myValue / factor, this->myUnit);
}

Quantity Quantity::pow(const Quantity& other) const
{
    if (!other.isDimensionless()) {
        throw Base::UnitsMismatchError("Quantity::pow(): exponent must not have a unit");
    }

    return Quantity(std::pow(this->myValue, other.myValue),
                    this->myUnit.pow(static_cast<signed char>(other.myValue)));
}

Quantity Quantity::pow(double exp) const
{
    return Quantity(std::pow(this->myValue, exp), this->myUnit.pow(exp));
}

Quantity Quantity::operator+(const Quantity& other) const
{
    if (this->myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator +(): Unit mismatch in plus operation");
    }

    return Quantity(this->myValue + other.myValue, this->myUnit);
}

Quantity& Quantity::operator+=(const Quantity& other)
{
    if (this->myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator +=(): Unit mismatch in plus operation");
    }

    myValue += other.myValue;

    return *this;
}

Quantity Quantity::operator-(const Quantity& other) const
{
    if (this->myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator -(): Unit mismatch in minus operation");
    }

    return Quantity(this->myValue - other.myValue, this->myUnit);
}

Quantity& Quantity::operator-=(const Quantity& other)
{
    if (this->myUnit != other.myUnit) {
        throw Base::UnitsMismatchError("Quantity::operator -=(): Unit mismatch in minus operation");
    }

    myValue -= other.myValue;

    return *this;
}

Quantity Quantity::operator-() const
{
    return Quantity(-(this->myValue), this->myUnit);
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

std::string
Quantity::getUserString(UnitsSchema* schema, double& factor, std::string& unitString) const
{
    return schema->translate(*this, factor, unitString);
}

std::string Quantity::getSafeUserString() const
{
    auto userStr = getUserString();
    if (myValue != 0.0 && parse(userStr).getValue() == 0) {
        auto unitStr = getUnit().getString();
        userStr = fmt::format("{}{}{}", myValue, unitStr.empty() ? "" : " ", unitStr);
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
const Quantity Quantity::NanoMetre              ( 1.0e-6                , Unit::Length                  );
const Quantity Quantity::MicroMetre             ( 1.0e-3                , Unit::Length                  );
const Quantity Quantity::MilliMetre             ( 1.0                   , Unit::Length                  );
const Quantity Quantity::CentiMetre             ( 10.0                  , Unit::Length                  );
const Quantity Quantity::DeciMetre              ( 100.0                 , Unit::Length                  );
const Quantity Quantity::Metre                  ( 1.0e3                 , Unit::Length                  );
const Quantity Quantity::KiloMetre              ( 1.0e6                 , Unit::Length                  );

const Quantity Quantity::MilliLiter             ( 1000.0                , Unit::Volume                  );
const Quantity Quantity::Liter                  ( 1.0e6                 , Unit::Volume                  );

const Quantity Quantity::Hertz                  ( 1.0                   , Unit::Frequency               );
const Quantity Quantity::KiloHertz              ( 1.0e3                 , Unit::Frequency               );
const Quantity Quantity::MegaHertz              ( 1.0e6                 , Unit::Frequency               );
const Quantity Quantity::GigaHertz              ( 1.0e9                 , Unit::Frequency               );
const Quantity Quantity::TeraHertz              ( 1.0e12                , Unit::Frequency               );

const Quantity Quantity::MicroGram              ( 1.0e-9                , Unit::Mass                    );
const Quantity Quantity::MilliGram              ( 1.0e-6                , Unit::Mass                    );
const Quantity Quantity::Gram                   ( 1.0e-3                , Unit::Mass                    );
const Quantity Quantity::KiloGram               ( 1.0                   , Unit::Mass                    );
const Quantity Quantity::Ton                    ( 1.0e3                 , Unit::Mass                    );

const Quantity Quantity::Second                 ( 1.0                   , Unit::TimeSpan                );
const Quantity Quantity::Minute                 ( 60.0                  , Unit::TimeSpan                );
const Quantity Quantity::Hour                   ( 3600.0                , Unit::TimeSpan                );

const Quantity Quantity::Ampere                 ( 1.0                   , Unit::ElectricCurrent         );
const Quantity Quantity::MilliAmpere            ( 0.001                 , Unit::ElectricCurrent         );
const Quantity Quantity::KiloAmpere             ( 1000.0                , Unit::ElectricCurrent         );
const Quantity Quantity::MegaAmpere             ( 1.0e6                 , Unit::ElectricCurrent         );

const Quantity Quantity::Kelvin                 ( 1.0                   , Unit::Temperature             );
const Quantity Quantity::MilliKelvin            ( 0.001                 , Unit::Temperature             );
const Quantity Quantity::MicroKelvin            ( 0.000001              , Unit::Temperature             );

const Quantity Quantity::MilliMole              ( 0.001                 , Unit::AmountOfSubstance       );
const Quantity Quantity::Mole                   ( 1.0                   , Unit::AmountOfSubstance       );

const Quantity Quantity::Candela                ( 1.0                   , Unit::LuminousIntensity       );

const Quantity Quantity::Inch                   ( 25.4                  , Unit::Length                  );
const Quantity Quantity::Foot                   ( 304.8                 , Unit::Length                  );
const Quantity Quantity::Thou                   ( 0.0254                , Unit::Length                  );
const Quantity Quantity::Yard                   ( 914.4                 , Unit::Length                  );
const Quantity Quantity::Mile                   ( 1609344.0             , Unit::Length                  );

const Quantity Quantity::MilePerHour            ( 447.04                , Unit::Velocity                );
const Quantity Quantity::SquareFoot             ( 92903.04              , Unit::Area                    );
const Quantity Quantity::CubicFoot              ( 28316846.592          , Unit::Volume                  );

const Quantity Quantity::Pound                  ( 0.45359237            , Unit::Mass                    );
const Quantity Quantity::Ounce                  ( 0.0283495231          , Unit::Mass                    );
const Quantity Quantity::Stone                  ( 6.35029318            , Unit::Mass                    );
const Quantity Quantity::Hundredweights         ( 50.80234544           , Unit::Mass                    );

const Quantity Quantity::PoundForce             ( 4448.22               , Unit::Force                   );  // lbf are ~= 4.44822 Newton

const Quantity Quantity::Newton                 ( 1000.0                , Unit::Force                   );  // Newton (kg*m/s^2)
const Quantity Quantity::MilliNewton            ( 1.0                   , Unit::Force                   );
const Quantity Quantity::KiloNewton             ( 1e+6                  , Unit::Force                   );
const Quantity Quantity::MegaNewton             ( 1e+9                  , Unit::Force                   );

const Quantity Quantity::NewtonPerMeter         ( 1.00                  , Unit::Stiffness               );  // Newton per meter (N/m or kg/s^2)
const Quantity Quantity::MilliNewtonPerMeter    ( 1e-3                  , Unit::Stiffness               );
const Quantity Quantity::KiloNewtonPerMeter     ( 1e3                   , Unit::Stiffness               );
const Quantity Quantity::MegaNewtonPerMeter     ( 1e6                   , Unit::Stiffness               );

const Quantity Quantity::Pascal                 ( 0.001                 , Unit::CompressiveStrength     );  // Pascal (kg/m/s^2 or N/m^2)
const Quantity Quantity::KiloPascal             ( 1.00                  , Unit::CompressiveStrength     );
const Quantity Quantity::MegaPascal             ( 1000.0                , Unit::CompressiveStrength     );
const Quantity Quantity::GigaPascal             ( 1e+6                  , Unit::CompressiveStrength     );

const Quantity Quantity::MilliBar               ( 0.1                   , Unit::CompressiveStrength     );
const Quantity Quantity::Bar                    ( 100.0                 , Unit::CompressiveStrength     );  // 1 bar = 100 kPa

const Quantity Quantity::Torr                   ( 101.325 / 760.0       , Unit::CompressiveStrength     );  // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)
const Quantity Quantity::mTorr                  ( 0.101325 / 760.0      , Unit::CompressiveStrength     );  // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)
const Quantity Quantity::yTorr                  ( 0.000101325 / 760.0   , Unit::CompressiveStrength     );  // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)

const Quantity Quantity::PSI                    ( 6.894744825494        , Unit::CompressiveStrength     );  // pounds/in^2
const Quantity Quantity::KSI                    ( 6894.744825494        , Unit::CompressiveStrength     );  // 1000 x pounds/in^2
const Quantity Quantity::MPSI                   ( 6894744.825494        , Unit::CompressiveStrength     );  // 1000 ksi

const Quantity Quantity::Watt                   ( 1e+6                  , Unit::Power                   );  // Watt (kg*m^2/s^3)
const Quantity Quantity::MilliWatt              ( 1e+3                  , Unit::Power                   );
const Quantity Quantity::KiloWatt               ( 1e+9                  , Unit::Power                   );
const Quantity Quantity::VoltAmpere             ( 1e+6                  , Unit::Power                   );  // VoltAmpere (kg*m^2/s^3)

const Quantity Quantity::Volt                   ( 1e+6                  , Unit::ElectricPotential       );  // Volt (kg*m^2/A/s^3)
const Quantity Quantity::MilliVolt              ( 1e+3                  , Unit::ElectricPotential       );
const Quantity Quantity::KiloVolt               ( 1e+9                  , Unit::ElectricPotential       );

const Quantity Quantity::MegaSiemens            ( 1.0                   , Unit::ElectricalConductance   );
const Quantity Quantity::KiloSiemens            ( 1e-3                  , Unit::ElectricalConductance   );
const Quantity Quantity::Siemens                ( 1e-6                  , Unit::ElectricalConductance   );  // Siemens (A^2*s^3/kg/m^2)
const Quantity Quantity::MilliSiemens           ( 1e-9                  , Unit::ElectricalConductance   );
const Quantity Quantity::MicroSiemens           ( 1e-12                 , Unit::ElectricalConductance   );

const Quantity Quantity::Ohm                    ( 1e+6                  , Unit::ElectricalResistance    );  // Ohm (kg*m^2/A^2/s^3)
const Quantity Quantity::KiloOhm                ( 1e+9                  , Unit::ElectricalResistance    );
const Quantity Quantity::MegaOhm                ( 1e+12                 , Unit::ElectricalResistance    );

const Quantity Quantity::Coulomb                ( 1.0                   , Unit::ElectricCharge          );  // Coulomb (A*s)

const Quantity Quantity::Tesla                  ( 1.0                   , Unit::MagneticFluxDensity     );  // Tesla (kg/s^2/A)
const Quantity Quantity::Gauss                  ( 1e-4                  , Unit::MagneticFluxDensity     );  // 1 G = 1e-4 T

const Quantity Quantity::Weber                  ( 1e6                   , Unit::MagneticFlux            );  // Weber (kg*m^2/s^2/A)

const Quantity Quantity::PicoFarad              ( 1e-18                 , Unit::ElectricalCapacitance   );
const Quantity Quantity::NanoFarad              ( 1e-15                 , Unit::ElectricalCapacitance   );
const Quantity Quantity::MicroFarad             ( 1e-12                 , Unit::ElectricalCapacitance   );
const Quantity Quantity::MilliFarad             ( 1e-9                  , Unit::ElectricalCapacitance   );
const Quantity Quantity::Farad                  ( 1e-6                  , Unit::ElectricalCapacitance   );  // Farad (s^4*A^2/m^2/kg)

const Quantity Quantity::NanoHenry              ( 1e-3                  , Unit::ElectricalInductance    );
const Quantity Quantity::MicroHenry             ( 1.0                   , Unit::ElectricalInductance    );
const Quantity Quantity::MilliHenry             ( 1e+3                  , Unit::ElectricalInductance    );
const Quantity Quantity::Henry                  ( 1e+6                  , Unit::ElectricalInductance    );  // Henry (kg*m^2/s^2/A^2)

const Quantity Quantity::Joule                  ( 1e+6                  , Unit::Moment                  );  // Joule (kg*m^2/s^2)
const Quantity Quantity::MilliJoule             ( 1e+3                  , Unit::Moment                  );
const Quantity Quantity::KiloJoule              ( 1e+9                  , Unit::Moment                  );
const Quantity Quantity::NewtonMeter            ( 1e+6                  , Unit::Moment                  );  // Joule (kg*m^2/s^2)
const Quantity Quantity::VoltAmpereSecond       ( 1e+6                  , Unit::Moment                  );  // Joule (kg*m^2/s^2)
const Quantity Quantity::WattSecond             ( 1e+6                  , Unit::Moment                  );  // Joule (kg*m^2/s^2)
const Quantity Quantity::KiloWattHour           ( 3.6e+12               , Unit::Moment                  );  // 1 kWh = 3.6e6 J
const Quantity Quantity::ElectronVolt           ( 1.602176634e-13       , Unit::Moment                  );  // 1 eV = 1.602176634e-19 J
const Quantity Quantity::KiloElectronVolt       ( 1.602176634e-10       , Unit::Moment                  );
const Quantity Quantity::MegaElectronVolt       ( 1.602176634e-7        , Unit::Moment                  );
const Quantity Quantity::Calorie                ( 4.1868e+6             , Unit::Moment                  );  // 1 cal = 4.1868 J
const Quantity Quantity::KiloCalorie            ( 4.1868e+9             , Unit::Moment                  );

const Quantity Quantity::KMH                    ( 277.778               , Unit::Velocity                );  // km/h
const Quantity Quantity::MPH                    ( 447.04                , Unit::Velocity                );  // Mile/h

const Quantity Quantity::AngMinute              ( 1.0 / 60.0            , Unit::Angle                   );  // angular minute
const Quantity Quantity::AngSecond              ( 1.0 / 3600.0          , Unit::Angle                   );  // angular second
const Quantity Quantity::Degree                 ( 1.0                   , Unit::Angle                   );  // degree (internal standard angle)
const Quantity Quantity::Radian                 ( 180 / std::numbers::pi, Unit::Angle                   );  // radian
const Quantity Quantity::Gon                    ( 360.0 / 400.0         , Unit::Angle                   );  // gon
// clang-format on

// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the 'Quantity's

// NOLINTNEXTLINE
Quantity QuantResult;

/* helper function for tuning number strings with groups in a locale agnostic way... */
// NOLINTBEGIN
double num_change(char* yytext, char dez_delim, char grp_delim)
{
    double ret_val {};
    const int num = 40;
    std::array<char, num> temp {};
    int iter = 0;
    for (char* ch = yytext; *ch != '\0'; ch++) {
        // skip group delimiter
        if (*ch == grp_delim) {
            continue;
        }
        // check for a dez delimiter other then dot
        if (*ch == dez_delim && dez_delim != '.') {
            temp[iter++] = '.';
        }
        else {
            temp[iter++] = *ch;
        }
        // check buffer overflow
        if (iter >= num) {
            return 0.0;
        }
    }

    temp[iter] = '\0';

    ret_val = atof(temp.data());
    return ret_val;
}
// NOLINTEND

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

// error func
void Quantity_yyerror(const char* errorinfo)
{
    throw Base::ParserError(errorinfo);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif

namespace QuantityParser
{

// NOLINTNEXTLINE
#define YYINITDEPTH 20
// show parser the lexer method
#define yylex QuantityLexer
int QuantityLexer();

// Parser, defined in Quantity.y
// NOLINTNEXTLINE
#include "Quantity.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in Quantity.l
// NOLINTNEXTLINE
#include "Quantity.lex.c"
#endif  // DOXYGEN_SHOULD_SKIP_THIS

class StringBufferCleaner
{
public:
    explicit StringBufferCleaner(YY_BUFFER_STATE buffer)
        : my_string_buffer {buffer}
    {}
    ~StringBufferCleaner()
    {
        // free the scan buffer
        yy_delete_buffer(my_string_buffer);
    }

    StringBufferCleaner(const StringBufferCleaner&) = delete;
    StringBufferCleaner(StringBufferCleaner&&) = delete;
    StringBufferCleaner& operator=(const StringBufferCleaner&) = delete;
    StringBufferCleaner& operator=(StringBufferCleaner&&) = delete;

private:
    YY_BUFFER_STATE my_string_buffer;
};

}  // namespace QuantityParser

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

Quantity Quantity::parse(const std::string& string)
{
    // parse from buffer
    QuantityParser::YY_BUFFER_STATE my_string_buffer =
        QuantityParser::yy_scan_string(string.c_str());
    QuantityParser::StringBufferCleaner cleaner(my_string_buffer);
    // set the global return variables
    QuantResult = Quantity(std::numeric_limits<double>::min());
    // run the parser
    QuantityParser::yyparse();

    return QuantResult;
}
