/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/
#include "PreCompiled.h"

#include <Inventor/draggers/SoSpotLightDragger.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/SoNodeKitPath.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include "SoFCSpotLight.h"
#include "InventorBase.h"

using namespace Gui;

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

class SoFCSpotLightP {
public:
  SoFCSpotLight *master;
  CoinPtr<SoSwitch> draggerSwitch;
  CoinPtr<SoAnnotation> sep;
  CoinPtr<SoDragger> dragger;

  SoFCSpotLightP(SoFCSpotLight *p)
    :master(p)
  {
    draggerSwitch = new SoSwitch;
    sep = new SoAnnotation;
    draggerSwitch->addChild(sep);
  }
};

SO_NODE_SOURCE(SoFCSpotLight);

// documented in superclass
void
SoFCSpotLight::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCSpotLight, SoSpotLight, "FCSpotLight");
}

SoFCSpotLight::SoFCSpotLight(void)
{
  SO_NODE_CONSTRUCTOR(SoFCSpotLight);
  SO_NODE_ADD_FIELD(showDragger, (FALSE));

  this->pimpl = new SoFCSpotLightP(this);

  this->children = new SoChildList(this);
  this->children->append(PRIVATE(this)->draggerSwitch);

  this->locationFieldSensor = new SoFieldSensor(SoFCSpotLight::fieldSensorCB, this);
  this->locationFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoFCSpotLight::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoFCSpotLight::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->directionFieldSensor = new SoFieldSensor(SoFCSpotLight::fieldSensorCB, this);
  this->directionFieldSensor->setPriority(0);

  this->angleFieldSensor = new SoFieldSensor(SoFCSpotLight::fieldSensorCB, this);
  this->angleFieldSensor->setPriority(0);

  this->attachSensors(TRUE);
  this->setDragger(new SoFCSpotLightDragger);
}

SoFCSpotLight::~SoFCSpotLight()
{
  this->setDragger(NULL);

  delete this->colorFieldSensor;
  delete this->locationFieldSensor;
  delete this->directionFieldSensor;
  delete this->angleFieldSensor;

  delete this->children;
  delete this->pimpl;
}

void
SoFCSpotLight::notify(SoNotList * nl)
{
  if (nl->getLastField() == &this->showDragger)
    PRIVATE(this)->draggerSwitch->whichChild = this->showDragger.getValue() ? 0 : -1;
  inherited::notify(nl);
}

void
SoFCSpotLight::setDragger(SoDragger * newdragger)
{
  SoDragger * olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoFCSpotLight::valueChangedCB, this);
    PRIVATE(this)->sep->removeChild(olddragger);
    PRIVATE(this)->dragger.reset();
  }
  if (newdragger != NULL) {
    PRIVATE(this)->dragger = newdragger;
    PRIVATE(this)->sep->addChild(newdragger);
    SoFCSpotLight::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoFCSpotLight::valueChangedCB, this);
  }
}

SoDragger *
SoFCSpotLight::getDragger(void)
{
  return PRIVATE(this)->dragger;
}

void
SoFCSpotLight::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
  else {
    this->children->traverse(action);
  }
}

void
SoFCSpotLight::callback(SoCallbackAction * action)
{
  SoFCSpotLight::doAction(action);
  inherited::callback(action);
}

void
SoFCSpotLight::GLRender(SoGLRenderAction * action)
{
  SoFCSpotLight::doAction(action);
  inherited::GLRender(action);
}

void
SoFCSpotLight::getBoundingBox(SoGetBoundingBoxAction * action)
{
  int numindices;
  const int * indices;
  int lastchild;
  SbVec3f center(0.0f, 0.0f, 0.0f);
  int numcenters = 0;

  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    lastchild  = indices[numindices-1];
  }
  else {
    lastchild = this->children->getLength() - 1;
  }
  for (int i = 0; i <= lastchild; i++) {
    this->children->traverse(action, i, i);
    if (action->isCenterSet()) {
      center += action->getCenter();
      numcenters++;
      action->resetCenter();
    }
  }
  inherited::getBoundingBox(action);
  if (action->isCenterSet()) {
    center += action->getCenter();
    numcenters++;
    action->resetCenter();
  }
  if (numcenters != 0) {
    action->setCenter(center / (float) numcenters, FALSE);
  }
}

// documented in superclass
void
SoFCSpotLight::getMatrix(SoGetMatrixAction * action)
{
  int numindices;
  const int * indices;
  switch (action->getPathCode(numindices, indices)) {
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    break;
  case SoAction::IN_PATH:
    this->children->traverseInPath(action, numindices, indices);
    break;
  case SoAction::OFF_PATH:
    this->children->traverse(action);
    break;
  default:
    assert(0 && "unknown path code");
    break;
  }
}

void
SoFCSpotLight::handleEvent(SoHandleEventAction * action)
{
  SoFCSpotLight::doAction(action);
  inherited::handleEvent(action);
}

void
SoFCSpotLight::pick(SoPickAction * action)
{
  SoFCSpotLight::doAction(action);
  inherited::pick(action);
}

void
SoFCSpotLight::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoFCSpotLight::doAction(action);
}

SoChildList *
SoFCSpotLight::getChildren(void) const
{
  return this->children;
}

void
SoFCSpotLight::valueChangedCB(void * m, SoDragger * dragger)
{
  SoFCSpotLight * thisp = (SoFCSpotLight *)m;

  SbMatrix matrix = dragger->getMotionMatrix();
  SbVec3f t, s;
  SbRotation r, so;
  matrix.getTransform(t, r, s, so);

  SbVec3f direction(0.0f, 0.0f, -1.0f);
  matrix.multDirMatrix(direction, direction);
  if (direction.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::post("SoFCSpotLight::valueChangedCB",
                       "Invalid motion matrix.");
#endif // debug
  }
  thisp->attachSensors(FALSE);
  if (thisp->location.getValue() != t) {
    thisp->location = t;
  }
  if (thisp->direction.getValue() != direction) {
    thisp->direction = direction;
  }
  if (dragger->isOfType(SoSpotLightDragger::getClassTypeId())) {
    if (thisp->cutOffAngle.getValue() != ((SoSpotLightDragger*)dragger)->angle.getValue()) {
      float angle = ((SoSpotLightDragger*)dragger)->angle.getValue();
      if (thisp->cutOffAngle.getValue() != angle) {
        thisp->cutOffAngle = angle;
      }
    }
  }
  thisp->attachSensors(TRUE);
}

void
SoFCSpotLight::fieldSensorCB(void * m, SoSensor *)
{
  SoFCSpotLight * thisp = (SoFCSpotLight *)m;
  SoDragger *dragger = thisp->getDragger();
  if (dragger != NULL) {
    float cutoffangle = thisp->cutOffAngle.getValue();
    SbVec3f direction = thisp->direction.getValue();
    SbMatrix matrix = dragger->getMotionMatrix();
    SbVec3f t, s;
    SbRotation r, so;
    matrix.getTransform(t, r, s, so);
    r.setValue(SbVec3f(0.0f, 0.0f, -1.0f), direction);
    t = thisp->location.getValue();

    matrix.setTransform(t, r, s, so);
    if (dragger->isOfType(SoSpotLightDragger::getClassTypeId()))
      ((SoSpotLightDragger*)dragger)->angle = thisp->cutOffAngle.getValue();
    dragger->setMotionMatrix(matrix);

    SoMaterial * material = (SoMaterial*)dragger->getPart("material", TRUE);
    if (material->emissiveColor.getNum() != 1 ||
        material->emissiveColor[0].getValue() != thisp->color.getValue()) {
      // replace with new material since the material is reused between
      // all draggers.
      material = new SoMaterial;
      material->diffuseColor = SbColor(0.0f, 0.0f, 0.0f);
      material->emissiveColor = thisp->color.getValue();
      dragger->setPart("material", material);
    }
    if (dragger->isOfType(SoSpotLightDragger::getClassTypeId())) {
      SoSpotLightDragger * sldragger = (SoSpotLightDragger*) dragger;
      if (sldragger->angle.getValue() != cutoffangle) {
        sldragger->angle = cutoffangle;
      }
    }
  }
}

void
SoFCSpotLight::copyContents(const SoFieldContainer * fromfc, SbBool copyconnections)
{
  assert(fromfc->isOfType(SoFCSpotLight::getClassTypeId()));
  this->setDragger(((SoFCSpotLight *)fromfc)->getDragger());
  inherited::copyContents(fromfc, copyconnections);
}

void
SoFCSpotLight::transferFieldValues(const SoSpotLight * from, SoSpotLight * to)
{
  to->location = from->location;
  to->color = from->color;
  to->direction = from->direction;
  to->cutOffAngle = from->cutOffAngle;
}

void
SoFCSpotLight::attachSensors(const SbBool onoff)
{
  if (onoff) {
    this->locationFieldSensor->attach(&this->location);
    this->colorFieldSensor->attach(&this->color);
    this->directionFieldSensor->attach(&this->direction);
    this->angleFieldSensor->attach(&this->cutOffAngle);
  }
  else {
    this->locationFieldSensor->detach();
    this->colorFieldSensor->detach();
    this->directionFieldSensor->detach();
    this->angleFieldSensor->detach();
  }
}

SO_NODE_SOURCE(SoFCSpotLightDragger);

void
SoFCSpotLightDragger::initClass()
{
  SO_NODE_INIT_CLASS(SoFCSpotLightDragger, SoSpotLightDragger, "FCSpotLightDragger");
}

SoFCSpotLightDragger::SoFCSpotLightDragger()
{
  SO_NODE_CONSTRUCTOR(SoFCSpotLightDragger);
  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));
}

void
SoFCSpotLightDragger::notify(SoNotList * nl)
{
  if (nl->getLastField() == &this->scaleFactor) {
    // Just borrow the translation sensor to be safe.
    // SoDirectionLightDragger::fieldSensorCB() actually do not use this
    // argument.
    fieldSensorCB(this, this->translFieldSensor);
  }
  inherited::notify(nl);
}

void
SoFCSpotLightDragger::doAction(SoAction * action)
{
  if (action->getState()->isElementEnabled(SoViewVolumeElement::getClassStackIndex())) {
    SbViewVolume vv = SoViewVolumeElement::get(action->getState());
    float aspectRatio = SoViewportRegionElement::get(action->getState()).getViewportAspectRatio();
    float scale = vv.getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f) / (5*aspectRatio);
    SbVec3f sf(scale, scale, scale);
    if (sf != this->scaleFactor.getValue())
      this->scaleFactor = sf;
  }
  inherited::doAction(action);
}

void
SoFCSpotLightDragger::GLRender(SoGLRenderAction * action)
{
  SoFCSpotLightDragger::doAction(action);
  inherited::GLRender(action);
}

void
SoFCSpotLightDragger::pick(SoPickAction * action)
{
  SoFCSpotLightDragger::doAction(action);
  inherited::pick(action);
}


// vim: noai:ts=2:sw=2
