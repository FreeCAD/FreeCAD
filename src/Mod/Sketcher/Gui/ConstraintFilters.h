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

#ifndef SKETCHERGUI_ConstraintFilters_H
#define SKETCHERGUI_ConstraintFilters_H

#include <array>
#include <bitset>

namespace SketcherGui {

namespace ConstraintFilter {

    enum FilterValue {
        All = 0,
        Geometric = 1,
        Datums = 2,
        Named = 3,
        NonDriving = 4,
        Coincident = 5,
        PointOnObject = 6,
        Vertical = 7,
        Horizontal = 8,
        Parallel = 9,
        Perpendicular = 10,
        Tangent = 11,
        Equality = 12,
        Symmetric = 13,
        Block = 14,
        HorizontalDistance = 15,
        VerticalDistance = 16,
        Distance = 17,
        Radius = 18,
        Weight = 19,
        Diameter = 20,
        Angle = 21,
        SnellsLaw = 22,
        InternalAlignment = 23,
        NumFilterValue
    };

    enum SpecialFilterValue {
        Multiple = 24,
        Selection = 25,
        NumSpecialFilterValue
    };

    constexpr std::array< std::bitset<FilterValue::NumFilterValue>, FilterValue::NumFilterValue> filterAggregates {
        1 << FilterValue::All | 1 << FilterValue::Geometric | 1 << FilterValue::Datums | 1 << FilterValue::Named | 1 << FilterValue::NonDriving |
        1 << FilterValue::Horizontal | 1 << FilterValue::Vertical | 1 << FilterValue::Coincident | 1 << FilterValue::PointOnObject |
        1 << FilterValue::Parallel | 1 << FilterValue::Perpendicular | 1 << FilterValue::Tangent | 1 << FilterValue::Equality |
        1 << FilterValue::Symmetric | 1 << FilterValue::Block | 1 << FilterValue::Distance | 1 << FilterValue::HorizontalDistance |
        1 << FilterValue::VerticalDistance | 1 << FilterValue::Radius | 1 << FilterValue::Weight | 1 << FilterValue::Diameter |
        1 << FilterValue::Angle | 1 << FilterValue::SnellsLaw | 1 << FilterValue::InternalAlignment, // All = All other groups are covered (0)
        1 << FilterValue::Geometric | 1 << FilterValue::Horizontal | 1 << FilterValue::Vertical | 1 << FilterValue::Coincident |
        1 << FilterValue::PointOnObject | 1 << FilterValue::Parallel | 1 << FilterValue::Perpendicular | 1 << FilterValue::Tangent |
        1 << FilterValue::Equality | 1 << FilterValue::Symmetric | 1 << FilterValue::Block | 1 << FilterValue::InternalAlignment, // Geometric = All others not being datums (1)
        1 << FilterValue::Datums | 1 << FilterValue::Distance | 1 << FilterValue::HorizontalDistance | 1 << FilterValue::VerticalDistance | 1 << FilterValue::Radius | 1 << FilterValue::Weight | 1 << FilterValue::Diameter | 1 << FilterValue::Angle | 1 << FilterValue::SnellsLaw, // Datum = all others not being geometric (2)
        1 << FilterValue::Named, // Named = Just this (3)
        1 << FilterValue::NonDriving, // NonDriving = Just this (4)
        1 << FilterValue::Coincident, // Coincident = Just this (5)
        1 << FilterValue::PointOnObject, // PointOnObject = Just this (6)
        1 << FilterValue::Vertical, // Vertical = Just this (7)
        1 << FilterValue::Horizontal, // Horizontal = Just this (8)
        1 << FilterValue::Parallel, // Parallel = Just this (9)
        1 << FilterValue::Perpendicular, // Perpendicular = Just this (10)
        1 << FilterValue::Tangent, // Tangent = Just this (11)
        1 << FilterValue::Equality, // Equality = Just this (12)
        1 << FilterValue::Symmetric, // Symmetric = Just this (13)
        1 << FilterValue::Block, // Block = Just this (14)
        1 << FilterValue::HorizontalDistance, // HorizontalDistance = Just this (15)
        1 << FilterValue::VerticalDistance, // VerticalDistance = Just this (16)
        1 << FilterValue::Distance, // Distance = Just this (17)
        1 << FilterValue::Radius, // Radius = Just this (18)
        1 << FilterValue::Weight, // Weight = Just this (19)
        1 << FilterValue::Diameter, // Diameter = Just this (20)
        1 << FilterValue::Angle, // Angle = Just this (21)
        1 << FilterValue::SnellsLaw, // SnellsLaw = Just this (22)
        1 << FilterValue::InternalAlignment, // InternalAlignment = Just this (23)
    };

}

}

#endif // SKETCHERGUI_ConstraintFilters_H
