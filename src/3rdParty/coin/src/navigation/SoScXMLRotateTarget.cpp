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

#include <Inventor/navigation/SoScXMLRotateTarget.h>

/*!
  \class SoScXMLRotateTarget SoScXMLRotateTarget.h Inventor/scxml/SoScXMLRotateTarget.h
  \brief Navigation system event target for rotating operations.

  \ingroup coin_navigation
*/

#
#include <cassert>
#include <cmath>
#include <cfloat>

#include <memory>

#include <Inventor/SbViewVolume.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbLine.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoRefPtr.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/fields/SoSFVec3d.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/projectors/SbSphereSheetProjector.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/navigation/SoScXMLNavigation.h>
#include <Inventor/navigation/SoScXMLFlightControlTarget.h>
#include <Inventor/navigation/SoScXMLDollyTarget.h>

#include "scxml/SbStringConvert.h"
#include "SbBasicP.h"
#include "coindefs.h"
#include "base/coinString.h"

namespace {

class RotateData : public SoScXMLNavigationTarget::Data {
// sendspinstart
// should be persistent over rotations, but individually settable per session
// or better; per event origin point
public:
  RotateData(void) {
    this->projector.reset(new SbSphereSheetProjector);
    SbViewVolume volume;
    volume.ortho(-1, 1, -1, 1, -1, 1);
    this->projector->setViewVolume(volume);
    this->logsize = 0;
  }

  SbVec2f downposn;
  SoRefPtr<SoCamera> cameraclone;
  std::unique_ptr<SbSphereSheetProjector> projector;

  struct log {
    SbVec2f posn;
    SbTime time;
  } mouselog[3];
  int logsize;
};

static SoScXMLNavigationTarget::Data * NewRotateData(void) { return new RotateData; }

} // namespace

class SoScXMLRotateTarget::PImpl {
public:
  // received
  static SbName BEGIN;
  static SbName UPDATE;
  static SbName END;
  static SbName SET_FOCAL_POINT;
  // sent
  static SbName TRIGGER_SPIN;
};

SbName SoScXMLRotateTarget::PImpl::BEGIN;
SbName SoScXMLRotateTarget::PImpl::UPDATE;
SbName SoScXMLRotateTarget::PImpl::END;
SbName SoScXMLRotateTarget::PImpl::SET_FOCAL_POINT;
SbName SoScXMLRotateTarget::PImpl::TRIGGER_SPIN;

// *************************************************************************

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLRotateTarget);

void
SoScXMLRotateTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLRotateTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX COIN_NAVIGATION_ROTATE_TARGET_EVENT_PREFIX
  PImpl::BEGIN            = SbName(EVENT_PREFIX ".BEGIN");
  PImpl::UPDATE           = SbName(EVENT_PREFIX ".UPDATE");
  PImpl::END              = SbName(EVENT_PREFIX ".END");
  PImpl::SET_FOCAL_POINT  = SbName(EVENT_PREFIX ".SET_FOCAL_POINT");
  PImpl::TRIGGER_SPIN     = SbName(EVENT_PREFIX ".TRIGGER_SPIN");
#undef EVENT_PREFIX
}

void
SoScXMLRotateTarget::cleanClass(void)
{
  SoScXMLRotateTarget::classTypeId = SoType::badType();
}

SoScXMLRotateTarget * SoScXMLRotateTarget::theSingleton = NULL;

SoScXMLRotateTarget *
SoScXMLRotateTarget::constructSingleton(void)
{
  assert(SoScXMLRotateTarget::theSingleton == NULL);
  SoScXMLRotateTarget::theSingleton =
    static_cast<SoScXMLRotateTarget *>(SoScXMLRotateTarget::classTypeId.createInstance());
  return SoScXMLRotateTarget::theSingleton;
}

void
SoScXMLRotateTarget::destructSingleton(void)
{
  assert(SoScXMLRotateTarget::theSingleton != NULL);
  delete SoScXMLRotateTarget::theSingleton;
  SoScXMLRotateTarget::theSingleton = NULL;
}

SoScXMLRotateTarget *
SoScXMLRotateTarget::singleton(void)
{
  assert(SoScXMLRotateTarget::theSingleton != NULL);
  return SoScXMLRotateTarget::theSingleton;
}

const SbName &
SoScXMLRotateTarget::BEGIN(void)
{
  return PImpl::BEGIN;
}

const SbName &
SoScXMLRotateTarget::UPDATE(void)
{
  return PImpl::UPDATE;
}

const SbName &
SoScXMLRotateTarget::END(void)
{
  return PImpl::END;
}

const SbName &
SoScXMLRotateTarget::SET_FOCAL_POINT(void)
{
  return PImpl::SET_FOCAL_POINT;
}

const SbName &
SoScXMLRotateTarget::TRIGGER_SPIN(void)
{
  return PImpl::TRIGGER_SPIN;
}

SoScXMLRotateTarget::SoScXMLRotateTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Rotate");
}

SoScXMLRotateTarget::~SoScXMLRotateTarget(void)
{
}


SbBool
SoScXMLRotateTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();

  if (eventname == BEGIN()) {
    RotateData * data =
      static_cast<RotateData *>(this->getSessionData(sessionid, NewRotateData));
    assert(data);

    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    if (!inherited::getEventSbVec2f(event, "mouseposition", data->downposn)) {
      return FALSE;
    }

    data->mouselog[0].posn = data->downposn;
    data->mouselog[0].time = SbTime::getTimeOfDay();
    data->logsize = 1;

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    // store current camera position
    data->cameraclone.reset(static_cast<SoCamera *>(camera->copy()));

    return TRUE;
  }

  else if (eventname == UPDATE()) {
    RotateData * data = static_cast<RotateData *>(this->getSessionData(sessionid, NewRotateData));
    assert(data);

    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = statemachine->getActiveCamera();
    if unlikely (!camera) {
      SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                         "while processing %s: no current camera",
                         eventname.getString());
      return FALSE;
    }

    assert(data->cameraclone.get());
    if unlikely (camera->getTypeId() != data->cameraclone->getTypeId()) {
      SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                         "while processing %s: camera type was changed",
                         eventname.getString());
      return FALSE;
    }

    // get mouse position
    SbVec2f currentpos;
    if (!inherited::getEventSbVec2f(event, "mouseposition", currentpos)) {
      return FALSE;
    }

    // update mouse log
    data->mouselog[2].time = data->mouselog[1].time;
    data->mouselog[2].posn = data->mouselog[1].posn;
    data->mouselog[1].time = data->mouselog[0].time;
    data->mouselog[1].posn = data->mouselog[0].posn;
    data->mouselog[0].posn = currentpos;
    data->mouselog[0].time = SbTime::getTimeOfDay();
    data->logsize += 1;

    // find rotation
    data->projector->project(data->downposn);
    SbRotation rot;
    data->projector->projectAndGetRotation(currentpos, rot);
    rot.invert();

    // restore camera to original position and do full reorientation
    camera->copyFieldValues(data->cameraclone.get());
    reorientCamera(camera, rot);

    return TRUE;
  }

  else if (eventname == END()) {
    SbBool triggerspincheck = FALSE; // default if not set
    inherited::getEventSbBool(event, "triggerspin", triggerspincheck, FALSE);

    if (triggerspincheck) {
      RotateData * data = static_cast<RotateData *>(this->getSessionData(sessionid, NewRotateData));
      assert(data);

      SbBool triggerspin = FALSE;
      SbRotation spinrotation;

      if (data->logsize > 2) {
        SbTime stoptime = (SbTime::getTimeOfDay() - data->mouselog[0].time);
        if (stoptime.getValue() < 0.100) {
          SbVec3f from = data->projector->project(data->mouselog[2].posn);
          SbVec3f to = data->projector->project(data->mouselog[0].posn);
          spinrotation = data->projector->getRotation(from, to);

          SbTime delta = data->mouselog[0].time - data->mouselog[2].time;
          double deltatime = delta.getValue();
          spinrotation.invert();
          spinrotation.scaleAngle(float(0.200 / deltatime));

          SbVec3f axis;
          float radians;
          spinrotation.getValue(axis, radians);
          if ((radians > 0.01f) && (deltatime < 0.300)) {
            triggerspin = TRUE;
          }
        }
      }

      if (triggerspin) {
        SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
        if (!statemachine) { return FALSE; }
        SbString rotationstr;
        rotationstr = SbStringConvert::toString(spinrotation);

        SbString updatetimestr;
        double fromtime = SbTime::getTimeOfDay().getValue();
        updatetimestr = SbStringConvert::toString(fromtime);

        ScXMLEvent triggerspinevent;
        triggerspinevent.setEventName(TRIGGER_SPIN());
        triggerspinevent.setAssociation("rotation", rotationstr.getString());
        triggerspinevent.setAssociation("from", updatetimestr.getString());
        statemachine->queueEvent(&triggerspinevent);
      }
    }

    this->freeSessionData(sessionid);
    return TRUE;
  }

  else if (eventname == SET_FOCAL_POINT()) {
    // _sessionid
    // worldspace {SbVec3f}
    // [upvector] {SbVec3f}
    // [focaldistance] {double}

    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    SbVec3f worldspace(0.0f, 0.0f, 0.0f);
    if (event->getAssociation("worldspace")) {
      SbString valuestr = event->getAssociation("worldspace");
      if (SbStringConvert::typeOf(valuestr) == SbStringConvert::SBVEC3F) {
        if (!inherited::getEventSbVec3f(event, "worldspace", worldspace)) {
          return FALSE;
        }
      } else {
        return FALSE;
      }
    }

    SbVec3f upvector(0.0f, 0.0f, 0.0f);
    SbBool useupvector = inherited::getEventSbVec3f(event, "upvector", upvector, FALSE);

    double focaldistance = 0.0f;
    SbBool usefocaldistance = inherited::getEventDouble(event, "focaldistance", focaldistance, FALSE);

    if (!useupvector) {
      // camera->pointAt() will turn the model away from its current up vector, so we
      // try to preserve the existing up vector here instead of calling the up-vector-less
      // version.
      camera->orientation.getValue().multVec(SbVec3f(0.0f, 1.0f, 0.0f), upvector);
      useupvector = TRUE;
    }

    SoScXMLRotateTarget::setFocalPoint(camera, worldspace, upvector);

    if (usefocaldistance) {
      SoScXMLDollyTarget::jump(camera, float(focaldistance));
    }

    return TRUE;
  }

  else {
    SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                       "processing %s: unknown event",
                       eventname.getString());
    return FALSE;
  }

  return TRUE;
}

// Rotate camera around its focal point.
void
SoScXMLRotateTarget::reorientCamera(SoCamera * camera, const SbRotation & rot)
{
  if (camera == NULL) return;

  // Find global coordinates of focal point.
  SbVec3f direction;
  camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
  SbVec3f focalpoint = camera->position.getValue() +
    camera->focalDistance.getValue() * direction;

  // Set new orientation value by accumulating the new rotation.
  camera->orientation = rot * camera->orientation.getValue();

  // Reposition camera so we are still pointing at the same old focal point.
  camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
  camera->position = focalpoint - camera->focalDistance.getValue() * direction;

  // some custom code to support UTMCamera cameras
  static const SoType utmcamtype(SoType::fromName("UTMCamera"));
  if (utmcamtype != SoType::badType() && camera->isOfType(utmcamtype)) {
    SbVec3d offset;
    offset.setValue(camera->position.getValue());
    SoSFVec3d * utmpositionfield = coin_assert_cast<SoSFVec3d *>(camera->getField("utmposition"));
    utmpositionfield->setValue(utmpositionfield->getValue()+offset);
    camera->position.setValue(0.0f, 0.0f, 0.0f);
  }
}

void
SoScXMLRotateTarget::setFocalPoint(SoCamera * camera, const SbVec3f & worldspace)
{
  camera->pointAt(worldspace);
}

void
SoScXMLRotateTarget::setFocalPoint(SoCamera * camera, const SbVec3f & worldspace, const SbVec3f & upvector)
{
  camera->pointAt(worldspace, upvector);
  SoScXMLFlightControlTarget::resetRoll(camera, upvector);
}

#undef PRIVATE
