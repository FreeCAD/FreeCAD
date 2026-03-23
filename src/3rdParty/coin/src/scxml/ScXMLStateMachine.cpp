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
  \class ScXMLStateMachine ScXMLStateMachine.h Inventor/scxml/ScXMLStateMachine.h
  \brief Manager for processing events and setting states in SCXML structures.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <Inventor/scxml/ScXMLStateMachine.h>

#ifdef _MSC_VER
#pragma warning(disable:4786) // symbol truncated
#endif // _MSC_VER

#include <cassert>
#include <cstring>
#include <algorithm>
#include <list>
#include <map>
#include <vector>

#include <memory>

#include <Inventor/errors/SoDebugError.h>

#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/ScXMLDocument.h>

#include <Inventor/scxml/ScXMLScxmlElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLStateElt.h>
#include <Inventor/scxml/ScXMLParallelElt.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>
#include <Inventor/scxml/ScXMLTransitionElt.h>
#include <Inventor/scxml/ScXMLAnchorElt.h>
#include <Inventor/scxml/ScXMLHistoryElt.h>
#include <Inventor/scxml/ScXMLLogElt.h>
#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLMinimumEvaluator.h>
#include <Inventor/scxml/ScXMLCoinEvaluator.h>
#include "scxml/ScXMLP.h"
#include "misc/SbHash.h"

// *************************************************************************

/*!
  \typedef void ScXMLStateMachineDeleteCB(void * userdata, ScXMLStateMachine * statemachine);

  This is the type definition for all callback functions to be invoked when a state machine
  is deleted.

  \typedef void ScXMLParallelStateChangeCB(void * userdata,
                                        ScXMLStateMachine * statemachine,
                                        int numstates,
                                        const char ** stateidentifiers,
                                        SbBool enterstate,
                                        SbBool success);

  This typedef is currently unused.

*/
									
// *************************************************************************

struct EventInfo {
  const ScXMLEvent * eventptr;
  SbBool deallocate;
};

class ScXMLStateMachine::PImpl {
public:
  PImpl(void)
    : pub(NULL),
      active(FALSE), finished(FALSE),
      name(SbName::empty()), sessionid(SbName::empty()),
      loglevel(3),
      description(NULL),
      evaluator(NULL)
  {
  }

  ~PImpl(void)
  {
    delete this->description;
    this->description = NULL;
  }

  ScXMLStateMachine * pub;

  SbBool active;
  SbBool finished;

  SbName name;
  SbName sessionid;
  int loglevel;
  ScXMLDocument * description;
  ScXMLEvaluator * evaluator;

  SbList<const char *> modules;

  mutable SbString varstring;

  // delete callbacks:
  typedef std::pair<ScXMLStateMachineDeleteCB *, void *> DeleteCBInfo;
  typedef std::vector<DeleteCBInfo> DeleteCallbackList;
  DeleteCallbackList deletecallbacklist;
  void invokeDeleteCallbacks(void);

  // state change callbacks:
  typedef std::pair<ScXMLStateChangeCB *, void *> StateChangeCBInfo;
  typedef std::vector<StateChangeCBInfo> StateChangeCallbackList;
  StateChangeCallbackList statechangecallbacklist;
  void invokeStateChangeCallbacks(const char * identifier, SbBool enterstate);

  std::unique_ptr<ScXMLTransitionElt> initializer;

  std::vector<ScXMLElt *> activestatelist;

  typedef std::pair<ScXMLElt *, ScXMLTransitionElt *> StateTransition;
  typedef std::vector<StateTransition> TransitionList;

  void findTransitions(TransitionList & transitions, ScXMLElt * stateobj, const ScXMLEvent * event);

  void exitState(ScXMLElt * object);
  void enterState(ScXMLElt * object);

  static long nextsessionid;
  static SbHash<const char *, ScXMLStateMachine *> * sessiondictionary;
}; // ScXMLStateMachine::PImpl

SbHash<const char *, ScXMLStateMachine *> * ScXMLStateMachine::PImpl::sessiondictionary = NULL;
long ScXMLStateMachine::PImpl::nextsessionid = 0;

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->pub)

SCXML_OBJECT_SOURCE(ScXMLStateMachine);

void
ScXMLStateMachine::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLStateMachine, ScXMLEventTarget, "ScXMLEventTarget");
  ScXMLStateMachine::PImpl::nextsessionid = 1;
  ScXMLStateMachine::PImpl::sessiondictionary =
    new SbHash<const char *, ScXMLStateMachine *>;
}

void
ScXMLStateMachine::cleanClass(void)
{
  ScXMLStateMachine::PImpl::nextsessionid = 0;
  delete ScXMLStateMachine::PImpl::sessiondictionary;
  ScXMLStateMachine::PImpl::sessiondictionary = NULL;
  ScXMLStateMachine::classTypeId = SoType::badType();
}

ScXMLStateMachine *
ScXMLStateMachine::getStateMachineForSessionId(const SbName & sessionid)
{
  if (sessionid == SbName::empty()) {
    return NULL;
  }
  const char * id = sessionid.getString();
  ScXMLStateMachine * statemachine = NULL;
  if (!ScXMLStateMachine::PImpl::sessiondictionary->get(id, statemachine)) {
    return NULL;
  }
  return statemachine;
}

ScXMLStateMachine::ScXMLStateMachine(void)
{
  PRIVATE(this)->pub = this;
  this->setEventTargetType("scxml");
  ScXMLP::lock();
  const long id = ScXMLStateMachine::PImpl::nextsessionid;
  ScXMLStateMachine::PImpl::nextsessionid += 1;
  ScXMLP::unlock();
  char sessionidstr[32];
  sprintf(sessionidstr, "x-coin-scxml-session%03ld", id);
  this->setSessionId(SbName(sessionidstr));
}

ScXMLStateMachine::~ScXMLStateMachine(void)
{
  PRIVATE(this)->invokeDeleteCallbacks();
  this->setSessionId(SbName::empty());
  this->setEnabledModulesList(SbList<const char *>());
}

// *************************************************************************

void
ScXMLStateMachine::setName(const SbName & nameobj)
{
  PRIVATE(this)->name = nameobj;
  this->setEventTargetName(nameobj.getString());
}

const SbName &
ScXMLStateMachine::getName(void) const
{
  return PRIVATE(this)->name;
}

void
ScXMLStateMachine::setDescription(ScXMLDocument * document)
{
  assert(!PRIVATE(this)->active);
  PRIVATE(this)->description = document;
  PRIVATE(this)->initializer.reset(NULL);
  PRIVATE(this)->active = FALSE;
  PRIVATE(this)->finished = FALSE;
  PRIVATE(this)->activestatelist.clear();

  // set up the correct evaluator and identify the modules that are enabled
  ScXMLElt * rootelt = document->getRoot();
  if (rootelt->isOfType(ScXMLScxmlElt::getClassTypeId())) {
    ScXMLScxmlElt * scxmlelt = static_cast<ScXMLScxmlElt *>(rootelt);
    const char * profile = scxmlelt->getProfileAttribute();
    SbName profilename = SbName::empty();
    if (profile) {
      profilename = profile;
    }
   
   SoType evaluatortype = ScXML::getEvaluatorTypeForProfile(profilename);
   if (evaluatortype != SoType::badType()) {
     assert(evaluatortype.canCreateInstance());
     ScXMLEvaluator * evaluator = 
       static_cast<ScXMLEvaluator *>(evaluatortype.createInstance());
     evaluator->setStateMachine(this);
     this->setEvaluator(evaluator);
   }
   else {
     SoDebugError::post("ScXMLStateMachine::setDescription",
                        "No available evaluator for profile '%s'.",
                        profilename.getString());
   }
  }
}

const ScXMLDocument *
ScXMLStateMachine::getDescription(void) const
{
  return PRIVATE(this)->description;
}

/*!
  This sets the session identifier for the state machine.  Using this is
  optional, since state machines are already assigned unique session ids at
  construction-time.
*/
void
ScXMLStateMachine::setSessionId(const SbName & sessionidarg)
{
  if (PRIVATE(this)->sessionid != SbName::empty()) {
    ScXMLStateMachine::PImpl::sessiondictionary->erase(PRIVATE(this)->sessionid.getString());
    PRIVATE(this)->sessionid = SbName::empty();
  }
  if (sessionidarg != SbName::empty()) {
    PRIVATE(this)->sessionid = sessionidarg;
    ScXMLStateMachine::PImpl::sessiondictionary->put(PRIVATE(this)->sessionid.getString(), this);
  }
}

/*!
  Returns the session identifier string for the state machine.
*/
const SbName &
ScXMLStateMachine::getSessionId(void) const
{
  return PRIVATE(this)->sessionid;
}

void
ScXMLStateMachine::setLogLevel(int loglevel)
{
  PRIVATE(this)->loglevel = loglevel;
}

int
ScXMLStateMachine::getLogLevel(void) const
{
  return PRIVATE(this)->loglevel;
}

// *************************************************************************

/*!
  Fire up the engine.
*/
void
ScXMLStateMachine::initialize(void)
{
  assert(!PRIVATE(this)->active);
  PRIVATE(this)->active = TRUE;
  PRIVATE(this)->finished = FALSE;
  PRIVATE(this)->activestatelist.clear();
  this->processOneEvent(NULL); // process the 'initial' initializer
  this->processEventQueue(); // process any pending events from the initial-processing
}

// *************************************************************************

/*!
  Processes one event.
  This is an internal inner event-loop utility function.
*/
SbBool
ScXMLStateMachine::processOneEvent(const ScXMLEvent * event)
{
  // this function seriously needs more structuring
  this->setCurrentEvent(event);

  if (0 /* debug */) {
    if (event)
      SoDebugError::postInfo("ScXMLStateMachine::processOneEvent",
                             "event: %s",
                             event->getEventName().getString());
    else
      SoDebugError::postInfo("ScXMLStateMachine::processOneEvent",
                             "NULL event");
  }

  if (0 /* debug */) {
    std::vector<ScXMLElt *>::iterator it =
      PRIVATE(this)->activestatelist.begin();
    while (it != PRIVATE(this)->activestatelist.end()) {
      SoDebugError::postInfo("ScXMLStateMachine::processOneEvent",
                             "active state: %s", (*it)->getXMLAttribute("id"));
      ++it;
    }
  }

  PImpl::TransitionList transitions;
  if (PRIVATE(this)->activestatelist.size() == 0) {
    if (PRIVATE(this)->initializer.get() == NULL) {
      PRIVATE(this)->initializer.reset(new ScXMLTransitionElt);
      if (PRIVATE(this)->description->getRoot()->getInitial()) {
        // FIXME: implement proper action
      } else {
        PRIVATE(this)->initializer->setTargetAttribute(PRIVATE(this)->description->getRoot()->getInitialAttribute());
      }
    }
    transitions.push_back(PImpl::StateTransition(static_cast<ScXMLElt*>(NULL), PRIVATE(this)->initializer.get()));
  } else {
    for (int c = 0; c < static_cast<int>(PRIVATE(this)->activestatelist.size()); ++c) {
      // containers are also active states and must be checked
      ScXMLElt * stateobj = PRIVATE(this)->activestatelist.at(c);
      while (stateobj != NULL) {
        PRIVATE(this)->findTransitions(transitions, stateobj, event);
        stateobj = stateobj->getContainer();
      }
    }
  }

  // no transitions means no changes, just return
  if (transitions.size() == 0) {
    if (this->getEvaluator())
      this->getEvaluator()->clearTemporaryVariables();
    this->setCurrentEvent(NULL);
    return FALSE;
  }

  // we handle all targetless transitions first
  {
    PImpl::TransitionList::iterator transit = transitions.begin();
    while (transit != transitions.end()) {
      if (transit->second->isTargetLess()) {
        transit->second->execute(this);
      }
      ++transit;
    }
  }

  // handle self-targeting transitions next (not sure this is the right
  // place, but it's not improbable either)...
  {
    PImpl::TransitionList::iterator transit = transitions.begin();
    while (transit != transitions.end()) {
      if (transit->second->isSelfReferencing()) {
        ScXMLElt * containerobj = transit->second->getContainer();
        /*ScXMLAbstractStateElt * targetobj = */PRIVATE(this)->description->getStateById(transit->second->getTargetAttribute());

        if (containerobj->isOfType(ScXMLStateElt::getClassTypeId())) {
          ScXMLStateElt * state = static_cast<ScXMLStateElt *>(containerobj);
          PRIVATE(this)->exitState(state);
          transit->second->execute(this);
          PRIVATE(this)->enterState(state);
        } else {
          transit->second->execute(this);
        }
      }
      ++transit;
    }
  }

  std::vector<ScXMLElt *> newstateslist;

  // handle those with other targets next
  PImpl::TransitionList::iterator transit = transitions.begin();
  while (transit != transitions.end()) {
    if (transit->second->isTargetLess() ||
        transit->second->isSelfReferencing()) {
      ++transit;
      continue;
    }

    const char * targetid = transit->second->getTargetAttribute();
    ScXMLElt * targetstate = PRIVATE(this)->description->getStateById(targetid);
    if (!targetstate) {
      SoDebugError::post("ScXMLStateMachine::processOneEvent",
                         "transition to unknown state '%s' failed.", targetid);
      ++transit;
      continue;
    }

    std::vector<ScXMLElt *> sourcestates;

    ScXMLElt * sourcestate = transit->first;
    if (sourcestate != NULL) { // ignore sourcestate NULL (initializer)
      // find all activestate object contained within source state
      std::vector<ScXMLElt *>::iterator activeit =
        PRIVATE(this)->activestatelist.begin();
      while (activeit != PRIVATE(this)->activestatelist.end()) {
        if ((*activeit)->isContainedIn(sourcestate)) {
          ScXMLElt * active = *activeit;
          sourcestates.push_back(active); // remember, to remove from activelist
          while (active != sourcestate) {
            //SoDebugError::post("process",
            //       "found activestate as substate of transition source");

            PRIVATE(this)->exitState(active); // exit substates of transition point
            active = active->getContainer();
            assert(active);
          }
        }
        ++activeit;
      }

      while (!targetstate->isContainedIn(sourcestate)) {
        //SoDebugError::postInfo("process", "going up to find common ancestor");
        PRIVATE(this)->exitState(sourcestate);
        sourcestate = sourcestate->getContainer();
      }
    }

    // executable content in the transition
    //SoDebugError::postInfo("process", "executing transition code");
    transit->second->execute(this);

    {
      std::vector<ScXMLElt *> path;
      //SoDebugError::postInfo("process", "finding target-path from sourcestate %p",
      //                       sourcestate);
      while (sourcestate != targetstate) {
        path.push_back(targetstate);
        targetstate = targetstate->getContainer();
      }
      targetstate = PRIVATE(this)->description->getStateById(targetid); // restore

      //SoDebugError::postInfo("process", "reversing downward path");
      std::reverse(path.begin(), path.end());

      std::vector<ScXMLElt *>::iterator pathit = path.begin();
      while (pathit != path.end()) {
        // SoDebugError::postInfo("process", "entering down towards target");
        PRIVATE(this)->enterState(*pathit);
        ++pathit;
      }
    }

    //SoDebugError::post("process", "list of source states to remove - %d",
    //                   sourcestates.size());
    // remove source states form activestates
    std::vector<ScXMLElt *>::iterator it = sourcestates.begin();
    while (it != sourcestates.end()) {
      std::vector<ScXMLElt *>::iterator findit =
        std::find(PRIVATE(this)->activestatelist.begin(),
                  PRIVATE(this)->activestatelist.end(), *it);
      if (findit != PRIVATE(this)->activestatelist.end()) {
        //SoDebugError::post("process", "erasing old activestate");
        PRIVATE(this)->activestatelist.erase(findit);
      } else {
        SoDebugError::post("ScXMLStateMachine::processOneEvent",
                           "source state not found in activestate list");
      }
      ++it;
    }

    // add targetstate to active states
    if (std::find(PRIVATE(this)->activestatelist.begin(), PRIVATE(this)->activestatelist.end(), targetstate) == PRIVATE(this)->activestatelist.end()) {
      newstateslist.push_back(targetstate);
    }

    ++transit;
  }

  //SoDebugError::postInfo("process", "new states to potentially append to activestates: %d",
  //                       newstateslist.size());


  // inspect target states for substates + <initial> children
  std::vector<ScXMLElt *>::iterator appendit = newstateslist.begin();
  while (appendit != newstateslist.end()) {
    SbBool pushedsubstate = FALSE;
    ScXMLElt * newstate = *appendit;

    SbBool settled = FALSE;
    while (!settled) {
      settled = TRUE;
      if (newstate->isOfType(ScXMLStateElt::getClassTypeId())) {
        ScXMLStateElt * state = static_cast<ScXMLStateElt *>(newstate);
        if (state->getNumStates() > 0 || state->getNumParallels() > 0) {
          do {
            const ScXMLInitialElt * initial = state->getInitial();
            if (!initial) {
              SoDebugError::post("ScXMLStateMachine::processOneEvent",
                                 "state '%s' has substates but no <initial>.",
                                 state->getIdAttribute());
              break;
            }
            ScXMLTransitionElt * transition = initial->getTransition();
            if (!transition) {
              SoDebugError::post("ScXMLStateMachine::processOneEvent",
                                 "state '%s' has <initial> without a transition.",
                                 state->getIdAttribute());
              break;
            }
            const char * targetid = transition->getTargetAttribute();
            if (!targetid) {
              SoDebugError::post("ScXMLStateMachine::processOneEvent",
                                 "state '%s' has <initial> with a targetless transition.",
                                 state->getIdAttribute());
              break;
            }
            ScXMLElt * targetobj = PRIVATE(this)->description->getStateById(targetid);
            if (!targetobj) {
              SoDebugError::post("ScXMLStateMachine::processOneEvent",
                                 "could not find target of state \"%s\"'s <initial> transition.",
                                 state->getIdAttribute());
              break;
            }

            if (targetobj->getContainer() != state) {
              SoDebugError::post("ScXMLStateMachine::processOneEvent",
                                 "target of state \"%s\"'s <initial> transition is not an immediate child of the state",
                                 state->getIdAttribute());
              break;
            }

            // perform executable code
            transition->execute(this);

            PRIVATE(this)->enterState(targetobj);

            newstate = targetobj;
            settled = FALSE; // need to loop over on new state one more time
          } while ( FALSE );

        } else {
          // no substates in this state - can be marked as the deepest active state
          if (state->getInitial()) { // just checking
            SoDebugError::post("ScXMLStateMachine::processOneEvent",
                               "state '%s' has <initial> but no sub-states.",
                               state->getIdAttribute());
          }

          PRIVATE(this)->activestatelist.push_back(state);
          pushedsubstate = TRUE; // need to avoid adding parent state before doing outer loop
          ++appendit;
        }
      } else {
        // non-ScXMLStateElt object (ScXMLFinalElt for instance)
        if (newstate != *appendit) {
          PRIVATE(this)->activestatelist.push_back(newstate);
          pushedsubstate = TRUE; // need to avoid adding parent state before doing outer loop
          ++appendit;
        }
      }
    }
    if (!pushedsubstate) {
      PRIVATE(this)->activestatelist.push_back(*appendit);
      ++appendit;
    }
  }

  // if all active states are <final> states of the root scxml element,
  // we should set 'finished' to true and stop/hinder event processing

  if (this->getEvaluator())
    this->getEvaluator()->clearTemporaryVariables();
  this->setCurrentEvent(NULL);
  return TRUE; // transitions have been taken
}

// *************************************************************************

/*!
  Returns whether the state machine is active or not.
*/
SbBool
ScXMLStateMachine::isActive(void) const
{
  return PRIVATE(this)->active;
}

/*!
  Returns whether the state machine has run to completion or not.
*/
SbBool
ScXMLStateMachine::isFinished(void) const
{
  return PRIVATE(this)->finished;
}

// *************************************************************************


/*!
  This method returns the current event during event processing, and \c NULL
  when not processing events.

  Event processing is in special cases done with \c NULL as the current event,
  as for instance during state machine initialization.
*/

// *************************************************************************

/*!
  Returns the number of active states in the state machine.  This number
  should currently be 1, but in the future, when &lt;parallel&gt; is implemented,
  it can be greater.
*/
int
ScXMLStateMachine::getNumActiveStates(void) const
{
  return static_cast<int>(PRIVATE(this)->activestatelist.size());
}

/*!
  Returns the Nth active state.
*/
const ScXMLElt *
ScXMLStateMachine::getActiveState(int idx) const
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->activestatelist.size()));
  return PRIVATE(this)->activestatelist.at(idx);
}

// *************************************************************************

/*!
  Registers a callback to be called when the state machine object is being
  deleted.
*/
void
ScXMLStateMachine::addDeleteCallback(ScXMLStateMachineDeleteCB * cb, void * userdata)
{
  PRIVATE(this)->deletecallbacklist.push_back(PImpl::DeleteCBInfo(cb, userdata));
}

/*!
  Unregisters a callback to be called when the state machine object is being
  deleted.
*/
void
ScXMLStateMachine::removeDeleteCallback(ScXMLStateMachineDeleteCB * cb, void * userdata)
{
  PImpl::DeleteCallbackList::iterator it =
    std::find(PRIVATE(this)->deletecallbacklist.begin(),
              PRIVATE(this)->deletecallbacklist.end(),
              PImpl::DeleteCBInfo(cb, userdata));
  if (it != PRIVATE(this)->deletecallbacklist.end()) {
    PRIVATE(this)->deletecallbacklist.erase(it);
  }
}

/*
  Invoke all the delete callbacks.
*/

void
ScXMLStateMachine::PImpl::invokeDeleteCallbacks(void)
{
  DeleteCallbackList::const_iterator it = this->deletecallbacklist.begin();
  while (it != this->deletecallbacklist.end()) {
    (it->first)(it->second, PUBLIC(this));
    ++it;
  }
}

// *************************************************************************

/*!
  \var ScXMLStateChangeCB

  This callback type is for notifying listeners on when the state machine
  enters and exits states that are tagged as "tasks" for logging purposes.
  This is what the Boolean "task" attribute in the state element sets up.

  The \a success argument is currently unsupported (will always be TRUE),
  but has been preemptively added to avoid a signature change later.

  \sa addStateChangeCallback
*/

/*!
  Registers a callback to be called when the state machine exits or enters
  a state.
*/
void
ScXMLStateMachine::addStateChangeCallback(ScXMLStateChangeCB * callback, void * userdata)
{
  PRIVATE(this)->statechangecallbacklist.push_back(PImpl::StateChangeCBInfo(callback, userdata));
}

/*!
  Unregisters a callback to be called when the state machine exits or enters
  a state.
*/
void
ScXMLStateMachine::removeStateChangeCallback(ScXMLStateChangeCB * callback, void * userdata)
{
  PImpl::StateChangeCallbackList::iterator findit =
    std::find(PRIVATE(this)->statechangecallbacklist.begin(),
              PRIVATE(this)->statechangecallbacklist.end(),
              PImpl::StateChangeCBInfo(callback, userdata));
  if (findit != PRIVATE(this)->statechangecallbacklist.end()) {
    PRIVATE(this)->statechangecallbacklist.erase(findit);
  }
}

/*
  Invoke all the state change callbacks.
*/
void
ScXMLStateMachine::PImpl::invokeStateChangeCallbacks(const char * identifier, SbBool enterstate)
{
  StateChangeCallbackList::const_iterator it =
    this->statechangecallbacklist.begin();
  while (it != this->statechangecallbacklist.end()) {
    (it->first)(it->second, PUBLIC(this), identifier, enterstate, TRUE);
    ++it;
  }
}

// *************************************************************************

void
ScXMLStateMachine::setVariable(const char * name, const char * COIN_UNUSED_ARG(value))
{
  assert(name);
  if (name[0] == '_') { // reserved system variables
    // core reserved names
    if (strcmp(name, "_name") == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    else if (strcmp(name, "_sessionID") == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    else if (strcmp(name, "_event") == 0 ||
             strncmp(name, "_event.", 7) == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    // data module
    else if (strcmp(name, "_data") == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    // fallthrough
    else {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' violates the reserved '_'-prefix "
                         "namespace for system variables.", name);
    }
  }

  else if (strncmp(name, "coin:", 5) == 0) {
    // coin profile
    if (strcmp(name, "coin:root") == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    else if (strcmp(name, "coin:camera") == 0) {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' is a reserved system variable.", name);
    }
    // fallthrough
    else {
      SoDebugError::post("ScXMLStateMachine::setVariable",
                         "Name '%s' violates the reserved 'coin:'-prefix "
                         "namespace for system variables.", name);
    }
  }
  else {
    // FIXME
  }
}

const char *
ScXMLStateMachine::getVariable(const char * name) const
{
  if (strcmp(name, "_sessionid") == 0) {
    PRIVATE(this)->varstring.sprintf("'%s'", PRIVATE(this)->sessionid.getString());
    return PRIVATE(this)->varstring.getString();
    // return PRIVATE(this)->sessionid.getString();
  }
  if (strcmp(name, "_name") == 0) {
    PRIVATE(this)->varstring.sprintf("'%s'", PRIVATE(this)->name.getString());
    return PRIVATE(this)->varstring.getString();
    // return PRIVATE(this)->name.getString();
  }
  return NULL;
}

// *************************************************************************

void
ScXMLStateMachine::PImpl::findTransitions(TransitionList & transitions, ScXMLElt * stateobj, const ScXMLEvent * event)
{
  assert(stateobj);

  if (stateobj->isOfType(ScXMLHistoryElt::getClassTypeId())) {
    ScXMLHistoryElt * history = static_cast<ScXMLHistoryElt *>(stateobj);
    if (history->getTransition() &&
        history->getTransition()->isEventMatch(event) &&
        history->getTransition()->evaluateCondition(PUBLIC(this))) {
      StateTransition transition(stateobj, history->getTransition());
      TransitionList::iterator findit =
        std::find(transitions.begin(), transitions.end(), transition);
      if (findit == transitions.end()) {
        transitions.push_back(transition);
      }
    }
  }
  else if (stateobj->isOfType(ScXMLInitialElt::getClassTypeId())) {
    ScXMLInitialElt * initial = static_cast<ScXMLInitialElt *>(stateobj);
    if (initial->getTransition() &&
        initial->getTransition()->isEventMatch(event) &&
        initial->getTransition()->evaluateCondition(PUBLIC(this))) {
      StateTransition transition(stateobj, initial->getTransition());
      TransitionList::iterator findit =
        std::find(transitions.begin(), transitions.end(), transition);
      if (findit == transitions.end()) {
        transitions.push_back(transition);
      }
    }
  }
  else if (stateobj->isOfType(ScXMLStateElt::getClassTypeId())) {
    ScXMLStateElt * state = static_cast<ScXMLStateElt *>(stateobj);
    for (int j = 0; j < state->getNumTransitions(); ++j) {
      if (state->getTransition(j)->isEventMatch(event) &&
          state->getTransition(j)->evaluateCondition(PUBLIC(this))) {
        StateTransition transition(stateobj, state->getTransition(j));
        TransitionList::iterator findit =
          std::find(transitions.begin(), transitions.end(), transition);
        if (findit == transitions.end()) {
          transitions.push_back(transition);
        }
      }
    }
  }
}

// *************************************************************************

void
ScXMLStateMachine::PImpl::exitState(ScXMLElt * object)
{
  assert(object);
  if (object->isOfType(ScXMLStateElt::getClassTypeId())) {
    ScXMLStateElt * state = static_cast<ScXMLStateElt *>(object);
    const char * id = state->getIdAttribute();
    this->invokeStateChangeCallbacks(id, FALSE);
    ScXMLOnExitElt * onexit = state->getOnExit();
    if (onexit) {
      onexit->execute(PUBLIC(this));
    }
  }
}

void
ScXMLStateMachine::PImpl::enterState(ScXMLElt * object)
{
  assert(object);

  if (object->isOfType(ScXMLFinalElt::getClassTypeId())) {
    // When entering a <final>, ParentID.done should be posted
    ScXMLFinalElt * final = static_cast<ScXMLFinalElt *>(object);
    const ScXMLElt * container = final->getContainer();
    assert(container);
    const char * id = container->getXMLAttribute("id");
    if (!id || id[0] == '\0') {
      if (container->isOfType(ScXMLDocument::getClassTypeId())) {
        // there is not ParentID to post a ParentID.done event in
        // this case. study SCXML state to see what to do?
        this->finished = TRUE;
        this->active = FALSE;
      } else {
        SoDebugError::post("ScXMLStateMachine::PImpl::enterState",
                           "<final> container has no id - can't post done-event");
      }
      return;
    }
    SbString eventstr;
    eventstr.sprintf("%s.done", id);
    PUBLIC(this)->queueInternalEvent(eventstr.getString());
  }
  else if (object->isOfType(ScXMLStateElt::getClassTypeId())) {
    ScXMLStateElt * state = static_cast<ScXMLStateElt *>(object);
    const char * id = state->getIdAttribute();
    this->invokeStateChangeCallbacks(id, TRUE);
    ScXMLOnEntryElt * onentry = state->getOnEntry();
    if (onentry) {
      onentry->execute(PUBLIC(this));
    }
  }
}

void
ScXMLStateMachine::setEvaluator(ScXMLEvaluator * evaluator)
{
  PRIVATE(this)->evaluator = evaluator;
}

ScXMLEvaluator *
ScXMLStateMachine::getEvaluator(void) const
{
  return PRIVATE(this)->evaluator;
}

SbBool
ScXMLStateMachine::isModuleEnabled(const char * modulename) const
{
  for (int i = 0; i < PRIVATE(this)->modules.getLength(); ++i) {
    if (strcmp(modulename, PRIVATE(this)->modules[i]) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}

int
ScXMLStateMachine::getNumEnabledModules(void) const
{
  return PRIVATE(this)->modules.getLength();
}

const char *
ScXMLStateMachine::getEnabledModuleName(int idx) const
{
  assert(idx >= 0 && idx < PRIVATE(this)->modules.getLength());
  return PRIVATE(this)->modules[idx];
}

void
ScXMLStateMachine::setEnabledModulesList(const SbList<const char *> & modulenames)
{
  int i;
  for (i = 0; i < PRIVATE(this)->modules.getLength(); ++i) {
    delete [] PRIVATE(this)->modules[i];
  }
  PRIVATE(this)->modules.truncate(0);
  for (i = 0; i < modulenames.getLength(); ++i) {
    char * dup = new char [ strlen(modulenames[i]) + 1 ];
    strcpy(dup, modulenames[i]);
    PRIVATE(this)->modules.append(dup);
  }
}

#undef PUBLIC
#undef PRIVATE
