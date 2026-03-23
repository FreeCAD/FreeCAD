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

#include <Inventor/scxml/ScXMLTransitionElt.h>

/*!
  \class ScXMLTransitionElt ScXMLTransitionElt.h Inventor/scxml/ScXMLTransitionElt.h
  \brief the \c &lt;transition&gt; SCXML element.

  The \c &lt;transition&gt; element is used for invoking executable code
  and transition from the current active states in a state machine to a
  new set of active states.

  By plural active states, it is meant that all the parent states of active
  states are also active at the same time, which is helpful when organizing
  common event triggered \c &lt;transition&gt; transitions.  Putting transitions
  in the root \c &lt;scxml&gt; element means that they are always active.

  You also have the concept of parallel states in SCXML. This is not \a yet
  supported in Coin SCXML.

  The following attributes are accepted:
  \li \c event
  \li \c cond
  \li \c target
  \li \c anchor - not supported

  The \c event attribute specifies the event that the transition will trigger
  for, if the \c &lt;transition&gt; element is in an active state.  The \a
  event attribute can be specified with wildcard matching using \c "*", but
  with the following restrictions:
  \li \c "*" must be at the end of the event name match.
  \li \c "*" must match full dot-separated words.

  Example 1: \c event="*"

  Example 2: \c event="error.*"

  The \c cond attribute can be used to specify additional conditions that must
  be met for the \c &lt;transition&gt; to be triggered.  The condition
  expression language depends on which profile the SCXML state machine uses.

  The \c "minimum" profile only specifies one function, which is the
  \c In({ID}) function.  In() evaluates to TRUE if the state machine is
  currently in the state with the given ID.

  The \c "x-coin" profile implements a number of expressions that evaluates to
  a truth value.  While in development, documenting the expression language
  seems like too much overhead, so that part will have to wait.  The source
  code can do the talking.  See Coin/src/scxml/eval-coin-tab.y.

  The \c target attribute specifies the target states to transition to.  If
  parallel states had been implemented, you would be able to specify multiple
  states in this attribute, but for now you can only specify one target state.
  If \c target is dropped, then the transition does not change what the active
  state is and will just execute the executable parts it contains.  If
  \c target is the \c id of the \c state the \c &lt;transition&gt; is in, the
  state will be left/exited and then reentered again.  For active substates,
  this means they will also be exited of course.

  The \c anchor attribute is not supported.

  \c &lt;transition&gt; elements can contain executable SCXML elements as
  XML children, meaning \c &lt;if&gt; /\c &lt;elseif&gt; / \c &lt;else&gt;,
  \c &lt;assign&gt;, \c &lt;send&gt;, \c &lt;event&gt;, \c &lt;log&gt;.

  \since Coin 3.0
  \sa ScXMLIfElt, ScXMLAssignElt, ScXMLSendElt, ScXMLEventElt, ScXMLLogElt
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLLogElt.h>
#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLAssignElt.h>
#include <Inventor/scxml/ScXMLEventElt.h>
#include <Inventor/scxml/ScXMLIfElt.h>
#include <Inventor/scxml/ScXMLScriptElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLEvaluator.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLTransitionEltReader : public ScXMLEltReader {
public:
  ScXMLTransitionEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLTransitionEltReader::ScXMLTransitionEltReader(void)
: ScXMLEltReader("transition")
{
}

ScXMLElt *
ScXMLTransitionEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLTransitionElt * transition = new ScXMLTransitionElt;
  transition->setContainer(container);
  this->setXMLAttributes(transition, xmlelt);

  // handle XML attributes
  if (unlikely(!transition->handleXMLAttributes())) {
    delete transition;
    return NULL;
  }

  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    if (strcmp(elementtype, "log") == 0) {
      // <log> - zero or more times
      ScXMLEltReader * logreader = ScXMLLogElt::getElementReader();
      assert(logreader);
      ScXMLElt * logobj = logreader->read(transition, element, doc, sm);
      if (unlikely(!logobj)) {
        delete transition;
        return NULL;
      }
      assert(logobj->isOfType(ScXMLLogElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLLogElt *>(logobj));
    }
    else if (strcmp(elementtype, "event") == 0) {
      // <event> - zero or more times
      ScXMLEltReader * eventreader = ScXMLEventElt::getElementReader();
      assert(eventreader);
      ScXMLElt * eventobj = eventreader->read(transition, element, doc, sm);
      if (unlikely(!eventobj)) {
        delete transition;
        return NULL;
      }
      assert(eventobj->isOfType(ScXMLEventElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLEventElt *>(eventobj));
    }
    else if (strcmp(elementtype, "assign") == 0) {
      // <assign> - zero or more times
      ScXMLEltReader * assignreader = ScXMLAssignElt::getElementReader();
      assert(assignreader);
      ScXMLElt * assignobj = assignreader->read(transition, element, doc, sm);
      if (unlikely(!assignobj)) {
        delete transition;
        return NULL;
      }
      assert(assignobj->isOfType(ScXMLAssignElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLAssignElt *>(assignobj));
    }
    else if (strcmp(elementtype, "send") == 0) {
      // <send> - zero or more times
      ScXMLEltReader * sendreader = ScXMLSendElt::getElementReader();
      assert(sendreader);
      ScXMLElt * sendobj = sendreader->read(transition, element, doc, sm);
      if (unlikely(!sendobj)) {
        delete transition;
        return NULL;
      }
      assert(sendobj->isOfType(ScXMLSendElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLSendElt *>(sendobj));
    }
    else if (strcmp(elementtype, "if") == 0) {
      // <if> - zero or more times
      ScXMLEltReader * ifreader = ScXMLIfElt::getElementReader();
      assert(ifreader);
      ScXMLElt * ifobj = ifreader->read(transition, element, doc, sm);
      if (unlikely(!ifobj)) {
        delete transition;
        return NULL;
      }
      assert(ifobj->isOfType(ScXMLIfElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLIfElt *>(ifobj));
    }
    else if (strcmp(elementtype, "script") == 0) {
      // <script> - zero or more times
      ScXMLEltReader * scriptreader = ScXMLScriptElt::getElementReader();
      assert(scriptreader);
      ScXMLElt * scriptobj = scriptreader->read(transition, element, doc, sm);
      if (unlikely(!scriptobj)) {
        delete transition;
        return NULL;
      }
      assert(scriptobj->isOfType(ScXMLScriptElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLScriptElt *>(scriptobj));
    }
    else if (strcmp(elementtype, "invoke") == 0) {
      // <invoke> - zero or more times
      ScXMLEltReader * invokereader = ScXMLInvokeElt::getElementReader();
      assert(invokereader);
      ScXMLElt * invokeobj = invokereader->read(transition, element, doc, sm);
      if (unlikely(!invokeobj)) {
        delete transition;
        return NULL;
      }
      assert(invokeobj->isOfType(ScXMLInvokeElt::getClassTypeId()));
      transition->addExecutable(static_cast<ScXMLInvokeElt *>(invokeobj));
    }
    else {
      SoDebugError::post("ScXMLTransitionEltReader::read",
                         "unexpected XML element '<%s>' found in <transition>",
                         elementtype);
      delete transition;
      return NULL;
    }
  }
  return transition;
} // read

// *************************************************************************

class ScXMLTransitionElt::PImpl {
public:
  PImpl(void) { }
  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->executablelist, ScXMLExecutableElt *);
  }

  std::vector<ScXMLExecutableElt *> executablelist;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLTransitionElt);

void
ScXMLTransitionElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLTransitionElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLTransitionElt, "transition", ScXMLTransitionEltReader);
}

void
ScXMLTransitionElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLTransitionElt);
  ScXMLTransitionElt::classTypeId = SoType::badType();
}

ScXMLTransitionElt::ScXMLTransitionElt(void)
: event(NULL),
  cond(NULL),
  target(NULL),
  anchor(NULL),
  needprefixmatching(FALSE),
  eventkey(SbName::empty()),
  targetkey(SbName::empty())
{
}

ScXMLTransitionElt::~ScXMLTransitionElt(void)
{
  this->setEventAttribute(NULL);
  this->setCondAttribute(NULL);
  this->setTargetAttribute(NULL);
  this->setAnchorAttribute(NULL);
}

void
ScXMLTransitionElt::setEventAttribute(const char * eventstr)
{
  if (this->event && this->event != this->getXMLAttribute("event")) {
    delete [] this->event;
  }
  this->event = NULL;
  this->eventkey = SbName::empty();
  this->needprefixmatching = FALSE;

  if (eventstr) {
    // You can do *-matching on event identifiers in transitions.
    // According to the spec., the allowed thing to do is to let the
    // event attribute _end_ with ".*", which should match zero or
    // more succeeding tokens - we need in other words not implement
    // any form of generic pattern matching here...
    const char * ptr = strstr(eventstr, ".*");
    if ((ptr != NULL) && (strlen(ptr) == 2)) {
      this->needprefixmatching = TRUE;
      // we'll chop off the pattern matching key and use the boolean
      const size_t len = strlen(eventstr) - 1;
      this->event = new char [ len ];
      strncpy(this->event, eventstr, len - 1);
      this->event[ len - 1 ] = '\0';
      this->eventkey = this->event;
    } else {
      this->event = new char [ strlen(eventstr) + 1 ];
      strcpy(this->event, eventstr);
      this->eventkey = this->event;
    }
  }
}

// const char * ScXMLTransition::getEventAttribute(void) const

void
ScXMLTransitionElt::setCondAttribute(const char * condstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->cond, "cond", condstr);
}

// const char * ScXMLTransition::getCondAttribute(void) const

void
ScXMLTransitionElt::setTargetAttribute(const char * targetstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->target, "target", targetstr);
}

// const char * ScXMLTransition::getTargetAttribute(void) const

void
ScXMLTransitionElt::setAnchorAttribute(const char * anchorstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->anchor, "anchor", anchorstr);
}

// const char * ScXMLTransition::getAnchorAttribute(void) const

SbBool
ScXMLTransitionElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) return FALSE;

  this->setEventAttribute(this->getXMLAttribute("event"));
  this->setCondAttribute(this->getXMLAttribute("cond"));
  this->setTargetAttribute(this->getXMLAttribute("target"));
  this->setAnchorAttribute(this->getXMLAttribute("anchor"));

  if (this->target && this->anchor) {
    SoDebugError::post("ScXMLTransitionElt::handleXMLAttributes",
                       "only one of 'target' and 'anchor' may be specified at once");

    return FALSE;
  }

  return TRUE;
}

void
ScXMLTransitionElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLTransitionElt * orig = coin_assert_cast<const ScXMLTransitionElt *>(rhs);
  this->setEventAttribute(orig->getEventAttribute());
  this->setCondAttribute(orig->getCondAttribute());
  this->setTargetAttribute(orig->getTargetAttribute());
  this->setAnchorAttribute(orig->getAnchorAttribute());
  int c = 0;
  for (c = 0; c < orig->getNumExecutables(); ++c) {
    ScXMLExecutableElt * executable =
      coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(c)->clone());
    this->addExecutable(executable);
  }
}

const ScXMLElt *
ScXMLTransitionElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "event") == 0) {
    if (this->event && strcmp(attrvalue, this->event) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "cond") == 0) {
    if (this->cond && strcmp(attrvalue, this->cond) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "target") == 0) {
    if (this->target && strcmp(attrvalue, this->target) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "anchor") == 0) {
    if (this->anchor && strcmp(attrvalue, this->anchor) == 0) {
      return this;
    }
  }
  return NULL;
}


/*!
  Returns whether this is a conditionless SCXML transition or not.

  A conditionless transition should always be taken.
*/

SbBool
ScXMLTransitionElt::isConditionLess(void) const
{
  if (!this->cond || this->cond[0] == '\0') return TRUE;
  return FALSE;
}

/*!
  Returns whether this is a transition without a target setting or not.

  When a targetless transition is taken, the state machine's state
  does not change. This differs from setting the target to its own
  state, which will cause the state machine to leave the state and
  reenter it again.
*/

SbBool
ScXMLTransitionElt::isTargetLess(void) const
{
  return (this->target == NULL);
}

/*!
  Returns whether this transition is referencing its container or not.

  If TRUE, this means you should end up in the same state as you
  started with when doing this transition.  Note however that as
  opposed to a targetless transition, this transition should actually
  exit the state and then reenter it again.  Executable content will
  be invoked after exiting the state, before reentering.
*/

SbBool
ScXMLTransitionElt::isSelfReferencing(void) const
{
  if (this->target != NULL && this->getContainer()) {
    const char * containerid = this->getContainer()->getXMLAttribute("id");
    if (strcmp(containerid, this->target) == 0) return TRUE;
  }
  return FALSE;
}

/*!
  This function returns TRUE if the transition matches the given \a eventobj
  object and FALSE otherwise.
*/
SbBool
ScXMLTransitionElt::isEventMatch(const ScXMLEvent * eventobj) const
{
  static const SbName globallkey("*");
  assert(eventobj);
  SbName eventid = eventobj->getEventName();

  if ((this->eventkey == SbName::empty()) ||
      (this->eventkey == globallkey)) return TRUE;

  if (!this->needprefixmatching) {
    return (eventid == this->eventkey);
  }

  if (this->eventkey == eventid) return TRUE;

  const size_t keylen = strlen(this->eventkey.getString());
  if (keylen < strlen(eventid.getString())) {
    if ((eventid.getString()[keylen] == '.') &&
        strncmp(this->eventkey.getString(), eventid.getString(), keylen) == 0) {
      return TRUE; // this->event matches up to eventobj token separator
    }
  }

  return FALSE;
}

// executable content

/*!
  This function uses the statemachine evaluator to evaluate its condition expression, and
  returns TRUE or FALSE based on the evaluation.

  On expression error, error events are emit'ed to the SCXML state machine.

  If the transition is conditionless, this function will return TRUE.
*/
SbBool
ScXMLTransitionElt::evaluateCondition(ScXMLStateMachine * statemachine)
{
  if (this->isConditionLess()) {
    return TRUE;
  }
  ScXMLEvaluator * evaluator = statemachine->getEvaluator();
  assert(evaluator);
  std::unique_ptr<ScXMLDataObj> cond(evaluator->evaluate(this->getCondAttribute()));
  if (cond.get()) {
    ScXMLDataObj * res = cond.get();
    if (cond->isOfType(ScXMLExprDataObj::getClassTypeId())) {
      ScXMLExprDataObj * exprobj = static_cast<ScXMLExprDataObj *>(cond.get());
      res = exprobj->evaluate(statemachine);
      if (!res) {
        if (COIN_DEBUG) {
          SoDebugError::post("ScXMLTransitionElt::evaluateCondition",
                             "expression evaluation failed (%s)\n", 
                             this->getCondAttribute());
        }
        statemachine->queueInternalEvent("error.InvalidCondExpr.Transition");
        return FALSE;
      }
    }
    if (res->isOfType(ScXMLBoolDataObj::getClassTypeId())) {
      ScXMLBoolDataObj * boolres = static_cast<ScXMLBoolDataObj *>(res);
      return boolres->getBool();
    }
    else {
      statemachine->queueInternalEvent("error.eval.minimum.Transition.NO_TRUTH_COND_EXPRESSION");
      return FALSE;
    }
  }
  else {
    statemachine->queueInternalEvent("error.eval.minimum.Transition.INVALID_COND_EXPRESSION");
    return FALSE;
  }
  return TRUE;
}

/*!
  Calls invoke on all the ScXMLInvoke children.
*/
void
ScXMLTransitionElt::execute(ScXMLStateMachine * statemachine) const
{
  std::vector<ScXMLExecutableElt *>::const_iterator it = PRIVATE(this)->executablelist.begin();
  while (it != PRIVATE(this)->executablelist.end()) {
    (*it)->execute(statemachine);
    ++it;
  }
}

SCXML_LIST_OBJECT_API_IMPL(ScXMLTransitionElt, ScXMLExecutableElt, PRIVATE(this)->executablelist, Executable, Executables);

#undef PRIVATE
