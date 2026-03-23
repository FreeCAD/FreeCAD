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
  \class SoPointLightManip SoPointLightManip.h Inventor/manips/SoPointLightManip.h
  \brief The SoPointLightManip class is used to manipulate point light nodes.

  \ingroup coin_manips

  A manipulator is used by replacing the node you want to edit in the
  graph with the manipulator. Draggers are used to manipulate the
  node. When manipulation is finished, the node is put back into the
  graph, replacing the manipulator.

  <center>
  \image html pointlight.png "Example of PointLight Manipulator"
  </center>
*/

/*!
  \var SoFieldSensor * SoPointLightManip::locationFieldSensor
  \COININTERNAL
*/

/*!
  \var SoFieldSensor * SoPointLightManip::colorFieldSensor
  \COININTERNAL
*/

/*!
  \var SoChildList * SoPointLightManip::children
  \COININTERNAL
*/

#include <Inventor/manips/SoPointLightManip.h>

#include <Inventor/draggers/SoPointLightDragger.h>
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

class SoPointLightManipP {
public:
};

SO_NODE_SOURCE(SoPointLightManip);


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoPointLightManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPointLightManip, SO_FROM_INVENTOR_1);
}

/*!
  The constructor. Creates a default dragger.
*/
SoPointLightManip::SoPointLightManip(void)
{
  this->children = new SoChildList(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoPointLightManip);

  this->locationFieldSensor = new SoFieldSensor(SoPointLightManip::fieldSensorCB, this);
  this->locationFieldSensor->setPriority(0);

  this->colorFieldSensor = new SoFieldSensor(SoPointLightManip::fieldSensorCB, this);
  this->colorFieldSensor->setPriority(0);

  this->attachSensors(TRUE);
  this->setDragger(new SoPointLightDragger);
}

/*!
  The destructor.
*/
SoPointLightManip::~SoPointLightManip()
{
  this->setDragger(NULL);

  delete this->colorFieldSensor;
  delete this->locationFieldSensor;

  delete this->children;
}

/*!
  The method can be used to replace the dragger with your own dragger.
*/
void
SoPointLightManip::setDragger(SoDragger * newdragger)
{
  SoDragger * olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoPointLightManip::valueChangedCB, this);
    this->children->remove(0);
  }
  if (newdragger != NULL) {
    if (this->children->getLength() > 0) {
      this->children->set(0, newdragger);
    }
    else {
      this->children->append(newdragger);
    }
    SoPointLightManip::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoPointLightManip::valueChangedCB, this);
  }
}

/*!
  Returns the current dragger.
  \sa setDragger()
*/
SoDragger *
SoPointLightManip::getDragger(void)
{
  if (this->children->getLength() > 0) {
    SoNode * node = (*children)[0];
    if (node->isOfType(SoDragger::getClassTypeId()))
      return (SoDragger *)node;
    else {
#if COIN_DEBUG
      SoDebugError::post("SoPointLightManip::getDragger",
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
SoPointLightManip::replaceNode(SoPath * path)
{
  SoFullPath * fullpath = (SoFullPath *)path;
  SoNode * fulltail = fullpath->getTail();
  if (!fulltail->isOfType(SoPointLight::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoPointLightManip::replaceNode",
                       "End of path is not a SoPointLight");
#endif // debug
    return FALSE;
  }
  SoNode * tail = path->getTail();
  if (tail->isOfType(SoBaseKit::getClassTypeId())) {
    SoBaseKit * kit = (SoBaseKit *) ((SoNodeKitPath *)path)->getTail();
    SbString partname = kit->getPartString(path);
    if (partname != "") {
      SoPointLight * oldpart = (SoPointLight *) kit->getPart(partname, TRUE);
      if (oldpart != NULL) {
        this->attachSensors(FALSE);
        this->transferFieldValues(oldpart, this);
        this->attachSensors(TRUE);
        SoPointLightManip::fieldSensorCB(this, NULL);
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
    SoDebugError::post("SoPointLightManip::replaceNode",
                       "Path is too short");
#endif // debug
    return FALSE;
  }
  SoNode * parent = fullpath->getNodeFromTail(1);
  if (!parent->isOfType(SoGroup::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoPointLightManip::replaceNode",
                       "Parent node is not a group");
#endif // debug
    return FALSE;
  }
  this->ref();
  this->attachSensors(FALSE);
  this->transferFieldValues((SoPointLight *)fulltail, this);
  this->attachSensors(TRUE);
  SoPointLightManip::fieldSensorCB(this, NULL);

  ((SoGroup *)parent)->replaceChild(fulltail, this);
  this->unrefNoDelete();
  return TRUE;
}

// Documented in superclass
void
SoPointLightManip::doAction(SoAction * action)
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

// Documented in superclass
void
SoPointLightManip::callback(SoCallbackAction * action)
{
  SoPointLightManip::doAction(action);
  SoPointLight::callback(action);
}

// Documented in superclass
void
SoPointLightManip::GLRender(SoGLRenderAction * action)
{
  SoPointLightManip::doAction(action);
  SoPointLight::GLRender(action);
}

// Documented in superclass
void
SoPointLightManip::getBoundingBox(SoGetBoundingBoxAction * action)
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
  SoPointLight::getBoundingBox(action);
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
SoPointLightManip::getMatrix(SoGetMatrixAction * action)
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

// Documented in superclass
void
SoPointLightManip::handleEvent(SoHandleEventAction * action)
{
  SoPointLightManip::doAction(action);
  SoPointLight::handleEvent(action);
}

// Documented in superclass
void
SoPointLightManip::pick(SoPickAction * action)
{
  SoPointLightManip::doAction(action);
  SoPointLight::pick(action);
}

// Documented in superclass
void
SoPointLightManip::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoPointLightManip::doAction(action);
}

/*!
  Returns the children of this node. This node only has the dragger
  as a child.
*/
SoChildList *
SoPointLightManip::getChildren(void) const
{
  return this->children;
}

/*!
  \COININTERNAL
*/
void
SoPointLightManip::valueChangedCB(void * m, SoDragger * dragger)
{
  SoPointLightManip * thisp = (SoPointLightManip *)m;

  SbMatrix matrix = dragger->getMotionMatrix();
  SbVec3f location = matrix[3];

  thisp->attachSensors(FALSE);
  if (thisp->location.getValue() != location) {
    thisp->location = location;
  }
  thisp->attachSensors(TRUE);
}

/*!
  \COININTERNAL
*/
void
SoPointLightManip::fieldSensorCB(void * m, SoSensor *)
{
  SoPointLightManip * thisp = (SoPointLightManip *)m;
  SoDragger * dragger = thisp->getDragger();
  if (dragger != NULL) {
    SbVec3f location = thisp->location.getValue();
    SbMatrix matrix = dragger->getMotionMatrix();
    matrix[3][0] = location[0];
    matrix[3][1] = location[1];
    matrix[3][2] = location[2];
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
SoPointLightManip::copyContents(const SoFieldContainer * fromfc, SbBool copyconnections)
{
  assert(fromfc->isOfType(SoPointLightManip::getClassTypeId()));
  this->setDragger(((SoPointLightManip *)fromfc)->getDragger());
  inherited::copyContents(fromfc, copyconnections);
}

/*!
  \COININTERNAL
  Copies field values.
*/
void
SoPointLightManip::transferFieldValues(const SoPointLight * from, SoPointLight * to)
{
  to->location = from->location;
  to->color = from->color;
}

void
SoPointLightManip::attachSensors(const SbBool onoff)
{
  if (onoff) {
    this->locationFieldSensor->attach(&this->location);
    this->colorFieldSensor->attach(&this->color);
  }
  else {
    this->locationFieldSensor->detach();
    this->colorFieldSensor->detach();
  }
}

#endif // HAVE_MANIPULATORS
