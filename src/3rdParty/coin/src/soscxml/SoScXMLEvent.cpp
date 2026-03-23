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

#include <Inventor/scxml/SoScXMLEvent.h>

#include <cassert>

#include <Inventor/SbString.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/scxml/ScXML.h>

// *************************************************************************

/*!
  \class SoScXMLEvent SoScXMLEvent.h Inventor/scxml/SoScXMLEvent.h
  \brief Adds an SoEvent holder to the ScXMLEvent base.

  This class is part of integrating the Coin types with the SCXML
  subsystem.  SoScXMLEvent objects are ScXMLEvents that wraps/passes
  an SoEvent to the SCXML state machine.

  \since Coin 3.0
  \ingroup coin_soscxml
*/

// *************************************************************************

namespace {

// copies all the values of any (builtin) event type
void
copyEventValues(SoEvent * lhs, const SoEvent * rhs)
{
  assert(lhs && rhs && lhs->getTypeId() == rhs->getTypeId());
  SoType eventtype = lhs->getTypeId();

  lhs->setTime(rhs->getTime());
  lhs->setPosition(rhs->getPosition());
  lhs->setShiftDown(rhs->wasShiftDown());
  lhs->setCtrlDown(rhs->wasCtrlDown());
  lhs->setAltDown(rhs->wasAltDown());

  static SoType buttoneventtype = SoButtonEvent::getClassTypeId();
  if (eventtype.isDerivedFrom(buttoneventtype)) {
    SoButtonEvent * blhs = static_cast<SoButtonEvent *>(lhs);
    const SoButtonEvent * brhs = static_cast<const SoButtonEvent *>(rhs);
    blhs->setState(brhs->getState());

    static SoType keyboardeventtype = SoKeyboardEvent::getClassTypeId();
    if (eventtype.isDerivedFrom(keyboardeventtype)) {
      SoKeyboardEvent * klhs = static_cast<SoKeyboardEvent *>(lhs);
      const SoKeyboardEvent * krhs = static_cast<const SoKeyboardEvent *>(rhs);
      klhs->setKey(krhs->getKey());
      klhs->setPrintableCharacter(krhs->getPrintableCharacter());
    }

    static SoType mousebuttoneventtype = SoMouseButtonEvent::getClassTypeId();
    if (eventtype.isDerivedFrom(mousebuttoneventtype)) {
      SoMouseButtonEvent * mlhs = static_cast<SoMouseButtonEvent *>(lhs);
      const SoMouseButtonEvent * mrhs = static_cast<const SoMouseButtonEvent *>(rhs);
      mlhs->setButton(mrhs->getButton());
    }

    static SoType spaceballbuttoneventtype = SoSpaceballButtonEvent::getClassTypeId();
    if (eventtype.isDerivedFrom(spaceballbuttoneventtype)) {
      SoSpaceballButtonEvent * slhs = static_cast<SoSpaceballButtonEvent *>(lhs);
      const SoSpaceballButtonEvent * srhs = static_cast<const SoSpaceballButtonEvent *>(rhs);
      slhs->setButton(srhs->getButton());
    }
  }
#if 0
  static SoType location2eventtype = SoLocation2Event::getClassTypeId();
  if (eventtype.isDerivedFrom(location2eventtype)) {
    SoLocation2Event * llhs = static_cast<SoLocation2Event *>(lhs);
    const SoLocation2Event * lrhs = static_cast<const SoLocation2Event *>(rhs);
    // nothing to do
  }
#endif
  static SoType motion3eventtype = SoMotion3Event::getClassTypeId();
  if (eventtype.isDerivedFrom(motion3eventtype)) {
    SoMotion3Event * mlhs = static_cast<SoMotion3Event *>(lhs);
    const SoMotion3Event * mrhs = static_cast<const SoMotion3Event *>(rhs);
    mlhs->setTranslation(mrhs->getTranslation());
    mlhs->setRotation(mrhs->getRotation());
  }
}

}

// *************************************************************************

SCXML_OBJECT_SOURCE(SoScXMLEvent);

void
SoScXMLEvent::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLEvent, ScXMLEvent, "ScXMLEvent");
}

void
SoScXMLEvent::cleanClass(void)
{
  SoScXMLEvent::classTypeId = SoType::badType();
}

SoScXMLEvent::SoScXMLEvent(void)
: soeventptr(NULL)
{
}

SoScXMLEvent::~SoScXMLEvent(void)
{
  delete this->soeventptr;
  this->soeventptr = NULL;
}

/*!
  This function stores a pointer to the originator SoEvent that
  caused the given event, if any.

  The event is duplicated when set, and can be deleted immediately
  by the caller.

  \sa setUpIdentifiers
*/
void
SoScXMLEvent::setSoEvent(const SoEvent * soevent)
{
  delete this->soeventptr;
  this->soeventptr = NULL;
  if (soevent) {
    SoEvent * newevent =
      static_cast<SoEvent *>(soevent->getTypeId().createInstance());
    copyEventValues(newevent, soevent);
    this->soeventptr = newevent;
  }
}

/*!
  Returns the pointer to the stored SoEvent or NULL if no event has
  been stored yet.

  \sa setSoEvent
*/
const SoEvent *
SoScXMLEvent::getSoEvent(void) const
{
  return this->soeventptr;
}

/*!
  This method updates the ScXMLEvent event class and event
  identifier strings based on the set SoEvent object.

  The identifiers will be cleared if no SoEvent object has been
  stored.

  \sa setSoEvent
*/
void
SoScXMLEvent::setUpIdentifier(void)
{
  if (this->soeventptr == NULL) {
    this->setEventName("");
    return;
  }

  const char * prefix = "sim.coin3d.coin";
  const char * eventclassname = "";
  SbString eventdata;
  if (this->soeventptr->isOfType(SoKeyboardEvent::getClassTypeId())) {
    eventclassname = "SoKeyboardEvent";
    const SoKeyboardEvent * mbevent = static_cast<const SoKeyboardEvent *>(this->soeventptr);
    SbString statestr;
    SoButtonEvent::enumToString(mbevent->getState(), statestr);
    SbString keystr;
    SoKeyboardEvent::enumToString(mbevent->getKey(), keystr);
    eventdata.sprintf("%s.%s", statestr.getString(), keystr.getString());
  }
  else if (this->soeventptr->isOfType(SoMouseButtonEvent::getClassTypeId())) {
    eventclassname = "SoMouseButtonEvent";
    const SoMouseButtonEvent * mbevent = static_cast<const SoMouseButtonEvent *>(this->soeventptr);
    SbString statestr;
    SoButtonEvent::enumToString(mbevent->getState(), statestr);
    SbString buttonstr;
    SoMouseButtonEvent::enumToString(mbevent->getButton(), buttonstr);
    eventdata.sprintf("%s.%s", statestr.getString(), buttonstr.getString());
  }
  else if (this->soeventptr->isOfType(SoSpaceballButtonEvent::getClassTypeId())) {
    eventclassname = "SoSpaceballButtonEvent";
    const SoSpaceballButtonEvent * sbevent =
      static_cast<const SoSpaceballButtonEvent *>(this->soeventptr);
    SbString statestr;
    SoButtonEvent::enumToString(sbevent->getState(), statestr);
    SbString buttonstr;
    SoSpaceballButtonEvent::enumToString(sbevent->getButton(), buttonstr);
    eventdata.sprintf("%s.%s", statestr.getString(), buttonstr.getString());
  }
  else if (this->soeventptr->isOfType(SoLocation2Event::getClassTypeId())) {
    eventclassname = "SoLocation2Event";
    // no eventdata to present
  }
  else if (this->soeventptr->isOfType(SoMotion3Event::getClassTypeId())) {
    eventclassname = "SoMotion3Event";
    // no eventdata to present
  }
  else {
    // fallback mechanism
    eventclassname = this->soeventptr->getTypeId().getName().getString();
  }

  SbString classidentifierstr;
  classidentifierstr.sprintf("%s.%s", prefix, eventclassname);
  if (eventdata.getLength() > 0) {
    SbString identifierstr;
    identifierstr.sprintf("%s.%s.%s", prefix, eventclassname, eventdata.getString());
    this->setEventName(identifierstr.getString());
  } else {
    this->setEventName(classidentifierstr.getString());
  }
}


void
SoScXMLEvent::copyContents(const ScXMLEvent * rhs)
{
  assert(rhs && rhs->isOfType(SoScXMLEvent::getClassTypeId()));
  inherited::copyContents(rhs);
  delete this->soeventptr;
  this->soeventptr = NULL;
  const SoScXMLEvent * sorhs = static_cast<const SoScXMLEvent *>(rhs);
  if (sorhs->soeventptr) {
    SoEvent * newevent =
      static_cast<SoEvent *>(sorhs->soeventptr->getTypeId().createInstance());
    copyEventValues(newevent, sorhs->soeventptr);
    this->soeventptr = newevent;
  }
}

// *************************************************************************
