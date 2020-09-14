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
# include <Precision.hxx>
# include <QMessageBox>
# include <QRegExp>
# include <QTextStream>
# include <sstream>
#endif

#include "TaskPrimitiveParameters.h"
#include "ui_TaskPrimitiveParameters.h"
#include "ViewProviderDatumCS.h"

#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>

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
            ui->boxLength->setMinimum(0.0);
            ui->boxLength->setMaximum(INT_MAX);
            ui->boxWidth->setMinimum(0.0);
            ui->boxWidth->setMaximum(INT_MAX);
            ui->boxHeight->setMinimum(0.0);
            ui->boxHeight->setMaximum(INT_MAX);
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            index = 2;
            ui->cylinderAngle->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle.getValue());
            ui->cylinderAngle->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle);
            ui->cylinderHeight->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height.getValue());
            ui->cylinderHeight->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height);
            ui->cylinderRadius->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius.getValue());
            ui->cylinderRadius->bind(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius);
            ui->cylinderAngle->setMaximum(360.0);
            ui->cylinderAngle->setMinimum(0.0);
            ui->cylinderHeight->setMaximum(INT_MAX);
            ui->cylinderHeight->setMinimum(0.0);
            ui->cylinderRadius->setMaximum(INT_MAX);
            ui->cylinderRadius->setMinimum(0.0);
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
            ui->sphereAngle1->setMinimum(-90.0);
            ui->sphereAngle2->setMaximum(90);
            ui->sphereAngle2->setMinimum(ui->sphereAngle1->rawValue());
            ui->sphereAngle3->setMaximum(360.0);
            ui->sphereAngle3->setMinimum(0.0);
            ui->sphereRadius->setMaximum(INT_MAX);
            ui->sphereRadius->setMinimum(0.0);
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
            ui->coneAngle->setMaximum(360.0);
            ui->coneAngle->setMinimum(0.0);
            ui->coneHeight->setMaximum(INT_MAX);
            ui->coneHeight->setMinimum(0.0);
            ui->coneRadius1->setMaximum(INT_MAX);
            ui->coneRadius1->setMinimum(0.0);
            ui->coneRadius2->setMaximum(INT_MAX);
            ui->coneRadius2->setMinimum(0.0);
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
            ui->ellipsoidAngle1->setMinimum(-90.0);
            ui->ellipsoidAngle2->setMaximum(90);
            ui->ellipsoidAngle2->setMinimum(ui->ellipsoidAngle1->rawValue());
            ui->ellipsoidAngle3->setMaximum(360.0);
            ui->ellipsoidAngle3->setMinimum(0.0);
            ui->ellipsoidRadius1->setMinimum(0.0);
            ui->ellipsoidRadius1->setMaximum(INT_MAX);
            ui->ellipsoidRadius2->setMinimum(0.0);
            ui->ellipsoidRadius2->setMaximum(INT_MAX);
            ui->ellipsoidRadius3->setMinimum(0.0);
            ui->ellipsoidRadius3->setMaximum(INT_MAX);
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
            ui->torusAngle1->setMinimum(-180.0);
            ui->torusAngle2->setMaximum(180);
            ui->torusAngle2->setMinimum(ui->torusAngle1->rawValue());
            ui->torusAngle3->setMaximum(360.0);
            ui->torusAngle3->setMinimum(0.0);
            // this is the outer radius that must not be smaller than the inner one
            // otherwise the geometry is impossible and we can even get a crash:
            // https://forum.freecadweb.org/viewtopic.php?f=3&t=44467
            ui->torusRadius1->setMaximum(INT_MAX);
            ui->torusRadius1->setMinimum(ui->torusRadius2->rawValue());
            ui->torusRadius2->setMaximum(ui->torusRadius1->rawValue());
            ui->torusRadius2->setMinimum(0.0);
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
            ui->prismCircumradius->setMaximum(INT_MAX);
            ui->prismCircumradius->setMinimum(0.0);
            ui->prismHeight->setMaximum(INT_MAX);
            ui->prismHeight->setMinimum(0.0);
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
            ui->wedgeXmin->setMaximum(ui->wedgeXmax->rawValue()); // must be <= than wedgeXmax
            ui->wedgeYmin->setMinimum(INT_MIN);
            ui->wedgeYmin->setMaximum(ui->wedgeYmax->rawValue()); // must be <= than wedgeYmax
            ui->wedgeZmin->setMinimum(INT_MIN);
            ui->wedgeZmin->setMaximum(ui->wedgeZmax->rawValue()); // must be <= than wedgeZmax
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
    connect(ui->boxLength, SIGNAL(valueChanged(double)), this, SLOT(onBoxLengthChanged(double)));
    connect(ui->boxWidth, SIGNAL(valueChanged(double)), this, SLOT(onBoxWidthChanged(double)));
    connect(ui->boxHeight, SIGNAL(valueChanged(double)), this, SLOT(onBoxHeightChanged(double)));

    // cylinder
    connect(ui->cylinderRadius, SIGNAL(valueChanged(double)), this, SLOT(onCylinderRadiusChanged(double)));
    connect(ui->cylinderHeight, SIGNAL(valueChanged(double)), this, SLOT(onCylinderHeightChanged(double)));
    connect(ui->cylinderAngle, SIGNAL(valueChanged(double)), this, SLOT(onCylinderAngleChanged(double)));

    // cone
    connect(ui->coneRadius1, SIGNAL(valueChanged(double)), this, SLOT(onConeRadius1Changed(double)));
    connect(ui->coneRadius2, SIGNAL(valueChanged(double)), this, SLOT(onConeRadius2Changed(double)));
    connect(ui->coneAngle, SIGNAL(valueChanged(double)), this, SLOT(onConeAngleChanged(double)));
    connect(ui->coneHeight, SIGNAL(valueChanged(double)), this, SLOT(onConeHeightChanged(double)));

    // sphere
    connect(ui->sphereRadius, SIGNAL(valueChanged(double)), this, SLOT(onSphereRadiusChanged(double)));
    connect(ui->sphereAngle1, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle1Changed(double)));
    connect(ui->sphereAngle2, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle2Changed(double)));
    connect(ui->sphereAngle3, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle3Changed(double)));

    // ellipsoid
    connect(ui->ellipsoidRadius1, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius1Changed(double)));
    connect(ui->ellipsoidRadius2, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius2Changed(double)));
    connect(ui->ellipsoidRadius3, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius3Changed(double)));
    connect(ui->ellipsoidAngle1, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle1Changed(double)));
    connect(ui->ellipsoidAngle2, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle2Changed(double)));
    connect(ui->ellipsoidAngle3, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle3Changed(double)));

    // torus
    connect(ui->torusRadius1, SIGNAL(valueChanged(double)), this, SLOT(onTorusRadius1Changed(double)));
    connect(ui->torusRadius2, SIGNAL(valueChanged(double)), this, SLOT(onTorusRadius2Changed(double)));
    connect(ui->torusAngle1, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle1Changed(double)));
    connect(ui->torusAngle2, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle2Changed(double)));
    connect(ui->torusAngle3, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle3Changed(double)));

    //prism
    connect(ui->prismCircumradius, SIGNAL(valueChanged(double)), this, SLOT(onPrismCircumradiusChanged(double)));
    connect(ui->prismHeight, SIGNAL(valueChanged(double)), this, SLOT(onPrismHeightChanged(double)));
    connect(ui->prismXSkew, SIGNAL(valueChanged(double)), this, SLOT(onPrismXSkewChanged(double)));
    connect(ui->prismYSkew, SIGNAL(valueChanged(double)), this, SLOT(onPrismYSkewChanged(double)));
    connect(ui->prismPolygon, SIGNAL(valueChanged(int)), this, SLOT(onPrismPolygonChanged(int)));

    // wedge
    connect(ui->wedgeXmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeXmaxChanged(double)));
    connect(ui->wedgeXmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeXminChanged(double)));
    connect(ui->wedgeYmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeYmaxChanged(double)));
    connect(ui->wedgeYmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeYminChanged(double)));
    connect(ui->wedgeZmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZmaxChanged(double)));
    connect(ui->wedgeZmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZminChanged(double)));
    connect(ui->wedgeX2max, SIGNAL(valueChanged(double)), this, SLOT(onWedgeX2maxChanged(double)));
    connect(ui->wedgeX2min, SIGNAL(valueChanged(double)), this, SLOT(onWedgeX2minChanged(double)));
    connect(ui->wedgeZ2max, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZ2maxChanged(double)));
    connect(ui->wedgeZ2min, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZ2minChanged(double)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
TaskBoxPrimitives::~TaskBoxPrimitives()
{
    //hide the parts coordinate system axis for selection
    try {
        PartDesign::Body * body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : 0;
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
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=44467
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
    // spin buttons he does not end at 90.0 but 89.9999 which is shown rounded to 90 degree
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
    // spin buttons he does not end at 90.0 but 89.9999 which is shown rounded to 90 degree
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
    ui->wedgeXmin->setMaximum(v); // must be <= than wedgeXmax
    sph->Xmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYminChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeYmax->setMinimum(v);
    sph->Ymin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeYmin->setMaximum(v);
    sph->Ymax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2minChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZ2max->setMinimum(v);
    sph->Z2min.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2maxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZ2min->setMaximum(v); // must be <= than wedgeXmax
    sph->Z2max.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZminChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZmax->setMinimum(v);
    sph->Zmin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    ui->wedgeZmin->setMaximum(v);
    sph->Zmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}




void  TaskBoxPrimitives::setPrimitive(App::DocumentObject *obj)
{
    try {
        QString name(QString::fromLatin1(Gui::Command::getObjectCmd(obj).c_str()));
        QString cmd;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            return;
        }
        switch(ui->widgetStack->currentIndex()) {
            case 1:         // box
                cmd = QString::fromLatin1(
                    "%1.Length=%2\n"
                    "%1.Width=%3\n"
                    "%1.Height=%4\n")
                    .arg(name)
                    .arg(ui->boxLength->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->boxWidth->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->boxHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 2:  // cylinder
                cmd = QString::fromLatin1(
                    "%1.Radius=%2\n"
                    "%1.Height=%3\n"
                    "%1.Angle=%4\n")
                    .arg(name)
                    .arg(ui->cylinderRadius->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->cylinderHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->cylinderAngle->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 3:  // cone
                cmd = QString::fromLatin1(
                    "%1.Radius1=%2\n"
                    "%1.Radius2=%3\n"
                    "%1.Height=%4\n"
                    "%1.Angle=%5\n")
                    .arg(name)
                    .arg(ui->coneRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->coneRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->coneHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->coneAngle->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                 break;

            case 4:  // sphere
                cmd = QString::fromLatin1(
                    "%1.Radius=%2\n"
                    "%1.Angle1=%3\n"
                    "%1.Angle2=%4\n"
                    "%1.Angle3=%5\n")
                    .arg(name)
                    .arg(ui->sphereRadius->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->sphereAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->sphereAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->sphereAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;
            case 5:  // ellipsoid
                cmd = QString::fromLatin1(
                    "%1.Radius1=%2\n"
                    "%1.Radius2=%3\n"
                    "%1.Radius3=%4\n"
                    "%1.Angle1=%5\n"
                    "%1.Angle2=%6\n"
                    "%1.Angle3=%7\n")
                    .arg(name)
                    .arg(ui->ellipsoidRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->ellipsoidRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->ellipsoidRadius3->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->ellipsoidAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->ellipsoidAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->ellipsoidAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 6:  // torus
                cmd = QString::fromLatin1(
                    "%1.Radius1=%2\n"
                    "%1.Radius2=%3\n"
                    "%1.Angle1=%4\n"
                    "%1.Angle2=%5\n"
                    "%1.Angle3=%6\n")
                    .arg(name)
                    .arg(ui->torusRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->torusRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->torusAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->torusAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->torusAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;
            case 7:  // prism
                cmd = QString::fromLatin1(
                    "%1.Polygon=%2\n"
                    "%1.Circumradius=%3\n"
                    "%1.Height=%4\n"
                    "%1.FirstAngle=%5\n"
                    "%1.SecondAngle=%6\n")
                    .arg(name)
                    .arg(ui->prismPolygon->value())
                    .arg(ui->prismCircumradius->value().getValue(), 0, 'f', Base::UnitsApi::getDecimals())
                    .arg(ui->prismHeight->value().getValue(), 0, 'f', Base::UnitsApi::getDecimals())
                    .arg(ui->prismXSkew->value().getValue(), 0, 'f', Base::UnitsApi::getDecimals())
                    .arg(ui->prismYSkew->value().getValue(), 0, 'f', Base::UnitsApi::getDecimals());
                break;
            case 8:  // wedge
                cmd = QString::fromLatin1(
                    "%1.Xmin=%2\n"
                    "%1.Ymin=%3\n"
                    "%1.Zmin=%4\n"
                    "%1.X2min=%5\n"
                    "%1.Z2min=%6\n"
                    "%1.Xmax=%7\n"
                    "%1.Ymax=%8\n"
                    "%1.Zmax=%9\n"
                    "%1.X2max=%10\n"
                    "%1.Z2max=%11\n")
                    .arg(name)
                    .arg(ui->wedgeXmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeYmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeZmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeX2min->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeZ2min->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeXmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeYmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeZmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeX2max->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui->wedgeZ2max->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
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
        QMessageBox::warning(this, tr("Create primitive"), QString::fromLatin1(e.what()));
    }
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

TaskPrimitiveParameters::~TaskPrimitiveParameters()
{

}

bool TaskPrimitiveParameters::accept()
{
    primitive->setPrimitive(vp_prm->getObject());
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

QDialogButtonBox::StandardButtons TaskPrimitiveParameters::getStandardButtons(void) const {
    return Gui::TaskView::TaskDialog::getStandardButtons();
}


#include "moc_TaskPrimitiveParameters.cpp"
