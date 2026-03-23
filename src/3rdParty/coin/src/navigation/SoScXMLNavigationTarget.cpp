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

/*!
  \class SoScXMLNavigationTarget SoScXMLNavigationTarget.h Inventor/navigation/SoScXMLNavigationTarget.h
  \brief base class for navigation system SCXML event target services

  This class contains some common, useful, utility functions for implementing
  navigation system event targets.

  \ingroup coin_navigation
  \since Coin 3.1
*/

#include <Inventor/navigation/SoScXMLNavigationTarget.h>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <map>

#include <memory>

#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbRotation.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/scxml/SoScXMLEvent.h>
#include <Inventor/navigation/SoScXMLNavigation.h>
#include <Inventor/errors/SoDebugError.h>
#include "scxml/SbStringConvert.h"
#include "coindefs.h"

#include "base/coinString.h"


class SoScXMLNavigationTarget::PImpl {
public:
  PImpl(void) : sessiondatamap(NULL) { }

  typedef std::map<const char *, SoScXMLNavigationTarget::Data *> SessionDataMap;
  typedef std::pair<const char *, SoScXMLNavigationTarget::Data *> SessionDataEntry;

  SessionDataMap * sessiondatamap;
};

SoScXMLNavigationTarget::Data::~Data(void)
{
}

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_OBJECT_ABSTRACT_SOURCE(SoScXMLNavigationTarget);

void
SoScXMLNavigationTarget::initClass(void)
{
  SCXML_OBJECT_INIT_ABSTRACT_CLASS(SoScXMLNavigationTarget, ScXMLEventTarget, "ScXMLEventTarget");
}

void
SoScXMLNavigationTarget::cleanClass(void)
{
  SoScXMLNavigationTarget::classTypeId = SoType::badType();
}

SoScXMLNavigationTarget::SoScXMLNavigationTarget(void)
{
  PRIVATE(this)->sessiondatamap = new PImpl::SessionDataMap;
}

SoScXMLNavigationTarget::~SoScXMLNavigationTarget(void)
{
  PImpl::SessionDataMap::iterator it = PRIVATE(this)->sessiondatamap->begin();
  while (it != PRIVATE(this)->sessiondatamap->end()) {
    // we could warn about undeleted data here, but it could be caused by natural
    // causes like systems being taken down while the user is interacting, so we'll
    // let these go unwarned.
    delete it->second;
    ++it;
  }
  delete PRIVATE(this)->sessiondatamap;
  PRIVATE(this)->sessiondatamap = NULL;
}

/*!
  Returns the Data* base handle for the data structure that corresponds to the given
  \a sessionid.  The \a constructor argument is the function responsible for creating
  the Data-derived object if the session is new (or have been cleaned up earlier).
*/

SoScXMLNavigationTarget::Data *
SoScXMLNavigationTarget::getSessionData(SbName sessionid, NewDataFunc * constructor)
{
  Data * data = NULL;
  SoScXMLNavigation::syncLock();
  PImpl::SessionDataMap::iterator findit =
    PRIVATE(this)->sessiondatamap->find(sessionid.getString());
  if (findit == PRIVATE(this)->sessiondatamap->end()) {
    assert(constructor);
    data = (*constructor)();
    assert(data);
    PImpl::SessionDataEntry entry(sessionid.getString(), data);
    PRIVATE(this)->sessiondatamap->insert(entry);
  } else {
    data = findit->second;
  }
  SoScXMLNavigation::syncUnlock();
  return data;
}

/*!
  Cleans out the data structure that is mapped to the given \a sessionid.
*/
void
SoScXMLNavigationTarget::freeSessionData(SbName sessionid)
{
  SoScXMLNavigation::syncLock();
  PImpl::SessionDataMap::iterator findit =
    PRIVATE(this)->sessiondatamap->find(sessionid.getString());
  if (findit != PRIVATE(this)->sessiondatamap->end()) {
    Data * data = findit->second;
    PRIVATE(this)->sessiondatamap->erase(findit);
    delete data;
  }
  SoScXMLNavigation::syncUnlock();
}

/*!
  Returns the session id that is associated with the \a event.  If no
  session id is found, SbName::empty() is returned.
*/
SbName
SoScXMLNavigationTarget::getSessionId(const ScXMLEvent * event)
{
  assert(event);
  const char * sessionidstr = event->getAssociation("_sessionid");
  if unlikely (!sessionidstr) {
    SoDebugError::post("SoScXMLNavigationTarget::getSessionId",
                       "while processing %s: no _sessionid found.",
                           event->getEventName().getString());
    return SbName::empty();
  }
  if (sessionidstr[0] == '\'') { // unwrap string representation
    std::unique_ptr<char[]> buf(new char [strlen(sessionidstr)+1]);
    int res = sscanf(sessionidstr, "'%[^']'", buf.get());
    if (res == 1) {
      return SbName(buf.get());
    }
  }
  return SbName(sessionidstr);
}

/*!
  Returns the state machine that is associated with the given \a sessionid, or NULL
  if there are no state machines registered for the session id.
*/
ScXMLStateMachine *
SoScXMLNavigationTarget::getStateMachine(const ScXMLEvent * event, SbName sessionid)
{
  assert(event);
  ScXMLStateMachine * sm = ScXMLStateMachine::getStateMachineForSessionId(sessionid.getString());
  if (!sm) {
    SoDebugError::post("SoScXMLNavigationTarget::getSoStateMachine",
                       "while processing %s: no statemachine for session '%s'.",
                       event->getEventName().getString(), sessionid.getString());
    return NULL;
  }
  return sm;
}

/*!
  Returns the So- state machine that is associated with the given \a sessionid, or NULL
  if there are no state machines registered for the session id or if the state machine
  is not of SoScXMLStateMachine type.
*/
SoScXMLStateMachine *
SoScXMLNavigationTarget::getSoStateMachine(const ScXMLEvent * event, SbName sessionid)
{
  assert(event);
  ScXMLStateMachine * sm = SoScXMLNavigationTarget::getStateMachine(event, sessionid);
  if unlikely (!sm) {
    return NULL;
  }
  if unlikely (!sm->isOfType(SoScXMLStateMachine::getClassTypeId())) {
    SoDebugError::post("SoScXMLNavigationTarget::getSoStateMachine",
                       "while processing %s: statemachine not of So-type for session '%s'.",
                       event->getEventName().getString(), sessionid.getString());
    return NULL;
  }
  return static_cast<SoScXMLStateMachine *>(sm);
}

/*!
  Returns the current active camera, or NULL if there is no active camera set.
  If NULL is returned, error messages have been posted.
*/
SoCamera *
SoScXMLNavigationTarget::getActiveCamera(const ScXMLEvent * event, SbName sessionid)
{
  SoScXMLStateMachine * statemachine = SoScXMLNavigationTarget::getSoStateMachine(event, sessionid);
  if unlikely (!statemachine) { return NULL; }

  SoCamera * camera = statemachine->getActiveCamera();
  if unlikely (!camera) {
    SoDebugError::post("SoScXMLZoomTarget::processOneEvent",
                       "while processing %s: no current camera",
                       event->getEventName().getString());
    return NULL;
  }
  return camera;
}

/*!
  Returns TRUE if a double was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventDouble(const ScXMLEvent * event, const char * label, double & dbl_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventDouble",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  SbBool conversionOk;
  dbl_out = SbStringConvert::fromString<double>(valuestr,&conversionOk);
  if (!conversionOk) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventDouble",
                         "while processing %s: parameter '%s' contains invalid float data ('%s').",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventDouble",
                                  "while processing %s: parameter '%s' contains invalid float data ('%s').",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  return TRUE;
}

/*!
  Returns TRUE if a string was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventString(const ScXMLEvent * event, const char * label, SbString & str_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventString",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  if (valuestr[0] != '\'') {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventString",
                         "while processing %s: parameter '%s' contains invalid string data (\"%s\").",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventString",
                                  "while processing %s: parameter '%s' contains invalid string data (\"%s\").",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  else {
    std::unique_ptr<char[]> buf(new char [strlen(valuestr) + 1]);
    int res = sscanf(valuestr, "'%[^']'", buf.get());
    if (res == 1) {
      str_out = buf.get();
      return TRUE;
    } else {
      SoDebugError::postWarning("SoScXMLNavigationTarget::getEventString",
                                "while processing %s: parameter '%s' contains invalid string data (\"%s\").",
                                event->getEventName().getString(), label, valuestr);
      return FALSE;
    }
  }
  return TRUE;
}


/*!
  Returns TRUE if a boolean value was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventSbBool(const ScXMLEvent * event, const char * label, SbBool & bool_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbBool",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  SbBool conversionOk;
  bool_out = SbStringConvert::fromString<bool>(valuestr,&conversionOk);
  if (!conversionOk) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbBool",
                         "while processing %s: parameter '%s' contains invalid bool data ('%s').",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventSbBool",
                                  "while processing %s: parameter '%s' contains invalid bool data ('%s').",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  return TRUE;
}

/*!
  Returns TRUE if an SbVec2f was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventSbVec2f(const ScXMLEvent * event, const char * label, SbVec2f & vec_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbVec2f",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  SbBool conversionOk;
  vec_out = SbStringConvert::fromString<SbVec2f>(valuestr, &conversionOk);
  if (!conversionOk) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbVec2f",
                         "while processing %s: parameter '%s' contains invalid SbVec2f data ('%s').",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventSbVec2f",
                                  "while processing %s: parameter '%s' contains invalid SbVec2f data ('%s').",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  return TRUE;
}

/*!
  Returns TRUE if an SbVec3f was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventSbVec3f(const ScXMLEvent * event, const char * label, SbVec3f & vec_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbVec3f",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  SbBool conversionOk;
  vec_out = SbStringConvert::fromString<SbVec3f>(valuestr, &conversionOk);
  if (!conversionOk) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbVec3f",
                         "while processing %s: parameter '%s' contains invalid SbVec3f data ('%s').",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventSbVec3f",
                                  "while processing %s: parameter '%s' contains invalid SbVec3f data ('%s').",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  return TRUE;
}

/*!
  Returns TRUE if an SbRotation was delivered with the event under the label \a label,
  and FALSE otherwise. If \a required is TRUE, then errors will be given, otherwise
  this function will remain quiet.
*/
SbBool
SoScXMLNavigationTarget::getEventSbRotation(const ScXMLEvent * event, const char * label, SbRotation & rot_out, SbBool required)
{
  assert(event);
  const char * valuestr = event->getAssociation(label);
  if (!valuestr) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbRotation",
                         "while processing %s: required parameter '%s' not found.",
                         event->getEventName().getString(), label);
    }
    return FALSE;
  }
  SbBool conversionOk;
  rot_out = SbStringConvert::fromString<SbRotation>(valuestr, &conversionOk);
  if (!conversionOk) {
    if (required) {
      SoDebugError::post("SoScXMLNavigationTarget::getEventSbRotation",
                         "while processing %s: parameter '%s' contains invalid Sbrotation data ('%s').",
                         event->getEventName().getString(), label, valuestr);
    } else {
      if (COIN_DEBUG) {
        SoDebugError::postWarning("SoScXMLNavigationTarget::getEventSbRotation",
                                  "while processing %s: parameter '%s' contains invalid SbRotation data ('%s').",
                                  event->getEventName().getString(), label, valuestr);
      }
    }
    return FALSE;
  }
  return TRUE;
}

#undef PRIVATE
