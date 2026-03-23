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
  \class SoClipPlaneManip SoClipPlaneManip.h Inventor/manips/SoClipPlaneManip.h
  \brief The SoClipPlaneManip class is used to manipulate clip planes.

  \ingroup coin_manips

  A manipulator is used by replacing the node you want to edit in the
  graph with the manipulator. Draggers are used to manipulate the
  node. When manipulation is finished, the node is put back into the
  graph, replacing the manipulator.

  <center>
  \image html jack.png "Example of ClipPlane Manipulator"
  </center>

  An SoJackDragger is used by instances of this manipulator class as
  the default dragger when manipulating SoClipPlane nodes. See the
  documentation of SoJackDragger for details about how to control the
  manipulator.

  An "action shot" of the SoClipPlaneManip:
  <center>
  \image html jack-in-action.png "Action Shot of ClipPlane Manipulator"
  </center>

  This manipulator is an extension versus the original SGI Inventor
  v2.1 API.  In addition to being a Coin extension, it is also present
  in TGS' Inventor implementation (with the same API).

  Please note that this manipulator is a bit different than the other
  manipulators, since it will not automatically scale and translate 
  the dragger to surround the geometry. The setValue() function must
  be used to initialize the manipulator/dragger. Below you'll find some
  example code that loads an Inventor file and adds a clip plane 
  manipulator.

  \code

  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/SoDB.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/actions/SoGetBoundingBoxAction.h>
  #include <Inventor/SoInput.h>
  #include <Inventor/manips/SoClipPlaneManip.h>
  #include <cassert>
  
  // *************************************************************************
  
  int
  main(int argc, char ** argv)
  {
    assert(argc >= 2);
    QWidget * window = SoQt::init(argv[0]);
    SoQtExaminerViewer * ex = new SoQtExaminerViewer( window );
    ex->setBackgroundColor(SbColor(0.1f, 0.3f, 0.5f));
  
    SoInput input;
    SbBool ok = input.openFile(argv[1]);
    if (!ok) {
      fprintf(stderr,"Unable to open file.\n");
      return -1;
    }

    SoSeparator * root = SoDB::readAll(&input); 
    if (!root) {
      fprintf(stderr,"Unable to read file.\n");
      return -1;
    }
    root->ref();
    SoGetBoundingBoxAction ba(ex->getViewportRegion());
    ba.apply(root);

    SbBox3f box = ba.getBoundingBox();
    SoClipPlaneManip * manip = new SoClipPlaneManip;
    manip->setValue(box, SbVec3f(1.0f, 0.0f, 0.0f), 1.02f);
    root->insertChild(manip, 0);

    ex->setSceneGraph(root);
    ex->show();
    SoQt::show( window );
    SoQt::mainLoop();
    delete ex;
    root->unref();
    return 0;
  }

  \endcode

  \since TGS Inventor 2.5
  \since Coin 1.0
*/

// *************************************************************************

#include <Inventor/manips/SoClipPlaneManip.h>

#include <Inventor/SoNodeKitPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoJackDragger.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoAntiSquish.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/sensors/SoFieldSensor.h>

// *************************************************************************

#include "nodes/SoSubNodeP.h"

class SoClipPlaneManipP {
public:
};

SO_NODE_SOURCE(SoClipPlaneManip);

// *************************************************************************

/*!
  \var SoSFVec3f * SoClipPlaneManip::draggerPosition
  \COININTERNAL
*/

/*!
  \var SoFieldSensor * SoClipPlaneManip::planeFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoClipPlaneManip::onFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoClipPlaneManip::draggerPosFieldSensor
  \COININTERNAL
*/
/*!
  \var SoChildList * SoClipPlaneManip::children
  \COININTERNAL
*/

// *************************************************************************

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoClipPlaneManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoClipPlaneManip, SO_FROM_INVENTOR_1);
}

/*!
  An SoJackDragger is set up here as the internal dragger used for
  manipulation of an SoClipPlane node.
*/
SoClipPlaneManip::SoClipPlaneManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoClipPlaneManip);
  SO_NODE_ADD_FIELD(draggerPosition, (0.0f, 0.0f, 0.0f));

  this->children = new SoChildList(this);

  this->planeFieldSensor =
    new SoFieldSensor(SoClipPlaneManip::fieldSensorCB, this);
  this->planeFieldSensor->setPriority(0);

  this->onFieldSensor =
    new SoFieldSensor(SoClipPlaneManip::fieldSensorCB, this);
  this->onFieldSensor->setPriority(0);

  this->draggerPosFieldSensor =
    new SoFieldSensor(SoClipPlaneManip::fieldSensorCB, this);
  this->draggerPosFieldSensor->setPriority(0);

  SoJackDragger * dragger = new SoJackDragger;
  this->setDragger(dragger);
  // needed to track the JackDragger main axis
  this->currAxis = 0;
}

/*!
  Destructor.
 */
SoClipPlaneManip::~SoClipPlaneManip()
{
  this->setDragger(NULL);

  delete this->draggerPosFieldSensor;
  delete this->onFieldSensor;
  delete this->planeFieldSensor;

  delete this->children;
}

// *************************************************************************

/*!
  Sets a dragger to use for this manipulator. The default dragger is
  an SoJackDragger.
 */
void
SoClipPlaneManip::setDragger(SoDragger * newdragger)
{
  // needed to track the JackDragger main axis
  this->currAxis = 0;

  SoDragger * olddragger = this->getDragger();
  if (olddragger) {
    olddragger->removeValueChangedCallback(SoClipPlaneManip::valueChangedCB, this);
    this->children->remove(0);
  }
  if (newdragger != NULL) {
    this->children->append(newdragger);
    SoClipPlaneManip::fieldSensorCB(this, NULL);
    newdragger->addValueChangedCallback(SoClipPlaneManip::valueChangedCB, this);
  }
}

/*!
  Returns pointer to dragger used for interaction.
 */
SoDragger *
SoClipPlaneManip::getDragger(void)
{
  if (this->children->getLength()) {
    SoNode *node = (*children)[0];
    if (node->isOfType(SoDragger::getClassTypeId()))
      return (SoDragger*)node;
    else {
#if COIN_DEBUG
      SoDebugError::post("SoClipPlaneManip::getDragger",
                         "Child is not a dragger!");
#endif // debug
    }
  }
  return NULL;
}

// *************************************************************************

// Documented in superclass. Overridden to copy the internal dragger
// instance.
void
SoClipPlaneManip::copyContents(const SoFieldContainer * fromfc, SbBool copyconnections)
{
  assert(fromfc->isOfType(SoClipPlaneManip::getClassTypeId()));
  this->setDragger(((SoClipPlaneManip*)fromfc)->getDragger());
  inherited::copyContents(fromfc, copyconnections);
}

/*!
  Sets the clip plane based on the center of \a box and \a normal.
  The size of \a box is used as a scale factor to the dragger,
  multiplied with \a draggerscalefactor.
*/
void
SoClipPlaneManip::setValue(const SbBox3f & box, const SbVec3f & planenormal, float draggerscalefactor)
{
  this->attachSensors(FALSE);
  SbPlane newplane(planenormal, box.getCenter());
  this->plane = newplane;
  this->draggerPosition = box.getCenter();

  float dx, dy, dz;
  box.getSize(dx, dy, dz);
  dx = SbMax(dx, SbMax(dy, dz));
  dx *= 0.5f;

  SoDragger * dragger = this->getDragger();
  SbVec3f s;
  s[0] = s[1] = s[2] = dx * draggerscalefactor;

  SbMatrix matrix;
  matrix.setScale(s);
  SbBool oldvalue = dragger->enableValueChangedCallbacks(FALSE);
  dragger->setMotionMatrix(matrix);
  (void) dragger->enableValueChangedCallbacks(oldvalue);
  this->attachSensors(TRUE);
  SoClipPlaneManip::fieldSensorCB(this, this->planeFieldSensor);
}

/*!
  Replaces the node specified by \a path with this manipulator.
  The manipulator will copy the field data from the node, to make
  it affect the state in the same way as the node.
*/
SbBool
SoClipPlaneManip::replaceNode(SoPath * path)
{
  SoFullPath *fullpath = (SoFullPath*)path;
  SoNode *fulltail = fullpath->getTail();
  if (!fulltail->isOfType(SoClipPlane::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoClipPlaneManip::replaceNode",
                       "End of path is not a SoClipPlane");
#endif // debug
    return FALSE;
  }
  SoNode *tail = path->getTail();
  if (tail->isOfType(SoBaseKit::getClassTypeId())) {
    SoBaseKit *kit = (SoBaseKit*) ((SoNodeKitPath*)path)->getTail();
    SbString partname = kit->getPartString(path);
    if (partname != "") {
      SoClipPlane *oldpart = (SoClipPlane*) kit->getPart(partname, TRUE);
      if (oldpart != NULL) {
        this->attachSensors(FALSE);
        this->transferFieldValues(oldpart, this);
        this->attachSensors(TRUE);
        SoClipPlaneManip::fieldSensorCB(this, this->planeFieldSensor);
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
    SoDebugError::post("SoClipPlaneManip::replaceNode",
                       "Path is too short");
#endif // debug
    return FALSE;
  }
  SoNode *parent = fullpath->getNodeFromTail(1);
  if (!parent->isOfType(SoGroup::getClassTypeId())) {
#if COIN_DEBUG
    SoDebugError::post("SoClipPlaneManip::replaceNode",
                       "Parent node is not a group");
#endif // debug
    return FALSE;
  }
  this->ref();
  this->attachSensors(FALSE);
  this->transferFieldValues((SoClipPlane*)fulltail, this);
  this->attachSensors(TRUE);
  SoClipPlaneManip::fieldSensorCB(this, this->planeFieldSensor);

  ((SoGroup*)parent)->replaceChild(fulltail, this);
  this->unrefNoDelete();
  return TRUE;
}

// doc from parent
void
SoClipPlaneManip::doAction(SoAction * action)
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

// doc from parent
void
SoClipPlaneManip::callback(SoCallbackAction * action)
{
  SoClipPlaneManip::doAction(action);
  SoClipPlane::callback(action);
}

// doc from parent
void
SoClipPlaneManip::GLRender(SoGLRenderAction * action)
{
  SoClipPlaneManip::doAction(action);
  SoClipPlane::GLRender(action);
}

// doc from parent
void
SoClipPlaneManip::getBoundingBox(SoGetBoundingBoxAction * action)
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
  SoClipPlane::getBoundingBox(action);
  if (action->isCenterSet()) {
    center += action->getCenter();
    numcenters++;
    action->resetCenter();
  }
  if (numcenters != 0) {
    action->setCenter(center / (float) numcenters, FALSE);
  }
}

// doc from parent
void
SoClipPlaneManip::getMatrix(SoGetMatrixAction * action)
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

// doc from parent
void
SoClipPlaneManip::handleEvent(SoHandleEventAction * action)
{
  // check for ctrl key here. If a JackDragger is used to drag the
  // clipping plane, it's possible to push the ctrl key to change the
  // major axis of the dragger.  FIXME: consider adding a function in
  // SoJackDragger that returns the current axis. pederb, 2005-02-04
  const SoEvent * event = action->getEvent();
  if (SO_KEY_PRESS_EVENT(event, LEFT_CONTROL) ||
      SO_KEY_PRESS_EVENT(event, RIGHT_CONTROL)) {
    const SoPickedPoint * pp = action->getPickedPoint();
    if (pp) {
      SoFullPath * path = (SoFullPath *)pp->getPath();
      for (int i = 0; i < path->getLength(); i++) {
        SoNode * node = path->getNode(i);
        if (node->isOfType(SoDragPointDragger::getClassTypeId())) {
          this->currAxis--;
          if (this->currAxis < 0) this->currAxis = 2;
          SoClipPlaneManip::valueChangedCB(this, this->getDragger());
          break;
        }
      }
    }
  }
  SoClipPlaneManip::doAction(action);
  SoClipPlane::handleEvent(action);
}

// doc from parent
void
SoClipPlaneManip::pick(SoPickAction * action)
{
  SoClipPlaneManip::doAction(action);
  SoClipPlane::pick(action);
}

// doc from parent
void
SoClipPlaneManip::search(SoSearchAction * action)
{
  inherited::search(action);
  if (action->isFound()) return;
  SoClipPlaneManip::doAction(action);
}

/*!
  Returns the children of this node.
*/
SoChildList *
SoClipPlaneManip::getChildren(void) const
{
  return this->children;
}

/*!
  \COININTERNAL
  Called whenever dragger is touched.
*/
void
SoClipPlaneManip::valueChangedCB(void * m, SoDragger * dragger)
{
  SoClipPlaneManip * thisp = (SoClipPlaneManip*)m;

  SbMatrix matrix = dragger->getMotionMatrix();

  SbRotation rot = SbRotation::identity();
  if (thisp->currAxis == 2)
    rot.setValue(SbVec3f(1.0f, 0.0f, 0.0f), float(M_PI/2));
  if (thisp->currAxis == 1) {
    rot.setValue(SbVec3f(0.0f, 0.0f, 1.0f), float(M_PI/2));
  }
  SbVec3f tt, s;
  SbRotation r, so;
  matrix.getTransform(tt, r, s, so);
  rot *= r;
  matrix.setTransform(tt, rot, s, so);

  SbPlane plane(SbVec3f(0.0f, 1.0f, 0.0f), 0.0f);
  // transform plane so that it matches the dragger geometry
  plane.transform(matrix);
  // extract the translation-part of the matrix
  SbVec3f t = SbVec3f(matrix[3][0], matrix[3][1], matrix[3][2]);

  thisp->attachSensors(FALSE);
  if (thisp->plane.getValue() != plane) {
    thisp->plane = plane;
  }
  if (t != thisp->draggerPosition.getValue()) {
    thisp->draggerPosition = t;
  }
  thisp->attachSensors(TRUE);
}

/*!
  \COININTERNAL
  Called whenever one of the fields changes value.
*/
void
SoClipPlaneManip::fieldSensorCB(void * m, SoSensor * sensor)
{
  SoClipPlaneManip *thisp = (SoClipPlaneManip*)m;

  if (sensor == thisp->onFieldSensor) return; // FIXME: should we care? pederb, 2003-02-28

  SoDragger * dragger = thisp->getDragger();
  if (dragger != NULL) {
    SbMatrix matrix = dragger->getMotionMatrix();

    SbVec3f planept;
    SbVec3f n = thisp->plane.getValue().getNormal();
    planept = thisp->draggerPosition.getValue();

    if (sensor == thisp->planeFieldSensor) {
      float dist = thisp->plane.getValue().getDistance(planept);
      planept += n * dist;
    }

    SbVec3f t, s;
    SbRotation r, so;
    matrix.getTransform(t, r, s, so);
    r.setValue(SbVec3f(0.0f, 1.0f, 0.0f), n);
    t = planept;
    matrix.setTransform(t, r, s, so);

    dragger->setMotionMatrix(matrix);

    // make sure draggerPosition field is up-to-date
    thisp->attachSensors(FALSE);
    if (t != thisp->draggerPosition.getValue()) {
      thisp->draggerPosition = t;
    }
    thisp->attachSensors(TRUE);
  }
}

// Convenience method used to attach and detach field sensors.
void
SoClipPlaneManip::attachSensors(const SbBool onoff)
{
  if (onoff) {
    this->planeFieldSensor->attach(&this->plane);
    this->onFieldSensor->attach(&this->on);
    this->draggerPosFieldSensor->attach(&this->draggerPosition);
  }
  else {
    this->planeFieldSensor->detach();
    this->onFieldSensor->detach();
    this->draggerPosFieldSensor->detach();
  }
}

/*!
  Copies field values from one clip plane node to another.
*/
void
SoClipPlaneManip::transferFieldValues(const SoClipPlane * from, SoClipPlane * to)
{
  to->plane = from->plane;
  to->on = from->on;
}

#endif // HAVE_MANIPULATORS
