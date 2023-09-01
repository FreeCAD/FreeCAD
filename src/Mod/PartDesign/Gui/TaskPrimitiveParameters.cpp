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
# include <QMessageBox>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>

#include "TaskPrimitiveParameters.h"
#include "ui_TaskPrimitiveParameters.h"


using namespace PartDesignGui;

TaskBoxPrimitives::TaskBoxPrimitives(ViewProviderPrimitive* vp, QWidget* parent)
  : TaskBox(QPixmap(),tr("Primitive parameters"), true, parent)
  , ui(new Ui_DlgPrimitives)
  , vp(vp)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    int index = 0;
    switch(static_cast<PartDesign::FeaturePrimitive*>(vp->getObject())->getPrimitiveType()) {

        case PartDesign::FeaturePrimitive::Box:
            index = 1;
            ui->boxLength->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Length.getValue());
            ui->boxLength->bind(static_cast<PartDesign::Box*>(vp->getObject())->Length);
            ui->boxHeight->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Height.getValue());
            ui->boxHeight->bind(static_cast<PartDesign::Box*>(vp->getObject())->Height);
            ui->boxWidth->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Width.getValue());
            ui->boxWidth->bind(static_cast<PartDesign::Box*>(vp->getObject())->Width);
            ui->boxLength->setMinimum(static_cast<PartDesign::Box*>(vp->getObject())->Length.getMinimum());
            ui->boxLength->setMaximum(static_cast<PartDesign::Box*>(vp->getObject())->Length.getMaximum());
            ui->boxWidth->setMinimum(static_cast<PartDesign::Box*>(vp->getObject())->Width.getMinimum());
            ui->boxWidth->setMaximum(static_cast<PartDesign::Box*>(vp->getObject())->Width.getMaximum());
            ui->boxHeight->setMinimum(static_cast<PartDesign::Box*>(vp->getObject())->Height.getMinimum());
            ui->boxHeight->setMaximum(static_cast<PartDesign::Box*>(vp->getObject())->Height.getMaximum());
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            index = 2;
            ui->cylinderAngle->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle.getValue());
            ui->cylinderAngle->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle);
            ui->cylinderHeight->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height.getValue());
            ui->cylinderHeight->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height);
            ui->cylinderRadius->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius.getValue());
            ui->cylinderRadius->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius);
            ui->cylinderXSkew->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->FirstAngle.getValue());
            ui->cylinderXSkew->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->FirstAngle);
            ui->cylinderYSkew->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->SecondAngle.getValue());
            ui->cylinderYSkew->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->SecondAngle);
            ui->cylinderAngle->setMaximum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle.getMaximum());
            ui->cylinderAngle->setMinimum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle.getMinimum());
            ui->cylinderHeight->setMaximum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height.getMaximum());
            ui->cylinderHeight->setMinimum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height.getMinimum());
            ui->cylinderRadius->setMaximum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius.getMaximum());
            ui->cylinderRadius->setMinimum(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Sphere:
            index = 4;
            ui->sphereAngle1->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle1.getValue());
            ui->sphereAngle1->bind(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle1);
            ui->sphereAngle2->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle2.getValue());
            ui->sphereAngle2->bind(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle2);
            ui->sphereAngle3->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle3.getValue());
            ui->sphereAngle3->bind(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle3);
            ui->sphereRadius->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Radius.getValue());
            ui->sphereRadius->bind(static_cast<PartDesign::Sphere*>(vp->getObject())->Radius);
            ui->sphereAngle1->setMaximum(ui->sphereAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->sphereAngle1->setMinimum(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle1.getMinimum());
            ui->sphereAngle2->setMaximum(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle2.getMaximum());
            ui->sphereAngle2->setMinimum(ui->sphereAngle1->rawValue());
            ui->sphereAngle3->setMaximum(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle3.getMaximum());
            ui->sphereAngle3->setMinimum(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle3.getMinimum());
            ui->sphereRadius->setMaximum(static_cast<PartDesign::Sphere*>(vp->getObject())->Radius.getMaximum());
            ui->sphereRadius->setMinimum(static_cast<PartDesign::Sphere*>(vp->getObject())->Radius.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Cone:
            index = 3;
            ui->coneAngle->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Angle.getValue());
            ui->coneAngle->bind(static_cast<PartDesign::Cone*>(vp->getObject())->Angle);
            ui->coneHeight->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Height.getValue());
            ui->coneHeight->bind(static_cast<PartDesign::Cone*>(vp->getObject())->Height);
            ui->coneRadius1->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Radius1.getValue());
            ui->coneRadius1->bind(static_cast<PartDesign::Cone*>(vp->getObject())->Radius1);
            ui->coneRadius2->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Radius2.getValue());
            ui->coneRadius2->bind(static_cast<PartDesign::Cone*>(vp->getObject())->Radius2);
            ui->coneAngle->setMaximum(static_cast<PartDesign::Cone*>(vp->getObject())->Angle.getMaximum());
            ui->coneAngle->setMinimum(static_cast<PartDesign::Cone*>(vp->getObject())->Angle.getMinimum());
            ui->coneHeight->setMaximum(static_cast<PartDesign::Cone*>(vp->getObject())->Height.getMaximum());
            ui->coneHeight->setMinimum(static_cast<PartDesign::Cone*>(vp->getObject())->Height.getMinimum());
            ui->coneRadius1->setMaximum(static_cast<PartDesign::Cone*>(vp->getObject())->Radius1.getMaximum());
            ui->coneRadius1->setMinimum(static_cast<PartDesign::Cone*>(vp->getObject())->Radius1.getMinimum());
            ui->coneRadius2->setMaximum(static_cast<PartDesign::Cone*>(vp->getObject())->Radius2.getMaximum());
            ui->coneRadius2->setMinimum(static_cast<PartDesign::Cone*>(vp->getObject())->Radius2.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Ellipsoid:
            index = 5;
            ui->ellipsoidAngle1->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle1.getValue());
            ui->ellipsoidAngle1->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle1);
            ui->ellipsoidAngle2->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle2.getValue());
            ui->ellipsoidAngle2->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle2);
            ui->ellipsoidAngle3->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle3.getValue());
            ui->ellipsoidAngle3->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle3);
            ui->ellipsoidRadius1->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius1.getValue());
            ui->ellipsoidRadius1->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius1);
            ui->ellipsoidRadius2->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius2.getValue());
            ui->ellipsoidRadius2->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius2);
            ui->ellipsoidRadius3->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius3.getValue());
            ui->ellipsoidRadius3->bind(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius3);
            ui->ellipsoidAngle1->setMaximum(ui->ellipsoidAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->ellipsoidAngle1->setMinimum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle1.getMinimum());
            ui->ellipsoidAngle2->setMaximum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle2.getMaximum());
            ui->ellipsoidAngle2->setMinimum(ui->ellipsoidAngle1->rawValue());
            ui->ellipsoidAngle3->setMaximum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle3.getMaximum());
            ui->ellipsoidAngle3->setMinimum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle3.getMinimum());
            ui->ellipsoidRadius1->setMinimum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius1.getMinimum());
            ui->ellipsoidRadius1->setMaximum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius1.getMaximum());
            ui->ellipsoidRadius2->setMinimum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius2.getMinimum());
            ui->ellipsoidRadius2->setMaximum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius2.getMaximum());
            ui->ellipsoidRadius3->setMinimum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius3.getMinimum());
            ui->ellipsoidRadius3->setMaximum(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius3.getMaximum());
            break;
        case PartDesign::FeaturePrimitive::Torus:
            index = 6;
            ui->torusAngle1->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle1.getValue());
            ui->torusAngle1->bind(static_cast<PartDesign::Torus*>(vp->getObject())->Angle1);
            ui->torusAngle2->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle2.getValue());
            ui->torusAngle2->bind(static_cast<PartDesign::Torus*>(vp->getObject())->Angle2);
            ui->torusAngle3->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle3.getValue());
            ui->torusAngle3->bind(static_cast<PartDesign::Torus*>(vp->getObject())->Angle3);
            ui->torusRadius1->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Radius1.getValue());
            ui->torusRadius1->bind(static_cast<PartDesign::Torus*>(vp->getObject())->Radius1);
            ui->torusRadius2->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Radius2.getValue());
            ui->torusRadius2->bind(static_cast<PartDesign::Torus*>(vp->getObject())->Radius2);
            ui->torusAngle1->setMaximum(ui->torusAngle2->rawValue()); // must geometrically be <= than sphereAngle2
            ui->torusAngle1->setMinimum(static_cast<PartDesign::Torus*>(vp->getObject())->Angle1.getMinimum());
            ui->torusAngle2->setMaximum(static_cast<PartDesign::Torus*>(vp->getObject())->Angle2.getMaximum());
            ui->torusAngle2->setMinimum(ui->torusAngle1->rawValue());
            ui->torusAngle3->setMaximum(static_cast<PartDesign::Torus*>(vp->getObject())->Angle3.getMaximum());
            ui->torusAngle3->setMinimum(static_cast<PartDesign::Torus*>(vp->getObject())->Angle3.getMinimum());
            // this is the outer radius that must not be smaller than the inner one
            // otherwise the geometry is impossible and we can even get a crash:
            // https://forum.freecad.org/viewtopic.php?f=3&t=44467
            ui->torusRadius1->setMaximum(static_cast<PartDesign::Torus*>(vp->getObject())->Radius1.getMaximum());
            ui->torusRadius1->setMinimum(ui->torusRadius2->rawValue());
            ui->torusRadius2->setMaximum(ui->torusRadius1->rawValue());
            ui->torusRadius2->setMinimum(static_cast<PartDesign::Torus*>(vp->getObject())->Radius2.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Prism:
            index = 7;
            ui->prismPolygon->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Polygon.getValue());
            ui->prismCircumradius->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Circumradius.getValue());
            ui->prismCircumradius->bind(static_cast<PartDesign::Prism*>(vp->getObject())->Circumradius);
            ui->prismHeight->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Height.getValue());
            ui->prismHeight->bind(static_cast<PartDesign::Prism*>(vp->getObject())->Height);
            ui->prismXSkew->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->FirstAngle.getValue());
            ui->prismXSkew->bind(static_cast<PartDesign::Prism*>(vp->getObject())->FirstAngle);
            ui->prismYSkew->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->SecondAngle.getValue());
            ui->prismYSkew->bind(static_cast<PartDesign::Prism*>(vp->getObject())->SecondAngle);
            ui->prismCircumradius->setMaximum(static_cast<PartDesign::Prism*>(vp->getObject())->Circumradius.getMaximum());
            ui->prismCircumradius->setMinimum(static_cast<PartDesign::Prism*>(vp->getObject())->Circumradius.getMinimum());
            ui->prismHeight->setMaximum(static_cast<PartDesign::Prism*>(vp->getObject())->Height.getMaximum());
            ui->prismHeight->setMinimum(static_cast<PartDesign::Prism*>(vp->getObject())->Height.getMinimum());
            break;
        case PartDesign::FeaturePrimitive::Wedge:
            index = 8;
            ui->wedgeXmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmax.getValue());
            ui->wedgeXmax->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmax);
            ui->wedgeXmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmin.getValue());
            ui->wedgeXmin->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmin);
            ui->wedgeX2max->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->X2max.getValue());
            ui->wedgeX2max->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->X2max);
            ui->wedgeX2min->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->X2min.getValue());
            ui->wedgeX2min->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->X2min);
            ui->wedgeYmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymax.getValue());
            ui->wedgeYmax->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymax);
            ui->wedgeYmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymin.getValue());
            ui->wedgeYmin->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymin);
            ui->wedgeZmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmax.getValue());
            ui->wedgeZmax->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmax);
            ui->wedgeZmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmin.getValue());
            ui->wedgeZmin->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmin);
            ui->wedgeZ2max->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2max.getValue());
            ui->wedgeZ2max->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2max);
            ui->wedgeZ2min->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2min.getValue());
            ui->wedgeZ2min->bind(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2min);
            ui->wedgeXmin->setMinimum(INT_MIN);
            ui->wedgeXmin->setMaximum(ui->wedgeXmax->rawValue()); // must be < than wedgeXmax
            ui->wedgeYmin->setMinimum(INT_MIN);
            ui->wedgeYmin->setMaximum(ui->wedgeYmax->rawValue()); // must be < than wedgeYmax
            ui->wedgeZmin->setMinimum(INT_MIN);
            ui->wedgeZmin->setMaximum(ui->wedgeZmax->rawValue()); // must be < than wedgeZmax
            ui->wedgeX2min->setMinimum(INT_MIN);
            ui->wedgeX2min->setMaximum(ui->wedgeX2max->rawValue()); // must be <= than wedgeXmax
            ui->wedgeZ2min->setMinimum(INT_MIN);
            ui->wedgeZ2min->setMaximum(ui->wedgeZ2max->rawValue()); // must be <= than wedgeXmax
            ui->wedgeXmax->setMinimum(ui->wedgeXmin->rawValue());
            ui->wedgeXmax->setMaximum(INT_MAX);
            ui->wedgeYmax->setMinimum(ui->wedgeYmin->rawValue());
            ui->wedgeYmax->setMaximum(INT_MAX);
            ui->wedgeZmax->setMinimum(ui->wedgeZmin->rawValue());
            ui->wedgeZmax->setMaximum(INT_MAX);
            ui->wedgeX2max->setMinimum(ui->wedgeX2min->rawValue());
            ui->wedgeX2max->setMaximum(INT_MAX);
            ui->wedgeZ2max->setMinimum(ui->wedgeZ2min->rawValue());
            ui->wedgeZ2max->setMaximum(INT_MAX);
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
    PartDesign::Body * body = PartDesign::Body::findBodyOf(vp->getObject());
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            Gui::ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<Gui::ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, true);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
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

/*
 *  Destroys the object and frees any allocated resources
 */
TaskBoxPrimitives::~TaskBoxPrimitives()
{
    //hide the parts coordinate system axis for selection
    try {
        PartDesign::Body * body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : nullptr;
        if (body) {
            App::Origin *origin = body->getOrigin();
            Gui::ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<Gui::ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what () );
    }
}

void TaskBoxPrimitives::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj)
        this->vp = nullptr;
}

void TaskBoxPrimitives::onBoxHeightChanged(double v) {
    PartDesign::Box* box = static_cast<PartDesign::Box*>(vp->getObject());
    box->Height.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onBoxWidthChanged(double v) {
    PartDesign::Box* box = static_cast<PartDesign::Box*>(vp->getObject());
    box->Width.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onBoxLengthChanged(double v) {
    PartDesign::Box* box = static_cast<PartDesign::Box*>(vp->getObject());
    box->Length.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onCylinderAngleChanged(double v) {
    PartDesign::Cylinder* cyl = static_cast<PartDesign::Cylinder*>(vp->getObject());
    cyl->Angle.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onCylinderHeightChanged(double v) {
    PartDesign::Cylinder* cyl = static_cast<PartDesign::Cylinder*>(vp->getObject());
    cyl->Height.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onCylinderRadiusChanged(double v) {
    PartDesign::Cylinder* cyl = static_cast<PartDesign::Cylinder*>(vp->getObject());
    cyl->Radius.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onCylinderXSkewChanged(double v) {
    PartDesign::Cylinder* cyl = static_cast<PartDesign::Cylinder*>(vp->getObject());
    // we must assure that if the user incremented from e.g. 85 degree with the
    // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
    if ((v < 90.0) && (v > -90.0)) {
        cyl->FirstAngle.setValue(v);
    }
    else {
        if (v == 90.0)
            cyl->FirstAngle.setValue(cyl->FirstAngle.getMaximum());
        else if (v == -90.0)
            cyl->FirstAngle.setValue(cyl->FirstAngle.getMinimum());
        ui->cylinderXSkew->setValue(cyl->FirstAngle.getQuantityValue());
    }
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onCylinderYSkewChanged(double v) {
    PartDesign::Cylinder* cyl = static_cast<PartDesign::Cylinder*>(vp->getObject());
    // we must assure that if the user incremented from e.g. 85 degree with the
    // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
    if ((v < 90.0) && (v > -90.0)) {
        cyl->SecondAngle.setValue(v);
    }
    else {
        if (v == 90.0)
            cyl->SecondAngle.setValue(cyl->SecondAngle.getMaximum());
        else if (v == -90.0)
            cyl->SecondAngle.setValue(cyl->SecondAngle.getMinimum());
        ui->cylinderYSkew->setValue(cyl->SecondAngle.getQuantityValue());
    }
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onSphereAngle1Changed(double v) {
    PartDesign::Sphere* sph = static_cast<PartDesign::Sphere*>(vp->getObject());
    ui->sphereAngle2->setMinimum(v); // Angle1 must geometrically be <= than Angle2
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onSphereAngle2Changed(double v) {
    PartDesign::Sphere* sph = static_cast<PartDesign::Sphere*>(vp->getObject());
    ui->sphereAngle1->setMaximum(v); // Angle1 must geometrically be <= than Angle2
    sph->Angle2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onSphereAngle3Changed(double v) {
    PartDesign::Sphere* sph = static_cast<PartDesign::Sphere*>(vp->getObject());
    sph->Angle3.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onSphereRadiusChanged(double  v) {
    PartDesign::Sphere* sph = static_cast<PartDesign::Sphere*>(vp->getObject());
    sph->Radius.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onConeAngleChanged(double v) {

    PartDesign::Cone* sph = static_cast<PartDesign::Cone*>(vp->getObject());
    sph->Angle.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onConeHeightChanged(double v) {
    PartDesign::Cone* sph = static_cast<PartDesign::Cone*>(vp->getObject());
    sph->Height.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onConeRadius1Changed(double v) {

    PartDesign::Cone* sph = static_cast<PartDesign::Cone*>(vp->getObject());
    sph->Radius1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onConeRadius2Changed(double v) {

    PartDesign::Cone* sph = static_cast<PartDesign::Cone*>(vp->getObject());
    sph->Radius2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidAngle1Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    ui->ellipsoidAngle2->setMinimum(v); // Angle1 must geometrically be <= than Angle2
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidAngle2Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    ui->ellipsoidAngle1->setMaximum(v); // Angle1 must geometrically be <= than Angle22
    sph->Angle2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidAngle3Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    sph->Angle3.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidRadius1Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    sph->Radius1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidRadius2Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    sph->Radius2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidRadius3Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
    sph->Radius3.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusAngle1Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
    ui->torusAngle2->setMinimum(v); // Angle1 must geometrically be <= than Angle2
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusAngle2Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
    ui->torusAngle1->setMaximum(v); // Angle1 must geometrically be <= than Angle2
    sph->Angle2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusAngle3Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
    sph->Angle3.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusRadius1Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
    // this is the outer radius that must not be smaller than the inner one
    // otherwise the geometry is impossible and we can even get a crash:
    // https://forum.freecad.org/viewtopic.php?f=3&t=44467
    ui->torusRadius2->setMaximum(v);
    sph->Radius1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusRadius2Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
    ui->torusRadius1->setMinimum(v);
    sph->Radius2.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onPrismCircumradiusChanged(double v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    sph->Circumradius.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onPrismHeightChanged(double v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    sph->Height.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onPrismXSkewChanged(double v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    // we must assure that if the user incremented from e.g. 85 degree with the
    // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
    if ((v < 90.0) && (v > -90.0)) {
        sph->FirstAngle.setValue(v);
    }
    else {
        if (v == 90.0)
            sph->FirstAngle.setValue(89.99999);
        else if (v == -90.0)
            sph->FirstAngle.setValue(-89.99999);
        ui->prismXSkew->setValue(sph->FirstAngle.getQuantityValue());
    }
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onPrismYSkewChanged(double v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    // we must assure that if the user incremented from e.g. 85 degree with the
    // spin buttons, they do not end at 90.0 but at 89.9999 which is shown rounded to 90 degree
    if ((v < 90.0) && (v > -90.0)) {
        sph->SecondAngle.setValue(v);
    }
    else {
        if (v == 90.0)
            sph->SecondAngle.setValue(89.99999);
        else if (v == -90.0)
            sph->SecondAngle.setValue(-89.99999);
        ui->prismYSkew->setValue(sph->SecondAngle.getQuantityValue());
    }
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onPrismPolygonChanged(int v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    sph->Polygon.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}


void TaskBoxPrimitives::onWedgeX2minChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeX2max->setMinimum(v); // wedgeX2min must be <= than wedgeX2max
    sph->X2min.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeX2maxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeX2min->setMaximum(v); // wedgeX2min must be <= than wedgeX2max
    sph->X2max.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeXminChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeXmax->setMinimum(v);
    sph->Xmin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeXmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeXmin->setMaximum(v); // must be < than wedgeXmax
    sph->Xmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYminChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeYmax->setMinimum(v); // must be > than wedgeYmin
    sph->Ymin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeYmin->setMaximum(v); // must be < than wedgeYmax
    sph->Ymax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2minChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZ2max->setMinimum(v); // must be >= than wedgeZ2min
    sph->Z2min.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2maxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZ2min->setMaximum(v); // must be <= than wedgeZ2max
    sph->Z2max.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZminChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZmax->setMinimum(v); // must be > than wedgeZmin
    sph->Zmin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZmin->setMaximum(v); // must be < than wedgeZmax
    sph->Zmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}


bool TaskBoxPrimitives::setPrimitive(App::DocumentObject *obj)
{
    try {
        QString name(QString::fromLatin1(Gui::Command::getObjectCmd(obj).c_str()));
        QString cmd;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            return false;
        }

        Base::QuantityFormat format(Base::QuantityFormat::Fixed, Base::UnitsApi::getDecimals());
        switch(ui->widgetStack->currentIndex()) {
            case 1:         // box
                cmd = QString::fromLatin1(
                    "%1.Length='%2'\n"
                    "%1.Width='%3'\n"
                    "%1.Height='%4'\n")
                    .arg(name,
                         ui->boxLength->value().getSafeUserString(),
                         ui->boxWidth->value().getSafeUserString(),
                         ui->boxHeight->value().getSafeUserString());
                break;

            case 2:  // cylinder
                cmd = QString::fromLatin1(
                    "%1.Radius='%2'\n"
                    "%1.Height='%3'\n"
                    "%1.Angle='%4'\n"
                    "%1.FirstAngle='%5'\n"
                    "%1.SecondAngle='%6'\n")
                    .arg(name,
                         ui->cylinderRadius->value().getSafeUserString(),
                         ui->cylinderHeight->value().getSafeUserString(),
                         ui->cylinderAngle->value().getSafeUserString(),
                         ui->cylinderXSkew->value().getSafeUserString(),
                         ui->cylinderYSkew->value().getSafeUserString());
                break;

            case 3:  // cone
                // the cone radii must not be equal
                if (ui->coneRadius1->value().getValue() == ui->coneRadius2->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(), tr("Cone radii are equal"),
                        tr("The radii for cones must not be equal!"));
                    return false;
                }
                cmd = QString::fromLatin1(
                    "%1.Radius1='%2'\n"
                    "%1.Radius2='%3'\n"
                    "%1.Height='%4'\n"
                    "%1.Angle='%5'\n")
                    .arg(name,
                         ui->coneRadius1->value().getSafeUserString(),
                         ui->coneRadius2->value().getSafeUserString(),
                         ui->coneHeight->value().getSafeUserString(),
                         ui->coneAngle->value().getSafeUserString());
                 break;

            case 4:  // sphere
                cmd = QString::fromLatin1(
                    "%1.Radius='%2'\n"
                    "%1.Angle1='%3'\n"
                    "%1.Angle2='%4'\n"
                    "%1.Angle3='%5'\n")
                    .arg(name,
                         ui->sphereRadius->value().getSafeUserString(),
                         ui->sphereAngle1->value().getSafeUserString(),
                         ui->sphereAngle2->value().getSafeUserString(),
                         ui->sphereAngle3->value().getSafeUserString());
                break;
            case 5:  // ellipsoid
                cmd = QString::fromLatin1(
                    "%1.Radius1='%2'\n"
                    "%1.Radius2='%3'\n"
                    "%1.Radius3='%4'\n"
                    "%1.Angle1='%5'\n"
                    "%1.Angle2='%6'\n"
                    "%1.Angle3='%7'\n")
                    .arg(name,
                         ui->ellipsoidRadius1->value().getSafeUserString(),
                         ui->ellipsoidRadius2->value().getSafeUserString(),
                         ui->ellipsoidRadius3->value().getSafeUserString(),
                         ui->ellipsoidAngle1->value().getSafeUserString(),
                         ui->ellipsoidAngle2->value().getSafeUserString(),
                         ui->ellipsoidAngle3->value().getSafeUserString());
                break;

            case 6:  // torus
                cmd = QString::fromLatin1(
                    "%1.Radius1='%2'\n"
                    "%1.Radius2='%3'\n"
                    "%1.Angle1='%4'\n"
                    "%1.Angle2='%5'\n"
                    "%1.Angle3='%6'\n")
                    .arg(name,
                         ui->torusRadius1->value().getSafeUserString(),
                         ui->torusRadius2->value().getSafeUserString(),
                         ui->torusAngle1->value().getSafeUserString(),
                         ui->torusAngle2->value().getSafeUserString(),
                         ui->torusAngle3->value().getSafeUserString());
                break;
            case 7:  // prism
                cmd = QString::fromLatin1(
                    "%1.Polygon=%2\n"
                    "%1.Circumradius='%3'\n"
                    "%1.Height='%4'\n"
                    "%1.FirstAngle='%5'\n"
                    "%1.SecondAngle='%6'\n")
                    .arg(name,
                         QString::number(ui->prismPolygon->value()),
                         ui->prismCircumradius->value().getSafeUserString(),
                         ui->prismHeight->value().getSafeUserString(),
                         ui->prismXSkew->value().getSafeUserString(),
                         ui->prismYSkew->value().getSafeUserString());
                break;
            case 8:  // wedge
                // Xmin/max, Ymin/max and Zmin/max must each not be equal
                if (ui->wedgeXmin->value().getValue() == ui->wedgeXmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(), tr("Invalid wedge parameters"),
                        tr("X min must not be equal to X max!"));
                    return false;
                }
                else if (ui->wedgeYmin->value().getValue() == ui->wedgeYmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(), tr("Invalid wedge parameters"),
                        tr("Y min must not be equal to Y max!"));
                    return false;
                }
                else if (ui->wedgeZmin->value().getValue() == ui->wedgeZmax->value().getValue()) {
                    QMessageBox::warning(Gui::getMainWindow(), tr("Invalid wedge parameters"),
                        tr("Z min must not be equal to Z max!"));
                    return false;
                }
                cmd = QString::fromLatin1(
                    "%1.Xmin='%2'\n"
                    "%1.Ymin='%3'\n"
                    "%1.Zmin='%4'\n"
                    "%1.X2min='%5'\n"
                    "%1.Z2min='%6'\n"
                    "%1.Xmax='%7'\n"
                    "%1.Ymax='%8'\n"
                    "%1.Zmax='%9'\n"
                    "%1.X2max='%10'\n"
                    "%1.Z2max='%11'\n")
                    .arg(name,
                         ui->wedgeXmin->value().getSafeUserString(),
                         ui->wedgeYmin->value().getSafeUserString(),
                         ui->wedgeZmin->value().getSafeUserString(),
                         ui->wedgeX2min->value().getSafeUserString(),
                         ui->wedgeZ2min->value().getSafeUserString(),
                         ui->wedgeXmax->value().getSafeUserString(),
                         ui->wedgeYmax->value().getSafeUserString(),
                         ui->wedgeZmax->value().getSafeUserString())
                    .arg(ui->wedgeX2max->value().getSafeUserString(),
                         ui->wedgeZ2max->value().getSafeUserString());
                break;

            default:
                break;
        }

        // Execute the Python block
        // No need to open a transaction because this is already done in the command
        // class or when starting to edit a primitive.
        Gui::Command::runCommand(Gui::Command::Doc, cmd.toUtf8());
        Gui::Command::runCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create primitive"), QApplication::translate("Exception", e.what()));
        return false;
    }
    return true;
}

TaskPrimitiveParameters::TaskPrimitiveParameters(ViewProviderPrimitive* PrimitiveView) : vp_prm(PrimitiveView)
{

    assert(PrimitiveView);

    primitive = new TaskBoxPrimitives(PrimitiveView);
    Content.push_back(primitive);

    /*
    // handle visibility automation differently to the default method
    auto customvisfunc = [] (bool opening_not_closing,
                             const std::string &postfix,
                             Gui::ViewProviderDocumentObject* vp,
                             App::DocumentObject *editObj,
                             const std::string& editSubName) {
        if (opening_not_closing) {
            QString code = QString::fromLatin1(
                "import Show\n"
                "_tv_%4 = Show.TempoVis(App.ActiveDocument, tag= 'PartGui::TaskAttacher')\n"
                "tvObj = %1\n"
                "dep_features = _tv_%4.get_all_dependent(%2, '%3')\n"
                "if tvObj.isDerivedFrom('PartDesign::CoordinateSystem'):\n"
                "\tvisible_features = [feat for feat in tvObj.InList if feat.isDerivedFrom('PartDesign::FeaturePrimitive')]\n"
                "\tdep_features = [feat for feat in dep_features if feat not in visible_features]\n"
                "\tdel(visible_features)\n"
                "_tv_%4.hide(dep_features)\n"
                "del(dep_features)\n"
                "del(tvObj)"
                ).arg(
                    QString::fromLatin1(Gui::Command::getObjectCmd(vp->getObject()).c_str()),
                    QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                    QString::fromLatin1(editSubName.c_str()),
                    QString::fromLatin1(postfix.c_str()));
            Gui::Command::runCommand(Gui::Command::Gui,code.toLatin1().constData());
        } else if(postfix.size()) {
            QString code = QString::fromLatin1(
                "_tv_%1.restore()\n"
                "del(_tv_%1)"
                ).arg(QString::fromLatin1(postfix.c_str()));
            Gui::Command::runCommand(Gui::Command::Gui,code.toLatin1().constData());
        }
    };
    parameter = new PartGui::TaskAttacher(PrimitiveView, nullptr, QString(), tr("Attachment"), customvisfunc);
    */
    parameter = new PartGui::TaskAttacher(PrimitiveView, nullptr, QString(), tr("Attachment"));
    Content.push_back(parameter);
}

TaskPrimitiveParameters::~TaskPrimitiveParameters() = default;

bool TaskPrimitiveParameters::accept()
{
    bool primitiveOK = primitive->setPrimitive(vp_prm->getObject());
    if (!primitiveOK)
        return primitiveOK;
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}

bool TaskPrimitiveParameters::reject()
{
    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}

QDialogButtonBox::StandardButtons TaskPrimitiveParameters::getStandardButtons() const {
    return Gui::TaskView::TaskDialog::getStandardButtons();
}


#include "moc_TaskPrimitiveParameters.cpp"
