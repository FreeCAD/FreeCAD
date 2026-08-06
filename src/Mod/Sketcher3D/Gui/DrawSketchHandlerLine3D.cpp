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
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerLine3D.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

DrawSketchHandlerLine3D::DrawSketchHandlerLine3D() = default;

DrawSketchHandlerLine3D::~DrawSketchHandlerLine3D() = default;

void DrawSketchHandlerLine3D::onActivated()
{
    setupLineRubberBandPreview();
}

bool DrawSketchHandlerLine3D::mouseMove(const Base::Vector3d& pos)
{
    applyConstructionPreviewColor(previewMaterial);
    if (state == State::PickSecond && rubberCoords) {
        rubberCoords->point.set1Value(
            1,
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
    }
    return false;
}

bool DrawSketchHandlerLine3D::pressButton(const Base::Vector3d& pos)
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch) {
        return false;
    }

    if (state == State::PickFirst) {
        seekAutoConstraint(sugConstr1, pos, Base::Vector3d());

        startPos = pos;
        state = State::PickSecond;
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
            setRubberBandVisible(true);
        }
        // Anchor the workplane at the segment's start for the second pick.
        if (auto* vp = getSketchVP()) {
            vp->setPlaneBase(pos);
        }
        return true;
    }

    if (pos.IsEqual(startPos, 1e-9)) {
        return true;
    }

    std::vector<AutoConstraint3D> sugConstr2;
    seekAutoConstraint(sugConstr2, pos, pos - startPos);

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create 3D line"));
    auto seg = std::make_unique<Part::GeomLineSegment>();
    seg->setPoints(startPos, pos);
    int newGeoId = sketch->addGeometry(std::move(seg), isConstructionMode());
    if (newGeoId >= 0) {
        createAutoConstraints(sugConstr1, newGeoId, Sketcher3D::PointPos::start, Sketcher3D::GeoKind::Line);
        createAutoConstraints(sugConstr2, newGeoId, Sketcher3D::PointPos::end, Sketcher3D::GeoKind::Line);
    }
    sugConstr1.clear();
    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);

    if (auto* vp = getSketchVP()) {
        vp->setPlaneBase(pos);
    }

    resetToPickFirst();
    return true;
}

bool DrawSketchHandlerLine3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE) {
        if (state == State::PickSecond) {
            resetToPickFirst();
            return true;
        }
    }
    return DrawSketchHandler3D::keyPressed(key);
}

void DrawSketchHandlerLine3D::resetToPickFirst()
{
    state = State::PickFirst;
    sugConstr1.clear();
    if (rubberSwitch) {
        setRubberBandVisible(false);
    }
}
