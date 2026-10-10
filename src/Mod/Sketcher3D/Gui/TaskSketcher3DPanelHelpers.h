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

#include <map>
#include <vector>

#include <QIcon>
#include <QString>

#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>

namespace Sketcher3DGui
{
namespace PanelHelpers
{

inline QString posName(Sketcher3D::PointPos pos)
{
    switch (pos) {
        case Sketcher3D::PointPos::start:
            return QStringLiteral(".start");
        case Sketcher3D::PointPos::end:
            return QStringLiteral(".end");
        case Sketcher3D::PointPos::mid:
            return QStringLiteral(".mid");
        case Sketcher3D::PointPos::none:
        default:
            return QString();
    }
}

inline void buildGeometryLabels(
    const std::vector<Part::Geometry*>& geos,
    std::map<int, int>& pointLabels,
    std::map<int, int>& lineLabels
)
{
    int pointNext = 0;
    int lineNext = 0;
    for (std::size_t i = 0; i < geos.size(); ++i) {
        int geoId = static_cast<int>(i);
        Part::Geometry* geo = geos[i];
        if (!geo) {
            continue;
        }
        if (geo->is<Part::GeomPoint>()) {
            pointLabels.emplace(geoId, ++pointNext);
        }
        else if (geo->is<Part::GeomLineSegment>()) {
            lineLabels.emplace(geoId, ++lineNext);
        }
    }
}

inline QString displayNameForRef(
    const Sketcher3D::GeoElementId3D& ref,
    const std::map<int, int>& pointLabels,
    const std::map<int, int>& lineLabels
)
{
    if (auto it = pointLabels.find(ref.GeoId); it != pointLabels.end()) {
        return QStringLiteral("Point%1").arg(it->second);
    }
    if (auto it = lineLabels.find(ref.GeoId); it != lineLabels.end()) {
        return QStringLiteral("Line%1%2").arg(it->second).arg(posName(ref.Pos));
    }
    return QStringLiteral("G%1%2").arg(ref.GeoId).arg(posName(ref.Pos));
}

inline QIcon iconForConstraint(Sketcher3D::Constraint3D::Constraint3DType type)
{
    switch (type) {
        case Sketcher3D::Constraint3D::Coincident3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_PointOnPoint");
        case Sketcher3D::Constraint3D::Parallel3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Parallel");
        case Sketcher3D::Constraint3D::Angle3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle");
        case Sketcher3D::Constraint3D::AlongX:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Horizontal");
        case Sketcher3D::Constraint3D::AlongY:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Vertical");
        case Sketcher3D::Constraint3D::AlongZ:
            return Gui::BitmapFactory().iconFromTheme("Std_Axis");
        case Sketcher3D::Constraint3D::DistanceX3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance");
        case Sketcher3D::Constraint3D::DistanceY3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance");
        case Sketcher3D::Constraint3D::DistanceZ3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Length");
        case Sketcher3D::Constraint3D::PointOnCurve3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_PointOnObject");
        case Sketcher3D::Constraint3D::PointAtLineMidpoint3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Symmetric");
        case Sketcher3D::Constraint3D::Symmetric3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Symmetric");
        case Sketcher3D::Constraint3D::Collinear3D:
        case Sketcher3D::Constraint3D::Tangent3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Tangent");
        case Sketcher3D::Constraint3D::Radius3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Radius");
        case Sketcher3D::Constraint3D::Distance3D:
        default:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Length");
    }
}

}  // namespace PanelHelpers
}  // namespace Sketcher3DGui
