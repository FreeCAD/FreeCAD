/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_MANIPULATORS

/*!
  \class SoSpotLightManip SoSpotLightManip.h Inventor/manips/SoSpotLightManip.h
  \brief The SoSpotLightManip class is used to manipulate spot light nodes.

  \ingroup coin_manips

  A manipulator is used by replacing the node you want to edit in the
  graph with the manipulator. Draggers are used to manipulate the
  node. When manipulation is finished, the node is put back into the
  graph, replacing the manipulator.

  <center>
  \image html spotlight.png "Example of SpotLight Manipulator"
  </center>
*/

/*!
  \var SoFieldSensor * SoSpotLightManip::locationFieldSensor
  \COININTERNAL
*/

/*!
  \var SoFieldSensor * SoSpotLightManip::directionFieldSensor
  \COININTERNAL
*/

/*!
  \var SoFieldSensor * SoSpotLightManip::angleFieldSensor
  \COININTERNAL
*/

/*!
  \var SoFieldSensor * SoSpotLightManip::colorFieldSensor
  \COININTERNAL
*/

/*!
  \var SoChildList * SoSpotLightManip::children
  \COININTERNAL
*/

#include <Inventor/manips/SoSpotLightManip.h>

#include <Inventor/draggers/SoSpotLightDragger.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/SoNodeKitPath.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"

class SoSpotLightManipP {
public:
};

SO_NODE_SOURCE(SoSpotLightManip);


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoSpotLightManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoSpotLightManip, SO_FROM_INVENTOR_1);
}

/*!
  The constructor sets up the internal SoSpotLightDragger used for
  manipulation.
 */
SoSpotLightManip::SoSpotLightManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoSpotLightManip);

  this->children = new SoChildList(this);

  this->locationFieldSensor = new SoFieldSensor(SoSpotLightManip::fieldSensorCB, this);
  this->locationFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoSpotLightManip::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->directionFieldSensor = new SoFieldSensor(SoSpotLightManip::fieldSensorCB, this);
  this->directionFieldSensor->setPriority(0);

  this->angleFieldSensor = new SoFieldSensor(SoSpotLightManip::fieldSensorCB, this);
  this->angleFieldSensor->setPriority(0);

  this->attachSensors(TRUE);
  this->setDragger(new SoSpotLightDragger);
}

/*!
  Destructor.
 */
SoSpotLightManip::~SoSpotLightManip()
{
  this->setDragger(NULL);

  delete this->colorFieldSensor;
  delete this->locationFieldSensor;
  delete this->directionFieldSensor;
  delete this->angleFieldSensor;

  delete this->children;
}

/*!
  Set dragger to use for user interaction.
 */
void
SoSpotLightManip::setDragger(SoDragger * newdragger)
{
  SoDragger *olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoSpotLightManip::valueChangedCB, this);
    this->children->remove(0);
  }
  if (newdragger != NULL) {
    if (this->children->getLength() > 0) {
      this->children->set(0, newdragger);
    }
    else {
      this->children->append(newdragger);
    }
    SoSpotLightManip::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoSpotLightManip::valueChangedCB, this);
  }
}

/*!
  Returns pointer to internal dragger.
 */
SoDragger *
SoSpotLightManip::getDragger(void)
{
  if (this->children->getLength() > 0) {
    SoNode *node = (*children)[0];
    if (node->isOfType(SoDragger::getClassTypeId()))
      return (SoDragger*)node;
    else {
#if COIN_DEBUG
      SoDebugError::post("SoSpotLightManip::getDragger",
                         "Child is not a dragger!");
#endif // debug
    }
  }
  return NULL;
}

/*!
  Replaces the node specified by \a path with this manipulator.
  The manipulator will copy the field data from the node, to make
  it affect the state in the same way as the node.
*/
SbBool
SoSpotLightManip::replaceNode(SoPath * path)
{
  SoFullPath *fullpath = (SoFullPath*)path;
  SoNode *fulltail = fullpath->getTail();
  if (!fulltail->isOfType(SoSpotLight::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoSpotLightManip::replaceNode",
                       "End of path is not an SoSpotLight");
#endif // debug
    return FALSE;
  }
  SoNode *tail = path->getTail();
  if (tail->isOfType(SoBaseKit::getClassTypeId())) {
    SoBaseKit *kit = (SoBaseKit*) ((SoNodeKitPath*)path)->getTail();
    SbString partname = kit->getPartString(path);
    if (partname != "") {
      SoSpotLight *oldpart = (SoSpotLight*) kit->getPart(partname, TRUE);
      if (oldpart != NULL) {
        this->attachSensors(FALSE);
        this->transferFieldValues(oldpart, this);
        this->attachSensors(TRUE);
        SoSpotLightManip::fieldSensorCB(this, NULL);
        kit->setPart(partname, this);
        return TRUE;
      }
      else {
        return FALSE;
      }
    }
  }
  if (fullpath->getLength() < 2) {
#if COIN_DEBUG
    SoDebugError::post("SoSpotLightManip::replaceNode",
                       "Path is too short");
#endif // debug
    return FALSE;
  }
  SoNode *parent = fullpath->getNodeFromTail(1);
  if (!parent->isOfType(SoGroup::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoSpotLightManip::replaceNode",
                       "Parent node is not a group");
#endif // debug
    return FALSE;
  }
  this->ref();
  this->attachSensors(FALSE);
  this->transferFieldValues((SoSpotLight*)fulltail, this);
  this->attachSensors(TRUE);
  SoSpotLightManip::fieldSensorCB(this, NULL);

  ((SoGroup*)parent)->replaceChild(fulltail, this);
  this->unrefNoDelete();
  return TRUE;
}

// Documented in superclass
void
SoSpotLightManip::doAction(SoAction * action)
{
  int numindices;
  const int *indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
  else {
    this->children->traverse(action);
  }
}

// Documented in superclass
void
SoSpotLightManip::callback(SoCallbackAction * action)
{
  SoSpotLightManip::doAction(action);
  SoSpotLight::callback(action);
}

// Documented in superclass
void
SoSpotLightManip::GLRender(SoGLRenderAction * action)
{
  SoSpotLightManip::doAction(action);
  SoSpotLight::GLRender(action);
}

// Documented in superclass
void
SoSpotLightManip::getBoundingBox(SoGetBoundingBoxAction * action)
{
  int numindices;
  const int *indices;
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
  SoSpotLight::getBoundingBox(action);
  if (action->isCenterSet()) {
    center += action->getCenter();
    numcenters++;
    action->resetCenter();
  }
  if (numcenters != 0) {
    action->setCenter(center / (float) numcenters, FALSE);
  }
}

// Documented in superclass
void
SoSpotLightManip::getMatrix(SoGetMatrixAction * action)
{
  int numindices;
  const int *indices;
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

// Documented in superclass
void
SoSpotLightManip::handleEvent(SoHandleEventAction * action)
{
  SoSpotLightManip::doAction(action);
  SoSpotLight::handleEvent(action);
}

// Documented in superclass
void
SoSpotLightManip::pick(SoPickAction * action)
{
  SoSpotLightManip::doAction(action);
  SoSpotLight::pick(action);
}

// Documented in superclass
void
SoSpotLightManip::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoSpotLightManip::doAction(action);
}

/*!
  \COININTERNAL
  Returns the children of this node. This node only has the dragger as
  a child.
*/
SoChildList *
SoSpotLightManip::getChildren(void) const
{
  return this->children;
}

/*!
  \COININTERNAL
  Updates the fields when the motion matrix changes.
*/
void
SoSpotLightManip::valueChangedCB(void * m, SoDragger * dragger)
{
  SoSpotLightManip * thisp = (SoSpotLightManip*)m;

  SbMatrix matrix = dragger->getMotionMatrix();
  SbVec3f t, s;
  SbRotation r, so;
  matrix.getTransform(t, r, s, so);

  SbVec3f direction(0.0f, 0.0f, -1.0f);
  matrix.multDirMatrix(direction, direction);
  if (direction.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::post("SoSpotLightManip::valueChangedCB",
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

/*!
  \COININTERNAL
  Updates the dragger whenever a field is modified.
*/
void
SoSpotLightManip::fieldSensorCB(void * m, SoSensor *)
{
  SoSpotLightManip *thisp = (SoSpotLightManip*)m;
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

// Documented in superclass. Overridden to copy the internal dragger
// instance.
void
SoSpotLightManip::copyContents(const SoFieldContainer * fromfc, SbBool copyconnections)
{
  assert(fromfc->isOfType(SoSpotLightManip::getClassTypeId()));
  SoDragger * dragger = ((SoSpotLightManip*)fromfc)->getDragger();
  this->setDragger(dragger ? (SoDragger*)dragger->copy() : NULL);
  inherited::copyContents(fromfc, copyconnections);
}

/*!
  \COININTERNAL
  Copies field values.
*/
void
SoSpotLightManip::transferFieldValues(const SoSpotLight * from, SoSpotLight * to)
{
  to->location = from->location;
  to->color = from->color;
  to->direction = from->direction;
  to->cutOffAngle = from->cutOffAngle;
}

void
SoSpotLightManip::attachSensors(const SbBool onoff)
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

#endif // HAVE_MANIPULATORS
