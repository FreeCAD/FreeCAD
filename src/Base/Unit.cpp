/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#endif

#include <vector>
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "Unit.h"
#include "Units.h"
#include "Exception.h"

using namespace Base;


bool Unit::operator==(const Unit& that) const
{
    return vals == that.vals;
}

bool Unit::operator!=(const Unit& that) const
{
    return vals != that.vals;
}

Unit& Unit::operator*=(const Unit& that)
{
    *this = *this * that;
    return *this;
}

Unit& Unit::operator/=(const Unit& that)
{
    *this = *this / that;
    return *this;
}

std::array<int8_t, unitNumVals> Unit::getVals() const
{
    return vals;
}

std::string Unit::getString() const
{
    auto buildSubStr = [&](auto index) {
        const std::string unitStrString {valNames.at(index)};
        const auto absol {abs(vals.at(index))};

        return absol <= 1 ? unitStrString
                          : fmt::format("{}^{}", unitStrString, std::to_string(absol));
    };

    auto buildStr = [&](auto indexes) {
        std::vector<std::string> subStrings {};
        std::transform(indexes.begin(), indexes.end(), std::back_inserter(subStrings), buildSubStr);

        return format("{}", fmt::join(subStrings, "*"));
    };

    //------------------------------------------------------------------------------

    auto [posValIndexes, negValIndexes] = nonZeroValsIndexes();
    auto numeratorStr = buildStr(posValIndexes);
    if (negValIndexes.empty()) {
        return numeratorStr;
    }

    auto denominatorStr = buildStr(negValIndexes);

    return fmt::format("{}/{}",
                       numeratorStr.empty() ? "1" : numeratorStr,
                       negValIndexes.size() > 1 ? fmt::format("({})", denominatorStr)
                                                : denominatorStr);
}

Unit Unit::root(const uint8_t num) const
{
    auto check = [&](auto val) {
        return val % num == 0;
    };

    auto apply = [&](auto val) {
        return static_cast<int8_t>(val / num);
    };

    //---------------------------------------------------------

    if (num < 1) {
        throw UnitsMismatchError("root must be > 0");
    }

    if (!std::all_of(vals.begin(), vals.end(), check)) {
        throw UnitsMismatchError("unit values must be divisible by root");
    }

    std::array<int8_t, unitNumVals> res {};
    std::transform(vals.begin(), vals.end(), res.begin(), apply);

    return Unit {res};
}

int Unit::getVal(const UnitPart part) const
{
    return vals.at(static_cast<const int>(part));
}

std::string Unit::representation() const
{
    auto str = format("{}", fmt::join(vals, ","));
    auto inParen = fmt::format("Unit: {} ({})", getString(), str);
    const auto nameStr = std::string {name};
    return nameStr.empty() ? inParen : fmt::format("{} [{}]", inParen, nameStr);
}

Unit Unit::pow(const double exp) const
{
    auto isInt = [&exp](const auto val) {
        const auto num {val * exp};
        return std::fabs(std::round(num) - num) < std::numeric_limits<double>::epsilon();
    };

    auto checkPow = [&] {
        if (!std::all_of(vals.begin(), vals.end(), isInt)) {
            throw UnitsMismatchError("pow() of unit not possible");
        }
    };

    auto apply = [&](const auto val) {
        return static_cast<int>(val * exp);
    };

    //--------------------------------------------------

    checkPow();
    std::array<int8_t, unitNumVals> res {};
    std::transform(vals.begin(), vals.end(), res.begin(), apply);

    return Unit {res};
}

Unit Unit::operator*(const Unit& right) const
{
    auto add = [&](auto leftVal, auto rightVal) {
        return leftVal + rightVal;
    };

    std::array<int8_t, unitNumVals> res {};
    std::transform(vals.begin(), vals.end(), right.vals.begin(), res.begin(), add);

    return Unit {res};
}

Unit Unit::operator/(const Unit& right) const
{
    auto div = [&](auto leftVal, auto rightVal) mutable {
        return leftVal - rightVal;
    };

    std::array<int8_t, unitNumVals> res {};
    std::transform(vals.begin(), vals.end(), right.vals.begin(), res.begin(), div);

    return Unit {res};
}

std::string Unit::getTypeString() const
{
    return name.empty() ? Units::getString(vals) : std::string {name.data(), name.size()};
}

std::pair<std::vector<size_t>, std::vector<size_t>> Unit::nonZeroValsIndexes() const
{
    std::vector<size_t> pos {};
    std::vector<size_t> neg {};

    auto posNeg = [&](auto index) {
        if (vals.at(index) > 0) {
            pos.push_back(index);
        }
        else if (vals.at(index) < 0) {
            neg.push_back(index);
        }
    };

    static constexpr std::array<size_t, unitNumVals> indexes {0, 1, 2, 3, 4, 5, 6, 7};
    std::for_each(indexes.begin(), indexes.end(), posNeg);

    return {pos, neg};
}
