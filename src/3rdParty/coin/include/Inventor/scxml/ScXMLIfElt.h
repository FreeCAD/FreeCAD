#ifndef COIN_SCXMLIFELT_H
#define COIN_SCXMLIFELT_H

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

#include <Inventor/tools/SbPimplPtr.h>

class ScXMLElseElt;
class ScXMLElseIfElt;

class COIN_DLL_API ScXMLIfElt : public ScXMLExecutableElt {
  typedef ScXMLExecutableElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLIfElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLIfElt(void);
  virtual ~ScXMLIfElt(void);

  virtual void setCondAttribute(const char * cond);
  virtual const char * getCondAttribute(void) const { return this->cond; }

  virtual SbBool handleXMLAttributes(void);

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  virtual int getNumElseIfs(void) const;
  virtual ScXMLElseIfElt * getElseIf(int idx) const;
  virtual void addElseIf(ScXMLElseIfElt * elseif);
  virtual void removeElseIf(ScXMLElseIfElt * elseif);
  virtual void clearAllElseIfs(void);

  virtual void setElse(ScXMLElseElt * elseelt);
  virtual ScXMLElseElt * getElse(void) const;

  virtual int getNumExecutables(const ScXMLExecutableElt * conditional) const;
  virtual ScXMLExecutableElt * getExecutable(const ScXMLExecutableElt * conditional, int idx) const;
  virtual void addExecutable(ScXMLExecutableElt * conditional, ScXMLExecutableElt * executable);
  virtual void removeExecutable(ScXMLExecutableElt * conditional, ScXMLExecutableElt * executable);
  virtual void clearAllExecutables(ScXMLExecutableElt * conditional);

  virtual void execute(ScXMLStateMachine * statemachine) const;

protected:
  char * cond;

private:
  ScXMLIfElt(const ScXMLIfElt & rhs); // N/A
  ScXMLIfElt & operator = (const ScXMLIfElt & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLIfElt

#endif // !COIN_SCXMLIFELT_H
