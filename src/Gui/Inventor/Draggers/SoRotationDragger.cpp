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
#include <Inventor/engines/SoCalculator.h>
#endif

#include <Base/Quantity.h>

#include "SoRotationDragger.h"

#include "MainWindow.h"
#include "Utilities.h"

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

    FC_ADD_CATALOG_ENTRY(activeSwitch, SoSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(secondaryColor, SoBaseColor, activeSwitch);
    FC_ADD_CATALOG_ENTRY(rotator, SoSeparator, geomSeparator);

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (std::numbers::pi / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCount, (0));
    SO_KIT_ADD_FIELD(activeColor, (1, 1, 0));
    SO_KIT_ADD_FIELD(arcRadius, (8.0));
    SO_KIT_ADD_FIELD(arcAngle, (std::numbers::pi_v<float> / 2.0f));
    SO_KIT_ADD_FIELD(sphereRadius, (0.8));
    SO_KIT_ADD_FIELD(arcThickness, (4.0));

    SO_KIT_INIT_INSTANCE();

    setupGeometryCalculator();
    setPart("secondaryColor", buildActiveColor());
    setPart("rotator", buildGeometry());

    FC_SET_SWITCH("activeSwitch", SO_SWITCH_NONE);

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

    arcRadiusSensor.setData(nullptr);
    arcRadiusSensor.detach();
    arcAngleSensor.setData(nullptr);
    arcAngleSensor.detach();

    this->removeStartCallback(&SoRotationDragger::startCB);
    this->removeMotionCallback(&SoRotationDragger::motionCB);
    this->removeFinishCallback(&SoRotationDragger::finishCB);
    removeValueChangedCallback(&SoRotationDragger::valueChangedCB);
}

SoSeparator* SoRotationDragger::buildGeometry()
{
    auto root = new SoSeparator();

    auto coordinates = new SoCoordinate3();
    this->coordinates = coordinates;
    setupArcSensors();
    root->addChild(coordinates);

    auto drawStyle = new SoDrawStyle();
    drawStyle->lineWidth.connectFrom(&arcThickness);
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
    auto sphereTranslation = new SoTranslation();
    sphereTranslation->translation.connectFrom(&calculator->oA);
    root->addChild(sphereTranslation);

    auto sphere = new SoSphere();
    sphere->radius.connectFrom(&sphereRadius);
    root->addChild(sphere);

    return root;
}

void SoRotationDragger::arcSensorCB(void* userdata, SoSensor*)
{
    auto sudoThis = static_cast<SoRotationDragger*>(userdata);
    assert(sudoThis->coordinates);
    auto coordinates = sudoThis->coordinates;
    float angleIncrement = sudoThis->arcAngle.getValue() / static_cast<float>(segments);
    SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), angleIncrement);
    SbVec3f point(sudoThis->arcRadius.getValue(), 0.0, 0.0);
    for (unsigned int index = 0; index <= segments; ++index) {
        coordinates->point.set1Value(index, point);
        rotation.multVec(point, point);
    }
}

SoBaseColor* SoRotationDragger::buildActiveColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&activeColor);

    return color;
}

void SoRotationDragger::setupGeometryCalculator()
{
    calculator = new SoCalculator;
    // Recalculate the corresponding variables in the left hand side whenever any of the variables in the right hand side change
    // oA -> sphereTranslation
    // a  -> arcRadius
    // b  -> arcAngle
    calculator->expression = "oA = vec3f(a * cos(b / 2), a * sin(b / 2), 0)";

    calculator->a.connectFrom(&arcRadius);
    calculator->b.connectFrom(&arcAngle);
}

void SoRotationDragger::setupArcSensors()
{
    arcRadiusSensor.setFunction(arcSensorCB);
    arcRadiusSensor.setData(this);
    arcRadiusSensor.setPriority(0);
    arcRadiusSensor.attach(&arcRadius);
    arcRadius.touch();

    arcAngleSensor.setFunction(arcSensorCB);
    arcAngleSensor.setData(this);
    arcAngleSensor.setPriority(0);
    arcAngleSensor.attach(&arcAngle);
    arcAngle.touch();
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

    SbRotation localRotation = getMatrixTransform(matrix).rotation;

    sudoThis->fieldSensor.detach();
    if (sudoThis->rotation.getValue() != localRotation) {
        sudoThis->rotation = localRotation;
    }
    sudoThis->fieldSensor.attach(&sudoThis->rotation);
}

void SoRotationDragger::dragStart()
{
    FC_SET_SWITCH("activeSwitch", SO_SWITCH_ALL);

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
    FC_SET_SWITCH("activeSwitch", SO_SWITCH_NONE);
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

SO_KIT_SOURCE(SoRotationDraggerContainer)

void SoRotationDraggerContainer::initClass()
{
    SoRotationDragger::initClass();
    SO_KIT_INIT_CLASS(SoRotationDraggerContainer, SoInteractionKit, "InteractionKit");
}

SoRotationDraggerContainer::SoRotationDraggerContainer()
{
    SO_KIT_CONSTRUCTOR(SoRotationDraggerContainer);

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    FC_ADD_CATALOG_ENTRY(draggerSwitch, SoSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(transform, SoTransform, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(dragger, SoRotationDragger, draggerSwitch);

    SO_KIT_ADD_FIELD(rotation, (0, 0, 0, 0));
    SO_KIT_ADD_FIELD(color, (0, 0, 0));
    SO_KIT_ADD_FIELD(translation, (0, 0, 0));

    SO_KIT_INIT_INSTANCE();

    setPart("baseColor", buildColor());
    setPart("transform", buildTransform());

    setVisibility(true);
}

SoBaseColor* SoRotationDraggerContainer::buildColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&this->color);

    return color;
}

SoTransform* SoRotationDraggerContainer::buildTransform() {
    auto transform = new SoTransform;
    transform->translation.connectFrom(&this->translation);
    transform->rotation.connectFrom(&this->rotation);

    return transform;
}

void SoRotationDraggerContainer::setVisibility(bool visible)
{
    FC_SET_SWITCH("draggerSwitch", visible? SO_SWITCH_ALL : SO_SWITCH_NONE);
}

bool SoRotationDraggerContainer::isVisible() {
    auto* sw = SO_GET_ANY_PART(this, "draggerSwitch", SoSwitch);
    return sw->whichChild.getValue() == SO_SWITCH_ALL;
}

SoRotationDragger* SoRotationDraggerContainer::getDragger()
{
    return SO_GET_PART(this, "dragger", SoRotationDragger);
}

void Gui::SoRotationDraggerContainer::setPointerDirection(const Base::Vector3d& dir)
{
    // This is the direction of normal from the origin to the spherical handle of
    // the dragger in its local space
    float arcAngle = getDragger()->arcAngle.getValue();
    Base::Vector3d draggerDir{std::cos(arcAngle), std::sin(arcAngle), 0};

    Base::Vector3d axis = draggerDir.Cross(dir).Normalize();
    double ang = draggerDir.GetAngleOriented(dir, axis);

    SbRotation rot{Base::convertTo<SbVec3f>(axis), static_cast<float>(ang)};
    rotation.setValue(rot);
}
