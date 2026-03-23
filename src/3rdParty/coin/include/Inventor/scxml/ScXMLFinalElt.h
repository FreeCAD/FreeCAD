#ifndef COIN_SCXMLFINALELT_H
#define COIN_SCXMLFINALELT_H

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

#include <Inventor/scxml/ScXMLAbstractStateElt.h>
#include <Inventor/tools/SbPimplPtr.h>

class ScXMLOnEntryElt;
class ScXMLOnExitElt;

class COIN_DLL_API ScXMLFinalElt : public ScXMLAbstractStateElt {
  typedef ScXMLAbstractStateElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLFinalElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLFinalElt(void);
  virtual ~ScXMLFinalElt(void);

  virtual void setOnEntry(ScXMLOnEntryElt * onentry);
  virtual ScXMLOnEntryElt * getOnEntry(void) const;

  virtual void setOnExit(ScXMLOnExitElt * onexit);
  virtual ScXMLOnExitElt * getOnExit(void) const;

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

private:
  ScXMLFinalElt(const ScXMLFinalElt & rhs); // N/A
  ScXMLFinalElt & operator = (const ScXMLFinalElt & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLFinalElt

#endif // !COIN_SCXMLFINALELT_H
