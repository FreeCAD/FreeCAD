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

#include <Inventor/navigation/SoScXMLFlightControlTarget.h>

/*!
  \class SoScXMLFlightControlTarget SoScXMLFlightControlTarget.h Inventor/navigation/SoScXMLFlightControlTarget.h
  \brief SCXML navigation service for typical flight motions.

  EVENTS:

    PITCH
      _sessionid
      angle

    YAW
      _sessionid
      angle

    ROLL
      _sessionid
      angle

  \ingroup coin_navigation
*/

#include <cassert>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/navigation/SoScXMLNavigation.h>

class SoScXMLFlightControlTarget::PImpl {
public:
  static SbName PITCH;
  static SbName YAW;
  static SbName ROLL;
  static SbName RESET_ROLL;
};

SbName SoScXMLFlightControlTarget::PImpl::PITCH;
SbName SoScXMLFlightControlTarget::PImpl::YAW;
SbName SoScXMLFlightControlTarget::PImpl::ROLL;
SbName SoScXMLFlightControlTarget::PImpl::RESET_ROLL;

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLFlightControlTarget);

void
SoScXMLFlightControlTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLFlightControlTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX COIN_NAVIGATION_FLIGHT_CONTROL_TARGET_EVENT_PREFIX
  SoScXMLFlightControlTarget::PImpl::PITCH       = SbName(EVENT_PREFIX ".PITCH");
  SoScXMLFlightControlTarget::PImpl::YAW         = SbName(EVENT_PREFIX ".YAW");
  SoScXMLFlightControlTarget::PImpl::ROLL        = SbName(EVENT_PREFIX ".ROLL");
  SoScXMLFlightControlTarget::PImpl::RESET_ROLL  = SbName(EVENT_PREFIX ".RESET_ROLL");
#undef EVENT_PREFIX
}

void
SoScXMLFlightControlTarget::cleanClass(void)
{
  SoScXMLFlightControlTarget::classTypeId = SoType::badType();
}

SoScXMLFlightControlTarget * SoScXMLFlightControlTarget::theSingleton = NULL;

SoScXMLFlightControlTarget *
SoScXMLFlightControlTarget::constructSingleton(void)
{
  assert(SoScXMLFlightControlTarget::theSingleton == NULL);
  SoScXMLFlightControlTarget::theSingleton =
    static_cast<SoScXMLFlightControlTarget *>(SoScXMLFlightControlTarget::classTypeId.createInstance());
  return SoScXMLFlightControlTarget::theSingleton;
}

void
SoScXMLFlightControlTarget::destructSingleton(void)
{
  assert(SoScXMLFlightControlTarget::theSingleton != NULL);
  delete SoScXMLFlightControlTarget::theSingleton;
  SoScXMLFlightControlTarget::theSingleton = NULL;
}

SoScXMLFlightControlTarget *
SoScXMLFlightControlTarget::singleton(void)
{
  assert(SoScXMLFlightControlTarget::theSingleton != NULL);
  return SoScXMLFlightControlTarget::theSingleton;
}

const SbName &
SoScXMLFlightControlTarget::PITCH(void)
{
  return SoScXMLFlightControlTarget::PImpl::PITCH;
}

const SbName &
SoScXMLFlightControlTarget::YAW(void)
{
  return SoScXMLFlightControlTarget::PImpl::YAW;
}

const SbName &
SoScXMLFlightControlTarget::ROLL(void)
{
  return SoScXMLFlightControlTarget::PImpl::ROLL;
}

const SbName &
SoScXMLFlightControlTarget::RESET_ROLL(void)
{
  return SoScXMLFlightControlTarget::PImpl::RESET_ROLL;
}

SoScXMLFlightControlTarget::SoScXMLFlightControlTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("FlightControl");
}

SoScXMLFlightControlTarget::~SoScXMLFlightControlTarget(void)
{
}

SbBool
SoScXMLFlightControlTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  const SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();

  if (eventname == PITCH()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    double angle = 0.0f;
    if (!inherited::getEventDouble(event, "angle", angle)) {
      return FALSE;
    }

    SoScXMLFlightControlTarget::pitch(camera, float(angle));
  }

  else if (eventname == YAW()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    double angle = 0.0f;
    if (!inherited::getEventDouble(event, "angle", angle)) {
      return FALSE;
    }

    SoScXMLFlightControlTarget::yaw(camera, float(angle));
  }

  else if (eventname == ROLL()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    double angle = 0.0f;
    if (!inherited::getEventDouble(event, "angle", angle)) {
      return FALSE;
    }

    SoScXMLFlightControlTarget::roll(camera, float(angle));
  }

  else if (eventname == RESET_ROLL()) {
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    SbVec3f upvec;
    if (!inherited::getEventSbVec3f(event, "upvector", upvec)) {
      return FALSE;
    }

    SoScXMLFlightControlTarget::resetRoll(camera, upvec);
  }

  else {
    SoDebugError::post("SoScXMLFlightControlTarget::processOneEvent",
                       "received unknown event '%s'",
                       eventname.getString());
    return FALSE;
  }


  return TRUE;
}

void
SoScXMLFlightControlTarget::pitch(SoCamera * camera, float radians)
{
  assert(camera);
  SbMatrix camerarot;
  camerarot.setRotate(camera->orientation.getValue());
  SbMatrix pitchmat;
  pitchmat.setRotate(SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), radians));
  camerarot.multLeft(pitchmat);
  camera->orientation.setValue(SbRotation(camerarot));
}

void
SoScXMLFlightControlTarget::yaw(SoCamera * camera, float radians)
{
  assert(camera);
  SbMatrix camerarot;
  camerarot.setRotate(camera->orientation.getValue());
  SbMatrix yawmat;
  yawmat.setRotate(SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), radians));
  camerarot.multLeft(yawmat);
  camera->orientation.setValue(SbRotation(camerarot));
}

void
SoScXMLFlightControlTarget::roll(SoCamera * camera, float radians)
{
  assert(camera);
  SbMatrix camerarot;
  camerarot.setRotate(camera->orientation.getValue());
  SbMatrix rollmat;
  rollmat.setRotate(SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), radians));
  camerarot.multLeft(rollmat);
  camera->orientation.setValue(SbRotation(camerarot));
}

void
SoScXMLFlightControlTarget::resetRoll(SoCamera * camera, const SbVec3f & upvector)
{
  assert(camera);
  if (upvector == SbVec3f(0.0f, 0.0f, 0.0f)) return;

  SbMatrix camerarot;
  camerarot.setRotate(camera->orientation.getValue());

  SbVec3f Z;
  Z[0] = camerarot[2][0];
  Z[1] = camerarot[2][1];
  Z[2] = camerarot[2][2];

  if (fabs(Z.dot(upvector)) > 0.99f) {
    // just give up
    return;
  }
  SbVec3f newx = upvector.cross(Z);
  SbVec3f newy = Z.cross(newx);

  newx.normalize();
  newy.normalize();
  camerarot[0][0] = newx[0];
  camerarot[0][1] = newx[1];
  camerarot[0][2] = newx[2];
  camerarot[1][0] = newy[0];
  camerarot[1][1] = newy[1];
  camerarot[1][2] = newy[2];
  camera->orientation.setValue(SbRotation(camerarot));
}

#undef PRIVATE
