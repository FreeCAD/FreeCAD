/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <cassert>
#include <numbers>

#include <Inventor/SbRotation.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoFontStyle.h>
#endif

#include <Base/Quantity.h>

#include "SoRotationDragger.h"

#include "MainWindow.h"
#include "SoFCDB.h"

#include <SoTextLabel.h>

using namespace Gui;

SO_KIT_SOURCE(SoRotationDragger)

void SoRotationDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoRotationDragger, SoDragger, "Dragger");
}

SoRotationDragger::SoRotationDragger()
{
    SO_KIT_CONSTRUCTOR(SoRotationDragger);
#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    SO_KIT_ADD_CATALOG_ENTRY(rotatorSwitch, SoSwitch, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(rotator, SoSeparator, TRUE, rotatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(rotatorActive, SoSeparator, TRUE, rotatorSwitch, "", TRUE);

    arcRadius = 8.0;

    if (SO_KIT_IS_FIRST_INSTANCE()) {
        buildFirstInstance();
    }

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (std::numbers::pi / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCount, (0));

    SO_KIT_INIT_INSTANCE();

    // initialize default parts.
    // first is from 'SO_KIT_CATALOG_ENTRY_HEADER' macro
    // second is unique name from buildFirstInstance().
    this->setPartAsDefault("rotator", "CSysDynamics_RDragger_Rotator");
    this->setPartAsDefault("rotatorActive", "CSysDynamics_RDragger_RotatorActive");

    SoSwitch* sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);

    this->addStartCallback(&SoRotationDragger::startCB);
    this->addMotionCallback(&SoRotationDragger::motionCB);
    this->addFinishCallback(&SoRotationDragger::finishCB);

    addValueChangedCallback(&SoRotationDragger::valueChangedCB);

    fieldSensor.setFunction(&SoRotationDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}

SoRotationDragger::~SoRotationDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&SoRotationDragger::startCB);
    this->removeMotionCallback(&SoRotationDragger::motionCB);
    this->removeFinishCallback(&SoRotationDragger::finishCB);
    removeValueChangedCallback(&SoRotationDragger::valueChangedCB);
}

void SoRotationDragger::buildFirstInstance()
{
    SoGroup* geometryGroup = buildGeometry();

    auto localRotator = new SoSeparator();
    localRotator->setName("CSysDynamics_RDragger_Rotator");
    localRotator->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localRotator);

    auto localRotatorActive = new SoSeparator();
    localRotatorActive->setName("CSysDynamics_RDragger_RotatorActive");
    auto colorActive = new SoBaseColor();
    colorActive->rgb.setValue(1.0, 1.0, 0.0);
    localRotatorActive->addChild(colorActive);
    localRotatorActive->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localRotatorActive);
}

SoGroup* SoRotationDragger::buildGeometry()
{
    auto root = new SoGroup();

    // arc
    auto coordinates = new SoCoordinate3();

    unsigned int segments = 15;

    float angleIncrement = (std::numbers::pi_v<float> / 2.f) / static_cast<float>(segments);
    SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), angleIncrement);
    SbVec3f point(arcRadius, 0.0, 0.0);
    for (unsigned int index = 0; index <= segments; ++index) {
        coordinates->point.set1Value(index, point);
        rotation.multVec(point, point);
    }
    root->addChild(coordinates);

    auto drawStyle = new SoDrawStyle();
    drawStyle->lineWidth = 4.0;
    root->addChild(drawStyle);

    auto lightModel = new SoLightModel();
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    auto lineSet = new SoLineSet();
    lineSet->numVertices.setValue(segments + 1);
    root->addChild(lineSet);

    auto pickStyle = new SoPickStyle();
    pickStyle->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    pickStyle->setOverride(TRUE);
    root->addChild(pickStyle);

    // sphere.
    SbVec3f origin(1.0, 1.0, 0.0);
    origin.normalize();
    origin *= arcRadius;
    auto sphereTranslation = new SoTranslation();
    sphereTranslation->translation.setValue(origin);
    root->addChild(sphereTranslation);

    auto sphere = new SoSphere();
    sphere->radius.setValue(0.8F);
    root->addChild(sphere);

    return root;
}

void SoRotationDragger::startCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoRotationDragger*>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void SoRotationDragger::motionCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoRotationDragger*>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void SoRotationDragger::finishCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoRotationDragger*>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void SoRotationDragger::fieldSensorCB(void* f, SoSensor*)
{
    auto sudoThis = static_cast<SoRotationDragger*>(f);

    if (!f) {
        return;
    }

    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoRotationDragger::valueChangedCB(void*, SoDragger* d)
{
    auto sudoThis = dynamic_cast<SoRotationDragger*>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft

    // all this just to get the translation?
    SbVec3f translationDummy, scaleDummy;
    SbRotation localRotation, scaleOrientationDummy;
    matrix.getTransform(translationDummy, localRotation, scaleDummy, scaleOrientationDummy);

    sudoThis->fieldSensor.detach();
    if (sudoThis->rotation.getValue() != localRotation) {
        sudoThis->rotation = localRotation;
    }
    sudoThis->fieldSensor.attach(&sudoThis->rotation);
}

void SoRotationDragger::dragStart()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);

    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());
    projector.setPlane(SbPlane(SbVec3f(0.0, 0.0, 1.0), 0.0));

    SbVec3f hitPoint;
    if (!projector.tryProject(getNormalizedLocaterPosition(), 0.0, hitPoint)) {
        return;
    }
    hitPoint.normalize();

    SbMatrix localToWorld = getLocalToWorldMatrix();
    localToWorld.multVecMatrix(hitPoint, hitPoint);
    setStartingPoint((hitPoint));

    rotationIncrementCount.setValue(0);
}

void SoRotationDragger::drag()
{
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());

    SbVec3f hitPoint;
    if (!projector.tryProject(getNormalizedLocaterPosition(), 0.0, hitPoint)) {
        return;
    }
    hitPoint.normalize();

    SbVec3f startingPoint = getLocalStartingPoint();
    startingPoint.normalize();

    SbRotation localRotation(startingPoint, hitPoint);
    // getting some slop from this. grab vector and put it absolute.
    SbVec3f tempVec;
    float tempRadians;
    localRotation.getValue(tempVec, tempRadians);
    tempVec[0] = 0.0;
    tempVec[1] = 0.0;
    tempVec.normalize();
    if (tempVec[2] < 0.0) {
        tempRadians *= -1.0;
        tempVec.negate();
    }
    int incrementCount = roundIncrement(tempRadians);
    rotationIncrementCount.setValue(incrementCount);
    localRotation =
        SbRotation(tempVec, incrementCount * static_cast<float>(rotationIncrement.getValue()));

    // same problem as described in tDragger::drag.
    if (localRotation.equals(SbRotation(SbVec3f(0.0, 0.0, 1.0), 0.0), 0.00001f)) {
        setMotionMatrix(getStartMotionMatrix());
        this->valueChanged();
    }
    else {
        setMotionMatrix(
            appendRotation(getStartMotionMatrix(), localRotation, SbVec3f(0.0, 0.0, 0.0)));
    }

    Base::Quantity quantity(
        static_cast<double>(rotationIncrementCount.getValue())
            * (180.0 / std::numbers::pi)* rotationIncrement.getValue(),
        Base::Unit::Angle);

    QString message =
        QStringLiteral("%1 %2").arg(QObject::tr("Rotation:"), QString::fromStdString(quantity.getUserString()));
    getMainWindow()->showMessage(message, 3000);
}

void SoRotationDragger::dragFinish()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);
}

SbBool SoRotationDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if (!doitalways && this->connectionsSetUp == onoff) {
        return onoff;
    }

    SbBool oldval = this->connectionsSetUp;

    if (onoff) {
        inherited::setUpConnections(onoff, doitalways);
        SoRotationDragger::fieldSensorCB(this, nullptr);
        if (this->fieldSensor.getAttachedField() != &this->rotation) {
            this->fieldSensor.attach(&this->rotation);
        }
    }
    else {
        if (this->fieldSensor.getAttachedField()) {
            this->fieldSensor.detach();
        }
        inherited::setUpConnections(onoff, doitalways);
    }
    this->connectionsSetUp = onoff;
    return oldval;
}

int SoRotationDragger::roundIncrement(const float& radiansIn)
{
    int rCount = 0;

    auto increment = static_cast<float>(rotationIncrement.getValue());
    if (fabs(radiansIn) > (increment / 2.0)) {
        rCount = static_cast<int>(radiansIn / increment);
        float remainder = fmod(radiansIn, increment);
        if (remainder >= (increment / 2.0)) {
            rCount++;
        }
    }

    return rCount;
}
