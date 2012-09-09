/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>
#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchemaInternal.h"
#include "UnitsSchemaImperial1.h"
#include "UnitsSchemaMKS.h"
#include "Math.h"
//#include "UnitsApiPy.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_E
#define M_E        2.71828182845904523536
#endif
#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif
#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif

using namespace Base;

// suppress annoying warnings from generated source files
#ifdef _MSC_VER
# pragma warning(disable : 4003)
# pragma warning(disable : 4018)
# pragma warning(disable : 4065)
# pragma warning( disable : 4273 )
# pragma warning(disable : 4335) // disable MAC file format warning on VC
#endif

// === names =============================================================

char *QuantityNames[] = {
        "length"      ,   
        "area"        ,   
        "volume"      ,   
        "angle"       , 
        "time span"    , 
        "velocity"    , 
        "acceleration", 
        "mass"        ,
        "temperature"
};

const QString  UnitsApi::getQuantityName(QuantityType t)
{
    // check limits
    assert(t<9);
    // returns 
    return QString::fromLatin1(QuantityNames[t]);
}
// === static attributes  ================================================

UnitsSchema  *UnitsApi::UserPrefSystem = new UnitsSchemaInternal();

double   UnitsApi::UserPrefFactor [50];
QString  UnitsApi::UserPrefUnit   [50];
int      UnitsApi::UserPrefDecimals = 2;

UnitsApi::UnitsApi(const char* filter)
{
    bool temp;
    Result = parse(filter);
}

UnitsApi::UnitsApi(const std::string& filter)
{
    bool temp;
    Result = parse(filter.c_str());
}

UnitsApi::~UnitsApi()
{
}

void UnitsApi::setSchema(UnitSystem s)
{
    delete UserPrefSystem;
    switch (s) {
        case SI1      : UserPrefSystem = new UnitsSchemaInternal(); break;
        case SI2      : UserPrefSystem = new UnitsSchemaMKS(); break;
        case Imperial1: UserPrefSystem = new UnitsSchemaImperial1(); break;
    }
    UserPrefSystem->setSchemaUnits();
}


UnitsSignature UnitsApi::translateUnit(const char* str)
{
    return parse(str);
}

UnitsSignature UnitsApi::translateUnit(const QString & str)
{
    return parse(str.toAscii());
}


// === static translation methodes ==========================================

QString UnitsApi::toStrWithUserPrefs(QuantityType t,double Value)
{
    return UserPrefSystem->toStrWithUserPrefs(t,Value);
    //double UnitValue = Value/UserPrefFactor[t];
    //return QString::fromAscii("%1 %2").arg(UnitValue).arg(UserPrefUnit[t]);
}

void UnitsApi::toStrWithUserPrefs(QuantityType t,double Value,QString &outValue,QString &outUnit)
{
    UserPrefSystem->toStrWithUserPrefs(t,Value,outValue,outUnit);
}

PyObject *UnitsApi::toPyWithUserPrefs(QuantityType t,double Value)
{
    return PyFloat_FromDouble(Value * UserPrefFactor[t]);
}

double UnitsApi::toDblWithUserPrefs(QuantityType t,const QString & Str)
{
    return toDblWithUserPrefs(t,(const char*) Str.toUtf8());
}

double UnitsApi::toDblWithUserPrefs(QuantityType t,const char* Str)
{
    UnitsSignature Value = parse( Str ); 

    if (Value.UsedUnit())
        return Value;
    else
        return toDblWithUserPrefs(t,Value);
}

double UnitsApi::toDblWithUserPrefs(QuantityType t,double UserVal)
{
    return UserVal*UserPrefFactor[t];
}

double UnitsApi::toDblWithUserPrefs(QuantityType t,PyObject *ArgObj)
{
    if (PyString_Check(ArgObj)) 
        return toDblWithUserPrefs(t,PyString_AsString(ArgObj));
    else if (PyFloat_Check(ArgObj))
        return toDblWithUserPrefs(t,PyFloat_AsDouble(ArgObj));
    else if (PyInt_Check(ArgObj))
        return toDblWithUserPrefs(t,(double)PyInt_AsLong(ArgObj));
    else
        throw Base::Exception("Wrong parameter type!");
}

void UnitsApi::setPrefOf(QuantityType t,const char* Str)
{
    double Factor = translateUnit(Str);
    UserPrefUnit[t] = QString::fromLatin1(Str);
    UserPrefFactor[t] = Factor;
}

const QString & UnitsApi::getPrefUnitOf(QuantityType t)
{
    return UserPrefUnit[t];
}

const double UnitsApi::getPrefFactorOf(QuantityType t)
{
    return UserPrefFactor[t];
}

void UnitsApi::setDecimals(int prec)
{
    UserPrefDecimals = prec;
}

int UnitsApi::getDecimals()
{
    return UserPrefDecimals;
}

void UnitsApi::setDefaults(void)
{
    setPrefOf( Length       ,"mm"       );
    setPrefOf( Area         ,"mm^2"     );
    setPrefOf( Volume       ,"mm^3"     );
    setPrefOf( Angle        ,"deg"      );
    setPrefOf( TimeSpan     ,"s"        );
    setPrefOf( Velocity     ,"mm/s"     );
    setPrefOf( Acceleration ,"mm/s^2"   );
    setPrefOf( Mass         ,"kg"       );
    setPrefOf( Temperature  ,"K"        );
  
}

// === UnitsSignature =======================================================

UnitsSignature::UnitsSignature(
    double      Value,
    double      Spread,
    signed char Length,
    signed char Mass,
    signed char Time,
    signed char ElectricCurrent,
    signed char ThermodynamicTemperature,
    signed char AmountOfSubstance,
    signed char LuminoseIntensity,
    signed char Angle
    )
    :Value(Value),
    Spread(Spread),
    Length(Length),
    Mass(Mass),
    Time(Time),
    ElectricCurrent(ElectricCurrent),
    ThermodynamicTemperature(ThermodynamicTemperature),
    AmountOfSubstance(AmountOfSubstance),
    LuminoseIntensity(LuminoseIntensity),
    Angle(Angle)
{}

UnitsSignature UnitsSignature::pow(double num)const
{
    return UnitsSignature(
        std::pow(Value,num)
        
        );
}

UnitsSignature UnitsSignature::pow(UnitsSignature num)const
{
    return UnitsSignature(
        std::pow(Value,num.Value)
        
        );
}

UnitsSignature UnitsSignature::neg(void)const
{
    return UnitsSignature(-Value);
}

UnitsSignature UnitsSignature::operator*(const UnitsSignature& right)const 
{
    UnitsSignature result(0);
    if(Value==DOUBLE_MAX || right.Value==DOUBLE_MAX)
        result.Value = DOUBLE_MAX;
    else
        result.Value = Value * right.Value;

    result.Spread = Spread * right.Spread;
    
    result.Length                   = Length                    + right.Length;
    result.Mass                     = Mass                      + right.Mass;
    result.Time                     = Time                      + right.Time;
    result.ElectricCurrent          = ElectricCurrent           + right.ElectricCurrent;
    result.ThermodynamicTemperature = ThermodynamicTemperature  + right.ThermodynamicTemperature;
    result.AmountOfSubstance        = AmountOfSubstance         + right.AmountOfSubstance;
    result.LuminoseIntensity        = LuminoseIntensity         + right.LuminoseIntensity;
    result.Angle                    = Angle                     + right.Angle;

    return result;
}

UnitsSignature UnitsSignature::operator/(const UnitsSignature& right)const 
{
    UnitsSignature result(0);
    if(Value==DOUBLE_MAX || right.Value==DOUBLE_MAX)
        result.Value = DOUBLE_MAX;
    else
        result.Value = Value / right.Value;

    result.Spread = Spread / right.Spread;
    
    result.Length                   = Length                    - right.Length;
    result.Mass                     = Mass                      - right.Mass;
    result.Time                     = Time                      - right.Time;
    result.ElectricCurrent          = ElectricCurrent           - right.ElectricCurrent;
    result.ThermodynamicTemperature = ThermodynamicTemperature  - right.ThermodynamicTemperature;
    result.AmountOfSubstance        = AmountOfSubstance         - right.AmountOfSubstance;
    result.LuminoseIntensity        = LuminoseIntensity         - right.LuminoseIntensity;
    result.Angle                    = Angle                     - right.Angle;

    return result;
}

UnitsSignature UnitsSignature::operator-(const UnitsSignature& right)const 
{
    return UnitsSignature (Value-right.Value);
}

UnitsSignature UnitsSignature::operator+(const UnitsSignature& right)const 
{
    return UnitsSignature (Value+right.Value);
}

// === Parser & Scanner stuff ===============================================

// include the Scanner and the Parser for the filter language

UnitsSignature ScanResult;

// error func
void Unit_yyerror(char *errorinfo)
{  throw Base::Exception(errorinfo);  }


// for VC9 (isatty and fileno not supported anymore)
#ifdef _MSC_VER
int isatty (int i) {return _isatty(i);}
int fileno(FILE *stream) {return _fileno(stream);}
#endif

namespace UnitParser {

// show the parser the lexer method
#define yylex UnitsApilex
int UnitsApilex(void);

// Parser, defined in UnitsApi.y
#include "UnitsApiParser.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in UnitsApi.l
#include "UnitsApiLexer.c"
#endif // DOXYGEN_SHOULD_SKIP_THIS
}

UnitsSignature UnitsApi::parse(const char* buffer)
{
    // parse from buffer
    UnitParser::YY_BUFFER_STATE my_string_buffer = UnitParser::UnitsApi_scan_string (buffer);
    // set the global return variables
    ScanResult = DOUBLE_MIN;
    // run the parser
    UnitParser::Unit_yyparse ();
    // free the scan buffer
    UnitParser::UnitsApi_delete_buffer (my_string_buffer);

    if (ScanResult == DOUBLE_MIN)
        throw Base::Exception("Unknown error in Unit expression");
    return ScanResult;
}
