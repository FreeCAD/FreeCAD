#ifndef COIN_SCXMLEVENT_H
#define COIN_SCXMLEVENT_H

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
#include <Inventor/tools/SbLazyPimplPtr.h>
#include <Inventor/lists/SbList.h>

class COIN_DLL_API ScXMLEvent : public ScXMLObject {
  typedef ScXMLObject inherited;
  SCXML_OBJECT_HEADER(ScXMLEvent)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLEvent(void);
  virtual ~ScXMLEvent(void);

  virtual void setEventName(const SbName & name);
  const SbName & getEventName(void) const { return this->name; }

  ScXMLEvent * clone(void) const;

  // associations
  void setAssociation(const char * key, const char * value);
  const char * getAssociation(const char * key) const;

  size_t getNumAssociations(void) const;
  size_t getAssociationKeys(SbList<const char *> & keys) const;

protected:
  SbName name;

  virtual void copyContents(const ScXMLEvent * rhs);

private:
  ScXMLEvent(const ScXMLEvent & rhs); // N/A
  ScXMLEvent & operator = (const ScXMLEvent & rhs); // N/A

  class PImpl;
  SbLazyPimplPtr<PImpl> pimpl;
}; // ScXMLEvent

#endif // !COIN_SCXMLEVENT_H
