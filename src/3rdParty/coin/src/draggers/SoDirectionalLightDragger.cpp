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
  \class SoDirectionalLightDragger SoDirectionalLightDragger.h Inventor/draggers/SoDirectionalLightDragger.h
  \brief The SoDirectionalLightDragger class provides interactive geometry for manipulating a directional light source.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html directionallight.png "Screen Shot of Default Dragger"
  </center>

  This dragger is well suited to use for setting up the fields of a
  SoDirectionalLight node, as it provides geometry for the end-user to
  interact with a directional vector.

  The image below is an action shot example, with the directional light being red
  and the camera headlight turned right down.

  <center>
  \image html directionallightdragger_actionshot.png "DirectionalLight Dragger Action Shot"
  </center>

  For convenience, this dragger also by default contains interaction
  geometry for placing the dragger itself. (SoDirectionalLight nodes
  don't have a position field, so this was strictly not needed.)

  The Coin library also includes a manipulator class,
  SoDirectionalLightManip, which wraps the functionality provided by
  this class inside the necessary mechanisms for connecting it to
  SoDirectionalLight node instances in a scene graph.

  \sa SoDirectionalLightManip
*/

#include <Inventor/draggers/SoDirectionalLightDragger.h>

#include <cstring>

#include <Inventor/draggers/SoDragPointDragger.h>
#include <Inventor/draggers/SoRotateSphericalDragger.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/directionalLightDragger.h>

#include "nodekits/SoSubKitP.h"
#include "SbBasicP.h"

/*!
  \var SoSFRotation SoDirectionalLightDragger::rotation

  This field is continuously updated to contain the rotation of the
  current direction vector. The application programmer will typically
  connect this to the rotation field of a SoDirectionalLight node
  (unless using the SoDirectionalLightManip class, where this is taken
  care of automatically).

  It may also of course be connected to any other rotation field
  controlling the direction of scene graph geometry, it does not have
  to part of a SoDirectionalLight node specifically.
*/
/*!
  \var SoSFVec3f SoDirectionalLightDragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position.

  This field is not used by the SoDirectionalLightManip, but may be of
  interest for the application programmer wanting to use the
  SoDirectionalLightDragger outside the context of controlling a
  directional light node.
*/

/*!
  \var SoFieldSensor * SoDirectionalLightDragger::rotFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoDirectionalLightDragger::translFieldSensor
  \COININTERNAL
*/

class SoDirectionalLightDraggerP {
public:
};

SO_KIT_SOURCE(SoDirectionalLightDragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoDirectionalLightDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoDirectionalLightDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoDirectionalLightDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "material"
  -->      "translatorSep"
  -->         "translatorRotInv"
  -->         "translator"
  -->      "rotator"
           "geomSeparator"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoDirectionalLightDragger
  PVT   "this",  SoDirectionalLightDragger  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
  PVT   "motionMatrix",  SoMatrixTransform  --- 
        "material",  SoMaterial  --- 
  PVT   "translatorSep",  SoSeparator  --- 
        "translatorRotInv",  SoRotation  --- 
        "translator",  SoDragPointDragger  --- 
        "rotator",  SoRotateSphericalDragger  --- 
  PVT   "geomSeparator",  SoSeparator  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoDirectionalLightDragger::SoDirectionalLightDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoDirectionalLightDragger);

  SO_KIT_ADD_CATALOG_ENTRY(material, SoMaterial, TRUE, topSeparator, translatorSep, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoRotateSphericalDragger, TRUE, topSeparator, geomSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, SoDragPointDragger, TRUE, translatorSep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorRotInv, SoRotation, TRUE, translatorSep, translator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorSep, SoSeparator, TRUE, topSeparator, rotator, FALSE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("directionalLightDragger.iv",
                                       DIRECTIONALLIGHTDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(DIRECTIONALLIGHTDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_INIT_INSTANCE();

  SoDragger *pdragger = SO_GET_ANY_PART(this, "translator", SoDragPointDragger);
  assert(pdragger);
  SoDragger *sdragger = SO_GET_ANY_PART(this, "rotator", SoDragPointDragger);
  assert(sdragger);

  this->setPartAsDefault("material", "directionalLightOverallMaterial");

  this->addValueChangedCallback(SoDirectionalLightDragger::valueChangedCB);

  this->rotFieldSensor = new SoFieldSensor(SoDirectionalLightDragger::fieldSensorCB, this);
  this->rotFieldSensor->setPriority(0);
  this->translFieldSensor = new SoFieldSensor(SoDirectionalLightDragger::fieldSensorCB, this);
  this->translFieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);

  // create this part to avoid changes in the scene graph while traversing it
  (void) SO_GET_ANY_PART(this, "translatorRotInv", SoRotation);

  this->translatorSep.setDefault(TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoDirectionalLightDragger::~SoDirectionalLightDragger()
{
  delete this->translFieldSensor;
  delete this->rotFieldSensor;
}

// doc in superclass
SbBool
SoDirectionalLightDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);
    SoDragger * therotator = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    therotator->setPartAsDefault("rotator", "directionalLightRotatorRotator");
    therotator->setPartAsDefault("rotatorActive",
                              "directionalLightRotatorRotatorActive");
    therotator->setPartAsDefault("feedback",
                              "directionalLightRotatorFeedback");
    therotator->setPartAsDefault("feedbackActive",
                              "directionalLightRotatorFeedbackActive");

    SoDragger *thetranslator = coin_assert_cast<SoDragger *>(this->getAnyPart("translator", FALSE));
    thetranslator->setPartAsDefault("yzTranslator.translator",
                                 "directionalLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("xzTranslator.translator",
                                 "directionalLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("xyTranslator.translator",
                                 "directionalLightTranslatorPlaneTranslator");
    thetranslator->setPartAsDefault("yzTranslator.translatorActive",
                                 "directionalLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xzTranslator.translatorActive",
                                 "directionalLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xyTranslator.translatorActive",
                                 "directionalLightTranslatorPlaneTranslatorActive");
    thetranslator->setPartAsDefault("xTranslator.translator",
                                 "directionalLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("yTranslator.translator",
                                 "directionalLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("zTranslator.translator",
                                 "directionalLightTranslatorLineTranslator");
    thetranslator->setPartAsDefault("xTranslator.translatorActive",
                                 "directionalLightTranslatorLineTranslatorActive");
    thetranslator->setPartAsDefault("yTranslator.translatorActive",
                                 "directionalLightTranslatorLineTranslatorActive");
    thetranslator->setPartAsDefault("zTranslator.translatorActive",
                                 "directionalLightTranslatorLineTranslatorActive");

    this->registerChildDragger(therotator);
    this->registerChildDragger(thetranslator);

    if (this->translFieldSensor->getAttachedField() != &this->translation)
      this->translFieldSensor->attach(&this->translation);
    if (this->rotFieldSensor->getAttachedField() != &this->rotation)
      this->rotFieldSensor->attach(&this->rotation);
  }
  else {
    SoDragger * thetranslator = coin_assert_cast<SoDragger *>(this->getAnyPart("translator", FALSE));
    this->unregisterChildDragger(thetranslator);
    SoDragger * therotator = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    this->unregisterChildDragger(therotator);

    if (this->rotFieldSensor->getAttachedField() != NULL)
      this->rotFieldSensor->detach();
    if (this->translFieldSensor->getAttachedField() != NULL)
      this->translFieldSensor->detach();

    inherited::setUpConnections(onoff, doitalways);
  }
  return !(this->connectionsSetUp = onoff);
}

// doc in superclass
void
SoDirectionalLightDragger::setDefaultOnNonWritingFields(void)
{
  this->translator.setDefault(TRUE);
  this->rotator.setDefault(TRUE);
  this->translatorRotInv.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoDirectionalLightDragger::fieldSensorCB(void * d, SoSensor *)
{
  SoDirectionalLightDragger * thisp = static_cast<SoDirectionalLightDragger *>(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoDirectionalLightDragger::valueChangedCB(void *, SoDragger * d)
{
  SoDirectionalLightDragger *thisp = static_cast<SoDirectionalLightDragger *>(d);
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

#endif // HAVE_DRAGGERS
