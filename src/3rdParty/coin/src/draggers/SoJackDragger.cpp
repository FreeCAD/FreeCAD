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
  \class SoJackDragger SoJackDragger.h Inventor/draggers/SoJackDragger.h
  \brief The SoJackDragger class is a dragger you can translate, rotate and scale.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html jack.png "Screen Shot of Default Dragger"
  </center>

  Translation is done with this dragger by picking the flat
  transparent box ("x-z" translation) or the solid middle part of the
  axis ("y"-axis translation). Press a SHIFT-key while translating in
  x-z to constrain to one of the principal axes.

  Uniform scale operations can be done by dragging any of the 6
  cubes. Non-uniform scale operations cannot be done with this
  dragger.

  Rotations are invoked by clicking and dragging the line parts of the
  3 principal "axes" of the dragger geometry.

  <b>NB:</b> How to rotate the dragger might be a bit unintuitive for
  the end user. If the dragger is to be used 'out of the box', a good
  documentation should be added to your application. An alternative is
  to modify the geometry to increase the affordance of the dragger.
  See the SoDragger documentation on how to change the geometry of a
  dragger.

  An "action shot" of the SoJackDragger used within an SoClipPlaneManip:
  <center>
  \image html jack-in-action.png "Action Shot of Jack Dragger in a Clip Plane Manipulator"
  </center>

  The name "Jack dragger" probably stems from this dragger being
  implemented by SGI for interacting with a "jack-in-the-box" type
  model way back in the Inventor history.

  For programmer convenience, this dragger comes fully wrapped within
  instances of the SoJackManip class. The SoClipPlaneManip manipulator
  also uses this dragger, for controlling an SoClipPlane node.
*/

#include <Inventor/draggers/SoJackDragger.h>

#include <cstring>

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoRotateSphericalDragger.h>
#include <Inventor/draggers/SoScaleUniformDragger.h>
#include <Inventor/nodes/SoAntiSquish.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/jackDragger.h>

#include "nodekits/SoSubKitP.h"
#include "SbBasicP.h"
#include "coindefs.h"

/*!
  \var SoSFVec3f SoJackDragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position.

  The application programmer applying this dragger in his code should
  connect the relevant node fields in the scene to this field to make
  it follow the dragger.
*/

/*!
  \var SoSFRotation SoJackDragger::rotation

  This field is continuously updated to contain the rotation of the
  current direction vector of the dragger.
*/

/*!
  \var SoSFVec3f SoJackDragger::scaleFactor

  Continuously updated to contain the current vector of scaling along
  the X, Y and Z axes.

  Only uniform scaling can be done with the SoJackDragger, so the 3
  vector components will always be equal.
*/


/*!
  \var SoFieldSensor * SoJackDragger::translFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoJackDragger::rotFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoJackDragger::scaleFieldSensor
  \COININTERNAL
*/

class SoJackDraggerP {
public:
};

SO_KIT_SOURCE(SoJackDragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoJackDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoJackDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoJackDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "surroundScale"
  -->      "antiSquish"
  -->      "scaler"
  -->      "rotator"
  -->      "translator"
           "geomSeparator"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoJackDragger
  PVT   "this",  SoJackDragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
        "surroundScale",  SoSurroundScale  ---
        "antiSquish",  SoAntiSquish  ---
        "scaler",  SoScaleUniformDragger  ---
        "rotator",  SoRotateSphericalDragger  ---
        "translator",  SoDragPointDragger  ---
  PVT   "geomSeparator",  SoSeparator  ---
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoJackDragger::SoJackDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoJackDragger);

  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale, TRUE, topSeparator, antiSquish, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(antiSquish, SoAntiSquish, FALSE, topSeparator, scaler, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scaler, SoScaleUniformDragger, TRUE, topSeparator, rotator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoRotateSphericalDragger, TRUE, topSeparator, translator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, SoDragPointDragger, TRUE, topSeparator, geomSeparator, TRUE);


  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("jackDragger.iv",
                                       JACKDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(JACKDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));

  SO_KIT_INIT_INSTANCE();

  SO_GET_ANY_PART(this, "translator", SoDragPointDragger);
  SO_GET_ANY_PART(this, "rotator", SoRotateSphericalDragger);
  SO_GET_ANY_PART(this, "scaler", SoScaleUniformDragger);

  SoAntiSquish *squish = SO_GET_ANY_PART(this, "antiSquish", SoAntiSquish);
  squish->sizing = SoAntiSquish::BIGGEST_DIMENSION;
  // if the antisquish node to recalculate its parameters on every
  // traversal, rotating this dragger destroys its geometry
  squish->recalcAlways = FALSE;

  this->addValueChangedCallback(SoJackDragger::valueChangedCB);
  this->rotFieldSensor = new SoFieldSensor(SoJackDragger::fieldSensorCB, this);
  this->rotFieldSensor->setPriority(0);
  this->translFieldSensor = new SoFieldSensor(SoJackDragger::fieldSensorCB, this);
  this->translFieldSensor->setPriority(0);
  this->scaleFieldSensor = new SoFieldSensor(SoJackDragger::fieldSensorCB, this);
  this->scaleFieldSensor->setPriority(0);
  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoJackDragger::~SoJackDragger()
{
  delete this->rotFieldSensor;
  delete this->scaleFieldSensor;
  delete this->translFieldSensor;
}

// Doc in superclass.
SbBool
SoJackDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);
    SoDragger *child;
    child = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    child->setPartAsDefault("rotator",
                            "jackRotatorRotator");
    child->setPartAsDefault("rotatorActive",
                            "jackRotatorRotatorActive");
    child->setPartAsDefault("feedback",
                            "jackRotatorFeedback");
    this->addChildDragger(child);

    child = coin_assert_cast<SoDragger *>(this->getAnyPart("scaler", FALSE));
    child->setPartAsDefault("scaler", "jackScalerScaler");
    child->setPartAsDefault("scalerActive", "jackScalerScalerActive");
    child->setPartAsDefault("feedback", "jackScalerFeedback");
    child->setPartAsDefault("feedbackActive", "jackScalerFeedbackActive");
    this->addChildDragger(child);

    child = coin_assert_cast<SoDragger *>(this->getAnyPart("translator", FALSE));
    child->setPartAsDefault("xTranslator.translator", "jackTranslatorLineTranslator");
    child->setPartAsDefault("yTranslator.translator", "jackTranslatorLineTranslator");
    child->setPartAsDefault("zTranslator.translator", "jackTranslatorLineTranslator");
    child->setPartAsDefault("xTranslator.translatorActive", "jackTranslatorLineTranslatorActive");
    child->setPartAsDefault("yTranslator.translatorActive", "jackTranslatorLineTranslatorActive");
    child->setPartAsDefault("zTranslator.translatorActive", "jackTranslatorLineTranslatorActive");
    child->setPartAsDefault("xzTranslator.translator", "jackTranslatorPlaneTranslator");
    child->setPartAsDefault("xyTranslator.translator", "jackTranslatorPlaneTranslator");
    child->setPartAsDefault("yzTranslator.translator", "jackTranslatorPlaneTranslator");
    child->setPartAsDefault("xzTranslator.translatorActive", "jackTranslatorPlaneTranslatorActive");
    child->setPartAsDefault("xyTranslator.translatorActive", "jackTranslatorPlaneTranslatorActive");
    child->setPartAsDefault("yzTranslator.translatorActive", "jackTranslatorPlaneTranslatorActive");
    child->setPartAsDefault("xFeedback", "jackTranslatorXFeedback");
    child->setPartAsDefault("yFeedback", "jackTranslatorYFeedback");
    child->setPartAsDefault("zFeedback", "jackTranslatorZFeedback");
    child->setPartAsDefault("xzFeedback", "jackTranslatorXZFeedback");
    child->setPartAsDefault("xyFeedback", "jackTranslatorXYFeedback");
    child->setPartAsDefault("yzFeedback", "jackTranslatorYZFeedback");
    this->addChildDragger(child);

    if (this->rotFieldSensor->getAttachedField() != &this->rotation) {
      this->rotFieldSensor->attach(&this->rotation);
    }
    if (this->scaleFieldSensor->getAttachedField() != &this->scaleFactor) {
      this->scaleFieldSensor->attach(&this->scaleFactor);
    }
    if (this->translFieldSensor->getAttachedField() != &this->translation) {
      this->translFieldSensor->attach(&this->translation);
    }
  }
  else {
    this->removeChildDragger("rotator");
    this->removeChildDragger("scaler");
    this->removeChildDragger("translator");
    if (this->rotFieldSensor->getAttachedField() != NULL) {
      this->rotFieldSensor->detach();
    }
    if (this->translFieldSensor->getAttachedField() != NULL) {
      this->translFieldSensor->detach();
    }
    if (this->scaleFieldSensor->getAttachedField() != NULL) {
      this->scaleFieldSensor->detach();
    }
    inherited::setUpConnections(onoff, doitalways);
  }
  return !(this->connectionsSetUp = onoff);
}

// Doc in superclass.
void
SoJackDragger::setDefaultOnNonWritingFields(void)
{
  this->translator.setDefault(TRUE);
  this->rotator.setDefault(TRUE);
  this->scaler.setDefault(TRUE);

  this->antiSquish.setDefault(TRUE);
  this->surroundScale.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoJackDragger::fieldSensorCB(void * d, SoSensor *)
{
  SoJackDragger * thisp = static_cast<SoJackDragger *>(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoJackDragger::valueChangedCB(void *, SoDragger * d)
{
  SoJackDragger * thisp = static_cast<SoJackDragger *>(d);
  SbMatrix matrix = thisp->getMotionMatrix();

  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  matrix.getTransform(trans, rot, scale, scaleOrient);

  thisp->translFieldSensor->detach();
  if (thisp->translation.getValue() != trans)
    thisp->translation = trans;
  thisp->translFieldSensor->attach(&thisp->translation);

  thisp->rotFieldSensor->detach();
  if (thisp->rotation.getValue() != rot)
    thisp->rotation = rot;
  thisp->rotFieldSensor->attach(&thisp->rotation);

  thisp->scaleFieldSensor->detach();
  if (thisp->scaleFactor.getValue() != scale)
    thisp->scaleFactor = scale;
  thisp->scaleFieldSensor->attach(&thisp->scaleFactor);
}

/*!
  Used to invalidate the \e surroundScale and \e antiSquish parts when
  a child dragger starts or finishes dragging.
*/
void
SoJackDragger::invalidateSurroundScaleCB(void * f, SoDragger * COIN_UNUSED_ARG(d))
{
  SoJackDragger * thisp = static_cast<SoJackDragger *>(f);
  SoSurroundScale * surround = SO_CHECK_PART(thisp, "surroundScale", SoSurroundScale);
  if (surround) surround->invalidate();
  SoAntiSquish *squish = SO_CHECK_PART(thisp, "antiSquish", SoAntiSquish);
  if (squish) squish->recalc();
}

//
// convenience method that takes care of callbacks before registering child
//
void
SoJackDragger::addChildDragger(SoDragger *child)
{
  child->addStartCallback(SoJackDragger::invalidateSurroundScaleCB, this);
  child->addFinishCallback(SoJackDragger::invalidateSurroundScaleCB, this);
  this->registerChildDragger(child);
}

//
// convenience method that removes callbacks before unregistering child
//
void
SoJackDragger::removeChildDragger(const char *childname)
{
  SoDragger * child = coin_assert_cast<SoDragger *>(this->getAnyPart(childname, FALSE));
  child->removeStartCallback(SoJackDragger::invalidateSurroundScaleCB, this);
  child->removeFinishCallback(SoJackDragger::invalidateSurroundScaleCB, this);
  this->unregisterChildDragger(child);
}

#endif // HAVE_DRAGGERS
