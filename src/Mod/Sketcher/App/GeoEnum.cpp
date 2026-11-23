// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include "GeoEnum.h"

#include <FCConfig.h>
#include <string>

using namespace Sketcher;

bool GeoElementId::operator==(const GeoElementId& obj) const
{
    return this->GeoId == obj.GeoId && this->Pos == obj.Pos;
}

bool GeoElementId::operator!=(const GeoElementId& obj) const
{
    return this->GeoId != obj.GeoId || this->Pos != obj.Pos;
}

std::string GeoElementId::pointPosToString(PointPos pos)
{
    const auto index = static_cast<size_t>(pos);

    if (index < pointPos2str.size()) {
        return pointPos2str[index];
    }

    return "unknown";
}

std::string GeoElementId::toString() const
{
    return "GeoId=" + std::to_string(GeoId) + ", Pos=" + pointPosToString(Pos);
}

#ifdef FC_OS_WIN32
constexpr const GeoElementId GeoElementId::RtPnt = GeoElementId(GeoEnum::RtPnt, PointPos::start);
constexpr const GeoElementId GeoElementId::HAxis = GeoElementId(GeoEnum::HAxis, PointPos::none);
constexpr const GeoElementId GeoElementId::VAxis = GeoElementId(GeoEnum::VAxis, PointPos::none);
#endif
