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

#ifdef HAVE_DRAGGERS

/*!
  \class SoTranslate2Dragger SoTranslate2Dragger.h Inventor/draggers/SoTranslate2Dragger.h
  \brief The SoTranslate2Dragger class provides a mechanism for the end-user to translate in a plane.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html translate2.png "Screen Shot of Default Dragger"
  </center>

  Use this dragger to allow the end-user of your application to
  translate along the X-axis and the Y-axis at the same time, i.e.
  freely inside a 3D plane. (Use a transformation node in front of the
  dragger to position it and re-orient it to translate in any plane.)

  By holding down a \c SHIFT key, the end-user can temporarily
  constrain the dragger to a single one of the two axes.

  \sa SoTranslate1Dragger, SoDragPointDragger
*/

#include <Inventor/draggers/SoTranslate2Dragger.h>

#include <cstring>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/SbRotation.h>

#include <data/draggerDefaults/translate2Dragger.h>

#include "nodekits/SoSubKitP.h"
#include "coindefs.h"

#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3

/*!
  \var SoSFVec3f SoTranslate2Dragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position. Only the first two components (the X
  and Y values) can be changed by the end-user.

  The application programmer applying this dragger in his code should
  connect the relevant node fields in the scene to this field to make
  it follow the dragger.
*/

/*!
  \var SoSFVec2f SoTranslate1Dragger::minTranslation

  Sets the minimum value allowed in each component of the translation
  field.  This is only active if minTranslation <= maxTranslation for
  each component.

  Default value is [1.0, 1.0]

  \since Coin 3.0
*/

/*!
  \var SoSFVec2f SoTranslate1Dragger::maxTranslation

  Sets the maximum value allowed in each component of the translation
  field.  This is only active if minTranslation <= maxTranslation for
  each component.

  Default value is [0.0, 0.0]

  \since Coin 3.0
*/

/*!
  \var SbPlaneProjector * SoTranslate2Dragger::planeProj

  The SbPlaneProjector instance used for projecting from 2D mouse
  cursor positions to 3D points.
*/

/*!
  \var SoFieldSensor * SoTranslate2Dragger::fieldSensor
  \COININTERNAL
*/
/*!
  \var SbVec3f SoTranslate2Dragger::worldRestartPt
  \COININTERNAL
*/

#ifndef DOXYGEN_SKIP_THIS // Don't document internal classes.

class SoTranslate2DraggerP {
public:
  SbVec3f lastmotion;
  SbVec3f extramotion;
};

#endif // DOXYGEN_SKIP_THIS

SO_KIT_SOURCE(SoTranslate2Dragger);

#define PRIVATE(obj) ((obj)->pimpl)
#define THISP(d) static_cast<SoTranslate2Dragger *>(d)

/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoTranslate2Dragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoTranslate2Dragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoTranslate2Dragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
           "geomSeparator"
  -->         "translatorSwitch"
  -->            "translator"
  -->            "translatorActive"
  -->         "feedbackSwitch"
  -->            "feedback"
  -->            "feedbackActive"
  -->         "axisFeedbackSwitch"
  -->            "xAxisFeedback"
  -->            "yAxisFeedback"

  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoTranslate2Dragger
  PVT   "this",  SoTranslate2Dragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
  PVT   "geomSeparator",  SoSeparator  ---
  PVT   "translatorSwitch",  SoSwitch  ---
        "translator",  SoSeparator  ---
        "translatorActive",  SoSeparator  ---
  PVT   "feedbackSwitch",  SoSwitch  ---
        "feedback",  SoSeparator  ---
        "feedbackActive",  SoSeparator  ---
  PVT   "axisFeedbackSwitch",  SoSwitch  ---
        "xAxisFeedback",  SoSeparator  ---
        "yAxisFeedback",  SoSeparator  ---
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoTranslate2Dragger::SoTranslate2Dragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoTranslate2Dragger);

  SO_KIT_ADD_CATALOG_ENTRY(translatorSwitch, SoSwitch, TRUE, geomSeparator, feedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, TRUE, translatorSwitch, translatorActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorActive, SoSeparator, TRUE, translatorSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch, SoSwitch, TRUE, geomSeparator, axisFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, TRUE, feedbackSwitch, feedbackActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackActive, SoSeparator, TRUE, feedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(axisFeedbackSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(xAxisFeedback, SoSeparator, TRUE, axisFeedbackSwitch, yAxisFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(yAxisFeedback, SoSeparator, TRUE, axisFeedbackSwitch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("translate2Dragger.iv",
                                       TRANSLATE2DRAGGER_draggergeometry,
                                       static_cast<int>(strlen(TRANSLATE2DRAGGER_draggergeometry)));
  }
  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(minTranslation, (1.0f, 1.0f));
  SO_KIT_ADD_FIELD(maxTranslation, (0.0f, 0.0f));
  SO_KIT_INIT_INSTANCE();

  // initialize default parts
  this->setPartAsDefault("translator", "translate2Translator");
  this->setPartAsDefault("translatorActive", "translate2TranslatorActive");
  this->setPartAsDefault("feedback", "translate2Feedback");
  this->setPartAsDefault("feedbackActive", "translate2FeedbackActive");
  this->setPartAsDefault("xAxisFeedback", "translate2XAxisFeedback");
  this->setPartAsDefault("yAxisFeedback", "translate2YAxisFeedback");

  // initialize switch values
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);

  // setup projector
  this->planeProj = new SbPlaneProjector();
  this->addStartCallback(SoTranslate2Dragger::startCB);
  this->addMotionCallback(SoTranslate2Dragger::motionCB);
  this->addFinishCallback(SoTranslate2Dragger::finishCB);
  this->addOtherEventCallback(SoTranslate2Dragger::metaKeyChangeCB);
  this->addValueChangedCallback(SoTranslate2Dragger::valueChangedCB);

  this->fieldSensor = new SoFieldSensor(SoTranslate2Dragger::fieldSensorCB, this);
  this->fieldSensor->setPriority(0);

  this->constraintState = CONSTRAINT_OFF;

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoTranslate2Dragger::~SoTranslate2Dragger()
{
  delete this->planeProj;
  delete this->fieldSensor;
}

// doc in superclass
SbBool
SoTranslate2Dragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoTranslate2Dragger::fieldSensorCB(this, NULL);

    if (this->fieldSensor->getAttachedField() != &this->translation) {
      this->fieldSensor->attach(&this->translation);
    }
  }
  else {
    if (this->fieldSensor->getAttachedField() != NULL) {
      this->fieldSensor->detach();
    }
    inherited::setUpConnections(onoff, doitalways);
  }
  this->connectionsSetUp = onoff;
  return oldval;
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::fieldSensorCB(void * d, SoSensor * COIN_UNUSED_ARG(s))
{
  assert(d);
  SoTranslate2Dragger * thisp = THISP(d);
  const SbVec2f minv = thisp->minTranslation.getValue();
  const SbVec2f maxv = thisp->maxTranslation.getValue();

  SbVec3f t = thisp->translation.getValue();
  SbVec3f orgt = t;

  for (int i = 0; i < 2; i++) {
    if (minv[i] <= maxv[i]) {
      t[i] = SbClamp(t[i], minv[i], maxv[i]);
    }
  }
  if (t != orgt) thisp->translation = t;
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::valueChangedCB(void *, SoDragger * d)
{
  SoTranslate2Dragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  SbVec3f trans = thisp->clampMatrix(matrix);
  if (thisp->translation.getValue() != trans) {
    thisp->fieldSensor->detach();
    thisp->translation = trans;
    thisp->fieldSensor->attach(&thisp->translation);
  }
}

// doc in parent
void
SoTranslate2Dragger::setMotionMatrix(const SbMatrix & matrix)
{
  SbMatrix m = matrix;
  (void) this->clampMatrix(m);
  inherited::setMotionMatrix(m);
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::startCB(void *, SoDragger * d)
{
  SoTranslate2Dragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::motionCB(void *, SoDragger * d)
{
  SoTranslate2Dragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::finishCB(void *, SoDragger * d)
{
  SoTranslate2Dragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL */
void
SoTranslate2Dragger::metaKeyChangeCB(void *, SoDragger *d)
{
  SoTranslate2Dragger * thisp = THISP(d);
  if (!thisp->isActive.getValue()) return;

  const SoEvent *event = thisp->getEvent();
  if (thisp->constraintState == CONSTRAINT_OFF &&
      event->wasShiftDown()) thisp->drag();
  else if (thisp->constraintState != CONSTRAINT_OFF &&
           !event->wasShiftDown()) thisp->drag();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoTranslate2Dragger::dragStart(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);
  sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

  SbVec3f hitPt = this->getLocalStartingPoint();
  this->planeProj->setPlane(SbPlane(SbVec3f(0.0f, 0.0f, 1.0f), hitPt));
  if (this->getEvent()->wasShiftDown()) {
    this->getLocalToWorldMatrix().multVecMatrix(hitPt, this->worldRestartPt);
    this->constraintState = CONSTRAINT_WAIT;
  }

  PRIVATE(this)->extramotion = SbVec3f(0, 0, 0);
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoTranslate2Dragger::drag(void)
{
  this->planeProj->setViewVolume(this->getViewVolume());
  this->planeProj->setWorkingSpace(this->getLocalToWorldMatrix());

  SbVec3f projPt;
  if (this->planeProj->tryProject(this->getNormalizedLocaterPosition(),
                                  this->getProjectorEpsilon(),
                                  projPt)) {
    const SoEvent *event = this->getEvent();
    if (event->wasShiftDown() && this->constraintState == CONSTRAINT_OFF) {
      this->constraintState = CONSTRAINT_WAIT;
      this->setStartLocaterPosition(event->getPosition());
      this->getLocalToWorldMatrix().multVecMatrix(projPt, this->worldRestartPt);
    }
    else if (!event->wasShiftDown() && this->constraintState != CONSTRAINT_OFF) {
      SbVec3f worldProjPt;
      this->getLocalToWorldMatrix().multVecMatrix(projPt, worldProjPt);
      this->setStartingPoint(worldProjPt);
      PRIVATE(this)->extramotion += PRIVATE(this)->lastmotion;

      SoSwitch *sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
      SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);
      this->constraintState = CONSTRAINT_OFF;
    }

    SbVec3f startPt = this->getLocalStartingPoint();
    SbVec3f motion;
    SbVec3f localrestartpt;

    if (this->constraintState != CONSTRAINT_OFF) {
      this->getWorldToLocalMatrix().multVecMatrix(this->worldRestartPt,
                                                  localrestartpt);
      motion = localrestartpt - startPt;
    }
    else motion = projPt - startPt;

    switch(this->constraintState) {
    case CONSTRAINT_OFF:
      break;
    case CONSTRAINT_WAIT:
      if (this->isAdequateConstraintMotion()) {
        SbVec3f newmotion = projPt - localrestartpt;
        if (fabs(newmotion[0]) >= fabs(newmotion[1])) {
        this->constraintState = CONSTRAINT_X;
        motion[0] += newmotion[0];
        SoSwitch *sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
        SoInteractionKit::setSwitchValue(sw, 0);
        }
        else {
          this->constraintState = CONSTRAINT_Y;
          motion[1] += newmotion[1];
          SoSwitch *sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
          SoInteractionKit::setSwitchValue(sw, 1);
        }
      }
      else {
      return;
      }
      break;
    case CONSTRAINT_X:
      motion[0] += projPt[0] - localrestartpt[0];
      break;
    case CONSTRAINT_Y:
      motion[1] += projPt[1] - localrestartpt[1];
    break;
    }
    PRIVATE(this)->lastmotion = motion;
    this->setMotionMatrix(this->appendTranslation(this->getStartMotionMatrix(),
                                                PRIVATE(this)->extramotion+motion));
  }
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoTranslate2Dragger::dragFinish(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "axisFeedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
  this->constraintState = CONSTRAINT_OFF;
}

SbVec3f
SoTranslate2Dragger::clampMatrix(SbMatrix & m) const
{
  const SbVec2f minv = this->minTranslation.getValue();
  const SbVec2f maxv = this->maxTranslation.getValue();

  SbVec3f pretrans = this->translation.getValue();

  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  m.getTransform(trans, rot, scale, scaleOrient);
  // avoid that the Z components drifts away from the dragger plane
  trans[2] = pretrans[2];
  SbVec3f t = trans;
  for (int i = 0; i < 2; i++) {
    if (minv[i] <= maxv[i]) {
      t[i] = SbClamp(t[i], minv[i], maxv[i]);
    }
  }

  if (t != trans) {
    m.setTransform(t, rot, scale, scaleOrient);
  }
  return t;
}

#undef THISP
#undef PRIVATE
#undef CONSTRAINT_OFF
#undef CONSTRAINT_WAIT
#undef CONSTRAINT_X
#undef CONSTRAINT_Y
#endif // HAVE_DRAGGERS
