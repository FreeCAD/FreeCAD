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
#include <Inventor/SoToggleSwitch.h>

using namespace Gui;

SO_KIT_SOURCE(SoRotatorGeometryKit)

void SoRotatorGeometryKit::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometryKit, SoBaseKit, "BaseKit");
}

SoRotatorGeometryKit::SoRotatorGeometryKit()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometryKit);

    SO_KIT_ADD_FIELD(pivotPosition, (0.0, 0.0, 0.0));

    SO_KIT_INIT_INSTANCE();
}

SO_KIT_SOURCE(SoRotatorGeometry)

void SoRotatorGeometry::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometry, SoRotatorGeometryKit, "RotatorGeometryKit");
}

SoRotatorGeometry::SoRotatorGeometry()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometry);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(drawStyle, SoDrawStyle, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(arcCoords, SoCoordinate3, false, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(arc, SoLineSet, false, this, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rotorPivot, SoSphere, false, this, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(_rotorPivotTranslation, SoTranslation, false, this, rotorPivot, false);

    SO_KIT_ADD_FIELD(arcRadius, (8.0f));
    SO_KIT_ADD_FIELD(arcAngle, (std::numbers::pi_v<float> / 2.0f));
    SO_KIT_ADD_FIELD(sphereRadius, (0.8f));
    SO_KIT_ADD_FIELD(arcThickness, (4.0f));

    SO_KIT_INIT_INSTANCE();

    auto rotorPivot = SO_GET_ANY_PART(this, "rotorPivot", SoSphere);
    rotorPivot->radius.connectFrom(&sphereRadius);

    auto drawStyle = SO_GET_ANY_PART(this, "drawStyle", SoDrawStyle);
    drawStyle->lineWidth.connectFrom(&arcThickness);

    auto translation = SO_GET_ANY_PART(this, "_rotorPivotTranslation", SoTranslation);
    pivotPosition.connectFrom(&translation->translation);

    auto arc = SO_GET_ANY_PART(this, "arc", SoLineSet);
    arc->numVertices = segments + 1;

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    // forces the notify method to get called so that the initial translations and other values are set
    arcRadius.touch();
}

void SoRotatorGeometry::notify(SoNotList* notList)
{
    assert(notList);
    SoField* lastField = notList->getLastField();

    if (lastField == &arcRadius || lastField == &arcAngle) {
        float angle = arcAngle.getValue();
        float radius = arcRadius.getValue();

        auto coordinates = SO_GET_ANY_PART(this, "arcCoords", SoCoordinate3);
        float angleIncrement = angle / static_cast<float>(segments);
        SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), angleIncrement);
        SbVec3f point(radius, 0.0, 0.0);
        for (int index = 0; index <= segments; ++index) {
            coordinates->point.set1Value(index, point);
            rotation.multVec(point, point);
        }

        auto translation = SO_GET_ANY_PART(this, "_rotorPivotTranslation", SoTranslation);
        translation->translation = {radius * cos(angle / 2.0f), radius * sin(angle / 2.0f), 0};
    }
}

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

    FC_ADD_CATALOG_ENTRY(activeSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(secondaryColor, SoBaseColor, activeSwitch);
    FC_ADD_CATALOG_ENTRY(scale, SoScale, geomSeparator);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(rotator, SoRotatorGeometryKit, SoRotatorGeometry, false, geomSeparator, "", true);

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (std::numbers::pi / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCount, (0));
    SO_KIT_ADD_FIELD(activeColor, (1, 1, 0));
    SO_KIT_ADD_FIELD(geometryScale, (1, 1, 1));

    SO_KIT_INIT_INSTANCE();

    setPart("secondaryColor", buildActiveColor());

    auto scale = SO_GET_ANY_PART(this, "scale", SoScale);
    scale->scaleFactor.connectFrom(&geometryScale);

    this->addStartCallback(&SoRotationDragger::startCB);
    this->addMotionCallback(&SoRotationDragger::motionCB);
    this->addFinishCallback(&SoRotationDragger::finishCB);

    addValueChangedCallback(&SoRotationDragger::valueChangedCB);

    fieldSensor.setFunction(&SoRotationDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);

    FC_SET_TOGGLE_SWITCH("activeSwitch", false);
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

SoBaseColor* SoRotationDragger::buildActiveColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&activeColor);

    return color;
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
    FC_SET_TOGGLE_SWITCH("activeSwitch", true);

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
    FC_SET_TOGGLE_SWITCH("activeSwitch", false);
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

    FC_ADD_CATALOG_ENTRY(draggerSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(transform, SoTransform, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(dragger, SoRotationDragger, draggerSwitch);

    SO_KIT_ADD_FIELD(rotation, (0, 0, 0, 0));
    SO_KIT_ADD_FIELD(color, (0, 0, 0));
    SO_KIT_ADD_FIELD(translation, (0, 0, 0));
    SO_KIT_ADD_FIELD(visible, (1));

    SO_KIT_INIT_INSTANCE();

    setPart("baseColor", buildColor());
    setPart("transform", buildTransform());

    auto sw = SO_GET_ANY_PART(this, "draggerSwitch", SoToggleSwitch);
    sw->on.connectFrom(&visible);
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

SoRotationDragger* SoRotationDraggerContainer::getDragger()
{
    return SO_GET_PART(this, "dragger", SoRotationDragger);
}

void Gui::SoRotationDraggerContainer::setPointerDirection(const Base::Vector3d& dir)
{
    // This is the direction from the origin to the spherical pivot of the rotator
    Base::Vector3d draggerDir = Base::convertTo<Base::Vector3d>(
        SO_GET_ANY_PART(this, "rotator", SoRotatorGeometryKit)->pivotPosition.getValue()
    );

    Base::Vector3d axis = draggerDir.Cross(dir).Normalize();
    double ang = draggerDir.GetAngleOriented(dir, axis);

    SbRotation rot{Base::convertTo<SbVec3f>(axis), static_cast<float>(ang)};
    rotation.setValue(rot);
}
