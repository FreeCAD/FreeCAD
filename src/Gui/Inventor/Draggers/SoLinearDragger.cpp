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
#include <algorithm>

#include <Inventor/SbRotation.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/engines/SoCalculator.h>
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
#include <Inventor/draggers/SoDragger.h>
#endif

#include <Base/Quantity.h>

#include "SoLinearDragger.h"

#include "MainWindow.h"
#include "Utilities.h"

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

    FC_ADD_CATALOG_ENTRY(translator, SoSeparator, geomSeparator);
    FC_ADD_CATALOG_ENTRY(activeSwitch, SoSwitch, translator);
    FC_ADD_CATALOG_ENTRY(secondaryColor, SoBaseColor, activeSwitch);
    FC_ADD_CATALOG_ENTRY(coneSeparator, SoSeparator, translator);
    FC_ADD_CATALOG_ENTRY(cylinderSeparator, SoSeparator, translator);
    // For some reason changing the whichChild parameter of this switch doesn't hide the label
    FC_ADD_CATALOG_ENTRY(labelSwitch, SoSwitch, translator);
    FC_ADD_CATALOG_ENTRY(labelSeparator, SoSeparator, labelSwitch);

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCount, (0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));
    SO_KIT_ADD_FIELD(coneBottomRadius, (0.8));
    SO_KIT_ADD_FIELD(coneHeight, (2.5));
    SO_KIT_ADD_FIELD(cylinderHeight, (10.0));
    SO_KIT_ADD_FIELD(cylinderRadius, (0.1));
    SO_KIT_ADD_FIELD(activeColor, (1, 1, 0));

    SO_KIT_INIT_INSTANCE();

    setupGeometryCalculator();
    SoInteractionKit::setPart("cylinderSeparator", buildCylinderGeometry());
    SoInteractionKit::setPart("coneSeparator", buildConeGeometry());
    SoInteractionKit::setPart("labelSeparator", buildLabelGeometry());
    SoInteractionKit::setPart("secondaryColor", buildActiveColor());

    FC_SET_SWITCH("activeSwitch", SO_SWITCH_NONE);
    setLabelVisibility(true);

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

SoSeparator* SoLinearDragger::buildCylinderGeometry()
{
    auto cylinderSeparator = new SoSeparator();

    auto cylinderLightModel = new SoLightModel();
    cylinderLightModel->model = SoLightModel::BASE_COLOR;
    cylinderSeparator->addChild(cylinderLightModel);

    auto cylinderTranslation = new SoTranslation();
    cylinderSeparator->addChild(cylinderTranslation);
    cylinderTranslation->translation.connectFrom(&calculator->oA);

    auto cylinder = new SoCylinder();
    cylinder->radius.setValue(cylinderRadius.getValue());
    cylinder->height.setValue(cylinderHeight.getValue());
    cylinderSeparator->addChild(cylinder);

    cylinder->radius.connectFrom(&cylinderRadius);
    cylinder->height.connectFrom(&cylinderHeight);
    calculator->a.connectFrom(&cylinder->height);

    return cylinderSeparator;
}

SoSeparator* SoLinearDragger::buildConeGeometry()
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
    coneSeparator->addChild(coneTranslation);
    coneTranslation->translation.connectFrom(&calculator->oB);

    auto cone = new SoCone();
    cone->bottomRadius.setValue(coneBottomRadius.getValue());
    cone->height.setValue(coneHeight.getValue());
    coneSeparator->addChild(cone);

    cone->bottomRadius.connectFrom(&coneBottomRadius);
    cone->height.connectFrom(&coneHeight);
    calculator->b.connectFrom(&cone->height);

    return coneSeparator;
}

SoSeparator* SoLinearDragger::buildLabelGeometry()
{
    auto labelSeparator = new SoSeparator;

    auto labelTranslation = new SoTranslation();
    labelSeparator->addChild(labelTranslation);
    labelTranslation->translation.connectFrom(&calculator->oC);

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

void SoLinearDragger::setupGeometryCalculator()
{
    calculator = new SoCalculator;
    // Recalculate the corresponding variables in the left hand side whenever any of the variables in the right hand side change
    // oA -> cylinderTranslation
    // oB -> coneTranslation
    // oC -> labelTranslation
    // a  -> cylinderHeight
    // b  -> coneHeight
    calculator->expression =
        "oA = vec3f(0, a * 0.5, 0); "
        "oB = vec3f(0, a + b * 0.5, 0); "
        "oC = vec3f(0, a + b * 1.5, 0); ";
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
    FC_SET_SWITCH("activeSwitch", SO_SWITCH_ALL);

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
    FC_SET_SWITCH("activeSwitch", SO_SWITCH_NONE);
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

void SoLinearDragger::setLabelVisibility(bool visible) {
    FC_SET_SWITCH("labelSwitch", visible? SO_SWITCH_ALL : SO_SWITCH_NONE);
}

bool SoLinearDragger::isLabelVisible() {
    auto* sw = SO_GET_ANY_PART(this, "labelSwitch", SoSwitch);
    return sw->whichChild.getValue() == SO_SWITCH_ALL;
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

    FC_ADD_CATALOG_ENTRY(draggerSwitch, SoSwitch, geomSeparator);
    FC_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(transform, SoTransform, draggerSwitch);
    FC_ADD_CATALOG_ENTRY(dragger, SoLinearDragger, draggerSwitch);

    SO_KIT_ADD_FIELD(rotation, (0, 0, 0, 0));
    SO_KIT_ADD_FIELD(color, (0, 0, 0));
    SO_KIT_ADD_FIELD(translation, (0, 0, 0));

    SO_KIT_INIT_INSTANCE();

    SoInteractionKit::setPart("baseColor", buildColor());
    SoInteractionKit::setPart("transform", buildTransform());

    setVisibility(true);
}

SoBaseColor* SoLinearDraggerContainer::buildColor()
{
    auto color = new SoBaseColor;
    color->rgb.connectFrom(&this->color);

    return color;
}

SoTransform* SoLinearDraggerContainer::buildTransform() {
    auto transform = new SoTransform;
    transform->translation.connectFrom(&this->translation);
    transform->rotation.connectFrom(&this->rotation);

    return transform;
}

void SoLinearDraggerContainer::setVisibility(bool visible)
{
    FC_SET_SWITCH("draggerSwitch", visible? SO_SWITCH_ALL : SO_SWITCH_NONE);
}

bool SoLinearDraggerContainer::isVisible() {
    auto* sw = SO_GET_ANY_PART(this, "draggerSwitch", SoSwitch);
    return sw->whichChild.getValue() == SO_SWITCH_ALL;
}

SoLinearDragger* SoLinearDraggerContainer::getDragger()
{
    return SO_GET_PART(this, "dragger", SoLinearDragger);
}

void Gui::SoLinearDraggerContainer::setPointerDirection(const Base::Vector3d& dir)
{
    // This is the direction along which the SoLinearDragger points in it local space
    Base::Vector3d draggerDir{0, 1, 0};
    Base::Vector3d axis = draggerDir.Cross(dir).Normalize();
    double ang = draggerDir.GetAngleOriented(dir, axis);

    SbRotation rot{Base::convertTo<SbVec3f>(axis), static_cast<float>(ang)};
    rotation.setValue(rot);
}
