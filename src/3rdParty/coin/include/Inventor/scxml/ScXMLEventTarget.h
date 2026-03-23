#ifndef SCXML_EVENTTARGET_H
#define SCXML_EVENTTARGET_H

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

#include <Inventor/scxml/ScXMLObject.h>
#include <Inventor/tools/SbPimplPtr.h>

class ScXMLSendElt;
class ScXMLEventElt;
class ScXMLEvent;

class COIN_DLL_API ScXMLEventTarget : public ScXMLObject {
  typedef ScXMLObject inherited;
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLEventTarget)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLEventTarget(void);
  virtual ~ScXMLEventTarget(void);

  // event-target identification
  virtual void setEventTargetType(const char * targettype);
  const char * getEventTargetType(void) const { return this->targettype; }

  virtual void setEventTargetName(const char * targetname);
  const char * getEventTargetName(void) const { return this->targetname; }

  // event processing
  virtual const ScXMLEvent * getCurrentEvent(void) const { return this->currentevent; }

  virtual void queueEvent(const ScXMLEvent * event);
  virtual void queueEvent(const SbName & eventid);

  virtual SbBool processEventQueue(void);

  // inter-system communication
  virtual SbBool sendExternalEvent(const ScXMLSendElt * sendelt);
  virtual SbBool sendInternalEvent(const ScXMLEventElt * sendelt);

  virtual void queueInternalEvent(const ScXMLEvent * event);
  virtual void queueInternalEvent(const SbName & eventid);

protected:
  char * targetname;
  char * targettype;

  const ScXMLEvent * currentevent;
  SbBool isprocessingqueue;

  virtual void setCurrentEvent(const ScXMLEvent * event);
  virtual const ScXMLEvent * getNextEvent(void);
  virtual const ScXMLEvent * getNextInternalEvent(void);
  virtual const ScXMLEvent * getNextExternalEvent(void);

  virtual SbBool processOneEvent(const ScXMLEvent * event);

  static void registerEventTarget(ScXMLEventTarget * target, const char * sessionid = NULL);
  static void unregisterEventTarget(ScXMLEventTarget * target, const char * sessionid = NULL);

  static ScXMLEventTarget * getEventTarget(const char * targettype, const char * targetname, const char * sessionid = NULL);

private:
  ScXMLEventTarget(const ScXMLEventTarget & rhs); // N/A
  ScXMLEventTarget & operator = (const ScXMLEventTarget & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLEventTarget

#endif // !SCXML_EVENTTARGET_H
