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

#include <functional>

#include <FCConfig.h>

#include <Mod/Sketcher/SketcherGlobal.h>

namespace Sketcher
{

/** @brief Sketcher Geometry is identified with an index called GeoId.
 *
 * @details
 * GeoId >= 0 are normal geometry elements
 * GeoId = -1 and -2 are the Horizontal and Vertical axes and the root point
 * GeoId <= -2 are external geometry elements
 * GeoId = -2000 is an undefined or unused geometry id
 *
 * GeoEnum struct provides convenience labels for these GeoIds.
 *
 * However, GeoEnum is not enough to define an element of a Geometry. The most
 * straightforward example is the RootPoint and the Horizontal Axis. Both have
 * the same GeoId (= -1).
 *
 * The same happens for elements of a given geometry. For example, a line has
 * a starting point, an endpoint and an edge. All these share the same GeoId, as the
 * GeoId identifies a geometry and not an element of a geometry.
 *
 * The elements of a given geometry are identified by the PointPos enum. In the case
 * of the root point, it is considered to be the start point of the Horizontal Axis, this
 * is a convention.
 *
 * Therefore, a geometry element (GeoElementId) is univocally defined by the combination of a
 * GeoId and a PointPos.
 *
 * Geometry shapes having more or different elements than those supported by the PointPos
 * struct, such as conics, in particular an arc of ellipse, are called complex geometries. The extra
 * elements of complex geometries are actual separate geometries (focus of an ellipse, line defining
 * the major axis of an ellipse, circle representing the weight of a BSpline), and they are call
 * InternalAlignment geometries.
 *
 * For Geometry lists, refer to GeoListModel template.
 */
enum GeoEnum
{
    RtPnt = -1,   // GeoId of the Root Point
    HAxis = -1,   // GeoId of the Horizontal Axis
    VAxis = -2,   // GeoId of the Vertical Axis
    RefExt = -3,  // Starting GeoID of external geometry ( negative geoIds starting at this index)
    GeoUndef = -2000  // GeoId of an undefined Geometry (uninitialised or unused GeoId)
};

/*!
 * @brief PointPos lets us refer to different aspects of a piece of geometry.
 * @details sketcher::none refers to an edge itself (eg., for a Perpendicular constraint
 * on two lines). sketcher::start and sketcher::end denote the endpoints of lines or bounded curves.
 * Sketcher::mid denotes geometries with geometrical centers (eg., circle, ellipse). Bare points use
 * 'start'. More complex geometries like parabola focus or b-spline knots use InternalAlignment
 * constraints in addition to PointPos.
 */
enum class PointPos : int
{
    none = 0,   // Edge of a geometry
    start = 1,  // Starting point of a geometry
    end = 2,    // End point of a geometry
    mid = 3     // Mid point of a geometry
};

/** @brief      Struct for storing a {GeoId, PointPos} pair.
 *
 * @details
 * {GeoId, PointPos} is pervasive in the sketcher as means to identify geometry (edges) and geometry
 * elements (vertices).
 *
 * GeoElementId intends to substitute this pair whenever appropriate. For example in containers and
 * ordered containers.
 *
 * It has overloaded equality operator and specialised std::less so that it can safely be used in
 * containers, including ordered containers.
 *
 */
class SketcherExport GeoElementId
{
public:
    /** @brief default constructor initialises object to an undefined (invalid) element.
     */
    explicit constexpr GeoElementId(int geoId = GeoEnum::GeoUndef, PointPos pos = PointPos::none);

    /** @brief equality operator
     */
    bool operator==(const GeoElementId& obj) const;

    /** @brief inequality operator
     */
    bool operator!=(const GeoElementId& obj) const;

    /** @brief Underlying GeoId (see GeoEnum for definition)
     */
    int GeoId;
    /** @brief Indication of vertex or curve (see PointPos)
     */
    PointPos Pos;

    bool isCurve() const;

    int posIdAsInt() const;

    /** @brief GeoElementId of the Root Point
     */
    static const GeoElementId RtPnt;
    /** @brief GeoElementId of the Horizontal Axis
     */
    static const GeoElementId HAxis;
    /** @brief GeoElementId of the Vertical Axis
     */
    static const GeoElementId VAxis;
};

// inline constexpr constructor
inline constexpr GeoElementId::GeoElementId(int geoId, PointPos pos)
    : GeoId(geoId)
    , Pos(pos)
{}

inline bool GeoElementId::isCurve() const
{
    return Pos == PointPos::none;
}

inline int GeoElementId::posIdAsInt() const
{
    return static_cast<int>(Pos);
}

#ifndef FC_OS_WIN32
constexpr const GeoElementId GeoElementId::RtPnt = GeoElementId(GeoEnum::RtPnt, PointPos::start);
constexpr const GeoElementId GeoElementId::HAxis = GeoElementId(GeoEnum::HAxis, PointPos::none);
constexpr const GeoElementId GeoElementId::VAxis = GeoElementId(GeoEnum::VAxis, PointPos::none);
#endif

}  // namespace Sketcher

namespace std
{
template<>
struct less<Sketcher::GeoElementId>
{
    bool operator()(const Sketcher::GeoElementId& lhs, const Sketcher::GeoElementId& rhs) const
    {
        return (lhs.GeoId != rhs.GeoId) ? (lhs.GeoId < rhs.GeoId)
                                        : (static_cast<int>(lhs.Pos) < static_cast<int>(rhs.Pos));
    }
};
}  // namespace std
