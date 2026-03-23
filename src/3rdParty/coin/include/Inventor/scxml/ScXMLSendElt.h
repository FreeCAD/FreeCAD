#ifndef COIN_SCXMLSENDELT_H
#define COIN_SCXMLSENDELT_H

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

#include <Inventor/scxml/ScXMLExecutableElt.h>

#include <Inventor/tools/SbLazyPimplPtr.h>

class ScXMLEvent;
class ScXMLEventTarget;

class COIN_DLL_API ScXMLSendElt : public ScXMLExecutableElt {
  typedef ScXMLExecutableElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLSendElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLSendElt(void);
  virtual ~ScXMLSendElt(void);

  virtual void setEventAttribute(const char * event);
  virtual const char * getEventAttribute(void) const { return this->event; }

  virtual void setTargetAttribute(const char * event);
  virtual const char * getTargetAttribute(void) const { return this->target; }

  virtual void setTargetTypeAttribute(const char * event);
  virtual const char * getTargetTypeAttribute(void) const { return this->targettype; }

  virtual void setSendIDAttribute(const char * event);
  virtual const char * getSendIDAttribute(void) const { return this->sendid; }

  virtual void setDelayAttribute(const char * event);
  virtual const char * getDelayAttribute(void) const { return this->delay; }

  virtual void setNameListAttribute(const char * event);
  virtual const char * getNameListAttribute(void) const { return this->namelist; }

  virtual void setHintsAttribute(const char * event);
  virtual const char * getHintsAttribute(void) const { return this->hints; }

  virtual SbBool handleXMLAttributes(void);

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  ScXMLEvent * createEvent(ScXMLEventTarget * host) const;

  virtual void execute(ScXMLStateMachine * statemachine) const;

protected:
  char * event;
  char * target;
  char * targettype;
  char * sendid;
  char * delay;
  char * namelist;
  char * hints;

private:
  ScXMLSendElt(const ScXMLSendElt & rhs); // N/A
  ScXMLSendElt & operator = (const ScXMLSendElt & rhs); // N/A

  class PImpl;
  SbLazyPimplPtr<PImpl> pimpl;

}; // ScXMLSendElt

#endif // !COIN_SCXMLSENDELT_H

