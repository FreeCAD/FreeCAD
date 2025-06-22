/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef _PreComp_
#include <limits>
#include <QMessageBox>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>

#include "TaskPrimitiveParameters.h"
#include "ui_TaskPrimitiveParameters.h"


using namespace PartDesignGui;

// clang-format off
TaskBoxPrimitives::TaskBoxPrimitives(ViewProviderPrimitive* vp, QWidget* parent)
  : TaskBox(QPixmap(),tr("Primitive Parameters"), true, parent)
  , ui(new Ui_DlgPrimitives)
  , vp(vp)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    int index = 0;
    switch(getObject<PartDesign::FeaturePrimitive>()->getPrimitiveType()) {

        case PartDesign::FeaturePrimitive::Box:
            index = 1;
            ui->boxLength->setValue(getObject<PartDesign::Box>()->Length.getValue());
            ui->boxLength->bind(getObject<PartDesign::Box>()->Length);
            ui->boxHeight->setValue(getObject<PartDesign::Box>()->Height.getValue());
            ui->boxHeight->bind(getObject<PartDesign::Box>()->Height);
            ui->boxWidth->setValue(getObject<PartDesign::Box>()->Width.getValue());
            ui->boxWidth->bind(getObject<PartDesign::Box>()->Width);
            ui->boxLength->setMinimum(getObject<PartDesign::Box>()->Length.getMinimum());
            ui->boxLength->setMaximum(getObject<PartDesign::Box>()->Length.getMaximum());
            ui->boxWidth->setMinimum(getObject<PartDesign::Box>()->Width.getMinimum());
            ui->boxWidth->setMaximum(getObject<PartDesign::Box>()->Width.getMaximum());
            ui->boxHeight->setMinimum(getObject<PartDesign::Box>()->Height.getMinimum());
            ui->boxHeight->setMaximum(getObject<PartDesign::Box>()->Height.getMaximum());
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            index = 2;
            ui->cylinderAngle->setValue(getObject<PartDesign::Cylinder>()->Angle.getValue());
            ui->cylinderAngle->bind(getObject<PartDesign::Cylinder>()->Angle);
            ui->cylinderHeight->setValue(getObject<PartDesign::Cylinder>()->Height.getValue());
            ui->cylinderHeight->bind(getObject<PartDesign::Cylinder>()->Height);
            ui->cylinderRadius->setValue(getObject<PartDesign::Cylinder>()->Radius.getValue());
            ui->cylinderRadius->bind(getObject<PartDesign::Cylinder>()->Radius);
            ui->cylinderXSkew->setValue(getObject<PartDesign::Cylinder>()->FirstAngle.getValue());
            ui->cylinderXSkew->bind(getObject<PartDesign::Cylinder>()->FirstAngle);
            ui->cylinderYSkew->setValue(getObject<PartDesign::Cylinder>()->SecondAngle.getValue());
            ui->cylinderYSkew->bind(getObject<PartDesign::Cylinder>()->SecondAngle);
            ui->cylinderAngle->setMaximum(getObject<PartDesign::Cylinder>()->Angle.getMaximum());
            ui->cylinderAngle->setMinimum(getObject<PartDesign::Cylinder>()->Angle.getMinimum());
            ui->cylinderHeight->setMaximum(getObject<PartDesign::Cylinder>()->Height.getMaximum());
            ui->cylinderHeight->setMinimum(getObject<PartDesign::Cylinder>()->Height.getMinimum());
            ui->cylinderRadius->setMaximum(getObject<PartDesign::Cylinder>()->Radius.getMaximum());
            ui->cylinderRadius->setMinimum(getObject<PartDesign::Cylinder>()->Radius.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Sphere:
            index = 4;
            ui->sphereAngle1->setValue(getObject<PartDesign::Sphere>()->Angle1.getValue());
            ui->sphereAngle1->bind(getObject<PartDesign::Sphere>()->Angle1);
            ui->sphereAngle2->setValue(getObject<PartDesign::Sphere>()->Angle2.getValue());
            ui->sphereAngle2->bind(getObject<PartDesign::Sphere>()->Angle2);
            ui->sphereAngle3->setValue(getObject<PartDesign::Sphere>()->Angle3.getValue());
            ui->sphereAngle3->bind(getObject<PartDesign::Sphere>()->Angle3);
            ui->sphereRadius->setValue(getObject<PartDesign::Sphere>()->Radius.getValue());
            ui->sphereRadius->bind(getObject<PartDesign::Sphere>()->Radius);
            ui->sphereAngle1->setMaximum(ui->sphereAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->sphereAngle1->setMinimum(getObject<PartDesign::Sphere>()->Angle1.getMinimum());
            ui->sphereAngle2->setMaximum(getObject<PartDesign::Sphere>()->Angle2.getMaximum());
            ui->sphereAngle2->setMinimum(ui->sphereAngle1->rawValue());
            ui->sphereAngle3->setMaximum(getObject<PartDesign::Sphere>()->Angle3.getMaximum());
            ui->sphereAngle3->setMinimum(getObject<PartDesign::Sphere>()->Angle3.getMinimum());
            ui->sphereRadius->setMaximum(getObject<PartDesign::Sphere>()->Radius.getMaximum());
            ui->sphereRadius->setMinimum(getObject<PartDesign::Sphere>()->Radius.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Cone:
            index = 3;
            ui->coneAngle->setValue(getObject<PartDesign::Cone>()->Angle.getValue());
            ui->coneAngle->bind(getObject<PartDesign::Cone>()->Angle);
            ui->coneHeight->setValue(getObject<PartDesign::Cone>()->Height.getValue());
            ui->coneHeight->bind(getObject<PartDesign::Cone>()->Height);
            ui->coneRadius1->setValue(getObject<PartDesign::Cone>()->Radius1.getValue());
            ui->coneRadius1->bind(getObject<PartDesign::Cone>()->Radius1);
            ui->coneRadius2->setValue(getObject<PartDesign::Cone>()->Radius2.getValue());
            ui->coneRadius2->bind(getObject<PartDesign::Cone>()->Radius2);
            ui->coneAngle->setMaximum(getObject<PartDesign::Cone>()->Angle.getMaximum());
            ui->coneAngle->setMinimum(getObject<PartDesign::Cone>()->Angle.getMinimum());
            ui->coneHeight->setMaximum(getObject<PartDesign::Cone>()->Height.getMaximum());
            ui->coneHeight->setMinimum(getObject<PartDesign::Cone>()->Height.getMinimum());
            ui->coneRadius1->setMaximum(getObject<PartDesign::Cone>()->Radius1.getMaximum());
            ui->coneRadius1->setMinimum(getObject<PartDesign::Cone>()->Radius1.getMinimum());
            ui->coneRadius2->setMaximum(getObject<PartDesign::Cone>()->Radius2.getMaximum());
            ui->coneRadius2->setMinimum(getObject<PartDesign::Cone>()->Radius2.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Ellipsoid:
            index = 5;
            ui->ellipsoidAngle1->setValue(getObject<PartDesign::Ellipsoid>()->Angle1.getValue());
            ui->ellipsoidAngle1->bind(getObject<PartDesign::Ellipsoid>()->Angle1);
            ui->ellipsoidAngle2->setValue(getObject<PartDesign::Ellipsoid>()->Angle2.getValue());
            ui->ellipsoidAngle2->bind(getObject<PartDesign::Ellipsoid>()->Angle2);
            ui->ellipsoidAngle3->setValue(getObject<PartDesign::Ellipsoid>()->Angle3.getValue());
            ui->ellipsoidAngle3->bind(getObject<PartDesign::Ellipsoid>()->Angle3);
            ui->ellipsoidRadius1->setValue(getObject<PartDesign::Ellipsoid>()->Radius1.getValue());
            ui->ellipsoidRadius1->bind(getObject<PartDesign::Ellipsoid>()->Radius1);
            ui->ellipsoidRadius2->setValue(getObject<PartDesign::Ellipsoid>()->Radius2.getValue());
            ui->ellipsoidRadius2->bind(getObject<PartDesign::Ellipsoid>()->Radius2);
            ui->ellipsoidRadius3->setValue(getObject<PartDesign::Ellipsoid>()->Radius3.getValue());
            ui->ellipsoidRadius3->bind(getObject<PartDesign::Ellipsoid>()->Radius3);
            ui->ellipsoidAngle1->setMaximum(ui->ellipsoidAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->ellipsoidAngle1->setMinimum(getObject<PartDesign::Ellipsoid>()->Angle1.getMinimum());
            ui->ellipsoidAngle2->setMaximum(getObject<PartDesign::Ellipsoid>()->Angle2.getMaximum());
            ui->ellipsoidAngle2->setMinimum(ui->ellipsoidAngle1->rawValue());
            ui->ellipsoidAngle3->setMaximum(getObject<PartDesign::Ellipsoid>()->Angle3.getMaximum());
            ui->ellipsoidAngle3->setMinimum(getObject<PartDesign::Ellipsoid>()->Angle3.getMinimum());
            ui->ellipsoidRadius1->setMinimum(getObject<PartDesign::Ellipsoid>()->Radius1.getMinimum());
            ui->ellipsoidRadius1->setMaximum(getObject<PartDesign::Ellipsoid>()->Radius1.getMaximum());
            ui->ellipsoidRadius2->setMinimum(getObject<PartDesign::Ellipsoid>()->Radius2.getMinimum());
            ui->ellipsoidRadius2->setMaximum(getObject<PartDesign::Ellipsoid>()->Radius2.getMaximum());
            ui->ellipsoidRadius3->setMinimum(getObject<PartDesign::Ellipsoid>()->Radius3.getMinimum());
            ui->ellipsoidRadius3->setMaximum(getObject<PartDesign::Ellipsoid>()->Radius3.getMaximum());
            break;
        case PartDesign::FeaturePrimitive::Torus:
            index = 6;
            ui->torusAngle1->setValue(getObject<PartDesign::Torus>()->Angle1.getValue());
            ui->torusAngle1->bind(getObject<PartDesign::Torus>()->Angle1);
            ui->torusAngle2->setValue(getObject<PartDesign::Torus>()->Angle2.getValue());
            ui->torusAngle2->bind(getObject<PartDesign::Torus>()->Angle2);
            ui->torusAngle3->setValue(getObject<PartDesign::Torus>()->Angle3.getValue());
            ui->torusAngle3->bind(getObject<PartDesign::Torus>()->Angle3);
            ui->torusRadius1->setValue(getObject<PartDesign::Torus>()->Radius1.getValue());
            ui->torusRadius1->bind(getObject<PartDesign::Torus>()->Radius1);
            ui->torusRadius2->setValue(getObject<PartDesign::Torus>()->Radius2.getValue());
            ui->torusRadius2->bind(getObject<PartDesign::Torus>()->Radius2);
            ui->torusAngle1->setMaximum(ui->torusAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->torusAngle1->setMinimum(getObject<PartDesign::Torus>()->Angle1.getMinimum());
            ui->torusAngle2->setMaximum(getObject<PartDesign::Torus>()->Angle2.getMaximum());
            ui->torusAngle2->setMinimum(ui->torusAngle1->rawValue());
            ui->torusAngle3->setMaximum(getObject<PartDesign::Torus>()->Angle3.getMaximum());
            ui->torusAngle3->setMinimum(getObject<PartDesign::Torus>()->Angle3.getMinimum());
            // this is the outer radius that must not be smaller than the inner one
            // otherwise the geometry is impossible and we can even get a crash:
            // https://forum.freecad.org/viewtopic.php?f=3&t=44467
            ui->torusRadius1->setMaximum(getObject<PartDesign::Torus>()->Radius1.getMaximum());
            ui->torusRadius1->setMinimum(ui->torusRadius2->rawValue());
            ui->torusRadius2->setMaximum(ui->torusRadius1->rawValue());
            ui->torusRadius2->setMinimum(getObject<PartDesign::Torus>()->Radius2.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Prism:
            index = 7;
            ui->prismPolygon->setValue(getObject<PartDesign::Prism>()->Polygon.getValue());
            ui->prismCircumradius->setValue(getObject<PartDesign::Prism>()->Circumradius.getValue());
            ui->prismCircumradius->bind(getObject<PartDesign::Prism>()->Circumradius);
            ui->prismHeight->setValue(getObject<PartDesign::Prism>()->Height.getValue());
            ui->prismHeight->bind(getObject<PartDesign::Prism>()->Height);
            ui->prismXSkew->setValue(getObject<PartDesign::Prism>()->FirstAngle.getValue());
            ui->prismXSkew->bind(getObject<PartDesign::Prism>()->FirstAngle);
            ui->prismYSkew->setValue(getObject<PartDesign::Prism>()->SecondAngle.getValue());
            ui->prismYSkew->bind(getObject<PartDesign::Prism>()->SecondAngle);
            ui->prismCircumradius->setMaximum(getObject<PartDesign::Prism>()->Circumradius.getMaximum());
            ui->prismCircumradius->setMinimum(getObject<PartDesign::Prism>()->Circumradius.getMinimum());
            ui->prismHeight->setMaximum(getObject<PartDesign::Prism>()->Height.getMaximum());
            ui->prismHeight->setMinimum(getObject<PartDesign::Prism>()->Height.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Wedge:
            index = 8;
            ui->wedgeXmax->setValue(getObject<PartDesign::Wedge>()->Xmax.getValue());
            ui->wedgeXmax->bind(getObject<PartDesign::Wedge>()->Xmax);
            ui->wedgeXmin->setValue(getObject<PartDesign::Wedge>()->Xmin.getValue());
            ui->wedgeXmin->bind(getObject<PartDesign::Wedge>()->Xmin);
            ui->wedgeX2max->setValue(getObject<PartDesign::Wedge>()->X2max.getValue());
            ui->wedgeX2max->bind(getObject<PartDesign::Wedge>()->X2max);
            ui->wedgeX2min->setValue(getObject<PartDesign::Wedge>()->X2min.getValue());
            ui->wedgeX2min->bind(getObject<PartDesign::Wedge>()->X2min);
            ui->wedgeYmax->setValue(getObject<PartDesign::Wedge>()->Ymax.getValue());
            ui->wedgeYmax->bind(getObject<PartDesign::Wedge>()->Ymax);
            ui->wedgeYmin->setValue(getObject<PartDesign::Wedge>()->Ymin.getValue());
            ui->wedgeYmin->bind(getObject<PartDesign::Wedge>()->Ymin);
            ui->wedgeZmax->setValue(getObject<PartDesign::Wedge>()->Zmax.getValue());
            ui->wedgeZmax->bind(getObject<PartDesign::Wedge>()->Zmax);
            ui->wedgeZmin->setValue(getObject<PartDesign::Wedge>()->Zmin.getValue());
            ui->wedgeZmin->bind(getObject<PartDesign::Wedge>()->Zmin);
            ui->wedgeZ2max->setValue(getObject<PartDesign::Wedge>()->Z2max.getValue());
            ui->wedgeZ2max->bind(getObject<PartDesign::Wedge>()->Z2max);
            ui->wedgeZ2min->setValue(getObject<PartDesign::Wedge>()->Z2min.getValue());
            ui->wedgeZ2min->bind(getObject<PartDesign::Wedge>()->Z2min);
            ui->wedgeXmin->setMinimum(std::numeric_limits<int>::min());
            ui->wedgeXmin->setMaximum(ui->wedgeXmax->rawValue()); // must be < than wedgeXmax
            ui->wedgeYmin->setMinimum(std::numeric_limits<int>::min());
            ui->wedgeYmin->setMaximum(ui->wedgeYmax->rawValue()); // must be < than wedgeYmax
            ui->wedgeZmin->setMinimum(std::numeric_limits<int>::min());
            ui->wedgeZmin->setMaximum(ui->wedgeZmax->rawValue()); // must be < than wedgeZmax
            ui->wedgeX2min->setMinimum(std::numeric_limits<int>::min());;
            ui->wedgeX2min->setMaximum(ui->wedgeX2max->rawValue()); // must be <= than wedgeXmax
            ui->wedgeZ2min->setMinimum(std::numeric_limits<int>::min());;
            ui->wedgeZ2min->setMaximum(ui->wedgeZ2max->rawValue()); // must be <= than wedgeXmax
            ui->wedgeXmax->setMinimum(ui->wedgeXmin->rawValue());
            ui->wedgeXmax->setMaximum(std::numeric_limits<int>::max());
            ui->wedgeYmax->setMinimum(ui->wedgeYmin->rawValue());
            ui->wedgeYmax->setMaximum(std::numeric_limits<int>::max());
            ui->wedgeZmax->setMinimum(ui->wedgeZmin->rawValue());
            ui->wedgeZmax->setMaximum(std::numeric_limits<int>::max());
            ui->wedgeX2max->setMinimum(ui->wedgeX2min->rawValue());
            ui->wedgeX2max->setMaximum(std::numeric_limits<int>::max());
            ui->wedgeZ2max->setMinimum(ui->wedgeZ2min->rawValue());
            ui->wedgeZ2max->setMaximum(std::numeric_limits<int>::max());
            break;
    }

    ui->widgetStack->setCurrentIndex(index);
    ui->widgetStack->setMinimumSize(ui->widgetStack->widget(index)->minimumSize());
    for(int i=0; i<ui->widgetStack->count(); ++i) {

        if(i != index)
            ui->widgetStack->widget(i)->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    }

    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);

    //show the parts coordinate system axis for selection
    if(PartDesign::Body * body = PartDesign::Body::findBodyOf(getObject())) {
        try {
            App::Origin *origin = body->getOrigin();
            Gui::ViewProviderCoordinateSystem* vpOrigin {};
            vpOrigin = static_cast<Gui::ViewProviderCoordinateSystem*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(Gui::DatumElement::Planes | Gui::DatumElement::Axes);
        } catch (const Base::Exception &ex) {
            Base::Console().error ("%s\n", ex.what () );
        }
    }

    // box
    connect(ui->boxLength, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onBoxLengthChanged);
    connect(ui->boxWidth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onBoxWidthChanged);
    connect(ui->boxHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onBoxHeightChanged);

    // cylinder
    connect(ui->cylinderRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onCylinderRadiusChanged);
    connect(ui->cylinderHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onCylinderHeightChanged);
    connect(ui->cylinderXSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onCylinderXSkewChanged);
    connect(ui->cylinderYSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onCylinderYSkewChanged);
    connect(ui->cylinderAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onCylinderAngleChanged);

    // cone
    connect(ui->coneRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onConeRadius1Changed);
    connect(ui->coneRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onConeRadius2Changed);
    connect(ui->coneAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onConeAngleChanged);
    connect(ui->coneHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onConeHeightChanged);

    // sphere
    connect(ui->sphereRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onSphereRadiusChanged);
    connect(ui->sphereAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onSphereAngle1Changed);
    connect(ui->sphereAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onSphereAngle2Changed);
    connect(ui->sphereAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onSphereAngle3Changed);

    // ellipsoid
    connect(ui->ellipsoidRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidRadius1Changed);
    connect(ui->ellipsoidRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidRadius2Changed);
    connect(ui->ellipsoidRadius3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidRadius3Changed);
    connect(ui->ellipsoidAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidAngle1Changed);
    connect(ui->ellipsoidAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidAngle2Changed);
    connect(ui->ellipsoidAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onEllipsoidAngle3Changed);

    // torus
    connect(ui->torusRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onTorusRadius1Changed);
    connect(ui->torusRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onTorusRadius2Changed);
    connect(ui->torusAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onTorusAngle1Changed);
    connect(ui->torusAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onTorusAngle2Changed);
    connect(ui->torusAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onTorusAngle3Changed);

    //prism
    connect(ui->prismCircumradius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onPrismCircumradiusChanged);
    connect(ui->prismHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onPrismHeightChanged);
    connect(ui->prismXSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onPrismXSkewChanged);
    connect(ui->prismYSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onPrismYSkewChanged);
    connect(ui->prismPolygon, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskBoxPrimitives::onPrismPolygonChanged);

    // wedge
    connect(ui->wedgeXmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeXmaxChanged);
    connect(ui->wedgeXmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeXminChanged);
    connect(ui->wedgeYmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeYmaxChanged);
    connect(ui->wedgeYmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeYminChanged);
    connect(ui->wedgeZmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeZmaxChanged);
    connect(ui->wedgeZmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeZminChanged);
    connect(ui->wedgeX2max, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeX2maxChanged);
    connect(ui->wedgeX2min, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeX2minChanged);
    connect(ui->wedgeZ2max, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeZ2maxChanged);
    connect(ui->wedgeZ2min, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskBoxPrimitives::onWedgeZ2minChanged);
}
// clang-format on

/*
 *  Destroys the object and frees any allocated resources
 */
TaskBoxPrimitives::~TaskBoxPrimitives()
{
    // hide the parts coordinate system axis for selection
    try {
        auto obj = getObject();
        if (PartDesign::Body* body = obj ? PartDesign::Body::findBodyOf(obj) : nullptr) {
            App::Origin* origin = body->getOrigin();
            Gui::ViewProviderCoordinateSystem* vpOrigin;
            vpOrigin = static_cast<Gui::ViewProviderCoordinateSystem*>(
                Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        Base::Console().error("%s\n", ex.what());
    }
}

void TaskBoxPrimitives::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj) {
        this->vp = nullptr;
    }
}

void TaskBoxPrimitives::onBoxHeightChanged(double v)
{
    if (auto box = getObject<PartDesign::Box>()) {
        box->Height.setValue(v);
        box->recomputeFeature();
    }
}

void TaskBoxPrimitives::onBoxWidthChanged(double v)
{
    if (auto box = getObject<PartDesign::Box>()) {
        box->Width.setValue(v);
        box->recomputeFeature();
    }
}

void TaskBoxPrimitives::onBoxLengthChanged(double v)
{
    if (auto box = getObject<PartDesign::Box>()) {
        box->Length.setValue(v);
        box->recomputeFeature();
    }
}

void TaskBoxPrimitives::onCylinderAngleChanged(double v)
{
    if (auto cyl = getObject<PartDesign::Cylinder>()) {
        cyl->Angle.setValue(v);
        cyl->recomputeFeature();
    }
}

void TaskBoxPrimitives::onCylinderHeightChanged(double v)
{
    if (auto cyl = getObject<PartDesign::Cylinder>()) {
        cyl->Height.setValue(v);
        cyl->recomputeFeature();
    }
}

void TaskBoxPrimitives::onCylinderRadiusChanged(double v)
{
    if (auto cyl = getObject<PartDesign::Cylinder>()) {
        cyl->Radius.setValue(v);
        cyl->recomputeFeature();
    }
}

void TaskBoxPrimitives::onCylinderXSkewChanged(double v)
{
    if (auto cyl = getObject<PartDesign::Cylinder>()) {
        // we must assure that if the user incremented from e.g. 85 degree with the
        // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
        if ((v < 90.0) && (v > -90.0)) {
            cyl->FirstAngle.setValue(v);
        }
        else {
            if (v == 90.0) {
                cyl->FirstAngle.setValue(cyl->FirstAngle.getMaximum());
            }
            else if (v == -90.0) {
                cyl->FirstAngle.setValue(cyl->FirstAngle.getMinimum());
            }
            ui->cylinderXSkew->setValue(cyl->FirstAngle.getQuantityValue());
        }
        cyl->recomputeFeature();
    }
}

void TaskBoxPrimitives::onCylinderYSkewChanged(double v)
{
    if (auto cyl = getObject<PartDesign::Cylinder>()) {
        // we must assure that if the user incremented from e.g. 85 degree with the
        // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
        if ((v < 90.0) && (v > -90.0)) {
            cyl->SecondAngle.setValue(v);
        }
        else {
            if (v == 90.0) {
                cyl->SecondAngle.setValue(cyl->SecondAngle.getMaximum());
            }
            else if (v == -90.0) {
                cyl->SecondAngle.setValue(cyl->SecondAngle.getMinimum());
            }
            ui->cylinderYSkew->setValue(cyl->SecondAngle.getQuantityValue());
        }
        cyl->recomputeFeature();
    }
}

void TaskBoxPrimitives::onSphereAngle1Changed(double v)
{
    if (auto sph = getObject<PartDesign::Sphere>()) {
        ui->sphereAngle2->setMinimum(v);  // Angle1 must geometrically be <= than Angle2
        sph->Angle1.setValue(v);
        sph->recomputeFeature();
    }
}

void TaskBoxPrimitives::onSphereAngle2Changed(double v)
{
    if (auto sph = getObject<PartDesign::Sphere>()) {
        ui->sphereAngle1->setMaximum(v);  // Angle1 must geometrically be <= than Angle2
        sph->Angle2.setValue(v);
        sph->recomputeFeature();
    }
}

void TaskBoxPrimitives::onSphereAngle3Changed(double v)
{
    if (auto sph = getObject<PartDesign::Sphere>()) {
        sph->Angle3.setValue(v);
        sph->recomputeFeature();
    }
}

void TaskBoxPrimitives::onSphereRadiusChanged(double v)
{
    if (auto sph = getObject<PartDesign::Sphere>()) {
        sph->Radius.setValue(v);
        sph->recomputeFeature();
    }
}

void TaskBoxPrimitives::onConeAngleChanged(double v)
{
    if (auto cone = getObject<PartDesign::Cone>()) {
        cone->Angle.setValue(v);
        cone->recomputeFeature();
    }
}

void TaskBoxPrimitives::onConeHeightChanged(double v)
{
    if (auto cone = getObject<PartDesign::Cone>()) {
        cone->Height.setValue(v);
        cone->recomputeFeature();
    }
}

void TaskBoxPrimitives::onConeRadius1Changed(double v)
{
    if (auto cone = getObject<PartDesign::Cone>()) {
        cone->Radius1.setValue(v);
        cone->recomputeFeature();
    }
}

void TaskBoxPrimitives::onConeRadius2Changed(double v)
{
    if (auto cone = getObject<PartDesign::Cone>()) {
        cone->Radius2.setValue(v);
        cone->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidAngle1Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ui->ellipsoidAngle2->setMinimum(v);  // Angle1 must geometrically be <= than Angle2
        ell->Angle1.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidAngle2Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ui->ellipsoidAngle1->setMaximum(v);  // Angle1 must geometrically be <= than Angle22
        ell->Angle2.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidAngle3Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ell->Angle3.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidRadius1Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ell->Radius1.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidRadius2Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ell->Radius2.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onEllipsoidRadius3Changed(double v)
{
    if (auto ell = getObject<PartDesign::Ellipsoid>()) {
        ell->Radius3.setValue(v);
        ell->recomputeFeature();
    }
}

void TaskBoxPrimitives::onTorusAngle1Changed(double v)
{
    if (auto tor = getObject<PartDesign::Torus>()) {
        ui->torusAngle2->setMinimum(v);  // Angle1 must geometrically be <= than Angle2
        tor->Angle1.setValue(v);
        tor->recomputeFeature();
    }
}

void TaskBoxPrimitives::onTorusAngle2Changed(double v)
{
    if (auto tor = getObject<PartDesign::Torus>()) {
        ui->torusAngle1->setMaximum(v);  // Angle1 must geometrically be <= than Angle2
        tor->Angle2.setValue(v);
        tor->recomputeFeature();
    }
}

void TaskBoxPrimitives::onTorusAngle3Changed(double v)
{
    if (auto tor = getObject<PartDesign::Torus>()) {
        tor->Angle3.setValue(v);
        tor->recomputeFeature();
    }
}

void TaskBoxPrimitives::onTorusRadius1Changed(double v)
{
    if (auto tor = getObject<PartDesign::Torus>()) {
        // this is the outer radius that must not be smaller than the inner one
        // otherwise the geometry is impossible and we can even get a crash:
        // https://forum.freecad.org/viewtopic.php?f=3&t=44467
        ui->torusRadius2->setMaximum(v);
        tor->Radius1.setValue(v);
        tor->recomputeFeature();
    }
}

void TaskBoxPrimitives::onTorusRadius2Changed(double v)
{
    if (auto tor = getObject<PartDesign::Torus>()) {
        ui->torusRadius1->setMinimum(v);
        tor->Radius2.setValue(v);
        tor->recomputeFeature();
    }
}

void TaskBoxPrimitives::onPrismCircumradiusChanged(double v)
{
    if (auto prim = getObject<PartDesign::Prism>()) {
        prim->Circumradius.setValue(v);
        prim->recomputeFeature();
    }
}

void TaskBoxPrimitives::onPrismHeightChanged(double v)
{
    if (auto prim = getObject<PartDesign::Prism>()) {
        prim->Height.setValue(v);
        prim->recomputeFeature();
    }
}

void TaskBoxPrimitives::onPrismXSkewChanged(double v)
{
    if (auto prim = getObject<PartDesign::Prism>()) {
        // we must assure that if the user incremented from e.g. 85 degree with the
        // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
        if ((v < 90.0) && (v > -90.0)) {
            prim->FirstAngle.setValue(v);
        }
        else {
            if (v == 90.0) {
                prim->FirstAngle.setValue(89.99999);
            }
            else if (v == -90.0) {
                prim->FirstAngle.setValue(-89.99999);
            }
            ui->prismXSkew->setValue(prim->FirstAngle.getQuantityValue());
        }
        prim->recomputeFeature();
    }
}

void TaskBoxPrimitives::onPrismYSkewChanged(double v)
{
    if (auto prim = getObject<PartDesign::Prism>()) {
        // we must assure that if the user incremented from e.g. 85 degree with the
        // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
        if ((v < 90.0) && (v > -90.0)) {
            prim->SecondAngle.setValue(v);
        }
        else {
            if (v == 90.0) {
                prim->SecondAngle.setValue(89.99999);
            }
            else if (v == -90.0) {
                prim->SecondAngle.setValue(-89.99999);
            }
            ui->prismYSkew->setValue(prim->SecondAngle.getQuantityValue());
        }
        prim->recomputeFeature();
    }
}

void TaskBoxPrimitives::onPrismPolygonChanged(int v)
{
    if (auto prim = getObject<PartDesign::Prism>()) {
        prim->Polygon.setValue(v);
        prim->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeX2minChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeX2max->setMinimum(v);  // wedgeX2min must be <= than wedgeX2max
        wedge->X2min.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeX2maxChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeX2min->setMaximum(v);  // wedgeX2min must be <= than wedgeX2max
        wedge->X2max.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeXminChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeXmax->setMinimum(v);
        wedge->Xmin.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeXmaxChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeXmin->setMaximum(v);  // must be < than wedgeXmax
        wedge->Xmax.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeYminChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeYmax->setMinimum(v);  // must be > than wedgeYmin
        wedge->Ymin.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeYmaxChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeYmin->setMaximum(v);  // must be < than wedgeYmax
        wedge->Ymax.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeZ2minChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeZ2max->setMinimum(v);  // must be >= than wedgeZ2min
        wedge->Z2min.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeZ2maxChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeZ2min->setMaximum(v);  // must be <= than wedgeZ2max
        wedge->Z2max.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeZminChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeZmax->setMinimum(v);  // must be > than wedgeZmin
        wedge->Zmin.setValue(v);
        wedge->recomputeFeature();
    }
}

void TaskBoxPrimitives::onWedgeZmaxChanged(double v)
{
    if (auto wedge = getObject<PartDesign::Wedge>()) {
        ui->wedgeZmin->setMaximum(v);  // must be < than wedgeZmax
        wedge->Zmax.setValue(v);
        wedge->recomputeFeature();
    }
}


bool TaskBoxPrimitives::setPrimitive(App::DocumentObject* obj)
{
    try {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            return false;
        }

        std::string cmd;
        std::string name(Gui::Command::getObjectCmd(obj));
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, Base::UnitsApi::getDecimals());
        switch (ui->widgetStack->currentIndex()) {
            case 1:  // box
                cmd = fmt::format("{0}.Length='{1}'\n"
                                  "{0}.Width='{2}'\n"
                                  "{0}.Height='{3}'\n",
                                  name,
                                  ui->boxLength->value().getSafeUserString(),
                                  ui->boxWidth->value().getSafeUserString(),
                                  ui->boxHeight->value().getSafeUserString());
                break;

            case 2:  // cylinder
                cmd = fmt::format("{0}.Radius='{1}'\n"
                                  "{0}.Height='{2}'\n"
                                  "{0}.Angle='{3}'\n"
                                  "{0}.FirstAngle='{4}'\n"
                                  "{0}.SecondAngle='{5}'\n",
                                  name,
                                  ui->cylinderRadius->value().getSafeUserString(),
                                  ui->cylinderHeight->value().getSafeUserString(),
                                  ui->cylinderAngle->value().getSafeUserString(),
                                  ui->cylinderXSkew->value().getSafeUserString(),
                                  ui->cylinderYSkew->value().getSafeUserString());
                break;

            case 3:  // cone
                cmd = fmt::format("{0}.Radius1='{1}'\n"
                                  "{0}.Radius2='{2}'\n"
                                  "{0}.Height='{3}'\n"
                                  "{0}.Angle='{4}'\n",
                                  name,
                                  ui->coneRadius1->value().getSafeUserString(),
                                  ui->coneRadius2->value().getSafeUserString(),
                                  ui->coneHeight->value().getSafeUserString(),
                                  ui->coneAngle->value().getSafeUserString());
                break;

            case 4:  // sphere
                cmd = fmt::format("{0}.Radius='{1}'\n"
                                  "{0}.Angle1='{2}'\n"
                                  "{0}.Angle2='{3}'\n"
                                  "{0}.Angle3='{4}'\n",
                                  name,
                                  ui->sphereRadius->value().getSafeUserString(),
                                  ui->sphereAngle1->value().getSafeUserString(),
                                  ui->sphereAngle2->value().getSafeUserString(),
                                  ui->sphereAngle3->value().getSafeUserString());
                break;
            case 5:  // ellipsoid
                cmd = fmt::format("{0}.Radius1='{1}'\n"
                                  "{0}.Radius2='{2}'\n"
                                  "{0}.Radius3='{3}'\n"
                                  "{0}.Angle1='{4}'\n"
                                  "{0}.Angle2='{5}'\n"
                                  "{0}.Angle3='{6}'\n",
                                  name,
                                  ui->ellipsoidRadius1->value().getSafeUserString(),
                                  ui->ellipsoidRadius2->value().getSafeUserString(),
                                  ui->ellipsoidRadius3->value().getSafeUserString(),
                                  ui->ellipsoidAngle1->value().getSafeUserString(),
                                  ui->ellipsoidAngle2->value().getSafeUserString(),
                                  ui->ellipsoidAngle3->value().getSafeUserString());
                break;

            case 6:  // torus
                cmd = fmt::format("{0}.Radius1='{1}'\n"
                                  "{0}.Radius2='{2}'\n"
                                  "{0}.Angle1='{3}'\n"
                                  "{0}.Angle2='{4}'\n"
                                  "{0}.Angle3='{5}'\n",
                                  name,
                                  ui->torusRadius1->value().getSafeUserString(),
                                  ui->torusRadius2->value().getSafeUserString(),
                                  ui->torusAngle1->value().getSafeUserString(),
                                  ui->torusAngle2->value().getSafeUserString(),
                                  ui->torusAngle3->value().getSafeUserString());
                break;
            case 7:  // prism
                cmd = fmt::format("{0}.Polygon={1}\n"
                                  "{0}.Circumradius='{2}'\n"
                                  "{0}.Height='{3}'\n"
                                  "{0}.FirstAngle='{4}'\n"
                                  "{0}.SecondAngle='{5}'\n",
                                  name,
                                  ui->prismPolygon->value(),
                                  ui->prismCircumradius->value().getSafeUserString(),
                                  ui->prismHeight->value().getSafeUserString(),
                                  ui->prismXSkew->value().getSafeUserString(),
                                  ui->prismYSkew->value().getSafeUserString());
                break;
            case 8:  // wedge
                // Xmin/max, Ymin/max and Zmin/max must each not be equal
                if (ui->wedgeXmin->value().getValue() == ui->wedgeXmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(),
                                         tr("Invalid wedge parameters"),
                                         tr("X min must not be equal to X max!"));
                    return false;
                }
                else if (ui->wedgeYmin->value().getValue() == ui->wedgeYmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(),
                                         tr("Invalid wedge parameters"),
                                         tr("Y min must not be equal to Y max!"));
                    return false;
                }
                else if (ui->wedgeZmin->value().getValue() == ui->wedgeZmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(),
                                         tr("Invalid wedge parameters"),
                                         tr("Z min must not be equal to Z max!"));
                    return false;
                }
                cmd = fmt::format("{0}.Xmin='{1}'\n"
                                  "{0}.Ymin='{2}'\n"
                                  "{0}.Zmin='{3}'\n"
                                  "{0}.X2min='{4}'\n"
                                  "{0}.Z2min='{5}'\n"
                                  "{0}.Xmax='{6}'\n"
                                  "{0}.Ymax='{7}'\n"
                                  "{0}.Zmax='{8}'\n"
                                  "{0}.X2max='{9}'\n"
                                  "{0}.Z2max='{10}'\n",
                                  name,
                                  ui->wedgeXmin->value().getSafeUserString(),
                                  ui->wedgeYmin->value().getSafeUserString(),
                                  ui->wedgeZmin->value().getSafeUserString(),
                                  ui->wedgeX2min->value().getSafeUserString(),
                                  ui->wedgeZ2min->value().getSafeUserString(),
                                  ui->wedgeXmax->value().getSafeUserString(),
                                  ui->wedgeYmax->value().getSafeUserString(),
                                  ui->wedgeZmax->value().getSafeUserString(),
                                  ui->wedgeX2max->value().getSafeUserString(),
                                  ui->wedgeZ2max->value().getSafeUserString());
                break;

            default:
                break;
        }

        // Execute the Python block
        // No need to open a transaction because this is already done in the command
        // class or when starting to edit a primitive.
        Gui::Command::runCommand(Gui::Command::Doc, cmd.c_str());
        Gui::Command::runCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this,
                             tr("Create primitive"),
                             QApplication::translate("Exception", e.what()));
        return false;
    }
    return true;
}

TaskPrimitiveParameters::TaskPrimitiveParameters(ViewProviderPrimitive* PrimitiveView)
    : vp_prm(PrimitiveView)
{
    assert(PrimitiveView);

    primitive = new TaskBoxPrimitives(PrimitiveView);
    Content.push_back(primitive);
    parameter = new PartGui::TaskAttacher(PrimitiveView, nullptr, QString(), tr("Attachment"));
    Content.push_back(parameter);
}

TaskPrimitiveParameters::~TaskPrimitiveParameters() = default;

bool TaskPrimitiveParameters::accept()
{
    bool primitiveOK = primitive->setPrimitive(vp_prm->getObject());
    if (!primitiveOK) {
        return primitiveOK;
    }
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");

    return true;
}

bool TaskPrimitiveParameters::reject()
{
    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");

    return true;
}

QDialogButtonBox::StandardButtons TaskPrimitiveParameters::getStandardButtons() const
{
    return Gui::TaskView::TaskDialog::getStandardButtons();
}


#include "moc_TaskPrimitiveParameters.cpp"
