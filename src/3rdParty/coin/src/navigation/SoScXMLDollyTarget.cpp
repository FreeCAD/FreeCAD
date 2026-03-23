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

#include <Inventor/navigation/SoScXMLDollyTarget.h>

/*!
  \class SoScXMLDollyTarget SoScXMLDollyTarget.h Inventor/scxml/SoScXMLDollyTarget.h
  \brief SCXML event target service for zoom behaviour.

  Events:

    x-coin-navigation.Dolly.*

    BEGIN
      _sessionid {string}
      mouseposition {SbVec2f}
      [absminfocaldistance] {float}
      [absmaxfocaldistance] {float}
      [setfocaldistance] {float}
      [motiontype] {string:exponential,linear}

    UPDATE
      _sessionid {string}
      mouseposition {SbVec2f}

    END
      _sessionid {string}
      mouseposition {SbVec2f}

    JUMP
      _sessionid {string}
      focaldistance {float}

    STEP_IN
      _sessionid
      [count=1] {float}
      [absminfocaldistance] {float}
      [motiontype] {string:exponential,linear}

    STEP_OUT
      _sessionid
      [count=1] {float}
      [absmaxfocaldistance] {float}
      [motiontype] {string:exponential,linear}

  \ingroup coin_navigation
  \since Coin 3.1
*/

#include <cassert>
#include <string>
#include <cfloat>

#include <memory>

#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoFrustumCamera.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/SoScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/C/threads/mutex.h>
#include <Inventor/navigation/SoScXMLNavigation.h>
#include "threads/threadsutilp.h"
#include "scxml/SbStringConvert.h"
#include "tidbitsp.h"
#include "coindefs.h"

namespace {

class DollyData : public SoScXMLNavigationTarget::Data {
public:
  DollyData(void) {
    lastpos = SbVec2f(0, 0);
    rangeislimited = FALSE;
    absminfocaldistance = 0.0f;
    absmaxfocaldistance = 0.0f;
    motionislinear = FALSE;
  }

  SbVec2f lastpos;
  SbBool rangeislimited;
  float absminfocaldistance;
  float absmaxfocaldistance;
  SbBool motionislinear;

};

static SoScXMLNavigationTarget::Data * NewDollyData(void) { return new DollyData; }

} // namespace

class SoScXMLDollyTarget::PImpl {
public:
  static SbName BEGIN;
  static SbName UPDATE;
  static SbName END;
  static SbName JUMP;
  static SbName STEP_IN;
  static SbName STEP_OUT;

};

SbName SoScXMLDollyTarget::PImpl::BEGIN;
SbName SoScXMLDollyTarget::PImpl::UPDATE;
SbName SoScXMLDollyTarget::PImpl::END;
SbName SoScXMLDollyTarget::PImpl::JUMP;
SbName SoScXMLDollyTarget::PImpl::STEP_IN;
SbName SoScXMLDollyTarget::PImpl::STEP_OUT;

// *************************************************************************

#define PRIVATE ((obj)->pimpl)

SCXML_OBJECT_SOURCE(SoScXMLDollyTarget);

void
SoScXMLDollyTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLDollyTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX COIN_NAVIGATION_DOLLY_TARGET_EVENT_PREFIX
  PImpl::BEGIN    = SbName(EVENT_PREFIX ".BEGIN");
  PImpl::UPDATE   = SbName(EVENT_PREFIX ".UPDATE");
  PImpl::END      = SbName(EVENT_PREFIX ".END");
  PImpl::JUMP     = SbName(EVENT_PREFIX ".JUMP");
  PImpl::STEP_IN  = SbName(EVENT_PREFIX ".STEP_IN");
  PImpl::STEP_OUT = SbName(EVENT_PREFIX ".STEP_OUT");
#undef EVENT_PREFIX
}

void
SoScXMLDollyTarget::cleanClass(void)
{
  SoScXMLDollyTarget::classTypeId = SoType::badType();
}

SoScXMLDollyTarget * SoScXMLDollyTarget::theSingleton = NULL;

SoScXMLDollyTarget *
SoScXMLDollyTarget::constructSingleton(void)
{
  assert(SoScXMLDollyTarget::theSingleton == NULL);
  SoScXMLDollyTarget::theSingleton =
    static_cast<SoScXMLDollyTarget *>(SoScXMLDollyTarget::classTypeId.createInstance());
  return SoScXMLDollyTarget::theSingleton;
}

void
SoScXMLDollyTarget::destructSingleton(void)
{
  assert(SoScXMLDollyTarget::theSingleton != NULL);
  delete SoScXMLDollyTarget::theSingleton;
  SoScXMLDollyTarget::theSingleton = NULL;
}

SoScXMLDollyTarget *
SoScXMLDollyTarget::singleton(void)
{
  assert(SoScXMLDollyTarget::theSingleton != NULL);
  return SoScXMLDollyTarget::theSingleton;
}

/*!
  Returns the full name for the dolly BEGIN SCXML event.
*/
const SbName &
SoScXMLDollyTarget::BEGIN(void)
{
  return PImpl::BEGIN;
}

/*!
  Returns the full name for the dolly UPDATE SCXML event.
*/
const SbName &
SoScXMLDollyTarget::UPDATE(void)
{
  return PImpl::UPDATE;
}

/*!
  Returns the full name for the dolly END SCXML event.
*/
const SbName &
SoScXMLDollyTarget::END(void)
{
  return PImpl::END;
}

/*!
  Returns the full name for the dolly JUMP SCXML event.
*/
const SbName &
SoScXMLDollyTarget::JUMP(void)
{
  return PImpl::JUMP;
}

/*!
  Returns the full name for the dolly STEP_IN SCXML event.
*/
const SbName &
SoScXMLDollyTarget::STEP_IN(void)
{
  return PImpl::STEP_IN;
}

/*!
  Returns the full name for the dolly STEP_OUT SCXML event.
*/
const SbName &
SoScXMLDollyTarget::STEP_OUT(void)
{
  return PImpl::STEP_OUT;
}

/*!
  The constructor registers the singleton with the proper name and target type.
*/
SoScXMLDollyTarget::SoScXMLDollyTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Dolly");
}

SoScXMLDollyTarget::~SoScXMLDollyTarget(void)
{
}

/*!
  Dispatches incoming SCXML events to the corresponding functionality.
*/
SbBool
SoScXMLDollyTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  const SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();


  if (eventname == BEGIN()) {
    // BEGIN
    //   _sessionid {string}
    //   mouseposition {SbVec2f}
    //   [absminfocaldistance] {float}
    //   [absmaxfocaldistance] {float}
    //   [setfocaldistance] {float}
    //   [motiontype] {string:exponential,linear}
    DollyData * data = static_cast<DollyData *>(this->getSessionData(sessionid, NewDollyData));
    assert(data);

    SoScXMLStateMachine * statemachine = this->getSoStateMachine(event, sessionid);
    if unlikely (!statemachine) { return FALSE; }

    SbVec2f mousepos;
    if (!inherited::getEventSbVec2f(event, "mouseposition", mousepos)) { return FALSE; }
    data->lastpos = mousepos;

    double absminfocaldistance, absmaxfocaldistance;
    if (inherited::getEventDouble(event, "absminfocaldistance", absminfocaldistance, FALSE) &&
        inherited::getEventDouble(event, "absmaxfocaldistance", absmaxfocaldistance, FALSE)) {
      data->rangeislimited = TRUE;
      data->absminfocaldistance = float(absminfocaldistance);
      data->absmaxfocaldistance = float(absmaxfocaldistance);
    }
    else {
      data->rangeislimited = FALSE;
    }
    double focaldistance;
    if (inherited::getEventDouble(event, "setfocaldistance", focaldistance, FALSE)) {
      // immediate setting of focal distance
      SoCamera * camera = statemachine->getActiveCamera();
      camera->focalDistance.setValue(float(focaldistance));
    }

    const char * motiontype = event->getAssociation("motiontype");
    if (motiontype) {
      SbString motiontypestr = motiontype;
      if (motiontype[0] == '\'') { // unwrap
        std::unique_ptr<char[]> buf(new char [strlen(motiontypestr.getString()) + 1]);
        int res = sscanf(motiontype, "'%[^']'", buf.get());
        if (res == 1) {
          motiontypestr = buf.get();
        }
      }
      if (motiontypestr == "linear") {
        data->motionislinear = TRUE;
      }
      else if (motiontypestr == "exponential") {
        data->motionislinear = FALSE;
      }
      else {
        SoDebugError::post("SoScXMLDollyTarget::processOneEvent",
                           "while processing %s: event parameter 'motiontype' has invalid value '%s'.",
                           event->getEventName().getString(),
                           motiontypestr.getString());
        data->motionislinear = FALSE;
      }
    }
    else {
      data->motionislinear = FALSE;
    }

    return TRUE;
  }


  else if (eventname == UPDATE()) {
    DollyData * data = static_cast<DollyData *>(this->getSessionData(sessionid, NewDollyData));
    assert(data);

    SoScXMLStateMachine * statemachine = this->getSoStateMachine(event, sessionid);
    if unlikely (!statemachine) { return FALSE; }

    SoCamera * camera = statemachine->getActiveCamera();
    if unlikely (!camera) {
      SoDebugError::post("SoScXMLDollyTarget::processOneEvent",
                         "while processing %s: no current camera",
                         eventname.getString());
      return FALSE;
    }

    SbVec2f prevpos = data->lastpos;
    SbVec2f thispos;
    if (!inherited::getEventSbVec2f(event, "mouseposition", thispos)) {
      return FALSE;
    }
    data->lastpos = thispos;

    // The value 20.0 is just a value found by trial.
    SoScXMLDollyTarget::dolly(camera, (thispos[1] - prevpos[1]) * 20.0f);

    return TRUE;
  }


  else if (eventname == END()) {
    this->freeSessionData(sessionid);
    return TRUE;
  }


  else if (eventname == JUMP()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    double wanteddistance;
    if (!inherited::getEventDouble(event, "focaldistance", wanteddistance)) {
      return FALSE;
    }

    SoScXMLDollyTarget::jump(camera, static_cast<float>(wanteddistance));
    return TRUE;
  }


  else if (eventname == STEP_IN() || eventname == STEP_OUT()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    double count = 1.0;
    inherited::getEventDouble(event, "count", count, FALSE);
    double stepsize = 1.0;
    inherited::getEventDouble(event, "stepsize", stepsize, FALSE);

    double absminfocaldistance = 0.0;
    inherited::getEventDouble(event, "absminfocaldistance", absminfocaldistance, FALSE);
    double absmaxfocaldistance = 0.0;
    inherited::getEventDouble(event, "absmaxfocaldistance", absmaxfocaldistance, FALSE);

    SbString motiontype("exponential");
    inherited::getEventString(event, "motiontype", motiontype, FALSE);
    SbBool exp = TRUE;
    if (motiontype == "exponential") {
      exp = TRUE;
    }
    else if (motiontype == "linear") {
      exp = FALSE;
    }
    else {
      SoDebugError::post("SoScXMLDollyTarget::processOneEvent",
                         "while processing %s: 'motiontype' must be exponential or linear.",
                         eventname.getString());
      return FALSE;
    }

    float diff = static_cast<float>(count * stepsize);
    if (eventname == STEP_IN()) { diff = -diff; }
    SoScXMLDollyTarget::step(camera, exp, float(diff), float(absminfocaldistance), float(absmaxfocaldistance));
    return TRUE;
  }


  else {
    SoDebugError::post("SoScXMLDollyTarget::processOneEvent",
                       "received unknown event '%s'",
                       eventname.getString());
    return FALSE;
  }

  return TRUE;
}

// *************************************************************************
// Dependent on the camera type this will either shrink or expand the
// height of the viewport (orthogonal camera) or move the camera
// closer or further away from the focal point in the scene.

void
SoScXMLDollyTarget::dolly(SoCamera * camera, float diffvalue)
{
  assert(camera);

  // This will be in the range of <0, ->>.
  float multiplicator = float(exp(diffvalue));

  const float oldfocaldist = camera->focalDistance.getValue();
  const float newfocaldist = oldfocaldist * multiplicator;

  SbVec3f direction;
  camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);

  const SbVec3f oldpos = camera->position.getValue();
  const SbVec3f newpos = oldpos + (newfocaldist - oldfocaldist) * -direction;

  // This catches a rather common user interface "buglet": if the
  // user zooms the camera out to a distance from origo larger than
  // what we still can safely do floating point calculations on
  // (i.e. without getting NaN or Inf values), the faulty floating
  // point values will propagate until we start to get debug error
  // messages and eventually an assert failure from core Coin code.
  //
  // With the below bounds check, this problem is avoided.
  //
  // (But note that we depend on the input argument ''diffvalue'' to
  // be small enough that zooming happens gradually. Ideally, we
  // should also check distorigo with isinf() and isnan() (or
  // inversely; isinfite()), but those only became standardized with
  // C99.)
  const float distorigo = newpos.length();
  // sqrt(FLT_MAX) == ~ 1e+19, which should be both safe for further
  // calculations and ok for the end-user and app-programmer.
  if (distorigo > float(sqrt(FLT_MAX))) {
    if (COIN_DEBUG && 0) {
      SoDebugError::postWarning("SoScXMLDollyTarget::dolly",
                                "zoomed too far (distance to origo==%f (%e))",
                                distorigo, distorigo);
    }
  }
  else {
    camera->position = newpos;
    camera->focalDistance = newfocaldist;
  }
}

// *************************************************************************
/*!
  Jumps the camera to \a focaldistance distance from the current focal point
  of the camera.  This function only changes the SoCamera::position field.

  This function is more or less useless to use on an SoOrthographicCamera.
*/
void
SoScXMLDollyTarget::jump(SoCamera * camera, float focaldistance)
{
  assert(camera);

  float currentdistance = camera->focalDistance.getValue();
  SbVec3f direction;
  camera->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), direction);
  direction.normalize();

  SbVec3f focalpoint = camera->position.getValue() + direction * currentdistance;

  SbVec3f offset = direction * focaldistance;
  SbVec3f newpos = focalpoint - offset;

  // The below test catches a rather common user interface "buglet": if the
  // user zooms the camera out to a distance from origo larger than
  // what we still can safely do floating point calculations on
  // (i.e. without getting NaN or Inf values), the faulty floating
  // point values will propagate until we start to get debug error
  // messages and eventually an assert failure from core Coin code.
  //
  // With the below bounds check, this problem is avoided.
  //
  // (But note that we depend on the input argument ''diffvalue'' to
  // be small enough that zooming happens gradually. Ideally, we
  // should also check distorigo with isinf() and isnan() (or
  // inversely; isinfite()), but those only became standardized with
  // C99.)
  const float distorigo = newpos.length();
  // sqrt(FLT_MAX) == ~ 1e+19, which should be both safe for further
  // calculations and ok for the end-user and app-programmer.
  if (distorigo > float(sqrt(FLT_MAX))) {
    if (COIN_DEBUG && 0) {
      SoDebugError::postWarning("SoScXMLDollyTarget::dolly",
                                "zoomed too far (distance to origo==%f (%e))",
                                distorigo, distorigo);
    }
  }
  else {
    camera->position.setValue(newpos);
    camera->focalDistance.setValue(focaldistance);
  }

  // FIXME: UTMCamera should have utmposition updated and position set to origo
}

// *************************************************************************
/*!
  Steps the camera relative to its orientation and focal point by \a diff.

  If \a exponential is FALSE, then the \a diff value is treated as an absolute
  distance value.

  If \a mindistance and/or \a maxdistance is anything but 0.0, they are checked
  against the focal distance, and the focal distance will be clamped inside the
  range. Both are not needed, if only one is specified, only that part of the
  range will be used to limit the dollying.

  If exponential is TRUE...FIXME
*/
void
SoScXMLDollyTarget::step(SoCamera * camera, SbBool exponential, float diff, float mindistance, float maxdistance)
{
  assert(camera);
  if (!exponential) {
    float focaldistance = camera->focalDistance.getValue();
    focaldistance += diff;
    if (mindistance != 0.0f && focaldistance < mindistance) {
      focaldistance = mindistance;
    }
    if (maxdistance != 0.0f && focaldistance > maxdistance) {
      focaldistance = maxdistance;
    }
    SoScXMLDollyTarget::jump(camera, focaldistance);
  }
  else {
    // This will be in the range of <0, ->>.
    float multiplicator = float(exp(diff * 0.1f));

    const float oldfocaldist = camera->focalDistance.getValue();

    float focaldistance = oldfocaldist * multiplicator;
    if (mindistance != 0.0f && focaldistance < mindistance) {
      focaldistance = mindistance;
    }
    if (maxdistance != 0.0f && focaldistance > maxdistance) {
      focaldistance = maxdistance;
    }

    SoScXMLDollyTarget::jump(camera, focaldistance);
  }
}

#undef PRIVATE
