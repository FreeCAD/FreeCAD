/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <sstream>
#include <iomanip>
#include <memory>

#include <CXX/WrapPython.h>
#include <fmt/format.h>
#include <QString>
#include "Exception.h"

#include "UnitsApi.h"
#include "UnitsSchemaCentimeters.h"
#include "UnitsSchemaInternal.h"
#include "UnitsSchemaImperial1.h"
#include "UnitsSchemaMKS.h"
#include "UnitsSchemaMmMin.h"
#include "UnitsSchemaFemMilliMeterNewton.h"
#include "UnitsSchemaMeterDecimal.h"

using namespace Base;

// === static attributes  ================================================

UnitsSchemaPtr UnitsApi::UserPrefSystem(new UnitsSchemaInternal());
UnitSystem UnitsApi::currentSystem = UnitSystem::SI1;

int UnitsApi::UserPrefDecimals = 2;

QString UnitsApi::getDescription(UnitSystem system)
{
    switch (system) {
        case UnitSystem::SI1:
            return tr("Standard (mm, kg, s, °)");
        case UnitSystem::SI2:
            return tr("MKS (m, kg, s, °)");
        case UnitSystem::Imperial1:
            return tr("US customary (in, lb)");
        case UnitSystem::ImperialDecimal:
            return tr("Imperial decimal (in, lb)");
        case UnitSystem::Centimeters:
            return tr("Building Euro (cm, m², m³)");
        case UnitSystem::ImperialBuilding:
            return tr("Building US (ft-in, sqft, cft)");
        case UnitSystem::MmMin:
            return tr("Metric small parts & CNC (mm, mm/min)");
        case UnitSystem::ImperialCivil:
            return tr("Imperial for Civil Eng (ft, ft/s)");
        case UnitSystem::FemMilliMeterNewton:
            return tr("FEM (mm, N, s)");
        case UnitSystem::MeterDecimal:
            return tr("Meter decimal (m, m², m³)");
        default:
            return tr("Unknown schema");
    }
}

UnitsSchemaPtr UnitsApi::createSchema(UnitSystem system)
{
    switch (system) {
        case UnitSystem::SI1:
            return std::make_unique<UnitsSchemaInternal>();
        case UnitSystem::SI2:
            return std::make_unique<UnitsSchemaMKS>();
        case UnitSystem::Imperial1:
            return std::make_unique<UnitsSchemaImperial1>();
        case UnitSystem::ImperialDecimal:
            return std::make_unique<UnitsSchemaImperialDecimal>();
        case UnitSystem::Centimeters:
            return std::make_unique<UnitsSchemaCentimeters>();
        case UnitSystem::ImperialBuilding:
            return std::make_unique<UnitsSchemaImperialBuilding>();
        case UnitSystem::MmMin:
            return std::make_unique<UnitsSchemaMmMin>();
        case UnitSystem::ImperialCivil:
            return std::make_unique<UnitsSchemaImperialCivil>();
        case UnitSystem::FemMilliMeterNewton:
            return std::make_unique<UnitsSchemaFemMilliMeterNewton>();
        case UnitSystem::MeterDecimal:
            return std::make_unique<UnitsSchemaMeterDecimal>();
        default:
            break;
    }

    return nullptr;
}

void UnitsApi::setSchema(UnitSystem system)
{
    if (UserPrefSystem) {
        UserPrefSystem->resetSchemaUnits();  // for schemas changed the Quantity constants
    }

    UserPrefSystem = createSchema(system);
    currentSystem = system;

    // for wrong value fall back to standard schema
    if (!UserPrefSystem) {
        UserPrefSystem = std::make_unique<UnitsSchemaInternal>();
        currentSystem = UnitSystem::SI1;
    }

    UserPrefSystem->setSchemaUnits();  // if necessary a unit schema can change the constants in
                                       // Quantity (e.g. mi=1.8km rather then 1.6km).
}

std::string UnitsApi::toString(const Base::Quantity& quantity, const QuantityFormat& format)
{
    return fmt::format("'{} {}'", toNumber(quantity, format), quantity.getUnit().getString());
}

std::string UnitsApi::toNumber(const Base::Quantity& quantity, const QuantityFormat& format)
{
    return toNumber(quantity.getValue(), format);
}

std::string UnitsApi::toNumber(double value, const QuantityFormat& format)
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
    ss << std::setprecision(format.precision) << value;

    return ss.str();
}

// return true if the current user schema uses multiple units for length (ex. Ft/In)
bool UnitsApi::isMultiUnitLength()
{
    return UserPrefSystem->isMultiUnitLength();
}

// return true if the current user schema uses multiple units for angles (ex. DMS)
bool UnitsApi::isMultiUnitAngle()
{
    return UserPrefSystem->isMultiUnitAngle();
}

std::string UnitsApi::getBasicLengthUnit()
{
    return UserPrefSystem->getBasicLengthUnit();
}

// === static translation methods ==========================================

std::string
UnitsApi::schemaTranslate(const Base::Quantity& quant, double& factor, std::string& unitString)
{
    return UserPrefSystem->schemaTranslate(quant, factor, unitString);
}

double UnitsApi::toDouble(PyObject* args, const Base::Unit& u)
{
    if (PyUnicode_Check(args)) {
        std::string str(PyUnicode_AsUTF8(args));
        // Parse the string
        Quantity q = Quantity::parse(str);
        if (q.getUnit() == u) {
            return q.getValue();
        }
        throw Base::UnitsMismatchError("Wrong unit type!");
    }

    if (PyFloat_Check(args)) {
        return PyFloat_AsDouble(args);
    }
    if (PyLong_Check(args)) {
        return static_cast<double>(PyLong_AsLong(args));
    }

    throw Base::UnitsMismatchError("Wrong parameter type!");
}

Quantity UnitsApi::toQuantity(PyObject* args, const Base::Unit& u)
{
    double d {};
    if (PyUnicode_Check(args)) {
        std::string str(PyUnicode_AsUTF8(args));
        // Parse the string
        Quantity q = Quantity::parse(str);
        d = q.getValue();
    }
    else if (PyFloat_Check(args)) {
        d = PyFloat_AsDouble(args);
    }
    else if (PyLong_Check(args)) {
        d = static_cast<double>(PyLong_AsLong(args));
    }
    else {
        throw Base::UnitsMismatchError("Wrong parameter type!");
    }

    return Quantity(d, u);
}

void UnitsApi::setDecimals(int prec)
{
    UserPrefDecimals = prec;
}

int UnitsApi::getDecimals()
{
    return UserPrefDecimals;
}
