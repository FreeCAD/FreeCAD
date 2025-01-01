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

#ifndef BASE_Unit_H
#define BASE_Unit_H

#include <cstdint>
#include <FCGlobal.h>
#include <array>
#include <vector>
#include <string>
#include <string_view>

namespace Base
{


constexpr auto unitNumVals {8};
using UnitVals = std::array<int8_t, unitNumVals>;
using NameVal = std::pair<int8_t, std::string_view>;

constexpr std::array<std::string_view, unitNumVals>
    valNames {"mm", "kg", "s", "A", "K", "mol", "cd", "deg"};
constexpr auto unitValueLimit {8};

class BaseExport Unit final
{
public:
    Unit() = default;

    explicit constexpr Unit(const std::array<int8_t, unitNumVals> vals,
                            const std::string_view name = "")
        : vals {vals}
        , name {name}
    {
        checkRange();
    }

    Unit operator*(const Unit&) const;
    Unit operator/(const Unit&) const;
    bool operator==(const Unit&) const;
    bool operator!=(const Unit& that) const;
    Unit& operator*=(const Unit& that);
    Unit& operator/=(const Unit& that);

    [[nodiscard]] Unit pow(double exp) const;
    [[nodiscard]] Unit root(uint8_t num) const;

    [[nodiscard]] std::array<int8_t, unitNumVals> getVals() const;
    [[nodiscard]] int getLength() const;

    [[nodiscard]] std::string getString() const;       // E.g. kg, mm^2, mm*kg/s^2
    [[nodiscard]] std::string getTypeString() const;   // E.g. "Area", "Length", "Pressure"
    [[nodiscard]] std::string representation() const;  // E.g. "Unit: mm (1,0,0,0,0,0,0,0) [Length]"

private:
    std::array<int8_t, unitNumVals> vals {};
    std::string_view name;

    /** Error resets vals (FreeCAD exceptions not constexpr) */
    constexpr void checkRange()
    {
        for (const auto val : vals) {
            if (val >= unitValueLimit || val < -unitValueLimit) {
                vals = {0, 0, 0, 0, 0, 0, 0, 0};
                return;
            }
        }
    }

    /** Returns posIndexes, NegIndexes*/
    std::pair<std::vector<size_t>, std::vector<size_t>> nonZeroValsIndexes() const;
};

}  // namespace Base

#endif  // BASE_Unit_H
