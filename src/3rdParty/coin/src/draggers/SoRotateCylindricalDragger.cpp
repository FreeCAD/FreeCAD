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
  \class SoRotateCylindricalDragger SoRotateCylindricalDragger.h Inventor/draggers/SoRotateCylindricalDragger.h
  \brief The SoRotateCylindricalDragger class is for rotating geometry around a single axis.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html rotatecylindrical.png "Screen Shot of Default Dragger"
  </center>

  Use an instance of this dragger class in your scene graph to let the
  end-users of your application rotate geometry around a predefined
  axis vector in 3D.

  For the dragger orientation and positioning itself, use some kind of
  transformation node in your scene graph, as usual.
*/

#include <Inventor/draggers/SoRotateCylindricalDragger.h>

#include <cstring>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/projectors/SbCylinderPlaneProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/rotateCylindricalDragger.h>

#include "nodekits/SoSubKitP.h"
#include "coindefs.h"
#include "SbBasicP.h"

#define THISP(d) static_cast<SoRotateCylindricalDragger *>(d)

class SoRotateCylindricalDraggerP {
public:
};

SO_KIT_SOURCE(SoRotateCylindricalDragger);

/*!
  \var SoSFRotation SoRotateCylindricalDragger::rotation

  This field is continuously updated to contain the rotation of the
  current direction vector of the dragger.

  The application programmer using this dragger in his scene graph
  should connect the relevant node fields in the scene to this field
  to make them follow the dragger orientation.
*/

/*!
  \var SoFieldSensor * SoRotateCylindricalDragger::fieldSensor
  \COININTERNAL
*/
/*!
  \var SbCylinderProjector * SoRotateCylindricalDragger::cylinderProj
  \COININTERNAL
*/
/*!
  \var SbBool SoRotateCylindricalDragger::userProj
  \COININTERNAL
*/


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoRotateCylindricalDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoRotateCylindricalDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoRotateCylindricalDragger
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
  CLASS SoRotateCylindricalDragger
  PVT   "this",  SoRotateCylindricalDragger  ---
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
SoRotateCylindricalDragger::SoRotateCylindricalDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoRotateCylindricalDragger);

  SO_KIT_ADD_CATALOG_ENTRY(rotatorSwitch, SoSwitch, TRUE, geomSeparator, feedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoSeparator, TRUE, rotatorSwitch, rotatorActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotatorActive, SoSeparator, TRUE, rotatorSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, TRUE, feedbackSwitch, feedbackActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(feedbackActive, SoSeparator, TRUE, feedbackSwitch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("rotateCylindricalDragger.iv",
                                       ROTATECYLINDRICALDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(ROTATECYLINDRICALDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_INIT_INSTANCE();

  // initialize default parts
  this->setPartAsDefault("rotator", "rotateCylindricalRotator");
  this->setPartAsDefault("rotatorActive", "rotateCylindricalRotatorActive");
  this->setPartAsDefault("feedback", "rotateCylindricalFeedback");
  this->setPartAsDefault("feedbackActive", "rotateCylindricalFeedbackActive");

  // initialize switch values
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);

  // setup projector
  this->cylinderProj = new SbCylinderPlaneProjector();
  this->userProj = FALSE;
  this->addStartCallback(SoRotateCylindricalDragger::startCB);
  this->addMotionCallback(SoRotateCylindricalDragger::motionCB);
  this->addFinishCallback(SoRotateCylindricalDragger::doneCB);

  this->addValueChangedCallback(SoRotateCylindricalDragger::valueChangedCB);

  this->fieldSensor = new SoFieldSensor(SoRotateCylindricalDragger::fieldSensorCB, this);
  this->fieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoRotateCylindricalDragger::~SoRotateCylindricalDragger()
{
  delete this->fieldSensor;
  if (!this->userProj) delete this->cylinderProj;
}

// Doc in superclass.
SbBool
SoRotateCylindricalDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  SbBool oldval = this->connectionsSetUp;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoRotateCylindricalDragger::fieldSensorCB(this, NULL);

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
SoRotateCylindricalDragger::fieldSensorCB(void * d, SoSensor *)
{
  assert(d);
  SoRotateCylindricalDragger * thisp = THISP(d);
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
SoRotateCylindricalDragger::valueChangedCB(void *, SoDragger * d)
{
  SoRotateCylindricalDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();

  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  matrix.getTransform(trans, rot, scale, scaleOrient);
  thisp->fieldSensor->detach();
  if (thisp->rotation.getValue() != rot)
    thisp->rotation = rot;
  thisp->fieldSensor->attach(&thisp->rotation);
}

/*!
  Replace the default cylinder projection strategy. You may want to do
  this if you change the dragger geometry, for instance.

  The default cylinder projection is an SbCylinderPlaneProjector.

  \sa SbCylinderSectionProjector, SbCylinderSheetProjector
*/
void
SoRotateCylindricalDragger::setProjector(SbCylinderProjector * p)
{
  if (!this->userProj) delete this->cylinderProj;
  this->cylinderProj = p;
}

/*!
  Returns projector instance used for converting from user interaction
  dragger movements to 3D dragger re-orientation.

  \sa setProjector()
*/
const SbCylinderProjector *
SoRotateCylindricalDragger::getProjector(void) const
{
  return this->cylinderProj;
}

// Doc in superclass.
void
SoRotateCylindricalDragger::copyContents(const SoFieldContainer * fromfc,
                                         SbBool copyconnections)
{
  inherited::copyContents(fromfc, copyconnections);

  assert(fromfc->isOfType(SoRotateCylindricalDragger::getClassTypeId()));
  const SoRotateCylindricalDragger * from = coin_assert_cast<const SoRotateCylindricalDragger *>(fromfc);
  if (!this->userProj) {
    delete this->cylinderProj;
  }
  this->cylinderProj = NULL;

  if (from->cylinderProj) {
    this->cylinderProj = static_cast<SbCylinderProjector *>(
      from->cylinderProj->copy()
      );
  }
  else {
    // just create a new one
    this->cylinderProj = new SbCylinderPlaneProjector();
  }
  // we copied or created a new one, and need to delete it.
  this->userProj = FALSE;
}

/*! \COININTERNAL */
void
SoRotateCylindricalDragger::startCB(void * COIN_UNUSED_ARG(f), SoDragger * d)
{
  SoRotateCylindricalDragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoRotateCylindricalDragger::motionCB(void * COIN_UNUSED_ARG(f), SoDragger * d)
{
  SoRotateCylindricalDragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoRotateCylindricalDragger::doneCB(void * COIN_UNUSED_ARG(f), SoDragger * d)
{
  SoRotateCylindricalDragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoRotateCylindricalDragger::dragStart(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);

  SbVec3f hitPt = this->getLocalStartingPoint();
  SbLine line(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f));
  SbVec3f ptOnLine = line.getClosestPoint(hitPt);
  this->cylinderProj->setCylinder(SbCylinder(line, (ptOnLine-hitPt).length()));

  this->cylinderProj->setViewVolume(this->getViewVolume());
  this->cylinderProj->setWorkingSpace(this->getLocalToWorldMatrix());

  switch (this->getFrontOnProjector()) {
  case FRONT:
  case BACK:
    this->cylinderProj->setFront(TRUE);
    break;
  default: // avoid warnings
  case USE_PICK:
    this->cylinderProj->setFront(this->cylinderProj->isPointInFront(hitPt));
    break;
  }

}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoRotateCylindricalDragger::drag(void)
{
  this->cylinderProj->setViewVolume(this->getViewVolume());
  this->cylinderProj->setWorkingSpace(this->getLocalToWorldMatrix());

  SbVec3f projPt = cylinderProj->project(this->getNormalizedLocaterPosition());
  SbVec3f startPt = this->getLocalStartingPoint();

  SbRotation rot = this->cylinderProj->getRotation(startPt, projPt);

  this->setMotionMatrix(this->appendRotation(this->getStartMotionMatrix(),
                                             rot, SbVec3f(0.0f, 0.0f, 0.0f)));
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoRotateCylindricalDragger::dragFinish(void)
{
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "rotatorSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
  sw = SO_GET_ANY_PART(this, "feedbackSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
}

#undef THISP
#endif // HAVE_DRAGGERS
