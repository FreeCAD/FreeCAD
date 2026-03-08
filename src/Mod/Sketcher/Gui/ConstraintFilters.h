// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#pragma once

#include <array>
#include <bitset>

namespace SketcherGui
{

namespace ConstraintFilter
{

// FilterValue and SpecialFilterValue are the filters used for Constraint Filtering in the
// Constraint's Widget. FilterValue includes the filters used for the Multi-Filter dialog
// (SpecialFilterValue is not part of the multi-filter).
//
// The values are hardcoded to be the same as the indices in the combobox and viewlist. Addition of
// an element here requires the addition of the corresponding entry there and vice versa.

enum class FilterValue
{
    All = 0,
    Geometric = 1,
    Coincident = 2,
    PointOnObject = 3,
    Vertical = 4,
    Horizontal = 5,
    Parallel = 6,
    Perpendicular = 7,
    Tangent = 8,
    Equality = 9,
    Symmetric = 10,
    Block = 11,
    InternalAlignment = 12,
    Datums = 13,
    HorizontalDistance = 14,
    VerticalDistance = 15,
    Distance = 16,
    Radius = 17,
    Weight = 18,
    Diameter = 19,
    Angle = 20,
    SnellsLaw = 21,
    Named = 22,
    NonDriving = 23,
    NumFilterValue  // SpecialFilterValue shall start at the same index as this
};

constexpr auto FilterValueLength = static_cast<std::underlying_type_t<FilterValue>>(
    FilterValue::NumFilterValue
);

enum class SpecialFilterValue
{
    Selection = FilterValueLength,  // = 24
    AssociatedConstraints,          // = 25
    NumSpecialFilterValue
};

constexpr auto SpecialFilterValue = static_cast<std::underlying_type_t<FilterValue>>(
    SpecialFilterValue::NumSpecialFilterValue
);

/// A std::bitset sized to provide one bit per FilterValue value
using FilterValueBitset = std::bitset<FilterValueLength>;

/// Helper function to retrieve the underlying integral type of a filter value
template<typename T>
inline auto getFilterIntegral(T filterValue)
{
    return static_cast<std::underlying_type_t<T>>(filterValue);
}

/// Helper function to test whether a FilterValue value is set in a FilterValueBitset
inline bool checkFilterBitset(FilterValueBitset set, FilterValue filter)
{
    auto underlyingFilterValue = static_cast<std::underlying_type_t<FilterValue>>(filter);

    return set[underlyingFilterValue];
}

/// Helper function expanding a parameter pack value of enum classes to create a integral underlying
/// type having the bits corresponding to the parameter pack set
template<typename... Args>
constexpr decltype(auto) buildBitset(Args... args)
{
    return (... | (1 << static_cast<std::underlying_type_t<Args>>(args)));
}

/// Array of FilterValue bit sets of size of the number of FilterValues indicating for each
/// FilterValue, which other FilterValues are comprised therein. It defines the dependencies between
/// filters.
constexpr std::array<FilterValueBitset, FilterValueLength> filterAggregates {
    buildBitset(
        FilterValue::All,
        FilterValue::Geometric,
        FilterValue::Horizontal,
        FilterValue::Vertical,
        FilterValue::Coincident,
        FilterValue::PointOnObject,
        FilterValue::Parallel,
        FilterValue::Perpendicular,
        FilterValue::Tangent,
        FilterValue::Equality,
        FilterValue::Symmetric,
        FilterValue::Block,
        FilterValue::Datums,
        FilterValue::Distance,
        FilterValue::HorizontalDistance,
        FilterValue::VerticalDistance,
        FilterValue::Radius,
        FilterValue::Weight,
        FilterValue::Diameter,
        FilterValue::Angle,
        FilterValue::SnellsLaw,
        FilterValue::Named,
        FilterValue::NonDriving,
        FilterValue::InternalAlignment
    ),  // All = All other groups are covered (0)
    buildBitset(
        FilterValue::Geometric,
        FilterValue::Horizontal,
        FilterValue::Vertical,
        FilterValue::Coincident,
        FilterValue::PointOnObject,
        FilterValue::Parallel,
        FilterValue::Perpendicular,
        FilterValue::Tangent,
        FilterValue::Equality,
        FilterValue::Symmetric,
        FilterValue::Block,
        FilterValue::InternalAlignment
    ),  // Geometric = All others not being datums (1)

    buildBitset(FilterValue::Coincident),         // Coincident = Just this (2)
    buildBitset(FilterValue::PointOnObject),      // PointOnObject = Just this (3)
    buildBitset(FilterValue::Vertical),           // Vertical = Just this (4)
    buildBitset(FilterValue::Horizontal),         // Horizontal = Just this (5)
    buildBitset(FilterValue::Parallel),           // Parallel = Just this (6)
    buildBitset(FilterValue::Perpendicular),      // Perpendicular = Just this (7)
    buildBitset(FilterValue::Tangent),            // Tangent = Just this (8)
    buildBitset(FilterValue::Equality),           // Equality = Just this (9)
    buildBitset(FilterValue::Symmetric),          // Symmetric = Just this (10)
    buildBitset(FilterValue::Block),              // Block = Just this (11)
    buildBitset(FilterValue::InternalAlignment),  // InternalAlignment = Just this (12)

    buildBitset(
        FilterValue::Datums,
        FilterValue::Distance,
        FilterValue::HorizontalDistance,
        FilterValue::VerticalDistance,
        FilterValue::Radius,
        FilterValue::Weight,
        FilterValue::Diameter,
        FilterValue::Angle,
        FilterValue::SnellsLaw
    ),  // Datum = all others not being geometric (13)

    buildBitset(FilterValue::HorizontalDistance),  // HorizontalDistance = Just this (14)
    buildBitset(FilterValue::VerticalDistance),    // VerticalDistance = Just this (15)
    buildBitset(FilterValue::Distance),            // Distance = Just this (16)
    buildBitset(FilterValue::Radius),              // Radius = Just this (17)
    buildBitset(FilterValue::Weight),              // Weight = Just this (18)
    buildBitset(FilterValue::Diameter),            // Diameter = Just this (19)
    buildBitset(FilterValue::Angle),               // Angle = Just this (20)
    buildBitset(FilterValue::SnellsLaw),           // SnellsLaw = Just this (21)

    buildBitset(FilterValue::Named),      // Named = Just this (22)
    buildBitset(FilterValue::NonDriving)  // NonDriving = Just this (23)
};

}  // namespace ConstraintFilter

}  // namespace SketcherGui
