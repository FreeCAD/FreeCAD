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

#include <Inventor/navigation/SoScXMLSpinTarget.h>

/*!
  \class SoScXMLSpinTarget SoScXMLSpinTarget.h Inventor/scxml/SoScXMLSpinTarget.h
  \brief Navigation system event target for spinning operations.

  \ingroup coin_navigation
*/

#include <cassert>
#include <cmath>
#include <cfloat>

#include <Inventor/SbViewVolume.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbLine.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoRefPtr.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/fields/SoSFVec3d.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/navigation/SoScXMLNavigation.h>

#include "scxml/SbStringConvert.h"
#include "coindefs.h"

namespace {

class SpinData : public SoScXMLNavigationTarget::Data {
public:
  SpinData(void) : spinning(FALSE) { }

  SbBool spinning;

  SoRefPtr<SoCamera> camera;
  SbTime updatetime;
  SbRotation spinrotation;
};

static SoScXMLNavigationTarget::Data * NewSpinData(void) { return new SpinData; }

} // namespace

class SoScXMLSpinTarget::PImpl {
public:
  static SbName BEGIN;
  static SbName UPDATE;
  static SbName END;
};

SbName SoScXMLSpinTarget::PImpl::BEGIN;
SbName SoScXMLSpinTarget::PImpl::UPDATE;
SbName SoScXMLSpinTarget::PImpl::END;

// *************************************************************************

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLSpinTarget);

void
SoScXMLSpinTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLSpinTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX COIN_NAVIGATION_SPIN_EVENT_PREFIX
  SoScXMLSpinTarget::PImpl::BEGIN   = SbName(EVENT_PREFIX ".BEGIN");
  SoScXMLSpinTarget::PImpl::UPDATE  = SbName(EVENT_PREFIX ".UPDATE");
  SoScXMLSpinTarget::PImpl::END     = SbName(EVENT_PREFIX ".END");
#undef EVENT_PREFIX
}

void
SoScXMLSpinTarget::cleanClass(void)
{
  SoScXMLSpinTarget::classTypeId = SoType::badType();
}

SoScXMLSpinTarget * SoScXMLSpinTarget::theSingleton = NULL;

SoScXMLSpinTarget *
SoScXMLSpinTarget::constructSingleton(void)
{
  assert(SoScXMLSpinTarget::theSingleton == NULL);
  SoScXMLSpinTarget::theSingleton =
    static_cast<SoScXMLSpinTarget *>(SoScXMLSpinTarget::classTypeId.createInstance());
  return SoScXMLSpinTarget::theSingleton;
}

void
SoScXMLSpinTarget::destructSingleton(void)
{
  assert(SoScXMLSpinTarget::theSingleton != NULL);
  delete SoScXMLSpinTarget::theSingleton;
  SoScXMLSpinTarget::theSingleton = NULL;
}

SoScXMLSpinTarget *
SoScXMLSpinTarget::singleton(void)
{
  assert(SoScXMLSpinTarget::theSingleton != NULL);
  return SoScXMLSpinTarget::theSingleton;
}

const SbName &
SoScXMLSpinTarget::BEGIN(void)
{
  return PImpl::BEGIN;
}

const SbName &
SoScXMLSpinTarget::UPDATE(void)
{
  return PImpl::UPDATE;
}

const SbName &
SoScXMLSpinTarget::END(void)
{
  return PImpl::END;
}

SoScXMLSpinTarget::SoScXMLSpinTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Spin");
}

SoScXMLSpinTarget::~SoScXMLSpinTarget(void)
{
}

SbBool
SoScXMLSpinTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);


  SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();

  if (eventname == BEGIN()) {
    SpinData * data = static_cast<SpinData *>(this->getSessionData(sessionid, NewSpinData));
    assert(data);

    assert(!data->spinning);

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    data->spinning = TRUE;

    data->camera.reset(static_cast<SoCamera *>(camera->copy()));

    double dtime = 0.0;
    SbRotation spinrot;
    if (!inherited::getEventDouble(event, "from", dtime) ||
        !inherited::getEventSbRotation(event, "rotation", spinrot)) {
      return FALSE;
    }

    data->updatetime = SbTime(dtime);
    data->spinrotation = spinrot;

    return TRUE;
  }

  else if (eventname == UPDATE()) {
    SpinData * data = static_cast<SpinData *>(this->getSessionData(sessionid, NewSpinData));
    assert(data);

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if unlikely (!camera) { return FALSE; }

    assert(data->camera.get());
    if unlikely (camera->getTypeId() != data->camera->getTypeId()) {
      SoDebugError::post("SoScXMLSpinTarget::processOneEvent",
                         "while processing %s: camera type was changed",
                         eventname.getString());
      return FALSE;
    }

    assert(data->spinning);

    const SbTime now(SbTime::getTimeOfDay());
    double secs = now.getValue() - data->updatetime.getValue();
    data->updatetime = now;

    SbRotation deltarot = data->spinrotation;
    deltarot.scaleAngle(float(secs * 5.0));
    SoScXMLSpinTarget::reorientCamera(camera, deltarot);

    return TRUE;
  }

  else if (eventname == END()) {
    this->freeSessionData(sessionid);
    return TRUE;
  }

  else {
    SoDebugError::post("SoScXMLSpinUtils::processOneEvent",
                       "while processing %s: unknown event",
                       eventname.getString());
    return FALSE;
  }
  return TRUE;
}

// Rotate camera around its focal point.
void
SoScXMLSpinTarget::reorientCamera(SoCamera * camera, const SbRotation & rot)
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
    SoSFVec3d * utmpositionfield =
      (SoSFVec3d *) camera->getField("utmposition");
    assert(utmpositionfield);
    utmpositionfield->setValue(utmpositionfield->getValue()+offset);
    camera->position.setValue(0.0f, 0.0f, 0.0f);
  }
}

#undef EVENT_PREFIX
#undef PRIVATE
