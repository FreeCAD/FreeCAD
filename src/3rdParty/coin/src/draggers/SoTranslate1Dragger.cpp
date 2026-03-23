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
  \class SoTranslate1Dragger SoTranslate1Dragger.h Inventor/draggers/SoTranslate1Dragger.h
  \brief The SoTranslate1Dragger class provides a mechanism for the end-user to translate along an axis.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html translate1.png "Screen Shot of Default Dragger"
  </center>

  Use this dragger to allow the end-user of your application to
  translate along the X-axis, i.e. freely along a line. (Use a
  transformation node in front of the dragger to position it and
  re-orient it to translate along any line.)

  \sa SoTranslate2Dragger, SoDragPointDragger
*/
// FIXME: in class-doc, explain that one should always connect to the
// x-coordinate of this dragger. Also show by a code snippet how to 0)
// use a SoDecomposeVec3fEngine for extracting the x-coordinate, and
// 1) set up a vertical or "depth" version of the dragger by using a
// rotation transform. 20011021 mortene.

#include <Inventor/draggers/SoTranslate1Dragger.h>

#include <cstring>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include <data/draggerDefaults/translate1Dragger.h>

#include "nodekits/SoSubKitP.h"

/*!
  \var SoSFVec3f SoTranslate1Dragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position. Only the first component (the X
  value) can be changed by the end-user.

  The application programmer applying this dragger in his code should
  connect the relevant node fields in the scene to this field to make
  it follow the dragger.
*/

/*!
  \var SoSFFloat SoTranslate1Dragger::minTranslation

  Sets the minimum value allowed in the x component of the translation
  field.  This is only active if minTranslation <= maxTranslation.

  Default value is 1.0

  \since Coin 3.0
*/

/*!
  \var SoSFFloat SoTranslate1Dragger::maxTranslation

  Sets the maximum value allowed in the x component of the translation
  field.  This is only active if minTranslation <= maxTranslation.

  Default value is 0.0.

  \since Coin 3.0
*/

/*!
  \var SbLineProjector * SoTranslate1Dragger::lineProj

  The SbLineProjector instance used for projecting from 2D mouse
  cursor positions to 3D points.
*/

/*!
  \var SoFieldSensor * SoTranslate1Dragger::fieldSensor
  \COININTERNAL
*/

#define THISP(d) static_cast<SoTranslate1Dragger *>(d)

class SoTranslate1DraggerP {
public:
};

SO_KIT_SOURCE(SoTranslate1Dragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoTranslate1Dragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoTranslate1Dragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoTranslate1Dragger
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
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoTranslate1Dragger
  PVT   "this",  SoTranslate1Dragger  --- 
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
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoTranslate1Dragger::SoTranslate1Dragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoTranslate1Dragger);

  SO_KIT_ADD_CATALOG_ENTRY(translatorSwitch, SoSwitch, TRUE, geomSeparator, feedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, TRUE, translatorSwitch, translatorActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorActive, SoSeparator, TRUE, translatorSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, TRUE, feedbackSwitch, feedbackActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackActive, SoSeparator, TRUE, feedbackSwitch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("translate1Dragger.iv",
                                       TRANSLATE1DRAGGER_draggergeometry,
                                       static_cast<int>(strlen(TRANSLATE1DRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(minTranslation, (1.0f));
  SO_KIT_ADD_FIELD(maxTranslation, (0.0f));
  
  SO_KIT_INIT_INSTANCE();

  // initialize default parts
  this->setPartAsDefault("translator", "translate1Translator");
  this->setPartAsDefault("translatorActive", "translate1TranslatorActive");
  this->setPartAsDefault("feedback", "translate1Feedback");
  this->setPartAsDefault("feedbackActive", "translate1FeedbackActive");

  // initialize switch values
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);

  // setup projector
  this->lineProj = new SbLineProjector();
  this->addStartCallback(SoTranslate1Dragger::startCB);
  this->addMotionCallback(SoTranslate1Dragger::motionCB);
  this->addFinishCallback(SoTranslate1Dragger::finishCB);

  this->addValueChangedCallback(SoTranslate1Dragger::valueChangedCB);

  this->fieldSensor = new SoFieldSensor(SoTranslate1Dragger::fieldSensorCB, this);
  this->fieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoTranslate1Dragger::~SoTranslate1Dragger()
{
  delete this->lineProj;
  delete this->fieldSensor;
}

// Doc in super.
SbBool
SoTranslate1Dragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoTranslate1Dragger::fieldSensorCB(this, NULL);

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
SoTranslate1Dragger::fieldSensorCB(void *d, SoSensor *)
{
  assert(d);
  SoTranslate1Dragger * thisp = THISP(d);
  const float minv = thisp->minTranslation.getValue();
  const float maxv = thisp->maxTranslation.getValue();
  if (minv <= maxv) {
    SbVec3f t = thisp->translation.getValue();
    if (t[0] < minv || t[0] > maxv) {
      t[0] = SbClamp(t[0], minv, maxv);
      thisp->translation = t;
    }
  }
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

// doc in parent
void 
SoTranslate1Dragger::setMotionMatrix(const SbMatrix & matrix)
{
  SbMatrix m = matrix;
  (void) this->clampMatrix(m);
  inherited::setMotionMatrix(m);  
}

/*! \COININTERNAL */
void
SoTranslate1Dragger::valueChangedCB(void *, SoDragger * d)
{
  SoTranslate1Dragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  SbVec3f trans = thisp->clampMatrix(matrix);
  thisp->fieldSensor->detach();
  if (thisp->translation.getValue() != trans)
    thisp->translation = trans;
  thisp->fieldSensor->attach(&thisp->translation);
}

/*! \COININTERNAL */
void
SoTranslate1Dragger::startCB(void *, SoDragger * d)
{
  SoTranslate1Dragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoTranslate1Dragger::motionCB(void *, SoDragger * d)
{
  SoTranslate1Dragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoTranslate1Dragger::finishCB(void *, SoDragger * d)
{
  SoTranslate1Dragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoTranslate1Dragger::dragStart(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);

  SbVec3f hitPt = this->getLocalStartingPoint();
  this->lineProj->setLine(SbLine(hitPt, hitPt + SbVec3f(1.0f, 0.0f, 0.0f)));
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoTranslate1Dragger::drag(void)
{
  this->lineProj->setViewVolume(this->getViewVolume());
  this->lineProj->setWorkingSpace(this->getLocalToWorldMatrix());
  
  const float epsilon = this->getProjectorEpsilon();
  SbVec3f projPt;
  if (this->lineProj->tryProject(this->getNormalizedLocaterPosition(), epsilon, projPt)) {
    SbVec3f startPt = this->getLocalStartingPoint();
    SbVec3f motion = projPt - startPt;
    SbMatrix mm = this->appendTranslation(this->getStartMotionMatrix(), motion);
    this->setMotionMatrix(mm);
  }
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoTranslate1Dragger::dragFinish(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "translatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
}

SbVec3f 
SoTranslate1Dragger::clampMatrix(SbMatrix & m) const
{
  const float minv = this->minTranslation.getValue();
  const float maxv = this->maxTranslation.getValue();
  
  SbVec3f pretrans = this->translation.getValue();
  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  m.getTransform(trans, rot, scale, scaleOrient);

  // avoid that the Y and Z components drift away from the line
  trans[1] = pretrans[1];
  trans[2] = pretrans[2];

  if (minv <= maxv) {    
    SbVec3f t = trans;
    t[0] = SbClamp(t[0], minv, maxv);
    if (t != trans) {
      m.setTransform(t, rot, scale, scaleOrient);
    }
  }
  return trans;
}

#undef THISP
#endif // HAVE_DRAGGERS
