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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
# include <boost/bind/bind.hpp>
# include <boost/bind.hpp>
#endif

#include "TaskPrimitiveParameters.h"
#include "ViewProviderDatumCS.h"
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProviderOrigin.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <App/Origin.h>


using namespace PartDesignGui;

TaskBoxPrimitives::TaskBoxPrimitives(ViewProviderPrimitive* vp, QWidget* parent)
  : TaskBox(QPixmap(),tr("Primitive parameters"), true, parent), vp(vp)
{
    proxy = new QWidget(this);
    ui.setupUi(proxy);

    // box
    ui.boxLength->setMaximum(INT_MAX);
    ui.boxWidth->setMaximum(INT_MAX);
    ui.boxHeight->setMaximum(INT_MAX);

    // cylinder
    ui.cylinderRadius->setMaximum(INT_MAX);
    ui.cylinderHeight->setMaximum(INT_MAX);

    // cone
    ui.coneRadius1->setMaximum(INT_MAX);
    ui.coneRadius2->setMaximum(INT_MAX);
    ui.coneHeight->setMaximum(INT_MAX);

    // sphere
    ui.sphereRadius->setMaximum(INT_MAX);

    // ellipsoid
    ui.ellipsoidRadius1->setMaximum(INT_MAX);
    ui.ellipsoidRadius2->setMaximum(INT_MAX);
    ui.ellipsoidRadius3->setMaximum(INT_MAX);

    // torus
    ui.torusRadius1->setMaximum(INT_MAX);
    ui.torusRadius2->setMaximum(INT_MAX);

    // wedge
    ui.wedgeXmin->setMinimum(INT_MIN);
    ui.wedgeXmin->setMaximum(INT_MAX);
    ui.wedgeYmin->setMinimum(INT_MIN);
    ui.wedgeYmin->setMaximum(INT_MAX);
    ui.wedgeZmin->setMinimum(INT_MIN);
    ui.wedgeZmin->setMaximum(INT_MAX);
    ui.wedgeX2min->setMinimum(INT_MIN);
    ui.wedgeX2min->setMaximum(INT_MAX);
    ui.wedgeZ2min->setMinimum(INT_MIN);
    ui.wedgeZ2min->setMaximum(INT_MAX);
    ui.wedgeXmax->setMinimum(INT_MIN);
    ui.wedgeXmax->setMaximum(INT_MAX);
    ui.wedgeYmax->setMinimum(INT_MIN);
    ui.wedgeYmax->setMaximum(INT_MAX);
    ui.wedgeZmax->setMinimum(INT_MIN);
    ui.wedgeZmax->setMaximum(INT_MAX);
    ui.wedgeX2max->setMinimum(INT_MIN);
    ui.wedgeX2max->setMaximum(INT_MAX);
    ui.wedgeZ2max->setMinimum(INT_MIN);
    ui.wedgeZ2max->setMaximum(INT_MAX);

    this->groupLayout()->addWidget(proxy);

    int index = 0;
    switch(static_cast<PartDesign::FeaturePrimitive*>(vp->getObject())->getPrimitiveType()) {

        case PartDesign::FeaturePrimitive::Box:
            index = 1;
            ui.boxLength->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Length.getValue());
            ui.boxHeight->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Height.getValue());
            ui.boxWidth->setValue(static_cast<PartDesign::Box*>(vp->getObject())->Width.getValue());
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            index = 2;
            ui.cylinderAngle->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Angle.getValue());
            ui.cylinderHeight->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Height.getValue());
            ui.cylinderRadius->setValue(static_cast<PartDesign::Cylinder*>(vp->getObject())->Radius.getValue());
            break;
        case PartDesign::FeaturePrimitive::Sphere:
            index = 4;
            ui.sphereAngle1->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle1.getValue());
            ui.sphereAngle2->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle2.getValue());
            ui.sphereAngle3->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Angle3.getValue());
            ui.sphereRadius->setValue(static_cast<PartDesign::Sphere*>(vp->getObject())->Radius.getValue());
            break;
        case PartDesign::FeaturePrimitive::Cone:
            index = 3;
            ui.coneAngle->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Angle.getValue());
            ui.coneHeight->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Height.getValue());
            ui.coneRadius1->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Radius1.getValue());
            ui.coneRadius2->setValue(static_cast<PartDesign::Cone*>(vp->getObject())->Radius2.getValue());
            break;
        case PartDesign::FeaturePrimitive::Ellipsoid:
            index = 5;
            ui.ellipsoidAngle1->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle1.getValue());
            ui.ellipsoidAngle2->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle2.getValue());
            ui.ellipsoidAngle3->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Angle3.getValue());
            ui.ellipsoidRadius1->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius1.getValue());
            ui.ellipsoidRadius2->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius2.getValue());
            ui.ellipsoidRadius3->setValue(static_cast<PartDesign::Ellipsoid*>(vp->getObject())->Radius3.getValue());
            break;
        case PartDesign::FeaturePrimitive::Torus:
            index = 6;
            ui.torusAngle1->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle1.getValue());
            ui.torusAngle2->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle2.getValue());
            ui.torusAngle3->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Angle3.getValue());
            ui.torusRadius1->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Radius1.getValue());
            ui.torusRadius2->setValue(static_cast<PartDesign::Torus*>(vp->getObject())->Radius2.getValue());
            break;
        case PartDesign::FeaturePrimitive::Prism:
            index = 7;
            ui.prismPolygon->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Polygon.getValue());
            ui.prismCircumradius->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Circumradius.getValue());
            ui.prismHeight->setValue(static_cast<PartDesign::Prism*>(vp->getObject())->Height.getValue());
            break;
        case PartDesign::FeaturePrimitive::Wedge:
            index = 8;
            ui.wedgeXmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmax.getValue());
            ui.wedgeXmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Xmin.getValue());
            ui.wedgeX2max->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->X2max.getValue());
            ui.wedgeX2min->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->X2min.getValue());
            ui.wedgeYmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymax.getValue());
            ui.wedgeYmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Ymin.getValue());
            ui.wedgeZmax->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmax.getValue());
            ui.wedgeZmin->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Zmin.getValue());
            ui.wedgeZ2max->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2max.getValue());
            ui.wedgeZ2min->setValue(static_cast<PartDesign::Wedge*>(vp->getObject())->Z2min.getValue());
            break;
    }

    ui.widgetStack->setCurrentIndex(index);
    ui.widgetStack->setMinimumSize(ui.widgetStack->widget(index)->minimumSize());
    for(int i=0; i<ui.widgetStack->count(); ++i) {

        if(i != index)
            ui.widgetStack->widget(i)->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
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
    connect(ui.boxLength, SIGNAL(valueChanged(double)), this, SLOT(onBoxLengthChanged(double)));
    connect(ui.boxWidth, SIGNAL(valueChanged(double)), this, SLOT(onBoxWidthChanged(double)));
    connect(ui.boxHeight, SIGNAL(valueChanged(double)), this, SLOT(onBoxHeightChanged(double)));

    // cylinder
    connect(ui.cylinderRadius, SIGNAL(valueChanged(double)), this, SLOT(onCylinderRadiusChanged(double)));
    connect(ui.cylinderHeight, SIGNAL(valueChanged(double)), this, SLOT(onCylinderHeightChanged(double)));
    connect(ui.cylinderAngle, SIGNAL(valueChanged(double)), this, SLOT(onCylinderAngleChanged(double)));

    // cone
    connect(ui.coneRadius1, SIGNAL(valueChanged(double)), this, SLOT(onConeRadius1Changed(double)));
    connect(ui.coneRadius2, SIGNAL(valueChanged(double)), this, SLOT(onConeRadius2Changed(double)));
    connect(ui.coneAngle, SIGNAL(valueChanged(double)), this, SLOT(onConeAngleChanged(double)));
    connect(ui.coneHeight, SIGNAL(valueChanged(double)), this, SLOT(onConeHeightChanged(double)));

    // sphere
    connect(ui.sphereRadius, SIGNAL(valueChanged(double)), this, SLOT(onSphereRadiusChanged(double)));
    connect(ui.sphereAngle1, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle1Changed(double)));
    connect(ui.sphereAngle2, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle2Changed(double)));
    connect(ui.sphereAngle3, SIGNAL(valueChanged(double)), this, SLOT(onSphereAngle3Changed(double)));

    // ellipsoid
    connect(ui.ellipsoidRadius1, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius1Changed(double)));
    connect(ui.ellipsoidRadius2, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius2Changed(double)));
    connect(ui.ellipsoidRadius3, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidRadius3Changed(double)));
    connect(ui.ellipsoidAngle1, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle1Changed(double)));
    connect(ui.ellipsoidAngle2, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle2Changed(double)));
    connect(ui.ellipsoidAngle3, SIGNAL(valueChanged(double)), this, SLOT(onEllipsoidAngle3Changed(double)));

    // torus
    connect(ui.torusRadius1, SIGNAL(valueChanged(double)), this, SLOT(onTorusRadius1Changed(double)));
    connect(ui.torusRadius2, SIGNAL(valueChanged(double)), this, SLOT(onTorusRadius2Changed(double)));
    connect(ui.torusAngle1, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle1Changed(double)));
    connect(ui.torusAngle2, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle2Changed(double)));
    connect(ui.torusAngle3, SIGNAL(valueChanged(double)), this, SLOT(onTorusAngle3Changed(double)));

    //prism
    connect(ui.prismCircumradius, SIGNAL(valueChanged(double)), this, SLOT(onPrismCircumradiusChanged(double)));
    connect(ui.prismHeight, SIGNAL(valueChanged(double)), this, SLOT(onPrismHeightChanged(double)));
    connect(ui.prismPolygon, SIGNAL(valueChanged(int)), this, SLOT(onPrismPolygonChanged(int)));

    // wedge
    connect(ui.wedgeXmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeXmaxChanged(double)));
    connect(ui.wedgeXmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeXinChanged(double)));
    connect(ui.wedgeYmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeYmaxChanged(double)));
    connect(ui.wedgeYmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeYinChanged(double)));
    connect(ui.wedgeZmax, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZmaxChanged(double)));
    connect(ui.wedgeZmin, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZinChanged(double)));
    connect(ui.wedgeX2max, SIGNAL(valueChanged(double)), this, SLOT(onWedgeX2maxChanged(double)));
    connect(ui.wedgeX2min, SIGNAL(valueChanged(double)), this, SLOT(onWedgeX2inChanged(double)));
    connect(ui.wedgeZ2max, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZ2maxChanged(double)));
    connect(ui.wedgeZ2min, SIGNAL(valueChanged(double)), this, SLOT(onWedgeZ2inChanged(double)));
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
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onSphereAngle2Changed(double v) {
    PartDesign::Sphere* sph = static_cast<PartDesign::Sphere*>(vp->getObject());
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
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onEllipsoidAngle2Changed(double v) {
    PartDesign::Ellipsoid* sph = static_cast<PartDesign::Ellipsoid*>(vp->getObject());
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
    sph->Angle1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusAngle2Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
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
    sph->Radius1.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onTorusRadius2Changed(double v) {
    PartDesign::Torus* sph = static_cast<PartDesign::Torus*>(vp->getObject());
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

void TaskBoxPrimitives::onPrismPolygonChanged(int v) {
    PartDesign::Prism* sph = static_cast<PartDesign::Prism*>(vp->getObject());
    sph->Polygon.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}


void TaskBoxPrimitives::onWedgeX2inChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->X2min.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeX2maxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->X2max.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeXinChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Xmin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeXmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Xmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYinChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Ymin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeYmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Ymax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2inChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Z2min.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZ2maxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Z2max.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZinChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Zmin.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}

void TaskBoxPrimitives::onWedgeZmaxChanged(double v) {
    PartDesign::Wedge* sph = static_cast<PartDesign::Wedge*>(vp->getObject());
    sph->Zmax.setValue(v);
    vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
}




void  TaskBoxPrimitives::setPrimitive(QString name)
{
    try {
        QString cmd;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            return;
        }
        switch(ui.widgetStack->currentIndex()) {
            case 1:         // box
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Length=%2\n"
                    "App.ActiveDocument.%1.Width=%3\n"
                    "App.ActiveDocument.%1.Height=%4\n")
                    .arg(name)
                    .arg(ui.boxLength->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.boxWidth->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.boxHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 2:  // cylinder
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Radius=%2\n"
                    "App.ActiveDocument.%1.Height=%3\n"
                    "App.ActiveDocument.%1.Angle=%4\n")
                    .arg(name)
                    .arg(ui.cylinderRadius->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.cylinderHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.cylinderAngle->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 3:  // cone
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Radius1=%2\n"
                    "App.ActiveDocument.%1.Radius2=%3\n"
                    "App.ActiveDocument.%1.Height=%4\n"
                    "App.ActiveDocument.%1.Angle=%5\n")
                    .arg(name)
                    .arg(ui.coneRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.coneRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.coneHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.coneAngle->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                 break;

            case 4:  // sphere
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Radius=%2\n"
                    "App.ActiveDocument.%1.Angle1=%3\n"
                    "App.ActiveDocument.%1.Angle2=%4\n"
                    "App.ActiveDocument.%1.Angle3=%5\n")
                    .arg(name)
                    .arg(ui.sphereRadius->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.sphereAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.sphereAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.sphereAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;
            case 5:  // ellipsoid
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Radius1=%2\n"
                    "App.ActiveDocument.%1.Radius2=%3\n"
                    "App.ActiveDocument.%1.Radius3=%4\n"
                    "App.ActiveDocument.%1.Angle1=%5\n"
                    "App.ActiveDocument.%1.Angle2=%6\n"
                    "App.ActiveDocument.%1.Angle3=%7\n")
                    .arg(name)
                    .arg(ui.ellipsoidRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.ellipsoidRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.ellipsoidRadius3->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.ellipsoidAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.ellipsoidAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.ellipsoidAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;

            case 6:  // torus
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Radius1=%2\n"
                    "App.ActiveDocument.%1.Radius2=%3\n"
                    "App.ActiveDocument.%1.Angle1=%4\n"
                    "App.ActiveDocument.%1.Angle2=%5\n"
                    "App.ActiveDocument.%1.Angle3=%6\n")
                    .arg(name)
                    .arg(ui.torusRadius1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.torusRadius2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.torusAngle1->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.torusAngle2->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.torusAngle3->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;
            case 7:  // prism
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Polygon=%2\n"
                    "App.ActiveDocument.%1.Circumradius=%3\n"
                    "App.ActiveDocument.%1.Height=%4\n")
                    .arg(name)
                    .arg(ui.prismPolygon->value())
                    .arg(ui.prismCircumradius->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.prismHeight->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
                break;
            case 8:  // wedge
                cmd = QString::fromLatin1(
                    "App.ActiveDocument.%1.Xmin=%2\n"
                    "App.ActiveDocument.%1.Ymin=%3\n"
                    "App.ActiveDocument.%1.Zmin=%4\n"
                    "App.ActiveDocument.%1.X2min=%5\n"
                    "App.ActiveDocument.%1.Z2min=%6\n"
                    "App.ActiveDocument.%1.Xmax=%7\n"
                    "App.ActiveDocument.%1.Ymax=%8\n"
                    "App.ActiveDocument.%1.Zmax=%9\n"
                    "App.ActiveDocument.%1.X2max=%10\n"
                    "App.ActiveDocument.%1.Z2max=%11\n")
                    .arg(name)
                    .arg(ui.wedgeXmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeYmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeZmin->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeX2min->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeZ2min->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeXmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeYmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeZmax->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeX2max->value().getValue(),0,'f',Base::UnitsApi::getDecimals())
                    .arg(ui.wedgeZ2max->value().getValue(),0,'f',Base::UnitsApi::getDecimals());
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

    parameter  = new PartGui::TaskAttacher(PrimitiveView);
    Content.push_back(parameter);
}

TaskPrimitiveParameters::~TaskPrimitiveParameters()
{

}

bool TaskPrimitiveParameters::accept()
{
    primitive->setPrimitive(QString::fromUtf8(vp_prm->getObject()->getNameInDocument()));
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
