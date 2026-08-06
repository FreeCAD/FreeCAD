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
#include <Gui/Command.h>
#include <Mod/Sketcher3D/App/GeomReferencePlane3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerReferencePlane3D.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"

using namespace Sketcher3DGui;

DrawSketchHandlerReferencePlane3D::DrawSketchHandlerReferencePlane3D() = default;
DrawSketchHandlerReferencePlane3D::~DrawSketchHandlerReferencePlane3D() = default;

void DrawSketchHandlerReferencePlane3D::onActivated()
{
    resetToPickFirst();
}

bool DrawSketchHandlerReferencePlane3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE && state != State::PickFirst) {
        resetToPickFirst();
        return true;
    }
    return DrawSketchHandler3D::keyPressed(key);
}

bool DrawSketchHandlerReferencePlane3D::pressButton(const Base::Vector3d& pos)
{
    if (state == State::PickFirst) {
        threePoints[0] = pos;
        if (auto* vp = getSketchVP()) {
            vp->setPlaneBase(pos);
        }
        state = State::PickSecondPoint;
        return true;
    }

    if (state == State::PickSecondPoint) {
        threePoints[1] = pos;
        state = State::PickThirdPoint;
        return true;
    }

    threePoints[2] = pos;
    auto plane = Sketcher3D::GeomReferencePlane3D::fromThreePoints(
        threePoints[0],
        threePoints[1],
        threePoints[2]
    );

    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create 3D Reference Plane")
    );
    auto* sketch = getSketch();
    auto* sketchVP = getSketchVP();
    sketch->addGeometry(std::move(plane), true);
    sketch->recomputeFeature();
    sketchVP->setPlaneBase(threePoints[0]);
    Gui::Command::commitCommand(tid);
    resetToPickFirst();
    return true;
}

void DrawSketchHandlerReferencePlane3D::resetToPickFirst()
{
    state = State::PickFirst;
    threePoints.fill(Base::Vector3d());
}
