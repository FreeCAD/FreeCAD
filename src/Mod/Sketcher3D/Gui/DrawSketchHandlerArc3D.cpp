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

#include "DrawSketchHandlerArc3D.h"
#include "Utils.h"

using namespace Sketcher3DGui;

// TODO: need rubber band preview
bool DrawSketchHandlerArc3D::pressButton(const Base::Vector3d& pos)
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch) {
        return false;
    }

    if (state == State::PickFirst) {
        seekAutoConstraint(sugConstr1, pos, Base::Vector3d());
        startPos = pos;
        state = State::PickSecond;
        return true;
    }

    if (state == State::PickSecond) {
        seekAutoConstraint(sugConstr2, pos, pos - startPos);
        endPos = pos;
        state = State::PickThird;
        return true;
    }

    GC_MakeArcOfCircle maker(
        gp_Pnt(startPos.x, startPos.y, startPos.z),
        gp_Pnt(pos.x, pos.y, pos.z),
        gp_Pnt(endPos.x, endPos.y, endPos.z)
    );
    if (!maker.IsDone()) {
        return true;
    }

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create 3D arc"));
    auto arc = std::make_unique<Part::GeomArcOfCircle>();
    arc->setHandle(maker.Value());
    int newGeoId = sketch->addGeometry(std::move(arc), isConstructionMode());
    if (newGeoId >= 0) {
        createAutoConstraints(sugConstr1, newGeoId, Sketcher3D::PointPos::start, Sketcher3D::GeoKind::Arc);
        createAutoConstraints(sugConstr2, newGeoId, Sketcher3D::PointPos::end, Sketcher3D::GeoKind::Arc);
    }
    sugConstr1.clear();
    sugConstr2.clear();
    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);

    state = State::PickFirst;
    return true;
}

bool DrawSketchHandlerArc3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE && state != State::PickFirst) {
        state = State::PickFirst;
        sugConstr1.clear();
        sugConstr2.clear();
        return true;
    }
    return DrawSketchHandler3D::keyPressed(key);
}
