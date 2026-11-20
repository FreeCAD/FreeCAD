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

#include "PreCompiled.h"

#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "SnapManager3D.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;


SnapManager3D::SnapManager3D(ViewProviderSketch3D& vp)
    : viewProvider(vp)
{}

SnapManager3D::~SnapManager3D() = default;

void SnapManager3D::snap(const Base::Vector3d& rawProjected, const Sketcher3D::GeoElementId3D& preselection)
{
    snapPosition = rawProjected;
    snapped = false;

    if (!preselection.isValid()) {
        return;
    }

    Base::Vector3d point;
    if (!viewProvider.getSketch3DObject()->getPointAt(preselection, point)) {
        return;
    }

    snapPosition = point;
    snapped = true;
}
