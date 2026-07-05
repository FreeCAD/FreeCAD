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

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandler3D.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

DrawSketchHandler3D::DrawSketchHandler3D() = default;

DrawSketchHandler3D::~DrawSketchHandler3D() = default;

void DrawSketchHandler3D::activate(ViewProviderSketch3D* v)
{
    vp = v;
    preview = new SoSeparator();
    preview->ref();
    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    preview->addChild(pick);
    if (vp) {
        vp->getRoot()->addChild(preview);
    }
    onActivated();
}

void DrawSketchHandler3D::quit()
{
    if (preview && vp) {
        vp->getRoot()->removeChild(preview);
    }
    if (preview) {
        preview->unref();
        preview = nullptr;
    }
    vp = nullptr;
}

bool DrawSketchHandler3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE && vp) {
        vp->purgeHandler();
        return true;
    }
    return false;
}

Sketcher3D::Sketch3DObject* DrawSketchHandler3D::getSketch() const
{
    return vp ? vp->getSketch3DObject() : nullptr;
}

DrawSketchHandler3D::PreselectionData DrawSketchHandler3D::getPreselectionData() const
{
    PreselectionData preSelData;
    if (!vp) {
        return preSelData;
    }

    const Sketcher3D::GeoElementId3D& target = vp->getSnapTarget();
    if (!target.isValid()) {
        return preSelData;
    }

    preSelData.geoId = target.GeoId;
    preSelData.posId = target.Pos;
    preSelData.kind = target.Kind;

    if (target.Kind == Sketcher3D::GeoKind::Line) {
        const Sketcher3D::Sketch3DObject* sketch = getSketch();
        if (!sketch) {
            return preSelData;
        }
        if (auto* line = sketch->getGeometry<Part::GeomLineSegment>(target.GeoId)) {
            preSelData.hitShapeDir = line->getEndPoint() - line->getStartPoint();
            preSelData.isLine = true;
        }
    }

    return preSelData;
}

int DrawSketchHandler3D::seekAutoConstraint(
    std::vector<AutoConstraint3D>& suggestedConstraints,
    const Base::Vector3d& Pos,
    const Base::Vector3d& Dir,
    AutoConstraint3D::TargetType type
) const
{
    suggestedConstraints.clear();

    if (!vp) {
        return 0;
    }

    seekPreselectionAutoConstraint(suggestedConstraints, Pos, Dir, type);

    return static_cast<int>(suggestedConstraints.size());
}

void DrawSketchHandler3D::seekPreselectionAutoConstraint(
    std::vector<AutoConstraint3D>& suggestedConstraints,
    const Base::Vector3d& Pos,
    const Base::Vector3d& Dir,
    AutoConstraint3D::TargetType type
) const
{
    (void)Pos;
    (void)Dir;

    PreselectionData preSel = getPreselectionData();
    if (preSel.geoId == Sketcher3D::GeoEnum3D::GeoUndef) {
        return;
    }
    if (type != AutoConstraint3D::VERTEX && type != AutoConstraint3D::VERTEX_NO_TANGENCY) {
        return;
    }

    bool isPoint = preSel.kind == Sketcher3D::GeoKind::Point;
    bool isLineEndpoint = preSel.kind == Sketcher3D::GeoKind::Line
        && (preSel.posId == Sketcher3D::PointPos::start || preSel.posId == Sketcher3D::PointPos::end);
    if (isPoint || isLineEndpoint) {
        AutoConstraint3D constr;
        constr.Type = Sketcher3D::Constraint3D::Coincident3D;
        constr.GeoId = preSel.geoId;
        constr.PosId = preSel.posId;
        constr.Kind = preSel.kind;
        suggestedConstraints.push_back(constr);
    }
}

void DrawSketchHandler3D::createAutoConstraints(
    const std::vector<AutoConstraint3D>& autoConstrs,
    int geoId1,
    Sketcher3D::PointPos posId1,
    Sketcher3D::GeoKind geoKind1
) const
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch || autoConstrs.empty() || geoId1 < 0) {
        return;
    }

    Sketcher3D::GeoElementId3D newPoint(geoId1, posId1, geoKind1);

    for (const AutoConstraint3D& cstr : autoConstrs) {
        Sketcher3D::GeoElementId3D target(cstr.GeoId, cstr.PosId, cstr.Kind);
        switch (cstr.Type) {
            case Sketcher3D::Constraint3D::Coincident3D: {
                if (!target.isValid() || target == newPoint
                    || sketch->arePointsCoincident3D(newPoint, target)) {
                    continue;
                }
                Sketcher3D::Constraint3D c;
                c.Type = Sketcher3D::Constraint3D::Coincident3D;
                c.setElements({newPoint, target});
                sketch->addConstraint(c);
                break;
            }
            default:
                break;
        }
    }
}
