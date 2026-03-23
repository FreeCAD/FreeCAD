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

#include <Inventor/navigation/SoScXMLZoomTarget.h>

/*!
  \class SoScXMLZoomTarget SoScXMLZoomTarget.h Inventor/scxml/SoScXMLZoomTarget.h
  \brief SCXML event target service for zoom behaviour.

  Events:

    x-coin-navigation.Zoom.*

    {
    BEGIN
      _sessionid {string}
      mouseposition {SbVec2f}

    UPDATE
      _sessionid {string}
      mouseposition {SbVec2f}

    END
      _sessionid {string}
      mouseposition {SbVec2f}
    }

    ZOOM
      _sessionid
      factor {float}

    ZOOM_IN
      _sessionid {string}
      [factor] {float=1.2}
      [count] {float=1}

    ZOOM_OUT
      _sessionid {string}
      [factor] {float=1.2}
      [count] {float=1}

    RESET
      _sessionid {string}

  \ingroup coin_navigation
  \since Coin 3.1
*/

#include <cassert>
#include <string>
#include <cfloat>
#include <cmath>

#include <Inventor/misc/SoRefPtr.h>

#include <Inventor/SbVec2f.h>
#include <Inventor/SbRotation.h>
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
#include "SbBasicP.h"

namespace {

class ZoomData : public SoScXMLNavigationTarget::Data {
public:
  SbVec2f lastposn;

};

static SoScXMLNavigationTarget::Data * NewZoomData(void) { return new ZoomData; }

} // namespace

class SoScXMLZoomTarget::PImpl {
public:
  static SbName BEGIN;
  static SbName UPDATE;
  static SbName END;
  static SbName ZOOM;
  static SbName ZOOM_IN;
  static SbName ZOOM_OUT;
  static SbName RESET;
};

SbName SoScXMLZoomTarget::PImpl::BEGIN;
SbName SoScXMLZoomTarget::PImpl::UPDATE;
SbName SoScXMLZoomTarget::PImpl::END;
SbName SoScXMLZoomTarget::PImpl::ZOOM;
SbName SoScXMLZoomTarget::PImpl::ZOOM_IN;
SbName SoScXMLZoomTarget::PImpl::ZOOM_OUT;
SbName SoScXMLZoomTarget::PImpl::RESET;

// *************************************************************************

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLZoomTarget);

void
SoScXMLZoomTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLZoomTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");
#define EVENT_PREFIX COIN_NAVIGATION_ZOOM_TARGET_EVENT_PREFIX
  SoScXMLZoomTarget::PImpl::BEGIN     = SbName(EVENT_PREFIX ".BEGIN");
  SoScXMLZoomTarget::PImpl::UPDATE    = SbName(EVENT_PREFIX ".UPDATE");
  SoScXMLZoomTarget::PImpl::END       = SbName(EVENT_PREFIX ".END");
  SoScXMLZoomTarget::PImpl::ZOOM      = SbName(EVENT_PREFIX ".ZOOM");
  SoScXMLZoomTarget::PImpl::ZOOM_IN   = SbName(EVENT_PREFIX ".ZOOM_IN");
  SoScXMLZoomTarget::PImpl::ZOOM_OUT  = SbName(EVENT_PREFIX ".ZOOM_OUT");
  SoScXMLZoomTarget::PImpl::RESET     = SbName(EVENT_PREFIX ".RESET");
#undef EVENT_PREFIX
}

void
SoScXMLZoomTarget::cleanClass(void)
{
  SoScXMLZoomTarget::classTypeId = SoType::badType();
}

SoScXMLZoomTarget * SoScXMLZoomTarget::theSingleton = NULL;

SoScXMLZoomTarget *
SoScXMLZoomTarget::constructSingleton(void)
{
  assert(SoScXMLZoomTarget::theSingleton == NULL);
  SoScXMLZoomTarget::theSingleton =
    static_cast<SoScXMLZoomTarget *>(SoScXMLZoomTarget::classTypeId.createInstance());
  return SoScXMLZoomTarget::theSingleton;
}

void
SoScXMLZoomTarget::destructSingleton(void)
{
  assert(SoScXMLZoomTarget::theSingleton != NULL);
  delete SoScXMLZoomTarget::theSingleton;
  SoScXMLZoomTarget::theSingleton = NULL;
}

SoScXMLZoomTarget *
SoScXMLZoomTarget::singleton(void)
{
  assert(SoScXMLZoomTarget::theSingleton != NULL);
  return SoScXMLZoomTarget::theSingleton;
}

const SbName &
SoScXMLZoomTarget::BEGIN(void)
{
  return SoScXMLZoomTarget::PImpl::BEGIN;
}

const SbName &
SoScXMLZoomTarget::UPDATE(void)
{
  return SoScXMLZoomTarget::PImpl::UPDATE;
}

const SbName &
SoScXMLZoomTarget::END(void)
{
  return SoScXMLZoomTarget::PImpl::END;
}

const SbName &
SoScXMLZoomTarget::ZOOM(void)
{
  return SoScXMLZoomTarget::PImpl::ZOOM;
}

const SbName &
SoScXMLZoomTarget::ZOOM_IN(void)
{
  return SoScXMLZoomTarget::PImpl::ZOOM_IN;
}

const SbName &
SoScXMLZoomTarget::ZOOM_OUT(void)
{
  return SoScXMLZoomTarget::PImpl::ZOOM_OUT;
}

const SbName &
SoScXMLZoomTarget::RESET(void)
{
  return SoScXMLZoomTarget::PImpl::RESET;
}

SoScXMLZoomTarget::SoScXMLZoomTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Zoom");
}

SoScXMLZoomTarget::~SoScXMLZoomTarget(void)
{
}


SbBool
SoScXMLZoomTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  const SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();

  if (eventname == BEGIN()) {
    // _sessionid
    // mouseposition
    ZoomData * data = static_cast<ZoomData *>(this->getSessionData(sessionid, NewZoomData));
    assert(data);

    SoScXMLStateMachine * statemachine = this->getSoStateMachine(event, sessionid);
    if unlikely (!statemachine) { return FALSE; }

    if (!inherited::getEventSbVec2f(event, "mouseposition", data->lastposn)) {
      return FALSE;
    }

    return TRUE;
  }

  else if (eventname == UPDATE()) {
    // _sessionid
    // mouseposition
    ZoomData * data = static_cast<ZoomData *>(this->getSessionData(sessionid, NewZoomData));
    assert(data);

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    SbVec2f prevposn = data->lastposn;
    if (!inherited::getEventSbVec2f(event, "mouseposition", data->lastposn)) {
      return FALSE;
    }
    SbVec2f thisposn = data->lastposn;

    // The value 20.0 is just a value found by trial. exp() brings this in the range of <0, ->>.
    SoScXMLZoomTarget::zoom(camera, float(exp((thisposn[1] - prevposn[1]) * 20.0f)));

    return TRUE;
  }

  else if (eventname == END()) {
    // _sessionid
    this->freeSessionData(sessionid);
    return TRUE;
  }


  else if (eventname == ZOOM()) {
    // _sessionid
    // factor
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    double factor = 1.0;
    if (!inherited::getEventDouble(event, "factor", factor)) {
      return FALSE;
    }

    if (abs(factor) <= FLT_EPSILON) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a 0 factor.",
                         eventname.getString());
      return FALSE;
    }
    if (factor < 0.0) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a negative factor.",
                         eventname.getString());
      return FALSE;
    }

    SoScXMLZoomTarget::zoom(camera, static_cast<float>(factor));
    return TRUE;
  }

  else if (eventname == ZOOM_IN() || eventname == ZOOM_OUT()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    double factor = 1.2;
    inherited::getEventDouble(event, "factor", factor, FALSE);

    if (abs(factor) <= FLT_EPSILON) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a 0 factor.",
                         eventname.getString());
      return FALSE;
    }
    if (factor < 0.0) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a negative factor.",
                         eventname.getString());
      return FALSE;
    }

    double count = 1.0;
    inherited::getEventDouble(event, "count", count, FALSE);
    if (abs(count) <= FLT_EPSILON) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a 0 zoom count.",
                         eventname.getString());
    }
    if (count < 0.0) {
      SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                         "while processing %s: can't zoom with a negative zoom count.",
                         eventname.getString());
      return FALSE;
    }

    double compounded = (eventname == ZOOM_IN()) ?  pow(1.0/factor, count) : pow(factor, count);

    SoScXMLZoomTarget::zoom(camera, static_cast<float>(compounded));
    return TRUE;
  }


  else if (eventname == RESET()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    SoScXMLZoomTarget::reset(camera);
  }


  else {
    SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
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
SoScXMLZoomTarget::zoom(SoCamera * camera, float multiplicator)
{
  assert(camera);

  if (camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    // Since there's no perspective, "zooming" in the original sense
    // of the word won't have any visible effect. So we just increase
    // or decrease the field-of-view values of the camera instead, to
    // "shrink" the projection size of the model / scene.
    SoOrthographicCamera * oc = coin_assert_cast<SoOrthographicCamera *>(camera);
    oc->height = oc->height.getValue() * multiplicator;
  }

  else if (camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    SoPerspectiveCamera * pc = coin_assert_cast<SoPerspectiveCamera *>(camera);
    pc->heightAngle = pc->heightAngle.getValue() * multiplicator;
  }

  else if (camera->isOfType(SoFrustumCamera::getClassTypeId())) {
    // this might not make any sense - debug later (2009-02-15 larsa)
    SoFrustumCamera * fc = coin_assert_cast<SoFrustumCamera *>(camera);
    fc->left = fc->left.getValue() * multiplicator;
    fc->right = fc->right.getValue() * multiplicator;
    fc->top = fc->top.getValue() * multiplicator;
    fc->bottom = fc->bottom.getValue() * multiplicator;
  }

  else {
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::postWarning("SoScXMLZoomTarget::zoom",
                                "Unknown camera type, "
                                "will zoom by moving position, "
                                "which is not correct.");
      first = FALSE;
    }

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

      //SoDebugError::postWarning("SoScXMLZoomTarget::zoom",
      //                          "zoomed too far (distance to origo==%f (%e))",
      //                          distorigo, distorigo);
    }
    else {
      camera->position = newpos;
      camera->focalDistance = newfocaldist;
    }
  }
}


/*!
  This function resets the zooming attributes of the camera to the default
  values.
*/

void
SoScXMLZoomTarget::reset(SoCamera * camera)
{
  assert(camera);

  SoType cameratype = camera->getTypeId();
  assert(cameratype.canCreateInstance());
  SoRefPtr<SoCamera> defaultcamera(static_cast<SoCamera *>(cameratype.createInstance()));

  if (camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    static_cast<SoOrthographicCamera *>(camera)->height =
      static_cast<SoOrthographicCamera *>(defaultcamera.get())->height.getValue();
  }
  else if (camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    static_cast<SoPerspectiveCamera *>(camera)->heightAngle =
      static_cast<SoPerspectiveCamera *>(defaultcamera.get())->heightAngle.getValue();
  }
  else if (camera->isOfType(SoFrustumCamera::getClassTypeId())) {
    SoFrustumCamera * fcamera = static_cast<SoFrustumCamera *>(camera);
    SoFrustumCamera * fdefaultcamera = static_cast<SoFrustumCamera *>(defaultcamera.get());
    fcamera->left = fdefaultcamera->left.getValue();
    fcamera->right = fdefaultcamera->right.getValue();
    fcamera->top = fdefaultcamera->top.getValue();
    fcamera->bottom = fdefaultcamera->bottom.getValue();
  }
}

#undef PRIVATE
