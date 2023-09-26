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

#include <Inventor/SbRotation.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoAnnotation.h>
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
#endif

#include <Base/Quantity.h>

#include "SoFCCSysDragger.h"
#include "MainWindow.h"
#include "SoFCDB.h"


/*
   GENERAL NOTE ON COIN3D CUSTOM DRAGGERS
   * You basically have two choices for creating custom dragger geometry.
   * 1) create an .iv file and set environment variable to the file path. This
   *     comes with install headaches.
   * 2) create an .iv file and run through a mock compiler that generates a header
   *     file to include in the project. I would have gone this way but after installing
   *     inventor-demo(ubuntu), the mock compiler tool was there only in source and make
   *     didn't do anything. Didn't want to put any time into something I didn't like anyway.
   *
   * static SbList <SoNode*> * defaultdraggerparts = NULL; is a global definition
   * in SoInteractionKit that contains the geometry. There doesn't appear to be anyway
   * to add to this other than readDefaultParts, that takes a file. So maybe a temp file?
   *
   * naming appears to be central to the core. It looks like as long as an object
   * is alive SoNode::getByName() will find it. So maybe just create my own little
   * container of objects to keep the default geometry alive....This appears to be
   * working and I like this solution.
   *
   * SoInteractionKit warns about these
   * names all being the same scope and do NOT have to be unique. Need to make names
   * descriptive to avoid collisions.

   * this is point of the SoGroup accessed from SoFCDB::getStorage().
*/

using namespace Gui;

SO_KIT_SOURCE(TDragger)

void TDragger::initClass()
{
  SO_KIT_INIT_CLASS(TDragger, SoDragger, "Dragger");
}

TDragger::TDragger()
{
    SO_KIT_CONSTRUCTOR(TDragger);

    SO_KIT_ADD_CATALOG_ENTRY(translatorSwitch, SoSwitch, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, TRUE, translatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(translatorActive, SoSeparator, TRUE, translatorSwitch, "", TRUE);

    if (SO_KIT_IS_FIRST_INSTANCE())
        buildFirstInstance();

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCount, (0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));

    SO_KIT_INIT_INSTANCE();

    // initialize default parts.
    // first is from 'SO_KIT_CATALOG_ENTRY_HEADER' macro
    // second is unique name from buildFirstInstance().
    this->setPartAsDefault("translator", "CSysDynamics_TDragger_Translator");
    this->setPartAsDefault("translatorActive", "CSysDynamics_TDragger_TranslatorActive");

    SoSwitch *sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);

    this->addStartCallback(&TDragger::startCB);
    this->addMotionCallback(&TDragger::motionCB);
    this->addFinishCallback(&TDragger::finishCB);

    addValueChangedCallback(&TDragger::valueChangedCB);

    fieldSensor.setFunction(&TDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}

TDragger::~TDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&TDragger::startCB);
    this->removeMotionCallback(&TDragger::motionCB);
    this->removeFinishCallback(&TDragger::finishCB);
    removeValueChangedCallback(&TDragger::valueChangedCB);
}

void TDragger::buildFirstInstance()
{
    SoGroup *geometryGroup = buildGeometry();

    auto localTranslator = new SoSeparator();
    localTranslator->setName("CSysDynamics_TDragger_Translator");
    localTranslator->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localTranslator);

    auto localTranslatorActive = new SoSeparator();
    localTranslatorActive->setName("CSysDynamics_TDragger_TranslatorActive");
    auto colorActive = new SoBaseColor();
    colorActive->rgb.setValue(1.0, 1.0, 0.0);
    localTranslatorActive->addChild(colorActive);
    localTranslatorActive->addChild(geometryGroup);
    SoFCDB::getStorage()->addChild(localTranslatorActive);
}

SoGroup* TDragger::buildGeometry()
{
    //this builds one leg in the Y+ direction because of default done direction.
    //the location anchor for shapes is the center of shape.

    auto root = new SoGroup();

    //cylinder
    float cylinderHeight = 10.0;
    float cylinderRadius = 0.1f;
    auto cylinderSeparator = new SoSeparator();
    root->addChild(cylinderSeparator);

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

    //cone
    float coneBottomRadius = 0.8F;
    float coneHeight = 2.5;
    auto coneSeparator = new SoSeparator();
    root->addChild(coneSeparator);

    auto coneLightModel = new SoLightModel();
    coneLightModel->model = SoLightModel::BASE_COLOR;
    coneSeparator->addChild(coneLightModel);

    auto pickStyle = new SoPickStyle();
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    pickStyle->setOverride(TRUE);
    coneSeparator->addChild(pickStyle);

    auto coneTranslation = new SoTranslation();
    coneTranslation->translation.setValue(0.0, cylinderHeight + coneHeight / 2.0, 0.0);
    coneSeparator->addChild(coneTranslation);

    auto cone = new SoCone();
    cone->bottomRadius.setValue(coneBottomRadius);
    cone->height.setValue(coneHeight);
    coneSeparator->addChild(cone);

    return root;
}

void TDragger::startCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TDragger *>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void TDragger::motionCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TDragger *>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void TDragger::finishCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TDragger *>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void TDragger::fieldSensorCB(void *f, SoSensor *)
{
    auto sudoThis = static_cast<TDragger *>(f);

    if(!f)
      return;

    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void TDragger::valueChangedCB(void *, SoDragger *d)
{
    auto sudoThis = dynamic_cast<TDragger *>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft

    //all this just to get the translation?
    SbVec3f trans, scaleDummy;
    SbRotation rotationDummy, scaleOrientationDummy;
    matrix.getTransform(trans, rotationDummy, scaleDummy, scaleOrientationDummy);

    sudoThis->fieldSensor.detach();
    if (sudoThis->translation.getValue() != trans)
        sudoThis->translation = trans;
    sudoThis->fieldSensor.attach(&sudoThis->translation);
}

void TDragger::dragStart()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);

    //do an initial projection to eliminate discrepancies
    //in arrow head pick. we define the arrow in the y+ direction
    //and we know local space will be relative to this. so y vector
    //line projection will work.
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

void TDragger::drag()
{
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());

    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());
    SbVec3f startingPoint = getLocalStartingPoint();
    SbVec3f localMovement = hitPoint - startingPoint;

    //scale the increment to match local space.
    float scaledIncrement = static_cast<float>(translationIncrement.getValue()) / autoScaleResult.getValue();

    localMovement = roundTranslation(localMovement, scaledIncrement);
    //when the movement vector is null either the appendTranslation or
    //the setMotionMatrix doesn't work. either way it stops translating
    //back to its initial starting point.
    if (localMovement.equals(SbVec3f(0.0, 0.0, 0.0), 0.00001f))
    {
        setMotionMatrix(getStartMotionMatrix());
        //don't know why I need the following but if I don't have it
        //it won't return to original position.
        this->valueChanged();
    }
    else
        setMotionMatrix(appendTranslation(getStartMotionMatrix(), localMovement));

    Base::Quantity quantity(
      static_cast<double>(translationIncrementCount.getValue()) * translationIncrement.getValue(), Base::Unit::Length);

    QString message = QString::fromLatin1("%1 %2")
            .arg(QObject::tr("Translation:"), quantity.getUserString());
    getMainWindow()->showMessage(message, 3000);
}

void TDragger::dragFinish()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);
}

SbBool TDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff)
      return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff)
  {
    inherited::setUpConnections(onoff, doitalways);
    TDragger::fieldSensorCB(this, nullptr);
    if (this->fieldSensor.getAttachedField() != &this->translation)
      this->fieldSensor.attach(&this->translation);
  }
  else
  {
    if (this->fieldSensor.getAttachedField())
      this->fieldSensor.detach();
    inherited::setUpConnections(onoff, doitalways);
  }
  this->connectionsSetUp = onoff;
  return oldval;
}

SbVec3f TDragger::roundTranslation(const SbVec3f &vecIn, float incrementIn)
{
    //everything is transformed into local space. That means we only have
    //worry about the y-value.

    int yCount = 0;
    float yValue = vecIn[1];

    if (fabs(yValue) > (incrementIn / 2.0))
    {
        yCount = static_cast<int>(yValue / incrementIn);
        float remainder = fmod(yValue, incrementIn);
        if (remainder >= (incrementIn / 2.0))
            yCount++;
    }

    translationIncrementCount.setValue(yCount);

    SbVec3f out;
    out[0] = 0;
    out[1] = static_cast<float>(yCount) * incrementIn;
    out[2] = 0.0;

    return out;
}


SO_KIT_SOURCE(TPlanarDragger)

void TPlanarDragger::initClass()
{
  SO_KIT_INIT_CLASS(TPlanarDragger, SoDragger, "Dragger");
}

TPlanarDragger::TPlanarDragger()
{
    SO_KIT_CONSTRUCTOR(TPlanarDragger);

    SO_KIT_ADD_CATALOG_ENTRY(planarTranslatorSwitch, SoSwitch, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(planarTranslator, SoSeparator, TRUE, planarTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(planarTranslatorActive, SoSeparator, TRUE, planarTranslatorSwitch, "", TRUE);

    if (SO_KIT_IS_FIRST_INSTANCE())
        buildFirstInstance();

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
    this->setPartAsDefault("planarTranslatorActive", "CSysDynamics_TPlanarDragger_TranslatorActive");

    SoSwitch *sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);

    this->addStartCallback(&TPlanarDragger::startCB);
    this->addMotionCallback(&TPlanarDragger::motionCB);
    this->addFinishCallback(&TPlanarDragger::finishCB);

    addValueChangedCallback(&TPlanarDragger::valueChangedCB);

    fieldSensor.setFunction(&TPlanarDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}

TPlanarDragger::~TPlanarDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&TPlanarDragger::startCB);
    this->removeMotionCallback(&TPlanarDragger::motionCB);
    this->removeFinishCallback(&TPlanarDragger::finishCB);
    removeValueChangedCallback(&TPlanarDragger::valueChangedCB);
}

void TPlanarDragger::buildFirstInstance()
{
    SoGroup *geometryGroup = buildGeometry();

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

SoGroup* TPlanarDragger::buildGeometry()
{
    auto root = new SoGroup();

    float cubeWidthHeight = 2.0;
    float cubeDepth = 0.1f;

    auto translation = new SoTranslation();
    translation->translation.setValue(cubeWidthHeight + 0.15, cubeWidthHeight + 0.15, 0.0);
    root->addChild(translation);

    auto pickStyle = new SoPickStyle();
    pickStyle->style.setValue(SoPickStyle::SHAPE);
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

void TPlanarDragger::startCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TPlanarDragger *>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void TPlanarDragger::motionCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TPlanarDragger *>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void TPlanarDragger::finishCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<TPlanarDragger *>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void TPlanarDragger::fieldSensorCB(void *f, SoSensor *)
{
    auto sudoThis = static_cast<TPlanarDragger *>(f);

    if(!f)
      return;

    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void TPlanarDragger::valueChangedCB(void *, SoDragger *d)
{
    auto sudoThis = dynamic_cast<TPlanarDragger *>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft

    //all this just to get the translation?
    SbVec3f trans, scaleDummy;
    SbRotation rotationDummy, scaleOrientationDummy;
    matrix.getTransform(trans, rotationDummy, scaleDummy, scaleOrientationDummy);

    sudoThis->fieldSensor.detach();
    if (sudoThis->translation.getValue() != trans)
        sudoThis->translation = trans;
    sudoThis->fieldSensor.attach(&sudoThis->translation);
}

void TPlanarDragger::dragStart()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);

    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());
    projector.setPlane(SbPlane(SbVec3f(0.0, 0.0, 0.0), SbVec3f(1.0, 0.0, 0.0), SbVec3f(0.0, 1.0, 0.0)));
    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());

    SbMatrix localToWorld = getLocalToWorldMatrix();
    localToWorld.multVecMatrix(hitPoint, hitPoint);
    setStartingPoint((hitPoint));

    translationIncrementXCount.setValue(0);
    translationIncrementYCount.setValue(0);
}

void TPlanarDragger::drag()
{
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());

    SbVec3f hitPoint = projector.project(getNormalizedLocaterPosition());
    SbVec3f startingPoint = getLocalStartingPoint();
    SbVec3f localMovement = hitPoint - startingPoint;

    //scale the increment to match local space.
    float scaledIncrement = static_cast<float>(translationIncrement.getValue()) / autoScaleResult.getValue();

    localMovement = roundTranslation(localMovement, scaledIncrement);
    //when the movement vector is null either the appendTranslation or
    //the setMotionMatrix doesn't work. either way it stops translating
    //back to its initial starting point.
    if (localMovement.equals(SbVec3f(0.0, 0.0, 0.0), 0.00001f))
    {
        setMotionMatrix(getStartMotionMatrix());
        //don't know why I need the following but if I don't have it
        //it won't return to original position.
        this->valueChanged();
    }
    else
        setMotionMatrix(appendTranslation(getStartMotionMatrix(), localMovement));

    Base::Quantity quantityX(
      static_cast<double>(translationIncrementXCount.getValue()) * translationIncrement.getValue(), Base::Unit::Length);
    Base::Quantity quantityY(
      static_cast<double>(translationIncrementYCount.getValue()) * translationIncrement.getValue(), Base::Unit::Length);

    QString message = QString::fromLatin1("%1 %2, %3")
            .arg(QObject::tr("Translation XY:"), quantityX.getUserString(), quantityY.getUserString());
    getMainWindow()->showMessage(message, 3000);
}

void TPlanarDragger::dragFinish()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "planarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);
}

SbBool TPlanarDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff)
      return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff)
  {
    inherited::setUpConnections(onoff, doitalways);
    TPlanarDragger::fieldSensorCB(this, nullptr);
    if (this->fieldSensor.getAttachedField() != &this->translation)
      this->fieldSensor.attach(&this->translation);
  }
  else
  {
    if (this->fieldSensor.getAttachedField())
      this->fieldSensor.detach();
    inherited::setUpConnections(onoff, doitalways);
  }
  this->connectionsSetUp = onoff;
  return oldval;
}

SbVec3f TPlanarDragger::roundTranslation(const SbVec3f &vecIn, float incrementIn)
{
    int xCount = 0;
    float xValue = vecIn[0];

    if (fabs(xValue) > (incrementIn / 2.0))
    {
        xCount = static_cast<int>(xValue / incrementIn);
        float remainder = fmod(xValue, incrementIn);
        if (remainder >= (incrementIn / 2.0))
            xCount++;
    }

    translationIncrementXCount.setValue(xCount);

    int yCount = 0;
    float yValue = vecIn[1];

    if (fabs(yValue) > (incrementIn / 2.0))
    {
        yCount = static_cast<int>(yValue / incrementIn);
        float remainder = fmod(yValue, incrementIn);
        if (remainder >= (incrementIn / 2.0))
            yCount++;
    }

    translationIncrementYCount.setValue(yCount);

    SbVec3f out;
    out[0] = static_cast<float>(xCount) * incrementIn;
    out[1] = static_cast<float>(yCount) * incrementIn;
    out[2] = 0.0;

    return out;
}


SO_KIT_SOURCE(RDragger)

void RDragger::initClass()
{
  SO_KIT_INIT_CLASS(RDragger, SoDragger, "Dragger");
}

RDragger::RDragger()
{
    SO_KIT_CONSTRUCTOR(RDragger);

    SO_KIT_ADD_CATALOG_ENTRY(rotatorSwitch, SoSwitch, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(rotator, SoSeparator, TRUE, rotatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(rotatorActive, SoSeparator, TRUE, rotatorSwitch, "", TRUE);

    arcRadius = 8.0;

    if (SO_KIT_IS_FIRST_INSTANCE())
        buildFirstInstance();

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (M_PI / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCount, (0));

    SO_KIT_INIT_INSTANCE();

    // initialize default parts.
    // first is from 'SO_KIT_CATALOG_ENTRY_HEADER' macro
    // second is unique name from buildFirstInstance().
    this->setPartAsDefault("rotator", "CSysDynamics_RDragger_Rotator");
    this->setPartAsDefault("rotatorActive", "CSysDynamics_RDragger_RotatorActive");

    SoSwitch *sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);

    this->addStartCallback(&RDragger::startCB);
    this->addMotionCallback(&RDragger::motionCB);
    this->addFinishCallback(&RDragger::finishCB);

    addValueChangedCallback(&RDragger::valueChangedCB);

    fieldSensor.setFunction(&RDragger::fieldSensorCB);
    fieldSensor.setData(this);
    fieldSensor.setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}

RDragger::~RDragger()
{
    fieldSensor.setData(nullptr);
    fieldSensor.detach();

    this->removeStartCallback(&RDragger::startCB);
    this->removeMotionCallback(&RDragger::motionCB);
    this->removeFinishCallback(&RDragger::finishCB);
    removeValueChangedCallback(&RDragger::valueChangedCB);
}

void RDragger::buildFirstInstance()
{
    SoGroup *geometryGroup = buildGeometry();

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

SoGroup* RDragger::buildGeometry()
{
    auto root = new SoGroup();

    //arc
    auto coordinates = new SoCoordinate3();

    unsigned int segments = 15;

    float angleIncrement = static_cast<float>(M_PI / 2.0) / static_cast<float>(segments);
    SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), angleIncrement);
    SbVec3f point(arcRadius, 0.0, 0.0);
    for (unsigned int index = 0; index <= segments; ++index)
    {
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
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    pickStyle->setOverride(TRUE);
    root->addChild(pickStyle);

    //sphere.
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

void RDragger::startCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<RDragger *>(d);
    assert(sudoThis);
    sudoThis->dragStart();
}

void RDragger::motionCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<RDragger *>(d);
    assert(sudoThis);
    sudoThis->drag();
}

void RDragger::finishCB(void *, SoDragger *d)
{
    auto sudoThis = static_cast<RDragger *>(d);
    assert(sudoThis);
    sudoThis->dragFinish();
}

void RDragger::fieldSensorCB(void *f, SoSensor *)
{
    auto sudoThis = static_cast<RDragger *>(f);

    if(!f)
      return;

    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void RDragger::valueChangedCB(void *, SoDragger *d)
{
    auto sudoThis = dynamic_cast<RDragger *>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft

    //all this just to get the translation?
    SbVec3f translationDummy, scaleDummy;
    SbRotation localRotation, scaleOrientationDummy;
    matrix.getTransform(translationDummy, localRotation, scaleDummy, scaleOrientationDummy);

    sudoThis->fieldSensor.detach();
    if (sudoThis->rotation.getValue() != localRotation)
        sudoThis->rotation = localRotation;
    sudoThis->fieldSensor.attach(&sudoThis->rotation);
}

void RDragger::dragStart()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);

    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());
    projector.setPlane(SbPlane(SbVec3f(0.0, 0.0, 1.0), 0.0));

    SbVec3f hitPoint;
    if (!projector.tryProject(getNormalizedLocaterPosition(), 0.0, hitPoint))
        return;
    hitPoint.normalize();

    SbMatrix localToWorld = getLocalToWorldMatrix();
    localToWorld.multVecMatrix(hitPoint, hitPoint);
    setStartingPoint((hitPoint));

    rotationIncrementCount.setValue(0);
}

void RDragger::drag()
{
    projector.setViewVolume(this->getViewVolume());
    projector.setWorkingSpace(this->getLocalToWorldMatrix());

    SbVec3f hitPoint;
    if (!projector.tryProject(getNormalizedLocaterPosition(), 0.0, hitPoint))
        return;
    hitPoint.normalize();

    SbVec3f startingPoint = getLocalStartingPoint();
    startingPoint.normalize();

    SbRotation localRotation(startingPoint, hitPoint);
    //getting some slop from this. grab vector and put it absolute.
    SbVec3f tempVec;
    float tempRadians;
    localRotation.getValue(tempVec, tempRadians);
    tempVec[0] = 0.0;
    tempVec[1] = 0.0;
    tempVec.normalize();
    if (tempVec[2] < 0.0)
    {
        tempRadians *= -1.0;
        tempVec.negate();
    }
    int incrementCount = roundIncrement(tempRadians);
    rotationIncrementCount.setValue(incrementCount);
    localRotation = SbRotation(tempVec, incrementCount * static_cast<float>(rotationIncrement.getValue()));

    //same problem as described in tDragger::drag.
    if (localRotation.equals(SbRotation(SbVec3f(0.0, 0.0, 1.0), 0.0), 0.00001f))
    {
        setMotionMatrix(getStartMotionMatrix());
        this->valueChanged();
    }
    else
        setMotionMatrix(appendRotation(getStartMotionMatrix(), localRotation, SbVec3f(0.0, 0.0, 0.0)));

    Base::Quantity quantity(
      static_cast<double>(rotationIncrementCount.getValue())  * (180.0 / M_PI) *
      rotationIncrement.getValue(), Base::Unit::Angle);

    QString message = QString::fromLatin1("%1 %2")
            .arg(QObject::tr("Rotation:"), quantity.getUserString());
    getMainWindow()->showMessage(message, 3000);
}

void RDragger::dragFinish()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 0);
}

SbBool RDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff)
      return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff)
  {
    inherited::setUpConnections(onoff, doitalways);
    RDragger::fieldSensorCB(this, nullptr);
    if (this->fieldSensor.getAttachedField() != &this->rotation)
      this->fieldSensor.attach(&this->rotation);
  }
  else
  {
    if (this->fieldSensor.getAttachedField())
      this->fieldSensor.detach();
    inherited::setUpConnections(onoff, doitalways);
  }
  this->connectionsSetUp = onoff;
  return oldval;
}

int RDragger::roundIncrement(const float &radiansIn)
{
    int rCount = 0;

    auto increment = static_cast<float>(rotationIncrement.getValue());
    if (fabs(radiansIn) > (increment / 2.0))
    {
        rCount = static_cast<int>(radiansIn / increment);
        float remainder = fmod(radiansIn, increment);
        if (remainder >= (increment / 2.0))
            rCount++;
    }

    return rCount;
}


SO_KIT_SOURCE(SoFCCSysDragger)

void SoFCCSysDragger::initClass()
{
    TDragger::initClass();
    TPlanarDragger::initClass();
    RDragger::initClass();
    SO_KIT_INIT_CLASS(SoFCCSysDragger, SoDragger, "Dragger");
}

SoFCCSysDragger::SoFCCSysDragger()
    : axisScale(1.0f,1.0f,1.0f)
{
    SO_KIT_CONSTRUCTOR(SoFCCSysDragger);

    SO_KIT_ADD_CATALOG_ENTRY(annotation, SoAnnotation, TRUE, geomSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(scaleNode, SoScale, TRUE, annotation, "", TRUE);

    // Translator

    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorSeparator, SoSeparator, TRUE, xTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorSeparator, SoSeparator, TRUE, yTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorSeparator, SoSeparator, TRUE, zTranslatorSwitch, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorColor, SoBaseColor, TRUE, xTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorColor, SoBaseColor, TRUE, yTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorColor, SoBaseColor, TRUE, zTranslatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorRotation, SoRotation, TRUE, xTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorRotation, SoRotation, TRUE, yTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorRotation, SoRotation, TRUE, zTranslatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorDragger, TDragger, TRUE, xTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorDragger, TDragger, TRUE, yTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorDragger, TDragger, TRUE, zTranslatorSeparator, "", TRUE);

    // Planar Translator

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorSeparator, SoSeparator, TRUE, xyPlanarTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorSeparator, SoSeparator, TRUE, yzPlanarTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorSeparator, SoSeparator, TRUE, zxPlanarTranslatorSwitch, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorColor, SoBaseColor, TRUE, xyPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorColor, SoBaseColor, TRUE, yzPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorColor, SoBaseColor, TRUE, zxPlanarTranslatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorRotation, SoRotation, TRUE, xyPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorRotation, SoRotation, TRUE, yzPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorRotation, SoRotation, TRUE, zxPlanarTranslatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xyPlanarTranslatorDragger, TPlanarDragger, TRUE, xyPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzPlanarTranslatorDragger, TPlanarDragger, TRUE, yzPlanarTranslatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zxPlanarTranslatorDragger, TPlanarDragger, TRUE, zxPlanarTranslatorSeparator, "", TRUE);

    // Rotator

    SO_KIT_ADD_CATALOG_ENTRY(xRotatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorSwitch, SoSwitch, TRUE, annotation, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xRotatorSeparator, SoSeparator, TRUE, xRotatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorSeparator, SoSeparator, TRUE, yRotatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorSeparator, SoSeparator, TRUE, zRotatorSwitch, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xRotatorColor, SoBaseColor, TRUE, xRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorColor, SoBaseColor, TRUE, yRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorColor, SoBaseColor, TRUE, zRotatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xRotatorRotation, SoRotation, TRUE, xRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorRotation, SoRotation, TRUE, yRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorRotation, SoRotation, TRUE, zRotatorSeparator, "", TRUE);

    SO_KIT_ADD_CATALOG_ENTRY(xRotatorDragger, RDragger, TRUE, xRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yRotatorDragger, RDragger, TRUE, yRotatorSeparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zRotatorDragger, RDragger, TRUE, zRotatorSeparator, "", TRUE);

    // Other

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(translationIncrement, (1.0));
    SO_KIT_ADD_FIELD(translationIncrementCountX, (0));
    SO_KIT_ADD_FIELD(translationIncrementCountY, (0));
    SO_KIT_ADD_FIELD(translationIncrementCountZ, (0));

    SO_KIT_ADD_FIELD(rotation, (SbVec3f(0.0, 0.0, 1.0), 0.0));
    SO_KIT_ADD_FIELD(rotationIncrement, (M_PI / 8.0));
    SO_KIT_ADD_FIELD(rotationIncrementCountX, (0));
    SO_KIT_ADD_FIELD(rotationIncrementCountY, (0));
    SO_KIT_ADD_FIELD(rotationIncrementCountZ, (0));

    SO_KIT_ADD_FIELD(draggerSize, (1.0));
    SO_KIT_ADD_FIELD(autoScaleResult, (1.0));

    SO_KIT_INIT_INSTANCE();

    // Colors
    setAxisColors(
      SbColor(1.0, 0, 0).getPackedValue(0.0f),
      SbColor(0, 1.0, 0).getPackedValue(0.0f),
      SbColor(0, 0, 1.0).getPackedValue(0.0f)
    );

    // Increments

    // Translator
    TDragger *tDragger;
    tDragger = SO_GET_ANY_PART(this, "xTranslatorDragger", TDragger);
    tDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountX.connectFrom(&tDragger->translationIncrementCount);
    tDragger = SO_GET_ANY_PART(this, "yTranslatorDragger", TDragger);
    tDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountY.connectFrom(&tDragger->translationIncrementCount);
    tDragger = SO_GET_ANY_PART(this, "zTranslatorDragger", TDragger);
    tDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountZ.connectFrom(&tDragger->translationIncrementCount);
    // Planar Translator
    TPlanarDragger *tPlanarDragger;
    tPlanarDragger = SO_GET_ANY_PART(this, "xyPlanarTranslatorDragger", TPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountX.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountY.appendConnection(&tPlanarDragger->translationIncrementYCount);
    tPlanarDragger = SO_GET_ANY_PART(this, "yzPlanarTranslatorDragger", TPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountZ.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountY.appendConnection(&tPlanarDragger->translationIncrementYCount);
    tPlanarDragger = SO_GET_ANY_PART(this, "zxPlanarTranslatorDragger", TPlanarDragger);
    tPlanarDragger->translationIncrement.connectFrom(&this->translationIncrement);
    tPlanarDragger->autoScaleResult.connectFrom(&this->autoScaleResult);
    translationIncrementCountX.appendConnection(&tPlanarDragger->translationIncrementXCount);
    translationIncrementCountZ.appendConnection(&tPlanarDragger->translationIncrementYCount);
    // Rotator
    RDragger *rDragger;
    rDragger = SO_GET_ANY_PART(this, "xRotatorDragger", RDragger);
    rDragger->rotationIncrement.connectFrom(&this->rotationIncrement);
    rotationIncrementCountX.connectFrom(&rDragger->rotationIncrementCount);
    rDragger = SO_GET_ANY_PART(this, "yRotatorDragger", RDragger);
    rDragger->rotationIncrement.connectFrom(&this->rotationIncrement);
    rotationIncrementCountY.connectFrom(&rDragger->rotationIncrementCount);
    rDragger = SO_GET_ANY_PART(this, "zRotatorDragger", RDragger);
    rDragger->rotationIncrement.connectFrom(&this->rotationIncrement);
    rotationIncrementCountZ.connectFrom(&rDragger->rotationIncrementCount);

    // Switches

    // Translator
    SoSwitch *sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    // Planar Translator
    sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    // Rotator
    sw = SO_GET_ANY_PART(this, "xRotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "yRotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
    sw = SO_GET_ANY_PART(this, "zRotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

    // Rotations

    SoRotation *localRotation;
    SbRotation tempRotation;
    auto angle = static_cast<float>(M_PI / 2.0);
    // Translator
    localRotation = SO_GET_ANY_PART(this, "xTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, 0.0, -1.0), angle);
    localRotation = SO_GET_ANY_PART(this, "yTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "zTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(1.0, 0.0, 0.0), angle);
    // Planar Translator
    localRotation = SO_GET_ANY_PART(this, "xyPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());
    localRotation = SO_GET_ANY_PART(this, "yzPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(0.0, -1.0, 0.0), angle);
    localRotation = SO_GET_ANY_PART(this, "zxPlanarTranslatorRotation", SoRotation);
    localRotation->rotation.setValue(SbVec3f(1.0, 0.0, 0.0), angle);
    // Rotator
    localRotation = SO_GET_ANY_PART(this, "xRotatorRotation", SoRotation);
    tempRotation = SbRotation(SbVec3f(1.0, 0.0, 0.0), angle);
    tempRotation *= SbRotation(SbVec3f(0.0, 0.0, 1.0), angle);
    localRotation->rotation.setValue(tempRotation);
    localRotation = SO_GET_ANY_PART(this, "yRotatorRotation", SoRotation);
    tempRotation = SbRotation(SbVec3f(0.0, -1.0, 0.0), angle);
    tempRotation *= SbRotation(SbVec3f(0.0, 0.0, -1.0), angle);
    localRotation->rotation.setValue(tempRotation);
    localRotation = SO_GET_ANY_PART(this, "zRotatorRotation", SoRotation);
    localRotation->rotation.setValue(SbRotation::identity());

    //this is for non-autoscale mode. this will be disconnected for autoscale
    //and won't be used. see setUpAutoScale.
    auto scaleEngine = new SoComposeVec3f(); //uses coin ref scheme.
    scaleEngine->x.connectFrom(&draggerSize);
    scaleEngine->y.connectFrom(&draggerSize);
    scaleEngine->z.connectFrom(&draggerSize);
    SoScale *localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
    localScaleNode->scaleFactor.connectFrom(&scaleEngine->vector);
    autoScaleResult.connectFrom(&draggerSize);

    addValueChangedCallback(&SoFCCSysDragger::valueChangedCB);

    translationSensor.setFunction(&SoFCCSysDragger::translationSensorCB);
    translationSensor.setData(this);
    translationSensor.setPriority(0);

    rotationSensor.setFunction(&SoFCCSysDragger::rotationSensorCB);
    rotationSensor.setData(this);
    rotationSensor.setPriority(0);

    cameraSensor.setFunction(&SoFCCSysDragger::cameraCB);
    cameraSensor.setData(this);

    idleSensor.setFunction(&SoFCCSysDragger::idleCB);
    idleSensor.setData(this);

    this->addFinishCallback(&SoFCCSysDragger::finishDragCB, this);

    this->setUpConnections(TRUE, TRUE);
}

SoFCCSysDragger::~SoFCCSysDragger()
{
    translationSensor.setData(nullptr);
    translationSensor.detach();
    rotationSensor.setData(nullptr);
    rotationSensor.detach();
    cameraSensor.setData(nullptr);
    cameraSensor.detach();
    idleSensor.setData(nullptr);
    idleSensor.unschedule();

    removeValueChangedCallback(&SoFCCSysDragger::valueChangedCB);
    removeFinishCallback(&SoFCCSysDragger::finishDragCB, this);
}


SbBool SoFCCSysDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if (!doitalways && (connectionsSetUp == onoff))
        return onoff;

    TDragger *tDraggerX = SO_GET_ANY_PART(this, "xTranslatorDragger", TDragger);
    TDragger *tDraggerY = SO_GET_ANY_PART(this, "yTranslatorDragger", TDragger);
    TDragger *tDraggerZ = SO_GET_ANY_PART(this, "zTranslatorDragger", TDragger);
    TPlanarDragger *tPlanarDraggerXZ = SO_GET_ANY_PART(this, "xyPlanarTranslatorDragger", TPlanarDragger);
    TPlanarDragger *tPlanarDraggerYZ = SO_GET_ANY_PART(this, "yzPlanarTranslatorDragger", TPlanarDragger);
    TPlanarDragger *tPlanarDraggerZX = SO_GET_ANY_PART(this, "zxPlanarTranslatorDragger", TPlanarDragger);
    RDragger *rDraggerX = SO_GET_ANY_PART(this, "xRotatorDragger", RDragger);
    RDragger *rDraggerY = SO_GET_ANY_PART(this, "yRotatorDragger", RDragger);
    RDragger *rDraggerZ = SO_GET_ANY_PART(this, "zRotatorDragger", RDragger);

    if (onoff)
    {
        inherited::setUpConnections(onoff, doitalways);

        registerChildDragger(tDraggerX);
        registerChildDragger(tDraggerY);
        registerChildDragger(tDraggerZ);
        registerChildDragger(tPlanarDraggerXZ);
        registerChildDragger(tPlanarDraggerYZ);
        registerChildDragger(tPlanarDraggerZX);
        registerChildDragger(rDraggerX);
        registerChildDragger(rDraggerY);
        registerChildDragger(rDraggerZ);

        translationSensorCB(this, nullptr);
        if (this->translationSensor.getAttachedField() != &this->translation)
          this->translationSensor.attach(&this->translation);

        rotationSensorCB(this, nullptr);
        if (this->rotationSensor.getAttachedField() != &this->rotation)
            this->rotationSensor.attach(&this->rotation);
    }
    else
    {
        unregisterChildDragger(tDraggerX);
        unregisterChildDragger(tDraggerY);
        unregisterChildDragger(tDraggerZ);
        unregisterChildDragger(tPlanarDraggerXZ);
        unregisterChildDragger(tPlanarDraggerYZ);
        unregisterChildDragger(tPlanarDraggerZX);
        unregisterChildDragger(rDraggerX);
        unregisterChildDragger(rDraggerY);
        unregisterChildDragger(rDraggerZ);

        inherited::setUpConnections(onoff, doitalways);

        if (this->translationSensor.getAttachedField())
          this->translationSensor.detach();

        if (this->rotationSensor.getAttachedField())
            this->rotationSensor.detach();
    }
    return !(this->connectionsSetUp = onoff);
}

void SoFCCSysDragger::translationSensorCB(void *f, SoSensor *)
{
    auto sudoThis = static_cast<SoFCCSysDragger *>(f);
    if(!f)
      return;

    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoFCCSysDragger::rotationSensorCB(void *f, SoSensor *)
{
    auto sudoThis = static_cast<SoFCCSysDragger *>(f);
    if(!f)
      return;

    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft
    sudoThis->workFieldsIntoTransform(matrix);
    sudoThis->setMotionMatrix(matrix);
}

void SoFCCSysDragger::valueChangedCB(void *, SoDragger *d)
{
    auto sudoThis = dynamic_cast<SoFCCSysDragger *>(d);
    assert(sudoThis);
    SbMatrix matrix = sudoThis->getMotionMatrix(); // clazy:exclude=rule-of-two-soft

    //all this just to get the translation?
    SbVec3f localTranslation, scaleDummy;
    SbRotation localRotation, scaleOrientationDummy;
    matrix.getTransform(localTranslation, localRotation, scaleDummy, scaleOrientationDummy);

    sudoThis->translationSensor.detach();
    if (sudoThis->translation.getValue() != localTranslation)
        sudoThis->translation = localTranslation;
    sudoThis->translationSensor.attach(&sudoThis->translation);

    sudoThis->rotationSensor.detach();
    if (sudoThis->rotation.getValue() != localRotation)
        sudoThis->rotation = localRotation;
    sudoThis->rotationSensor.attach(&sudoThis->rotation);
}

void SoFCCSysDragger::setUpAutoScale(SoCamera *cameraIn)
{
    //note: sofieldsensor checks if the current sensor is already attached
    //and takes appropriate action. So it is safe to attach to a field without
    //checking current attachment state.
    if (cameraIn->getTypeId() == SoOrthographicCamera::getClassTypeId())
    {
        auto localCamera = dynamic_cast<SoOrthographicCamera *>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->height);
        SoScale *localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
        localScaleNode->scaleFactor.disconnect();
        autoScaleResult.disconnect(&draggerSize);
        cameraCB(this, nullptr);
    }
    else if (cameraIn->getTypeId() == SoPerspectiveCamera::getClassTypeId())
    {
        auto localCamera = dynamic_cast<SoPerspectiveCamera *>(cameraIn);
        assert(localCamera);
        cameraSensor.attach(&localCamera->position);
        SoScale *localScaleNode = SO_GET_ANY_PART(this, "scaleNode", SoScale);
        localScaleNode->scaleFactor.disconnect();
        autoScaleResult.disconnect(&draggerSize);
        cameraCB(this, nullptr);
    }
}

void SoFCCSysDragger::cameraCB(void *data, SoSensor *)
{
    auto sudoThis = static_cast<SoFCCSysDragger *>(data);
    if (!sudoThis)
      return;
    if (!sudoThis->idleSensor.isScheduled())
        sudoThis->idleSensor.schedule();
}

void SoFCCSysDragger::GLRender(SoGLRenderAction * action)
{
    if(!scaleInited) {
        scaleInited = true;
        updateDraggerCache(action->getCurPath());
        updateAxisScale();
    }

    inherited::GLRender(action);
}

void SoFCCSysDragger::updateAxisScale() {
    SbMatrix localToWorld = getLocalToWorldMatrix();
    SbVec3f origin;
    localToWorld.multVecMatrix(SbVec3f(0.0, 0.0, 0.0), origin);
    SbVec3f vx,vy,vz;
    localToWorld.multVecMatrix(SbVec3f(1.0f, 0.0f, 0.0f), vx);
    localToWorld.multVecMatrix(SbVec3f(0.0f, 1.0f, 0.0f), vy);
    localToWorld.multVecMatrix(SbVec3f(0.0f, 0.0f, 1.0f), vz);
    float x = std::max((vx-origin).length(),1e-7f);
    float y = std::max((vy-origin).length(),1e-7f);
    float z = std::max((vz-origin).length(),1e-7f);
    if(!axisScale.equals(SbVec3f(x,y,z),1e-7f)) {
        axisScale.setValue(x,y,z);
        idleCB(this,&idleSensor);
    }
}

void SoFCCSysDragger::handleEvent(SoHandleEventAction * action)
{
    this->ref();

    inherited::handleEvent(action);
    updateAxisScale();

    this->unref();
}

void SoFCCSysDragger::idleCB(void *data, SoSensor *)
{
    auto sudoThis = static_cast<SoFCCSysDragger *>(data);
    if (!data)
      return;
    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field)
    {
        auto camera = static_cast<SoCamera*>(field->getContainer());
        SbMatrix localToWorld = sudoThis->getLocalToWorldMatrix();
        SbVec3f origin;
        localToWorld.multVecMatrix(SbVec3f(0.0, 0.0, 0.0), origin);

        SbViewVolume viewVolume = camera->getViewVolume();
        float radius = sudoThis->draggerSize.getValue() / 2.0;
        float localScale = viewVolume.getWorldToScreenScale(origin, radius);
        float sx,sy,sz;
        sudoThis->axisScale.getValue(sx,sy,sz);
        SbVec3f scaleVector(localScale/sx, localScale/sy, localScale/sz);
        SoScale *localScaleNode = SO_GET_ANY_PART(sudoThis, "scaleNode", SoScale);
        localScaleNode->scaleFactor.setValue(scaleVector);
        sudoThis->autoScaleResult.setValue(localScale);
    }
}

void SoFCCSysDragger::finishDragCB(void *data, SoDragger *)
{
    auto sudoThis = static_cast<SoFCCSysDragger *>(data);
    assert(sudoThis);

    // note: when creating a second view of the document and then closing
    // the first viewer it deletes the camera. However, the attached field
    // of the cameraSensor will be detached automatically.
    SoField* field = sudoThis->cameraSensor.getAttachedField();
    if (field)
    {
        auto camera = static_cast<SoCamera*>(field->getContainer());
        if (camera->getTypeId() == SoPerspectiveCamera::getClassTypeId())
            cameraCB(sudoThis, nullptr);
    }
}

void SoFCCSysDragger::clearIncrementCounts()
{
    translationIncrementCountX.setValue(0);
    translationIncrementCountY.setValue(0);
    translationIncrementCountZ.setValue(0);
    rotationIncrementCountX.setValue(0);
    rotationIncrementCountY.setValue(0);
    rotationIncrementCountZ.setValue(0);
}

void SoFCCSysDragger::setAxisColors(unsigned long x, unsigned long y, unsigned long z) {
    SbColor colorX;
    SbColor colorY;
    SbColor colorZ;

    float t = 0.0f;
    colorX.setPackedValue(x, t);
    colorY.setPackedValue(y, t);
    colorZ.setPackedValue(z, t);

    SoBaseColor* color;

    // Translator
    color = SO_GET_ANY_PART(this, "xTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorX[0], colorX[1], colorX[2]);
    color = SO_GET_ANY_PART(this, "yTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorY[0], colorY[1], colorY[2]);
    color = SO_GET_ANY_PART(this, "zTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorZ[0], colorZ[1], colorZ[2]);
    // Planar Translator
    color = SO_GET_ANY_PART(this, "xyPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorZ[0], colorZ[1], colorZ[2]);
    color = SO_GET_ANY_PART(this, "yzPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorX[0], colorX[1], colorX[2]);
    color = SO_GET_ANY_PART(this, "zxPlanarTranslatorColor", SoBaseColor);
    color->rgb.setValue(colorY[0], colorY[1], colorY[2]);
    // Rotator
    color = SO_GET_ANY_PART(this, "xRotatorColor", SoBaseColor);
    color->rgb.setValue(colorX[0], colorX[1], colorX[2]);
    color = SO_GET_ANY_PART(this, "yRotatorColor", SoBaseColor);
    color->rgb.setValue(colorY[0], colorY[1], colorY[2]);
    color = SO_GET_ANY_PART(this, "zRotatorColor", SoBaseColor);
    color->rgb.setValue(colorZ[0], colorZ[1], colorZ[2]);
}

// Visibility API Functions

// Translator
void SoFCCSysDragger::showTranslationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showTranslationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showTranslationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}

void SoFCCSysDragger::hideTranslationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hideTranslationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hideTranslationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}

bool SoFCCSysDragger::isShownTranslationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownTranslationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownTranslationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}

bool SoFCCSysDragger::isHiddenTranslationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenTranslationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenTranslationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}

// Planar Translator
void SoFCCSysDragger::showPlanarTranslationXY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showPlanarTranslationYZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showPlanarTranslationZX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}

void SoFCCSysDragger::hidePlanarTranslationXY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hidePlanarTranslationYZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hidePlanarTranslationZX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}

bool SoFCCSysDragger::isShownPlanarTranslationXY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownPlanarTranslationYZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownPlanarTranslationZX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}

bool SoFCCSysDragger::isHiddenPlanarTranslationXY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xyPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenPlanarTranslationYZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yzPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenPlanarTranslationZX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zxPlanarTranslatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}

// Rotator
void SoFCCSysDragger::showRotationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showRotationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}
void SoFCCSysDragger::showRotationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
}

void SoFCCSysDragger::hideRotationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hideRotationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}
void SoFCCSysDragger::hideRotationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zRotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}

bool SoFCCSysDragger::isShownRotationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownRotationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}
bool SoFCCSysDragger::isShownRotationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_ALL);
}

bool SoFCCSysDragger::isHiddenRotationX()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "xRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenRotationY()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "yRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
bool SoFCCSysDragger::isHiddenRotationZ()
{
  SoSwitch *sw = SO_GET_ANY_PART(this, "zRotatorSwitch", SoSwitch);
  return (sw->whichChild.getValue() == SO_SWITCH_NONE);
}
