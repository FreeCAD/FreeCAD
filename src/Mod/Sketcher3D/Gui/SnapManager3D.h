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

#include <string>

#include <Base/Vector3D.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>

namespace Sketcher3DGui
{

class ViewProviderSketch3D;

class SnapManager3D
{
public:
    explicit SnapManager3D(ViewProviderSketch3D& vp);
    ~SnapManager3D();

    SnapManager3D(const SnapManager3D&) = delete;
    SnapManager3D& operator=(const SnapManager3D&) = delete;

    Base::Vector3d snap(
        const Base::Vector3d& rawProjected,
        const std::string& pickedSubName,
        Sketcher3D::GeoElementId3D& target
    ) const;

private:
    bool snapToPickedObject(
        const std::string& pickedSubName,
        Base::Vector3d& snapPos,
        Sketcher3D::GeoElementId3D& target
    ) const;

    ViewProviderSketch3D& viewProvider;
};

}  // namespace Sketcher3DGui
