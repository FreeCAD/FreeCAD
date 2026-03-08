// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Persistence.h>
#include <Base/Quantity.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "GeoEnum.h"


// Flipping this to 0 removes old legazy members First, FirstPos, Second...
// Will be used when everything has been migrated to new api.
#define SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS 1


namespace Sketcher
{
/*!
 Important note: New constraint types must be always added at the end but before
 'NumConstraintTypes'. This is mandatory in order to keep the handling of constraint types upward
 compatible which means that this program version ignores later introduced constraint types when
 reading them from a project file. They also must be added to the 'type2str' array in this file.
 */
enum ConstraintType : int
{
    None = 0,
    Coincident = 1,
    Horizontal = 2,
    Vertical = 3,
    Parallel = 4,
    Tangent = 5,
    Distance = 6,
    DistanceX = 7,
    DistanceY = 8,
    Angle = 9,
    Perpendicular = 10,
    Radius = 11,
    Equal = 12,
    PointOnObject = 13,
    Symmetric = 14,
    InternalAlignment = 15,
    SnellsLaw = 16,
    Block = 17,
    Diameter = 18,
    Weight = 19,
    NumConstraintTypes  // must be the last item!
};

enum InternalAlignmentType
{
    Undef = 0,
    EllipseMajorDiameter = 1,
    EllipseMinorDiameter = 2,
    EllipseFocus1 = 3,
    EllipseFocus2 = 4,
    HyperbolaMajor = 5,
    HyperbolaMinor = 6,
    HyperbolaFocus = 7,
    ParabolaFocus = 8,
    BSplineControlPoint = 9,
    BSplineKnotPoint = 10,
    ParabolaFocalAxis = 11,
    NumInternalAlignmentType  // must be the last item!
};

class SketcherExport Constraint: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Constraint();
    // PVS V690: It is perfectly fine to use copy operator only internally

    // Constraints objects explicitly not copiable with standard methods
    // Copy constructor is private for internal use only
    Constraint& operator=(const Constraint& a) = delete;

    // Constraints objects explicitly not movable
    Constraint(Constraint&&) = delete;
    Constraint& operator=(Constraint&&) = delete;

    ~Constraint() override = default;

    // does copy the tag, it will be treated as a rename by the expression engine.
    Constraint* clone() const;
    // does not copy the tag, but generates a new one
    Constraint* copy() const;

    // from base class
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;

    PyObject* getPyObject() override;

    Base::Quantity getPresentationValue() const;
    inline void setValue(double newValue)
    {
        Value = newValue;
    }
    inline double getValue() const
    {
        return Value;
    }

    inline bool isDimensional() const
    {
        return Type == Distance || Type == DistanceX || Type == DistanceY || Type == Radius
            || Type == Diameter || Type == Angle || Type == SnellsLaw || Type == Weight;
    }

    /// utility function to swap the index in elements of the provided constraint from the
    /// fromGeoId GeoId to toGeoId
    void substituteIndex(int fromGeoId, int toGeoId);
    /// utility function to swap the index and position in elements of the provided
    /// constraint from {fromGeoId, fromPosId} to {toGeoId, toPosId}.
    void substituteIndexAndPos(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId);

    /// utility function to check if `geoId` is one of the geometries
    bool involvesGeoId(int geoId) const;

    /// utility function to check if (`geoId`, `posId`) is one of the points/curves
    bool involvesGeoIdAndPosId(int geoId, PointPos posId) const;

    std::string typeToString() const
    {
        return typeToString(Type);
    }
    static std::string typeToString(ConstraintType type);

    std::string internalAlignmentTypeToString() const
    {
        return internalAlignmentTypeToString(AlignmentType);
    }
    static std::string internalAlignmentTypeToString(InternalAlignmentType alignment);

    friend class PropertyConstraintList;

private:
    Constraint(const Constraint&) = default;  // only for internal use

private:
    double Value {0.0};

    // clang-format off
    constexpr static std::array<const char*, ConstraintType::NumConstraintTypes> type2str {
        {"None",
         "Coincident",
         "Horizontal",
         "Vertical",
         "Parallel",
         "Tangent",
         "Distance",
         "DistanceX",
         "DistanceY",
         "Angle",
         "Perpendicular",
         "Radius",
         "Equal",
         "PointOnObject",
         "Symmetric",
         "InternalAlignment",
         "SnellsLaw",
         "Block",
         "Diameter",
         "Weight"}};
    // clang-format on

    constexpr static std::array<const char*, InternalAlignmentType::NumInternalAlignmentType>
        internalAlignmentType2str {
            {"Undef",
             "EllipseMajorDiameter",
             "EllipseMinorDiameter",
             "EllipseFocus1",
             "EllipseFocus2",
             "HyperbolaMajor",
             "HyperbolaMinor",
             "HyperbolaFocus",
             "ParabolaFocus",
             "BSplineControlPoint",
             "BSplineKnotPoint",
             "ParabolaFocalAxis"}
        };

public:
    ConstraintType Type {None};
    InternalAlignmentType AlignmentType {Undef};
    std::string Name;
    float LabelDistance {10.F};
    float LabelPosition {0.F};
    bool isDriving {true};
    // Note: for InternalAlignment Type this index indexes equal internal geometry elements (e.g.
    // index of pole in a bspline). It is not a GeoId!!
    int InternalAlignmentIndex {-1};
    bool isInVirtualSpace {false};
    bool isVisible {true};

    bool isActive {true};

    GeoElementId getElement(size_t index) const;
    void setElement(size_t index, GeoElementId element);
    size_t getElementsSize() const;
    void addElement(GeoElementId element);

#ifdef SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    // Deprecated, use getElement/setElement instead
    int First {GeoEnum::GeoUndef};
    int Second {GeoEnum::GeoUndef};
    int Third {GeoEnum::GeoUndef};
    PointPos FirstPos {PointPos::none};
    PointPos SecondPos {PointPos::none};
    PointPos ThirdPos {PointPos::none};
#endif

private:
    // New way to access point ids and positions.
    // While the old way is still supported, it is recommended to the getters and setters instead.
    std::vector<GeoElementId> elements {GeoElementId(), GeoElementId(), GeoElementId()};

protected:
    boost::uuids::uuid tag;
};

}  // namespace Sketcher
