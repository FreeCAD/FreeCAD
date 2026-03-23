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
  \class SoDirectionalLightManip SoDirectionalLightManip.h Inventor/manips/SoDirectionalLightManip.h
  \brief The SoDirectionalLightManip class is used to manipulate SoDirectionalLight nodes.

  \ingroup coin_manips

  A manipulator is used by replacing the node you want to edit in the
  graph with the manipulator. Draggers are used to manipulate the
  node. When manipulation is finished, the node is put back into the
  graph, replacing the manipulator.

  <center>
  \image html directionallight.png "Example of DirectionalLight Manipulator"
  </center>
*/
// FIXME: improve the classdoc (snip from SoDirectionalLightDragger
// doc?). Usage code example. 20011023 mortene.

#include <Inventor/manips/SoDirectionalLightManip.h>

#include <Inventor/draggers/SoDirectionalLightDragger.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
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

class SoDirectionalLightManipP {
public:
};

SO_NODE_SOURCE(SoDirectionalLightManip);

/*!
  \var SoFieldSensor * SoDirectionalLightManip::directionFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoDirectionalLightManip::colorFieldSensor
  \COININTERNAL
*/
/*!
  \var SoChildList * SoDirectionalLightManip::children
  \COININTERNAL
*/


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoDirectionalLightManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoDirectionalLightManip, SO_FROM_INVENTOR_1);
}

/*!
  The constructor sets up the internal SoDirectionalLightDragger used
  by the manipulator.
*/
SoDirectionalLightManip::SoDirectionalLightManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoDirectionalLightManip);

  this->children = new SoChildList(this);
  this->directionFieldSensor = new SoFieldSensor(SoDirectionalLightManip::fieldSensorCB, this);
  this->directionFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoDirectionalLightManip::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->attachSensors(TRUE);
  this->setDragger(new SoDirectionalLightDragger);
}

/*!
  Destructor. Clean up resources used by manipulator.
*/
SoDirectionalLightManip::~SoDirectionalLightManip()
{
  this->setDragger(NULL);

  delete this->colorFieldSensor;
  delete this->directionFieldSensor;

  delete this->children;
}

/*!
  Set dragger to be used by this manipulator.
*/
void
SoDirectionalLightManip::setDragger(SoDragger * newdragger)
{
  SoDragger * olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoDirectionalLightManip::valueChangedCB, this);
    this->children->remove(0);
  }
  if (newdragger != NULL) {
    if (this->children->getLength() > 0) {
      this->children->set(0, newdragger);
    }
    else {
      this->children->append(newdragger);
    }
    SoDirectionalLightManip::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoDirectionalLightManip::valueChangedCB, this);
  }
}

/*!
  Return pointer to internal SoDirectionalLightDragger dragger.
*/
SoDragger *
SoDirectionalLightManip::getDragger(void)
{
  if (this->children->getLength() > 0) {
    SoNode * node = (*children)[0];
    if (node->isOfType(SoDragger::getClassTypeId()))
      return (SoDragger *)node;
    else {
#if COIN_DEBUG
      SoDebugError::post("SoDirectionalLightManip::getDragger",
                         "Child is not a dragger!");
#endif // debug
    }
  }
  return NULL;
}

/*!
  Replaces the node specified by \a path with this manipulator.  The
  manipulator will copy the field data from the node, to make it
  affect the state in the same way as the node.
*/
SbBool
SoDirectionalLightManip::replaceNode(SoPath * path)
{
  SoFullPath * fullpath = (SoFullPath *)path;
  SoNode * fulltail = fullpath->getTail();
  if (!fulltail->isOfType(SoDirectionalLight::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoDirectionalLightManip::replaceNode",
                       "End of path is not a SoDirectionalLight");
#endif // debug
    return FALSE;
  }
  SoNode * tail = path->getTail();
  if (tail->isOfType(SoBaseKit::getClassTypeId())) {
    SoBaseKit * kit = (SoBaseKit *) ((SoNodeKitPath *)path)->getTail();
    SbString partname = kit->getPartString(path);
    if (partname != "") {
      SoDirectionalLight * oldpart = (SoDirectionalLight *) kit->getPart(partname, TRUE);
      if (oldpart != NULL) {
        this->attachSensors(FALSE);
        this->transferFieldValues(oldpart, this);
        this->attachSensors(TRUE);
        SoDirectionalLightManip::fieldSensorCB(this, NULL);
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
    SoDebugError::post("SoDirectionalLightManip::replaceNode",
                       "Path is too short");
#endif // debug
    return FALSE;
  }
  SoNode * parent = fullpath->getNodeFromTail(1);
  if (!parent->isOfType(SoGroup::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoDirectionalLightManip::replaceNode",
                       "Parent node is not a group");
#endif // debug
    return FALSE;
  }
  this->ref();
  this->attachSensors(FALSE);
  this->transferFieldValues((SoDirectionalLight *)fulltail, this);
  this->attachSensors(TRUE);
  SoDirectionalLightManip::fieldSensorCB(this, NULL);

  ((SoGroup *)parent)->replaceChild(fulltail, this);
  this->unrefNoDelete();
  return TRUE;
}

// documented in superclass
void
SoDirectionalLightManip::doAction(SoAction * action)
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

// documented in superclass
void
SoDirectionalLightManip::callback(SoCallbackAction * action)
{
  SoDirectionalLightManip::doAction(action);
  SoDirectionalLight::callback(action);
}

// documented in superclass
void
SoDirectionalLightManip::GLRender(SoGLRenderAction * action)
{
  SoDirectionalLightManip::doAction(action);
  SoDirectionalLight::GLRender(action);
}

// documented in superclass
void
SoDirectionalLightManip::getBoundingBox(SoGetBoundingBoxAction * action)
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
  SoDirectionalLight::getBoundingBox(action);
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
SoDirectionalLightManip::getMatrix(SoGetMatrixAction * action)
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

// documented in superclass
void
SoDirectionalLightManip::handleEvent(SoHandleEventAction * action)
{
  SoDirectionalLightManip::doAction(action);
  SoDirectionalLight::handleEvent(action);
}

// documented in superclass
void
SoDirectionalLightManip::pick(SoPickAction * action)
{
  SoDirectionalLightManip::doAction(action);
  SoDirectionalLight::pick(action);
}

// documented in superclass
void
SoDirectionalLightManip::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoDirectionalLightManip::doAction(action);
}

/*!
  Returns the children of this node. This node only has the dragger as
  a child.
*/
SoChildList *
SoDirectionalLightManip::getChildren(void) const
{
  return this->children;
}

/*!
  \COININTERNAL
  Convert from matrix to direction.
*/
void
SoDirectionalLightManip::valueChangedCB(void * m, SoDragger * dragger)
{
  SoDirectionalLightManip * thisp = (SoDirectionalLightManip *)m;

  SbMatrix matrix = dragger->getMotionMatrix();
  SbVec3f direction(0.0f, 0.0f, -1.0f);
  matrix.multDirMatrix(direction, direction);
  if (direction.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::post("SoDirectionalLightManip::valueChangedCB",
                       "Invalid motion matrix.");
#endif // debug
  }
  thisp->attachSensors(FALSE);
  if (thisp->direction.getValue() != direction) {
    thisp->direction = direction;
  }
  thisp->attachSensors(TRUE);
}

/*!
  \COININTERNAL
*/
void
SoDirectionalLightManip::fieldSensorCB(void * m, SoSensor *)
{
  SoDirectionalLightManip * thisp = (SoDirectionalLightManip *)m;
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

// Documented in superclass. Overridden to copy the internal dragger
// instance.
void
SoDirectionalLightManip::copyContents(const SoFieldContainer * fromfc,
                                      SbBool copyconnections)
{
  assert(fromfc->isOfType(SoDirectionalLightManip::getClassTypeId()));
  this->setDragger(((SoDirectionalLightManip *)fromfc)->getDragger());
  inherited::copyContents(fromfc, copyconnections);
}

/*!
  Copies field values.
*/
void
SoDirectionalLightManip::transferFieldValues(const SoDirectionalLight * from,
                                             SoDirectionalLight * to)
{
  to->direction = from->direction;
  to->color = from->color;
}

void
SoDirectionalLightManip::attachSensors(const SbBool onoff)
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

#endif // HAVE_MANIPULATORS
