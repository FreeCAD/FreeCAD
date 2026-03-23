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
  \class SoHandleBoxDragger SoHandleBoxDragger.h Inventor/draggers/SoHandleBoxDragger.h
  \brief The SoHandleBoxDragger class provides support for interactive scaling and translation.

  \ingroup coin_draggers

  \DRAGGER_DEFAULT_SCREENSHOT

  <center>
  \image html handlebox.png "Screen Shot of Default Dragger"
  </center>

  A handle box dragger is convenient to use for letting the user
  interact with geometry that can be scaled (uniformly or in a
  non-uniform way) and translated in 3D.

  The dragger consists of a "cube" of interaction geometry. The
  end-user can click and drag any side of the cube to translate the
  dragger and click and drag any of the corner or edge markers for
  scaling operations.
*/
// FIXME: Should include an URL-link to the default geometry-file?
// Plus a small usage example.  20011113 mortene.

#include <Inventor/draggers/SoHandleBoxDragger.h>

#include <cstring>

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/projectors/SbPlaneProjector.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/actions/SoGetMatrixAction.h>

#include <data/draggerDefaults/handleBoxDragger.h>

#include "nodekits/SoSubKitP.h"
#include "coindefs.h"
#include "SbBasicP.h"

/*!
  \var SoSFVec3f SoHandleBoxDragger::scaleFactor

  Continuously updated to contain the current vector of scaling along
  the X, Y and Z axes.
*/

/*!
  \var SoSFVec3f SoHandleBoxDragger::translation

  Continuously updated to contain the current translation from the
  dragger's local origo position.

  The application programmer applying this dragger in his code should
  connect the relevant node fields in the scene to this field to make
  it follow the dragger.
*/


/*!
  \var SoFieldSensor * SoHandleBoxDragger::translFieldSensor
  \COININTERNAL
*/
/*!
  \var SoFieldSensor * SoHandleBoxDragger::scaleFieldSensor
  \COININTERNAL
*/

#define WHATKIND_NONE       0
#define WHATKIND_TRANSLATOR 1
#define WHATKIND_EXTRUDER   2
#define WHATKIND_UNIFORM    3

#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3
#define CONSTRAINT_Z    4

static int uniform_ctrl_lookup[8][6] = {
  { 1,4,5,2,3,6 },
  { 1,4,6,2,3,5 },
  { 2,4,5,1,3,6 },
  { 2,4,6,1,3,5 },
  { 1,3,5,2,4,6 },
  { 1,3,6,2,4,5 },
  { 2,3,5,1,4,6 },
  { 2,3,6,1,4,5 }
};

class SoHandleBoxDraggerP {
public:
};

SO_KIT_SOURCE(SoHandleBoxDragger);


/*!
  \copydetails SoDragger::initClass(void)
*/
void
SoHandleBoxDragger::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoHandleBoxDragger, SO_FROM_INVENTOR_1);
}

// FIXME: document which parts need to be present in the geometry
// scene graph, and what role they play in the dragger. 20010913 mortene.
/*!
  \DRAGGER_CONSTRUCTOR

  \NODEKIT_PRE_DIAGRAM

  \verbatim
  CLASS SoHandleBoxDragger
  -->"this"
        "callbackList"
        "topSeparator"
           "motionMatrix"
  -->      "surroundScale"
           "geomSeparator"
  -->         "drawStyle"
  -->         "translator1Switch"
  -->            "translator1"
  -->            "translator1Active"
  -->         "translator2Switch"
  -->            "translator2"
  -->            "translator2Active"
  -->         "translator3Switch"
  -->            "translator3"
  -->            "translator3Active"
  -->         "translator4Switch"
  -->            "translator4"
  -->            "translator4Active"
  -->         "translator5Switch"
  -->            "translator5"
  -->            "translator5Active"
  -->         "translator6Switch"
  -->            "translator6"
  -->            "translator6Active"
  -->         "extruder1Switch"
  -->            "extruder1"
  -->            "extruder1Active"
  -->         "extruder2Switch"
  -->            "extruder2"
  -->            "extruder2Active"
  -->         "extruder3Switch"
  -->            "extruder3"
  -->            "extruder3Active"
  -->         "extruder4Switch"
  -->            "extruder4"
  -->            "extruder4Active"
  -->         "extruder5Switch"
  -->            "extruder5"
  -->            "extruder5Active"
  -->         "extruder6Switch"
  -->            "extruder6"
  -->            "extruder6Active"
  -->         "uniform1Switch"
  -->            "uniform1"
  -->            "uniform1Active"
  -->         "uniform2Switch"
  -->            "uniform2"
  -->            "uniform2Active"
  -->         "uniform3Switch"
  -->            "uniform3"
  -->            "uniform3Active"
  -->         "uniform4Switch"
  -->            "uniform4"
  -->            "uniform4Active"
  -->         "uniform5Switch"
  -->            "uniform5"
  -->            "uniform5Active"
  -->         "uniform6Switch"
  -->            "uniform6"
  -->            "uniform6Active"
  -->         "uniform7Switch"
  -->            "uniform7"
  -->            "uniform7Active"
  -->         "uniform8Switch"
  -->            "uniform8"
  -->            "uniform8Active"
  -->         "arrowTranslation"
  -->         "arrow1Switch"
  -->            "arrow1"
  -->         "arrow2Switch"
  -->            "arrow2"
  -->         "arrow3Switch"
  -->            "arrow3"
  -->         "arrow4Switch"
  -->            "arrow4"
  -->         "arrow5Switch"
  -->            "arrow5"
  -->         "arrow6Switch"
  -->            "arrow6"
  \endverbatim

  \NODEKIT_POST_DIAGRAM


  \NODEKIT_PRE_TABLE

  \verbatim
  CLASS SoHandleBoxDragger
  PVT   "this",  SoHandleBoxDragger  ---
        "callbackList",  SoNodeKitListPart [ SoCallback, SoEventCallback ]
  PVT   "topSeparator",  SoSeparator  ---
  PVT   "motionMatrix",  SoMatrixTransform  ---
        "surroundScale",  SoSurroundScale  ---
  PVT   "geomSeparator",  SoSeparator  ---
  PVT   "drawStyle",  SoDrawStyle  ---
  PVT   "translator1Switch",  SoSwitch  ---
        "translator1",  SoSeparator  ---
        "translator1Active",  SoSeparator  ---
  PVT   "translator2Switch",  SoSwitch  ---
        "translator2",  SoSeparator  ---
        "translator2Active",  SoSeparator  ---
  PVT   "translator3Switch",  SoSwitch  ---
        "translator3",  SoSeparator  ---
        "translator3Active",  SoSeparator  ---
  PVT   "translator4Switch",  SoSwitch  ---
        "translator4",  SoSeparator  ---
        "translator4Active",  SoSeparator  ---
  PVT   "translator5Switch",  SoSwitch  ---
        "translator5",  SoSeparator  ---
        "translator5Active",  SoSeparator  ---
  PVT   "translator6Switch",  SoSwitch  ---
        "translator6",  SoSeparator  ---
        "translator6Active",  SoSeparator  ---
  PVT   "extruder1Switch",  SoSwitch  ---
        "extruder1",  SoSeparator  ---
        "extruder1Active",  SoSeparator  ---
  PVT   "extruder2Switch",  SoSwitch  ---
        "extruder2",  SoSeparator  ---
        "extruder2Active",  SoSeparator  ---
  PVT   "extruder3Switch",  SoSwitch  ---
        "extruder3",  SoSeparator  ---
        "extruder3Active",  SoSeparator  ---
  PVT   "extruder4Switch",  SoSwitch  ---
        "extruder4",  SoSeparator  ---
        "extruder4Active",  SoSeparator  ---
  PVT   "extruder5Switch",  SoSwitch  ---
        "extruder5",  SoSeparator  ---
        "extruder5Active",  SoSeparator  ---
  PVT   "extruder6Switch",  SoSwitch  ---
        "extruder6",  SoSeparator  ---
        "extruder6Active",  SoSeparator  ---
  PVT   "uniform1Switch",  SoSwitch  ---
        "uniform1",  SoSeparator  ---
        "uniform1Active",  SoSeparator  ---
  PVT   "uniform2Switch",  SoSwitch  ---
        "uniform2",  SoSeparator  ---
        "uniform2Active",  SoSeparator  ---
  PVT   "uniform3Switch",  SoSwitch  ---
        "uniform3",  SoSeparator  ---
        "uniform3Active",  SoSeparator  ---
  PVT   "uniform4Switch",  SoSwitch  ---
        "uniform4",  SoSeparator  ---
        "uniform4Active",  SoSeparator  ---
  PVT   "uniform5Switch",  SoSwitch  ---
        "uniform5",  SoSeparator  ---
        "uniform5Active",  SoSeparator  ---
  PVT   "uniform6Switch",  SoSwitch  ---
        "uniform6",  SoSeparator  ---
        "uniform6Active",  SoSeparator  ---
  PVT   "uniform7Switch",  SoSwitch  ---
        "uniform7",  SoSeparator  ---
        "uniform7Active",  SoSeparator  ---
  PVT   "uniform8Switch",  SoSwitch  ---
        "uniform8",  SoSeparator  ---
        "uniform8Active",  SoSeparator  ---
  PVT   "arrowTranslation",  SoTranslation  ---
  PVT   "arrow1Switch",  SoSwitch  ---
        "arrow1",  SoSeparator  ---
  PVT   "arrow2Switch",  SoSwitch  ---
        "arrow2",  SoSeparator  ---
  PVT   "arrow3Switch",  SoSwitch  ---
        "arrow3",  SoSeparator  ---
  PVT   "arrow4Switch",  SoSwitch  ---
        "arrow4",  SoSeparator  ---
  PVT   "arrow5Switch",  SoSwitch  ---
        "arrow5",  SoSeparator  ---
  PVT   "arrow6Switch",  SoSwitch  ---
        "arrow6",  SoSeparator  ---
  \endverbatim

  \NODEKIT_POST_TABLE
*/
SoHandleBoxDragger::SoHandleBoxDragger(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoHandleBoxDragger);

  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale, TRUE, topSeparator, geomSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(drawStyle, SoDrawStyle, TRUE, geomSeparator, translator1Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1Switch, SoSwitch, TRUE, geomSeparator, translator2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1, SoSeparator, TRUE, translator1Switch, translator1Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator1Active, SoSeparator, TRUE, translator1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2Switch, SoSwitch, TRUE, geomSeparator, translator3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2, SoSeparator, TRUE, translator2Switch, translator2Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator2Active, SoSeparator, TRUE, translator2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3Switch, SoSwitch, TRUE, geomSeparator, translator4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3, SoSeparator, TRUE, translator3Switch, translator3Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator3Active, SoSeparator, TRUE, translator3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4Switch, SoSwitch, TRUE, geomSeparator, translator5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4, SoSeparator, TRUE, translator4Switch, translator4Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator4Active, SoSeparator, TRUE, translator4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5Switch, SoSwitch, TRUE, geomSeparator, translator6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5, SoSeparator, TRUE, translator5Switch, translator5Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator5Active, SoSeparator, TRUE, translator5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6Switch, SoSwitch, TRUE, geomSeparator, extruder1Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6, SoSeparator, TRUE, translator6Switch, translator6Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator6Active, SoSeparator, TRUE, translator6Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder1Switch, SoSwitch, TRUE, geomSeparator, extruder2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder1, SoSeparator, TRUE, extruder1Switch, extruder1Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder1Active, SoSeparator, TRUE, extruder1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder2Switch, SoSwitch, TRUE, geomSeparator, extruder3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder2, SoSeparator, TRUE, extruder2Switch, extruder2Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder2Active, SoSeparator, TRUE, extruder2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder3Switch, SoSwitch, TRUE, geomSeparator, extruder4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder3, SoSeparator, TRUE, extruder3Switch, extruder3Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder3Active, SoSeparator, TRUE, extruder3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder4Switch, SoSwitch, TRUE, geomSeparator, extruder5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder4, SoSeparator, TRUE, extruder4Switch, extruder4Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder4Active, SoSeparator, TRUE, extruder4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder5Switch, SoSwitch, TRUE, geomSeparator, extruder6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder5, SoSeparator, TRUE, extruder5Switch, extruder5Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder5Active, SoSeparator, TRUE, extruder5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder6Switch, SoSwitch, TRUE, geomSeparator, uniform1Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder6, SoSeparator, TRUE, extruder6Switch, extruder6Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(extruder6Active, SoSeparator, TRUE, extruder6Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform1Switch, SoSwitch, TRUE, geomSeparator, uniform2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform1, SoSeparator, TRUE, uniform1Switch, uniform1Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform1Active, SoSeparator, TRUE, uniform1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform2Switch, SoSwitch, TRUE, geomSeparator, uniform3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform2, SoSeparator, TRUE, uniform2Switch, uniform2Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform2Active, SoSeparator, TRUE, uniform2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform3Switch, SoSwitch, TRUE, geomSeparator, uniform4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform3, SoSeparator, TRUE, uniform3Switch, uniform3Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform3Active, SoSeparator, TRUE, uniform3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform4Switch, SoSwitch, TRUE, geomSeparator, uniform5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform4, SoSeparator, TRUE, uniform4Switch, uniform4Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform4Active, SoSeparator, TRUE, uniform4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform5Switch, SoSwitch, TRUE, geomSeparator, uniform6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform5, SoSeparator, TRUE, uniform5Switch, uniform5Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform5Active, SoSeparator, TRUE, uniform5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform6Switch, SoSwitch, TRUE, geomSeparator, uniform7Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform6, SoSeparator, TRUE, uniform6Switch, uniform6Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform6Active, SoSeparator, TRUE, uniform6Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform7Switch, SoSwitch, TRUE, geomSeparator, uniform8Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform7, SoSeparator, TRUE, uniform7Switch, uniform7Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform7Active, SoSeparator, TRUE, uniform7Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform8Switch, SoSwitch, TRUE, geomSeparator, arrowTranslation, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform8, SoSeparator, TRUE, uniform8Switch, uniform8Active, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(uniform8Active, SoSeparator, TRUE, uniform8Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrowTranslation, SoTranslation, TRUE, geomSeparator, arrow1Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow1Switch, SoSwitch, TRUE, geomSeparator, arrow2Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow1, SoSeparator, TRUE, arrow1Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow2Switch, SoSwitch, TRUE, geomSeparator, arrow3Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow2, SoSeparator, TRUE, arrow2Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow3Switch, SoSwitch, TRUE, geomSeparator, arrow4Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow3, SoSeparator, TRUE, arrow3Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow4Switch, SoSwitch, TRUE, geomSeparator, arrow5Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow4, SoSeparator, TRUE, arrow4Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow5Switch, SoSwitch, TRUE, geomSeparator, arrow6Switch, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow5, SoSeparator, TRUE, arrow5Switch, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow6Switch, SoSwitch, TRUE, geomSeparator, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(arrow6, SoSeparator, TRUE, arrow6Switch, "", TRUE);

  if (SO_KIT_IS_FIRST_INSTANCE()) {
    SoInteractionKit::readDefaultParts("handleBoxDragger.iv",
                                       HANDLEBOXDRAGGER_draggergeometry,
                                       static_cast<int>(strlen(HANDLEBOXDRAGGER_draggergeometry)));
  }

  SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));

  SO_KIT_INIT_INSTANCE();

  this->setPartAsDefault("translator1", "handleBoxTranslator1");
  this->setPartAsDefault("translator2", "handleBoxTranslator2");
  this->setPartAsDefault("translator3", "handleBoxTranslator3");
  this->setPartAsDefault("translator4", "handleBoxTranslator4");
  this->setPartAsDefault("translator5", "handleBoxTranslator5");
  this->setPartAsDefault("translator6", "handleBoxTranslator6");

  this->setPartAsDefault("translator1Active", "handleBoxTranslator1Active");
  this->setPartAsDefault("translator2Active", "handleBoxTranslator2Active");
  this->setPartAsDefault("translator3Active", "handleBoxTranslator3Active");
  this->setPartAsDefault("translator4Active", "handleBoxTranslator4Active");
  this->setPartAsDefault("translator5Active", "handleBoxTranslator5Active");
  this->setPartAsDefault("translator6Active", "handleBoxTranslator6Active");

  this->setPartAsDefault("extruder1", "handleBoxExtruder1");
  this->setPartAsDefault("extruder2", "handleBoxExtruder2");
  this->setPartAsDefault("extruder3", "handleBoxExtruder3");
  this->setPartAsDefault("extruder4", "handleBoxExtruder4");
  this->setPartAsDefault("extruder5", "handleBoxExtruder5");
  this->setPartAsDefault("extruder6", "handleBoxExtruder6");

  this->setPartAsDefault("extruder1Active", "handleBoxExtruder1Active");
  this->setPartAsDefault("extruder2Active", "handleBoxExtruder2Active");
  this->setPartAsDefault("extruder3Active", "handleBoxExtruder3Active");
  this->setPartAsDefault("extruder4Active", "handleBoxExtruder4Active");
  this->setPartAsDefault("extruder5Active", "handleBoxExtruder5Active");
  this->setPartAsDefault("extruder6Active", "handleBoxExtruder6Active");

  this->setPartAsDefault("uniform1", "handleBoxUniform1");
  this->setPartAsDefault("uniform2", "handleBoxUniform2");
  this->setPartAsDefault("uniform3", "handleBoxUniform3");
  this->setPartAsDefault("uniform4", "handleBoxUniform4");
  this->setPartAsDefault("uniform5", "handleBoxUniform5");
  this->setPartAsDefault("uniform6", "handleBoxUniform6");
  this->setPartAsDefault("uniform7", "handleBoxUniform7");
  this->setPartAsDefault("uniform8", "handleBoxUniform8");

  this->setPartAsDefault("uniform1Active", "handleBoxUniform1Active");
  this->setPartAsDefault("uniform2Active", "handleBoxUniform2Active");
  this->setPartAsDefault("uniform3Active", "handleBoxUniform3Active");
  this->setPartAsDefault("uniform4Active", "handleBoxUniform4Active");
  this->setPartAsDefault("uniform5Active", "handleBoxUniform5Active");
  this->setPartAsDefault("uniform6Active", "handleBoxUniform6Active");
  this->setPartAsDefault("uniform7Active", "handleBoxUniform7Active");
  this->setPartAsDefault("uniform8Active", "handleBoxUniform8Active");

  this->setPartAsDefault("arrow1", "handleBoxArrow1");
  this->setPartAsDefault("arrow2", "handleBoxArrow2");
  this->setPartAsDefault("arrow3", "handleBoxArrow3");
  this->setPartAsDefault("arrow4", "handleBoxArrow4");
  this->setPartAsDefault("arrow5", "handleBoxArrow5");
  this->setPartAsDefault("arrow6", "handleBoxArrow6");

  this->constraintState = CONSTRAINT_OFF;
  this->whatkind = WHATKIND_NONE;

  this->setAllPartsActive(FALSE);

  this->planeProj = new SbPlaneProjector;
  this->lineProj = new SbLineProjector;

  this->addStartCallback(SoHandleBoxDragger::startCB);
  this->addMotionCallback(SoHandleBoxDragger::motionCB);
  this->addFinishCallback(SoHandleBoxDragger::finishCB);
  this->addValueChangedCallback(SoHandleBoxDragger::valueChangedCB);
  this->addOtherEventCallback(SoHandleBoxDragger::metaKeyChangeCB);

  this->translFieldSensor = new SoFieldSensor(SoHandleBoxDragger::fieldSensorCB, this);
  this->translFieldSensor->setPriority(0);
  this->scaleFieldSensor = new SoFieldSensor(SoHandleBoxDragger::fieldSensorCB, this);
  this->scaleFieldSensor->setPriority(0);

  this->setUpConnections(TRUE, TRUE);
}

/*!
  Protected destructor.

  (Dragger classes are derived from SoBase, so they are reference
  counted and automatically destroyed when their reference count goes
  to 0.)
 */
SoHandleBoxDragger::~SoHandleBoxDragger()
{
  delete this->lineProj;
  delete this->planeProj;
  delete this->translFieldSensor;
  delete this->scaleFieldSensor;
}

// Doc in superclass.
SbBool
SoHandleBoxDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
  if (!doitalways && this->connectionsSetUp == onoff) return onoff;

  if (onoff) {
    inherited::setUpConnections(onoff, doitalways);

    SoHandleBoxDragger::fieldSensorCB(this, NULL);

    if (this->translFieldSensor->getAttachedField() != &this->translation) {
      this->translFieldSensor->attach(&this->translation);
    }
    if (this->scaleFieldSensor->getAttachedField() != &this->scaleFactor) {
      this->scaleFieldSensor->attach(&this->scaleFactor);
    }

  }
  else {
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
SoHandleBoxDragger::setDefaultOnNonWritingFields(void)
{
  this->surroundScale.setDefault(TRUE);
  this->arrowTranslation.setDefault(TRUE);
  this->drawStyle.setDefault(TRUE);

  inherited::setDefaultOnNonWritingFields();
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::fieldSensorCB(void * d, SoSensor *)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
  SbMatrix matrix = thisp->getMotionMatrix();
  thisp->workFieldsIntoTransform(matrix);
  thisp->setMotionMatrix(matrix);
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::valueChangedCB(void * COIN_UNUSED_ARG(f), SoDragger * d)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
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
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::startCB(void *, SoDragger * d)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
  thisp->dragStart();
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::motionCB(void *, SoDragger * d)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
  thisp->drag();
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::finishCB(void *, SoDragger * d)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
  thisp->dragFinish();
}

/*! \COININTERNAL */
void
SoHandleBoxDragger::metaKeyChangeCB(void *, SoDragger * d)
{
  SoHandleBoxDragger * thisp = static_cast<SoHandleBoxDragger *>(d);
  if (!thisp->isActive.getValue()) return;

  const SoEvent *event = thisp->getEvent();
  if (SO_KEY_RELEASE_EVENT(event, LEFT_SHIFT) ||
      SO_KEY_RELEASE_EVENT(event, RIGHT_SHIFT)) {
    if (thisp->constraintState != CONSTRAINT_OFF) thisp->drag();
  }
  else if (thisp->ctrlDown != event->wasCtrlDown()) {
    thisp->ctrlDown = !thisp->ctrlDown;
    thisp->updateSwitches();
  }
}

// Invalidate surround scale node, if it exists.
//
// Note: keep the function name prefix to avoid name clashes with
// other dragger .cpp files for "--enable-compact" builds.
//
// FIXME: should collect these methods in a common method visible to
// all draggers implementing the exact same functionality. 20010826 mortene.
static void
SoHandleBoxDragger_invalidate_surroundscale(SoBaseKit * kit)
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
SoHandleBoxDragger::dragStart(void)
{
  SoHandleBoxDragger_invalidate_surroundscale(this);

  static const char translatorname[] = "translator";
  static const char extrudername[] = "extruder";
  static const char uniformname[] = "uniform";

  const SoPath *pickpath = this->getPickPath();

  SbBool found = FALSE;
  this->whatkind = WHATKIND_NONE;
  this->whatnum = 0;

  int i;
  SbString str;
  if (!found) {
    for (i = 1; i <= 6; i++) {
      str.sprintf("%s%d", translatorname, i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString())) >= 0||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 6) {
      found = TRUE;
      this->whatkind = WHATKIND_TRANSLATOR;
      this->whatnum = i;
    }
  }

  if (!found) {
    for (i = 1; i <= 6; i++) {
      str.sprintf("%s%d", extrudername, i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString()))>= 0 ||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 6) {
      found = TRUE;
      this->whatkind = WHATKIND_EXTRUDER;
      this->whatnum = i;
    }
  }
  if (!found) {
    for (i = 1; i <= 8; i++) {
      str.sprintf("%s%d", uniformname, i);
      if (pickpath->findNode(this->getNodeFieldNode(str.getString()))>= 0 ||
          this->getSurrogatePartPickedName() == str.getString()) break;
    }
    if (i <= 8) {
      found = TRUE;
      this->whatkind = WHATKIND_UNIFORM;
      this->whatnum = i;
    }
  }
  assert(found);
  if (!found) return;

  SbVec3f startPt = this->getLocalStartingPoint();

  switch(this->whatkind) {
  case WHATKIND_TRANSLATOR:
    {
      SbVec3f n;
      if (this->whatnum <= 2) {
        n = SbVec3f(0.0f, 1.0f, 0.0f);
      }
      else if (this->whatnum <= 4) {
        n = SbVec3f(1.0f, 0.0f, 0.0f);
      }
      else {
        n = SbVec3f(0.0f, 0.0f, 1.0f);
      }
      SbVec3f localPt;
      {
        SbMatrix mat, inv;
        this->getSurroundScaleMatrices(mat, inv);
        inv.multVecMatrix(startPt, localPt);
      }
      this->planeProj->setPlane(SbPlane(n, startPt));
      SbLine myline(SbVec3f(0.0f, 0.0f, 0.0f), n);
      SoTranslation *t = SO_GET_ANY_PART(this, "arrowTranslation", SoTranslation);
      t->translation = myline.getClosestPoint(localPt);
      if (this->getEvent()->wasShiftDown()) {
        this->getLocalToWorldMatrix().multVecMatrix(startPt, this->worldRestartPt);
        this->constraintState = CONSTRAINT_WAIT;
      }
    }
    break;
  case WHATKIND_EXTRUDER:
    this->lineProj->setLine(SbLine(this->getDraggerCenter(), startPt));
    this->ctrlOffset = this->calcCtrlOffset(startPt);
    break;
  case WHATKIND_UNIFORM:
    this->lineProj->setLine(SbLine(this->getDraggerCenter(), startPt));
    this->ctrlOffset = this->calcCtrlOffset(startPt);
    break;
  }
  this->ctrlDown = this->getEvent()->wasCtrlDown();
  this->updateSwitches();
}

/*! \COININTERNAL
  Called when user drags the mouse after picking the dragger.
*/
void
SoHandleBoxDragger::drag(void)
{
  SbVec3f startPt = this->getLocalStartingPoint();

  if (this->whatkind == WHATKIND_TRANSLATOR) {
    this->planeProj->setViewVolume(this->getViewVolume());
    this->planeProj->setWorkingSpace(this->getLocalToWorldMatrix());
    SbVec3f projPt = this->planeProj->project(this->getNormalizedLocaterPosition());

    const SoEvent *event = this->getEvent();
    if (event->wasShiftDown() && this->constraintState == CONSTRAINT_OFF) {
      this->constraintState = CONSTRAINT_WAIT;
      this->setStartLocaterPosition(event->getPosition());
      this->getLocalToWorldMatrix().multVecMatrix(projPt, this->worldRestartPt);
    }
    else if (!event->wasShiftDown() && this->constraintState != CONSTRAINT_OFF) {
      this->constraintState = CONSTRAINT_OFF;
      this->updateArrows();
    }

    SbVec3f motion, localrestartpt;
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
        int biggest = 0;
        double bigval = fabs(newmotion[0]);
        if (fabs(newmotion[1]) > bigval) {
          biggest = 1;
          bigval = fabs(newmotion[1]);
        }
        if (fabs(newmotion[2]) > bigval) {
          biggest = 2;
        }
        motion[biggest] += newmotion[biggest];
        this->constraintState = CONSTRAINT_X + biggest;
        this->updateArrows();
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
    case CONSTRAINT_Z:
      motion[2] += projPt[2] - localrestartpt[2];
    }
    this->setMotionMatrix(this->appendTranslation(this->getStartMotionMatrix(), motion));
  }
  else {
    this->lineProj->setViewVolume(this->getViewVolume());
    this->lineProj->setWorkingSpace(this->getLocalToWorldMatrix());
    SbVec3f projPt = this->lineProj->project(this->getNormalizedLocaterPosition());
    SbVec3f center = this->getDraggerCenter();
    if (this->getEvent()->wasCtrlDown()) {
      center += this->ctrlOffset;
    }

    float orglen = (startPt-center).length();
    float currlen = (projPt-center).length();
    float scale = 0.0f;

    if (orglen > 0.0f) scale = currlen / orglen;
    if (scale > 0.0f && (startPt-center).dot(projPt-center) <= 0.0f) scale = 0.0f;

    SbVec3f scalevec(scale, scale, scale);
    if (this->whatkind == WHATKIND_EXTRUDER) {
      if (this->whatnum <= 2) scalevec[0] = scalevec[2] = 1.0f;
      else if (this->whatnum <= 4) scalevec[1] = scalevec[2] = 1.0f;
      else scalevec[0] = scalevec[1] = 1.0f;
    }

    this->setMotionMatrix(this->appendScale(this->getStartMotionMatrix(),
                                            scalevec,
                                            center));
  }
}

/*! \COININTERNAL
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void
SoHandleBoxDragger::dragFinish(void)
{
  this->constraintState = CONSTRAINT_OFF;
  this->whatkind = WHATKIND_NONE;
  this->setAllPartsActive(FALSE);

  SoHandleBoxDragger_invalidate_surroundscale(this);
}

/*!
  Activate or deactivate all dragger geometry parts.
*/
void
SoHandleBoxDragger::setAllPartsActive(SbBool onoroff)
{
  int i;
  int val = onoroff ? 1 : 0;
  SoSwitch *sw;
  SbString str;
  for (i = 1; i <= 6; i++) {
    str.sprintf("translator%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, val);
  }
  for (i = 1; i <= 6; i++) {
    str.sprintf("extruder%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, val);
  }
  for (i = 1; i <= 8; i++) {
    str.sprintf("uniform%dSwitch", i);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, val);
  }
  this->updateArrows();
}

// Return node pointer from a SoSFNode field. Does misc sanity
// checking for robustness.
SoNode *
SoHandleBoxDragger::getNodeFieldNode(const char * fieldname)
{
  SoField * field = this->getField(fieldname);
  assert(field != NULL);
  assert(coin_assert_cast<SoSFNode *>(field)->getValue() != NULL);
  return coin_assert_cast<SoSFNode *>(field)->getValue();
}

void
SoHandleBoxDragger::updateSwitches(void)
{
  int i;
  SbString str;
  SoSwitch *sw;

  if (this->whatkind == WHATKIND_UNIFORM) {
    if (this->ctrlDown) {
      const int *ptr = uniform_ctrl_lookup[this->whatnum-1];
      for (i = 0; i < 6; i++) {
        str.sprintf("extruder%dSwitch", ptr[i]);
        sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
        SoInteractionKit::setSwitchValue(sw, i < 3 ? 1 : 0);
      }
    }
    else {
      for (i = 1; i <= 6; i++) {
        str.sprintf("extruder%dSwitch", i);
        sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
        SoInteractionKit::setSwitchValue(sw, 1);
      }
    }
    str.sprintf("uniform%dSwitch", this->whatnum);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);
  }
  else if (this->whatkind == WHATKIND_EXTRUDER) {
    int othernum = ((this->whatnum-1) & ~1) + 1;
    if (othernum == this->whatnum) othernum++;

    str.sprintf("extruder%dSwitch", this->whatnum);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, 1);
    str.sprintf("extruder%dSwitch", othernum);
    sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->ctrlDown ? 0 : 1);
  }
  else {
    this->setAllPartsActive(TRUE);
    this->updateArrows();
  }
}

void
SoHandleBoxDragger::updateArrows(void)
{
  int i;
  SbString str;
  SoSwitch *sw;

  if (this->constraintState >= CONSTRAINT_X) {
    int onval = -1;
    switch (this->constraintState) {
    case CONSTRAINT_X:
      onval = 3;
      break;
    case CONSTRAINT_Y:
      onval = 1;
      break;
    case CONSTRAINT_Z:
      onval = 5;
      break;
    }
    for (i = 1; i <= 6; i++) {
      str.sprintf("arrow%dSwitch", i);
      sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
      if (i == onval || i == onval + 1) {
        SoInteractionKit::setSwitchValue(sw, 0);
      }
      else {
        SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
      }
    }
  }
  else if (this->whatkind == WHATKIND_TRANSLATOR) {
    int num = (this->whatnum-1) & ~1;
    for (i = 0; i < 6; i++) {
      str.sprintf("arrow%dSwitch", i+1);
      sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
      if (i == num || i == num+1) {
        SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
      }
      else {
        SoInteractionKit::setSwitchValue(sw, 0);
      }
    }
  }
  else {
    for (i = 1; i <= 6; i++) {
      str.sprintf("arrow%dSwitch", i);
      sw = SO_GET_ANY_PART(this, str.getString(), SoSwitch);
      SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    }
  }
}

void
SoHandleBoxDragger::getSurroundScaleMatrices(SbMatrix &mat, SbMatrix &inv)
{
  if (this->surroundScale.getValue()) {
    this->getPartToLocalMatrix("surroundScale", mat, inv);
  }
  else {
    mat = inv = SbMatrix::identity();
  }
}

SbVec3f
SoHandleBoxDragger::getDraggerCenter(void)
{
  SbMatrix mat, inv;
  this->getSurroundScaleMatrices(mat, inv);
  return SbVec3f(mat[3][0], mat[3][1], mat[3][2]);
}

SbVec3f
SoHandleBoxDragger::calcCtrlOffset(const SbVec3f startpt)
{
  SbMatrix m, inv;
  this->getSurroundScaleMatrices(m, inv);
  SbVec3f v = SbVec3f(m[3][0], m[3][1], m[3][2]) - startpt;

  for (int i = 0; i < 3; i++) {
    v[i] *= inv[i][i];
    if (v[i] < -0.95) v[i] = -1.0f;
    else if (v[i] > 0.95) v[i] = 1.0f;
    else v[i] = 0.0f;
    v[i] *= m[i][i];
  }
  return v;
}

#undef WHATKIND_NONE
#undef WHATKIND_TRANSLATOR
#undef WHATKIND_EXTRUDER
#undef WHATKIND_UNIFORM
#undef CONSTRAINT_OFF
#undef CONSTRAINT_WAIT
#undef CONSTRAINT_X
#undef CONSTRAINT_Y
#undef CONSTRAINT_Z

#endif // HAVE_DRAGGERS
