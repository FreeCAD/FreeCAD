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
  \class SoCenterballDragger SoCenterballDragger.h Inventor/draggers/SoCenterballDragger.h
  \brief The SoCenterballDragger class is a dragger you can rotate and translate.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html centerball.png "Screen Shot of Default Dragger"
  </center>

  For rotation, click either the ball "bands" to constrain rotation to
  happen around the normal vector of the disc, or anywhere else on the
  (invisible) ball for free-form rotation.

  Translation is done by clicking and dragging any of the four arrow
  markers. Hold \c SHIFT to constrain translation to a single axis
  instead of in the plane.

  When translating, instead of modifying the translation part of the
  motion matrix, a \e center field is updated, and the geometry is
  moved using an internal transformation node.

  Here's a simple usage example, in the form of a scene graph as an
  iv-file:

  \verbatim
  #Inventor V2.1 ascii

  Separator {

     # The dragger, offset a little bit against the geometry to clip,
     # so as to not mess up the user interface.
     Separator {
        Translation { translation -4 0 0 }
        DEF cbdragger CenterballDragger { }
     }

     # The clipping plane sub-graph.
     TransformSeparator {

        # Connect transformations to those of the dragger.
        Rotation { rotation 0 0 1 0 = USE cbdragger . rotation }
        Translation { translation 0 0 0 = USE cbdragger . center }

        # Use a simple lineset-based indicator to show how the
        # clipping plane is situated in space.
        Coordinate3 {
           point [
            0 2 2, 0 2 -2,
            0 -2 2, 0 -2 -2,
            0 2 -2, 0 -2 -2,
            0 2 2, 0 -2 2
         ]
        }
        LineSet { numVertices [ 2, 2, 2, 2 ] }

        ClipPlane { }
     }

     # Then follows the geometry that gets clipped. For this
     # example, we use a simple sphere.

     Sphere { }
  }
  \endverbatim

  <center>
  \image html centerballdragger_actionexample.png "CenterBall Dragger Action Shot"
  </center>

  The figure above shows the example scene graph in a viewer with the nearest cross
  on the centerball dragger selected. Moving the dragger then moves the associated clip plane
  over the geometry (the sphere in this case).
*/

#include <Inventor/draggers/SoCenterballDragger.h>

#include <cstring>

#include <Inventor/draggers/SoRotateCylindricalDragger.h>
#include <Inventor/draggers/SoRotateSphericalDragger.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/nodes/SoAntiSquish.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <data/draggerDefaults/centerballDragger.h>

#include "nodekits/SoSubKitP.h"
#include "coindefs.h"
#include "SbBasicP.h"

/*!
  \var SoSFRotation SoCenterballDragger::rotation

  This field is continuously updated to contain the rotation of the
  ball.
*/
/*!
  \var SoSFVec3f SoCenterballDragger::center

  Continuously updated to contain the current translation from the
  dragger's internal local origo position.
*/

/*!
  \var SoFieldSensor * SoCenterballDragger::rotFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoCenterballDragger::centerFieldSensor
  \COININTERNAL
*/

class SoCenterballDraggerP {
public:
};

SO_KIT_SOURCE(SoCenterballDragger);

/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoCenterballDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoCenterballDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoCenterballDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "translateToCenter"
  -->      "surroundScale"
  -->      "antiSquish"
  -->      "lightModel"
           "geomSeparator"
  -->         "XAxisSwitch"
  -->            "XAxis"
  -->         "YAxisSwitch"
  -->            "YAxis"
  -->         "ZAxisSwitch"
  -->            "ZAxis"
  -->      "rotator"
  -->      "YRotator"
  -->      "ZCenterChanger"
  -->      "rotX90"
  -->      "ZRotator"
  -->      "YCenterChanger"
  -->      "rotY90"
  -->      "XCenterChanger"
  -->      "rot2X90"
  -->      "XRotator"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoCenterballDragger
  PVT   "this",  SoCenterballDragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
        "translateToCenter",  SoMatrixTransform  ---
        "surroundScale",  SoSurroundScale  ---
        "antiSquish",  SoAntiSquish  ---
        "lightModel",  SoLightModel  ---
  PVT   "geomSeparator",  SoSeparator  ---
        "rotator",  SoRotateSphericalDragger  ---
        "YRotator",  SoRotateCylindricalDragger  ---
        "ZCenterChanger",  SoTranslate2Dragger  ---
  PVT   "rotX90",  SoRotation  ---
        "ZRotator",  SoRotateCylindricalDragger  ---
        "YCenterChanger",  SoTranslate2Dragger  ---
  PVT   "rotY90",  SoRotation  ---
        "XCenterChanger",  SoTranslate2Dragger  ---
  PVT   "rot2X90",  SoRotation  ---
  PVT   "XAxisSwitch",  SoSwitch  ---
        "XAxis",  SoSeparator  ---
  PVT   "YAxisSwitch",  SoSwitch  ---
        "YAxis",  SoSeparator  ---
  PVT   "ZAxisSwitch",  SoSwitch  ---
        "ZAxis",  SoSeparator  ---
        "XRotator",  SoRotateCylindricalDragger  ---
  \endverbatim

  \NODEKIT_POST_TABLE
 */
SoCenterballDragger::SoCenterballDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoCenterballDragger);
  SO_KIT_ADD_CATALOG_ENTRY(XAxis, SoSeparator, TRUE, XAxisSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(XAxisSwitch, SoSwitch, TRUE, geomSeparator, YAxisSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(XCenterChanger, SoTranslate2Dragger, TRUE, topSeparator, rot2X90, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(XRotator, SoRotateCylindricalDragger, TRUE, topSeparator, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(YAxis, SoSeparator, TRUE, YAxisSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(YAxisSwitch, SoSwitch, TRUE, geomSeparator, ZAxisSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(YCenterChanger, SoTranslate2Dragger, TRUE, topSeparator, rotY90, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(YRotator, SoRotateCylindricalDragger, TRUE, topSeparator, ZCenterChanger, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(ZAxis, SoSeparator, TRUE, ZAxisSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(ZAxisSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(ZCenterChanger, SoTranslate2Dragger, TRUE, topSeparator, rotX90, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(ZRotator, SoRotateCylindricalDragger, TRUE, topSeparator, YCenterChanger, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(antiSquish, SoAntiSquish, FALSE, topSeparator, lightModel, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, TRUE, topSeparator, geomSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rot2X90, SoRotation, TRUE, topSeparator, XRotator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotX90, SoRotation, TRUE, topSeparator, ZRotator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotY90, SoRotation, TRUE, topSeparator, XCenterChanger, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator, SoRotateSphericalDragger, TRUE, topSeparator, YRotator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale, TRUE, topSeparator, antiSquish, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translateToCenter, SoMatrixTransform, TRUE, topSeparator, surroundScale, TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("centerballDragger.iv",
                                       CENTERBALLDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(CENTERBALLDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_ADD_FIELD(center, (0.0f, 0.0f, 0.0f));

  SO_KIT_INIT_INSTANCE();

  // create subdraggers
  SO_GET_ANY_PART(this, "XCenterChanger", SoTranslate2Dragger);
  SO_GET_ANY_PART(this, "YCenterChanger", SoTranslate2Dragger);
  SO_GET_ANY_PART(this, "ZCenterChanger", SoTranslate2Dragger);
  SO_GET_ANY_PART(this, "XRotator", SoRotateCylindricalDragger);
  SO_GET_ANY_PART(this, "YRotator", SoRotateCylindricalDragger);
  SO_GET_ANY_PART(this, "ZRotator", SoRotateCylindricalDragger);
  SO_GET_ANY_PART(this, "rotator", SoRotateSphericalDragger);

  // create default parts
  this->setPartAsDefault("XAxis", "centerballXAxis");
  this->setPartAsDefault("YAxis", "centerballYAxis");
  this->setPartAsDefault("ZAxis", "centerballZAxis");

  // initialize some nodes
  SoRotation *rot;
  rot = SO_GET_ANY_PART(this, "rot2X90", SoRotation);
  rot->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), (static_cast<float>(M_PI))*0.5f);
  rot = SO_GET_ANY_PART(this, "rotX90", SoRotation);
  rot->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), (static_cast<float>(M_PI))*0.5f);
  rot = SO_GET_ANY_PART(this, "rotY90", SoRotation);
  rot->rotation = SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), (static_cast<float>(M_PI))*0.5f);

  // reset default flags for parts we set to a default value
  this->rot2X90.setDefault(TRUE);
  this->rotX90.setDefault(TRUE);
  this->rotY90.setDefault(TRUE);

  SoAntiSquish *squish = SO_GET_ANY_PART(this, "antiSquish", SoAntiSquish);
  squish->sizing = SoAntiSquish::LONGEST_DIAGONAL;
  squish->recalcAlways = FALSE;

  SoMatrixTransform *mt = SO_GET_ANY_PART(this, "translateToCenter", SoMatrixTransform);
  mt->matrix = SbMatrix::identity();

  this->addValueChangedCallback(SoCenterballDragger::valueChangedCB);

  this->rotFieldSensor = new SoFieldSensor(SoCenterballDragger::fieldSensorCB, this);
  this->centerFieldSensor = new SoFieldSensor(SoCenterballDragger::fieldSensorCB, this);
  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoCenterballDragger::~SoCenterballDragger()
{
  delete this->rotFieldSensor;
  delete this->centerFieldSensor;
}

// Doc in superclass. Overridden to save some information necessary to
// move center correctly.
void
SoCenterballDragger::saveStartParameters(void)
{
  inherited::saveStartParameters();
  const SbMatrix &m = this->getMotionMatrix();
  this->savedtransl[0] = m[3][0];
  this->savedtransl[1] = m[3][1];
  this->savedtransl[2] = m[3][2];
}

/*!
  Should be called after motion matrix has been updated by a child
  dragger.
*/
void
SoCenterballDragger::transferCenterDraggerMotion(SoDragger * childdragger)
{
  if (coin_assert_cast<SoNode *>(childdragger) == XCenterChanger.getValue() ||
      coin_assert_cast<SoNode *>(childdragger) == YCenterChanger.getValue() ||
      coin_assert_cast<SoNode *>(childdragger) == ZCenterChanger.getValue()) {
    // translate part of matrix should not change. Move motion
    // into center instead.

    SbVec3f transl;
    SbMatrix matrix = this->getMotionMatrix();
    transl[0] = matrix[3][0];
    transl[1] = matrix[3][1];
    transl[2] = matrix[3][2];

    SbVec3f difftransl = transl - this->savedtransl;
    { // consider rotation before translating
      SbRotation rot = this->rotation.getValue();
      SbMatrix tmp;
      tmp.setRotate(rot.inverse());
      tmp.multVecMatrix(difftransl, difftransl);
    }

    this->centerFieldSensor->detach();
    this->center = difftransl + this->savedcenter;
    this->centerFieldSensor->attach(&this->center);

    matrix[3][0] = this->savedtransl[0];
    matrix[3][1] = this->savedtransl[1];
    matrix[3][2] = this->savedtransl[2];

    SbBool oldval = this->enableValueChangedCallbacks(FALSE);
    this->setMotionMatrix(matrix);
    this->enableValueChangedCallbacks(oldval);

    SoMatrixTransform *mt = SO_GET_ANY_PART(this, "translateToCenter", SoMatrixTransform);
    matrix.setTranslate(this->center.getValue());
    mt->matrix = matrix;
  }
}

/*!
  Sets values for the internal SoSwitch parts. If \a activechild ==
  \c NULL, all feedback is deactivated.
*/
void
SoCenterballDragger::setSwitches(SoDragger * activechild)
{
  SoSwitch *sw;

  if (activechild == NULL || coin_safe_cast<SoNode *>(activechild) == rotator.getValue()) {
    // special feedback when rotator is activated/deactivated
    int switchval = activechild != NULL ? 1 : 0;
    sw = SO_GET_ANY_PART(this, "XCenterChanger.translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
    sw = SO_GET_ANY_PART(this, "YCenterChanger.translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
    sw = SO_GET_ANY_PART(this, "ZCenterChanger.translatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
    sw = SO_GET_ANY_PART(this, "XRotator.rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
    sw = SO_GET_ANY_PART(this, "YRotator.rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
    sw = SO_GET_ANY_PART(this, "ZRotator.rotatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, switchval);;
  }

  // internal feedback
  int vals[3] = { SO_SWITCH_NONE, SO_SWITCH_NONE, SO_SWITCH_NONE };

  if (coin_safe_cast<SoNode *>(activechild) == XRotator.getValue()) {
    vals[0] = 0;
  }
  else if (coin_safe_cast<SoNode *>(activechild) == YRotator.getValue()) {
    vals[1] = 0;
  }
  else if (coin_safe_cast<SoNode *>(activechild) == ZRotator.getValue()) {
    vals[2] = 0;
  }
  else if (activechild != NULL) {
    vals[0] = vals[1] = vals[2] = 0;
  }

  sw = SO_GET_ANY_PART(this, "XAxisSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, vals[0]);
  sw = SO_GET_ANY_PART(this, "YAxisSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, vals[1]);
  sw = SO_GET_ANY_PART(this, "ZAxisSwitch", SoSwitch);
  SoInteractionKit::setSwitchValue(sw, vals[2]);

}

// Doc in superclass.
SbBool
SoCenterballDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  int i;
  SbString str;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);
    SoDragger *child;
    child = coin_assert_cast<SoDragger *>(this->getAnyPart("rotator", FALSE));
    child->setPartAsDefault("rotator",
                            "centerballRotator");
    child->setPartAsDefault("rotatorActive",
                            "centerballRotatorActive");
    child->setPartAsDefault("feedback", new SoSeparator);
    child->setPartAsDefault("feedbackActive", new SoSeparator);
    this->addChildDragger(child);

    for (i = 0; i < 3; i++) {
      str.sprintf("%cRotator", 'X' + i);
      child = static_cast<SoDragger *>(this->getAnyPart(str.getString(), FALSE));
      child->setPartAsDefault("rotator",
                              "centerballStripe");
      child->setPartAsDefault("rotatorActive",
                              "centerballStripeActive");
      child->setPartAsDefault("feedback", new SoSeparator);
      child->setPartAsDefault("feedbackActive", new SoSeparator);
      this->addChildDragger(child);
    }

    for (i = 0; i < 3; i++) {
      str.sprintf("%cCenterChanger", 'X' + i);
      child = coin_assert_cast<SoDragger *>(this->getAnyPart(str.getString(), FALSE));
      child->setPartAsDefault("translator",
                              "centerballCenterChanger");
      child->setPartAsDefault("translatorActive",
                              "centerballCenterChangerActive");
      child->setPartAsDefault("xAxisFeedback",
                              "centerballCenterXAxisFeedback");
      child->setPartAsDefault("yAxisFeedback",
                              "centerballCenterYAxisFeedback");
      this->addChildDragger(child);
    }

    // Update dragger in case fields have changed values before connection
    SoCenterballDragger::fieldSensorCB(this, NULL);

    if (this->rotFieldSensor->getAttachedField() != &this->rotation) {
      this->rotFieldSensor->attach(&this->rotation);
    }
    if (this->centerFieldSensor->getAttachedField() != &this->center) {
      this->centerFieldSensor->attach(&this->center);
    }
  }
  else {
    this->removeChildDragger("rotator");
    this->removeChildDragger("XRotator");
    this->removeChildDragger("YRotator");
    this->removeChildDragger("ZRotator");
    this->removeChildDragger("XCenterChanger");
    this->removeChildDragger("YCenterChanger");
    this->removeChildDragger("ZCenterChanger");

    if (this->rotFieldSensor->getAttachedField() != NULL) {
      this->rotFieldSensor->detach();
    }
    if (this->centerFieldSensor->getAttachedField() != NULL) {
      this->centerFieldSensor->detach();
    }
    inherited::setUpConnections(onoff, doitalways);
  }
  return !(this->connectionsSetUp = onoff);
}

// Doc in superclass.
void
SoCenterballDragger::setDefaultOnNonWritingFields(void)
{
  this->rotator.setDefault(TRUE);
  this->XCenterChanger.setDefault(TRUE);
  this->YCenterChanger.setDefault(TRUE);
  this->ZCenterChanger.setDefault(TRUE);
  this->XRotator.setDefault(TRUE);
  this->YRotator.setDefault(TRUE);
  this->ZRotator.setDefault(TRUE);

  this->surroundScale.setDefault(TRUE);
  this->antiSquish.setDefault(TRUE);
  this->translateToCenter.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoCenterballDragger::fieldSensorCB(void * d, SoSensor *)
{
  SoCenterballDragger * thisp = static_cast<SoCenterballDragger *>(d);

  // Save center variable and translate dragger to correct position
  thisp->savedcenter = thisp->center.getValue();
  SbMatrix centermat;
  centermat.setTranslate(thisp->savedcenter);
  SoMatrixTransform * mt =
    SO_GET_ANY_PART(thisp, "translateToCenter", SoMatrixTransform);
  mt->matrix = centermat;

  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoCenterballDragger::valueChangedCB(void *, SoDragger * d)
{
  SoCenterballDragger * thisp = static_cast<SoCenterballDragger *>(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  SbVec3f t, s;
  SbRotation r, so;

  // Eliminate center variable of matrix
  if (thisp->savedcenter != SbVec3f(0.0f, 0.0f, 0.0f)) {
    SbMatrix trans;
    trans.setTranslate(thisp->savedcenter);
    matrix.multLeft(trans);
    trans.setTranslate(-(thisp->savedcenter));
    matrix.multRight(trans);
  }

  // Do an inverse rotation, using matrix with center eliminated
  // to obtain correct translation
  matrix.getTransform(t, r, s, so);
  SbMatrix rotmat;
  rotmat.setRotate(r);
  //SbMatrix tmp = matrix;
  matrix.multLeft(rotmat.inverse());

  // Update center of object if dragger has translated
  SbVec3f difftrans(matrix[3][0], matrix[3][1], matrix[3][2]);
  if (difftrans != SbVec3f(0.0f, 0.0f, 0.0f)) {
    thisp->centerFieldSensor->detach();
    thisp->center.setValue(thisp->savedcenter + difftrans);
    thisp->centerFieldSensor->attach(&thisp->center);
  }

  thisp->rotFieldSensor->detach();
  if (thisp->rotation.getValue() != r) {
    thisp->rotation = r;
  }
  thisp->rotFieldSensor->attach(&thisp->rotation);
}

/*!
  \COININTERNAL
  Needed to activate some feedback.
*/
void
SoCenterballDragger::kidStartCB(void * d , SoDragger * child)
{
  SoCenterballDragger * thisp = static_cast<SoCenterballDragger *>(d);
  thisp->setSwitches(child);

  SoSurroundScale * scale = coin_safe_cast<SoSurroundScale *>(
    thisp->getPart("surroundScale", FALSE)
    );
  if (scale) scale->invalidate();
}

/*!
  \COININTERNAL
  Needed to deactivate some feedback.
*/
void
SoCenterballDragger::kidFinishCB(void * d, SoDragger * COIN_UNUSED_ARG(child))
{
  SoCenterballDragger * thisp = static_cast<SoCenterballDragger *>(d);
  thisp->setSwitches(NULL);

  SoSurroundScale * scale = coin_safe_cast<SoSurroundScale *>(
    thisp->getPart("surroundScale", FALSE)
    );
  if (scale) scale->invalidate();
}

// Convenience method that takes care of callbacks before registering
// child.
void
SoCenterballDragger::addChildDragger(SoDragger *child)
{
  child->addStartCallback(SoCenterballDragger::kidStartCB, this);
  child->addFinishCallback(SoCenterballDragger::kidFinishCB, this);
  this->registerChildDragger(child);
}

//
// convenience method that removes callbacks before unregistering child
//
void
SoCenterballDragger::removeChildDragger(const char *childname)
{
  SoDragger * child = coin_assert_cast<SoDragger *>(this->getAnyPart(childname, FALSE));
  child->removeStartCallback(SoCenterballDragger::kidStartCB, this);
  child->removeFinishCallback(SoCenterballDragger::kidFinishCB, this);
  this->unregisterChildDragger(child);
}

// Doc in superclass.
void
SoCenterballDragger::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoSurroundScale * scale = coin_safe_cast<SoSurroundScale *>(
    this->getPart("surroundScale", FALSE)
    );
  if (scale) {
    SbBool dotrans = scale->isDoingTranslations();
    scale->setDoingTranslations(FALSE);
    inherited::getBoundingBox(action);
    scale->setDoingTranslations(dotrans);
  }
  else inherited::getBoundingBox(action);
}

// Doc in superclass.
void
SoCenterballDragger::getMatrix(SoGetMatrixAction * action)
{
  SoSurroundScale * scale = coin_safe_cast<SoSurroundScale *>(
    this->getPart("surroundScale", FALSE)
    );
  if (scale) {
    SbBool dotrans = scale->isDoingTranslations();
    scale->setDoingTranslations(FALSE);
    inherited::getMatrix(action);
    scale->setDoingTranslations(dotrans);
  }
  else inherited::getMatrix(action);
}

#endif // HAVE_DRAGGERS
