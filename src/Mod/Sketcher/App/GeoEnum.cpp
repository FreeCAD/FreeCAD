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

#include "PreCompiled.h"

#include "GeoEnum.h"

using namespace Sketcher;

bool GeoElementId::operator==(const GeoElementId& obj) const
{
    return this->GeoId == obj.GeoId && this->Pos == obj.Pos;
}

bool GeoElementId::operator!=(const GeoElementId& obj) const
{
    return this->GeoId != obj.GeoId || this->Pos != obj.Pos;
}

#ifdef FC_OS_WIN32
constexpr const GeoElementId GeoElementId::RtPnt = GeoElementId(GeoEnum::RtPnt, PointPos::start);
constexpr const GeoElementId GeoElementId::HAxis = GeoElementId(GeoEnum::HAxis, PointPos::none);
constexpr const GeoElementId GeoElementId::VAxis = GeoElementId(GeoEnum::VAxis, PointPos::none);
#endif
