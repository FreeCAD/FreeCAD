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
#include "UnitsSchemaCentimeters.h"
#include "UnitsSchemaMmMin.h"

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


//const QString  UnitsApi::getQuantityName(QuantityType t)
//{
//    // check limits
//    assert(t<9);
//    // returns 
//    return QString::fromLatin1(QuantityNames[t]);
//}
// === static attributes  ================================================
double UnitsApi::defaultFactor = 1.0;

UnitsSchema  *UnitsApi::UserPrefSystem = new UnitsSchemaInternal();
UnitSystem    UnitsApi::actSystem = SI1;

//double   UnitsApi::UserPrefFactor [50];
//QString  UnitsApi::UserPrefUnit   [50];
int      UnitsApi::UserPrefDecimals = 2;

UnitsApi::UnitsApi(const char* /*filter*/)
{
}

UnitsApi::UnitsApi(const std::string& /*filter*/)
{
}

UnitsApi::~UnitsApi()
{
}

const char* UnitsApi::getDescription(UnitSystem system)
{
    switch (system) {
    case SI1:
        return "Standard (mm/kg/s/degree)";
    case SI2:
        return "MKS (m/kg/s/degree)";
    case Imperial1:
        return "US customary (in/lb)";
    case ImperialDecimal:
        return "Imperial decimal (in/lb)";
    case Centimeters:
        return "Building Euro (cm/m²/m³)";
    case ImperialBuilding:
        return "Building US (ft-in/sqft/cuft)";
    case MmMin:
        return "Metric small parts & CNC(mm, mm/min)";
    case ImperialCivil:
        return "Imperial for Civil Eng (ft, ft/sec)";
    default:
        return "Unknown schema";
    }
}

UnitsSchema* UnitsApi::createSchema(UnitSystem s)
{
    switch (s) {
    case SI1:
        return new UnitsSchemaInternal();
    case SI2:
        return new UnitsSchemaMKS();
    case Imperial1:
        return new UnitsSchemaImperial1();
    case ImperialDecimal:
        return new UnitsSchemaImperialDecimal();
    case Centimeters:
        return new UnitsSchemaCentimeters();
    case ImperialBuilding:
        return new UnitsSchemaImperialBuilding();
    case MmMin:
        return new UnitsSchemaMmMin();
    case ImperialCivil:
        return new UnitsSchemaImperialCivil();
    default:
        break;
    }

    return 0;
}

void UnitsApi::setSchema(UnitSystem s)
{
    if (UserPrefSystem) {
        UserPrefSystem->resetSchemaUnits(); // for schemas changed the Quantity constants
        delete UserPrefSystem;
        UserPrefSystem = 0;
    }

    UserPrefSystem = createSchema(s);
    actSystem = s;

    // for wrong value fall back to standard schema
    if (!UserPrefSystem) {
        UserPrefSystem = new UnitsSchemaInternal();
        actSystem = SI1;
    }

    UserPrefSystem->setSchemaUnits(); // if necessary a unit schema can change the constants in Quantity (e.g. mi=1.8km rather then 1.6km).
}


//double UnitsApi::translateUnit(const char* str)
//{
//    bool temp;
//    return parse(str,temp );
//}
//
//double UnitsApi::translateUnit(const QString & str)
//{
//    bool temp;
//    return parse(str.toUtf8() ,temp);
//}
//

// === static translation methods ==========================================

QString UnitsApi::schemaTranslate(const Base::Quantity& quant, double &factor, QString &unitString)
{
    return UserPrefSystem->schemaTranslate(quant,factor,unitString);
}


//QString UnitsApi::toStrWithUserPrefs(QuantityType t,double Value)
//{
//    return UserPrefSystem->toStrWithUserPrefs(t,Value);
//    //double UnitValue = Value/UserPrefFactor[t];
//    //return QString::fromLatin1("%1 %2").arg(UnitValue).arg(UserPrefUnit[t]);
//}
//
//void UnitsApi::toStrWithUserPrefs(QuantityType t,double Value,QString &outValue,QString &outUnit)
//{
//    UserPrefSystem->toStrWithUserPrefs(t,Value,outValue,outUnit);
//}
//
//PyObject *UnitsApi::toPyWithUserPrefs(QuantityType t,double Value)
//{
//    return PyFloat_FromDouble(Value * UserPrefFactor[t]);
//}
//

double UnitsApi::toDbl(PyObject *ArgObj, const Base::Unit &u)
{
#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(ArgObj)) {
        QString str = QString::fromUtf8(PyUnicode_AsUTF8(ArgObj));
#else
    if (PyString_Check(ArgObj)) {
        QString str = QString::fromLatin1(PyString_AsString(ArgObj));
#endif
        // Parse the string
        Quantity q = Quantity::parse(str);
        if (q.getUnit() == u)
            return q.getValue();
        throw Base::UnitsMismatchError("Wrong unit type!");
    }
    else if (PyFloat_Check(ArgObj)) {
        return PyFloat_AsDouble(ArgObj);
    }
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(ArgObj)) {
        return static_cast<double>(PyInt_AsLong(ArgObj));
#else
    else if (PyLong_Check(ArgObj)) {
        return static_cast<double>(PyLong_AsLong(ArgObj));
#endif
    }
    else {
        throw Base::UnitsMismatchError("Wrong parameter type!");
    }
}

Quantity UnitsApi::toQuantity(PyObject *ArgObj, const Base::Unit &u)
{
    double d;
#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(ArgObj)) {
        QString str = QString::fromUtf8(PyUnicode_AsUTF8(ArgObj));
#else
    if (PyString_Check(ArgObj)) {
        QString str = QString::fromLatin1(PyString_AsString(ArgObj));
#endif
        // Parse the string
        Quantity q = Quantity::parse(str);
        d = q.getValue();
    }
    else if (PyFloat_Check(ArgObj)) {
        d = PyFloat_AsDouble(ArgObj);
    }
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(ArgObj)) {
        d = static_cast<double>(PyInt_AsLong(ArgObj));
#else
    else if (PyLong_Check(ArgObj)) {
        d = static_cast<double>(PyLong_AsLong(ArgObj));
#endif
    }
    else {
        throw Base::UnitsMismatchError("Wrong parameter type!");
    }

    return Quantity(d,u);
}

void UnitsApi::setDecimals(int prec)
{
    UserPrefDecimals = prec;
}

int UnitsApi::getDecimals()
{
    return UserPrefDecimals;
}

