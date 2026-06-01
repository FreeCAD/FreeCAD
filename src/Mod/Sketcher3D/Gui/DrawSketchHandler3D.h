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

#include <vector>

#include <Base/Vector3D.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "AutoConstraint3D.h"

class SoSeparator;

namespace Sketcher3D
{
class Sketch3DObject;
}

namespace Sketcher3DGui
{

class ViewProviderSketch3D;

class Sketcher3DGuiExport DrawSketchHandler3D
{
public:
    DrawSketchHandler3D();
    virtual ~DrawSketchHandler3D();

    DrawSketchHandler3D(const DrawSketchHandler3D&) = delete;
    DrawSketchHandler3D& operator=(const DrawSketchHandler3D&) = delete;

    void activate(ViewProviderSketch3D* vp);

    void quit();

    /// pos is in sketch local 3D coords. Return
    /// true if the handler consumed the event.
    virtual bool pressButton(const Base::Vector3d& pos) = 0;

    virtual bool mouseMove(const Base::Vector3d& /*pos*/)
    {
        return false;
    }

    virtual bool keyPressed(int key);

protected:
    virtual void onActivated()
    {}

    Sketcher3D::Sketch3DObject* getSketch() const;
    ViewProviderSketch3D* getSketchVP() const
    {
        return vp;
    }

    SoSeparator* getPreviewRoot() const
    {
        return preview;
    }

    struct PreselectionData
    {
        int geoId {Sketcher3D::GeoEnum3D::GeoUndef};
        Sketcher3D::PointPos posId {Sketcher3D::PointPos::none};
        Sketcher3D::GeoKind kind {Sketcher3D::GeoKind::Unknown};
        Base::Vector3d hitShapeDir {0.0, 0.0, 0.0};
        bool isLine {false};
    };
    PreselectionData getPreselectionData() const;

    int seekAutoConstraint(
        std::vector<AutoConstraint3D>& suggestedConstraints,
        const Base::Vector3d& Pos,
        const Base::Vector3d& Dir,
        AutoConstraint3D::TargetType type = AutoConstraint3D::VERTEX
    ) const;

    void seekPreselectionAutoConstraint(
        std::vector<AutoConstraint3D>& suggestedConstraints,
        const Base::Vector3d& Pos,
        const Base::Vector3d& Dir,
        AutoConstraint3D::TargetType type
    ) const;

    void createAutoConstraints(
        const std::vector<AutoConstraint3D>& autoConstrs,
        int geoId1,
        Sketcher3D::PointPos posId1 = Sketcher3D::PointPos::none,
        Sketcher3D::GeoKind geoKind1 = Sketcher3D::GeoKind::Unknown
    ) const;

private:
    ViewProviderSketch3D* vp {nullptr};
    SoSeparator* preview {nullptr};
};

}  // namespace Sketcher3DGui
