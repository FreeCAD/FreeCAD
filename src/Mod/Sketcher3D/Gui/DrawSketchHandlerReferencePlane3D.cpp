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

#include <Inventor/nodes/SoTransform.h>

#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Gui/Utilities.h>
#include <Gui/Command.h>
#include <Gui/PrefWidgets.h>
#include <Mod/Sketcher3D/App/GeomReferencePlane3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerReferencePlane3D.h"
#include "Sketcher3DToolWidget.h"
#include "ViewProviderSketch3D.h"

using namespace Sketcher3DGui;

namespace
{

class ReferencePlaneAngleWidget: public Sketcher3DToolWidget
{
public:
    explicit ReferencePlaneAngleWidget(QWidget* parent = nullptr)
        : Sketcher3DToolWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);

        spin = new Gui::PrefQuantitySpinBox(this);
        spin->setUnit(Base::Unit::Angle);
        spin->setValue(Base::Quantity(0.0, Base::Unit::Angle));

        auto* form = new QFormLayout();
        form->addRow(tr("Angle:"), spin);
        root->addLayout(form);

        spin->selectNumber();
        spin->setFocus();
    }

    double angleDegrees() const
    {
        return spin->rawValue();
    }

private:
    Gui::PrefQuantitySpinBox* spin {nullptr};
};

}  // namespace

DrawSketchHandlerReferencePlane3D::DrawSketchHandlerReferencePlane3D() = default;

DrawSketchHandlerReferencePlane3D::~DrawSketchHandlerReferencePlane3D() = default;

void DrawSketchHandlerReferencePlane3D::onActivated()
{
    arrowSwitch = new SoSwitch();
    arrowSwitch->whichChild = SO_SWITCH_NONE;
    getPreviewRoot()->addChild(arrowSwitch);

    auto* group = new SoSeparator();
    arrowSwitch->addChild(group);

    arrowTransform = new SoTransform();
    group->addChild(arrowTransform);

    auto* mat = new SoMaterial();
    mat->diffuseColor.setValue(1.0F, 0.75F, 0.1F);
    group->addChild(mat);

    auto* style = new SoDrawStyle();
    style->lineWidth.setValue(2.5F);
    group->addChild(style);

    auto* coords = new SoCoordinate3();
    coords->point.set1Value(0, SbVec3f(7.5F, 0.0F, 0.0F));
    coords->point.set1Value(1, SbVec3f(7.5F, 10.0F, 0.0F));
    group->addChild(coords);

    auto* lines = new SoLineSet();
    lines->numVertices.setValue(2);
    group->addChild(lines);
}

bool DrawSketchHandlerReferencePlane3D::keyPressed(int key)
{
    if (key == SoKeyboardEvent::ESCAPE && phase != Phase::None) {
        reset();
        return true;
    }
    return DrawSketchHandler3D::keyPressed(key);
}

bool DrawSketchHandlerReferencePlane3D::pressButton(const Base::Vector3d& pos)
{
    switch (phase) {
        case Phase::HaveLine:
            commitPlane(
                Sketcher3D::GeomReferencePlane3D::fromThreePoints(points[0], points[1], pos),
                points[0]
            );
            reset();
            return true;
        case Phase::CollectPoints:
            points[nPoints++] = pos;
            if (nPoints >= 3) {
                commitPlane(
                    Sketcher3D::GeomReferencePlane3D::fromThreePoints(points[0], points[1], points[2]),
                    points[0]
                );
                reset();
            }
            return true;
        case Phase::None:
            break;
    }

    const auto& pre = getPreselection();
    Base::Vector3d vertex;
    if (getSketch()->getPointAt(pre, vertex)) {
        points[0] = vertex;
        nPoints = 1;
        phase = Phase::CollectPoints;
    }
    else if (pre.Kind == Sketcher3D::GeoKind::Line) {
        auto* line = getSketch()->getGeometry<Part::GeomLineSegment>(pre.GeoId);
        points[0] = line->getStartPoint();
        points[1] = line->getEndPoint();
        phase = Phase::HaveLine;

        auto dir = (points[1] - points[0]).Normalized();
        auto mid = (points[0] + points[1]) * 0.5;
        auto radial = getSketchVP()->getActivePlaneFrame().normal.Cross(dir).Normalized();
        auto tangent = dir.Cross(radial).Normalized();
        arrowTransform->translation.setValue(float(mid.x), float(mid.y), float(mid.z));
        arrowTransform->rotation.setValue(
            Base::convertTo<SbRotation>(Base::Rotation::makeRotationByAxes(radial, tangent, dir, "XYZ"))
        );
        arrowSwitch->whichChild = SO_SWITCH_ALL;

        auto widget = std::make_unique<ReferencePlaneAngleWidget>();
        widget->setAcceptCallback([this]() {
            auto* w = static_cast<ReferencePlaneAngleWidget*>(toolWidget());
            commitPlane(
                Sketcher3D::GeomReferencePlane3D::fromLineAndAngle(
                    points[0],
                    points[1],
                    getSketchVP()->getActivePlaneFrame().normal,
                    Base::toRadians(w->angleDegrees())
                ),
                points[0]
            );
            reset();
        });
        setToolWidget(std::move(widget));
    }
    return true;
}

void DrawSketchHandlerReferencePlane3D::commitPlane(
    std::unique_ptr<Sketcher3D::GeomReferencePlane3D> plane,
    const Base::Vector3d& base
)
{
    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create 3D Reference Plane")
    );
    getSketch()->addGeometry(std::move(plane), true);
    getSketch()->recomputeFeature();
    getSketchVP()->setPlaneBase(base);
    Gui::Command::commitCommand(tid);
}

void DrawSketchHandlerReferencePlane3D::reset()
{
    clearToolWidget();
    arrowSwitch->whichChild = SO_SWITCH_NONE;
    phase = Phase::None;
    nPoints = 0;
}
