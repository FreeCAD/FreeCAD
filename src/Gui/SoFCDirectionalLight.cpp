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

#include <Inventor/draggers/SoDirectionalLightDragger.h>
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

#include "SoFCDirectionalLight.h"
#include "InventorBase.h"

using namespace Gui;

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

class SoFCDirectionalLightP {
public:
  SoFCDirectionalLight *master;
  CoinPtr<SoSwitch> draggerSwitch;
  CoinPtr<SoAnnotation> sep;
  CoinPtr<SoDragger> dragger;

  SoFCDirectionalLightP(SoFCDirectionalLight *p)
    :master(p)
  {
    draggerSwitch = new SoSwitch;
    sep = new SoAnnotation;
    draggerSwitch->addChild(sep);
  }
};

SO_NODE_SOURCE(SoFCDirectionalLight);

// documented in superclass
void
SoFCDirectionalLight::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCDirectionalLight, SoShadowDirectionalLight, "FCDirectionalLight");
}

SoFCDirectionalLight::SoFCDirectionalLight(void)
{
  SO_NODE_CONSTRUCTOR(SoFCDirectionalLight);
  SO_NODE_ADD_FIELD(showDragger, (FALSE));

  this->pimpl = new SoFCDirectionalLightP(this);

  this->children = new SoChildList(this);
  this->children->append(PRIVATE(this)->draggerSwitch);
  this->directionFieldSensor = new SoFieldSensor(SoFCDirectionalLight::fieldSensorCB, this);
  this->directionFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoFCDirectionalLight::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->attachSensors(TRUE);
  this->setDragger(new SoFCDirectionalLightDragger);
}

SoFCDirectionalLight::~SoFCDirectionalLight()
{
  this->setDragger(NULL);

  delete this->colorFieldSensor;
  delete this->directionFieldSensor;
  delete this->children;
  delete this->pimpl;
}

void
SoFCDirectionalLight::notify(SoNotList * nl)
{
  if (nl->getLastField() == &this->showDragger)
    PRIVATE(this)->draggerSwitch->whichChild = this->showDragger.getValue() ? 0 : -1;
  inherited::notify(nl);
}

void
SoFCDirectionalLight::setDragger(SoDragger * newdragger)
{
  SoDragger * olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoFCDirectionalLight::valueChangedCB, this);
    PRIVATE(this)->sep->removeChild(olddragger);
    PRIVATE(this)->dragger.reset();
  }
  if (newdragger != NULL) {
    PRIVATE(this)->dragger = newdragger;
    PRIVATE(this)->sep->addChild(newdragger);
    SoFCDirectionalLight::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoFCDirectionalLight::valueChangedCB, this);
  }
}

SoDragger *
SoFCDirectionalLight::getDragger(void)
{
  return PRIVATE(this)->dragger;
}

void
SoFCDirectionalLight::doAction(SoAction * action)
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
SoFCDirectionalLight::callback(SoCallbackAction * action)
{
  SoFCDirectionalLight::doAction(action);
  inherited::callback(action);
}

void
SoFCDirectionalLight::GLRender(SoGLRenderAction * action)
{
  SoFCDirectionalLight::doAction(action);
  inherited::GLRender(action);
}

void
SoFCDirectionalLight::getBoundingBox(SoGetBoundingBoxAction * action)
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

void
SoFCDirectionalLight::getMatrix(SoGetMatrixAction * action)
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
SoFCDirectionalLight::handleEvent(SoHandleEventAction * action)
{
  SoFCDirectionalLight::doAction(action);
  inherited::handleEvent(action);
}

void
SoFCDirectionalLight::pick(SoPickAction * action)
{
  SoFCDirectionalLight::doAction(action);
  inherited::pick(action);
}

void
SoFCDirectionalLight::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoFCDirectionalLight::doAction(action);
}

SoChildList *
SoFCDirectionalLight::getChildren(void) const
{
  return this->children;
}

void
SoFCDirectionalLight::valueChangedCB(void * m, SoDragger * dragger)
{
  SoFCDirectionalLight * thisp = (SoFCDirectionalLight *)m;

  SbMatrix matrix = dragger->getMotionMatrix();
  SbVec3f direction(0.0f, 0.0f, -1.0f);
  matrix.multDirMatrix(direction, direction);
  if (direction.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::post("SoFCDirectionalLight::valueChangedCB",
                       "Invalid motion matrix.");
#endif // debug
  }
  thisp->attachSensors(FALSE);
  if (thisp->direction.getValue() != direction) {
    thisp->direction = direction;
  }
  thisp->attachSensors(TRUE);
}

void
SoFCDirectionalLight::fieldSensorCB(void * m, SoSensor *)
{
  SoFCDirectionalLight * thisp = (SoFCDirectionalLight *)m;
  SoDragger * dragger = thisp->getDragger();
  if (dragger != NULL) {
    SbVec3f direction = thisp->direction.getValue();
    SbMatrix matrix = dragger->getMotionMatrix();
    SbVec3f t, s;
    SbRotation r, so;
    matrix.getTransform(t, r, s, so);
    r.setValue(SbVec3f(0.0f, 0.0f, -1.0f), direction);
    matrix.setTransform(t, r, s, so);
    dragger->setMotionMatrix(matrix);

    SoMaterial * material = (SoMaterial *)dragger->getPart("material", TRUE);
    if (material->emissiveColor.getNum() != 1 ||
        material->emissiveColor[0].getValue() != thisp->color.getValue()) {
      // replace with new material since the material is reused between
      // all draggers.
      material = new SoMaterial;
      material->diffuseColor = SbColor(0.0f, 0.0f, 0.0f);
      material->emissiveColor = thisp->color.getValue();
      dragger->setPart("material", material);
    }
  }
}

void
SoFCDirectionalLight::copyContents(const SoFieldContainer * fromfc, SbBool copyconnections)
{
  assert(fromfc->isOfType(SoFCDirectionalLight::getClassTypeId()));
  this->setDragger(((SoFCDirectionalLight *)fromfc)->getDragger());
  inherited::copyContents(fromfc, copyconnections);
}

void
SoFCDirectionalLight::transferFieldValues(const SoDirectionalLight * from, SoDirectionalLight * to)
{
  to->direction = from->direction;
  to->color = from->color;
}

void
SoFCDirectionalLight::attachSensors(const SbBool onoff)
{
  if (onoff) {
    this->directionFieldSensor->attach(&this->direction);
    this->colorFieldSensor->attach(&this->color);
  }
  else {
    this->directionFieldSensor->detach();
    this->colorFieldSensor->detach();
  }
}


SO_NODE_SOURCE(SoFCDirectionalLightDragger);

void
SoFCDirectionalLightDragger::initClass()
{
  SO_NODE_INIT_CLASS(SoFCDirectionalLightDragger, SoDirectionalLightDragger, "FCDirectionalLightDragger");
}

SoFCDirectionalLightDragger::SoFCDirectionalLightDragger()
{
  SO_NODE_CONSTRUCTOR(SoFCDirectionalLightDragger);
  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));
}

void
SoFCDirectionalLightDragger::notify(SoNotList * nl)
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
SoFCDirectionalLightDragger::doAction(SoAction * action)
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
SoFCDirectionalLightDragger::GLRender(SoGLRenderAction * action)
{
  SoFCDirectionalLightDragger::doAction(action);
  inherited::GLRender(action);
}

void
SoFCDirectionalLightDragger::pick(SoPickAction * action)
{
  SoFCDirectionalLightDragger::doAction(action);
  inherited::pick(action);
}

// vim: noai:ts=2:sw=2
