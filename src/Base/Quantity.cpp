/***************************************************************************
 *   Copyright (c) 2013 Juergen Riegel                                     *
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
# ifdef FC_OS_WIN32
# define _USE_MATH_DEFINES
# endif // FC_OS_WIN32
# include <cmath>
#endif

#include "Quantity.h"
#include "Exception.h"
#include "UnitsApi.h"
#include "Console.h"

// suppress annoying warnings from generated source files
#ifdef _MSC_VER
# pragma warning(disable : 4003)
# pragma warning(disable : 4018)
# pragma warning(disable : 4065)
# pragma warning( disable : 4273 )
# pragma warning(disable : 4335) // disable MAC file format warning on VC
#endif

using namespace Base;

// ====== Static attributes =========================
int QuantityFormat::defaultDenominator = 8; // for 1/8"


QuantityFormat::QuantityFormat()
  : option(OmitGroupSeparator | RejectGroupSeparator)
  , format(Fixed)
  , precision(UnitsApi::getDecimals())
  , denominator(defaultDenominator)
{
}

// ----------------------------------------------------------------------------

Quantity::Quantity()
{
    this->_Value = 0.0;
}

Quantity::Quantity(const Quantity& that)
{
    *this = that ;
}

Quantity::Quantity(double Value, const Unit& unit)
{
    this->_Unit = unit;
    this->_Value = Value;
}

double Quantity::getValueAs(const Quantity &q)const
{
    return _Value/q.getValue();
}

bool Quantity::operator ==(const Quantity& that) const
{
    return (this->_Value == that._Value) && (this->_Unit == that._Unit) ;
}

bool Quantity::operator <(const Quantity& that) const
{
    if (this->_Unit != that._Unit)
        throw Base::UnitsMismatchError("Quantity::operator <(): quantities need to have same unit to compare");

    return (this->_Value < that._Value) ;
}

bool Quantity::operator >(const Quantity& that) const
{
    if (this->_Unit != that._Unit)
        throw Base::UnitsMismatchError("Quantity::operator >(): quantities need to have same unit to compare");

    return (this->_Value > that._Value) ;
}

bool Quantity::operator <=(const Quantity& that) const
{
    if (this->_Unit != that._Unit)
        throw Base::UnitsMismatchError("Quantity::operator <=(): quantities need to have same unit to compare");

    return (this->_Value <= that._Value) ;
}

bool Quantity::operator >=(const Quantity& that) const
{
    if (this->_Unit != that._Unit)
        throw Base::UnitsMismatchError("Quantity::operator >=(): quantities need to have same unit to compare");

    return (this->_Value >= that._Value) ;
}

Quantity Quantity::operator *(const Quantity &p) const
{
    return Quantity(this->_Value * p._Value,this->_Unit * p._Unit);
}

Quantity Quantity::operator *(double p) const
{
    return Quantity(this->_Value * p,this->_Unit);
}

Quantity Quantity::operator /(const Quantity &p) const
{
    return Quantity(this->_Value / p._Value,this->_Unit / p._Unit);
}

Quantity Quantity::operator /(double p) const
{
    return Quantity(this->_Value / p,this->_Unit);
}

Quantity Quantity::pow(const Quantity &p) const
{
    if (!p._Unit.isEmpty())
        throw Base::UnitsMismatchError("Quantity::pow(): exponent must not have a unit");
    return Quantity(
        std::pow(this->_Value, p._Value),
        this->_Unit.pow((short)p._Value)
        );
}

Quantity Quantity::pow(double p) const
{
    return Quantity(
        std::pow(this->_Value, p),
        this->_Unit.pow((short)p)
        );
}

Quantity Quantity::operator +(const Quantity &p) const
{
    if (this->_Unit != p._Unit)
        throw Base::UnitsMismatchError("Quantity::operator +(): Unit mismatch in plus operation");
    return Quantity(this->_Value + p._Value,this->_Unit);
}

Quantity& Quantity::operator +=(const Quantity &p)
{
    if (this->_Unit != p._Unit)
        throw Base::UnitsMismatchError("Quantity::operator +=(): Unit mismatch in plus operation");

    _Value += p._Value;

    return *this;
}

Quantity Quantity::operator -(const Quantity &p) const
{
    if (this->_Unit != p._Unit)
        throw Base::UnitsMismatchError("Quantity::operator -(): Unit mismatch in minus operation");
    return Quantity(this->_Value - p._Value,this->_Unit);
}

Quantity& Quantity::operator -=(const Quantity &p)
{
    if (this->_Unit != p._Unit)
        throw Base::UnitsMismatchError("Quantity::operator -=(): Unit mismatch in minus operation");

    _Value -= p._Value;

    return *this;
}

Quantity Quantity::operator -(void) const
{
    return Quantity(-(this->_Value),this->_Unit);
}

Quantity& Quantity::operator = (const Quantity &New)
{
    this->_Value = New._Value;
    this->_Unit = New._Unit;
    this->_Format = New._Format;
    return *this;
}

QString Quantity::getUserString(double& factor, QString& unitString) const
{
    return Base::UnitsApi::schemaTranslate(*this, factor, unitString);
}

/// true if it has a number without a unit
bool Quantity::isDimensionless(void)const
{
    return _Value != DOUBLE_MIN && _Unit.isEmpty();
}

// true if it has a number and a valid unit
bool Quantity::isQuantity(void)const
{
    return _Value != DOUBLE_MIN && !_Unit.isEmpty();
}
// true if it has a number with or without a unit
bool Quantity::isValid(void)const
{
    return _Value != DOUBLE_MIN ;
}

void Quantity::setInvalid(void)
{
    _Value = DOUBLE_MIN ;
}

// === Predefined types =====================================================

Quantity Quantity::NanoMetre        (1.0e-6         ,Unit(1));
Quantity Quantity::MicroMetre       (1.0e-3         ,Unit(1));
Quantity Quantity::MilliMetre       (1.0            ,Unit(1));
Quantity Quantity::CentiMetre       (10.0           ,Unit(1));
Quantity Quantity::DeciMetre        (100.0          ,Unit(1));
Quantity Quantity::Metre            (1.0e3          ,Unit(1));
Quantity Quantity::KiloMetre        (1.0e6          ,Unit(1));

Quantity Quantity::Liter            (1000000.0      ,Unit(3));

Quantity Quantity::MicroGram        (1.0e-9         ,Unit(0,1));
Quantity Quantity::MilliGram        (1.0e-6         ,Unit(0,1));
Quantity Quantity::Gram             (1.0e-3         ,Unit(0,1));
Quantity Quantity::KiloGram         (1.0            ,Unit(0,1));
Quantity Quantity::Ton              (1.0e3          ,Unit(0,1));

Quantity Quantity::Second           (1.0            ,Unit(0,0,1));
Quantity Quantity::Minute           (60.0           ,Unit(0,0,1));
Quantity Quantity::Hour             (3600.0         ,Unit(0,0,1));

Quantity Quantity::Ampere           (1.0           ,Unit(0,0,0,1));
Quantity Quantity::MilliAmpere      (0.001         ,Unit(0,0,0,1));
Quantity Quantity::KiloAmpere       (1000.0        ,Unit(0,0,0,1));
Quantity Quantity::MegaAmpere       (1.0e6         ,Unit(0,0,0,1));

Quantity Quantity::Kelvin           (1.0           ,Unit(0,0,0,0,1));
Quantity Quantity::MilliKelvin      (0.001         ,Unit(0,0,0,0,1));
Quantity Quantity::MicroKelvin      (0.000001      ,Unit(0,0,0,0,1));

Quantity Quantity::Mole             (1.0           ,Unit(0,0,0,0,0,1));

Quantity Quantity::Candela          (1.0           ,Unit(0,0,0,0,0,0,1));

Quantity Quantity::Inch             (25.4          ,Unit(1));
Quantity Quantity::Foot             (304.8         ,Unit(1));
Quantity Quantity::Thou             (0.0254        ,Unit(1));
Quantity Quantity::Yard             (914.4         ,Unit(1));
Quantity Quantity::Mile             (1609344.0     ,Unit(1));

Quantity Quantity::Pound            (0.45359237    ,Unit(0,1));
Quantity Quantity::Ounce            (0.0283495231  ,Unit(0,1));
Quantity Quantity::Stone            (6.35029318    ,Unit(0,1));
Quantity Quantity::Hundredweights   (50.80234544   ,Unit(0,1));

Quantity Quantity::PoundForce       (224.81        ,Unit(1,1,-2));  // Newton  are ~= 0.22481 lbF

Quantity Quantity::Newton           (1000.0        ,Unit(1,1,-2));  // Newton (kg*m/s^2)
Quantity Quantity::KiloNewton       (1e+6          ,Unit(1,1,-2));
Quantity Quantity::MegaNewton       (1e+9          ,Unit(1,1,-2));
Quantity Quantity::MilliNewton      (1.0           ,Unit(1,1,-2));

Quantity Quantity::Pascal           (0.001         ,Unit(-1,1,-2)); // Pascal (kg/m/s^2 or N/m^2)
Quantity Quantity::KiloPascal       (1.00          ,Unit(-1,1,-2));
Quantity Quantity::MegaPascal       (1000.0        ,Unit(-1,1,-2));
Quantity Quantity::GigaPascal       (1e+6          ,Unit(-1,1,-2));

Quantity Quantity::Torr             (101.325/760.0 ,Unit(-1,1,-2)); // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)
Quantity Quantity::mTorr            (0.101325/760.0,Unit(-1,1,-2)); // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)
Quantity Quantity::yTorr            (0.000101325/760.0 ,Unit(-1,1,-2)); // Torr is a defined fraction of Pascal (kg/m/s^2 or N/m^2)

Quantity Quantity::PSI              (6.894744825494,Unit(-1,1,-2)); // pounds/in^2
Quantity Quantity::KSI              (6894.744825494,Unit(-1,1,-2)); // 1000 x pounds/in^2

Quantity Quantity::Watt             (1e+6          ,Unit(2,1,-3));  // Watt (kg*m^2/s^3)
Quantity Quantity::VoltAmpere       (1e+6          ,Unit(2,1,-3));  // VoltAmpere (kg*m^2/s^3)

Quantity Quantity::Volt             (1e+6          ,Unit(2,1,-3,-1));  // Volt (kg*m^2/A/s^3)

Quantity Quantity::Joule            (1e+6          ,Unit(2,1,-2));  // Joule (kg*m^2/s^2)
Quantity Quantity::NewtonMeter      (1e+6          ,Unit(2,1,-2));  // Joule (kg*m^2/s^2)
Quantity Quantity::VoltAmpereSecond (1e+6          ,Unit(2,1,-2));  // Joule (kg*m^2/s^2)
Quantity Quantity::WattSecond       (1e+6          ,Unit(2,1,-2));  // Joule (kg*m^2/s^2)

Quantity Quantity::KMH              (277.778       ,Unit(1,0,-1));  // km/h
Quantity Quantity::MPH              (447.04        ,Unit(1,0,-1));  // Mile/h

Quantity Quantity::AngMinute        (1.0/60.0      ,Unit(0,0,0,0,0,0,0,1)); // angular minute
Quantity Quantity::AngSecond        (1.0/3600.0    ,Unit(0,0,0,0,0,0,0,1)); // angular minute
Quantity Quantity::Degree           (1.0           ,Unit(0,0,0,0,0,0,0,1)); // degree         (internal standard angle)
Quantity Quantity::Radian           (180/M_PI      ,Unit(0,0,0,0,0,0,0,1)); // radian
Quantity Quantity::Gon              (360.0/400.0   ,Unit(0,0,0,0,0,0,0,1)); // gon




// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the Quantitys

Quantity QuantResult;

/* helper function for tuning number strings with groups in a locale agnostic way... */
double num_change(char* yytext,char dez_delim,char grp_delim)
{
    double ret_val;
    char temp[40];
    int i = 0;
    for(char* c=yytext;*c!='\0';c++){ 
        // skip group delimiter
        if(*c==grp_delim) continue;
        // check for a dez delimiter other then dot
        if(*c==dez_delim && dez_delim !='.')
             temp[i++] = '.';
        else
            temp[i++] = *c; 
        // check buffer overflow
        if (i>39) return 0.0;
    }
    temp[i] = '\0';
    
    ret_val = atof( temp ); 
    return ret_val;
};

// error func
void Quantity_yyerror(char *errorinfo)
{
    throw Base::ParserError(errorinfo);
}


// for VC9 (isatty and fileno not supported anymore)
//#ifdef _MSC_VER
//int isatty (int i) {return _isatty(i);}
//int fileno(FILE *stream) {return _fileno(stream);}
//#endif

namespace QuantityParser {

#define YYINITDEPTH 20
// show parser the lexer method
#define yylex QuantityLexer
int QuantityLexer(void);

// Parser, defined in QuantityParser.y
#include "QuantityParser.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in QuantityParser.l
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wsign-compare"
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "QuantityLexer.c"
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif
#endif // DOXYGEN_SHOULD_SKIP_THIS
}

Quantity Quantity::parse(const QString &string)
{
    // parse from buffer
    QuantityParser::YY_BUFFER_STATE my_string_buffer = QuantityParser::yy_scan_string (string.toUtf8().data());
    // set the global return variables
    QuantResult = Quantity(DOUBLE_MIN);
    // run the parser
    QuantityParser::yyparse ();
    // free the scan buffer
    QuantityParser::yy_delete_buffer (my_string_buffer);

    //if (QuantResult == Quantity(DOUBLE_MIN))
    //    throw Base::ParserError("Unknown error in Quantity expression");
    return QuantResult;
}
