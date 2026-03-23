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

#include <Inventor/navigation/SoScXMLMiscTarget.h>

/*!
  \class SoScXMLMiscTarget SoScXMLMiscTarget.h Inventor/navigation/SoScXMLMiscTarget.h
  \brief Miscellaneous navigation utility functions.

  \ingroup coin_navigation
*/

#include <cassert>

#include <Inventor/SbName.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/navigation/SoScXMLNavigation.h>
#include "coindefs.h"

// *************************************************************************

class SoScXMLMiscTarget::PImpl {
public:
  static SbName VIEW_ALL;
  static SbName REDRAW;
  static SbName POINT_AT;
  static SbName SET_FOCAL_DISTANCE;
  static SbName SET_CAMERA_POSITION;

};

SbName SoScXMLMiscTarget::PImpl::VIEW_ALL;
SbName SoScXMLMiscTarget::PImpl::REDRAW;
SbName SoScXMLMiscTarget::PImpl::POINT_AT;
SbName SoScXMLMiscTarget::PImpl::SET_FOCAL_DISTANCE;
SbName SoScXMLMiscTarget::PImpl::SET_CAMERA_POSITION;

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLMiscTarget);

void
SoScXMLMiscTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLMiscTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX SOSCXML_NAVIGATION_MISC_TARGET_EVENT_PREFIX
  SoScXMLMiscTarget::PImpl::VIEW_ALL             = SbName(EVENT_PREFIX ".VIEW_ALL");
  SoScXMLMiscTarget::PImpl::REDRAW               = SbName(EVENT_PREFIX ".REDRAW");
  SoScXMLMiscTarget::PImpl::POINT_AT             = SbName(EVENT_PREFIX ".POINT_AT");
  SoScXMLMiscTarget::PImpl::SET_FOCAL_DISTANCE   = SbName(EVENT_PREFIX ".SET_FOCAL_DISTANCE");
  SoScXMLMiscTarget::PImpl::SET_CAMERA_POSITION  = SbName(EVENT_PREFIX ".SET_CAMERA_POSITION");
#undef EVENT_PREFIX
}

void
SoScXMLMiscTarget::cleanClass(void)
{
  SoScXMLMiscTarget::classTypeId = SoType::badType();
}

SoScXMLMiscTarget * SoScXMLMiscTarget::theSingleton = NULL;

SoScXMLMiscTarget *
SoScXMLMiscTarget::constructSingleton(void)
{
  assert(SoScXMLMiscTarget::theSingleton == NULL);
  SoScXMLMiscTarget::theSingleton =
    static_cast<SoScXMLMiscTarget *>(SoScXMLMiscTarget::classTypeId.createInstance());
  return SoScXMLMiscTarget::theSingleton;
}

void
SoScXMLMiscTarget::destructSingleton(void)
{
  assert(SoScXMLMiscTarget::theSingleton != NULL);
  delete SoScXMLMiscTarget::theSingleton;
  SoScXMLMiscTarget::theSingleton = NULL;
}

SoScXMLMiscTarget *
SoScXMLMiscTarget::singleton(void)
{
  assert(SoScXMLMiscTarget::theSingleton != NULL);
  return SoScXMLMiscTarget::theSingleton;
}

const SbName &
SoScXMLMiscTarget::VIEW_ALL(void)
{
  return SoScXMLMiscTarget::PImpl::VIEW_ALL;
}


const SbName &
SoScXMLMiscTarget::REDRAW(void)
{
  return SoScXMLMiscTarget::PImpl::REDRAW;
}


const SbName &
SoScXMLMiscTarget::POINT_AT(void)
{
  return SoScXMLMiscTarget::PImpl::POINT_AT;
}


const SbName &
SoScXMLMiscTarget::SET_FOCAL_DISTANCE(void)
{
  return SoScXMLMiscTarget::PImpl::SET_FOCAL_DISTANCE;
}


const SbName &
SoScXMLMiscTarget::SET_CAMERA_POSITION(void)
{
  return SoScXMLMiscTarget::PImpl::SET_CAMERA_POSITION;
}


SoScXMLMiscTarget::SoScXMLMiscTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Misc");
}

SoScXMLMiscTarget::~SoScXMLMiscTarget(void)
{
}


SbBool
SoScXMLMiscTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  const SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName eventname(event->getEventName());

#define EVENT_PREFIX SOSCXML_NAVIGATION_MISC_TARGET_EVENT_PREFIX
  static const SbName VIEW_ALL2(EVENT_PREFIX ".ViewAll");
  static const SbName REDRAW2(EVENT_PREFIX ".Redraw");
  static const SbName POINT_AT2(EVENT_PREFIX ".PointAt");
#undef EVENT_PREFIX

  if (eventname == VIEW_ALL() || eventname == VIEW_ALL2 /* compat */) {
    // _sessionid
    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    const SbViewportRegion & viewport = statemachine->getViewportRegion();
    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    SoNode * scenegraph = statemachine->getSceneGraphRoot();
    if (!scenegraph) {
      if (COIN_DEBUG) {
        SoDebugError::postInfo("SoScXMLMiscTarget::processOneEvent",
                               "while processing %s: no scene graph set",
                               eventname.getString());
      }
      return FALSE;
    }

    camera->viewAll(scenegraph, viewport);
  }

  else if (eventname == REDRAW() || eventname == REDRAW2 /* compat */) {
    // _sessionid
    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoNode * scenegraph = statemachine->getSceneGraphRoot();
    if (!scenegraph) {
      if (COIN_DEBUG) {
        SoDebugError::postInfo("SoScXMLMiscTarget::processOneEvent",
                               "while processing %s: no scene graph set",
                               eventname.getString());
      }
      return FALSE;
    }

    scenegraph->touch();
  }

  else if (eventname == POINT_AT() || eventname == POINT_AT2 /* compat */) {
    // _sessionid
    // focuspoint || _event.pickposition3 {SbVec3f}
    // [upvector] {SbVec3f}
    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    SbVec3f focuspoint;
    SbBool focusset = FALSE;
    focusset = inherited::getEventSbVec3f(event, "_event.pickposition3", focuspoint, FALSE);
    if (!inherited::getEventSbVec3f(event, "focuspoint", focuspoint, !focusset)) {
      return FALSE;
    }

    SbVec3f upvector;
    SbBool useupvector = inherited::getEventSbVec3f(event, "upvector", upvector, FALSE);

    if (useupvector) {
      camera->pointAt(focuspoint, upvector);
    } else {
      camera->pointAt(focuspoint);
    }
  }

  else if (eventname == SET_FOCAL_DISTANCE()) {
    // _sessionid
    // focaldistance {real}
    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    double focaldistance;
    if (!inherited::getEventDouble(event, "focaldistance", focaldistance)) {
      return FALSE;
    }

    camera->focalDistance.setValue(static_cast<float>(focaldistance));
  }

  else if (eventname == SET_CAMERA_POSITION()) {
    // _sessionid
    // position {SbVec3f}

    SoScXMLStateMachine * statemachine = inherited::getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = inherited::getActiveCamera(event, sessionid);
    if (!camera) { return FALSE; }

    SbVec3f position(0.0f, 0.0f, 0.0f);
    if (!inherited::getEventSbVec3f(event, "position", position)) {
      return FALSE;
    }

    camera->position.setValue(position);
  }

  else {
    SoDebugError::post("SoScXMLMiscTarget::processOneEvent",
                       "received unknown event '%s'",
                       eventname.getString());
    return FALSE;
  }

  return TRUE;
}

#undef PRIVATE
