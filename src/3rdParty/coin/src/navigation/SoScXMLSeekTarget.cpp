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

#include <Inventor/navigation/SoScXMLSeekTarget.h>

/*!
  \class SoScXMLSeekTarget SoScXMLSeekTarget.h Inventor/scxml/SoScXMLSeekTarget.h
  \brief Navigation system event target for seekmotion operations.

   emits events .MISS and .DONE to the sessionid state-machine

  \ingroup coin_navigation
*/



#include <cassert>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/navigation/SoScXMLNavigation.h>
#include "coindefs.h"

namespace {

class SeekData : public SoScXMLNavigationTarget::Data {
public:
  SeekData(void) {
    seeking = FALSE;
  }
  SbBool seeking;
  SbTime seekstart;
  float seektime;
  SbVec3f camerastartposition, cameraendposition;
  SbRotation camerastartorient, cameraendorient;
};

static SoScXMLNavigationTarget::Data * NewSeekData(void) { return new SeekData; }

} // namespace

// *************************************************************************

class SoScXMLSeekTarget::PImpl {
public:
  static void getCameraCoordinateSystem(SoScXMLSeekTarget * pub, SoCamera * cameraarg,
                                        SoNode * root, SbMatrix & matrix, SbMatrix & inverse);

  static SbName BEGIN;
  static SbName UPDATE;
  static SbName END;
  static SbName MISS;
  static SbName DONE;
};

SbName SoScXMLSeekTarget::PImpl::BEGIN;
SbName SoScXMLSeekTarget::PImpl::UPDATE;
SbName SoScXMLSeekTarget::PImpl::END;
SbName SoScXMLSeekTarget::PImpl::MISS;
SbName SoScXMLSeekTarget::PImpl::DONE;

void
SoScXMLSeekTarget::PImpl::getCameraCoordinateSystem(
  SoScXMLSeekTarget * pub,
  SoCamera * cameraarg,
  SoNode * root,
  SbMatrix & matrix,
  SbMatrix & inverse)
{
  assert(pub->searchaction);
  pub->searchaction->reset();
  pub->searchaction->setSearchingAll(TRUE);
  pub->searchaction->setInterest(SoSearchAction::FIRST);
  pub->searchaction->setNode(cameraarg);
  pub->searchaction->apply(root);

  matrix = inverse = SbMatrix::identity();
  if (pub->searchaction->getPath()) {
    assert(pub->getmatrixaction);
    pub->getmatrixaction->apply(pub->searchaction->getPath());
    matrix = pub->getmatrixaction->getMatrix();
    inverse = pub->getmatrixaction->getInverse();
  }
  pub->searchaction->reset();
}

#define PRIVATE

SCXML_OBJECT_SOURCE(SoScXMLSeekTarget);

void
SoScXMLSeekTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLSeekTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");

#define EVENT_PREFIX COIN_NAVIGATION_SEEK_EVENT_PREFIX
  SoScXMLSeekTarget::PImpl::BEGIN   = SbName(EVENT_PREFIX ".BEGIN");
  SoScXMLSeekTarget::PImpl::UPDATE  = SbName(EVENT_PREFIX ".UPDATE");
  SoScXMLSeekTarget::PImpl::END     = SbName(EVENT_PREFIX ".END");
  SoScXMLSeekTarget::PImpl::MISS    = SbName(EVENT_PREFIX ".MISS");
  SoScXMLSeekTarget::PImpl::DONE    = SbName(EVENT_PREFIX ".DONE");
#undef EVENT_PREFIX
}

void
SoScXMLSeekTarget::cleanClass(void)
{
  SoScXMLSeekTarget::classTypeId = SoType::badType();
}

SoScXMLSeekTarget * SoScXMLSeekTarget::theSingleton = NULL;

SoScXMLSeekTarget *
SoScXMLSeekTarget::constructSingleton(void)
{
  assert(SoScXMLSeekTarget::theSingleton == NULL);
  SoScXMLSeekTarget::theSingleton =
    static_cast<SoScXMLSeekTarget *>(SoScXMLSeekTarget::classTypeId.createInstance());
  return SoScXMLSeekTarget::theSingleton;
}

void
SoScXMLSeekTarget::destructSingleton(void)
{
  assert(SoScXMLSeekTarget::theSingleton != NULL);
  delete SoScXMLSeekTarget::theSingleton;
  SoScXMLSeekTarget::theSingleton = NULL;
}

SoScXMLSeekTarget *
SoScXMLSeekTarget::singleton(void)
{
  assert(SoScXMLSeekTarget::theSingleton != NULL);
  return SoScXMLSeekTarget::theSingleton;
}

const SbName &
SoScXMLSeekTarget::BEGIN(void)
{
  return SoScXMLSeekTarget::PImpl::BEGIN;
}

const SbName &
SoScXMLSeekTarget::UPDATE(void)
{
  return SoScXMLSeekTarget::PImpl::UPDATE;
}

const SbName &
SoScXMLSeekTarget::END(void)
{
  return SoScXMLSeekTarget::PImpl::END;
}

const SbName &
SoScXMLSeekTarget::MISS(void)
{
  return SoScXMLSeekTarget::PImpl::MISS;
}

const SbName &
SoScXMLSeekTarget::DONE(void)
{
  return SoScXMLSeekTarget::PImpl::DONE;
}

SoScXMLSeekTarget::SoScXMLSeekTarget(void)
: searchaction(NULL),
  getmatrixaction(NULL),
  raypickaction(NULL)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Seek");
  this->raypickaction = new SoRayPickAction(SbViewportRegion(100, 100));
  this->getmatrixaction = new SoGetMatrixAction(SbViewportRegion(100, 100));
  this->searchaction = new SoSearchAction;
}

SoScXMLSeekTarget::~SoScXMLSeekTarget(void)
{
  if (this->raypickaction) {
    delete this->raypickaction;
    this->raypickaction = NULL;
  }
  if (this->getmatrixaction) {
    delete this->getmatrixaction;
    this->getmatrixaction = NULL;
  }
  if (this->searchaction) {
    delete this->searchaction;
    this->searchaction = NULL;
  }
}


SbBool
SoScXMLSeekTarget::processOneEvent(const ScXMLEvent * event)
{
  assert(event);

  const SbName sessionid = this->getSessionId(event);
  if (sessionid == SbName::empty()) { return FALSE; }

  const SbName & eventname = event->getEventName();

  if (eventname == BEGIN()) {
    SeekData * data = static_cast<SeekData *>(this->getSessionData(sessionid, NewSeekData));
    assert(data);

    SoScXMLStateMachine * statemachine = this->getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoNode * sceneroot = statemachine->getSceneGraphRoot();
    if unlikely (!sceneroot) {
      SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                         "processing %s: state machine has no scene graph",
                         eventname.getString());
      return FALSE;
    }

    SoCamera * camera = statemachine->getActiveCamera();
    if unlikely (!camera) {
      SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                         "processing %s: state machine has no current camera",
                         eventname.getString());
      return FALSE;
    }

    SbVec2s screenpos;
    {
      SbVec2f mouseposn;
      if (!inherited::getEventSbVec2f(event, "mouseposition", mouseposn)) {
        return FALSE;
      }

      SbVec2s vpsize = statemachine->getViewportRegion().getViewportSizePixels();
      screenpos.setValue(short(vpsize[0] * mouseposn[0]), short(vpsize[1] * mouseposn[1]));
    }

    assert(this->raypickaction);
    this->raypickaction->setViewportRegion(statemachine->getViewportRegion());
    this->raypickaction->setPoint(screenpos);
    this->raypickaction->setRadius(2);
    this->raypickaction->apply(sceneroot);

    SoPickedPoint * picked = this->raypickaction->getPickedPoint();
    if (!picked) {
      statemachine->queueEvent(MISS());
      this->raypickaction->reset();
      return TRUE;
    }

    SbVec3f hitpoint = picked->getPoint();

    this->raypickaction->reset();

    data->camerastartposition = camera->position.getValue();
    data->camerastartorient = camera->orientation.getValue();

    data->seekstart = SbTime::getTimeOfDay();
    data->seektime = 2.0f;

    // move point to the camera coordinate system, consider
    // transformations before camera in the scene graph
    SbMatrix cameramatrix, camerainverse;
    PImpl::getCameraCoordinateSystem(this, camera, sceneroot,
                                    cameramatrix, camerainverse);
    camerainverse.multVecMatrix(hitpoint, hitpoint);

    float fd = 25;
    fd *= (hitpoint - camera->position.getValue()).length()/100.0f;
    camera->focalDistance = fd;

    SbVec3f dir = hitpoint - data->camerastartposition;
    dir.normalize();

    // find a rotation that rotates current camera direction into new
    // camera direction.
    SbVec3f olddir;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), olddir);
    SbRotation diffrot(olddir, dir);
    data->cameraendposition = hitpoint - fd * dir;
    data->cameraendorient = camera->orientation.getValue() * diffrot;

    data->seeking = TRUE;
  }

  else if (eventname == UPDATE()) {
    SeekData * data = static_cast<SeekData *>(this->getSessionData(sessionid, NewSeekData));
    assert(data);

    if (!data->seeking) {
      return TRUE;
    }

    SoScXMLStateMachine * statemachine = this->getSoStateMachine(event, sessionid);
    if (!statemachine) { return FALSE; }

    SoCamera * camera = statemachine->getActiveCamera();
    if unlikely (!camera) {
      SoDebugError::post("SoScXMLRotateTarget::processOneEvent",
                         "processing %s: state machine has no current camera",
                         eventname.getString());
      return FALSE;
    }

    SbTime currenttime = SbTime::getTimeOfDay();

    float t = float((currenttime.getValue() - data->seekstart.getValue()) / data->seektime);
    if (t >= 1.0f) t = 1.0f;

    SbBool end = (t == 1.0f);

    t = (float) ((1.0 - cos(M_PI*t)) * 0.5);

    camera->position = data->camerastartposition +
      (data->cameraendposition - data->camerastartposition) * t;

    camera->orientation =
      SbRotation::slerp(data->camerastartorient, data->cameraendorient, t);

    if (end) {
      statemachine->queueEvent(DONE());
    }
    return TRUE;
  }

  else if (eventname == END()) {
    this->freeSessionData(sessionid);
  }

  else {
    SoDebugError::post("SoScXMLSeekTarget::processOneEvent",
                       "processing %s: unknown event",
                       eventname.getString());
    return FALSE;
  }
  return TRUE;
}

#undef PRIVATE
