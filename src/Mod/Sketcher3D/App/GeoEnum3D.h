// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <functional>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

namespace Part
{
class Geometry;
}

namespace Sketcher3D
{

enum GeoEnum3D
{
    RtPnt = -1,        // GeoId of the Root Point
    GeoUndef = -2000,  // Undefined geometry
};

/// Position on a geometry element (start/end/mid/none).
enum class PointPos : int
{
    none = 0,
    start = 1,
    end = 2,
    mid = 3
};

/// geometry element a GeoElementId3D can refer to.
enum class GeoKind
{
    Unknown,
    Point,
    Line,
    Circle,
    Arc,
    Plane
};

struct Sketcher3DExport GeoElementId3D
{
    int GeoId = GeoEnum3D::GeoUndef;
    PointPos Pos = PointPos::none;
    GeoKind Kind = GeoKind::Unknown;

    constexpr GeoElementId3D() = default;
    constexpr GeoElementId3D(int geoId, PointPos pos, GeoKind kind = GeoKind::Unknown)
        : GeoId(geoId)
        , Pos(pos)
        , Kind(kind)
    {}

    static const GeoElementId3D RtPnt;

    bool isValid() const
    {
        return GeoId != GeoEnum3D::GeoUndef;
    }

    bool isRootPoint() const
    {
        return GeoId == RtPnt.GeoId && Pos == RtPnt.Pos;
    }

    int posIdAsInt() const
    {
        return static_cast<int>(Pos);
    }

    bool operator==(const GeoElementId3D& o) const
    {
        return GeoId == o.GeoId && Pos == o.Pos;
    }

    bool operator!=(const GeoElementId3D& o) const
    {
        return !(*this == o);
    }
};

inline const GeoElementId3D GeoElementId3D::RtPnt
    = GeoElementId3D(GeoEnum3D::RtPnt, PointPos::start, GeoKind::Point);

Sketcher3DExport GeoKind kindOfGeometry(const Part::Geometry* geo);

}  // namespace Sketcher3D

namespace std
{
template<>
struct less<Sketcher3D::GeoElementId3D>
{
    bool operator()(const Sketcher3D::GeoElementId3D& lhs, const Sketcher3D::GeoElementId3D& rhs) const
    {
        return (lhs.GeoId != rhs.GeoId) ? (lhs.GeoId < rhs.GeoId)
                                        : (static_cast<int>(lhs.Pos) < static_cast<int>(rhs.Pos));
    }
};
}  // namespace std
