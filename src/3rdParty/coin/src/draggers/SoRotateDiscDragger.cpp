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
  \class SoRotateDiscDragger SoRotateDiscDragger.h Inventor/draggers/SoRotateDiscDragger.h
  \brief The SoRotateDiscDragger class is for rotating geometry around a single axis.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html rotatedisc.png "Screen Shot of Default Dragger"
  </center>

  Use an instance of this dragger class in your scene graph to let the
  end-users of your application rotate geometry around a predefined
  axis vector in 3D.

  The special feature of this dragger that sets it apart from the
  other draggers that provides rotation around an axis (like the
  SoRotateCylindricalDragger) is that it provides a convenient
  interface to rotate around an axis that is pointing in approximately
  the same direction as the camera. This is useful for interacting
  with for instance something like a "volume" knob.

  For the dragger orientation and positioning itself, use some kind of
  transformation node in your scene graph, as usual.
*/

#include <Inventor/draggers/SoRotateDiscDragger.h>

#include <cstring>
#include <cmath>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/rotateDiscDragger.h>

#include "nodekits/SoSubKitP.h"
#include "coindefs.h"

/*!
  \var SoSFRotation SoRotateDiscDragger::rotation

  This field is continuously updated to contain the rotation of the
  current direction vector of the dragger.

  The application programmer using this dragger in his scene graph
  should connect the relevant node fields in the scene to this field
  to make them follow the dragger orientation.
*/

/*!
  \var SoFieldSensor * SoRotateDiscDragger::fieldSensor
  \COININTERNAL
*/
/*!
  \var SbPlaneProjector * SoRotateDiscDragger::planeProj
  \COININTERNAL
*/

#define THISP(d) static_cast<SoRotateDiscDragger *>(d)

class SoRotateDiscDraggerP {
public:
};

SO_KIT_SOURCE(SoRotateDiscDragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoRotateDiscDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoRotateDiscDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoRotateDiscDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
           "geomSeparator"
  -->         "rotatorSwitch"
  -->            "rotator"
  -->            "rotatorActive"
  -->         "feedbackSwitch"
  -->            "feedback"
  -->            "feedbackActive"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoRotateDiscDragger
  PVT   "this",  SoRotateDiscDragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
  PVT   "geomSeparator",  SoSeparator  ---
  PVT   "rotatorSwitch",  SoSwitch  ---
        "rotator",  SoSeparator  ---
        "rotatorActive",  SoSeparator  ---
  PVT   "feedbackSwitch",  SoSwitch  ---
        "feedback",  SoSeparator  ---
        "feedbackActive",  SoSeparator  ---
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoRotateDiscDragger::SoRotateDiscDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoRotateDiscDragger);

  SO_KIT_ADD_CATALOG_ENTRY(rotatorSwitch, SoSwitch, TRUE, geomSeparator, feedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoSeparator, TRUE, rotatorSwitch, rotatorActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotatorActive, SoSeparator, TRUE, rotatorSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, TRUE, feedbackSwitch, feedbackActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackActive, SoSeparator, TRUE, feedbackSwitch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("rotateDiscDragger.iv",
                                       ROTATEDISCDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(ROTATEDISCDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_INIT_INSTANCE();

  // initialize default parts
  this->setPartAsDefault("rotator", "rotateDiscRotator");
  this->setPartAsDefault("rotatorActive", "rotateDiscRotatorActive");
  this->setPartAsDefault("feedback", "rotateDiscFeedback");
  this->setPartAsDefault("feedbackActive", "rotateDiscFeedbackActive");

  // initialize switch values
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);

  // setup projector
  this->planeProj = new SbPlaneProjector();
  this->addStartCallback(SoRotateDiscDragger::startCB);
  this->addMotionCallback(SoRotateDiscDragger::motionCB);
  this->addFinishCallback(SoRotateDiscDragger::doneCB);

  this->addValueChangedCallback(SoRotateDiscDragger::valueChangedCB);

  this->fieldSensor = new SoFieldSensor(SoRotateDiscDragger::fieldSensorCB, this);
  this->fieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoRotateDiscDragger::~SoRotateDiscDragger()
{
  delete this->fieldSensor;
  delete this->planeProj;
}

// Doc in superclass.
SbBool
SoRotateDiscDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoRotateDiscDragger::fieldSensorCB(this, NULL);

    if (this->fieldSensor->getAttachedField() != &this->rotation) {
      this->fieldSensor->attach(&this->rotation);
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
SoRotateDiscDragger::fieldSensorCB(void * d, SoSensor *)
{
  assert(d);
  SoRotateDiscDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();

  SbVec3f t, s;
  SbRotation r, so;

  matrix.getTransform(t, r, s, so);
  r = thisp->rotation.getValue();
  matrix.setTransform(t, r, s, so);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoRotateDiscDragger::valueChangedCB(void *, SoDragger * d)
{
  SoRotateDiscDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();

  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  matrix.getTransform(trans, rot, scale, scaleOrient);
  thisp->fieldSensor->detach();
  if (thisp->rotation.getValue() != rot)
    thisp->rotation = rot;
  thisp->fieldSensor->attach(&thisp->rotation);
}

/*! \COININTERNAL */
void
SoRotateDiscDragger::startCB(void *, SoDragger * d)
{
  SoRotateDiscDragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoRotateDiscDragger::motionCB(void *, SoDragger * d)
{
  SoRotateDiscDragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoRotateDiscDragger::doneCB(void * COIN_UNUSED_ARG(f), SoDragger * d)
{
  SoRotateDiscDragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoRotateDiscDragger::dragStart(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);

  SbVec3f hitPt = this->getLocalStartingPoint();
  this->planeProj->setPlane(SbPlane(SbVec3f(0.0f, 0.0f, 1.0f),
                                    hitPt));
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoRotateDiscDragger::drag(void)
{
  this->planeProj->setViewVolume(this->getViewVolume());
  this->planeProj->setWorkingSpace(this->getLocalToWorldMatrix());

  SbVec3f projPt = planeProj->project(this->getNormalizedLocaterPosition());
  SbVec3f startPt = this->getLocalStartingPoint();

  SbPlane plane(SbVec3f(0.0f, 0.0f, 1.0f), startPt);
  SbLine line(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 0.0f, 1.0f));
  SbVec3f center;
  plane.intersect(line, center);
  startPt -= center;
  projPt -= center;
  // just normalize, null vector are ok (the angle will 0)
  (void) startPt.normalize();
  (void) projPt.normalize();
  SbVec3f dir = startPt.cross(projPt);
  float angle = static_cast<float>(acos(SbClamp(startPt.dot(projPt), -1.0f, 1.0f)));
  if (dir[2] < 0.0f) angle = -angle;

  this->setMotionMatrix(this->appendRotation(this->getStartMotionMatrix(),
                                             SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), angle),
                                             SbVec3f(0.0f, 0.0f, 0.0f)));
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoRotateDiscDragger::dragFinish(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
}

#undef THISP
#endif // HAVE_DRAGGERS
