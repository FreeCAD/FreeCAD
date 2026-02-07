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

#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodekits/SoSubKit.h>

#include "SoLinearDragger.h"

#include <Base/Quantity.h>
#include <Gui/Inventor/SoToggleSwitch.h>
#include <Gui/MainWindow.h>
#include <Gui/SoTextLabel.h>
#include <Gui/Utilities.h>

#include "SoLinearDraggerGeometry.h"

using namespace Gui;

SO_KIT_SOURCE(SoLinearDragger)

void SoLinearDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoLinearDragger, SoDragger, "Dragger");
}

SoLinearDragger::SoLinearDragger()
{
    SO_KIT_CONSTRUCTOR(SoLinearDragger);

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    SO_KIT_ADD_CATALOG_ENTRY(baseGeomSwitch, SoToggleSwitch, true, topSeparator, motionMatrix, true);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(
        baseGeom,
        SoLinearGeometryBaseKit,
        SoArrowBase,
        true,
        baseGeomSwitch,
        "",
        true
    );

    FC_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, geomSeparator);
    FC_ADD_CATALOG_ENTRY(activeSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(secondaryColor, SoBaseColor, activeSwitch);
    FC_ADD_CATALOG_ENTRY(labelSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(labelSeparator, SoSeparator, labelSwitch);
    FC_ADD_CATALOG_ENTRY(scale, SoScale, geomSeparator);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(
        arrow,
        SoLinearGeometryKit,
        SoArrowGeometry,
        true,
        geomSeparator,
        "",
        true
    );

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCount, (0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));
    SO_KIT_ADD_FIELD(color, (1, 0, 0));
    SO_KIT_ADD_FIELD(activeColor, (1, 1, 0));
    SO_KIT_ADD_FIELD(labelVisible, (1));
    SO_KIT_ADD_FIELD(geometryScale, (1, 1, 1));
    SO_KIT_ADD_FIELD(active, (false));
    SO_KIT_ADD_FIELD(baseGeomVisible, (false));

    SO_KIT_INIT_INSTANCE();

    setPart("baseColor", buildColor());
    setPart("labelSeparator", buildLabelGeometry());
    setPart("secondaryColor", buildActiveColor());

    auto sw = SO_GET_ANY_PART(this, "labelSwitch", SoToggleSwitch);
    sw->on.connectFrom(&labelVisible);

    auto scale = SO_GET_ANY_PART(this, "scale", SoScale);
    scale->scaleFactor.connectFrom(&geometryScale);

    this->addStartCallback(&SoLinearDragger::startCB);
    this->addMotionCallback(&SoLinearDragger::motionCB);
    this->addFinishCallback(&SoLinearDragger::finishCB);

    addValueChangedCallback(&SoLinearDragger::valueChangedCB);

    fieldSensor.setFunction(&SoLinearDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);

    sw = SO_GET_ANY_PART(this, "activeSwitch", SoToggleSwitch);
    sw->on.connectFrom(&active);

    sw = SO_GET_ANY_PART(this, "baseGeomSwitch", SoToggleSwitch);
    sw->on.connectFrom(&baseGeomVisible);
}

SoLinearDragger::~SoLinearDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&SoLinearDragger::startCB);
    this->removeMotionCallback(&SoLinearDragger::motionCB);
    this->removeFinishCallback(&SoLinearDragger::finishCB);
    removeValueChangedCallback(&SoLinearDragger::valueChangedCB);
}

SoSeparator* SoLinearDragger::buildLabelGeometry()
{
    auto labelSeparator = new SoSeparator;

    auto labelTranslation = new SoTranslation();
    labelSeparator->addChild(labelTranslation);

    auto arrow = SO_GET_PART(this, "arrow", SoLinearGeometryKit);
    labelTranslation->translation.connectFrom(&arrow->tipPosition);

    auto label = new SoFrameLabel();
    label->string.connectFrom(&this->label);
    label->textColor.setValue(1.0, 1.0, 1.0);
    label->horAlignment = SoImage::CENTER;
    label->vertAlignment = SoImage::HALF;
    label->border = false;
    label->backgroundUseBaseColor = true;
    labelSeparator->addChild(label);

    return labelSeparator;
}

SoBaseColor* SoLinearDragger::buildActiveColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&activeColor);

    return color;
}

SoBaseColor* SoLinearDragger::buildColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&this->color);

    return color;
}

void SoLinearDragger::startCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoLinearDragger*>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void SoLinearDragger::motionCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoLinearDragger*>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void SoLinearDragger::finishCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoLinearDragger*>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void SoLinearDragger::fieldSensorCB(void* f, SoSensor*)
{
    auto sudoThis = static_cast<SoLinearDragger*>(f);

    if (!f) {
        return;
    }

    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoLinearDragger::valueChangedCB(void*, SoDragger* d)
{
    auto sudoThis = dynamic_cast<SoLinearDragger*>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    SbVec3f trans = getMatrixTransform(matrix).translation;

    sudoThis->fieldSensor.detach();
    if (sudoThis->translation.getValue() != trans) {
        sudoThis->translation = trans;
    }
    sudoThis->fieldSensor.attach(&sudoThis->translation);
}

void SoLinearDragger::dragStart()
{
    active = true;

    // do an initial projection to eliminate discrepancies
    // in arrow head pick. we define the arrow in the y+ direction
    // and we know local space will be relative to this. so y vector
    // line projection will work.
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());
    projector.setLine(SbLine(SbVec3f(0.0, 0.0, 0.0), SbVec3f(0.0, 1.0, 0.0)));
    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());

    projector.setLine(SbLine(SbVec3f(0.0, 0.0, 0.0), hitPoint));

    SbMatrix localToWorld = getLocalToWorldMatrix();
    localToWorld.multVecMatrix(hitPoint, hitPoint);
    setStartingPoint((hitPoint));

    translationIncrementCount.setValue(0);
}

void SoLinearDragger::drag()
{
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());

    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());
    SbVec3f startingPoint = getLocalStartingPoint();
    SbVec3f localMovement = hitPoint - startingPoint;

    // scale the increment to match local space.
    float scaledIncrement = static_cast<float>(translationIncrement.getValue())
        / autoScaleResult.getValue();

    localMovement = roundTranslation(localMovement, scaledIncrement);
    // when the movement vector is null either the appendTranslation or
    // the setMotionMatrix doesn't work. either way it stops translating
    // back to its initial starting point.
    if (localMovement.equals(SbVec3f(0.0, 0.0, 0.0), 0.00001f)) {
        setMotionMatrix(getStartMotionMatrix());
        // don't know why I need the following but if I don't have it
        // it won't return to original position.
        this->valueChanged();
    }
    else {
        setMotionMatrix(appendTranslation(getStartMotionMatrix(), localMovement));
    }

    Base::Quantity quantity(
        static_cast<double>(translationIncrementCount.getValue()) * translationIncrement.getValue(),
        Base::Unit::Length
    );

    QString message = QStringLiteral("%1 %2").arg(
        QObject::tr("Translation:"),
        QString::fromStdString(quantity.getUserString())
    );
    getMainWindow()->showMessage(message, 3000);
}

void SoLinearDragger::dragFinish()
{
    active = false;
}

SbBool SoLinearDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if (!doitalways && this->connectionsSetUp == onoff) {
        return onoff;
    }

    SbBool oldval = this->connectionsSetUp;

    if (onoff) {
        inherited::setUpConnections(onoff, doitalways);
        SoLinearDragger::fieldSensorCB(this, nullptr);
        if (this->fieldSensor.getAttachedField() != &this->translation) {
            this->fieldSensor.attach(&this->translation);
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

SbVec3f SoLinearDragger::roundTranslation(const SbVec3f& vecIn, float incrementIn)
{
    // everything is transformed into local space. That means we only have
    // worry about the y-value.

    int yCount = 0;
    float yValue = vecIn[1];

    if (fabs(yValue) > (incrementIn / 2.0)) {
        yCount = static_cast<int>(yValue / incrementIn);
        float remainder = fmod(yValue, incrementIn);
        if (remainder >= (incrementIn / 2.0)) {
            yCount++;
        }
    }

    translationIncrementCount.setValue(yCount);

    SbVec3f out;
    out[0] = 0;
    out[1] = static_cast<float>(yCount) * incrementIn;
    out[2] = 0.0;

    return out;
}

void SoLinearDragger::instantiateBaseGeometry()
{
    baseGeomVisible = true;

    auto baseGeom = SO_GET_ANY_PART(this, "baseGeom", SoLinearGeometryBaseKit);
    baseGeom->geometryScale.connectFrom(&geometryScale);
    baseGeom->translation.connectFrom(&translation);
    baseGeom->active.connectFrom(&active);
}

SO_KIT_SOURCE(SoLinearDraggerContainer)

void SoLinearDraggerContainer::initClass()
{
    SoLinearDragger::initClass();
    SO_KIT_INIT_CLASS(SoLinearDraggerContainer, SoInteractionKit, "InteractionKit");
}

SoLinearDraggerContainer::SoLinearDraggerContainer()
{
    SO_KIT_CONSTRUCTOR(SoLinearDraggerContainer);

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    FC_ADD_CATALOG_ENTRY(draggerSwitch, SoToggleSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(transform, SoTransform, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(dragger, SoLinearDragger, draggerSwitch);

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

SoTransform* SoLinearDraggerContainer::buildTransform()
{
    auto transform = new SoTransform;
    transform->translation.connectFrom(&this->translation);
    transform->rotation.connectFrom(&this->rotation);

    return transform;
}

SoLinearDragger* SoLinearDraggerContainer::getDragger()
{
    return SO_GET_PART(this, "dragger", SoLinearDragger);
}

SbVec3f SoLinearDraggerContainer::getPointerDirection()
{
    // This is the direction along which the SoLinearDragger points in it local space
    SbVec3f draggerDir = SO_GET_ANY_PART(this, "arrow", SoLinearGeometryKit)->tipPosition.getValue();
    rotation.getValue().multVec(draggerDir, draggerDir);

    return draggerDir;
}

void SoLinearDraggerContainer::setPointerDirection(const SbVec3f& dir)
{
    // This is the direction from the origin to the tip of the dragger
    SbVec3f draggerDir = SO_GET_ANY_PART(this, "arrow", SoLinearGeometryKit)->tipPosition.getValue();

    SbRotation rot {draggerDir, dir};
    rotation.setValue(rot);
}
