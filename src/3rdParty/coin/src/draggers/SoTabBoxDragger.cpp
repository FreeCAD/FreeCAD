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
  \class SoTabBoxDragger SoTabBoxDragger.h Inventor/draggers/SoTabBoxDragger.h
  \brief The SoTabBoxDragger wraps a box around geometry you can then translate and scale.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html tabbox.png "Screen Shot of Default Dragger"
  </center>

  This dragger lets the end-user do translation and non-uniform
  scaling of geometry with an easy to understand interface.

  Click and drag any side of the box to translate (hold down a SHIFT
  key to lock to one axis) and click and drag any of the tab markers
  in the corners to scale. The way the different tabs influences the
  scale operation should be straight-forward and intuitive to the
  end-user.

  The SoTabBoxDragger is a composite dragger, implemented with 6
  SoTabPlaneDragger instances set up as the sides of a box.
*/

#include <Inventor/draggers/SoTabBoxDragger.h>

#include <cstring>

#include <Inventor/draggers/SoTabPlaneDragger.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/tabBoxDragger.h>

#include "nodekits/SoSubKitP.h"
#include "SbBasicP.h"

/*!
  \var SoSFVec3f SoTabBoxDragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position.
*/
/*!
  \var SoSFVec3f SoTabBoxDragger::scaleFactor

  Continuously updated to contain the current vector of scaling along
  the X, Y and Z axes.
*/

/*!
  \var SoFieldSensor * SoTabBoxDragger::translFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoTabBoxDragger::scaleFieldSensor
  \COININTERNAL
*/

#define THISP(d) static_cast<SoTabBoxDragger *>(d)

class SoTabBoxDraggerP {
public:
};

SO_KIT_SOURCE(SoTabBoxDragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoTabBoxDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoTabBoxDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoTabBoxDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "surroundScale"
  -->      "tabPlane1Sep"
  -->         "tabPlane1Xf"
  -->         "tabPlane1"
  -->      "tabPlane2Sep"
  -->         "tabPlane2Xf"
  -->         "tabPlane2"
  -->      "tabPlane3Sep"
  -->         "tabPlane3Xf"
  -->         "tabPlane3"
  -->      "tabPlane4Sep"
  -->         "tabPlane4Xf"
  -->         "tabPlane4"
  -->      "tabPlane5Sep"
  -->         "tabPlane5Xf"
  -->         "tabPlane5"
  -->      "tabPlane6Sep"
  -->         "tabPlane6Xf"
  -->         "tabPlane6"
           "geomSeparator"
  -->         "boxGeom"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoTabBoxDragger
  PVT   "this",  SoTabBoxDragger  --- 
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ] 
  PVT   "topSeparator",  SoSeparator  --- 
  PVT   "motionMatrix",  SoMatrixTransform  --- 
        "surroundScale",  SoSurroundScale  --- 
  PVT   "tabPlane1Sep",  SoSeparator  --- 
  PVT   "tabPlane1Xf",  SoTransform  --- 
        "tabPlane1",  SoTabPlaneDragger  --- 
  PVT   "tabPlane2Sep",  SoSeparator  --- 
  PVT   "tabPlane2Xf",  SoTransform  --- 
        "tabPlane2",  SoTabPlaneDragger  --- 
  PVT   "tabPlane3Sep",  SoSeparator  --- 
  PVT   "tabPlane3Xf",  SoTransform  --- 
        "tabPlane3",  SoTabPlaneDragger  --- 
  PVT   "tabPlane4Sep",  SoSeparator  --- 
  PVT   "tabPlane4Xf",  SoTransform  --- 
        "tabPlane4",  SoTabPlaneDragger  --- 
  PVT   "tabPlane5Sep",  SoSeparator  --- 
  PVT   "tabPlane5Xf",  SoTransform  --- 
        "tabPlane5",  SoTabPlaneDragger  --- 
  PVT   "tabPlane6Sep",  SoSeparator  --- 
  PVT   "tabPlane6Xf",  SoTransform  --- 
        "tabPlane6",  SoTabPlaneDragger  --- 
  PVT   "geomSeparator",  SoSeparator  --- 
        "boxGeom",  SoSeparator  --- 
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoTabBoxDragger::SoTabBoxDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoTabBoxDragger);

  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale, TRUE, topSeparator, tabPlane1Sep, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane1Sep, SoSeparator, FALSE, topSeparator, tabPlane2Sep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane1Xf, SoTransform, TRUE, tabPlane1Sep, tabPlane1, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane1, SoTabPlaneDragger, TRUE, tabPlane1Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane2Sep, SoSeparator, FALSE, topSeparator, tabPlane3Sep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane2Xf, SoTransform, TRUE, tabPlane2Sep, tabPlane2, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane2, SoTabPlaneDragger, TRUE, tabPlane2Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane3Sep, SoSeparator, FALSE, topSeparator, tabPlane4Sep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane3Xf, SoTransform, TRUE, tabPlane3Sep, tabPlane3, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane3, SoTabPlaneDragger, TRUE, tabPlane3Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane4Sep, SoSeparator, FALSE, topSeparator, tabPlane5Sep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane4Xf, SoTransform, TRUE, tabPlane4Sep, tabPlane4, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane4, SoTabPlaneDragger, TRUE, tabPlane4Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane5Sep, SoSeparator, FALSE, topSeparator, tabPlane6Sep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane5Xf, SoTransform, TRUE, tabPlane5Sep, tabPlane5, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane5, SoTabPlaneDragger, TRUE, tabPlane5Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane6Sep, SoSeparator, FALSE, topSeparator, geomSeparator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane6Xf, SoTransform, TRUE, tabPlane6Sep, tabPlane6, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tabPlane6, SoTabPlaneDragger, TRUE, tabPlane6Sep, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(boxGeom, SoSeparator, TRUE, geomSeparator, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("tabBoxDragger.iv",
                                       TABBOXDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(TABBOXDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));

  SO_KIT_INIT_INSTANCE();

  // create subdraggers
  SO_GET_ANY_PART(this, "tabPlane1", SoTabPlaneDragger);
  SO_GET_ANY_PART(this, "tabPlane2", SoTabPlaneDragger);
  SO_GET_ANY_PART(this, "tabPlane3", SoTabPlaneDragger);
  SO_GET_ANY_PART(this, "tabPlane4", SoTabPlaneDragger);
  SO_GET_ANY_PART(this, "tabPlane5", SoTabPlaneDragger);
  SO_GET_ANY_PART(this, "tabPlane6", SoTabPlaneDragger);

  this->setPartAsDefault("boxGeom", "tabBoxBoxGeom");

  this->initTransformNodes();

  this->addValueChangedCallback(SoTabBoxDragger::valueChangedCB);

  this->scaleFieldSensor = new SoFieldSensor(SoTabBoxDragger::fieldSensorCB, this);
  this->translFieldSensor = new SoFieldSensor(SoTabBoxDragger::fieldSensorCB, this);
  this->setUpConnections(TRUE, TRUE);
}


/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoTabBoxDragger::~SoTabBoxDragger()
{
  delete this->scaleFieldSensor;
  delete this->translFieldSensor;
}

// Doc in superclass.
SbBool
SoTabBoxDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  SbString str;
  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);
    SoDragger *child;
    for (int i = 1; i <= 6; i++) {
      str.sprintf("tabPlane%d", i);
      child = coin_assert_cast<SoDragger *>(this->getAnyPart(str.getString(), FALSE));
      child->setPartAsDefault("translator", "tabBoxTranslator");
      child->setPartAsDefault("scaleTabMaterial", "tabBoxScaleTabMaterial");
      child->setPartAsDefault("scaleTabHints", "tabBoxScaleTabHints");
      child->addStartCallback(SoTabBoxDragger::invalidateSurroundScaleCB, this);
      child->addFinishCallback(SoTabBoxDragger::invalidateSurroundScaleCB, this);
      this->registerChildDragger(child);
    }

    if (this->translFieldSensor->getAttachedField() != &this->translation) {
      this->translFieldSensor->attach(&this->translation);
    }
    if (this->scaleFieldSensor->getAttachedField() != &this->scaleFactor) {
      this->scaleFieldSensor->attach(&this->scaleFactor);
    }
  }
  else {
    SoDragger *child;
    for (int i = 1; i <= 6; i++) {
      str.sprintf("tabPlane%d", i);
      child = coin_assert_cast<SoDragger *>(this->getAnyPart(str.getString(), FALSE));
      child->removeStartCallback(SoTabBoxDragger::invalidateSurroundScaleCB, this);
      child->removeFinishCallback(SoTabBoxDragger::invalidateSurroundScaleCB, this);
      this->unregisterChildDragger(child);
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
SoTabBoxDragger::setDefaultOnNonWritingFields(void)
{
  this->surroundScale.setDefault(TRUE);

  this->tabPlane1.setDefault(TRUE);
  this->tabPlane2.setDefault(TRUE);
  this->tabPlane3.setDefault(TRUE);
  this->tabPlane4.setDefault(TRUE);
  this->tabPlane5.setDefault(TRUE);
  this->tabPlane6.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoTabBoxDragger::fieldSensorCB(void * d, SoSensor *)
{
  SoTabBoxDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoTabBoxDragger::valueChangedCB(void *, SoDragger *d)
{
  SoTabBoxDragger * thisp = THISP(d);

  const SbMatrix &matrix = thisp->getMotionMatrix();
  SbVec3f t, s;
  SbRotation r, so;
  matrix.getTransform(t, r, s, so);

  thisp->translFieldSensor->detach();
  if (thisp->translation.getValue() != t) {
    thisp->translation = t;
  }
  thisp->translFieldSensor->attach(&thisp->translation);

  thisp->scaleFieldSensor->detach();
  if (thisp->scaleFactor.getValue() != s) {
    thisp->scaleFactor = s;
  }
  thisp->scaleFieldSensor->attach(&thisp->scaleFactor);
}

/*!
  \e surroundScale is invalidated every time a child dragger is
  activated/deactivated using this callback
*/
void
SoTabBoxDragger::invalidateSurroundScaleCB(void * d, SoDragger *)
{
  SoTabBoxDragger * thisp = THISP(d);
  SoSurroundScale *ss = SO_CHECK_PART(thisp, "surroundScale", SoSurroundScale);
  if (ss) ss->invalidate();
}

// private
void
SoTabBoxDragger::initTransformNodes(void)
{
  SoTransform *tf;
  tf = SO_GET_ANY_PART(this, "tabPlane1Xf", SoTransform);
  tf->translation = SbVec3f(0.0f, 0.0f, 1.0f);
  this->tabPlane1Xf.setDefault(TRUE);
  tf = SO_GET_ANY_PART(this, "tabPlane2Xf", SoTransform);
  tf->translation = SbVec3f(0.0f, 0.0f, -1.0f);
  tf->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), static_cast<float>(M_PI));
  this->tabPlane2Xf.setDefault(TRUE);
  tf = SO_GET_ANY_PART(this, "tabPlane3Xf", SoTransform);
  tf->translation = SbVec3f(1.0f, 0.0f, 0.0f);
  tf->rotation = SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), (static_cast<float>(M_PI))*0.5f);
  this->tabPlane3Xf.setDefault(TRUE);
  tf = SO_GET_ANY_PART(this, "tabPlane4Xf", SoTransform);
  tf->translation = SbVec3f(-1.0f, 0.0f, 0.0f);
  tf->rotation = SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), (-static_cast<float>(M_PI))*0.5f);
  this->tabPlane4Xf.setDefault(TRUE);
  tf = SO_GET_ANY_PART(this, "tabPlane5Xf", SoTransform);
  tf->translation = SbVec3f(0.0f, 1.0f, 0.0f);
  tf->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), (-static_cast<float>(M_PI))*0.5f);
  this->tabPlane5Xf.setDefault(TRUE);
  tf = SO_GET_ANY_PART(this, "tabPlane6Xf", SoTransform);
  tf->translation = SbVec3f(0.0f, -1.0f, 0.0f);
  tf->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), (static_cast<float>(M_PI))*0.5f);
  this->tabPlane6Xf.setDefault(TRUE);

}

/*!
  Signals the dragger to recalculate the size of its tabs.

  Simply calls SoTabPlaneDragger::adjustScaleTabSize() for all 6
  planes this dragger consists of.
*/
void
SoTabBoxDragger::adjustScaleTabSize(void)
{
  SO_GET_ANY_PART(this, "tabPlane1", SoTabPlaneDragger)->adjustScaleTabSize();
  SO_GET_ANY_PART(this, "tabPlane2", SoTabPlaneDragger)->adjustScaleTabSize();
  SO_GET_ANY_PART(this, "tabPlane3", SoTabPlaneDragger)->adjustScaleTabSize();
  SO_GET_ANY_PART(this, "tabPlane4", SoTabPlaneDragger)->adjustScaleTabSize();
  SO_GET_ANY_PART(this, "tabPlane5", SoTabPlaneDragger)->adjustScaleTabSize();
  SO_GET_ANY_PART(this, "tabPlane6", SoTabPlaneDragger)->adjustScaleTabSize();
}

#undef THISP
#endif // HAVE_DRAGGERS
