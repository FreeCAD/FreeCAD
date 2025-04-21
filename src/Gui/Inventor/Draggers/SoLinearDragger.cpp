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

#include "SoLinearDragger.h"

#include "MainWindow.h"
#include "SoFCDB.h"

#include <SoTextLabel.h>

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

    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(activeSwitch, SoSwitch, TRUE, translator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(activeColor, SoBaseColor, TRUE, activeSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(coneSeparator, SoSeparator, TRUE, translator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(cylinderSeparator, SoSeparator, TRUE, translator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(labelSeparator, SoSeparator, TRUE, translator, "", TRUE);

    if (SO_KIT_IS_FIRST_INSTANCE()) {
        buildFirstInstance();
    }

    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, TRUE, geomSeparator, "", TRUE);

    SO_KIT_ADD_FIELD(label, (""));
    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCount, (0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));

    SO_KIT_INIT_INSTANCE();

    // initialize default parts.
    // first is from 'SO_KIT_CATALOG_ENTRY_HEADER' macro
    // second is unique name from buildFirstInstance().
    SoInteractionKit::setPartAsDefault("coneSeparator", "CSysDynamics_TDragger_Cone");
    SoInteractionKit::setPartAsDefault("cylinderSeparator", "CSysDynamics_TDragger_Cylinder");
    SoInteractionKit::setPartAsDefault("activeColor", "CSysDynamics_TDragger_ActiveColor");

    SoInteractionKit::setPart("labelSeparator", buildLabelGeometry());

    auto sw = SO_GET_ANY_PART(this, "activeSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);


    this->addStartCallback(&SoLinearDragger::startCB);
    this->addMotionCallback(&SoLinearDragger::motionCB);
    this->addFinishCallback(&SoLinearDragger::finishCB);

    addValueChangedCallback(&SoLinearDragger::valueChangedCB);

    fieldSensor.setFunction(&SoLinearDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
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

void SoLinearDragger::buildFirstInstance()
{
    auto cylinderSeparator = buildCylinderGeometry();
    auto coneSeparator = buildConeGeometry();
    auto activeColor = buildActiveColor();

    cylinderSeparator->setName("CSysDynamics_TDragger_Cylinder");
    coneSeparator->setName("CSysDynamics_TDragger_Cone");
    activeColor->setName("CSysDynamics_TDragger_ActiveColor");

    SoFCDB::getStorage()->addChild(cylinderSeparator);
    SoFCDB::getStorage()->addChild(coneSeparator);
    SoFCDB::getStorage()->addChild(activeColor);
}

SoSeparator* SoLinearDragger::buildCylinderGeometry() const
{
    auto cylinderSeparator = new SoSeparator();

    auto cylinderLightModel = new SoLightModel();
    cylinderLightModel->model = SoLightModel::BASE_COLOR;
    cylinderSeparator->addChild(cylinderLightModel);

    auto cylinderTranslation = new SoTranslation();
    cylinderTranslation->translation.setValue(0.0, cylinderHeight / 2.0, 0.0);
    cylinderSeparator->addChild(cylinderTranslation);

    auto cylinder = new SoCylinder();
    cylinder->radius.setValue(cylinderRadius);
    cylinder->height.setValue(cylinderHeight);
    cylinderSeparator->addChild(cylinder);

    return cylinderSeparator;
}

SoSeparator* SoLinearDragger::buildConeGeometry() const
{
    auto coneLightModel = new SoLightModel();
    coneLightModel->model = SoLightModel::BASE_COLOR;

    auto coneSeparator = new SoSeparator();
    coneSeparator->addChild(coneLightModel);

    auto pickStyle = new SoPickStyle();
    pickStyle->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    pickStyle->setOverride(TRUE);
    coneSeparator->addChild(pickStyle);

    auto coneTranslation = new SoTranslation();
    coneTranslation->translation.setValue(0.0, cylinderHeight + coneHeight / 2.0, 0.0);
    coneSeparator->addChild(coneTranslation);

    auto cone = new SoCone();
    cone->bottomRadius.setValue(coneBottomRadius);
    cone->height.setValue(coneHeight);
    coneSeparator->addChild(cone);

    return coneSeparator;
}

SoSeparator* SoLinearDragger::buildLabelGeometry()
{
    auto labelSeparator = new SoSeparator();

    auto labelTranslation = new SoTranslation();
    labelTranslation->translation.setValue(0.0, cylinderHeight + coneHeight * 1.5, 0.0);
    labelSeparator->addChild(labelTranslation);

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
    auto colorActive = new SoBaseColor();
    colorActive->rgb.setValue(1.0, 1.0, 0.0);

    return colorActive;
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

    // all this just to get the translation?
    SbVec3f trans, scaleDummy;
    SbRotation rotationDummy, scaleOrientationDummy;
    matrix.getTransform(trans, rotationDummy, scaleDummy, scaleOrientationDummy);

    sudoThis->fieldSensor.detach();
    if (sudoThis->translation.getValue() != trans) {
        sudoThis->translation = trans;
    }
    sudoThis->fieldSensor.attach(&sudoThis->translation);
}

void SoLinearDragger::dragStart()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "activeSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

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
    float scaledIncrement =
        static_cast<float>(translationIncrement.getValue()) / autoScaleResult.getValue();

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

    Base::Quantity quantity(static_cast<double>(translationIncrementCount.getValue())
                                * translationIncrement.getValue(),
                            Base::Unit::Length);

    QString message =
        QStringLiteral("%1 %2").arg(QObject::tr("Translation:"), QString::fromStdString(quantity.getUserString()));
    getMainWindow()->showMessage(message, 3000);
}

void SoLinearDragger::dragFinish()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "activeSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
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
