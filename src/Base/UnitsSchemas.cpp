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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <QCoreApplication>

#include "UnitsSchemas.h"
#include "Exception.h"
#include "Quantity.h"
#include "UnitsApi.h"
#include "UnitsSchema.h"
#include "UnitsSchemasSpecs.h"
#include "UnitsSchemasData.h"

using Base::Quantity;
using Base::UnitsSchema;
using Base::UnitsSchemas;
using Base::UnitsSchemaSpec;

UnitsSchemas::UnitsSchemas(const UnitsSchemasDataPack& pack)
    : pack {pack}
    , denominator {pack.defDenominator}
    , decimals {pack.defDecimals}
{}

size_t UnitsSchemas::count() const
{
    return pack.specs.size();
}

std::vector<std::string> UnitsSchemas::getVec(const std::function<std::string(UnitsSchemaSpec)>& fn)
{
    std::vector<std::string> vec;
    auto specs = pack.specs;
    std::sort(specs.begin(), specs.end(), [](const UnitsSchemaSpec& a, const UnitsSchemaSpec& b) {
        return a.num < b.num;
    });
    std::transform(specs.begin(), specs.end(), std::back_inserter(vec), fn);

    return vec;
}

std::vector<std::string> UnitsSchemas::names()
{
    return getVec([](const UnitsSchemaSpec& spec) {
        return spec.name;
    });
}

std::vector<std::string> UnitsSchemas::descriptions()
{
    return getVec([](const UnitsSchemaSpec& spec) {
        return QCoreApplication::translate("UnitsApi", spec.description).toStdString();
    });
}

std::size_t UnitsSchemas::getDecimals() const
{
    return pack.defDecimals;
}

std::size_t UnitsSchemas::defFractDenominator() const
{
    return pack.defDenominator;
}

void UnitsSchemas::setdefFractDenominator(const std::size_t size)
{
    denominator = size;
}

void UnitsSchemas::select()
{
    makeCurr(spec());
}

void UnitsSchemas::select(const std::string_view& name)
{
    makeCurr(spec(name));
}

void UnitsSchemas::select(const std::size_t num)
{
    makeCurr(spec(num));
}

UnitsSchema* UnitsSchemas::currentSchema() const
{
    return current.get();
}

void UnitsSchemas::makeCurr(const UnitsSchemaSpec& spec)
{
    current = std::make_unique<UnitsSchema>(spec);
}

UnitsSchemaSpec UnitsSchemas::findSpec(const std::function<bool(UnitsSchemaSpec)>& fn)
{
    const auto found = std::find_if(pack.specs.begin(), pack.specs.end(), fn);

    if (found == pack.specs.end()) {
        throw RuntimeError {"UnitSchemaSpec not found"};
    }

    return *found;
}

UnitsSchemaSpec UnitsSchemas::spec()
{
    return findSpec([](const UnitsSchemaSpec& spec) {
        return spec.isDefault;
    });
}

UnitsSchemaSpec UnitsSchemas::spec(const std::string_view& name)
{
    return findSpec([&name](const UnitsSchemaSpec& spec) {
        return spec.name == name;
    });
}

UnitsSchemaSpec UnitsSchemas::spec(const std::size_t num)
{
    return findSpec([&num](const UnitsSchemaSpec& spec) {
        return spec.num == num;
    });
}
