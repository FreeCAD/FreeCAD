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

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Gui/Command.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerPoint3D.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

DrawSketchHandlerPoint3D::DrawSketchHandlerPoint3D() = default;

DrawSketchHandlerPoint3D::~DrawSketchHandlerPoint3D() = default;

void DrawSketchHandlerPoint3D::onActivated()
{
    SoSeparator* root = getPreviewRoot();
    if (!root) {
        return;
    }

    auto* material = new SoMaterial();
    previewMaterial = material;
    applyConstructionPreviewColor(previewMaterial);
    root->addChild(material);

    auto* style = new SoDrawStyle();
    style->pointSize.setValue(8.0F);
    root->addChild(style);

    previewCoords = new SoCoordinate3();
    previewCoords->point.setNum(1);
    previewCoords->point.set1Value(0, 0.0F, 0.0F, 0.0F);
    root->addChild(previewCoords);

    auto* pointSet = new SoPointSet();
    pointSet->numPoints.setValue(1);
    root->addChild(pointSet);
}

bool DrawSketchHandlerPoint3D::mouseMove(const Base::Vector3d& pos)
{
    applyConstructionPreviewColor(previewMaterial);
    if (previewCoords) {
        previewCoords->point.set1Value(
            0,
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
    }
    return false;
}

bool DrawSketchHandlerPoint3D::pressButton(const Base::Vector3d& pos)
{
    Sketcher3D::Sketch3DObject* sketch = getSketch();
    if (!sketch) {
        return false;
    }

    std::vector<AutoConstraint3D> sugConstr;
    seekAutoConstraint(sugConstr, pos, Base::Vector3d());

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create 3D point"));
    int newGeoId = sketch->addGeometry(std::make_unique<Part::GeomPoint>(pos), isConstructionMode());
    createAutoConstraints(sugConstr, newGeoId, Sketcher3D::PointPos::none, Sketcher3D::GeoKind::Point);

    sketch->recomputeFeature();
    Gui::Command::commitCommand(tid);

    if (auto* vp = getSketchVP()) {
        vp->setPlaneBase(pos);
    }

    return true;
}
