// SPDX-License-Identifier: LGPL-2.1-or-later

/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#include <iomanip>
#include <sstream>
#include <string>

#include <QLocale>
#include <QString>

#include "Quantity.h"
#include "UnitsSchema.h"
#include "UnitsSchemasData.h"
#include "UnitsSchemasSpecs.h"
#include "Exception.h"
#include "Quantity.h"

#include "Console.h"

using Base::UnitsSchema;
using Base::UnitsSchemaSpec;


UnitsSchema::UnitsSchema(UnitsSchemaSpec spec)
    : spec {std::move(spec)}
{}

std::string UnitsSchema::translate(const Quantity& quant) const
{  // to satisfy GCC
    double dummy1 {};
    std::string dummy2;
    return translate(quant, dummy1, dummy2);
}

std::string
UnitsSchema::translate(const Quantity& quant, double& factor, std::string& unitString) const
{
    // Use defaults without schema-level translation.
    factor = 1.0;
    unitString = quant.getUnit().getString();

    if (spec.translationSpecs.empty()) {
        return toLocale(quant, factor, unitString);
    }

    const auto unitName = quant.getUnit().getTypeString();
    if (!spec.translationSpecs.contains(unitName)) {
        return toLocale(quant, factor, unitString);
    }

    const auto value = quant.getValue();
    auto isSuitable = [&](const UnitTranslationSpec& row) {
        return row.threshold > value || row.threshold == 0;  // zero indicates default
    };

    auto unitSpecs = spec.translationSpecs.at(unitName);
    const auto unitSpec = std::find_if(unitSpecs.begin(), unitSpecs.end(), isSuitable);
    if (unitSpec == unitSpecs.end()) {
        throw RuntimeError("Suitable threshold not found. Schema: " + spec.name
                           + " value: " + std::to_string(value));
    }

    if (unitSpec->factor == 0) {
        const QuantityFormat& format = quant.getFormat();
        return UnitsSchemasData::runSpecial(unitSpec->unitString,
                                            value,
                                            format.getPrecision(),
                                            format.getDenominator(),
                                            factor,
                                            unitString);
    }

    factor = unitSpec->factor;
    unitString = unitSpec->unitString;

    return toLocale(quant, factor, unitString);
}

std::string
UnitsSchema::toLocale(const Quantity& quant, const double factor, const std::string& unitString)
{
    QLocale Lc;
    const QuantityFormat& format = quant.getFormat();
    if (format.option != QuantityFormat::None) {
        Lc.setNumberOptions(static_cast<QLocale::NumberOptions>(format.option));
    }

    auto valueString =
        Lc.toString(quant.getValue() / factor, format.toFormat(), format.getPrecision())
            .toStdString();

    auto notUnit = [](auto s) {
        return s.empty() || s == "°" || s == "″" || s == "′" || s == "\"" || s == "'";
    };

    return fmt::format("{}{}{}", valueString, notUnit(unitString) ? "" : " ", unitString);
}

bool UnitsSchema::isMultiUnitLength() const
{
    return spec.isMultUnitLen;
}

bool UnitsSchema::isMultiUnitAngle() const
{
    return spec.isMultUnitAngle;
}

std::string UnitsSchema::getBasicLengthUnit() const
{
    return spec.basicLengthUnitStr;
}

std::string UnitsSchema::getName() const
{
    return spec.name;
}

std::string UnitsSchema::getDescription() const
{
    return spec.description;
}

int UnitsSchema::getNum() const
{
    return static_cast<int>(spec.num);
}


//! return the unit text for this quantity in this schema. ex 10 mm => "mm"
//! a more general approach than getBasicLengthUnit.
//! TODO: some common code here with translate()
std::string UnitsSchema::getUnitText(const Base::Quantity& quant) const
{
    std::string typeString = quant.getUnit().getTypeString();  // "Area", "Mass", ...
    const auto value = quant.getValue();

    // TODO: some common code here with translate()
    if (!spec.translationSpecs.contains(typeString)) {
        Base::Console().log("Schema %s has no entry for %s\n",
                            getName().c_str(),
                            typeString.c_str());
        return {};
    }
    auto unitSpecs = spec.translationSpecs.at(typeString);

    auto isSuitable = [&](const UnitTranslationSpec& row) {
        return row.threshold > value || row.threshold == 0;
    };

    const auto unitSpec = std::ranges::find_if(unitSpecs, isSuitable);
    if (unitSpec == unitSpecs.end()) {
        throw RuntimeError("Suitable threshold not found (2). Schema: " + spec.name
                           + " value: " + std::to_string(value));
    }

    return unitSpec->unitString;
}
