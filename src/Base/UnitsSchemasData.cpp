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

#include "UnitsSchemasData.h"
#include "UnitsApi.h"

/** utility function for toFractional */
std::size_t greatestCommonDenominator(const std::size_t a, const std::size_t b)
{
    return b == 0 ? a : greatestCommonDenominator(b, a % b);  // Euclid's algorithm
}

/**
 * double -> [feet'] [inches" [+ fraction]"], e.g.: 3' 4" + 3/8"
 */
std::string toFractional(const double value)
{
    constexpr auto inchPerFoot {12};
    constexpr auto mmPerInch {25.4};

    int fractDenominator = Base::UnitsApi::getDenominator();

    auto numFractUnits =
        static_cast<std::size_t>(std::round(std::abs(value) / mmPerInch * fractDenominator));
    if (numFractUnits == 0) {
        return "0";
    }

    const auto feet =
        static_cast<std::size_t>(std::floor(numFractUnits / (inchPerFoot * fractDenominator)));
    numFractUnits -= inchPerFoot * fractDenominator * feet;

    const auto inches = static_cast<std::size_t>(std::floor(numFractUnits / fractDenominator));
    const std::size_t fractNumerator = numFractUnits - (fractDenominator * inches);

    const std::size_t common_denom = greatestCommonDenominator(fractNumerator, fractDenominator);
    const std::size_t numerator = fractNumerator / common_denom;
    const std::size_t denominator = fractDenominator / common_denom;

    bool addSpace {false};
    std::string result;

    if (value < 0) {
        result += "-";
    }

    if (feet > 0) {
        result += fmt::format("{}'", feet);
        addSpace = true;
    }

    if (inches > 0) {
        result += fmt::format("{}{}\"", addSpace ? " " : "", inches);
        addSpace = false;
    }

    if (numerator > 0) {
        if (inches > 0) {
            result += fmt::format(" {} ", value < 0 ? "-" : "+");
            addSpace = false;
        }
        result += fmt::format("{}{}/{}\"", addSpace ? " " : "", numerator, denominator);
    }

    return result;
}

/**
 * double -> degrees°[minutes′[seconds″]]
 */
std::string toDms(const double value)
{
    constexpr auto dmsRatio {60.0};

    auto calc = [&](const double total) -> std::pair<int, double> {
        const double whole = std::floor(total);
        return {static_cast<int>(whole), dmsRatio * (total - whole)};
    };

    auto [degrees, totalMinutes] = calc(value);
    std::string out = fmt::format("{}°", degrees);

    if (totalMinutes > 0) {
        auto [minutes, totalSeconds] = calc(totalMinutes);
        out += fmt::format("{}′", minutes);

        if (totalSeconds > 0) {
            out += fmt::format("{}″", std::round(totalSeconds));
        }
    }

    return out;
}

/**
 * Special functions caller
 */

// clang-format off
const std::map<std::string, std::function<std::string(double, double&, std::string&)>> specials
{
    {
        { "toDMS"        , [](const double val, double& factor, std::string& unitString) {
            factor = 1.0;
            unitString = "deg";
            return toDms(val);
        }},
        { "toFractional" , [](const double val, double& factor, std::string& unitString) {
            factor = 25.4;
            unitString = "in";
            return toFractional(val);
        }}
    }
};  // clang-format on

std::string Base::UnitsSchemasData::runSpecial(const std::string& name,
                                               const double value,
                                               double& factor,
                                               std::string& unitString)
{
    return specials.contains(name) ? specials.at(name)(value, factor, unitString) : "";
}