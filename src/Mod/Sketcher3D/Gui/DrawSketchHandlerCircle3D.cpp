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
 **************************************************************************/


#include "PreCompiled.h"

#include <Inventor/events/SoKeyboardEvent.h>

#include <Gui/Command.h>

#include <Mod/Part/App/OpenCascadeAll.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerCircle3D.h"
#include "Utils.h"

using namespace Sketcher3DGui;

// TODO: need rubber band preview
bool DrawSketchHandlerCircle3D::pressButton(const Base::Vector3d& pos)
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch) {
        return false;
    }

    // need seek auto constraint and point on curve
    if (state == State::PickFirst) {
        point1 = pos;
        state = State::PickSecond;
        return true;
    }

    if (state == State::PickSecond) {
        point2 = pos;
        state = State::PickThird;
        return true;
    }

    GC_MakeCircle maker(
        gp_Pnt(point1.x, point1.y, point1.z),
        gp_Pnt(point2.x, point2.y, point2.z),
        gp_Pnt(pos.x, pos.y, pos.z)
    );
    if (!maker.IsDone()) {
        return true;
    }

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create 3D circle"));
    sketch->addGeometry(std::make_unique<Part::GeomCircle>(maker.Value()), isConstructionMode());
    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);

    state = State::PickFirst;
    return true;
}

bool DrawSketchHandlerCircle3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE && state != State::PickFirst) {
        state = State::PickFirst;
        // clear all auto constraints
        return true;
    }
    return DrawSketchHandler3D::keyPressed(key);
}
