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
  \class SoTransformerDragger SoTransformerDragger.h Inventor/draggers/SoTransformerDragger.h
  \brief The SoTransformerDragger provides geometry for translation, scaling and rotations.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html transformer.png "Screen Shot of Default Dragger"
  </center>

  Translate the dragger by clicking and dragging any of the
  (invisible) sides. Translation will default be done in the plane of
  the side the end-user selected. The user can hold down a \c SHIFT
  key to lock translation to a single of the axes in the plane. By
  holding down a \c CTRL key instead, translation can be done along
  the plane's normal vector.

  Scaling is done by dragging the corner cubes. By default, uniform
  scaling will be done. Hold down \c SHIFT before selecting any of the
  corners to do non-uniform scaling. Uniform scaling towards a
  corner-point can be accomplished by holding down \c CTRL before
  clicking and dragging one of the cubes.

  Rotation is done by dragging any of the 6 end-markers of the axis
  cross. The initial drag direction decides which orientation the
  rotation will be done in. Hold down \c SHIFT to do free-form
  rotation around the sphere instead.


  This is a big and complex dragger which needs a fair amount of
  proper documentation when provided in end-user applications. If what
  you are trying to accomplish in your application does not really
  demand most of the features of this dragger, you are advised to
  investigate whether or not any of the less complex draggers can
  fulfill your requirements -- so you can provide an as simple as
  possible user interface to your end-users.


  For the application programmer's convenience, the Coin library also
  provides a manipulator class called SoTransformerManip, which wraps
  the SoTransformerDragger into the necessary mechanisms for making
  direct insertion of this dragger into a scene graph possible with
  very little effort.

  \sa SoTransformerManip
*/

#include <Inventor/draggers/SoTransformerDragger.h>

#include <cstring>

#include <Inventor/nodes/SoAntiSquish.h>
#include <Inventor/nodes/SoLocateHighlight.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbSphereSectionProjector.h>
#include <Inventor/projectors/SbCylinderPlaneProjector.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/lists/SoPathList.h>

#include <data/draggerDefaults/transformerDragger.h>

#include "coindefs.h" // COIN_STUB() & COIN_OBSOLETED()
#include "nodekits/SoSubKitP.h"
#include "SbBasicP.h"

// FIXME, bugs or missing features (pederb, 20000224):
// o some feedback is missing (mostly crosshair)
// o detect if disc or cylinder rotator should be used (disc-only right now)
//
// Also the translation feedback is a bit different from OIV. Coin
// always places the feedback axes at the center of the face being
// translated. OIV places them at the picked point. I think our
// strategy is better, since when switching between constrained
// translations and unconstrained translation, the OIV feedback axes
// can easily be positioned outside the face being dragged.
//
// MATRICES AND SPACES:
// There are many matrices and spaces that can take some time to get
// understand. The matrices calculated are:
//
// localToWorld = motionMatrix * draggerToWorld
// worldToLocal = worldToDragger * motionMatrix^-1
// draggerToWorld = worldToDragger^-1
//
// localToWorking = surroundScaleMatrix^-1
// workingToLocal = surroundScaleMatrix
//
// workingToWorld = surroundScaleMatrix * localToWorld
// worldToWorking = worldToLocal * surroundScaleMatrix
//
// boxPointInWorldSpace = p * surroundScaleMatrix * localToWorld
// worldPointInBoxSpace = p * worldToLocal * surroundScaleMatrix

/*!
  \enum SoTransformerDragger::State

  The various possible states the dragger might be in at any given
  time. That is: either SoTransformerDragger::INACTIVE if there's no
  interaction, or any of the other values to indicate what operation
  the end-user is currently executing.
*/

/*!
  \var SoSFRotation SoTransformerDragger::rotation

  This field is continuously updated to contain the orientation of the
  dragger.
*/
/*!
  \var SoSFVec3f SoTransformerDragger::translation

  The dragger's offset position from the local origo.
*/
/*!
  \var SoSFVec3f SoTransformerDragger::scaleFactor

  Continuously updated to contain the current vector of scaling along
  the X, Y and Z axes.
*/

// FIXME: can't see what this is for -- investigate. 20011208 mortene.
//  /*!
//    \var SoSFFloat SoTransformerDragger::minDiscRotDot
//  */

/*!
  \var SoFieldSensor * SoTransformerDragger::translFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoTransformerDragger::scaleFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoTransformerDragger::rotateFieldSensor
  \COININTERNAL
*/
/*!
  \var SoNodeList SoTransformerDragger::antiSquishList
  \COININTERNAL
*/

#define WHATKIND_NONE      0
#define WHATKIND_SCALE     1
#define WHATKIND_TRANSLATE 2
#define WHATKIND_ROTATE    3

#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3
#define CONSTRAINT_Z    4

#define KNOB_DISTANCE 1.25f   // distance from center to rotate-knobs


#ifndef DOXYGEN_SKIP_THIS

class SoTransformerDraggerP {
public:
  SbMatrix prevMotionMatrix;
  SbVec3f prevWorldHitPt;
  SbVec3f ctrlOffset;
  SbBool ctrlDown;
  SbBool shiftDown;
  SbVec2f normalizedStartLocaterPosition;

  SbBool locateHighlighting;
  static int colinearThreshold;
  int constraintState;

  int whatkind;
  int whatnum;
  int dimension;
};

int SoTransformerDraggerP::colinearThreshold = 3; // FIXME: find default value from somewhere

#endif // DOXYGEN_SKIP_THIS

SO_KIT_SOURCE(SoTransformerDragger);

/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoTransformerDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoTransformerDragger, SO_FROM_INVENTOR_1);
  SoTransformerDraggerP::colinearThreshold = 3;
}

void
SoTransformerDragger::build_catalog1(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale, TRUE, topSeparator, overallStyle, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(overallStyle, SoGroup, TRUE, topSeparator, geomSeparator, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translatorSep, SoSeparator, TRUE, topSeparator, rotatorSep, FALSE);
}

void
SoTransformerDragger::build_catalog2(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(translator1Switch, SoSwitch, TRUE, translatorSep, translator2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1LocateGroup, SoLocateHighlight, TRUE, translator1Switch, translator1Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1, SoSeparator, TRUE, translator1LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1Active, SoSeparator, TRUE, translator1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2Switch, SoSwitch, TRUE, translatorSep, translator3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2LocateGroup, SoLocateHighlight, TRUE, translator2Switch, translator2Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2, SoSeparator, TRUE, translator2LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2Active, SoSeparator, TRUE, translator2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3Switch, SoSwitch, TRUE, translatorSep, translator4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3LocateGroup, SoLocateHighlight, TRUE, translator3Switch, translator3Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3, SoSeparator, TRUE, translator3LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3Active, SoSeparator, TRUE, translator3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4Switch, SoSwitch, TRUE, translatorSep, translator5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4LocateGroup, SoLocateHighlight, TRUE, translator4Switch, translator4Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4, SoSeparator, TRUE, translator4LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4Active, SoSeparator, TRUE, translator4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5Switch, SoSwitch, TRUE, translatorSep, translator6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5LocateGroup, SoLocateHighlight, TRUE, translator5Switch, translator5Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5, SoSeparator, TRUE, translator5LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5Active, SoSeparator, TRUE, translator5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6Switch, SoSwitch, TRUE, translatorSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6LocateGroup, SoLocateHighlight, TRUE, translator6Switch, translator6Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6, SoSeparator, TRUE, translator6LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6Active, SoSeparator, TRUE, translator6Switch, "", TRUE);
}

void
SoTransformerDragger::build_catalog3(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(rotatorSep, SoSeparator, TRUE, topSeparator, scaleSep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator1Switch, SoSwitch, TRUE, rotatorSep, rotator2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator1LocateGroup, SoLocateHighlight, TRUE, rotator1Switch, rotator1Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator1, SoSeparator, TRUE, rotator1LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator1Active, SoSeparator, TRUE, rotator1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator2Switch, SoSwitch, TRUE, rotatorSep, rotator3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator2LocateGroup, SoLocateHighlight, TRUE, rotator2Switch, rotator2Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator2, SoSeparator, TRUE, rotator2LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator2Active, SoSeparator, TRUE, rotator2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator3Switch, SoSwitch, TRUE, rotatorSep, rotator4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator3LocateGroup, SoLocateHighlight, TRUE, rotator3Switch, rotator3Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator3, SoSeparator, TRUE, rotator3LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator3Active, SoSeparator, TRUE, rotator3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator4Switch, SoSwitch, TRUE, rotatorSep, rotator5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator4LocateGroup, SoLocateHighlight, TRUE, rotator4Switch, rotator4Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator4, SoSeparator, TRUE, rotator4LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator4Active, SoSeparator, TRUE, rotator4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator5Switch, SoSwitch, TRUE, rotatorSep, rotator6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator5LocateGroup, SoLocateHighlight, TRUE, rotator5Switch, rotator5Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator5, SoSeparator, TRUE, rotator5LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator5Active, SoSeparator, TRUE, rotator5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator6Switch, SoSwitch, TRUE, rotatorSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator6LocateGroup, SoLocateHighlight, TRUE, rotator6Switch, rotator6Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator6, SoSeparator, TRUE, rotator6LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(rotator6Active, SoSeparator, TRUE, rotator6Switch, "", TRUE);
}

void
SoTransformerDragger::build_catalog4(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(scaleSep, SoSeparator, TRUE, topSeparator, circleFeedbackSep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale1Switch, SoSwitch, TRUE, scaleSep, scale2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale1LocateGroup, SoLocateHighlight, TRUE, scale1Switch, scale1Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale1, SoSeparator, TRUE, scale1LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale1Active, SoSeparator, TRUE, scale1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale2Switch, SoSwitch, TRUE, scaleSep, scale3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale2LocateGroup, SoLocateHighlight, TRUE, scale2Switch, scale2Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale2, SoSeparator, TRUE, scale2LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale2Active, SoSeparator, TRUE, scale2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale3Switch, SoSwitch, TRUE, scaleSep, scale4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale3LocateGroup, SoLocateHighlight, TRUE, scale3Switch, scale3Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale3, SoSeparator, TRUE, scale3LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale3Active, SoSeparator, TRUE, scale3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale4Switch, SoSwitch, TRUE, scaleSep, scale5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale4LocateGroup, SoLocateHighlight, TRUE, scale4Switch, scale4Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale4, SoSeparator, TRUE, scale4LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale4Active, SoSeparator, TRUE, scale4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale5Switch, SoSwitch, TRUE, scaleSep, scale6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale5LocateGroup, SoLocateHighlight, TRUE, scale5Switch, scale5Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale5, SoSeparator, TRUE, scale5LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale5Active, SoSeparator, TRUE, scale5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale6Switch, SoSwitch, TRUE, scaleSep, scale7Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale6LocateGroup, SoLocateHighlight, TRUE, scale6Switch, scale6Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale6, SoSeparator, TRUE, scale6LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale6Active, SoSeparator, TRUE, scale6Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale7Switch, SoSwitch, TRUE, scaleSep, scale8Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale7LocateGroup, SoLocateHighlight, TRUE, scale7Switch, scale7Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale7, SoSeparator, TRUE, scale7LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale7Active, SoSeparator, TRUE, scale7Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale8Switch, SoSwitch, TRUE, scaleSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale8LocateGroup, SoLocateHighlight, TRUE, scale8Switch, scale8Active, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scale8, SoSeparator, TRUE, scale8LocateGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scale8Active, SoSeparator, TRUE, scale8Switch, "", TRUE);
}

void
SoTransformerDragger::build_catalog5(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(axisFeedbackSep, SoSeparator, TRUE, geomSeparator, translateBoxFeedbackSep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(axisFeedbackLocation, SoTranslation, TRUE, axisFeedbackSep, xAxisFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(xAxisFeedbackSwitch, SoSwitch, TRUE, axisFeedbackSep, yAxisFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(xAxisFeedbackActive, SoSeparator, TRUE, xAxisFeedbackSwitch, xAxisFeedbackSelect, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(xAxisFeedbackSelect, SoSeparator, TRUE, xAxisFeedbackSwitch, xCrosshairFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(xCrosshairFeedback, SoSeparator, TRUE, xAxisFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(yAxisFeedbackSwitch, SoSwitch, TRUE, axisFeedbackSep, zAxisFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(yAxisFeedbackActive, SoSeparator, TRUE, yAxisFeedbackSwitch, yAxisFeedbackSelect, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(yAxisFeedbackSelect, SoSeparator, TRUE, yAxisFeedbackSwitch, yCrosshairFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(yCrosshairFeedback, SoSeparator, TRUE, yAxisFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(zAxisFeedbackSwitch, SoSwitch, TRUE, axisFeedbackSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(zAxisFeedbackActive, SoSeparator, TRUE, zAxisFeedbackSwitch, zAxisFeedbackSelect, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(zAxisFeedbackSelect, SoSeparator, TRUE, zAxisFeedbackSwitch, zCrosshairFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(zCrosshairFeedback, SoSeparator, TRUE, zAxisFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translateBoxFeedbackSep, SoSeparator, TRUE, geomSeparator, scaleBoxFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translateBoxFeedbackSwitch, SoSwitch, TRUE, translateBoxFeedbackSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translateBoxFeedbackRotation, SoRotation, TRUE, translateBoxFeedbackSwitch, translateBoxFeedback, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translateBoxFeedback, SoSeparator, TRUE, translateBoxFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(scaleBoxFeedbackSwitch, SoSwitch, TRUE, geomSeparator, posXWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(scaleBoxFeedback, SoSeparator, TRUE, scaleBoxFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posXWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, posYWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(posXWallFeedback, SoSeparator, TRUE, posXWallFeedbackSwitch, posXRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posXRoundWallFeedback, SoSeparator, TRUE, posXWallFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posYWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, posZWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(posYWallFeedback, SoSeparator, TRUE, posYWallFeedbackSwitch, posYRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posYRoundWallFeedback, SoSeparator, TRUE, posYWallFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posZWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, negXWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(posZWallFeedback, SoSeparator, TRUE, posZWallFeedbackSwitch, posZRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(posZRoundWallFeedback, SoSeparator, TRUE, posZWallFeedbackSwitch, "", TRUE);
}

void
SoTransformerDragger::build_catalog6(void)
{
  SO_KIT_ADD_CATALOG_ENTRY(negXWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, negYWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(negXWallFeedback, SoSeparator, TRUE, negXWallFeedbackSwitch, negXRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(negXRoundWallFeedback, SoSeparator, TRUE, negXWallFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(negYWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, negZWallFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(negYWallFeedback, SoSeparator, TRUE, negYWallFeedbackSwitch, negYRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(negYRoundWallFeedback, SoSeparator, TRUE, negYWallFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(negZWallFeedbackSwitch, SoSwitch, TRUE, geomSeparator, radialFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(negZWallFeedback, SoSeparator, TRUE, negZWallFeedbackSwitch, negZRoundWallFeedback, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(negZRoundWallFeedback, SoSeparator, TRUE, negZWallFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(radialFeedbackSwitch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(radialFeedback, SoSeparator, TRUE, radialFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(circleFeedbackSep, SoSeparator, TRUE, topSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(circleFeedbackTransformSwitch, SoSwitch, TRUE, circleFeedbackSep, xCircleFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(circleFeedbackAntiSquish, SoAntiSquish, TRUE, circleFeedbackTransformSwitch, circleFeedbackTransform, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(circleFeedbackTransform, SoTransform, TRUE, circleFeedbackTransformSwitch, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(xCircleFeedbackSwitch, SoSwitch, TRUE, circleFeedbackSep, yCircleFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(xCircleFeedback, SoSeparator, TRUE, xCircleFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(yCircleFeedbackSwitch, SoSwitch, TRUE, circleFeedbackSep, zCircleFeedbackSwitch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(yCircleFeedback, SoSeparator, TRUE, yCircleFeedbackSwitch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(zCircleFeedbackSwitch, SoSwitch, TRUE, circleFeedbackSep, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(zCircleFeedback, SoSeparator, TRUE, zCircleFeedbackSwitch, "", TRUE);
}

#define PRIVATE(obj) ((obj)->pimpl)
#define THISP(d) static_cast<SoTransformerDragger *>(d)

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoTransformerDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "surroundScale"
  -->      "overallStyle"
           "geomSeparator"
  -->         "axisFeedbackSep"
  -->            "axisFeedbackLocation"
  -->            "xAxisFeedbackSwitch"
  -->               "xAxisFeedbackActive"
  -->               "xAxisFeedbackSelect"
  -->               "xCrosshairFeedback"
  -->            "yAxisFeedbackSwitch"
  -->               "yAxisFeedbackActive"
  -->               "yAxisFeedbackSelect"
  -->               "yCrosshairFeedback"
  -->            "zAxisFeedbackSwitch"
  -->               "zAxisFeedbackActive"
  -->               "zAxisFeedbackSelect"
  -->               "zCrosshairFeedback"
  -->         "translateBoxFeedbackSep"
  -->            "translateBoxFeedbackSwitch"
  -->               "translateBoxFeedbackRotation"
  -->               "translateBoxFeedback"
  -->         "scaleBoxFeedbackSwitch"
  -->            "scaleBoxFeedback"
  -->         "posXWallFeedbackSwitch"
  -->            "posXWallFeedback"
  -->            "posXRoundWallFeedback"
  -->         "posYWallFeedbackSwitch"
  -->            "posYWallFeedback"
  -->            "posYRoundWallFeedback"
  -->         "posZWallFeedbackSwitch"
  -->            "posZWallFeedback"
  -->            "posZRoundWallFeedback"
  -->         "negXWallFeedbackSwitch"
  -->            "negXWallFeedback"
  -->            "negXRoundWallFeedback"
  -->         "negYWallFeedbackSwitch"
  -->            "negYWallFeedback"
  -->            "negYRoundWallFeedback"
  -->         "negZWallFeedbackSwitch"
  -->            "negZWallFeedback"
  -->            "negZRoundWallFeedback"
  -->         "radialFeedbackSwitch"
  -->            "radialFeedback"
  -->      "translatorSep"
  -->         "translator1Switch"
  -->            "translator1LocateGroup"
  -->               "translator1"
  -->            "translator1Active"
  -->         "translator2Switch"
  -->            "translator2LocateGroup"
  -->               "translator2"
  -->            "translator2Active"
  -->         "translator3Switch"
  -->            "translator3LocateGroup"
  -->               "translator3"
  -->            "translator3Active"
  -->         "translator4Switch"
  -->            "translator4LocateGroup"
  -->               "translator4"
  -->            "translator4Active"
  -->         "translator5Switch"
  -->            "translator5LocateGroup"
  -->               "translator5"
  -->            "translator5Active"
  -->         "translator6Switch"
  -->            "translator6LocateGroup"
  -->               "translator6"
  -->            "translator6Active"
  -->      "rotatorSep"
  -->         "rotator1Switch"
  -->            "rotator1LocateGroup"
  -->               "rotator1"
  -->            "rotator1Active"
  -->         "rotator2Switch"
  -->            "rotator2LocateGroup"
  -->               "rotator2"
  -->            "rotator2Active"
  -->         "rotator3Switch"
  -->            "rotator3LocateGroup"
  -->               "rotator3"
  -->            "rotator3Active"
  -->         "rotator4Switch"
  -->            "rotator4LocateGroup"
  -->               "rotator4"
  -->            "rotator4Active"
  -->         "rotator5Switch"
  -->            "rotator5LocateGroup"
  -->               "rotator5"
  -->            "rotator5Active"
  -->         "rotator6Switch"
  -->            "rotator6LocateGroup"
  -->               "rotator6"
  -->            "rotator6Active"
  -->      "scaleSep"
  -->         "scale1Switch"
  -->            "scale1LocateGroup"
  -->               "scale1"
  -->            "scale1Active"
  -->         "scale2Switch"
  -->            "scale2LocateGroup"
  -->               "scale2"
  -->            "scale2Active"
  -->         "scale3Switch"
  -->            "scale3LocateGroup"
  -->               "scale3"
  -->            "scale3Active"
  -->         "scale4Switch"
  -->            "scale4LocateGroup"
  -->               "scale4"
  -->            "scale4Active"
  -->         "scale5Switch"
  -->            "scale5LocateGroup"
  -->               "scale5"
  -->            "scale5Active"
  -->         "scale6Switch"
  -->            "scale6LocateGroup"
  -->               "scale6"
  -->            "scale6Active"
  -->         "scale7Switch"
  -->            "scale7LocateGroup"
  -->               "scale7"
  -->            "scale7Active"
  -->         "scale8Switch"
  -->            "scale8LocateGroup"
  -->               "scale8"
  -->            "scale8Active"
  -->      "circleFeedbackSep"
  -->         "circleFeedbackTransformSwitch"
  -->            "circleFeedbackAntiSquish"
  -->            "circleFeedbackTransform"
  -->         "xCircleFeedbackSwitch"
  -->            "xCircleFeedback"
  -->         "yCircleFeedbackSwitch"
  -->            "yCircleFeedback"
  -->         "zCircleFeedbackSwitch"
  -->            "zCircleFeedback"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoTransformerDragger
  PVT   "this",  SoTransformerDragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
        "surroundScale",  SoSurroundScale  ---
  PVT   "overallStyle",  SoGroup  ---
  PVT   "geomSeparator",  SoSeparator  ---
  PVT   "translatorSep",  SoSeparator  ---
  PVT   "translator1Switch",  SoSwitch  ---
  PVT   "translator1LocateGroup",  SoLocateHighlight  ---
        "translator1",  SoSeparator  ---
        "translator1Active",  SoSeparator  ---
  PVT   "translator2Switch",  SoSwitch  ---
  PVT   "translator2LocateGroup",  SoLocateHighlight  ---
        "translator2",  SoSeparator  ---
        "translator2Active",  SoSeparator  ---
  PVT   "translator3Switch",  SoSwitch  ---
  PVT   "translator3LocateGroup",  SoLocateHighlight  ---
        "translator3",  SoSeparator  ---
        "translator3Active",  SoSeparator  ---
  PVT   "translator4Switch",  SoSwitch  ---
  PVT   "translator4LocateGroup",  SoLocateHighlight  ---
        "translator4",  SoSeparator  ---
        "translator4Active",  SoSeparator  ---
  PVT   "translator5Switch",  SoSwitch  ---
  PVT   "translator5LocateGroup",  SoLocateHighlight  ---
        "translator5",  SoSeparator  ---
        "translator5Active",  SoSeparator  ---
  PVT   "translator6Switch",  SoSwitch  ---
  PVT   "translator6LocateGroup",  SoLocateHighlight  ---
        "translator6",  SoSeparator  ---
        "translator6Active",  SoSeparator  ---
  PVT   "rotatorSep",  SoSeparator  ---
  PVT   "rotator1Switch",  SoSwitch  ---
  PVT   "rotator1LocateGroup",  SoLocateHighlight  ---
        "rotator1",  SoSeparator  ---
        "rotator1Active",  SoSeparator  ---
  PVT   "rotator2Switch",  SoSwitch  ---
  PVT   "rotator2LocateGroup",  SoLocateHighlight  ---
        "rotator2",  SoSeparator  ---
        "rotator2Active",  SoSeparator  ---
  PVT   "rotator3Switch",  SoSwitch  ---
  PVT   "rotator3LocateGroup",  SoLocateHighlight  ---
        "rotator3",  SoSeparator  ---
        "rotator3Active",  SoSeparator  ---
  PVT   "rotator4Switch",  SoSwitch  ---
  PVT   "rotator4LocateGroup",  SoLocateHighlight  ---
        "rotator4",  SoSeparator  ---
        "rotator4Active",  SoSeparator  ---
  PVT   "rotator5Switch",  SoSwitch  ---
  PVT   "rotator5LocateGroup",  SoLocateHighlight  ---
        "rotator5",  SoSeparator  ---
        "rotator5Active",  SoSeparator  ---
  PVT   "rotator6Switch",  SoSwitch  ---
  PVT   "rotator6LocateGroup",  SoLocateHighlight  ---
        "rotator6",  SoSeparator  ---
        "rotator6Active",  SoSeparator  ---
  PVT   "scaleSep",  SoSeparator  ---
  PVT   "scale1Switch",  SoSwitch  ---
  PVT   "scale1LocateGroup",  SoLocateHighlight  ---
        "scale1",  SoSeparator  ---
        "scale1Active",  SoSeparator  ---
  PVT   "scale2Switch",  SoSwitch  ---
  PVT   "scale2LocateGroup",  SoLocateHighlight  ---
        "scale2",  SoSeparator  ---
        "scale2Active",  SoSeparator  ---
  PVT   "scale3Switch",  SoSwitch  ---
  PVT   "scale3LocateGroup",  SoLocateHighlight  ---
        "scale3",  SoSeparator  ---
        "scale3Active",  SoSeparator  ---
  PVT   "scale4Switch",  SoSwitch  ---
  PVT   "scale4LocateGroup",  SoLocateHighlight  ---
        "scale4",  SoSeparator  ---
        "scale4Active",  SoSeparator  ---
  PVT   "scale5Switch",  SoSwitch  ---
  PVT   "scale5LocateGroup",  SoLocateHighlight  ---
        "scale5",  SoSeparator  ---
        "scale5Active",  SoSeparator  ---
  PVT   "scale6Switch",  SoSwitch  ---
  PVT   "scale6LocateGroup",  SoLocateHighlight  ---
        "scale6",  SoSeparator  ---
        "scale6Active",  SoSeparator  ---
  PVT   "scale7Switch",  SoSwitch  ---
  PVT   "scale7LocateGroup",  SoLocateHighlight  ---
        "scale7",  SoSeparator  ---
        "scale7Active",  SoSeparator  ---
  PVT   "scale8Switch",  SoSwitch  ---
  PVT   "scale8LocateGroup",  SoLocateHighlight  ---
        "scale8",  SoSeparator  ---
        "scale8Active",  SoSeparator  ---
  PVT   "circleFeedbackSep",  SoSeparator  ---
  PVT   "circleFeedbackTransformSwitch",  SoSwitch  ---
  PVT   "circleFeedbackAntiSquish",  SoAntiSquish  ---
  PVT   "circleFeedbackTransform",  SoTransform  ---
  PVT   "xCircleFeedbackSwitch",  SoSwitch  ---
        "xCircleFeedback",  SoSeparator  ---
  PVT   "yCircleFeedbackSwitch",  SoSwitch  ---
        "yCircleFeedback",  SoSeparator  ---
  PVT   "zCircleFeedbackSwitch",  SoSwitch  ---
        "zCircleFeedback",  SoSeparator  ---
  PVT   "axisFeedbackSep",  SoSeparator  ---
  PVT   "axisFeedbackLocation",  SoTranslation  ---
  PVT   "xAxisFeedbackSwitch",  SoSwitch  ---
        "xAxisFeedbackActive",  SoSeparator  ---
        "xAxisFeedbackSelect",  SoSeparator  ---
        "xCrosshairFeedback",  SoSeparator  ---
  PVT   "yAxisFeedbackSwitch",  SoSwitch  ---
        "yAxisFeedbackActive",  SoSeparator  ---
        "yAxisFeedbackSelect",  SoSeparator  ---
        "yCrosshairFeedback",  SoSeparator  ---
  PVT   "zAxisFeedbackSwitch",  SoSwitch  ---
        "zAxisFeedbackActive",  SoSeparator  ---
        "zAxisFeedbackSelect",  SoSeparator  ---
        "zCrosshairFeedback",  SoSeparator  ---
  PVT   "translateBoxFeedbackSep",  SoSeparator  ---
  PVT   "translateBoxFeedbackSwitch",  SoSwitch  ---
  PVT   "translateBoxFeedbackRotation",  SoRotation  ---
        "translateBoxFeedback",  SoSeparator  ---
  PVT   "scaleBoxFeedbackSwitch",  SoSwitch  ---
        "scaleBoxFeedback",  SoSeparator  ---
  PVT   "posXWallFeedbackSwitch",  SoSwitch  ---
        "posXWallFeedback",  SoSeparator  ---
        "posXRoundWallFeedback",  SoSeparator  ---
  PVT   "posYWallFeedbackSwitch",  SoSwitch  ---
        "posYWallFeedback",  SoSeparator  ---
        "posYRoundWallFeedback",  SoSeparator  ---
  PVT   "posZWallFeedbackSwitch",  SoSwitch  ---
        "posZWallFeedback",  SoSeparator  ---
        "posZRoundWallFeedback",  SoSeparator  ---
  PVT   "negXWallFeedbackSwitch",  SoSwitch  ---
        "negXWallFeedback",  SoSeparator  ---
        "negXRoundWallFeedback",  SoSeparator  ---
  PVT   "negYWallFeedbackSwitch",  SoSwitch  ---
        "negYWallFeedback",  SoSeparator  ---
        "negYRoundWallFeedback",  SoSeparator  ---
  PVT   "negZWallFeedbackSwitch",  SoSwitch  ---
        "negZWallFeedback",  SoSeparator  ---
        "negZRoundWallFeedback",  SoSeparator  ---
  PVT   "radialFeedbackSwitch",  SoSwitch  ---
        "radialFeedback",  SoSeparator  ---
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoTransformerDragger::SoTransformerDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoTransformerDragger);

  // split-up to avoid one huge method
  this->build_catalog1();
  this->build_catalog2();
  this->build_catalog3();
  this->build_catalog4();
  this->build_catalog5();
  this->build_catalog6();

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("transformerDragger.iv",
                                       TRANSFORMERDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(TRANSFORMERDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));
  // FIXME: it doesn't look like this field is actually used or set
  // anywhere else but here. Investigate. 20011208 mortene.
  SO_KIT_ADD_FIELD(minDiscRotDot, (0.025f));

  SO_KIT_INIT_INSTANCE();

  this->setPartAsDefault("overallStyle", "transformerOverallStyle");
  this->setPartAsDefault("translator1", "transformerTranslator1");
  this->setPartAsDefault("translator2", "transformerTranslator2");
  this->setPartAsDefault("translator3", "transformerTranslator3");
  this->setPartAsDefault("translator4", "transformerTranslator4");
  this->setPartAsDefault("translator5", "transformerTranslator5");
  this->setPartAsDefault("translator6", "transformerTranslator6");
  this->setPartAsDefault("translator1Active", "transformerTranslator1Active");
  this->setPartAsDefault("translator2Active", "transformerTranslator2Active");
  this->setPartAsDefault("translator3Active", "transformerTranslator3Active");
  this->setPartAsDefault("translator4Active", "transformerTranslator4Active");
  this->setPartAsDefault("translator5Active", "transformerTranslator5Active");
  this->setPartAsDefault("translator6Active", "transformerTranslator6Active");
  this->setPartAsDefault("rotator1", "transformerRotator1");
  this->setPartAsDefault("rotator2", "transformerRotator2");
  this->setPartAsDefault("rotator3", "transformerRotator3");
  this->setPartAsDefault("rotator4", "transformerRotator4");
  this->setPartAsDefault("rotator5", "transformerRotator5");
  this->setPartAsDefault("rotator6", "transformerRotator6");
  this->setPartAsDefault("rotator1Active", "transformerRotator1Active");
  this->setPartAsDefault("rotator2Active", "transformerRotator2Active");
  this->setPartAsDefault("rotator3Active", "transformerRotator3Active");
  this->setPartAsDefault("rotator4Active", "transformerRotator4Active");
  this->setPartAsDefault("rotator5Active", "transformerRotator5Active");
  this->setPartAsDefault("rotator6Active", "transformerRotator6Active");
  this->setPartAsDefault("scale1", "transformerScale1");
  this->setPartAsDefault("scale2", "transformerScale2");
  this->setPartAsDefault("scale3", "transformerScale3");
  this->setPartAsDefault("scale4", "transformerScale4");
  this->setPartAsDefault("scale5", "transformerScale5");
  this->setPartAsDefault("scale6", "transformerScale6");
  this->setPartAsDefault("scale7", "transformerScale7");
  this->setPartAsDefault("scale8", "transformerScale8");
  this->setPartAsDefault("scale1Active", "transformerScale1Active");
  this->setPartAsDefault("scale2Active", "transformerScale2Active");
  this->setPartAsDefault("scale3Active", "transformerScale3Active");
  this->setPartAsDefault("scale4Active", "transformerScale4Active");
  this->setPartAsDefault("scale5Active", "transformerScale5Active");
  this->setPartAsDefault("scale6Active", "transformerScale6Active");
  this->setPartAsDefault("scale7Active", "transformerScale7Active");
  this->setPartAsDefault("scale8Active", "transformerScale8Active");
  this->setPartAsDefault("xAxisFeedbackActive", "transformerXAxisFeedbackActive");
  this->setPartAsDefault("xAxisFeedbackSelect", "transformerXAxisFeedbackSelect");
  this->setPartAsDefault("yAxisFeedbackActive", "transformerYAxisFeedbackActive");
  this->setPartAsDefault("yAxisFeedbackSelect", "transformerYAxisFeedbackSelect");
  this->setPartAsDefault("zAxisFeedbackActive", "transformerZAxisFeedbackActive");
  this->setPartAsDefault("zAxisFeedbackSelect", "transformerZAxisFeedbackSelect");
  this->setPartAsDefault("xCrosshairFeedback", "transformerXCrosshairFeedback");
  this->setPartAsDefault("yCrosshairFeedback", "transformerYCrosshairFeedback");
  this->setPartAsDefault("zCrosshairFeedback", "transformerZCrosshairFeedback");
  this->setPartAsDefault("xCircleFeedback", "transformerXCircleFeedback");
  this->setPartAsDefault("yCircleFeedback", "transformerYCircleFeedback");
  this->setPartAsDefault("zCircleFeedback", "transformerZCircleFeedback");
  this->setPartAsDefault("radialFeedback", "transformerRadialFeedback");
  this->setPartAsDefault("translateBoxFeedback", "transformerTranslateBoxFeedback");

  this->setPartAsDefault("scaleBoxFeedback", "transformerScaleBoxFeedback");
  this->setPartAsDefault("posXWallFeedback", "transformerPosXWallFeedback");
  this->setPartAsDefault("posYWallFeedback", "transformerPosYWallFeedback");
  this->setPartAsDefault("posZWallFeedback", "transformerPosZWallFeedback");
  this->setPartAsDefault("negXWallFeedback", "transformerNegXWallFeedback");
  this->setPartAsDefault("negYWallFeedback", "transformerNegYWallFeedback");
  this->setPartAsDefault("negZWallFeedback", "transformerNegZWallFeedback");
  this->setPartAsDefault("posXRoundWallFeedback", "transformerPosXRoundWallFeedback");
  this->setPartAsDefault("posYRoundWallFeedback", "transformerPosYRoundWallFeedback");
  this->setPartAsDefault("posZRoundWallFeedback", "transformerPosZRoundWallFeedback");
  this->setPartAsDefault("negXRoundWallFeedback", "transformerNegXRoundWallFeedback");
  this->setPartAsDefault("negYRoundWallFeedback", "transformerNegYRoundWallFeedback");
  this->setPartAsDefault("negZRoundWallFeedback", "transformerNegZRoundWallFeedback");

  this->state = INACTIVE;
  PRIVATE(this)->constraintState = CONSTRAINT_OFF;
  // FIXME: according to SGI classdoc, this flag is supposed to be
  // default TRUE?  Investigate. 20011208 mortene.
  PRIVATE(this)->locateHighlighting = FALSE;
  PRIVATE(this)->whatkind = WHATKIND_NONE;
  PRIVATE(this)->whatnum = -1;

  this->setAllPartSwitches(0, 0, 0);

  this->addStartCallback(SoTransformerDragger::startCB);
  this->addMotionCallback(SoTransformerDragger::motionCB);
  this->addFinishCallback(SoTransformerDragger::finishCB);
  this->addValueChangedCallback(SoTransformerDragger::valueChangedCB);
  this->addOtherEventCallback(SoTransformerDragger::metaKeyChangeCB);

  this->planeProj = new SbPlaneProjector;
  this->lineProj = new SbLineProjector;
  this->sphereProj = new SbSphereSectionProjector;
  this->cylProj = new SbCylinderPlaneProjector;

  this->translFieldSensor = new SoFieldSensor(SoTransformerDragger::fieldSensorCB, this);
  this->translFieldSensor->setPriority(0);
  this->scaleFieldSensor = new SoFieldSensor(SoTransformerDragger::fieldSensorCB, this);
  this->scaleFieldSensor->setPriority(0);
  this->rotateFieldSensor = new SoFieldSensor(SoTransformerDragger::fieldSensorCB, this);
  this->rotateFieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);

  // make sure these are not written if they have the default value.
  // FIXME: investigate why this is needed. There must be a
  // notification that is sent somewhere that causes the fields to
  // become non-default. pederb, 2003-04-01
  this->translatorSep.setDefault(TRUE);
  this->rotatorSep.setDefault(TRUE);
  this->scaleSep.setDefault(TRUE);
  this->translateBoxFeedbackSep.setDefault(TRUE);
  this->axisFeedbackSep.setDefault(TRUE);
  this->scale8LocateGroup.setDefault(TRUE);
  this->scale7LocateGroup.setDefault(TRUE);
  this->circleFeedbackSep.setDefault(TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoTransformerDragger::~SoTransformerDragger()
{
  delete this->planeProj;
  delete this->lineProj;
  delete this->sphereProj;
  delete this->cylProj;

  delete this->translFieldSensor;
  delete this->scaleFieldSensor;
  delete this->rotateFieldSensor;
}

// Doc in super.
SbBool
SoTransformerDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoTransformerDragger::fieldSensorCB(this, NULL);

    if (this->translFieldSensor->getAttachedField() != &this->translation) {
      this->translFieldSensor->attach(&this->translation);
    }
    if (this->scaleFieldSensor->getAttachedField() != &this->scaleFactor) {
      this->scaleFieldSensor->attach(&this->scaleFactor);
    }
    if (this->rotateFieldSensor->getAttachedField() != &this->rotation) {
      this->rotateFieldSensor->attach(&this->rotation);
    }
  }
  else {
    if (this->translFieldSensor->getAttachedField() != NULL) {
      this->translFieldSensor->detach();
    }
    if (this->scaleFieldSensor->getAttachedField() != NULL) {
      this->scaleFieldSensor->detach();
    }
    if (this->rotateFieldSensor->getAttachedField() != NULL) {
      this->rotateFieldSensor->detach();
    }
    inherited::setUpConnections(onoff, doitalways);
  }
  return !(this->connectionsSetUp = onoff);
}

// Convenience method used to call setDefault on similar fields.
//
// Note: keep the function name prefix to avoid name clashes with
// other dragger .cpp files for "--enable-compact" builds.
//
// FIXME: should collect these methods in a common method visible to
// all draggers implementing the exact same functionality. 20010826 mortene.
static void
SoTransformerDragger_set_default(SoDragger * dragger, const char * fmt,
                                 int minval, int maxval)
{
  SbString str;
  for (int i = minval; i <= maxval; i++) {
    str.sprintf(fmt, i);
    SoField * f = dragger->getField(str.getString());
    assert(f);
    f->setDefault(TRUE);
  }
}

// Doc in superclass.
void
SoTransformerDragger::setDefaultOnNonWritingFields(void)
{
  this->surroundScale.setDefault(TRUE);
  this->circleFeedbackAntiSquish.setDefault(TRUE);
  this->circleFeedbackTransform.setDefault(TRUE);
  this->axisFeedbackLocation.setDefault(TRUE);
  this->translateBoxFeedbackRotation.setDefault(TRUE);

  SoTransformerDragger_set_default(this, "translator%dLocateGroup", 1, 6);
  SoTransformerDragger_set_default(this, "rotator%dLocateGroup", 1, 6);
  SoTransformerDragger_set_default(this, "scale%dLocateGroup", 1, 6);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoTransformerDragger::fieldSensorCB(void *d, SoSensor *)
{
  SoTransformerDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoTransformerDragger::valueChangedCB(void *, SoDragger * d)
{
  SoTransformerDragger * thisp = THISP(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  SbVec3f trans, scale;
  SbRotation rot, scaleOrient;
  matrix.getTransform(trans, rot, scale, scaleOrient);

  thisp->translFieldSensor->detach();
  if (thisp->translation.getValue() != trans)
    thisp->translation = trans;
  thisp->translFieldSensor->attach(&thisp->translation);

  thisp->scaleFieldSensor->detach();
  if (thisp->scaleFactor.getValue() != scale)
    thisp->scaleFactor = scale;
  thisp->scaleFieldSensor->attach(&thisp->scaleFactor);

  thisp->rotateFieldSensor->detach();
  if (thisp->rotation.getValue() != rot) {
    thisp->rotation = rot;
  }
  thisp->rotateFieldSensor->attach(&thisp->rotation);
}

/*!
  Returns an indicator for the current operation executed on the
  dragger by the end-user -- or SoTransformerDragger::INACTIVE if
  none.
*/
SoTransformerDragger::State
SoTransformerDragger::getCurrentState(void)
{
  return this->state;
}

void
SoTransformerDragger::unsquishKnobs(void)
{
  this->updateAntiSquishList();
}

SbBool
SoTransformerDragger::isLocateHighlighting(void)
{
  return PRIVATE(this)->locateHighlighting;
}

void
SoTransformerDragger::setLocateHighlighting(SbBool onoff)
{
  // FIXME: I can't see that this flag is actually used anywhere..?
  // 20011208 mortene.
  PRIVATE(this)->locateHighlighting = onoff;
}

void
SoTransformerDragger::setColinearThreshold(int newval)
{
  SoTransformerDraggerP::colinearThreshold = newval;
}

int
SoTransformerDragger::getColinearThreshold(void)
{
  return SoTransformerDraggerP::colinearThreshold;
}

SbVec3f
SoTransformerDragger::getBoxPointInWorldSpace(const SbVec3f & pointonunitbox)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  mat.multRight(this->getLocalToWorldMatrix());
  SbVec3f ret;
  mat.multVecMatrix(pointonunitbox, ret);
  return ret;
}

SbVec3f
SoTransformerDragger::getBoxDirInWorldSpace(const SbVec3f & dironunitbox)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  mat.multRight(this->getLocalToWorldMatrix());
  SbVec3f ret;
  mat.multDirMatrix(dironunitbox, ret);
  return ret;
}

SbVec3f
SoTransformerDragger::getWorldPointInBoxSpace(const SbVec3f & pointinworldspace)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  mat.multLeft(this->getWorldToLocalMatrix());
  SbVec3f ret;
  mat.multVecMatrix(pointinworldspace, ret);
  return ret;
}

SbVec2f
SoTransformerDragger::getWorldPointInPixelSpace(const SbVec3f & thepoint)
{
  SbVec3f screenpt;
  this->getViewVolume().projectToScreen(thepoint, screenpt);
  return SbVec2f(screenpt[0], screenpt[1]);
}

SbVec3f
SoTransformerDragger::getInteractiveCenterInBoxSpace(void)
{
  if (PRIVATE(this)->ctrlDown) return PRIVATE(this)->ctrlOffset;
  else return SbVec3f(0.0f, 0.0f, 0.0f);
}

/*! \COININTERNAL */
void
SoTransformerDragger::startCB(void *, SoDragger * d)
{
  SoTransformerDragger * thisp = THISP(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoTransformerDragger::motionCB(void *, SoDragger * d)
{
  SoTransformerDragger * thisp = THISP(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoTransformerDragger::finishCB(void *, SoDragger * d)
{
  SoTransformerDragger * thisp = THISP(d);
  thisp->dragFinish();
}

/*! \COININTERNAL */
void
SoTransformerDragger::metaKeyChangeCB(void *, SoDragger *d)
{
  SoTransformerDragger * thisp = THISP(d);
  if (!thisp->isActive.getValue()) return;

  const SoEvent *event = thisp->getEvent();
  if (PRIVATE(thisp)->shiftDown != event->wasShiftDown()) {
    thisp->drag();
  }
  if (PRIVATE(thisp)->ctrlDown != event->wasCtrlDown()) {
    thisp->drag();
  }
}

// invalidate surround scale node, if it exists
static void
invalidate_surroundscale(SoBaseKit * kit)
{
  SoSurroundScale * ss = coin_safe_cast<SoSurroundScale *>(
    kit->getPart("surroundScale", FALSE)
    );
  if (ss) ss->invalidate();
}

/*! \COININTERNAL
  Called when dragger is selected (picked) by the user.
*/
void
SoTransformerDragger::dragStart(void)
{
  invalidate_surroundscale(this);

  int i;
  const SoPath *pickpath = this->getPickPath();
  const SoEvent *event = this->getEvent();

  SbBool found = FALSE;
  this->state = INACTIVE;

  SbVec3f startpt = this->getLocalStartingPoint();
  startpt = this->localToWorking(startpt);

  SbString str;
  if (!found) {
    for (i = 1; i <= 6; i++) {
      str.sprintf("translator%d", i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString())) >= 0 ||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 6) {
      found = TRUE;
      this->state = static_cast<State>((int(RIT_TRANSLATE) + (i-1)));
      PRIVATE(this)->whatkind = WHATKIND_TRANSLATE;
      PRIVATE(this)->whatnum = i;
      if (i <= 2) PRIVATE(this)->dimension = 1;
      else if (i <= 4) PRIVATE(this)->dimension = 0;
      else PRIVATE(this)->dimension = 2;
    }
  }

  if (!found) {
    for (i = 1; i <= 6; i++) {
      str.sprintf("rotator%d", i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString()))>= 0 ||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 6) {
      found = TRUE;
      this->state = static_cast<State>((int(RIT_X_ROTATE) + (i-1)));
      PRIVATE(this)->whatkind = WHATKIND_ROTATE;
      PRIVATE(this)->whatnum = i;
      if (i <= 2) PRIVATE(this)->dimension = 1;
      else if (i <= 4) PRIVATE(this)->dimension = 0;
      else PRIVATE(this)->dimension = 2;
    }
  }
  if (!found) {
    for (i = 1; i <= 8; i++) {
      str.sprintf("scale%d", i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString()))>= 0 ||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 8) {
      found = TRUE;
      this->state = static_cast<State>((int(PX_PY_PZ_3D_SCALE) + (i-1)));
      PRIVATE(this)->whatkind = WHATKIND_SCALE;
      PRIVATE(this)->whatnum = i;
    }
  }
  assert(found);

  PRIVATE(this)->ctrlDown = event->wasCtrlDown();
  PRIVATE(this)->shiftDown = event->wasShiftDown();
  PRIVATE(this)->ctrlOffset = this->calcCtrlOffset(startpt);

  switch(PRIVATE(this)->whatkind) {
  case WHATKIND_TRANSLATE:
    {
      SbVec3f n(0.0f, 0.0f, 0.0f);
      n[PRIVATE(this)->dimension] = 1.0f;
      this->planeProj->setPlane(SbPlane(n, startpt));
      this->lineProj->setLine(SbLine(startpt, startpt + n));
      PRIVATE(this)->constraintState = CONSTRAINT_OFF;
      if (PRIVATE(this)->shiftDown) {
        PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
      }
      SbLine myline(SbVec3f(0.0f, 0.0f, 0.0f), n);
      SoTranslation *t = SO_GET_ANY_PART(this, "axisFeedbackLocation", SoTranslation);
      t->translation = myline.getClosestPoint(startpt);

      this->setAllPartSwitches(SO_SWITCH_NONE, SO_SWITCH_NONE, SO_SWITCH_NONE);
      str.sprintf("translator%dSwitch", PRIVATE(this)->whatnum);
      this->setSwitchValue(str.getString(), 1);
      this->setSwitchValue("translateBoxFeedbackSwitch", SO_SWITCH_ALL);
      SoRotation * feedbackrot = coin_assert_cast<SoRotation *>(this->getAnyPart("translateBoxFeedbackRotation", TRUE));
      assert(feedbackrot);
      switch (PRIVATE(this)->whatnum) {
      default:
      case 1:
        feedbackrot->rotation = SbRotation::identity();
        break;
      case 2:
        feedbackrot->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), float(M_PI));
        break;
      case 3:
        feedbackrot->rotation = SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), float(M_PI)*0.5f);
        break;
      case 4:
        feedbackrot->rotation = SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), -float(M_PI)*0.5f);
        break;
      case 5:
        feedbackrot->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), float(M_PI)*0.5f);
        break;
      case 6:
        feedbackrot->rotation = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), -float(M_PI)*0.5f);
        break;
      }
      (void) this->setDynamicTranslatorSwitches(event);
    }
    break;
  case WHATKIND_SCALE:
    {
      SoTranslation *t = SO_GET_ANY_PART(this, "axisFeedbackLocation", SoTranslation);
      t->translation = startpt;
      this->lineProj->setLine(SbLine(SbVec3f(0.0f, 0.0f, 0.0f), startpt));
      PRIVATE(this)->constraintState = CONSTRAINT_OFF;
      if (PRIVATE(this)->shiftDown) {
        PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
      }

      str.sprintf("scale%dSwitch", PRIVATE(this)->whatnum);
      this->setAllPartSwitches(0, SO_SWITCH_NONE, SO_SWITCH_NONE);
      this->setSwitchValue(str.getString(), 1);
      (void) this->setDynamicScaleSwitches(event);
    }
    break;
  case WHATKIND_ROTATE:
    {
      SoTranslation *t = SO_GET_ANY_PART(this, "axisFeedbackLocation", SoTranslation);
      t->translation = startpt;
      this->sphereProj->setSphere(SbSphere(SbVec3f(0.0f, 0.0f, 0.0f), startpt.length()));
      this->sphereProj->setViewVolume(this->getViewVolume());
      this->sphereProj->setWorkingSpace(this->getWorkingToWorldMatrix());

      switch (this->getFrontOnProjector()) {
      case FRONT:
      case BACK:
        this->sphereProj->setFront(TRUE);
        break;
      default: // avoid warnings
      case USE_PICK:
        this->sphereProj->setFront(this->sphereProj->isPointInFront(startpt));
        break;
      }

      SbVec3f projpt = this->sphereProj->project(this->getNormalizedLocaterPosition());
      this->getWorkingToWorldMatrix().multVecMatrix(projpt, PRIVATE(this)->prevWorldHitPt);
      PRIVATE(this)->prevMotionMatrix = this->getMotionMatrix();

      PRIVATE(this)->constraintState = CONSTRAINT_OFF;
      if (!PRIVATE(this)->shiftDown) {
        PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
        // this plane is only used to find constraint direction
        this->planeProj->setPlane(SbPlane(startpt, startpt));
        PRIVATE(this)->normalizedStartLocaterPosition = this->getNormalizedLocaterPosition();
      }
      SoAntiSquish *squish = SO_GET_ANY_PART(this, "circleFeedbackAntiSquish", SoAntiSquish);
      SoAntiSquish::Sizing sizing;
      switch (PRIVATE(this)->dimension) {
      case 0: sizing = SoAntiSquish::X; break;
      case 1: sizing = SoAntiSquish::Y; break;
      case 2: sizing = SoAntiSquish::Z; break;
      default: assert(FALSE); sizing = SoAntiSquish::Z; // Dummy assignment to avoid compiler warning.
      }
      squish->sizing = sizing;
      squish->recalc();
      this->setAllPartSwitches(SO_SWITCH_NONE, 0, SO_SWITCH_NONE);
      (void) this->setDynamicRotatorSwitches(event);
    }
    break;
  default:
    assert(0 && "unknown whatkind");
    break;
  }
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoTransformerDragger::drag(void)
{
  switch(PRIVATE(this)->whatkind) {
  case WHATKIND_SCALE:
    this->dragScale();
    break;
  case WHATKIND_TRANSLATE:
    this->dragTranslate();
    break;
  case WHATKIND_ROTATE:
    this->dragRotate();
    break;
  default:
    assert(0 && "illegal whatkind");
    break;
  }
}

void
SoTransformerDragger::dragTranslate()
{
  SbVec3f startpt = this->getLocalStartingPoint();
  startpt = this->localToWorking(startpt);

  this->planeProj->setViewVolume(this->getViewVolume());
  this->planeProj->setWorkingSpace(this->getWorkingToWorldMatrix());
  SbVec3f projpt = this->planeProj->project(this->getNormalizedLocaterPosition());

  const SoEvent *event = this->getEvent();
  if (event->wasShiftDown() && PRIVATE(this)->constraintState == CONSTRAINT_OFF) {
    PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
    this->setStartLocaterPosition(event->getPosition());
  }
  else if (!event->wasShiftDown() && PRIVATE(this)->constraintState != CONSTRAINT_OFF) {
    PRIVATE(this)->constraintState = CONSTRAINT_OFF;
  }

  // Every time something changes (shift or ctrl is pressed), resave
  // the state so that the transformation to come starts with blank
  // sheets.
  if (this->setDynamicTranslatorSwitches(event)) {
    this->saveStartParameters();
    SbVec3f n(0.0f, 0.0f, 0.0f);
    n[PRIVATE(this)->dimension] = 1.0f;
    this->lineProj->setLine(SbLine(projpt, projpt+n));
    SbVec3f worldpt;
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldpt);
    this->setStartingPoint(worldpt);
    startpt = projpt;
  }

  SbVec3f motion;
  if (PRIVATE(this)->ctrlDown) {
    this->lineProj->setViewVolume(this->getViewVolume());
    this->lineProj->setWorkingSpace(this->getWorkingToWorldMatrix());
    projpt = this->lineProj->project(this->getNormalizedLocaterPosition());
    motion = projpt - startpt;
  }
  else {
    motion = projpt - startpt;

    switch(PRIVATE(this)->constraintState) {
    case CONSTRAINT_OFF:
      break;
    case CONSTRAINT_WAIT:
      if (this->isAdequateConstraintMotion()) {
        int biggest = 0;
        double bigval = fabs(motion[0]);
        if (fabs(motion[1]) > bigval) {
          biggest = 1;
          bigval = fabs(motion[1]);
        }
        if (fabs(motion[2]) > bigval) {
          biggest = 2;
        }

        // Set all but the constraint axis to 0
        motion[(biggest + 1) % 3] = 0.0f;
        motion[(biggest + 2) % 3] = 0.0f;

        PRIVATE(this)->constraintState = CONSTRAINT_X + biggest;
      }
      else {
        return;
      }
      break;
    case CONSTRAINT_X:
      motion[1] = 0.0f;
      motion[2] = 0.0f;
      break;
    case CONSTRAINT_Y:
      motion[0] = 0.0f;
      motion[2] = 0.0f;
      break;
    case CONSTRAINT_Z:
      motion[0] = 0.0f;
      motion[1] = 0.0f;
      break;
    }
  }

  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  this->setMotionMatrix(this->appendTranslation(this->getStartMotionMatrix(), motion, &mat));
  this->unsquishKnobs();
}

void
SoTransformerDragger::dragScale()
{
  SbVec3f startpt = this->getLocalStartingPoint();
  startpt = this->localToWorking(startpt);

  this->lineProj->setViewVolume(this->getViewVolume());
  this->lineProj->setWorkingSpace(this->getWorkingToWorldMatrix());
  SbVec3f projpt = this->lineProj->project(this->getNormalizedLocaterPosition());
  const SoEvent *event = this->getEvent();

  if (event->wasShiftDown() && PRIVATE(this)->constraintState == CONSTRAINT_OFF) {
    PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
    this->setStartLocaterPosition(event->getPosition());
  }
  else if (!event->wasShiftDown() && PRIVATE(this)->constraintState != CONSTRAINT_OFF) {
    this->saveStartParameters();
    PRIVATE(this)->constraintState = CONSTRAINT_OFF;
    this->lineProj->setLine(SbLine(SbVec3f(0.0f, 0.0f, 0.0f), projpt));
    PRIVATE(this)->ctrlOffset = this->calcCtrlOffset(projpt);
    startpt = projpt;
    SbVec3f worldpt;
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldpt);
    this->setStartingPoint(worldpt);
  }

  if (PRIVATE(this)->constraintState == CONSTRAINT_WAIT && this->isAdequateConstraintMotion()) {
    // detect which dimension user has moved mouse the most. Done by projecting
    // mouse positions onto the near plane, finding that world vector, and
    // transforming that world vector into working space.
    const SbViewVolume &vv = this->getViewVolume();
    const SbViewportRegion &vp = this->getViewportRegion();
    SbVec2s move = this->getLocaterPosition() - this->getStartLocaterPosition();
    SbVec2f normmove(
                   static_cast<float>(move[0])/static_cast<float>(vp.getViewportSizePixels()[0]),
                     static_cast<float>(move[1])/static_cast<float>(vp.getViewportSizePixels()[1])
                   );
    SbVec3f tmp = vv.getPlanePoint(vv.getNearDist(), SbVec2f(0.5f, 0.5f));
    SbVec3f dir = vv.getPlanePoint(vv.getNearDist(), SbVec2f(0.5f, 0.5f) + normmove);
    dir -= tmp;
    (void) dir.normalize(); // ok if null (no movement)
    this->getWorldToWorkingMatrix().multDirMatrix(dir, dir);
    int biggest = 0;
    double bigval = fabs(dir[0]);
    if (fabs(dir[1]) > bigval) {
      biggest = 1;
      bigval = fabs(dir[1]);
    }
    if (fabs(dir[2]) > bigval) {
      biggest = 2;
    }
    SbVec3f n(0.0f, 0.0f, 0.0f);
    n[biggest] = 1.0f;

    PRIVATE(this)->constraintState = CONSTRAINT_X + biggest;

    this->saveStartParameters();
    this->lineProj->setLine(SbLine(projpt, projpt+n));
    startpt = projpt;
    projpt[(biggest+1)%3] = 0.0f;
    projpt[(biggest+2)%3] = 0.0f;
    PRIVATE(this)->ctrlOffset = this->calcCtrlOffset(projpt);
    projpt = startpt;
    SbVec3f worldpt;
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldpt);
    this->setStartingPoint(worldpt);
  }

  // If the control key is pressed or released,
  // setDynamicScaleSwitches will return TRUE. When this happens, we
  // have to resave our start motionmatrix to be able to do correct
  // scaling. If we do not do this, the scaled object will jump
  // because the scale center is changed, but not the scale.
  if (this->setDynamicScaleSwitches(event)) {
    this->saveStartParameters();
    startpt = projpt;

    SbVec3f worldpt;
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldpt);
    this->setStartingPoint(worldpt);
  }

  if (PRIVATE(this)->constraintState == CONSTRAINT_WAIT) return;

  if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    int num = PRIVATE(this)->constraintState - CONSTRAINT_X;
    projpt[(num+1)%3] = 0.0f;
    projpt[(num+2)%3] = 0.0f;
    startpt[(num+1)%3] = 0.0f;
    startpt[(num+2)%3] = 0.0f;
  }

  SbVec3f center(0.0f, 0.0f, 0.0f);
  if (PRIVATE(this)->ctrlDown) {
    center -= PRIVATE(this)->ctrlOffset;
  }

  float orglen = (startpt-center).length();
  float currlen = (projpt-center).length();
  float scale = 0.0f;

  if (orglen > 0.0f) scale = currlen / orglen;
  if (scale > 0.0f && (startpt-center).dot(projpt-center) <= 0.0f) scale = 0.0f;

  SbVec3f scalevec(scale, scale, scale);
  if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    int num = PRIVATE(this)->constraintState - CONSTRAINT_X;
    scalevec[(num+1)%3] = 1.0f;
    scalevec[(num+2)%3] = 1.0f;
  }

  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  this->setMotionMatrix(this->appendScale(this->getStartMotionMatrix(),
                                          scalevec,
                                          center, &mat));
  this->unsquishKnobs();
}

// The dragRotate method has been implemented somewhat differently
// than the other drag functions. This is because of the unfortunate
// effect that happens when a non-uniform scale has been applied to
// the motion-matrix, and rotation is attempted to be done before the
// scale. Both operations are perfectly valid, but the end result is
// not what you'd expect; the transformation comes out sheared. To
// prevent this from happening, the rotation matrices are applied in
// world space, that is after all other transformations have been
// applied (scale and translation are applied in local
// space). What is basically done is this:
//
// Calculates the matrix (as done in SoDragger::appendRotation()):
//
// C := conversion
// P := rotcenter
// R := rot
// M := new motion matrix
// Mold := previous motionmatrix
//
// M = C^-1 * P^-1 * R * P * C * Mold
//
// What essentially happens is that we transform into C's coordinate
// system, then we move the rotation center to origo and apply the
// new rotation.  The rotation has now been applied, but we are in
// the wrong coordinate system, so we reapply the rotation center
// and the conversion. Finally we transform the matrix by the old
// transformation, which gives us the new rotated transformation.
//
// The rotation happens in the local coordinate system of the object
// if C = P = I, but if P is specified, then the rotation center
// will be adjusted before the rotation is applied. If the
// conversion matrix has been specified to be e.g:
//
// C = (Mold * W)^-1
//
// Then the resulting matrix looks something like this:
//
// M = ((Mold * W)^-1)^-1 * P^-1 * R * P * (Mold * W)^-1 * Mold
//   = Mold * W * P^-1 * R * P * W^-1 * Mold^-1 * Mold
//   = Mold * W * P^-1 * R * P * W^-1
//
// Take a closer look at the resulting matrix: It basically reverses
// the transformation. Instead of having the rotation applied before
// the old transformation, the rotation is applied after the old
// transformation. The rotation also happens in world space - that is
// the coordinate system is transformed to the world coordinate system
// before the rotation is applied around rotationcenter. Finally the
// matrix is transformed back to the local coordinate system.
void
SoTransformerDragger::dragRotate(void)
{
  // Update the sphere projector to the current view so that it
  // accurately can react to events.
  this->sphereProj->setViewVolume(this->getViewVolume());
  this->sphereProj->setWorkingSpace(this->getWorkingToWorldMatrix());

  const SoEvent *event = this->getEvent();

  SbVec3f startpt, projpt;
  startpt = this->getLocalStartingPoint();
  startpt = this->localToWorking(startpt);

  // If shift was pressed and not in non-constrained state, then
  // we switch to non-constrained state.
  if (event->wasShiftDown() && PRIVATE(this)->constraintState != CONSTRAINT_OFF) {
    PRIVATE(this)->constraintState = CONSTRAINT_OFF;
    projpt = this->sphereProj->project(this->getNormalizedLocaterPosition());
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, PRIVATE(this)->prevWorldHitPt);
    PRIVATE(this)->prevMotionMatrix = this->getMotionMatrix();
    this->saveStartParameters();
    this->setStartingPoint(PRIVATE(this)->prevWorldHitPt);
  }
  // If shift was released while in non-constrained state, then the
  // state jumps back to constrained.
  else if (!event->wasShiftDown() && PRIVATE(this)->constraintState == CONSTRAINT_OFF) {
    PRIVATE(this)->constraintState = CONSTRAINT_WAIT;
    this->setStartingPoint(PRIVATE(this)->prevWorldHitPt);
    startpt = this->getLocalStartingPoint();
    startpt = this->localToWorking(startpt);
    // This plane is only used to find the constraint direction
    this->planeProj->setPlane(SbPlane(startpt, startpt));
    this->setStartLocaterPosition(event->getPosition());
    PRIVATE(this)->normalizedStartLocaterPosition = this->getNormalizedLocaterPosition();
    this->saveStartParameters();
  }

  SbVec3f center(0.0f, 0.0f, 0.0f);
  if (PRIVATE(this)->ctrlDown) {
    // Adjust the center to be on the other side of the bounding box
    // when the control key has been pressed.
    center -= PRIVATE(this)->ctrlOffset * KNOB_DISTANCE;
  }

  // Show the necessary feedback geometry for rotation.
  (void) this->setDynamicRotatorSwitches(event);

  if (PRIVATE(this)->constraintState == CONSTRAINT_OFF) {
    SbVec3f worldcenter = this->getBoxPointInWorldSpace(center);
    SbVec3f prevworldprojpt = PRIVATE(this)->prevWorldHitPt;
    SbVec3f worldprojpt;

    // Find locater position in world space
    projpt = this->sphereProj->project(this->getNormalizedLocaterPosition());
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldprojpt);
    SbVec3f wppt = worldprojpt;

    // Find the projection vectors from the box center in world space.
    worldprojpt -= worldcenter;
    prevworldprojpt -= worldcenter;

    // FIXME: Without this test, the rotation behaves very strange,
    // especially at the edge/outside of the sphere. I will
    // investigate this some more to understand what happens.
    // 20040804 jornskaa

    // Normalize the vectors before finding the dotproduct angle
    // between them.
    if ((worldprojpt.normalize() > 0.0f) &&
        (prevworldprojpt.normalize() > 0.0f) &&
        (worldprojpt.dot(prevworldprojpt) > 0.8f)) {
      PRIVATE(this)->prevWorldHitPt = wppt;

      // Calculate the rotation from previous prevworldprojpt to
      // worldprojpt
      SbRotation rot(prevworldprojpt, worldprojpt);

      // Calculate new motionmatrix by rotating in world space to
      // prevent shearing with non-uniform scale.
      SbMatrix mat = this->getWorldToLocalMatrix();
      PRIVATE(this)->prevMotionMatrix = this->appendRotation(PRIVATE(this)->prevMotionMatrix, rot,
                                                    worldcenter, &mat);

      this->setMotionMatrix(PRIVATE(this)->prevMotionMatrix);
    }
  }
  else if (PRIVATE(this)->constraintState == CONSTRAINT_WAIT && this->isAdequateConstraintMotion()) {
    this->planeProj->setViewVolume(this->getViewVolume());
    this->planeProj->setWorkingSpace(this->getWorkingToWorldMatrix());
    projpt = this->planeProj->project(this->getNormalizedLocaterPosition());

    SbVec3f diff = projpt - startpt;
    diff[PRIVATE(this)->dimension] = 0.0f;

    int biggest = 0;
    double bigval = fabs(diff[0]);
    if (fabs(diff[1]) > bigval) {
      biggest = 1;
      bigval = fabs(diff[1]);
    }
    if (fabs(diff[2]) > bigval) {
      biggest = 2;
    }
    PRIVATE(this)->constraintState = CONSTRAINT_X + biggest;
    SbVec3f n(0.0f, 0.0f, 0.0f);
    n[biggest] = 1.0f;
    SbVec3f dim(0.0f, 0.0f, 0.0f);
    dim[PRIVATE(this)->dimension] = 1.0f;

    // set plane to do disc-rotate in
    this->planeProj->setPlane(SbPlane(SbVec3f(0.0f, 0.0f, 0.0f), dim, dim+n));
    (void) this->setDynamicRotatorSwitches(event);

    // Initialize prevWorldHitPt and prevMotionMatrix.
    projpt = this->planeProj->project(PRIVATE(this)->normalizedStartLocaterPosition);
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, PRIVATE(this)->prevWorldHitPt);
    PRIVATE(this)->prevMotionMatrix = this->getMotionMatrix();
  }
  if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    this->planeProj->setViewVolume(this->getViewVolume());
    this->planeProj->setWorkingSpace(this->getWorkingToWorldMatrix());

    SbVec3f worldcenter = this->getBoxPointInWorldSpace(center);
    SbVec3f prevworldprojpt;
    SbVec3f prevprojpt;
    SbVec3f worldprojpt;

    prevworldprojpt = PRIVATE(this)->prevWorldHitPt;

    // Find current locater position projected onto the plane in world
    // space.
    projpt = this->planeProj->project(this->getNormalizedLocaterPosition());
    this->getWorkingToWorldMatrix().multVecMatrix(projpt, worldprojpt);

    // Project the vectors onto the projection plane in world space.
    // This is needed to make the rotation vectors more numerically
    // stable especially when the dragger has been scaled to be very
    // small before rotation is attempted.
    SbPlane plane = this->planeProj->getPlane();
    plane.transform(this->getWorkingToWorldMatrix());
    prevworldprojpt -= plane.getNormal() * plane.getDistance(prevworldprojpt);
    worldprojpt -= plane.getNormal() * plane.getDistance(worldprojpt);

    // Save the values for the new projection point before it is
    // altered.
    SbVec3f wppt = worldprojpt;

    // Adjust the rotation vectors of the dragger to match the center
    // of the dragger in working space and world space.
    projpt -= center;
    prevworldprojpt -= worldcenter;
    worldprojpt -= worldcenter;

    // Make sure the length of the projected vector is greater than
    // a scalar constant. This ensures that the rotation is defined
    // and there is little room for error that might happen when the
    // vectors come very close to the rotation center.
    if (projpt.sqrLength() > 0.1f) {
      // Since we are using incremental changes, the changes will
      // never be very big, and the dot product between the
      // rotation-vectors should always be above zero, and never
      // negative because that indicates the angle between the vectors
      // is above PI/2. This might happen if dragging is done over the
      // rotation center, but we do not allow that and just wait until
      // the locater comes into the valid range before rotating.

      // Normalize the vectors before finding the dotproduct angle
      // between them.
      if ((prevworldprojpt.normalize() > 0.0f) &&
          (worldprojpt.normalize() > 0.0f) &&
          (prevworldprojpt.dot(worldprojpt) > 0.3f)) { // 0.3 == 72.5degrees
        PRIVATE(this)->prevWorldHitPt = wppt;

        // Rotate between the two points in the plane
        SbRotation rot(prevworldprojpt, worldprojpt);

        // Rotate in world space to prevent shearing when having
        // non-uniform scale.
        SbMatrix mat = this->getWorldToLocalMatrix();
        PRIVATE(this)->prevMotionMatrix = (this->appendRotation(PRIVATE(this)->prevMotionMatrix, rot,
                                                       worldcenter, &mat));

        this->setMotionMatrix(PRIVATE(this)->prevMotionMatrix);
      }
    }
  }
  this->unsquishKnobs();
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoTransformerDragger::dragFinish(void)
{
  switch (PRIVATE(this)->whatkind) {
  case WHATKIND_TRANSLATE:
    this->setSwitchValue("translateBoxFeedbackSwitch", SO_SWITCH_NONE);
    break;
  case WHATKIND_ROTATE:
  this->setSwitchValue("xCircleFeedbackSwitch", SO_SWITCH_NONE);
  this->setSwitchValue("yCircleFeedbackSwitch", SO_SWITCH_NONE);
  this->setSwitchValue("zCircleFeedbackSwitch", SO_SWITCH_NONE);
    break;
  case WHATKIND_SCALE:
    this->setSwitchValue("radialFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("posXWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negXWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("posYWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negYWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("posZWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negZWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("scaleBoxFeedbackSwitch", SO_SWITCH_NONE);
    break;
  default:
    assert(0 && "unknown whatkind");
    break;
  }

  PRIVATE(this)->whatkind = WHATKIND_NONE;
  this->state = INACTIVE;
  PRIVATE(this)->constraintState = CONSTRAINT_OFF;
  this->setAllPartSwitches(0,0,0);
  this->setSwitchValue("xAxisFeedbackSwitch", SO_SWITCH_NONE);
  this->setSwitchValue("yAxisFeedbackSwitch", SO_SWITCH_NONE);
  this->setSwitchValue("zAxisFeedbackSwitch", SO_SWITCH_NONE);

#if COIN_DEBUG && 0 // used to debug motion matrix (pederb, 20000225)
  SbMatrix m = this->getMotionMatrix();
  SbRotation r,so;
  SbVec3f t,s;

  fprintf(stderr,"motion matrix:\n");
  m.print(stderr);
  m.getTransform(t, r, s, so);
  SbVec3f rx, sox;
  float ra, soa;
  r.getValue(rx, ra);
  so.getValue(sox, soa);
  fprintf(stderr,
          "\nt: %g %g %g\n"
          "r: %g %g %g, %g\n"
          "s: %g %g %g\n"
          "so: %g %g %g, %g\n\n",
          t[0], t[1], t[2],
          rx[0], rx[1], rx[2], ra,
          s[0], s[1], s[2],
          sox[0], sox[1], sox[2], soa);
#endif // debug code
  invalidate_surroundscale(this);
}

void
SoTransformerDragger::updateAntiSquishList(void)
{
  if (this->antiSquishList.getLength() == 0) {
    SoSeparator *top = SO_GET_ANY_PART(this, "topSeparator", SoSeparator);
    assert(top);

    SoSearchAction sa;
    sa.setInterest(SoSearchAction::ALL);
    sa.setType(SoAntiSquish::getClassTypeId());
    sa.setSearchingAll(TRUE);
    sa.apply(top);

    SoPathList &pl = sa.getPaths();
    for (int i = 0; i < pl.getLength(); i++) {
      SoFullPath * path = reclassify_cast<SoFullPath *>(pl[i]);
      SoNode * tail = path->getTail();
      int j, n = this->antiSquishList.getLength();
      for (j = 0; j < n; j++) {
        if (this->antiSquishList[j] == tail) break;
      }
      if (j == n)
        this->antiSquishList.append(path->getTail());
    }
  }
  int n = this->antiSquishList.getLength();
  for (int i = 0; i < n; i++) {
    SoAntiSquish * squishy = coin_assert_cast<SoAntiSquish *>(this->antiSquishList[i]);
    squishy->recalc();
  }
}

void
SoTransformerDragger::setAllPartSwitches(int scalewhich, int rotatewhich, int translatewhich)
{
  int i;
  SoSwitch *sw;
  SbString str;

  for (i = 1; i <= 6; i++) {
    str.sprintf("translator%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, translatewhich);
  }
  for (i = 1; i <= 6; i++) {
    str.sprintf("rotator%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, rotatewhich);
  }
  for (i = 1; i <= 8; i++) {
    str.sprintf("scale%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, scalewhich);
  }
}

/*!
  Not implemented in Coin; should probably not have been public in the
  Open Inventor API.  We'll consider to implement it if requested.
*/
int
SoTransformerDragger::getMouseGestureDirection(SbBool COIN_UNUSED_ARG(x_ok), SbBool COIN_UNUSED_ARG(y_ok), SbBool COIN_UNUSED_ARG(z_ok))
{
  COIN_OBSOLETED();
  return -1;
}

/*!
  Not implemented in Coin; should probably not have been public in the
  Open Inventor API.  We'll consider to implement it if requested.
*/
int
SoTransformerDragger::getIgnoreAxis(SbVec2f COIN_UNUSED_ARG(axis[3][2]), SbBool COIN_UNUSED_ARG(x_ok), SbBool COIN_UNUSED_ARG(y_ok), SbBool COIN_UNUSED_ARG(z_ok))
{
  COIN_OBSOLETED();
  return -1;
}

/*!
  Not implemented in Coin; should probably not have been public in the
  Open Inventor API.  We'll consider to implement it if requested.
*/
void
SoTransformerDragger::makeMinorAxisPerpendicularIfColinear(SbVec2f COIN_UNUSED_ARG(origin), SbVec2f COIN_UNUSED_ARG(axisends[3][2]), int COIN_UNUSED_ARG(index_a), int COIN_UNUSED_ARG(index_b))
{
  COIN_OBSOLETED();
}

/*!
  Not implemented in Coin; should probably not have been public in the
  Open Inventor API.  We'll consider to implement it if requested.
*/
SbBool
SoTransformerDragger::isColinear(SbVec2f COIN_UNUSED_ARG(a1[2]), SbVec2f COIN_UNUSED_ARG(a2[2]), int COIN_UNUSED_ARG(pixels))
{
  COIN_OBSOLETED();
  return FALSE;
}

void
SoTransformerDragger::getSurroundScaleMatrices(SbMatrix & mat, SbMatrix & inv)
{
  if (this->surroundScale.getValue()) {
    this->getPartToLocalMatrix("surroundScale", mat, inv);
  }
  else {
    mat = inv = SbMatrix::identity();
  }
}

SoNode *
SoTransformerDragger::getNodeFieldNode(const char *fieldname)
{
  SoField *field = this->getField(fieldname);
  assert(field != NULL);
  assert(field->isOfType(SoSFNode::getClassTypeId()));
  assert(coin_assert_cast<SoSFNode *>(field)->getValue() != NULL);
  return coin_assert_cast<SoSFNode *>(field)->getValue();
}

SbMatrix
SoTransformerDragger::getWorkingToWorldMatrix()
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  mat.multRight(this->getLocalToWorldMatrix());
  return mat;
}

SbMatrix
SoTransformerDragger::getWorldToWorkingMatrix(void)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  mat.multLeft(this->getWorldToLocalMatrix());
  return mat;
}

SbVec3f
SoTransformerDragger::localToWorking(const SbVec3f &v)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  SbVec3f ret;
  inv.multVecMatrix(v, ret);
  return ret;
}

SbVec3f
SoTransformerDragger::workingToLocal(const SbVec3f &v)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  SbVec3f ret;
  mat.multVecMatrix(v, ret);
  return ret;
}

SbVec3f
SoTransformerDragger::calcCtrlOffset(const SbVec3f &startpt)
{
  SbVec3f v = startpt;
  for (int i = 0; i < 3; i++) {
    if (v[i] < -0.8) v[i] = -1.0f;
    else if (v[i] > 0.8) v[i] = 1.0f;
    else v[i] = 0.0f;
  }
  return v;
}

void
SoTransformerDragger::setSwitchValue(const char *str, const int which)
{
  SoSwitch *sw = SO_GET_ANY_PART(this, str, SoSwitch);
  SoInteractionKit::setSwitchValue(sw, which);
}

SbBool
SoTransformerDragger::setDynamicTranslatorSwitches(const SoEvent *event)
{
  SbBool changed = FALSE;
  if (PRIVATE(this)->ctrlDown != event->wasCtrlDown()) {
    changed = TRUE;
    PRIVATE(this)->ctrlDown = !PRIVATE(this)->ctrlDown;
  }
  if (PRIVATE(this)->shiftDown != event->wasShiftDown()) {
    changed = TRUE;
    PRIVATE(this)->shiftDown = !PRIVATE(this)->shiftDown;
  }

  SbString str;

  if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    int which = PRIVATE(this)->constraintState - CONSTRAINT_X;
    str.sprintf("%cAxisFeedbackSwitch", 'x' + which);
    this->setSwitchValue(str.getString(), 0);
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (which+1)%3);
    this->setSwitchValue(str.getString(), SO_SWITCH_NONE);
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (which+2)%3);
    this->setSwitchValue(str.getString(), SO_SWITCH_NONE);
  }
  else {
    str.sprintf("%cAxisFeedbackSwitch", 'x' + PRIVATE(this)->dimension);
    this->setSwitchValue(str.getString(), PRIVATE(this)->ctrlDown ? 0 : SO_SWITCH_NONE);
    int val = PRIVATE(this)->shiftDown ? 1 : 0;
    if (PRIVATE(this)->ctrlDown) val = SO_SWITCH_NONE;
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (PRIVATE(this)->dimension+1)%3);
    this->setSwitchValue(str.getString(), val);
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (PRIVATE(this)->dimension+2)%3);
    this->setSwitchValue(str.getString(), val);
  }
  return changed;
}

SbBool
SoTransformerDragger::setDynamicScaleSwitches(const SoEvent *event)
{
  SbBool changed = FALSE;
  if (PRIVATE(this)->ctrlDown != event->wasCtrlDown()) {
    changed = TRUE;
    PRIVATE(this)->ctrlDown = !PRIVATE(this)->ctrlDown;
  }
  if (PRIVATE(this)->shiftDown != event->wasShiftDown()) {
    changed = TRUE;
    PRIVATE(this)->shiftDown = !PRIVATE(this)->shiftDown;
  }
  if (PRIVATE(this)->constraintState == CONSTRAINT_WAIT) {
    this->setSwitchValue("xAxisFeedbackSwitch", 1);
    this->setSwitchValue("yAxisFeedbackSwitch", 1);
    this->setSwitchValue("zAxisFeedbackSwitch", 1);
    this->setSwitchValue("radialFeedbackSwitch", SO_SWITCH_NONE);
  }
  else if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    int which = PRIVATE(this)->constraintState - CONSTRAINT_X;
    SbString str;
    str.sprintf("%cAxisFeedbackSwitch", 'x' + which);
    this->setSwitchValue(str.getString(), 0);
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (which+1)%3);
    this->setSwitchValue(str.getString(), SO_SWITCH_NONE);
    str.sprintf("%cAxisFeedbackSwitch", 'x' + (which+2)%3);
    this->setSwitchValue(str.getString(), SO_SWITCH_NONE);
    this->setSwitchValue("radialFeedbackSwitch", SO_SWITCH_NONE);
  }
  else {
    this->setSwitchValue("xAxisFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("yAxisFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("zAxisFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("radialFeedbackSwitch", 0);
  }

  this->setSwitchValue("scaleBoxFeedbackSwitch", PRIVATE(this)->shiftDown ? 0 : SO_SWITCH_NONE);

  if (PRIVATE(this)->ctrlDown) {
    SbVec3f pt = this->getLocalStartingPoint();
    if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
      int num = PRIVATE(this)->constraintState - CONSTRAINT_X;
      pt[(num+1)%3] = 0.0f;
      pt[(num+2)%3] = 0.0f;
    }
    SbString str;
    for (int i = 0; i < 3; i++) {
      str.sprintf("pos%cWallFeedbackSwitch", 'X' + i);
      this->setSwitchValue(str.getString(), pt[i] < 0.0f ? 0 : SO_SWITCH_NONE);
      str.sprintf("neg%cWallFeedbackSwitch", 'X' + i);
      this->setSwitchValue(str.getString(), pt[i] > 0.0f ? 0 : SO_SWITCH_NONE);
    }
  }
  else {
    this->setSwitchValue("posXWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negXWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("posYWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negYWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("posZWallFeedbackSwitch", SO_SWITCH_NONE);
    this->setSwitchValue("negZWallFeedbackSwitch", SO_SWITCH_NONE);
  }
  return changed;
}


SbBool
SoTransformerDragger::setDynamicRotatorSwitches(const SoEvent *event)
{
  SbBool changed = FALSE;
  if (PRIVATE(this)->ctrlDown != event->wasCtrlDown()) {
    changed = TRUE;
    PRIVATE(this)->ctrlDown = !PRIVATE(this)->ctrlDown;
  }
  if (PRIVATE(this)->shiftDown != event->wasShiftDown()) {
    changed = TRUE;
    PRIVATE(this)->shiftDown = !PRIVATE(this)->shiftDown;
  }

  SbString str;
  {
    int axis0 = PRIVATE(this)->whatnum-1;
    int axis1 = (axis0 & 1) ? axis0 - 1 : axis0 + 1;

    str.sprintf("rotator%dSwitch", axis0 + 1);
    this->setSwitchValue(str.getString(), 1);
    str.sprintf("rotator%dSwitch", axis1 + 1);
    this->setSwitchValue(str.getString(), PRIVATE(this)->ctrlDown ? 0 : 1);
  }

  int axisval[3];
  int circleval[3];
  int dim = PRIVATE(this)->dimension;

  if (PRIVATE(this)->constraintState == CONSTRAINT_WAIT) {
    axisval[dim] = SO_SWITCH_NONE;
    axisval[(dim+1)%3] = 1;
    axisval[(dim+2)%3] = 1;
    circleval[dim] = SO_SWITCH_NONE;
    circleval[(dim+1)%3] = 0;
    circleval[(dim+2)%3] = 0;
  }
  else if (PRIVATE(this)->constraintState >= CONSTRAINT_X) {
    dim = PRIVATE(this)->constraintState - CONSTRAINT_X;
    axisval[dim] = 0;
    axisval[(dim+1)%3] = SO_SWITCH_NONE;
    axisval[(dim+2)%3] = SO_SWITCH_NONE;

    const SbVec3f &n = this->planeProj->getPlane().getNormal();
    circleval[0] = n[0] != 0.0f ? 0 : SO_SWITCH_NONE;
    circleval[1] = n[1] != 0.0f ? 0 : SO_SWITCH_NONE;
    circleval[2] = n[2] != 0.0f ? 0 : SO_SWITCH_NONE;
  }
  else {
    circleval[0] = 0;
    circleval[1] = 0;
    circleval[2] = 0;
    axisval[0] = SO_SWITCH_NONE;
    axisval[1] = SO_SWITCH_NONE;
    axisval[2] = SO_SWITCH_NONE;
  }
  if (PRIVATE(this)->ctrlDown) {
    SoTransform *transform = SO_GET_ANY_PART(this, "circleFeedbackTransform", SoTransform);
    SbVec3f offset = -PRIVATE(this)->ctrlOffset * KNOB_DISTANCE;
    if (transform->translation.getValue() != offset)
      transform->translation = offset;
    if (transform->scaleFactor.getValue() != SbVec3f(2.0f, 2.0f, 2.0f))
      transform->scaleFactor = SbVec3f(2.0f, 2.0f, 2.0f);
    this->setSwitchValue("circleFeedbackTransformSwitch", SO_SWITCH_ALL);
  }
  else {
    this->setSwitchValue("circleFeedbackTransformSwitch", 0);
  }

  this->setSwitchValue("xAxisFeedbackSwitch", axisval[0]);
  this->setSwitchValue("yAxisFeedbackSwitch", axisval[1]);
  this->setSwitchValue("zAxisFeedbackSwitch", axisval[2]);
  this->setSwitchValue("xCircleFeedbackSwitch", circleval[0]);
  this->setSwitchValue("yCircleFeedbackSwitch", circleval[1]);
  this->setSwitchValue("zCircleFeedbackSwitch", circleval[2]);

  return changed;
}


// Undefine these again, as some of them are also used in other
// dragger sourcecode files (which causes trouble when using the
// compact build hack where all .cpp files are included into all.cpp).

#undef WHATKIND_NONE
#undef WHATKIND_SCALE
#undef WHATKIND_TRANSLATE
#undef WHATKIND_ROTATE

#undef CONSTRAINT_OFF
#undef CONSTRAINT_WAIT
#undef CONSTRAINT_X
#undef CONSTRAINT_Y
#undef CONSTRAINT_Z

#undef KNOB_DISTANCE

#undef PRIVATE
#undef THISP

#ifdef COIN_TEST_SUITE

#include <Inventor/SbDict.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodes/SoSeparator.h>

static
SoCallbackAction::Response
register_cb(void * data, SoCallbackAction * action, const SoNode * node)
{
  assert(data);
  SbDict * dict = static_cast<SbDict *>(data);
  dict->enter(reinterpret_cast<uintptr_t>(node), NULL);
  return SoCallbackAction::CONTINUE;
}

static
void
ensure_unique_cb(uintptr_t entry, void * value, void * data)
{
  SbDict * copydict = static_cast<SbDict *>(data);
  void * val = NULL;
  BOOST_ASSERT(!copydict->find(entry, val));
}

BOOST_AUTO_TEST_CASE(dragger_deep_copy)
{
  SbDict origdict, copydict;

  SoSeparator * root = new SoSeparator;
  root->setName("dragger_deep_copy_root");
  root->ref();
  root->addChild(new SoTransformerDragger);

  SoSeparator * copy = static_cast<SoSeparator *>(root->copy());
  assert(copy);
  copy->setName("dragger_deep_copy_copy");
  copy->ref();

  {
    SoCallbackAction cba;
    cba.setCallbackAll(TRUE);

    cba.addPreCallback(SoNode::getClassTypeId(), register_cb, &origdict);
    cba.apply(root);
  }

  {
    SoCallbackAction cba;
    cba.setCallbackAll(TRUE);

    cba.addPreCallback(SoNode::getClassTypeId(), register_cb, &copydict);
    cba.apply(copy);
  }

  SbPList keys, values;

  origdict.makePList(keys, values);
  const int origdictsize = keys.getLength();

  keys.truncate(0);
  values.truncate(0);
  copydict.makePList(keys, values);
  const int copydictsize = keys.getLength();

  BOOST_ASSERT(origdictsize == copydictsize);

  // make sure pointer sets have an empty union
  origdict.applyToAll(ensure_unique_cb, &copydict);

  root->unref();
  copy->unref();
}

#endif // COIN_TEST_SUITE

#endif // HAVE_DRAGGERS
