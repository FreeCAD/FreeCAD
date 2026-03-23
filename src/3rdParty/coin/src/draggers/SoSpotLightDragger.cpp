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
  \class SoSpotLightDragger SoSpotLightDragger.h Inventor/draggers/SoSpotLightDragger.h
  \brief The SoSpotLightDragger class provides interactive geometry for manipulating a spotlight.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html spotlight.png "Screen Shot of Default Dragger"
  </center>

  This dragger is well suited for setting up the fields of a
  SoSpotLight node, as it provides geometry for the end-user to
  interact with a directional vector for the spotlight, to set up its
  position and to control the cut-off angle for the "lampshade" around
  the light source.

  Note that there is one aspect of SoSpotLight nodes that cannot be
  controlled with this dragger: the SoSpotLight::dropOffRate field.

  The Coin library includes a manipulator class, SoSpotLightManip,
  which wraps the functionality provided by this class inside the
  necessary mechanisms for connecting it to SoSpotLight node instances
  in a scene graph.

  \sa SoSpotLightManip, SoSpotLight
  \sa SoDirectionalLightDragger, SoPointLightDragger
*/

#include <Inventor/draggers/SoSpotLightDragger.h>

#include <cmath>
#include <cstring>

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoRotateSphericalDragger.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/projectors/SbPlaneProjector.h>

#include <data/draggerDefaults/spotLightDragger.h>

#include "nodekits/SoSubKitP.h"
#include "SbBasicP.h"

/*!
  \var SoSFRotation SoSpotLightDragger::rotation

  This field is continuously updated to contain the rotation of the
  current direction vector. The application programmer will typically
  connect this to the rotation field of a SoSpotLight node (unless
  using the SoSpotLightManip class, where this is taken care of
  automatically).

  It may also of course be connected to any other rotation field
  controlling the direction of scene graph geometry, it does not have
  to part of a SoSpotLight node specifically.
*/

/*!
  \var SoSFVec3f SoSpotLightDragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position.
*/

/*!
  \var SoSFFloat SoSpotLightDragger::angle

  The cut-off angle for the "lampshade" around the light source.

  Typically connected to a SoSpotLight::cutOffAngle field.
*/

/*!
  \var SoFieldSensor * SoSpotLightDragger::rotFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoSpotLightDragger::translFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoSpotLightDragger::angleFieldSensor
  \COININTERNAL
*/
/*!
  \var SbPlaneProjector * SoSpotLightDragger::planeProj
  \COININTERNAL
*/

#define THISP(d) static_cast<SoSpotLightDragger *>(d)

class SoSpotLightDraggerP {
public:
};

SO_KIT_SOURCE(SoSpotLightDragger);

/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoSpotLightDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoSpotLightDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoSpotLightDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "material"
  -->      "translatorSep"
  -->         "translatorRotInv"
  -->         "translator"
  -->      "rotator"
  -->      "beamSep"
  -->         "beamPlacement"
  -->         "beamScale"
  -->         "beamSwitch"
  -->            "beam"
  -->            "beamActive"
           "geomSeparator"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoSpotLightDragger
  PVT   "this",  SoSpotLightDragger  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
  PVT   "motionMatrix",  SoMatrixTransform  --- 
        "material",  SoMaterial  --- 
  PVT   "translatorSep",  SoSeparator  --- 
  PVT   "translatorRotInv",  SoRotation  --- 
        "translator",  SoDragPointDragger  --- 
        "rotator",  SoRotateSphericalDragger  --- 
  PVT   "beamSep",  SoSeparator  --- 
        "beamPlacement",  SoTranslation  --- 
        "beamScale",  SoScale  --- 
  PVT   "beamSwitch",  SoSwitch  --- 
        "beam",  SoSeparator  --- 
        "beamActive",  SoSeparator  --- 
  PVT   "geomSeparator",  SoSeparator  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoSpotLightDragger::SoSpotLightDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoSpotLightDragger);

  SO_KIT_ADD_CATALOG_ENTRY(material, SoMaterial, TRUE, topSeparator, translatorSep, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorSep, SoSeparator, TRUE, topSeparator, rotator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorRotInv, SoRotation, TRUE, translatorSep, translator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, SoDragPointDragger, TRUE, translatorSep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoRotateSphericalDragger, TRUE, topSeparator, beamSep, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(beamSep, SoSeparator, TRUE, topSeparator, geomSeparator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(beamPlacement, SoTranslation, TRUE, beamSep, beamScale, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(beamScale, SoScale, TRUE, beamSep, beamSwitch, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(beamSwitch, SoSwitch, TRUE, beamSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(beam, SoSeparator, TRUE, beamSwitch, beamActive, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(beamActive, SoSeparator, TRUE, beamSwitch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("spotLightDragger.iv",
                                       SPOTLIGHTDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(SPOTLIGHTDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(angle, (1.0f));
  SO_KIT_INIT_INSTANCE();

  SoDragger *pdragger = SO_GET_ANY_PART(this, "translator", SoDragPointDragger);
  assert(pdragger);
  SoDragger *sdragger = SO_GET_ANY_PART(this, "rotator", SoDragPointDragger);
  assert(sdragger);

  this->setPartAsDefault("beam", "spotLightBeam");
  this->setPartAsDefault("beamActive", "spotLightBeamActive");
  this->setPartAsDefault("beamPlacement", "spotLightBeamPlacement");
  this->setPartAsDefault("material", "spotLightOverallMaterial");

  // create this part here so that we don't add this node to the scene
  // graph during handleEvent() (causes changes to the scene graph
  // during traversal)
  (void)SO_GET_ANY_PART(this, "translatorRotInv", SoRotation);

  SoSwitch * sw = SO_GET_ANY_PART(this, "beamSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);

  this->setBeamScaleFromAngle(1.0f);

  this->planeProj = new SbPlaneProjector();
  this->addStartCallback(SoSpotLightDragger::startCB);
  this->addMotionCallback(SoSpotLightDragger::motionCB);
  this->addFinishCallback(SoSpotLightDragger::doneCB);
  this->addValueChangedCallback(SoSpotLightDragger::valueChangedCB);

  this->rotFieldSensor = new SoFieldSensor(SoSpotLightDragger::fieldSensorCB, this);
  this->rotFieldSensor->setPriority(0);
  this->translFieldSensor = new SoFieldSensor(SoSpotLightDragger::fieldSensorCB, this);
  this->translFieldSensor->setPriority(0);
  this->angleFieldSensor = new SoFieldSensor(SoSpotLightDragger::fieldSensorCB, this);
  this->angleFieldSensor->setPriority(0);

  this->translatorSep.setDefault(TRUE);
  this->beamSep.setDefault(TRUE);

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoSpotLightDragger::~SoSpotLightDragger()
{
  delete this->angleFieldSensor;
  delete this->translFieldSensor;
  delete this->rotFieldSensor;
  delete this->planeProj;
}

// Doc in superclass.
SbBool
SoSpotLightDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);
    SoDragger * therotator = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    therotator->setPartAsDefault("rotator", "spotLightRotatorRotator");
    therotator->setPartAsDefault("rotatorActive",
                              "spotLightRotatorRotatorActive");
    therotator->setPartAsDefault("feedback",
                              "spotLightRotatorFeedback");
    therotator->setPartAsDefault("feedbackActive",
                              "spotLightRotatorFeedbackActive");

    SoDragger *thetranslator = coin_assert_cast<SoDragger *>(this->getAnyPart("translator", FALSE));
    thetranslator->setPartAsDefault("yzTranslator.translator",
                                 "spotLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("xzTranslator.translator",
                                 "spotLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("xyTranslator.translator",
                                 "spotLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("yzTranslator.translatorActive",
                                 "spotLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xzTranslator.translatorActive",
                                 "spotLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xyTranslator.translatorActive",
                                 "spotLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xTranslator.translator",
                                 "spotLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("yTranslator.translator",
                                 "spotLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("zTranslator.translator",
                                 "spotLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("xTranslator.translatorActive",
                                 "spotLightTranslatorLineTranslatorActive");
    thetranslator->setPartAsDefault("yTranslator.translatorActive",
                                 "spotLightTranslatorLineTranslatorActive");
    thetranslator->setPartAsDefault("zTranslator.translatorActive",
                                 "spotLightTranslatorLineTranslatorActive");

    this->registerChildDragger(therotator);
    this->registerChildDragger(thetranslator);

    if (this->angleFieldSensor->getAttachedField() != &this->angle)
      this->angleFieldSensor->attach(&this->angle);
    if (this->translFieldSensor->getAttachedField() != &this->translation)
      this->translFieldSensor->attach(&this->translation);
    if (this->rotFieldSensor->getAttachedField() != &this->rotation)
      this->rotFieldSensor->attach(&this->rotation);
  }
  else {
    SoDragger *thetranslator = coin_assert_cast<SoDragger *>(this->getAnyPart("translator", FALSE));
    this->unregisterChildDragger(thetranslator);
    SoDragger * therotator = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    this->unregisterChildDragger(therotator);

    if (this->angleFieldSensor->getAttachedField() != NULL)
      this->angleFieldSensor->detach();
    if (this->rotFieldSensor->getAttachedField() != NULL)
      this->rotFieldSensor->detach();
    if (this->translFieldSensor->getAttachedField() != NULL)
      this->translFieldSensor->detach();

    inherited::setUpConnections(onoff, doitalways);
  }
  return !(this->connectionsSetUp = onoff);
}

// Doc in superclass.
void
SoSpotLightDragger::setDefaultOnNonWritingFields(void)
{
  if (!(this->angle.isConnectionEnabled() && this->angle.isConnected()) &&
      this->angle.getValue() == 1.0f) this->angle.setDefault(TRUE);
  
  this->translator.setDefault(TRUE);
  this->rotator.setDefault(TRUE);
  
  this->translatorRotInv.setDefault(TRUE);
  this->beamScale.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoSpotLightDragger::fieldSensorCB(void * d, SoSensor * s)
{
  SoSpotLightDragger * thisp = THISP(d);
 
  if (s == static_cast<SoSensor *>(thisp->angleFieldSensor)) {
    // need to update the beamScale geometry
    SoScale * scale = SO_GET_ANY_PART(thisp, "beamScale", SoScale);
    SbVec3f scaleFactor;
    scaleFactor[0] = scaleFactor[1] = static_cast<float>(tan(thisp->angle.getValue()));
    scaleFactor[2] = 1.0f;
    scale->scaleFactor = scaleFactor;
    thisp->valueChanged();
  }
  else {
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform(matrix);
    thisp->setMotionMatrix(matrix);
  }
}

/*! \COININTERNAL */
void
SoSpotLightDragger::valueChangedCB(void *, SoDragger * d)
{
  SoSpotLightDragger * thisp = THISP(d);
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

  SoRotation *invRot = SO_GET_ANY_PART(thisp, "translatorRotInv", SoRotation);
  invRot->rotation = rot.inverse();
}

/*! \COININTERNAL */
void
SoSpotLightDragger::startCB(void *, SoDragger * d)
{
  SoSpotLightDragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoSpotLightDragger::motionCB(void *, SoDragger * d)
{
  SoSpotLightDragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoSpotLightDragger::doneCB(void *, SoDragger * d)
{
  SoSpotLightDragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoSpotLightDragger::dragStart(void)
{
  if (this->getActiveChildDragger()) return;
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "beamSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 1);

  SbVec3f hitPt = this->getLocalStartingPoint();
  SbVec3f apex = SO_GET_ANY_PART(this, "beamPlacement", SoTranslation)->translation.getValue();
  apex[2] += 1.0f; // FIXME: This should probably be handled another way. 20020814 kristian.

  this->planeProj->setPlane(SbPlane(apex, apex+SbVec3f(0.0f, 0.0f, -1.0f), 
                                    hitPt));
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoSpotLightDragger::drag(void)
{
  if (this->getActiveChildDragger()) return;

  SbVec3f apex = SO_GET_ANY_PART(this, "beamPlacement", SoTranslation)->translation.getValue();
  apex[2] += 1.0f; // FIXME: This should probably be handled another way. 20020814 kristian.
  this->planeProj->setViewVolume(this->getViewVolume());
  this->planeProj->setWorkingSpace(this->getLocalToWorldMatrix());
  SbVec3f projPt = planeProj->project(this->getNormalizedLocaterPosition());

  SbVec3f vec = projPt - apex;
  float dot = SbVec3f(0.0f, 0.0f, -1.0f).dot(vec) / vec.length();
  if (dot < 0.0f) dot = 0.01f; // FIXME: pederb, 20000209
  this->setBeamScaleFromAngle(static_cast<float>(acos(dot)));
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoSpotLightDragger::dragFinish(void)
{
  if (this->getActiveChildDragger()) return;
  SoSwitch *sw;
  sw = SO_GET_ANY_PART(this, "beamSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, 0);
}

/*!
  Scales the geometry representing the "lampshade" around the
  light source to represent the given \a beamangle.
 */
void
SoSpotLightDragger::setBeamScaleFromAngle(float beamangle)
{
  SoScale *scale = SO_GET_ANY_PART(this, "beamScale", SoScale);
  SbVec3f scaleFactor;
  scaleFactor[0] = scaleFactor[1] = static_cast<float>(tan(beamangle));
  scaleFactor[2] = 1.0f;
  scale->scaleFactor = scaleFactor;
  this->angle = beamangle;
}

#undef THISP
#endif // HAVE_DRAGGERS
