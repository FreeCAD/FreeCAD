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

#include "SoPlanarDragger.h"

#include "MainWindow.h"
#include "SoFCDB.h"

#include <SoTextLabel.h>

using namespace Gui;

SO_KIT_SOURCE(SoPlanarDragger)

void SoPlanarDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoPlanarDragger, SoDragger, "Dragger");
}

SoPlanarDragger::SoPlanarDragger()
{
    SO_KIT_CONSTRUCTOR(SoPlanarDragger);
#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    this->ref();
#endif

    SO_KIT_ADD_CATALOG_ENTRY(planarTranslatorSwitch, SoSwitch, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(planarTranslator, SoSeparator, TRUE, planarTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(planarTranslatorActive,
                             SoSeparator,
                             TRUE,
                             planarTranslatorSwitch,
                             "",
                             TRUE);

    if (SO_KIT_IS_FIRST_INSTANCE()) {
        buildFirstInstance();
    }

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementXCount, (0));
    SO_KIT_ADD_FIELD(translationIncrementYCount, (0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));

    SO_KIT_INIT_INSTANCE();

    // initialize default parts.
    // first is from 'SO_KIT_CATALOG_ENTRY_HEADER' macro
    // second is unique name from buildFirstInstance().
    this->setPartAsDefault("planarTranslator", "CSysDynamics_TPlanarDragger_Translator");
    this->setPartAsDefault("planarTranslatorActive",
                           "CSysDynamics_TPlanarDragger_TranslatorActive");

    SoSwitch* sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);

    this->addStartCallback(&SoPlanarDragger::startCB);
    this->addMotionCallback(&SoPlanarDragger::motionCB);
    this->addFinishCallback(&SoPlanarDragger::finishCB);

    addValueChangedCallback(&SoPlanarDragger::valueChangedCB);

    fieldSensor.setFunction(&SoPlanarDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}

SoPlanarDragger::~SoPlanarDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&SoPlanarDragger::startCB);
    this->removeMotionCallback(&SoPlanarDragger::motionCB);
    this->removeFinishCallback(&SoPlanarDragger::finishCB);
    removeValueChangedCallback(&SoPlanarDragger::valueChangedCB);
}

void SoPlanarDragger::buildFirstInstance()
{
    SoGroup* geometryGroup = buildGeometry();

    auto localTranslator = new SoSeparator();
    localTranslator->setName("CSysDynamics_TPlanarDragger_Translator");
    localTranslator->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localTranslator);

    auto localTranslatorActive = new SoSeparator();
    localTranslatorActive->setName("CSysDynamics_TPlanarDragger_TranslatorActive");
    auto colorActive = new SoBaseColor();
    colorActive->rgb.setValue(1.0, 1.0, 0.0);
    localTranslatorActive->addChild(colorActive);
    localTranslatorActive->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localTranslatorActive);
}

SoGroup* SoPlanarDragger::buildGeometry()
{
    auto root = new SoGroup();

    float cubeWidthHeight = 2.0;
    float cubeDepth = 0.1f;

    auto translation = new SoTranslation();
    translation->translation.setValue(cubeWidthHeight + 0.15, cubeWidthHeight + 0.15, 0.0);
    root->addChild(translation);

    auto pickStyle = new SoPickStyle();
    pickStyle->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    pickStyle->setOverride(TRUE);
    root->addChild(pickStyle);

    auto lightModel = new SoLightModel();
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    auto cube = new SoCube();
    cube->width.setValue(cubeWidthHeight);
    cube->height.setValue(cubeWidthHeight);
    cube->depth.setValue(cubeDepth);
    root->addChild(cube);

    return root;
}

void SoPlanarDragger::startCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoPlanarDragger*>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void SoPlanarDragger::motionCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoPlanarDragger*>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void SoPlanarDragger::finishCB(void*, SoDragger* d)
{
    auto sudoThis = static_cast<SoPlanarDragger*>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void SoPlanarDragger::fieldSensorCB(void* f, SoSensor*)
{
    auto sudoThis = static_cast<SoPlanarDragger*>(f);

    if (!f) {
        return;
    }

    SbMatrix matrix = sudoThis->getMotionMatrix();  // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoPlanarDragger::valueChangedCB(void*, SoDragger* d)
{
    auto sudoThis = dynamic_cast<SoPlanarDragger*>(d);
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

void SoPlanarDragger::dragStart()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);

    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());
    projector.setPlane(
        SbPlane(SbVec3f(0.0, 0.0, 0.0), SbVec3f(1.0, 0.0, 0.0), SbVec3f(0.0, 1.0, 0.0)));
    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());

    SbMatrix localToWorld = getLocalToWorldMatrix();
    localToWorld.multVecMatrix(hitPoint, hitPoint);
    setStartingPoint((hitPoint));

    translationIncrementXCount.setValue(0);
    translationIncrementYCount.setValue(0);
}

void SoPlanarDragger::drag()
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

    Base::Quantity quantityX(static_cast<double>(translationIncrementXCount.getValue())
                                 * translationIncrement.getValue(),
                             Base::Unit::Length);
    Base::Quantity quantityY(static_cast<double>(translationIncrementYCount.getValue())
                                 * translationIncrement.getValue(),
                             Base::Unit::Length);

    QString message = QStringLiteral("%1 %2, %3")
                          .arg(QObject::tr("Translation XY:"),
                               QString::fromStdString(quantityX.getUserString()),
                               QString::fromStdString(quantityY.getUserString()));
    getMainWindow()->showMessage(message, 3000);
}

void SoPlanarDragger::dragFinish()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);
}

SbBool SoPlanarDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if (!doitalways && this->connectionsSetUp == onoff) {
        return onoff;
    }

    SbBool oldval = this->connectionsSetUp;

    if (onoff) {
        inherited::setUpConnections(onoff, doitalways);
        SoPlanarDragger::fieldSensorCB(this, nullptr);
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

SbVec3f SoPlanarDragger::roundTranslation(const SbVec3f& vecIn, float incrementIn)
{
    int xCount = 0;
    float xValue = vecIn[0];

    if (fabs(xValue) > (incrementIn / 2.0)) {
        xCount = static_cast<int>(xValue / incrementIn);
        float remainder = fmod(xValue, incrementIn);
        if (remainder >= (incrementIn / 2.0)) {
            xCount++;
        }
    }

    translationIncrementXCount.setValue(xCount);

    int yCount = 0;
    float yValue = vecIn[1];

    if (fabs(yValue) > (incrementIn / 2.0)) {
        yCount = static_cast<int>(yValue / incrementIn);
        float remainder = fmod(yValue, incrementIn);
        if (remainder >= (incrementIn / 2.0)) {
            yCount++;
        }
    }

    translationIncrementYCount.setValue(yCount);

    SbVec3f out;
    out[0] = static_cast<float>(xCount) * incrementIn;
    out[1] = static_cast<float>(yCount) * incrementIn;
    out[2] = 0.0;

    return out;
}
