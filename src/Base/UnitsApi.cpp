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

#include <iostream>
#include <iomanip>

#include <fmt/format.h>

#include "UnitsApi.h"
#include "UnitsSchema.h"
#include "UnitsSchemas.h"
#include "UnitsSchemasData.h"
#include "App/Application.h"

using Base::UnitsApi;
using Base::UnitsSchema;
using Base::UnitsSchemas;


void UnitsApi::init()
{
    schemas = std::make_unique<UnitsSchemas>(UnitsSchemasData::unitSchemasDataPack);
}

std::vector<std::string> UnitsApi::getDescriptions()
{
    return schemas->descriptions();
}

std::vector<std::string> UnitsApi::getNames()
{
    return schemas->names();
}

std::size_t UnitsApi::count()
{
    return static_cast<int>(schemas->count());
}

bool UnitsApi::isMultiUnitAngle()
{
    return schemas->currentSchema()->isMultiUnitAngle();
}

bool UnitsApi::isMultiUnitLength()
{
    return schemas->currentSchema()->isMultiUnitLength();
}

std::string UnitsApi::getBasicLengthUnit()
{
    return schemas->currentSchema()->getBasicLengthUnit();
}

std::size_t UnitsApi::getFractDenominator()
{
    return schemas->defFractDenominator();
}

std::unique_ptr<UnitsSchema> UnitsApi::createSchema(const std::size_t num)
{
    return std::make_unique<UnitsSchema>(schemas->spec(num));
}

void UnitsApi::setSchema(const std::string& name)
{
    schemas->select(name);
}

void UnitsApi::setSchema(const size_t num)
{
    schemas->select(num);
}

std::string UnitsApi::toString(const Quantity& quantity, const QuantityFormat& format)
{
    // return fmt::format("'{} {}'", toNumber(quantity, format), quantity.getUnit().getString());
    return fmt::format("'{}{}'", toNumber(quantity, format), quantity.getUnit().getString());
}

std::string UnitsApi::toNumber(const Quantity& quantity, const QuantityFormat& format)
{
    return toNumber(quantity.getValue(), format);
}

std::string UnitsApi::toNumber(const double value, const QuantityFormat& format)
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

std::string
UnitsApi::schemaTranslate(const Quantity& quant, double& factor, std::string& unitString)
{
    return schemas->currentSchema()->translate(quant, factor, unitString);
}

std::string UnitsApi::schemaTranslate(const Quantity& quant)
{  // to satisfy GCC
    double dummy1 {};
    std::string dummy2;
    return schemas->currentSchema()->translate(quant, dummy1, dummy2);
}

void UnitsApi::setDecimals(const std::size_t prec)
{
    decimals = prec;
}

size_t UnitsApi::getDecimals()
{
    return decimals;
}

size_t UnitsApi::getDefDecimals()
{
    return schemas->getDecimals();
}
