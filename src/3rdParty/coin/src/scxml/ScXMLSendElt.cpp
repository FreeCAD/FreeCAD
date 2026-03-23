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

#include <Inventor/scxml/ScXMLSendElt.h>

/*!
  \class ScXMLSendElt ScXMLSendElt.h Inventor/scxml/ScXMLSendElt.h
  \brief the \c &lt; send&gt; SCXML element.

  The \c &lt;send&gt; element is only useful in profiles that have
  an implementation of the External Communication Module.  Both
  the \c "minimum" and the \c "x-coin" profile have implementations,
  but the \c "minimum" profile does not implement The Data Module, so
  only plain events without associated data can be sent.

  The \c &lt;send&gt; element can take the following attributes:
  \li \c event
  \li \c target
  \li \c targettype
  \li \c sendid - not supported
  \li \c delay
  \li \c namelist
  \li \c hints - not supported

  The &lt;send&gt; element sends \b external events.  State machines have
  typically two event queues - one internal, and one external.  The internal
  event queue has full priority over the external event queue.
  State machines \e can send external events to itself, but it is more
  likely internal events are what you want.  To send internal events, see the
  \c &lt;event&gt; element implemented in ScXMLEventElt.
  To send an external event to the containing state machine, do not specify
  the \c target attribute.

  The \c event attribute specifies the event name for the event that is
  being sent.  Event names are dot-separated names like \c "error.InvalidExpr"
  or \c "x-coin-navigation.Rotate.BEGIN", and the event names are treated
  case-sensitive in the current implementation of Coin SCXML.  The \c event
  attribute is required.

  The \c targettype attribute specifies the type of the target system.
  Other SCXML state machines have the type \c "scxml".  Services related
  to Coin navigation systems have the targettype \c "x-coin-navigation".
  When sending external events to yourself, \c targettype can be omitted.

  The \c target attribute is the name of the target event system.
  If it is not specified, it means the event will be sent to the containing
  state machine.
  Coin navigation services are typically names \c "Rotate", but for other
  SCXML state machines, the \c &lt;scxml&gt; \c name attribute decides
  this.  That name is not unique though if multiple such names have been
  created, so \c target can also be the state machine \c _sessionid identifier.
  (FIXME: check if this is implemented)

  The \c sendid attribute is not supported.

  If \c delay is specified, it must take the form of \c "{number}s" or
  \c "{number}ms" giving you the option of specifying the delay in
  number of seconds or milliseconds before the event should be sent.
  For delayed events to be sent, the Coin delay queue processing must
  be performed. Most Coin users do that whether they know it or not. If
  \c delay is not specified, the event is passed to the target
  immediately. If a state machine is deleted, any events scheduled to
  be sent out in a delayed manner are going to be missing in action.
  The delay queue is a feature of the sending state machine, not the
  receiving one.

  The \c namelist attribute specifies which data variables to include
  with the event for the event target.  This attribute is only useful
  in profiles that implement the Data Module.  The \c "minimum" profile does
  not, but the \c "x-coin" profile does.  For the \c "x-coin" profile,
  you can specify \c &lt;data&gt; variables by addressing them with the
  \c _data.{ID} scheme, or you can specify temporary variables, either by
  using the \c coin:temp.{ID} scheme or have the prefix implicit by just
  specifying the {ID}.  The latter is the most useful method, since event targets
  do not want to know where in a data model a variable comes from, nor the
  name it had where it was contained.  With temporary variables, the values
  can get names that are intended for the event target instead of the name
  used at the source.

  The \c hints attribute is not supported.

  \sa ScXMLEventTarget, ScXMLEventElt, ScXMLDataElt, ScXMLAssignElt
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLEvaluator.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLSendEltReader : public ScXMLEltReader {
public:
  ScXMLSendEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLSendEltReader::ScXMLSendEltReader(void)
: ScXMLEltReader("send")
{
}

ScXMLElt *
ScXMLSendEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLSendElt * send = new ScXMLSendElt;
  send->setContainer(container);
  this->setXMLAttributes(send, xmlelt);

  // handle XML attributes
  if (unlikely(!send->handleXMLAttributes())) {
    delete send;
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

    SoDebugError::post("ScXMLSendEltReader::read",
                       "<send> contains unexpected <%s> element", elementtype);
    delete send;
    return NULL;
   }

  return send;
}

// *************************************************************************

class ScXMLSendElt::PImpl {
public:
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLSendElt);

void
ScXMLSendElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLSendElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLSendElt, "send", ScXMLSendEltReader);
}

void
ScXMLSendElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLSendElt);
  ScXMLSendElt::classTypeId = SoType::badType();
}

ScXMLSendElt::ScXMLSendElt(void)
: event(NULL),
  target(NULL),
  targettype(NULL),
  sendid(NULL),
  delay(NULL),
  namelist(NULL),
  hints(NULL)
{
}

ScXMLSendElt::~ScXMLSendElt(void)
{
  this->setEventAttribute(NULL);
  this->setTargetAttribute(NULL);
  this->setTargetTypeAttribute(NULL);
  this->setSendIDAttribute(NULL);
  this->setDelayAttribute(NULL);
  this->setNameListAttribute(NULL);
  this->setHintsAttribute(NULL);
}

void
ScXMLSendElt::setEventAttribute(const char * eventstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->event, "event", eventstr);
}

// const char * getEventAttribute(void) const;

void
ScXMLSendElt::setTargetAttribute(const char * targetstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->target, "target", targetstr);
}

// const char * getTargetAttribute(void) const;

void
ScXMLSendElt::setTargetTypeAttribute(const char * targettypestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->targettype, "targettype", targettypestr);
}

// const char * getTargetTypeAttribute(void) const;

void
ScXMLSendElt::setSendIDAttribute(const char * sendidstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->sendid, "sendid", sendidstr);
}

// const char * getSendIDAttribute(void) const;

void
ScXMLSendElt::setDelayAttribute(const char * delaystr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->delay, "delay", delaystr);
}

// const char * getDelayAttribute(void) const;

void
ScXMLSendElt::setNameListAttribute(const char * nameliststr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->namelist, "namelist", nameliststr);
}

// const char * getNameListAttribute(void) const;

void
ScXMLSendElt::setHintsAttribute(const char * hintsstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->hints, "hints", hintsstr);
}

// const char * getHintsAttribute(void) const;

SbBool
ScXMLSendElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setEventAttribute(this->getXMLAttribute("event"));
  this->setTargetAttribute(this->getXMLAttribute("target"));
  this->setTargetTypeAttribute(this->getXMLAttribute("targettype"));
  this->setSendIDAttribute(this->getXMLAttribute("sendid"));
  this->setDelayAttribute(this->getXMLAttribute("delay"));
  this->setNameListAttribute(this->getXMLAttribute("namelist"));
  this->setHintsAttribute(this->getXMLAttribute("hints"));

  return TRUE;
}

void
ScXMLSendElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLSendElt * orig = coin_assert_cast<const ScXMLSendElt *>(rhs);
  this->setEventAttribute(orig->getEventAttribute());
  this->setTargetAttribute(orig->getTargetAttribute());
  this->setTargetTypeAttribute(orig->getTargetTypeAttribute());
  this->setSendIDAttribute(orig->getSendIDAttribute());
  this->setDelayAttribute(orig->getDelayAttribute());
  this->setNameListAttribute(orig->getNameListAttribute());
  this->setHintsAttribute(orig->getHintsAttribute());
}

const ScXMLElt *
ScXMLSendElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "event") == 0) {
    if (this->target && strcmp(attrvalue, this->event) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "target") == 0) {
    if (this->target && strcmp(attrvalue, this->target) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "targettype") == 0) {
    if (this->targettype && strcmp(attrvalue, this->targettype) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "sendid") == 0) {
    if (this->sendid && strcmp(attrvalue, this->sendid) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "delay") == 0) {
    if (this->delay && strcmp(attrvalue, this->delay) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "namelist") == 0) {
    if (this->namelist && strcmp(attrvalue, this->namelist) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "hints") == 0) {
    if (this->hints && strcmp(attrvalue, this->hints) == 0) {
      return this;
    }
  }
  return NULL;
}

namespace {

void tokenize(const std::string & input, const std::string & delimiters, std::vector<std::string> & tokens, int count = -1)
{
  std::string::size_type last_pos = 0, pos = 0;
  while (TRUE) {
    --count;
    pos = input.find_first_of(delimiters, last_pos);
    if ((pos == std::string::npos) || (count == 0)) {
      tokens.push_back(input.substr(last_pos));
      break;
    } else {
      tokens.push_back(input.substr(last_pos, pos - last_pos));
      last_pos = pos + 1;
    }
  }
}

} // namespace

ScXMLEvent *
ScXMLSendElt::createEvent(ScXMLEventTarget * host) const
{
  ScXMLEvent * event = new ScXMLEvent;

  if (!this->event) {
    // for now, inline xml-content is not supported, so we emit error.fetch for
    // that case
    event->setEventName("error.Fetch");
    host->queueInternalEvent(event);
    delete event;
    return NULL;
  }

  event->setEventName(this->event);

  if (host->isOfType(ScXMLStateMachine::getClassTypeId())) {
    ScXMLStateMachine * sm = static_cast<ScXMLStateMachine *>(host);
    // standard associations
    event->setAssociation("_sessionid", sm->getSessionId().getString());
  }

  if (this->namelist && this->namelist[0] != '\0' &&
      host->isOfType(ScXMLStateMachine::getClassTypeId()))
  {
    ScXMLStateMachine * sm = static_cast<ScXMLStateMachine *>(host);

    // user-specific associations
    std::string nameliststr = this->namelist;
    std::vector<std::string> tokens;
    tokenize(this->namelist, " ", tokens);

    // FIXME: use evaluator
    for (size_t c = 0; c < tokens.size(); ++c) {
      std::string token = tokens[c];
      const char * value = sm->getVariable(token.c_str());
      if (value) {
        event->setAssociation(token.c_str(), value);
      } else {
        ScXMLEvaluator * evaluator = sm->getEvaluator();
        if (evaluator) {
          ScXMLDataObj * dataobj = evaluator->locate(token.c_str());
          if (dataobj &&
              dataobj->getTypeId().isDerivedFrom(ScXMLConstantDataObj::getClassTypeId())) {
            SbString str;
            ScXMLConstantDataObj * cobj = static_cast<ScXMLConstantDataObj *>(dataobj);
            cobj->convertToString(str);
            event->setAssociation(token.c_str(), str.getString());
          } else {
            host->queueInternalEvent("error.send.InvalidNameListObject");
          }
        }
      }
    }
  }

  return event;
}

void
ScXMLSendElt::execute(ScXMLStateMachine * statemachine) const
{
  inherited::execute(statemachine);
  statemachine->sendExternalEvent(this);
}

#undef PRIVATE
