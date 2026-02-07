// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#include <cassert>
#include <numbers>

#include <Inventor/SbRotation.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/nodes/SoTransform.h>

#include "SoRotationDragger.h"

#include <Base/Quantity.h>
#include <Gui/Inventor/SoToggleSwitch.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>

#include "SoRotationDraggerGeometry.h"

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

    SO_KIT_ADD_CATALOG_ENTRY(baseGeomSwitch, SoToggleSwitch, true, topSeparator, motionMatrix, true);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(
        baseGeom,
        SoRotatorGeometryBaseKit,
        SoRotatorBase,
        true,
        baseGeomSwitch,
        "",
        true
    );

    FC_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, geomSeparator);
    FC_ADD_CATALOG_ENTRY(activeSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(secondaryColor, SoBaseColor, activeSwitch);
    FC_ADD_CATALOG_ENTRY(scale, SoScale, geomSeparator);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(
        rotator,
        SoRotatorGeometryKit,
        SoRotatorGeometry,
        false,
        geomSeparator,
        "",
        true
    );

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (std::numbers::pi / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCount, (0));
    SO_KIT_ADD_FIELD(color, (1, 0, 0));
    SO_KIT_ADD_FIELD(activeColor, (1, 1, 0));
    SO_KIT_ADD_FIELD(geometryScale, (1, 1, 1));
    SO_KIT_ADD_FIELD(active, (false));
    SO_KIT_ADD_FIELD(baseGeomVisible, (false));

    SO_KIT_INIT_INSTANCE();

    setPart("baseColor", buildColor());
    setPart("secondaryColor", buildActiveColor());

    auto scale = SO_GET_ANY_PART(this, "scale", SoScale);
    scale->scaleFactor.connectFrom(&geometryScale);

    auto rotator = SO_GET_ANY_PART(this, "rotator", SoRotatorGeometryKit);
    rotator->geometryScale.connectFrom(&geometryScale);

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

SoBaseColor* SoRotationDragger::buildColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&this->color);

    return color;
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
    localRotation
        = SbRotation(tempVec, incrementCount * static_cast<float>(rotationIncrement.getValue()));

    // same problem as described in tDragger::drag.
    if (localRotation.equals(SbRotation(SbVec3f(0.0, 0.0, 1.0), 0.0), 0.00001f)) {
        setMotionMatrix(getStartMotionMatrix());
        this->valueChanged();
    }
    else {
        setMotionMatrix(appendRotation(getStartMotionMatrix(), localRotation, SbVec3f(0.0, 0.0, 0.0)));
    }

    Base::Quantity quantity(
        static_cast<double>(rotationIncrementCount.getValue()) * (180.0 / std::numbers::pi)
            * rotationIncrement.getValue(),
        Base::Unit::Angle
    );

    QString message = QStringLiteral("%1 %2").arg(
        QObject::tr("Rotation:"),
        QString::fromStdString(quantity.getUserString())
    );
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

void SoRotationDragger::instantiateBaseGeometry()
{
    baseGeomVisible = true;

    auto baseGeom = SO_GET_ANY_PART(this, "baseGeom", SoRotatorBase);
    baseGeom->geometryScale.connectFrom(&geometryScale);
    baseGeom->rotation.connectFrom(&rotation);
    baseGeom->active.connectFrom(&active);
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
    FC_ADD_CATALOG_ENTRY(transform, SoTransform, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(dragger, SoRotationDragger, draggerSwitch);

    SO_KIT_ADD_FIELD(rotation, (0, 0, 0, 0));
    SO_KIT_ADD_FIELD(color, (0, 0, 0));
    SO_KIT_ADD_FIELD(translation, (0, 0, 0));
    SO_KIT_ADD_FIELD(visible, (1));

    SO_KIT_INIT_INSTANCE();

    setPart("transform", buildTransform());

    auto sw = SO_GET_ANY_PART(this, "draggerSwitch", SoToggleSwitch);
    sw->on.connectFrom(&visible);

    getDragger()->color.connectFrom(&color);
}

SoTransform* SoRotationDraggerContainer::buildTransform()
{
    auto transform = new SoTransform;
    transform->translation.connectFrom(&this->translation);
    transform->rotation.connectFrom(&this->rotation);

    return transform;
}

SoRotationDragger* SoRotationDraggerContainer::getDragger()
{
    return SO_GET_PART(this, "dragger", SoRotationDragger);
}

SbVec3f SoRotationDraggerContainer::getPointerDirection()
{
    // This is the direction along which the SoLinearDragger points in it local space
    SbVec3f draggerDir
        = SO_GET_ANY_PART(this, "rotator", SoRotatorGeometryKit)->pivotPosition.getValue();
    rotation.getValue().multVec(draggerDir, draggerDir);

    return draggerDir;
}

void SoRotationDraggerContainer::setPointerDirection(const SbVec3f& dir)
{
    // This is the direction from the origin to the spherical pivot of the rotator
    SbVec3f draggerDir
        = SO_GET_ANY_PART(this, "rotator", SoRotatorGeometryKit)->pivotPosition.getValue();

    SbRotation rot {draggerDir, dir};
    rotation.setValue(rot);
}

void SoRotationDraggerContainer::setArcNormalDirection(const SbVec3f& dir)
{
    SbVec3f currentNormal = {0, 0, 1};
    auto currentRot = rotation.getValue();
    currentRot.multVec(currentNormal, currentNormal);

    // If the two directions are collinear and opposite then ensure that the
    // dragger is rotated along the pointer axis
    if (dir.equals(-currentNormal, 1e-5)) {
        SbRotation rot {getPointerDirection(), std::numbers::pi_v<float>};
        rotation = currentRot * rot;
        return;
    }

    SbRotation rot {currentNormal, dir};
    rotation = currentRot * rot;
}
