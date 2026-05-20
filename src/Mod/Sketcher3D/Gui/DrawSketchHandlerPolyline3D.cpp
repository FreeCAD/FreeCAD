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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

#include <Gui/Command.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerPolyline3D.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

DrawSketchHandlerPolyline3D::DrawSketchHandlerPolyline3D() = default;

DrawSketchHandlerPolyline3D::~DrawSketchHandlerPolyline3D() = default;

void DrawSketchHandlerPolyline3D::onActivated()
{
    SoSeparator* root = getPreviewRoot();
    if (!root) {
        return;
    }

    auto* material = new SoMaterial();
    material->diffuseColor.setValue(1.0F, 1.0F, 1.0F);
    root->addChild(material);

    auto* style = new SoDrawStyle();
    style->lineWidth.setValue(2.0F);
    root->addChild(style);

    rubberSwitch = new SoSwitch();
    rubberSwitch->whichChild = SO_SWITCH_NONE;
    root->addChild(rubberSwitch);

    auto* rubberGroup = new SoSeparator();
    rubberSwitch->addChild(rubberGroup);

    rubberCoords = new SoCoordinate3();
    rubberCoords->point.setNum(2);
    rubberCoords->point.set1Value(0, 0.0F, 0.0F, 0.0F);
    rubberCoords->point.set1Value(1, 0.0F, 0.0F, 0.0F);
    rubberGroup->addChild(rubberCoords);

    auto* lineSet = new SoLineSet();
    lineSet->numVertices.setNum(1);
    lineSet->numVertices.set1Value(0, 2);
    rubberGroup->addChild(lineSet);
}

bool DrawSketchHandlerPolyline3D::mouseMove(const Base::Vector3d& pos)
{
    if (state == State::PickNext && rubberCoords) {
        rubberCoords->point.set1Value(
            1,
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
    }
    return false;
}

bool DrawSketchHandlerPolyline3D::pressButton(const Base::Vector3d& pos)
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch) {
        return false;
    }

    if (state == State::PickFirst) {
        seekAutoConstraint(sugConstr1, pos, Base::Vector3d());

        lastPos = pos;
        state = State::PickNext;
        if (rubberCoords) {
            rubberCoords->point.set1Value(
                0,
                static_cast<float>(pos.x),
                static_cast<float>(pos.y),
                static_cast<float>(pos.z)
            );
            rubberCoords->point.set1Value(
                1,
                static_cast<float>(pos.x),
                static_cast<float>(pos.y),
                static_cast<float>(pos.z)
            );
        }
        if (rubberSwitch) {
            rubberSwitch->whichChild = SO_SWITCH_ALL;
        }
        if (auto* vp = getSketchVP()) {
            vp->setPlaneBase(pos);
        }
        return true;
    }

    // the chain from pos.
    if (pos.IsEqual(lastPos, 1e-9)) {
        // wait for a non coincident click.
        return true;
    }

    std::vector<AutoConstraint3D> sugConstr2;
    seekAutoConstraint(sugConstr2, pos, pos - lastPos);

    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create 3D polyline segment")
    );
    auto seg = std::make_unique<Part::GeomLineSegment>();
    seg->setPoints(lastPos, pos);
    const int newGeoId = sketch->addGeometry(std::move(seg));

    // Chain the segments with a Coincident3D constraint so they stay connected when constraints are
    // applied.
    if (prevSegGeoId >= 0 && newGeoId >= 0) {
        Sketcher3D::Constraint3D c;
        c.Type = Sketcher3D::Constraint3D::Coincident3D;
        c.setElements({
            Sketcher3D::GeoElementId3D(prevSegGeoId, Sketcher3D::PointPos::end, Sketcher3D::GeoKind::Line),
            Sketcher3D::GeoElementId3D(newGeoId, Sketcher3D::PointPos::start, Sketcher3D::GeoKind::Line),
        });
        sketch->addConstraint(c);
    }

    if (newGeoId >= 0) {
        if (!sugConstr1.empty()) {
            createAutoConstraints(
                sugConstr1,
                newGeoId,
                Sketcher3D::PointPos::start,
                Sketcher3D::GeoKind::Line
            );
            sugConstr1.clear();
        }
        createAutoConstraints(sugConstr2, newGeoId, Sketcher3D::PointPos::end, Sketcher3D::GeoKind::Line);
    }

    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);

    prevSegGeoId = newGeoId;
    lastPos = pos;
    if (rubberCoords) {
        rubberCoords->point.set1Value(
            0,
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
    }
    if (auto* vp = getSketchVP()) {
        vp->setPlaneBase(pos);
    }
    return true;
}

bool DrawSketchHandlerPolyline3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE) {
        sugConstr1.clear();
        // end the polyline.
        return DrawSketchHandler3D::keyPressed(key);
    }
    return DrawSketchHandler3D::keyPressed(key);
}
